

#ifndef LOGGER_H_
#define LOGGER_H_

#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <filesystem>
#include <fstream>

#include <json11.hpp>

#include "common.h"
#include "utils.hpp"
#include "dataset-frames.h"
#include "scores.h"
#include "canvas-query-ranker.h"
#include "keyword-ranker.h"

using namespace json11;

namespace sh {


class EvalServerClient;
using ImageKeywords = KeywordRanker;

class Logger {

	
	EvalServerClient* _p_eval_server;

	std::vector<Json> backlog;
	std::int64_t last_submit_timestamp;
	

	const SubmitterConfig cfg;

#ifdef LOG_LOGS

	std::ofstream act_log;
	/** Just a shortcut so we have the unified log prefix. */
	auto& alog() { return act_log << utils::get_formated_timestamp("%H:%M:%S") << "\t" << utils::timestamp() << "\t"; }

#endif  // LOG_LOGS

#ifdef LOG_CURL_REQUESTS

	std::ofstream req_log;
	/** Just a shortcut so we have the unified log prefix. */
	auto& rlog() { return req_log << utils::get_formated_timestamp("%H:%M:%S") << "\t" << utils::timestamp() << "\t"; }

#endif  // LOG_CURL_REQUESTS

public:
	Logger() = delete;
	Logger(const SubmitterConfig& config, EvalServerClient* p_eval_server);

	// waits until the last thread submits
	~Logger() noexcept;

	// checks for terminated threads and logging timeout (call on each
	// frame)
	void poll();

	/** Called whenever we want to submit frame/shot into the server */
	bool submit_and_log_submit(const DatasetFrames& _dataset_frames, DisplayType disp_type, FrameId frame_ID);

	/** Called whenever we rescore (Bayes/LD) */
	void submit_and_log_rescore(const DatasetFrames& _dataset_frames, const ScoreModel& scores, const std::set<FrameId>& likes,
	                            const UsedTools& used_tools, DisplayType disp_type,
	                            const std::vector<FrameId>& topn_imgs, const std::string& sentence_query,
	                            const size_t topn_frames_per_video, const size_t topn_frames_per_shot);

	void log_text_query_change(const std::string& query_sentence);

	void log_canvas_query(const std::vector<TemporalQuery>& temp_queries, const std::vector<VideoFrame>* p_targets);

	void log_like(const DatasetFrames& _dataset_frames, const std::set<FrameId>& likes, DisplayType disp_type, FrameId frame_ID);

	void log_unlike(const DatasetFrames& _dataset_frames, const std::set<FrameId>& likes, DisplayType disp_type,
	                FrameId frame_ID);

	void log_show_som_display(const DatasetFrames& _dataset_frames, const std::vector<FrameId>& imgs);

	void log_show_som_relocation_display(const DatasetFrames& _dataset_frames, const std::vector<FrameId>& imgs);

	void log_show_random_display(const DatasetFrames& _dataset_frames, const std::vector<FrameId>& imgs);

	void log_show_topn_display(const DatasetFrames& _dataset_frames, const std::vector<FrameId>& imgs);

	void log_show_topn_context_display(const DatasetFrames& _dataset_frames, const std::vector<FrameId>& imgs);

	void log_show_topknn_display(const DatasetFrames& _dataset_frames, FrameId frame_ID, const std::vector<FrameId>& imgs);

	void log_show_detail_display(const DatasetFrames& _dataset_frames, FrameId frame_ID);

	void log_show_video_replay(const DatasetFrames& _dataset_frames, FrameId frame_ID, float delta);

	void log_scroll(const DatasetFrames& _dataset_frames, DisplayType from_disp_type, float dirY);

	void log_reset_search();

private:
	/** @mk We won't be callling this explicitly from the outside, will we?
	 */
	void send_backlog_only();

	/** Called by @ref submit_and_log_submit */
	void log_submit(const DatasetFrames& _dataset_frames, DisplayType disp_type, FrameId frame_ID);

	/** Called by @ref submit_and_log_rescore */
	void log_rerank(const DatasetFrames& _dataset_frames, DisplayType from_disp_type, const std::vector<FrameId>& topn_imgs);

	void push_event(const std::string& cat, const std::string& type, const std::string& value);
};

};  // namespace sh

#endif // LOGGER_H_