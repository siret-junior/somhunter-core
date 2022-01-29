/* This file is part of SOMHunter.
 *
 * Copyright (C) 2021 Frantisek Mejzlik <frankmejzlik@protonmail.com>
 *                    Mirek Kratochvil <exa.exa@gmail.com>
 *                    Patrik Vesely <prtrikvesely@gmail.com>
 *
 * SOMHunter is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 2 of the License, or (at your option)
 * any later version.
 *
 * SOMHunter is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * SOMHunter. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
// ---
#include <nlohmann/json.hpp>
// ---
#include "canvas-query-ranker.h"
#include "common.h"
#include "dataset-frames.h"
#include "keyword-ranker.h"
#include "scores.h"
#include "utils.hpp"

using namespace json11;

namespace sh {
using ImageKeywords = KeywordRanker;
class EvalServerClient;
class UserContext;

/**
 * Class responsible for all the logging for the given user (each user have it's own `Logger`.)
 *
 * \see class UserContext
 */
class Logger {
	// *** METHODS ***
public:
	Logger() = delete;
	Logger(const EvalServerSettings& settings, const UserContext* p_user_ctx, EvalServerClient* p_eval_server);
	~Logger() noexcept;
	// ---

	/** Does periodic cleanup & log flush. */
	void poll();

	void log_search_context_switch(std::size_t dest_context_ID, size_t src_context_ID);

	/** Called whenever we want to log submit frame/shot into the server. */
	void log_submit(const VideoFrame frame, bool submit_result);

	/** Logs the provided query as a whole. */
	void log_query(const LogHash& hash, const Query& query) const;
	void log_canvas_query(const std::vector<TemporalQuery>& temp_queries, const std::vector<VideoFrame>* p_targets);

	void log_rescore(const Query& prev_query, const Query& new_query);

	/** Called whenever we rescore. */
	void log_results(const DatasetFrames& _dataset_frames, const ScoreModel& scores, const std::set<FrameId>& likes,
	                 const UsedTools& used_tools, DisplayType disp_type, const std::vector<FrameId>& topn_imgs,
	                 const std::string& sentence_query, const size_t topn_frames_per_video,
	                 const size_t topn_frames_per_shot, const std::vector<bool>& dataset_parts_filter = { true, true });

	void log_text_query_change(const std::string& query_sentence);
	void log_canvas_query_change();

	void log_like(FrameId frame_ID);
	void log_unlike(FrameId frame_ID);

	void log_bookmark(FrameId frame_ID);
	void log_unbookmark(FrameId frame_ID);

	void log_show_som_display(const DatasetFrames& _dataset_frames, const std::vector<FrameId>& imgs);

	void log_show_som_relocation_display(const DatasetFrames& _dataset_frames, const std::vector<FrameId>& imgs);

	void log_show_random_display(const DatasetFrames& _dataset_frames, const std::vector<FrameId>& imgs);

	void log_show_topn_display(const DatasetFrames& _dataset_frames, const std::vector<FrameId>& imgs);

	void log_show_topn_context_display(const DatasetFrames& _dataset_frames, const std::vector<FrameId>& imgs);

	void log_show_topknn_display(const DatasetFrames& _dataset_frames, FrameId frame_ID,
	                             const std::vector<FrameId>& imgs);

	void log_show_detail_display(const DatasetFrames& _dataset_frames, FrameId frame_ID);

	void log_show_video_replay(const DatasetFrames& _dataset_frames, FrameId frame_ID, float delta);

	void log_scroll(const DatasetFrames& _dataset_frames, DisplayType from_disp_type, float dirY);

	void log_reset_search();

	/** Sends the accumulated logs to the evaluation server (through `_p_eval_server`). */
	void submit_interaction_logs_buffer();

	DebugLogStreamPtrs get_debug_streams() {
		return DebugLogStreamPtrs{ _summary_streams.emplace_back(std::stringstream{}),
			                       _actions_streams.emplace_back(std::stringstream{}),
			                       _results_streams.emplace_back(std::stringstream{}) };
	};

	void clear_debug_streams() {
		_summary_streams.clear();
		_actions_streams.clear();
		_results_streams.clear();
	}

private:
	void log_bothlike(FrameId frame_ID, const std::string& type);

	LogHash gen_action_hash(UnixTimestamp ts);
	LogHash push_action(const std::string& action_name, const std::string& cat, const std::string& type,
	                    const std::string& value, nlohmann::json&& our_log_JSON = {},
	                    std::initializer_list<std::string> summary_keys = {});

	/** Writes the log into the local file. */
	void write_result(const nlohmann::json& action_log) {
		std::stringstream ss;

		// If first time output
		if (_first_result) {
			_first_result = false;
		} else {
			ss << "," << std::endl;
		}
		ss << action_log.dump(4);

		_results_log_stream << ss.str();
		for (auto&& s : _results_streams) {
			s << ss.str();
		}
	}

	/** Writes the log into the local file. */
	void write_action(const nlohmann::json& action_log) {
		std::stringstream ss;

		// If first time output
		if (_first_actions) {
			_first_actions = false;
		} else {
			ss << "," << std::endl;
		}

		ss << action_log.dump(4);

		_actions_log_stream << ss.str();
		for (auto&& s : _actions_streams) {
			s << ss.str();
		}
	}

	/** Writes the log into the local file. */
	void write_summary(const nlohmann::json& log, const std::string& action_name,
	                   std::initializer_list<std::string> keys = {}) {
		auto ts{ log["metadata"]["timestamp"].get<UnixTimestamp>() };
		auto hash{ log["metadata"]["hash"].get<std::string>() };

		std::stringstream ss;

		ss << utils::get_formated_timestamp("%H:%M:%S", ts) << "\t" << hash << "\t" << action_name << "\t";

		for (auto& [key, value] : log.items()) {
			if (std::find(keys.begin(), keys.end(), key) == keys.end()) {
				continue;
			}
			ss << key << "=" << value << "\t";
		}
		ss << std::endl;

		// Write to actual stream
		_summary_log_stream << ss.str();

		for (auto&& s : _summary_streams) {
			s << ss.str();
		}
	}
	std::string get_log_dir_queries() const;
	std::string get_results_log_filepath() const;
	std::string get_actions_log_filepath() const;
	std::string get_summary_log_filepath() const;

	std::lock_guard<std::mutex> get_exclusive_actions_lock() const { return std::lock_guard{ _push_action_mtx }; };

	// *** MEMBER VARIABLES ***
private:
	const EvalServerSettings _logger_settings;
	const UserContext* _p_user_ctx;
	EvalServerClient* _p_eval_server;

	std::vector<nlohmann::json> _interactions_buffer;
	UnixTimestamp _last_interactions_submit_ts;

	std::ofstream _results_log_stream;
	std::ofstream _summary_log_stream;
	std::ofstream _actions_log_stream;

	std::vector<std::stringstream> _summary_streams;
	std::vector<std::stringstream> _actions_streams;
	std::vector<std::stringstream> _results_streams;

	bool _first_result{ true };
	bool _first_actions{ true };

	mutable std::mutex _push_action_mtx;
};

};  // namespace sh

#endif  // LOGGER_H_
