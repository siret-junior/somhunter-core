/* This file is part of SOMHunter.
 *
 * Copyright (C) 2021 Frantisek Mejzlik <frankmejzlik@protonmail.com>
 *                    Mirek Kratochvil <exa.exa@gmail.com>
 *                    Patrik Vesely <prtrikvesely@gmail.com>
 *                    Vit Skrhak <v.skrhak@gmail.com>
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

#include "tests.h"

#include <filesystem>
#include <map>
#include <stack>
#include <string>
// ---
#include <nlohmann/json.hpp>
// ---
#include "json-helpers.hpp"
#include "settings.h"
#include "somhunter.h"
#include "test-utils.hpp"
#include "utils.hpp"

namespace fs = std::filesystem;
using namespace sh::tests;
using json = nlohmann::json;

// clang-format off

void TESTER_Somhunter::run_all_tests(const std::string &cfg_fpth) {
	SHLOG("====================================================");
	SHLOG("\tInitializing the `Somhunter` class tests...");
	SHLOG("====================================================");

	// Parse config file
	auto config = Settings::parse_JSON_config(cfg_fpth);

	// Make sure that values are right for these tests
	config.presentation_views.topn_frames_per_video = 3;
	config.presentation_views.topn_frames_per_shot = 1;

	SHLOG("Running all the Somhunter class tests...");

	{
		Somhunter core{ cfg_fpth };
		TEST_log_results(core);
	}
	Somhunter core{ cfg_fpth };
	TEST_like_frames(core);
	TEST_bookmark_frames(core);
	TEST_autocomplete_keywords(core);
	TEST_rescore(core);
	TEST_canvas_queries(core);

#ifdef TEST_FILTERS
	TEST_rescore_filters(core);
#endif

	SHLOG("====================================================");
	SHLOG("\tIf you got here, all `Somhunter` tests were OK...");
	SHLOG("====================================================");
}

void TESTER_Somhunter::TEST_canvas_queries(Somhunter &core) {
	SHLOG("\t Testing `Somhunter::rescore` method with collage queries...");
	core.reset_search_session();

	
	SHLOG("\t Testing `Somhunter::rescore` method with collage queries finished.");
}

void TESTER_Somhunter::TEST_like_frames(Somhunter &core) {
	SHLOG("\t Testing `Somhunter::like_frames` method...");

	auto [disp, likes, _bookmarks, _video_seen]{ core.get_display(DisplayType::DTopN, 0, 0) };
	size_t size{ disp.size() };
	do_assert(size > 0, "Top N display is empty!");

	auto ff = *(disp.begin());
	auto fm = *(disp.begin() + utils::irand(1_z, size - 1));
	auto fl = *(--(disp.end()));

	using vec = std::vector<FrameId>;

	core.like_frames(vec{ ff->frame_ID });
	do_assert(likes.count(ff->frame_ID) == 1, "Frame SHOULD be liked.");
	core.like_frames(std::vector<FrameId>{ ff->frame_ID });
	do_assert(likes.count(ff->frame_ID) == 0, "Frame SHOULD NOT be liked.");

	core.like_frames(vec{ fm->frame_ID });
	do_assert(likes.count(fm->frame_ID) == 1, "Frame SHOULD be liked.");
	core.like_frames(std::vector<FrameId>{ fm->frame_ID });
	do_assert(likes.count(fm->frame_ID) == 0, "Frame SHOULD NOT be liked.");

	core.like_frames(vec{ fl->frame_ID });
	do_assert(likes.count(fl->frame_ID) == 1, "Frame SHOULD be liked.");
	core.like_frames(std::vector<FrameId>{ fl->frame_ID });
	do_assert(likes.count(fl->frame_ID) == 0, "Frame SHOULD NOT be liked.");

	vec all;
	for (auto &&f : disp) {
		all.emplace_back(f->frame_ID);
	}
	core.like_frames(all);
	do_assert(likes.size() == size, "All frames SHOULD be liked.");

	core.like_frames(all);
	do_assert(likes.size() == 0, "All frames SHOULD NOT be liked.");

	SHLOG("\t Testing `Somhunter::like_frames` finished.");
}

void TESTER_Somhunter::TEST_bookmark_frames(Somhunter &core) {
	SHLOG("\t Testing `Somhunter::bookmark_frames` method...");

	auto [disp, likes, _bookmarks, _video_seen]{ core.get_display(DisplayType::DTopN, 0, 0) };
	size_t size{ disp.size() };
	do_assert(size > 0, "Top N display is empty!");

	auto ff = *(disp.begin());
	auto fm = *(disp.begin() + utils::irand(1_z, size - 1));
	auto fl = *(--(disp.end()));

	using vec = std::vector<FrameId>;

	core.bookmark_frames(vec{ ff->frame_ID });
	do_assert(_bookmarks.count(ff->frame_ID) == 1, "Frame SHOULD be bookmarked.");
	core.bookmark_frames(std::vector<FrameId>{ ff->frame_ID });
	do_assert(_bookmarks.count(ff->frame_ID) == 0, "Frame SHOULD NOT be bookmarked.");

	core.bookmark_frames(vec{ fm->frame_ID });
	do_assert(_bookmarks.count(fm->frame_ID) == 1, "Frame SHOULD be bookmarked.");
	core.bookmark_frames(std::vector<FrameId>{ fm->frame_ID });
	do_assert(_bookmarks.count(fm->frame_ID) == 0, "Frame SHOULD NOT be bookmarked.");

	core.bookmark_frames(vec{ fl->frame_ID });
	do_assert(_bookmarks.count(fl->frame_ID) == 1, "Frame SHOULD be bookmarked.");
	core.bookmark_frames(std::vector<FrameId>{ fl->frame_ID });
	do_assert(_bookmarks.count(fl->frame_ID) == 0, "Frame SHOULD NOT be bookmarked.");

	vec all;
	for (auto &&f : disp) {
		all.emplace_back(f->frame_ID);
	}
	core.bookmark_frames(all);
	do_assert(_bookmarks.size() == size, "All frames SHOULD be bookmarked.");

	core.bookmark_frames(all);
	do_assert(_bookmarks.size() == 0, "All frames SHOULD NOT be bookmarked.");

	SHLOG("\t Testing `Somhunter::bookmark_frames` finished.");
}

void TESTER_Somhunter::TEST_autocomplete_keywords(Somhunter &core) {
	SHLOG("\t Testing `Somhunter::autocomplete_keywords` method...");

	/*
	 * Non-empty cases
	 */
#ifdef TESTING_BOW_W2VV
	std::map<std::string, std::vector<KeywordId>> correct{ { "cat", { 44, 7725, 8225, 9712 } },
		                                                   { "z", { 1615, 9127, 8767, 4316 } } };
#else
	std::map<std::string, std::vector<KeywordId>> correct{

	};
	SHLOG_W("No test values for this dataset.");
#endif
	for (auto &&[key, val] : correct) {
		auto ac_res{ core.autocomplete_keywords(key, 10) };

		for (size_t i{ 0 }; i < val.size(); ++i) {
			do_assert(ac_res[i]->synset_ID == val[i], "Incorrect keyword");
		}
	}

	/*
	 * Non-empty cases
	 */
	auto ac_res{ core.autocomplete_keywords("iax", 10) };
	do_assert(ac_res.empty(), "Results should be empty!");

	ac_res = core.autocomplete_keywords("\\/?!,.'\"", 10);
	do_assert(ac_res.empty(), "Results should be empty!");

	ac_res = core.autocomplete_keywords("cat", 0);
	do_assert(ac_res.empty(), "Results should be empty!");

	ac_res = core.autocomplete_keywords("", 10);
	do_assert(ac_res.empty(), "Results should be empty!");

	SHLOG("\t Testing `Somhunter::autocomplete_keywords` finished.");
}

void TESTER_Somhunter::TEST_rescore(Somhunter &core) {
	SHLOG("\t Testing `Somhunter::TEST_rescore` method...");

	FramePointerRange disp{};

	/*
	 * #1 Text
	 */
	Query q{ std::vector({ "cat" }) };
	auto h{ core.rescore(q)._history };
	auto state1{ core._user_context.ctx };
	do_assert(h.back() == core._user_context.ctx, "Inconsistent data.");

#ifdef TESTING_ITEC_DATASET
	disp = core.get_display(DisplayType::DTopN, 0, 0)._dataset_frames;
	do_assert_equals(disp[0]->frame_ID, 80U, "Incorrect frame in the display.");
	do_assert_equals(disp[1]->frame_ID, 130U, "Incorrect frame in the display.");

#endif

	/*
	 * #2 Temporal text
	 */
	core.like_frames(std::vector<FrameId>{ 80 });
	q = Query{ std::vector({ "dog catalog", "habitat " }) };
	h = core.rescore(q)._history;
	auto state2{ core._user_context.ctx };
	do_assert(h.back() == core._user_context.ctx, "Inconsistent data.");

#ifdef TESTING_ITEC_DATASET
	disp = core.get_display(DisplayType::DTopN, 0, 0)._dataset_frames;

	do_assert_equals(disp[0]->frame_ID, 224U, "Incorrect frame in the display.");
	do_assert_equals(disp[1]->frame_ID, 125U, "Incorrect frame in the display.");

	do_assert_equals(disp[7]->frame_ID, 331U, "Incorrect frame in the display.");
	do_assert_equals(disp[8]->frame_ID, 140U, "Incorrect frame in the display.");
#endif

	/*
	 * #3: Text & likes
	 */
	core.like_frames(std::vector<FrameId>{ 187 });
	core.like_frames(std::vector<FrameId>{ 217 });
	core.like_frames(std::vector<FrameId>{ 581 });
	q = Query{ std::vector({ "chicken" }) };
	h = core.rescore(q)._history;
	auto state3{ core._user_context.ctx };
	do_assert(h.back() == core._user_context.ctx, "Inconsistent data.");
	do_assert_equals(h.back().likes.size(), 0, "Likes should be reset with rescore.");

#ifdef TESTING_ITEC_DATASET
	disp = core.get_display(DisplayType::DTopN, 0, 0)._dataset_frames;
	
	do_assert_equals(disp[0]->frame_ID, 489U, "Incorrect frame in the display.");
	do_assert_equals(disp[1]->frame_ID, 475U, "Incorrect frame in the display.");

	do_assert_equals(disp[7]->frame_ID, 589U, "Incorrect frame in the display.");
	do_assert_equals(disp[8]->frame_ID, 308U, "Incorrect frame in the display.");
#endif

	/*
	 * #4: `conext_switch`
	 */
	core.switch_search_context(1);
	do_assert(state1 == core._user_context.ctx, "State SHOULD BE equal.");
#ifdef TESTING_ITEC_DATASET
	disp = core.get_display(DisplayType::DTopN, 0, 0)._dataset_frames;
	
	do_assert_equals(disp[0]->frame_ID, 80U, "Incorrect frame in the display.");
	do_assert_equals(disp[1]->frame_ID, 130U, "Incorrect frame in the display.");
#endif

	core.switch_search_context(2);
	do_assert(state2 == core._user_context.ctx, "State SHOULD BE equal.");
#ifdef TESTING_ITEC_DATASET
	disp = core.get_display(DisplayType::DTopN, 0, 0)._dataset_frames;

	do_assert_equals(disp[0]->frame_ID, 224U, "Incorrect frame in the display.");
	do_assert_equals(disp[1]->frame_ID, 125U, "Incorrect frame in the display.");

	do_assert_equals(disp[7]->frame_ID, 331U, "Incorrect frame in the display.");
	do_assert_equals(disp[8]->frame_ID, 140U, "Incorrect frame in the display.");
#endif

	core.switch_search_context(3);
	do_assert(state3 == core._user_context.ctx, "State SHOULD BE equal.");
#ifdef TESTING_ITEC_DATASET
	disp = core.get_display(DisplayType::DTopN, 0, 0)._dataset_frames;

	do_assert_equals(disp[0]->frame_ID, 489U, "Incorrect frame in the display.");
	do_assert_equals(disp[1]->frame_ID, 475U, "Incorrect frame in the display.");

	do_assert_equals(disp[7]->frame_ID, 589U, "Incorrect frame in the display.");
	do_assert_equals(disp[8]->frame_ID, 308U, "Incorrect frame in the display.");
#endif

	SHLOG("\t Testing `Somhunter::TEST_rescore` finished.");
}

void TESTER_Somhunter::TEST_rescore_filters(Somhunter &core) {
	SHLOG("\t Testing `Somhunter::TEST_rescore` score filter...");

	std::vector<std::tuple<std::string, Hour, Hour, uint8_t>> input{
		{ "", Hour(0), Hour(24), uint8_t(0x00) },    // Empty result
		{ "cat", Hour(0), Hour(0), uint8_t(0x3F) },  // Only 00:xx
		{ "", Hour(9), Hour(17), uint8_t(0x01) },    // 9-17h, mondays
		{ "", Hour(17), Hour(17), uint8_t(0x01) },   // 9-17h, mondays
		{ "", Hour(12), Hour(12), uint8_t(0x03) },   // 12h, mondays, wednesdays
		{ "", Hour(0), Hour(24), uint8_t(0x03) },    // all hours, mondays, wednesdays, fridays
	};

	std::vector<std::vector<FrameId>> spec_tests;
#ifdef TESTING_LSC5DAYS_DATASET
	spec_tests.emplace_back(std::vector<ImageId>{});
	spec_tests.emplace_back(std::vector<ImageId>{ 1769 });
	spec_tests.emplace_back(std::vector<ImageId>{ 356 });
	spec_tests.emplace_back(std::vector<ImageId>{ 356 });
	spec_tests.emplace_back(std::vector<ImageId>{ 752, 193 });
	spec_tests.emplace_back(std::vector<ImageId>{ 977, 508 });
#endif

	size_t t_i{ 0 };
	for (auto &&[tq, fr, to, days_mask] : input) {
		Filters fs{ TimeFilter{ fr, to },YearFilter{ 2000, 2030 }, WeekDaysFilter{ days_mask } };

		Query q{ std::vector({ tq }) };
		q.filters = fs;

		core.rescore(q);
		auto disp{ core.get_topn_display(0) };

		size_t i{ 0 };
		for (auto &&f : disp) {
			do_assert(fr <= f->hour && f->hour <= to, "Should not be in the result.");
			do_assert(utils::is_set(days_mask, f->weekday), "Should not be in the result.");

			// Dataset specific tests
			if (spec_tests[t_i].size() > i) {
				do_assert(spec_tests[t_i][i] == f->frame_ID, "Incorrect frame ID.");
			}

			// std::cout << "H: " << size_t(f->hour) << " WD: " << size_t(f->weekday) << ", ID: " << f->frame_ID <<
			// std::endl;
			++i;
		}

		// Empty case
		if (spec_tests[t_i].empty()) {
			do_assert(disp.begin() == disp.end(), "Should be empty.");
		}

		// std::cout << "\n ================================== \n" << std::endl;
		++t_i;
	}

	SHLOG("\t Testing `Somhunter::TEST_rescore` score filter finished...");
}


void TESTER_Somhunter::TEST_log_results(Somhunter &core) {
	using namespace LOGGING_STRINGS::ACTION_NAMES;
	using namespace LOGGING_STRINGS;

	// Attach new log pipes
	auto&& [summary, actions, results]{core._user_context._logger.get_debug_streams()};

	/* ***
	 * BASIC
	 */
	{
		// <!> ACTION: RESET_ALL
		core.reset_search_session();

		json val{ RESET_ALL };
		json data{ wrap_and_parse(actions) };

		contains_key_with_value_recur(data, STD_KEYS::ACTION_NAME, val);
		assert_column_contains(summary.str(), 2, RESET_ALL);

		actions.clear();
		summary.clear();
		results.clear();

		// <!> ACTION: SHOW_TOP_SCORED_DISPLAY
		core.get_display(DisplayType::DTopN, 0, 0);

		std::cout << summary.str() << std::endl;
		std::cout << actions.str() << std::endl;
		std::cout << results.str() << std::endl;

		// <!> ACTION: SHOW_TOP_SCORED_CONTEXT_DISPLAY
		core.get_display(DisplayType::DTopNContext, 0, 0);

		std::cout << summary.str() << std::endl;
		std::cout << actions.str() << std::endl;
		std::cout << results.str() << std::endl;

		// <!> ACTION: SHOW_SOM_DISPLAY
		core.get_display(DisplayType::DSom, 0, 0);
	
		std::cout << summary.str() << std::endl;
		std::cout << actions.str() << std::endl;
		std::cout << results.str() << std::endl;

	}

	/* ***
	 * BASIC
	 */
	{

	}
}


const char *json_contents = R"(
{ "core": {
  "user_token": "admin",
  "eval_server":{
		"submit_to_VBS": true,

		"submit_server": "dres",
		"server_config": {
		  "vbs": {
			"submit_interaction_URL": "http://herkules.ms.mff.cuni.cz:8080/vbs/query",
			"submit_rerank_URL": "http://herkules.ms.mff.cuni.cz:8080/vbs/result",
			"submit_URL": "http://herkules.ms.mff.cuni.cz:8080/vbs/submit"
		  },
		  "dres": {
			"submit_interaction_URL": "http://localhost:8080/log/query",
			"submit_rerank_URL": "http://localhost:8080/log/result",
			"submit_URL": "http://localhost:8080/submit",
			"cookie_file": "cookie.txt",
			"login_URL": "http://localhost:8080/api/login",
			"username": "admin",
			"password": "adminadmin"
			}
		},
	
		"team_ID": 4,
		"member_ID": 1,
	  
		"log_dir_eval_server_requests": "logs/submitted_logs/",
		"log_dir_user_actions": "logs/actions/",
		"log_dir_user_actions_summary": "logs/collages/",
		"log_dir_debug": "logs/requests/",
		"log_file_suffix": ".json",
		"extra_verbose_log": false,
	  
		"send_logs_to_server_period": 10000,
		"log_action_timeout": 500
	},

	"max_frame_filename_len": 64,
	"display_page_size": 128,
	"topn_frames_per_video": 3,
	"topn_frames_per_shot": 1,
	
	"filename_offsets": {
		"fr_filename_vid_ID_off": 7,
		"fr_filename_vid_ID_len": 5,
		"fr_filename_shot_ID_off": 14,
		"fr_filename_shot_ID_len": 5,
		"fr_filename_frame_num_off": 42,
		"fr_filename_frame_num_len": 8
	},

	"kws_file": "data/LSC2020_5days/word2idx.txt",

	"frames_list_file": "data/LSC2020_5days/LSC-5days.keyframes.dataset",
	"frames_dir": "data/ITEC_w2vv/frames/",
	"thumbs_dir": "data/ITEC_w2vv/thumbs/",

	"features_file_data_off": 0,
	"features_dim": 128,
	"features_file": "data/LSC2020_5days/LSC-5days.w2vv.bin",
	
	"kw_bias_vec_file": "data/LSC2020_5days/txt_bias-2048floats.bin",
	"kw_scores_mat_file": "data/LSC2020_5days/txt_weight-11147x2048floats.bin",

	"kw_PCA_mat_dim": 128,
	"pre_PCA_features_dim": 2048,
	"kw_PCA_mean_vec_file": "data/LSC2020_5days/LSC-5days.w2vv.pca.mean.bin",
	"kw_PCA_mat_file": "data/LSC2020_5days/LSC-5days.w2vv.pca.matrix.bin",
	
  "LSC_metadata_file": "data/LSC2020_5days/lsc2020-metadata.csv",

	"model_W2VV_img_bias": "data/LSC2020_5days/nn_models/w2vv-img_bias-2048floats.bin",
  "model_W2VV_img_weigths": "data/LSC2020_5days/nn_models/w2vv-img_weight-2048x4096floats.bin",
	"model_ResNet_file": "data/LSC2020_5days/nn_models/traced_Resnet152.pt",
	"model_ResNext_file": "data/LSC2020_5days/nn_models/traced_Resnext101.pt"
  
}}
)";

void TESTER_Config::run_all_tests(const std::string & /*cfg_fpth*/) {
	SHLOG("====================================================");
	SHLOG("\tInitializing the `(Settings` struct tests...");
	SHLOG("====================================================");

	// Parse config file
	Settings config = Settings::parse_JSON_config_string(json_contents);

	TEST_parse_JSON_config(config);
	TEST_LSC_addition(config);

	SHLOG("====================================================");
	SHLOG("\tIf you got here, all `Config` tests were OK...");
	SHLOG("====================================================");
}

void TESTER_Config::TEST_parse_JSON_config(const Settings &c) {
	SHLOG("\t Testing `Config::parse_JSON_config`...");

	const auto &sbc{ c.eval_server };
	do_assert(sbc.submit_LSC_IDs == true, "Incorrect parse.");
	
	do_assert(sbc.team_ID == 4_z, "Incorrect parse.");
	do_assert(sbc.member_ID == 1_z, "Incorrect parse.");
	do_assert(sbc.log_dir_eval_server_requests == "logs/submitted_logs/", "Incorrect parse.");
	do_assert(sbc.log_dir_user_actions == "logs/actions/", "Incorrect parse.");
	do_assert(sbc.log_dir_user_actions_summary == "logs/collages/", "Incorrect parse.");
	do_assert(sbc.log_dir_results == "logs/requests/", "Incorrect parse.");
	do_assert(sbc.log_file_suffix == ".json", "Incorrect parse.");
	do_assert(sbc.extra_verbose_log == false, "Incorrect parse.");
	do_assert(sbc.send_logs_to_server_period == 10000_z, "Incorrect parse.");
	do_assert(sbc.apply_log_action_timeout == false, "Incorrect parse.");
	do_assert(sbc.log_action_timeout == 500_z, "Incorrect parse.");
	do_assert(sbc.server_type == "dres", "Incorrect parse.");

	const auto &sc{ std::get<EvalServerSettings::ServerConfigDres>(c.eval_server.server_cfg) };
	do_assert(sc.submit_URL == "http://localhost:8080/submit", "Incorrect parse.");
	do_assert(sc.submit_rerank_URL == "http://localhost:8080/log/result", "Incorrect parse.");
	do_assert(sc.submit_interaction_URL == "http://localhost:8080/log/query", "Incorrect parse.");
	do_assert(sc.cookie_file == "cookie.txt", "Incorrect parse.");
	do_assert(sc.login_URL == "http://localhost:8080/api/login", "Incorrect parse.");
	do_assert(sc.username == "admin", "Incorrect parse.");
	do_assert(sc.password == "adminadmin", "Incorrect parse.");

	//do_assert(c.filename_offsets.vid_ID_off == 7_z, "Incorrect parse.");
	//do_assert(c.filename_offsets.vid_ID_len == 5_z, "Incorrect parse.");
	//do_assert(c.filename_offsets.shot_ID_off == 14_z, "Incorrect parse.");
	//do_assert(c.filename_offsets.shot_ID_len == 5_z, "Incorrect parse.");
	//do_assert(c.filename_offsets.frame_num_off == 42_z, "Incorrect parse.");
	//do_assert(c.filename_offsets.frame_num_len == 8_z, "Incorrect parse.");

	//do_assert(c.frames_list_file == "data/LSC2020_5days/LSC-5days.keyframes.dataset", "Incorrect parse.");
	//do_assert(c.frames_dir == "data/ITEC_w2vv/frames/", "Incorrect parse.");
	//do_assert(c.thumbs_dir == "data/ITEC_w2vv/thumbs/", "Incorrect parse.");

	//do_assert(c.features_file_data_off == 0_z, "Incorrect parse.");
	//do_assert(c.features_file == "data/LSC2020_5days/LSC-5days.w2vv.bin", "Incorrect parse.");
	//do_assert(c.features_dim == 128, "Incorrect parse.");

	//do_assert(c.pre_PCA_features_dim == 2048, "Incorrect parse.");
	//do_assert(c.kw_scores_mat_file == "data/LSC2020_5days/txt_weight-11147x2048floats.bin", "Incorrect parse.");
	//do_assert(c.kw_bias_vec_file == "data/LSC2020_5days/txt_bias-2048floats.bin", "Incorrect parse.");
	//do_assert(c.kw_PCA_mean_vec_file == "data/LSC2020_5days/LSC-5days.w2vv.pca.mean.bin", "Incorrect parse.");
	//do_assert(c.kw_PCA_mat_file == "data/LSC2020_5days/LSC-5days.w2vv.pca.matrix.bin", "Incorrect parse.");
	//do_assert(c.kw_PCA_mat_dim == 128, "Incorrect parse.");

	//do_assert(c.kws_file == "data/LSC2020_5days/word2idx.txt", "Incorrect parse.");

	//do_assert(c.display_page_size == 128_z, "Incorrect parse.");
	//do_assert(c.topn_frames_per_video == 3, "Incorrect parse.");
	//do_assert(c.topn_frames_per_shot == 1, "Incorrect parse.");

	SHLOG("\t Finishing `Config::parse_JSON_config`...");
}

void TESTER_Config::TEST_LSC_addition(const Settings &config) {
	SHLOG("\t Testing `Config::parse_JSON_config` for LSC changes...");

	do_assert(config.datasets.LSC_metadata_file == "data/LSC2020_5days/lsc2020-metadata.csv", "Incorrect parse.");

	SHLOG("\t Finishing `Config::parse_JSON_config` for LSC changes...");
}

void TESTER_Config::TEST_collage_addition(const Settings &config) {
	SHLOG("\t Testing `Config::parse_JSON_config` for collage changes...");

	do_assert(config.models.model_W2VV_img_bias == "data/LSC2020_5days/nn_models/w2vv-img_bias-2048floats.bin", "Incorrect parse.");
	do_assert(config.models.model_W2VV_img_weigths == "data/LSC2020_5days/nn_models/w2vv-img_weight-2048x4096floats.bin", "Incorrect parse.");
	do_assert(config.models.model_ResNet_file == "data/LSC2020_5days/nn_models/traced_Resnet152.pt", "Incorrect parse.");
	do_assert(config.models.model_ResNext_file == "data/LSC2020_5days/nn_models/traced_Resnext101.pt", "Incorrect parse.");

	SHLOG("\t Finishing `Config::parse_JSON_config` for collage changes...");
}

// clang-format on
