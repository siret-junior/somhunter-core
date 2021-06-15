
/* This file is part of SOMHunter.
 *
 * Copyright (C) 2020 František Mejzlík <frankmejzlik@gmail.com>
 *                    Mirek Kratochvil <exa.exa@gmail.com>
 *                    Patrik Veselý <prtrikvesely@gmail.com>
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

#include "logger.h"
// ---
#include <filesystem>
#include <fstream>
#include <memory>
// ---
#include "common.h"
#include "eval-server-client.h"
#include "http.h"
#include "image-processor.h"
#include "query-types.h"
#include "user-context.h"

using namespace sh;

Logger::Logger(const EvalServerSettings& settings, const UserContext* p_user_ctx, EvalServerClient* p_eval_server)
    : _logger_settings{ settings },
      _p_user_ctx{ p_user_ctx },
      _p_eval_server{ p_eval_server },
      _last_interactions_submit_ts{ utils::timestamp() }
{
	// Make sure that log directories exist
	if (!utils::dir_create(_logger_settings.log_dir_user_actions + "/" + _p_user_ctx->get_username()) ||
	    !utils::dir_create(_logger_settings.log_dir_user_actions_summary + "/" + _p_user_ctx->get_username())) {
		std::string msg{ "Unable to create log directories!" };
		SHLOG_E(msg);
		throw std::runtime_error{ msg };
	}

	std::string filepath_actions{ get_actions_log_filepath() };
	std::string filepath_summary{ get_summary_log_filepath() };

	_actions_log_stream.open(filepath_actions);
	_summary_log_stream.open(filepath_summary);

	if (!_actions_log_stream.is_open() || !_summary_log_stream.is_open()) {
		std::string msg{ "Unable to create log directories!" };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	// Enable automatic flushing
	_summary_log_stream << std::unitbuf << "[" << std::endl;
	_actions_log_stream << std::unitbuf << "[" << std::endl;
}

Logger::~Logger()
{
	submit_interaction_logs_buffer();
	_summary_log_stream << "]" << std::endl;
	_actions_log_stream << "]" << std::endl;
}

void Logger::log_submit(const VideoFrame frame, bool submit_result)
{
	// Create the log
	push_action("submit", "OTHER", "submit", (submit_result ? "true" : "false"));
}

void Logger::log_query(const Query& query, const std::vector<VideoFrame>* p_targets) const
{
	// \todo Implement...
}

void Logger::submit_interaction_logs_buffer()
{
	// Send interaction logs
	if (!_interactions_buffer.empty()) {
		nlohmann::json a = nlohmann::json{ { "timestamp", double(utils::timestamp()) },
			                               { "events", std::move(_interactions_buffer) },
			                               { "type", "interaction" },
			                               { "teamId", int(_logger_settings.team_ID) },
			                               { "memberId", int(_logger_settings.member_ID) } };
		_interactions_buffer.clear();
	}

	// We always reset timer
	_last_interactions_submit_ts = utils::timestamp();
}

void Logger::log_rescore(const DatasetFrames& _dataset_frames, const ScoreModel& scores, const std::set<FrameId>& likes,
                         const UsedTools& used_tools, DisplayType /*disp_type*/, const std::vector<FrameId>& topn_imgs,
                         const std::string& sentence_query, const size_t topn_frames_per_video,
                         const size_t topn_frames_per_shot)
{
	/* ***
	 * Prepare the log compatible with the eval server. */
	UnixTimestamp ts{ utils::timestamp() };
	auto hash{ gen_action_hash(ts) };

	std::vector<nlohmann::json> results;
	results.reserve(topn_imgs.size());

	{
		size_t i{ 0 };
		for (auto&& img_ID : topn_imgs) {
			auto vf = _dataset_frames.get_frame(img_ID);

			// If LSC type of submit
			if (_logger_settings.submit_LSC_IDs) {
				results.push_back(nlohmann::json{ { "video", vf.LSC_id },
				                                  { "frame", int(vf.frame_number) },
				                                  { "score", double(scores[img_ID]) },
				                                  { "rank", int(i) } });

			}
			// Non-LSC submit
			else {
				results.push_back(nlohmann::json{ { "video", std::to_string(vf.video_ID + 1) },
				                                  { "frame", int(vf.frame_number) },
				                                  { "score", double(scores[img_ID]) },
				                                  { "rank", int(i) } });
			}

			++i;
		}
	}

	std::vector<nlohmann::json> used_cats;
	std::vector<nlohmann::json> used_types;
	std::vector<nlohmann::json> sort_types;

	std::string query_val(sentence_query + ";");
	std::stringstream filters_val_ss;

	// If Top KNN request
	if (used_tools.topknn_used) {
		// Mark this as KNN request
		query_val += "show_knn;";

		used_cats.push_back("image");
		used_types.push_back("globalFeatures");
		sort_types.push_back("globalFeatures");
	}
	// Else normal rescore
	else {
		// Just mark that this was NOT KNN request
		query_val += "normal_rescore;";

		if (used_tools.KWs_used) {
			used_cats.push_back("text");
			used_types.push_back("jointEmbedding");
			sort_types.push_back("jointEmbedding");
		}

		if (used_tools.bayes_used) {
			used_cats.push_back("image");
			used_types.push_back("feedbackModel");
			sort_types.push_back("feedbackModel");
		}

		if (used_tools.filters != nullptr) {
			used_cats.push_back("filter");
			used_types.push_back("lifelog");

			const Filters* fs{ used_tools.filters };

			// Write values value
			filters_val_ss << "h_from=" << size_t(fs->time.from) << ";"
			               << "h_to=" << size_t(fs->time.to) << ";"
			               << "days=";

			for (size_t i{ 0 }; i < 7; ++i) {
				auto&& d{ fs->days[i] };
				filters_val_ss << (d ? "1" : "0");
			}
			filters_val_ss << ";";
		}
	}

	query_val += "from_video_limit=";
	query_val += std::to_string(topn_frames_per_video);
	query_val += ";from_shot_limit=";
	query_val += std::to_string(topn_frames_per_shot);

	nlohmann::json result_json_arr = nlohmann::json(results);

	std::vector<nlohmann::json> values{ query_val };

	auto filters_val{ filters_val_ss.str() };
	if (!filters_val.empty()) {
		values.emplace_back(filters_val);
	}

	nlohmann::json values_arr = nlohmann::json(values);

	nlohmann::json top = nlohmann::json{ { "teamId", int(_logger_settings.team_ID) },
		                                 { "memberId", int(_logger_settings.member_ID) },
		                                 { "timestamp", double(utils::timestamp()) },
		                                 { "usedCategories", used_cats },
		                                 { "usedTypes", used_types },
		                                 { "sortType", sort_types },
		                                 { "resultSetAvailability", "top" },
		                                 { "type", "result" },
		                                 { "values", values_arr },
		                                 { "results", std::move(result_json_arr) } };

	/* ***
	 * Augment the log with extra data */
	top["hash"] = hash;
	top["serverTimestamp"] = _p_eval_server->get_server_ts();
	top["userToken"] = _p_eval_server->get_user_token();
	top["currentTask"] = _p_eval_server->get_current_task();
	top["likes"] = likes;

	// Write summary only when non-KNN "rescore"
	if (!used_tools.topknn_used) {
		write_summary(top, "rescore", { "likes" });
	}
}

void Logger::log_canvas_query(const std::vector<TemporalQuery>& temp_queries /*canvas_query*/,
                              const std::vector<VideoFrame>* p_targets)
{
	if (temp_queries.empty()) return;

	auto path{ _logger_settings.log_dir_user_actions_summary + "/"s + std::to_string(utils::timestamp()) + "/"s };

	if (temp_queries[0].canvas.is_save) {
		path = "saved-queries/"s + std::to_string(utils::timestamp()) + "/"s;
	}

	// One directory for each query
	std::filesystem::create_directories(path);

	std::string readable_timestamp{ utils::get_formated_timestamp("%d-%m-%Y_%H-%M-%S") };

	// Serialize the instance for possible debugging
	utils::serialize_to_file(temp_queries, path + "query-instance-" + readable_timestamp + ".bin");

	// Write log info
	std::ofstream o(path + "query-info" + readable_timestamp + ".json");
	if (!o) {
		std::string msg{ "Could not write a log file: " + path + "query_info.json" };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	std::vector<int> tars;
	if (p_targets != nullptr) {
		for (auto&& t : *p_targets) {
			tars.push_back((int)t.frame_ID);
		}
	}

	std::vector<json11::Json> arr;
	for (auto&& tq : temp_queries) arr.push_back(tq.canvas.to_JSON());
	json11::Json json{ arr };

	// Write bitmaps as JPEGs
	// For each temporal part

	for (size_t qidx{ 0 }; qidx < temp_queries.size(); ++qidx) {
		auto&& canvas_query{ temp_queries[qidx].canvas };
		for (size_t i{ 0 }; i < canvas_query.size(); ++i) {
			auto& sqo{ json.array_items()[qidx].array_items()[i].object_items() };

			const CanvasSubquery& subquery{ canvas_query[i] };

			if (std::holds_alternative<CanvasSubqueryBitmap>(subquery)) {
				const CanvasSubqueryBitmap& q{ std::get<CanvasSubqueryBitmap>(subquery) };
				auto& jpeg_filename{ sqo.at("bitmap_filename").string_value() };

				ImageManipulator::store_JPEG(path + jpeg_filename, q.data(), q.width_pixels(), q.height_pixels(), 100,
				                             q.num_channels());
			}
		}
	}
	json11::Json obj{ json11::Json::object{
		{ "targets", tars }, { "timestamp", readable_timestamp }, { "canvas_query", json } } };

	PrettyPrintOptions pretty_print_opts;
	pretty_print_opts.indent_increment = 4;
	// o << obj.dump();
	o << obj.pretty_print(pretty_print_opts);
}

void Logger::log_text_query_change(const std::string& text_query)
{
	static int64_t last_logged{ 0 };

	// If timeout should be handled here
	if (_logger_settings.apply_log_action_timeout) {
		// If no need to log now
		if (last_logged + _logger_settings.log_action_timeout > size_t(utils::timestamp())) return;
	}

	push_action("text_query_change", "TEXT", "jointEmbedding", text_query);
}
void Logger::log_like(const DatasetFrames& _dataset_frames, const std::set<FrameId>& /*likes*/,
                      DisplayType /*disp_type*/, FrameId frame_ID)
{
	auto vf = _dataset_frames.get_frame(frame_ID);

	std::stringstream data_ss;
	data_ss << "VId" << (vf.video_ID + 1) << ",FN" << vf.frame_number << ";FId" << frame_ID << ";like;";

	push_action("like", "IMAGE", "feedbackModel", data_ss.str());
}

void Logger::log_unlike(const DatasetFrames& _dataset_frames, const std::set<FrameId>& /*likes*/,
                        DisplayType /*disp_type*/, FrameId frame_ID)
{
	auto vf = _dataset_frames.get_frame(frame_ID);

	std::stringstream data_ss;
	data_ss << "VId" << (vf.video_ID + 1) << ",FN" << vf.frame_number << ";FId" << frame_ID << ";unlike;";

	push_action("unline", "IMAGE", "feedbackModel", data_ss.str());
}

void Logger::log_show_random_display(const DatasetFrames& /*frames*/, const std::vector<FrameId>& /*imgs*/)
{
	push_action("show_random_display", "BROWSING", "randomSelection", "random_display;");
}

void Logger::log_show_som_display(const DatasetFrames& /*frames*/, const std::vector<FrameId>& /*imgs*/)
{
	push_action("show_SOM_display", "BROWSING", "exploration", "som_display");
}

void Logger::log_show_som_relocation_display(const DatasetFrames& /*frames*/, const std::vector<FrameId>& /*imgs*/)
{
	push_action("show_SOM_relocation_display", "BROWSING", "exploration", "som_relocation_display");
}

void Logger::log_show_topn_display(const DatasetFrames& /*frames*/, const std::vector<FrameId>& /*imgs*/)
{
	push_action("show_top_scored_display", "BROWSING", "rankedList", "topn_display");
}

void Logger::log_show_topn_context_display(const DatasetFrames& /*frames*/, const std::vector<FrameId>& /*imgs*/)
{
	push_action("show_top_scored_context_display", "BROWSING", "rankedList", "topn_context_display;");
}

void Logger::log_show_topknn_display(const DatasetFrames& _dataset_frames, FrameId frame_ID,
                                     const std::vector<FrameId>& /*imgs*/)
{
	auto vf = _dataset_frames.get_frame(frame_ID);

	std::stringstream data_ss;
	data_ss << "VId" << (vf.video_ID + 1) << ",FN" << vf.frame_number << ";FId" << frame_ID << ";topknn_display;";

	push_action("show_nearest_neighbours_display", "IMAGE", "globalFeatures", data_ss.str());
}

void Logger::log_show_detail_display(const DatasetFrames& _dataset_frames, FrameId frame_ID)
{
	auto vf = _dataset_frames.get_frame(frame_ID);

	std::stringstream data_ss;
	data_ss << "VId" << (vf.video_ID + 1) << ",FN" << vf.frame_number << ";FId" << frame_ID << ";video_detail;";

	push_action("show_detail_display", "BROWSING", "videoSummary", data_ss.str());
}

void Logger::log_show_video_replay(const DatasetFrames& _dataset_frames, FrameId frame_ID, float delta)
{
	static int64_t last_replay_submit = 0;
	static FrameId last_frame_ID = IMAGE_ID_ERR_VAL;

	// If timeout should be handled here
	if (_logger_settings.apply_log_action_timeout) {
		// If no need to log now
		if (last_replay_submit + _logger_settings.log_action_timeout > size_t(utils::timestamp()) &&
		    frame_ID == last_frame_ID)
			return;
	}

	last_replay_submit = utils::timestamp();
	last_frame_ID = frame_ID;

	auto vf = _dataset_frames.get_frame(frame_ID);

	std::stringstream data_ss;
	data_ss << "VId" << (vf.video_ID + 1) << ",FN" << vf.frame_number << ";FId" << frame_ID << ";delta=" << delta
	        << ";replay;";

	push_action("replay_video", "BROWSING", "temporalContext", data_ss.str());
}

void Logger::log_scroll(const DatasetFrames& /*frames*/, DisplayType from_disp_type, float dirY)
{
	std::string ev_type("rankedList");
	std::string disp_type;

	switch (from_disp_type) {
		case DisplayType::DTopN:
			ev_type = "rankedList";
			disp_type = "topn_display";
			break;

		case DisplayType::DTopNContext:
			ev_type = "rankedList";
			disp_type = "topn_display_with_context";
			break;

		case DisplayType::DTopKNN:
			ev_type = "rankedList";
			disp_type = "topknn_display";
			break;

		case DisplayType::DVideoDetail:
			ev_type = "videoSummary";
			disp_type = "video_detail";
			break;

		default:
			return;
			break;
	}

	static int64_t last_logged = 0;
	static DisplayType last_disp_type = DisplayType::DNull;

	// If timeout should be handled here
	if (_logger_settings.apply_log_action_timeout) {
		// If no need to log now
		if (last_logged + _logger_settings.log_action_timeout > size_t(utils::timestamp()) &&
		    from_disp_type == last_disp_type)
			return;
	}

	last_logged = utils::timestamp();
	last_disp_type = from_disp_type;

	std::stringstream data_ss;
	data_ss << "scroll" << (dirY > 0 ? "Up" : "Down") << ";" << dirY << ";" << disp_type << ";";

	push_action("mouse_scroll", "BROWSING", ev_type, data_ss.str());
}

void Logger::log_reset_search() { push_action("reset_all", "BROWSING", "resetAll", ""); }

void Logger::poll()
{
	if (_last_interactions_submit_ts + _logger_settings.send_logs_to_server_period < size_t(utils::timestamp()))
		submit_interaction_logs_buffer();
}

LogHash Logger::gen_action_hash(UnixTimestamp ts)
{
	std::string hash{ _p_user_ctx->get_username() };
	hash.append("#");
	hash.append(std::to_string(ts));
	return hash;
}

LogHash Logger::push_action(const std::string& action_name, const std::string& cat, const std::string& type,
                            const std::string& value, std::initializer_list<std::string> summary_keys)
{
	/* ***
	 * Prepare the log compatible with the eval server. */
	UnixTimestamp ts{ utils::timestamp() };
	auto hash{ gen_action_hash(ts) };

	// clang-format off
	nlohmann::json log_JSON{
		{ "timestamp", ts }, 
		{ "category", cat }, 
		{ "type",  type }, 
		{ "value", value }
	};
	// clang-format on
	_interactions_buffer.emplace_back(log_JSON);

	/* ***
	 * Augment the log with extra data */
	log_JSON["hash"] = hash;
	log_JSON["serverTimestamp"] = _p_eval_server->get_server_ts();
	log_JSON["userToken"] = _p_eval_server->get_user_token();
	log_JSON["currentTask"] = _p_eval_server->get_current_task();

	write_action(log_JSON);
	write_summary(log_JSON, action_name, summary_keys);

	return hash;
}

std::string Logger::get_actions_log_filepath() const
{
	std::filesystem::path p{ _logger_settings.log_dir_user_actions + "/" + _p_user_ctx->get_username() };
	p = p / ("user-actions." + utils::get_formated_timestamp("%d-%m-%YT%H-%M-%S") + "." + _p_user_ctx->get_username() +
	         ".log");

	return p.string();
}

std::string Logger::get_summary_log_filepath() const
{
	std::filesystem::path p{ _logger_settings.log_dir_user_actions_summary + "/" + _p_user_ctx->get_username() };
	p = p / ("user-actions-summary." + utils::get_formated_timestamp("%d-%m-%YT%H-%M-%S") + "." +
	         _p_user_ctx->get_username() + ".log");

	return p.string();
}
