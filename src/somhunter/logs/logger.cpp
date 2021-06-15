
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

Logger::Logger(const SubmitterConfig& settings, const UserContext* p_user_ctx, EvalServerClient* p_eval_server)
    : cfg{ settings },
      _p_user_ctx{ p_user_ctx },
      _p_eval_server{ p_eval_server },
      last_submit_timestamp{ utils::timestamp() }
{
	// Make sure the directory exists
	if (!(std::filesystem::exists(cfg.log_dir_user_actions))) {
		std::filesystem::create_directories(cfg.log_dir_user_actions);
	}

	std::string filepath{ cfg.log_dir_user_actions + "/actions_" + utils::get_formated_timestamp("%d-%m-%Y_%H-%M-%S") +
		                  ".log" };

	act_log.open(filepath, std::ios::out);
	if (!act_log.is_open()) {
		std::string msg{ "Error openning file: " + filepath };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	// Enable automatic flushing
	act_log << std::unitbuf;
}

Logger::~Logger() { send_backlog_only(); }

void Logger::log_submit(const UserContext& /*user_ctx*/, const VideoFrame frame)
{
	//// clang-format off
	// nlohmann::json body{
	//	{ "username", _settings.username },
	//	{ "password", _settings.password }
	//};
	//// clang-format on

	/*alog() << "submit_frame\t"
	       << "\t"
	       << "frame_ID=" << frame.frame_ID << "\t"
	       << "disp_type=" << disp_type_to_str(disp_type) << std::endl;*/
}

void Logger::log_rerank(const DatasetFrames& /*frames*/, DisplayType /*from_disp_type*/,
                        const std::vector<FrameId>& /*topn_imgs*/)
{
	// @todo we can log them for our purporses
}

void Logger::send_backlog_only()
{
	// Send interaction logs
	if (!backlog.empty()) {
		Json a = Json::object{ { "timestamp", double(utils::timestamp()) },
			                   { "events", std::move(backlog) },
			                   { "type", "interaction" },
			                   { "teamId", int(cfg.team_ID) },
			                   { "memberId", int(cfg.member_ID) } };
		backlog.clear();
		// start_poster(_p_eval_server->get_interaction_URL(), ""s, a.dump());
	}

	// We always reset timer
	last_submit_timestamp = utils::timestamp();
}

void Logger::submit_and_log_rescore(const DatasetFrames& _dataset_frames, const ScoreModel& scores,
                                    const std::set<FrameId>& likes, const UsedTools& used_tools,
                                    DisplayType /*disp_type*/, const std::vector<FrameId>& topn_imgs,
                                    const std::string& sentence_query, const size_t topn_frames_per_video,
                                    const size_t topn_frames_per_shot)
{
	std::vector<Json> results;
	results.reserve(topn_imgs.size());

	{
		size_t i{ 0 };
		for (auto&& img_ID : topn_imgs) {
			auto vf = _dataset_frames.get_frame(img_ID);
			// LSC submit
#ifdef SUBMIT_FILENAME_ID

			results.push_back(Json::object{ { "video", vf.LSC_id },
			                                { "frame", int(vf.frame_number) },
			                                { "score", double(scores[img_ID]) },
			                                { "rank", int(i) } });

			// Non-LSC submit
#else

			results.push_back(Json::object{ { "video", std::to_string(vf.video_ID + 1) },
			                                { "frame", int(vf.frame_number) },
			                                { "score", double(scores[img_ID]) },
			                                { "rank", int(i) } });

#endif  // SUBMIT_FILENAME_ID

			++i;
		}
	}

	std::vector<Json> used_cats;
	std::vector<Json> used_types;
	std::vector<Json> sort_types;

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

	Json result_json_arr = Json::array(results);

	std::vector<Json> values{ query_val };

	auto filters_val{ filters_val_ss.str() };
	if (!filters_val.empty()) {
		values.emplace_back(filters_val);
	}

	Json values_arr = Json::array(values);

	Json top = Json::object{ { "teamId", int(cfg.team_ID) },
		                     { "memberId", int(cfg.member_ID) },
		                     { "timestamp", double(utils::timestamp()) },
		                     { "usedCategories", used_cats },
		                     { "usedTypes", used_types },
		                     { "sortType", sort_types },
		                     { "resultSetAvailability", "top" },
		                     { "type", "result" },
		                     { "values", values_arr },
		                     { "results", std::move(result_json_arr) } };

	// start_poster(_p_eval_server->get_rerank_URL(), "", top.dump());

#ifdef LOG_LOGS

	// KNN is not rescore, so we ignore it
	if (!used_tools.topknn_used) {
		auto& ss{ alog() };

		ss << "rescore\t"
		   << "text_query=\"" << sentence_query << "\"\t"
		   << "likes=[";

		{
			size_t ii{ 0 };
			for (auto&& l : likes) {
				if (ii > 0) ss << ",";

				ss << l;
				++ii;
			}
		}

		ss << "]\t" << std::endl;
	}

#endif  // LOG_LOGS
}

void Logger::log_canvas_query(const std::vector<TemporalQuery>& temp_queries /*canvas_query*/,
                              const std::vector<VideoFrame>* p_targets)
{
	if (temp_queries.empty()) return;

	auto path{ cfg.log_dir_user_actions_summary + "/"s + std::to_string(utils::timestamp()) + "/"s };

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

	Json::array tars;
	if (p_targets != nullptr) {
		for (auto&& t : *p_targets) {
			tars.push_back((int)t.frame_ID);
		}
	}

	std::vector<json11::Json> arr;
	for (auto&& tq : temp_queries) arr.push_back(tq.canvas.to_JSON());
	Json json{ arr };

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
	Json obj{ Json::object{ { "targets", tars }, { "timestamp", readable_timestamp }, { "canvas_query", json } } };

	PrettyPrintOptions pretty_print_opts;
	pretty_print_opts.indent_increment = 4;
	// o << obj.dump();
	o << obj.pretty_print(pretty_print_opts);
}

void Logger::log_text_query_change(const std::string& text_query)
{
	static int64_t last_logged{ 0 };

	// If timeout should be handled here
	if (cfg.apply_log_action_timeout) {
		// If no need to log now
		if (last_logged + cfg.log_action_timeout > size_t(utils::timestamp())) return;
	}

#ifdef LOG_LOGS

	alog() << "text_query\t"
	       << "text_query=\"" << text_query << "\"" << std::endl;

#endif  // LOG_LOGS

	push_event("text", "jointEmbedding", text_query);
}
void Logger::log_like(const DatasetFrames& _dataset_frames, const std::set<FrameId>& likes, DisplayType /*disp_type*/,
                      FrameId frame_ID)
{
	auto vf = _dataset_frames.get_frame(frame_ID);

#ifdef LOG_LOGS

	alog() << "like\t"
	       << "frame_ID=" << frame_ID << "\t"
	       << "liked=" << (likes.count(frame_ID) == 1 ? "true" : "false") << "\t" << std::endl;

#endif  // LOG_LOGS

	std::stringstream data_ss;
	data_ss << "VId" << (vf.video_ID + 1) << ",FN" << vf.frame_number << ";FId" << frame_ID << ";like;";

	push_event("image", "feedbackModel", data_ss.str());
}

void Logger::log_unlike(const DatasetFrames& _dataset_frames, const std::set<FrameId>& likes, DisplayType /*disp_type*/,
                        FrameId frame_ID)
{
	auto vf = _dataset_frames.get_frame(frame_ID);

#ifdef LOG_LOGS

	alog() << "unlike\t"
	       << "frame_ID=" << frame_ID << "\t"
	       << "liked=" << (likes.count(frame_ID) == 1 ? "true" : "false") << "\t" << std::endl;

#endif  // LOG_LOGS

	std::stringstream data_ss;
	data_ss << "VId" << (vf.video_ID + 1) << ",FN" << vf.frame_number << ";FId" << frame_ID << ";unlike;";

	push_event("image", "feedbackModel", data_ss.str());
}

void Logger::log_show_random_display(const DatasetFrames& /*frames*/, const std::vector<FrameId>& /*imgs*/)
{
	push_event("browsing", "randomSelection", "random_display;");

#ifdef LOG_LOGS

	alog() << "show_random_display\t" << std::endl;

#endif  // LOG_LOGS
}

void Logger::log_show_som_display(const DatasetFrames& /*frames*/, const std::vector<FrameId>& /*imgs*/)
{
	push_event("browsing", "exploration", "som_display");

#ifdef LOG_LOGS

	alog() << "show_SOM_display\t" << std::endl;

#endif  // LOG_LOGS
}

void Logger::log_show_som_relocation_display(const DatasetFrames& /*frames*/, const std::vector<FrameId>& /*imgs*/)
{
	push_event("browsing", "exploration", "som_relocation_display");

#ifdef LOG_LOGS

	alog() << "show_SOM_relocation_display\t" << std::endl;

#endif  // LOG_LOGS
}

void Logger::log_show_topn_display(const DatasetFrames& /*frames*/, const std::vector<FrameId>& /*imgs*/)
{
	push_event("browsing", "rankedList", "topn_display");

#ifdef LOG_LOGS

	alog() << "show_topN_display\t" << std::endl;

#endif  // LOG_LOGS
}

void Logger::log_show_topn_context_display(const DatasetFrames& /*frames*/, const std::vector<FrameId>& /*imgs*/)
{
	push_event("browsing", "rankedList", "topn_context_display;");

#ifdef LOG_LOGS

	alog() << "show_topN_context_display\t" << std::endl;

#endif  // LOG_LOGS
}

void Logger::log_show_topknn_display(const DatasetFrames& _dataset_frames, FrameId frame_ID,
                                     const std::vector<FrameId>& /*imgs*/)
{
	auto vf = _dataset_frames.get_frame(frame_ID);

	std::stringstream data_ss;
	data_ss << "VId" << (vf.video_ID + 1) << ",FN" << vf.frame_number << ";FId" << frame_ID << ";topknn_display;";

	push_event("image", "globalFeatures", data_ss.str());

#ifdef LOG_LOGS

	alog() << "show_topKNN_display\t"
	       << "frame_ID=" << frame_ID << std::endl;

#endif  // LOG_LOGS
}

void Logger::log_show_detail_display(const DatasetFrames& _dataset_frames, FrameId frame_ID)
{
	auto vf = _dataset_frames.get_frame(frame_ID);

	std::stringstream data_ss;
	data_ss << "VId" << (vf.video_ID + 1) << ",FN" << vf.frame_number << ";FId" << frame_ID << ";video_detail;";

	push_event("browsing", "videoSummary", data_ss.str());

#ifdef LOG_LOGS

	alog() << "show_detail_display\t"
	       << "frame_ID=" << frame_ID << std::endl;

#endif  // LOG_LOGS
}

void Logger::log_show_video_replay(const DatasetFrames& _dataset_frames, FrameId frame_ID, float delta)
{
	static int64_t last_replay_submit = 0;
	static FrameId last_frame_ID = IMAGE_ID_ERR_VAL;

	// If timeout should be handled here
	if (cfg.apply_log_action_timeout) {
		// If no need to log now
		if (last_replay_submit + cfg.log_action_timeout > size_t(utils::timestamp()) && frame_ID == last_frame_ID)
			return;
	}

	last_replay_submit = utils::timestamp();
	last_frame_ID = frame_ID;

	auto vf = _dataset_frames.get_frame(frame_ID);

	std::stringstream data_ss;
	data_ss << "VId" << (vf.video_ID + 1) << ",FN" << vf.frame_number << ";FId" << frame_ID << ";replay;";

	push_event("browsing", "temporalContext", data_ss.str());

#ifdef LOG_LOGS

	alog() << "replay_video\t"
	       << "frame_ID=" << frame_ID << "\t"
	       << "video_ID=" << vf.video_ID << "\t"
	       << "dir=" << (delta > 0.0F ? "forward" : "backwards") << std::endl;

#endif  // LOG_LOGS
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
	if (cfg.apply_log_action_timeout) {
		// If no need to log now
		if (last_logged + cfg.log_action_timeout > size_t(utils::timestamp()) && from_disp_type == last_disp_type)
			return;
	}

	last_logged = utils::timestamp();
	last_disp_type = from_disp_type;

	std::stringstream data_ss;
	data_ss << "scroll" << (dirY > 0 ? "Up" : "Down") << ";" << dirY << ";" << disp_type << ";";

	push_event("browsing", ev_type, data_ss.str());

#ifdef LOG_LOGS

	alog() << "mouse_scroll\t"
	       << "dir=" << (dirY > 0 ? "up" : "down") << "\t"
	       << "disp_type=" << disp_type << std::endl;

#endif  // LOG_LOGS
}

void Logger::log_reset_search()
{
	push_event("browsing", "resetAll", "");

#ifdef LOG_LOGS

	alog() << "reset_all\t" << std::endl;

#endif  // LOG_LOGS
}

void Logger::poll()
{
	if (last_submit_timestamp + cfg.send_logs_to_server_period < size_t(utils::timestamp())) send_backlog_only();
}

void Logger::push_event(const std::string& cat, const std::string& type, const std::string& value)
{
	std::vector<Json> types{ type };
	Json types_arr = Json::array(types);

	Json a = Json::object{
		{ "timestamp", double(utils::timestamp()) }, { "category", cat }, { "type", types_arr }, { "value", value }
	};

	backlog.emplace_back(std::move(a));
}
