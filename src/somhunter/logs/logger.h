

#ifndef LOGGER_H_
#define LOGGER_H_

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
// ---
#include <json11.hpp>
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

	/** Called whenever we want to log submit frame/shot into the server. */
	void log_submit(const VideoFrame frame);

	/** Called whenever we rescore. */
	void log_rescore(const DatasetFrames& _dataset_frames, const ScoreModel& scores, const std::set<FrameId>& likes,
	                 const UsedTools& used_tools, DisplayType disp_type, const std::vector<FrameId>& topn_imgs,
	                 const std::string& sentence_query, const size_t topn_frames_per_video,
	                 const size_t topn_frames_per_shot);

	void log_text_query_change(const std::string& query_sentence);

	void log_canvas_query(const std::vector<TemporalQuery>& temp_queries, const std::vector<VideoFrame>* p_targets);

	void log_like(const DatasetFrames& _dataset_frames, const std::set<FrameId>& likes, DisplayType disp_type,
	              FrameId frame_ID);

	void log_unlike(const DatasetFrames& _dataset_frames, const std::set<FrameId>& likes, DisplayType disp_type,
	                FrameId frame_ID);

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
	void push_event(const std::string& cat, const std::string& type, const std::string& value);

	/** Just a shortcut so we have the unified log prefix. */
	auto& summary_log_stream()
	{
		return _summary_log_stream << utils::get_formated_timestamp("%H:%M:%S") << "\t" << utils::timestamp() << "\t";
	}

	/** Just a shortcut so we have the unified log prefix. */
	auto& debug_log_stream()
	{
		return _debug_log_stream << utils::get_formated_timestamp("%H:%M:%S") << "\t" << utils::timestamp() << "\t";
	}

	// *** MEMBER VARIABLES ***
private:
	const EvalServerSettings _logger_settings;
	const UserContext* _p_user_ctx;
	EvalServerClient* _p_eval_server;

	std::vector<Json> _interactions_buffer;
	UnixTimestamp _last_interactions_submit_ts;

	std::ofstream _summary_log_stream;
	std::ofstream _debug_log_stream;
};

};  // namespace sh

#endif  // LOGGER_H_
