
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
using namespace sh::LOGGING_STRINGS;

Logger::Logger(const EvalServerSettings& settings, const UserContext* p_user_ctx, EvalServerClient* p_eval_server)
    : _logger_settings{ settings },
      _p_user_ctx{ p_user_ctx },
      _p_eval_server{ p_eval_server },
      _last_interactions_submit_ts{ utils::timestamp() }
{
	// Make sure that log directories exist
	if (!utils::dir_create(get_log_dir_queries()) ||
	    !utils::dir_create(_logger_settings.log_dir_results + "/" + _p_user_ctx->get_username()) ||
	    !utils::dir_create(_logger_settings.log_dir_user_actions + "/" + _p_user_ctx->get_username()) ||
	    !utils::dir_create(_logger_settings.log_dir_user_actions_summary + "/" + _p_user_ctx->get_username())) {
		std::string msg{ "Unable to create log directories!" };
		SHLOG_E(msg);
		throw std::runtime_error{ msg };
	}

	std::string filepath_results{ get_results_log_filepath() };
	std::string filepath_actions{ get_actions_log_filepath() };
	std::string filepath_summary{ get_summary_log_filepath() };

	_results_log_stream.open(filepath_results);
	_actions_log_stream.open(filepath_actions);
	_summary_log_stream.open(filepath_summary);

	if (!_actions_log_stream.is_open() || !_summary_log_stream.is_open()) {
		std::string msg{ "Unable to create log directories!" };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	// Enable automatic flushing
	_results_log_stream << std::unitbuf << "[" << std::endl;
	for (auto&& s : _results_streams) {
		s << "[";
	}
	_actions_log_stream << std::unitbuf << "[" << std::endl;
	for (auto&& s : _actions_streams) {
		s << "[";
	}

	_summary_log_stream << std::unitbuf << "[" << std::endl;
	for (auto&& s : _summary_streams) {
		s << "[";
	}
}

Logger::~Logger()
{
	submit_interaction_logs_buffer();
	_results_log_stream << "]" << std::endl;
	for (auto&& s : _results_streams) {
		s << "]";
	}

	_actions_log_stream << "]" << std::endl;
	for (auto&& s : _actions_streams) {
		s << "]";
	}

	_summary_log_stream << "]" << std::endl;
	for (auto&& s : _summary_streams) {
		s << "]";
	}
}

void Logger::log_submit(const VideoFrame frame, bool submit_result)
{
	nlohmann::json log(frame.to_JSON());

	// clang-format off
	nlohmann::json log_JSON{
		{ "submit_result", submit_result }
	};
	// clang-format on

	log_JSON.insert(log.begin(), log.end());

	// Create the log
	push_action("submit", "OTHER", "submit", (submit_result ? "true" : "false"), std::move(log_JSON),
	            { "videoId", "frameNumber", "frameId" });
}

void Logger::log_query(const LogHash& hash, const Query& query) const
{
	auto dir{ get_log_dir_queries() };

	auto filepath_bin{ dir + "/" + hash + ".bin" };
	auto filepath_JSON{ dir + "/" + hash + ".json" };

	// \todo Implement...
	SHLOG_D("Writing the query to '" + filepath_bin + "'...");
	SHLOG_D("Writing the query to '" + filepath_JSON + "'...");

	utils::serialize_to_file<Query>(query, filepath_bin);
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
		_p_eval_server->send_interactions_log(a);
		_interactions_buffer.clear();
	}

	// We always reset timer
	_last_interactions_submit_ts = utils::timestamp();
}

void Logger::log_results(const DatasetFrames& _dataset_frames, const ScoreModel& /*scores*/,
                         const std::set<FrameId>& /*likes*/, const UsedTools& used_tools, DisplayType /*disp_type*/,
                         const std::vector<FrameId>& topn_imgs, const std::string& sentence_query,
                         const size_t topn_frames_per_video, const size_t topn_frames_per_shot)
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
			std::stringstream item_ss;

			// If LSC type of submit
			if (_logger_settings.submit_LSC_IDs) {
				item_ss << vf.LSC_id;
				/* !!!
				  We use "rank" field as our internal frame ID */
				results.push_back(nlohmann::json{ { "item", item_ss.str() }, { "rank", int(vf.frame_ID) } });

			}
			// Non-LSC submit
			else {
				item_ss << std::setfill('0') << std::setw(5) << (vf.video_ID + 1);  //< !! VBS videos start from 1

				/* !!!
				  We use "rank" field as our internal frame ID */
				results.push_back(nlohmann::json{
				    { "item", item_ss.str() }, { "frame", int(vf.frame_number) }, { "rank", int(vf.frame_ID) } });
			}

			++i;
		}
	}

	std::set<nlohmann::json> used_cats;
	std::set<nlohmann::json> used_types;
	std::set<nlohmann::json> sort_types;

	std::string query_val(sentence_query + ";");
	std::stringstream filters_val_ss;

	// If Top KNN request
	if (used_tools.topknn_used) {
		// Mark this as KNN request
		query_val += "showNearestNeighboursDisplay;";

		used_cats.insert("image");
		used_types.insert("globalFeatures");
		sort_types.insert("globalFeatures");
	}
	// Else normal rescore
	else {
		// Just mark that this was NOT KNN request
		query_val += "rescore;";

		if (used_tools.KWs_used) {
			used_cats.insert("text");
			used_types.insert("jointEmbedding");
			sort_types.insert("jointEmbedding");
		}

		if (used_tools.canvas_bitmap_used) {
			used_cats.insert("image");
			used_types.insert("localizedObjectBitmap");
			sort_types.insert("localizedObjectBitmap");
		}

		if (used_tools.canvas_text_used) {
			used_cats.insert("text");
			used_types.insert("localizedObjectText");
			sort_types.insert("localizedObjectText");
		}

		if (used_tools.relocation_used) {
			used_cats.insert("image");
			used_types.insert("textQueryRelocation");
			sort_types.insert("textQueryRelocation");
		}

		if (used_tools.bayes_used) {
			used_cats.insert("image");
			used_types.insert("feedbackModel");
			sort_types.insert("feedbackModel");
		}

		if (used_tools.filters != nullptr) {
			used_cats.insert("filter");
			used_types.insert("lifelog");

			const Filters* fs{ used_tools.filters };

			// Write values value
			filters_val_ss << "hFrom=" << size_t(fs->time.from) << ";"
			               << "hTo=" << size_t(fs->time.to) << ";"
			               << "days=";

			for (size_t i{ 0 }; i < 7; ++i) {
				auto&& d{ fs->days[i] };
				filters_val_ss << (d ? "1" : "0");
			}
			filters_val_ss << ";";
		}
	}

	query_val += "fromVideoLimit=";
	query_val += std::to_string(topn_frames_per_video);
	query_val += ";fromShotLimit=";
	query_val += std::to_string(topn_frames_per_shot);

	nlohmann::json result_json_arr = nlohmann::json(results);

	std::vector<nlohmann::json> values{ query_val };

	auto filters_val{ filters_val_ss.str() };
	if (!filters_val.empty()) {
		values.emplace_back(filters_val);
	}

	nlohmann::json values_arr = nlohmann::json(values);

	nlohmann::json top = nlohmann::json{
		{ "timestamp", utils::timestamp() },
		{ "sortType", sort_types },
		{ "resultSetAvailability", "top" },
		{ "results", std::move(result_json_arr) },
		{ "events", nlohmann::json::array() },
		// ---
		{ "usedCategories", used_cats },
		{ "usedTypes", used_types },
		{ "values", values_arr },
	};

	// Send it to the eval server
	_p_eval_server->send_results_log(top);

	/* ***
	 * Augment the log with extra data */
	top["hash"] = hash;
	top["serverTimestamp"] = _p_eval_server->get_server_ts();
	top["userToken"] = _p_eval_server->get_user_token();
	// top["currentTask"] = _p_eval_server->get_current_task();
	// top["likes"] = likes;

	// Write separate result log
	write_result(top);
	// Write the action
	push_action("reportResults", "OTHER", "reportResults", "");
}

void Logger::log_canvas_query(const std::vector<TemporalQuery>& temp_queries /*canvas_query*/,
                              const std::vector<VideoFrame>* p_targets)
{
	if (temp_queries.empty()) return;

	auto path{ _logger_settings.log_dir_user_actions_summary + "/"s + std::to_string(utils::timestamp()) + "/"s };

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

void sh::Logger::log_rescore(const Query& /*prev_query*/, const Query& new_query)
{
	// Log query
	auto h{ push_action("rescore", "OTHER ", "rescore", "") };
	log_query(h, new_query);
}

void Logger::log_text_query_change(const std::string& text_query)
{
	static int64_t last_logged{ 0 };

	// If timeout should be handled here
	if (_logger_settings.apply_log_action_timeout) {
		// If no need to log now
		if (last_logged + _logger_settings.log_action_timeout > size_t(utils::timestamp())) return;
	}

	push_action("textQueryChange", "TEXT", "jointEmbedding", text_query);
}
void Logger::log_like(FrameId frame_ID) { log_bothlike(frame_ID, "like"); }

void Logger::log_unlike(FrameId frame_ID) { log_bothlike(frame_ID, "unlike"); }

void Logger::log_bookmark(FrameId frame_ID) { log_bothlike(frame_ID, "bookmark"); }

void Logger::log_unbookmark(FrameId frame_ID) { log_bothlike(frame_ID, "unbookmark"); }

void Logger::log_show_random_display(const DatasetFrames& /*frames*/, const std::vector<FrameId>& /*imgs*/)
{
	push_action(ACTION_NAMES::SHOW_RANDOM_DISPLAY, STD_CATEGORIES::BROWSING, STD_TYPES::RANDOM_SELECTION,
	            STD_VALUES::RANDOM_DISPLAY);
}

void Logger::log_show_som_display(const DatasetFrames& /*frames*/, const std::vector<FrameId>& /*imgs*/)
{
	push_action(ACTION_NAMES::SHOW_SOM_DISPLAY, STD_CATEGORIES::BROWSING, STD_TYPES::EXPLORATION,
	            STD_VALUES::SOM_DISPLAY);
}

void Logger::log_show_som_relocation_display(const DatasetFrames& /*frames*/, const std::vector<FrameId>& /*imgs*/)
{
	push_action(ACTION_NAMES::SHOW_SOM_RELOC_DISPLAY, STD_CATEGORIES::BROWSING, STD_TYPES::EXPLORATION,
	            STD_VALUES::SOM_RELOC_DISPLAY);
}

void Logger::log_show_topn_display(const DatasetFrames& /*frames*/, const std::vector<FrameId>& /*imgs*/)
{
	push_action(ACTION_NAMES::SHOW_TOP_SCORED_DISPLAY, STD_CATEGORIES::BROWSING, STD_TYPES::RANKED_LIST,
	            STD_VALUES::TOP_SCORED_DISPLAY);
}

void Logger::log_show_topn_context_display(const DatasetFrames& /*frames*/, const std::vector<FrameId>& /*imgs*/)
{
	push_action(ACTION_NAMES::SHOW_TOP_SCORED_CONTEXT_DISPLAY, STD_CATEGORIES::BROWSING, STD_TYPES::RANKED_LIST,
	            STD_VALUES::TOP_SCORED_CONTEXT_DISPLAY);
}

void Logger::log_show_topknn_display(const DatasetFrames& _dataset_frames, FrameId frame_ID,
                                     const std::vector<FrameId>& /*imgs*/)
{
	auto frame = _dataset_frames.get_frame(frame_ID);

	nlohmann::json log(frame.to_JSON());

	push_action("showNearestNeighboursDisplay", "IMAGE", "globalFeatures", log.dump(4), std::move(log), { "frameId" });
}

void Logger::log_show_detail_display(const DatasetFrames& _dataset_frames, FrameId frame_ID)
{
	auto vf = _dataset_frames.get_frame(frame_ID);

	std::stringstream data_ss;
	data_ss << "VId" << (vf.video_ID + 1) << ",FN" << vf.frame_number << ";FId" << frame_ID << ";video_detail;";

	push_action("showDetailDisplay", STD_CATEGORIES::BROWSING, "videoSummary", data_ss.str());
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

	push_action("replayVideo", STD_CATEGORIES::BROWSING, "temporalContext", data_ss.str());
}

void Logger::log_scroll(const DatasetFrames& /*frames*/, DisplayType from_disp_type, float dirY)
{
	std::string ev_type("rankedList");
	std::string disp_type;

	switch (from_disp_type) {
		case DisplayType::DTopN:
			ev_type = "rankedList";
			disp_type = "topnDisplay";
			break;

		case DisplayType::DTopNContext:
			ev_type = "rankedList";
			disp_type = "topnDisplayWithContext";
			break;

		case DisplayType::DTopKNN:
			ev_type = "rankedList";
			disp_type = "topknnDisplay";
			break;

		case DisplayType::DVideoDetail:
			ev_type = "videoSummary";
			disp_type = "videoDetail";
			break;

		case DisplayType::DVideoReplay:
			ev_type = "videoSummary";
			disp_type = "videoReplay";
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
	data_ss << "scroll" << (dirY > 0 ? "up" : "down") << ";" << dirY << ";" << disp_type << ";";

	// clang-format off
	nlohmann::json log_JSON{
		{ "displayType", disp_type },
		{ "direction", (dirY > 0 ? "up" : "down") },
		{ "delta", dirY }
	};
	// clang-format on

	push_action("scroll", STD_CATEGORIES::BROWSING, ev_type, data_ss.str(), std::move(log_JSON),
	            { "displayType", "direction" });
}

void Logger::log_reset_search() { push_action("resetAll", STD_CATEGORIES::BROWSING, "resetAll", ""); }

void Logger::poll()
{
	if (_last_interactions_submit_ts + _logger_settings.send_logs_to_server_period < size_t(utils::timestamp()))
		submit_interaction_logs_buffer();
}

void Logger::log_search_context_switch(std::size_t dest_context_ID, size_t src_context_ID)
{
	// clang-format off
	nlohmann::json log{ 
		{"sourceContextId", src_context_ID},
		{"destinationContextId", dest_context_ID}
	};
	// clang-format on

	// Create the log
	push_action("searchContextSwitch", "OTHER", "searchContextSwitch", log.dump(4), std::move(log),
	            { "sourceContextId", "destinationContextId" });
}

void Logger::log_bothlike(FrameId frame_ID, const std::string& type)
{
	auto vf = _p_user_ctx->get_frames()->get_frame(frame_ID);
	auto f_JSON(vf.to_JSON());

	// clang-format off
		nlohmann::json log_JSON{
			{ "likes", _p_user_ctx->ctx.likes },
			{ "dispType", disp_type_to_str(_p_user_ctx->ctx.curr_disp_type) },
		};
	// clang-format on

	log_JSON.insert(f_JSON.begin(), f_JSON.end());

	push_action(type, "IMAGE", "feedbackModel", f_JSON.dump(4), std::move(log_JSON), { "frameId" });
}

LogHash Logger::gen_action_hash(UnixTimestamp ts)
{
	std::string hash{ _p_user_ctx->get_username() };
	hash.append("#");
	hash.append(std::to_string(ts));
	return hash;
}

LogHash Logger::push_action(const std::string& action_name, const std::string& cat, const std::string& type,
                            const std::string& value, nlohmann::json&& our,
                            std::initializer_list<std::string> summary_keys)
{
	/* ***
	 * Prepare the log compatible with the eval server. */
	UnixTimestamp ts{ utils::timestamp() };
	UnixTimestamp server_ts{ _p_eval_server->get_server_ts() };
	auto hash{ gen_action_hash(ts) };

	std::string vval{ "|" + action_name + "|" };

	// clang-format off
	nlohmann::json log_JSON{
		{ "timestamp", ts }, 
		{ "category", cat }, 
		{ "type",  type }, 
		{ "value", vval + value }
	};
	// clang-format on
	_interactions_buffer.emplace_back(log_JSON);

	/* ***
	 * Construct our log (use server if none specified). */
	nlohmann::json our_log_JSON(our);

	/* ***
	 * Augment the log with extra data */

	// clang-format off
	nlohmann::json meta_JSON{
		{ "hash", hash },
		{ "serverTimestamp", server_ts },
		{ "userToken", _p_eval_server->get_user_token() },
	};
	// clang-format on
	meta_JSON.insert(log_JSON.begin(), log_JSON.end());

	our_log_JSON["metadata"] = meta_JSON;
	our_log_JSON["actionName"] = action_name;

	write_action(our_log_JSON);
	write_summary(our_log_JSON, action_name, summary_keys);

	return hash;
}

std::string Logger::get_log_dir_queries() const
{
	return _logger_settings.log_dir_queries + "/" + _p_user_ctx->get_username();
}

std::string Logger::get_results_log_filepath() const
{
	std::filesystem::path p{ _logger_settings.log_dir_results + "/" + _p_user_ctx->get_username() };
	p = p /
	    ("results." + utils::get_formated_timestamp("%d-%m-%YT%H-%M-%S") + "." + _p_user_ctx->get_username() + ".json");

	return p.string();
}

std::string Logger::get_actions_log_filepath() const
{
	std::filesystem::path p{ _logger_settings.log_dir_user_actions + "/" + _p_user_ctx->get_username() };
	p = p / ("user-actions." + utils::get_formated_timestamp("%d-%m-%YT%H-%M-%S") + "." + _p_user_ctx->get_username() +
	         ".json");

	return p.string();
}

std::string Logger::get_summary_log_filepath() const
{
	std::filesystem::path p{ _logger_settings.log_dir_user_actions_summary + "/" + _p_user_ctx->get_username() };
	p = p / ("user-actions-summary." + utils::get_formated_timestamp("%d-%m-%YT%H-%M-%S") + "." +
	         _p_user_ctx->get_username() + ".log");

	return p.string();
}
