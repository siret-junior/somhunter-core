

#ifndef LOGGER_H_
#define LOGGER_H_

#include <filesystem>
#include <fstream>
#include <memory>
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

namespace sh
{
using ImageKeywords = KeywordRanker;
class EvalServerClient;
class UserContext;

/**
 * Class responsible for all the logging for the given user (each user have it's own `Logger`.)
 *
 * \see class UserContext
 */
class Logger
{
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
	void log_query(const Query& query, const std::vector<VideoFrame>* p_targets) const;
	void log_canvas_query(const std::vector<TemporalQuery>& temp_queries, const std::vector<VideoFrame>* p_targets);

	void log_rescore(const Query& prev_query, const Query& new_query, const std::vector<VideoFrame>& p_targets);

	/** Called whenever we rescore. */
	void log_results(const DatasetFrames& _dataset_frames, const ScoreModel& scores, const std::set<FrameId>& likes,
	                 const UsedTools& used_tools, DisplayType disp_type, const std::vector<FrameId>& topn_imgs,
	                 const std::string& sentence_query, const size_t topn_frames_per_video,
	                 const size_t topn_frames_per_shot);

	void log_text_query_change(const std::string& query_sentence);

	void log_like(FrameId frame_ID);

	void log_unlike(FrameId frame_ID);

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

private:
	void log_bothlike(FrameId frame_ID, const std::string& type);

	LogHash gen_action_hash(UnixTimestamp ts);
	LogHash push_action(const std::string& action_name, const std::string& cat, const std::string& type,
	                    const std::string& value, nlohmann::json&& our_log_JSON = {},
	                    std::initializer_list<std::string> summary_keys = {});

	/** Writes the log into the local file. */
	void write_result(const nlohmann::json& action_log)
	{
		// If first time output
		if (_first_result) {
			_first_result = false;
		} else {
			_results_log_stream << "," << std::endl;
		}
		_results_log_stream << action_log.dump(4);
	}

	/** Writes the log into the local file. */
	void write_action(const nlohmann::json& action_log)
	{
		// If first time output
		if (_first_result) {
			_first_result = false;
		} else {
			_actions_log_stream << "," << std::endl;
		}

		_actions_log_stream << action_log.dump(4);
	}

	/** Writes the log into the local file. */
	void write_summary(const nlohmann::json& log, const std::string& action_name,
	                   std::initializer_list<std::string> keys = {})
	{
		auto ts{ log["timestamp"].get<UnixTimestamp>() };
		auto hash{ log["hash"].get<std::string>() };

		_summary_log_stream << utils::get_formated_timestamp("%H:%M:%S", ts) << "\t" << hash << "\t" << action_name
		                    << "\t";

		for (auto& [key, value] : log.items()) {
			if (std::find(keys.begin(), keys.end(), key) == keys.end()) {
				continue;
			}
			_summary_log_stream << key << "=" << value << "\t";
		}
		_summary_log_stream << std::endl;
	}

	std::string get_results_log_filepath() const;
	std::string get_actions_log_filepath() const;
	std::string get_summary_log_filepath() const;

	// *** MEMBER VARIABLES ***
private:
	const EvalServerSettings _logger_settings;
	const UserContext* _p_user_ctx;
	EvalServerClient* _p_eval_server;

	std::vector<nlohmann::json> _interactions_buffer;
	UnixTimestamp _last_interactions_submit_ts;

	std::ofstream _results_log_stream;
	bool _first_result{ true };
	std::ofstream _summary_log_stream;
	std::ofstream _actions_log_stream;
	bool _first_actions{ true };
};

};  // namespace sh

#endif  // LOGGER_H_
