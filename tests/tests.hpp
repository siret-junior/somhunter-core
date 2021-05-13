
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

// clang-format off

#include <stack>
#include <string>

#include "SomHunter.h"
#include "config_json.h"
#include "log.h"
#include "utils.h"

namespace sh {
namespace tests {


// clang-format off

class TESTER_SomHunter
{
public:
	static void run_all_tests(const std::string &cfg_fpth)
	{
		SHLOG("====================================================");
		SHLOG("\tInitializing the `SomHunter` class tests...");
		SHLOG("====================================================");

		// Parse config file
		auto config = Config::parse_json_config(cfg_fpth);

		// Make sure that values are right for these tests
		config.topn_frames_per_video = 3;
		config.topn_frames_per_shot = 1;

		// Instantiate the SOMHunter
		SomHunter core{ config, cfg_fpth };

		SHLOG("Running all the SomHunter class tests...");

		TEST_like_frames(core);
		TEST_bookmark_frames(core);
		TEST_autocomplete_keywords(core);
		TEST_rescore(core);

#ifdef TEST_FILTERS
		TEST_rescore_filters(core);
#endif

		SHLOG("====================================================");
		SHLOG("\tIf you got here, all `SomHunter` tests were OK...");
		SHLOG("====================================================");
	}

private:
	static void TEST_like_frames(SomHunter &core)
	{
		SHLOG("\t Testing `SomHunter::like_frames` method...");

		auto [disp, likes, bookmarks]{ core.get_display(DisplayType::DTopN, 0, 0) };
		size_t size{ disp.size() };
		do_assert(size > 0, "Top N display is empty!");

		auto ff = *(disp.begin());
		auto fm = *(disp.begin() + utils::irand(1_z, size - 1));
		auto fl = *(--(disp.end()));

		using vec = std::vector<ImageId>;

		core.like_frames(vec{ ff->frame_ID });
		do_assert(likes.count(ff->frame_ID) == 1,
		       "Frame SHOULD be liked.");
		core.like_frames(std::vector<ImageId>{ ff->frame_ID });
		do_assert(likes.count(ff->frame_ID) == 0,
		       "Frame SHOULD NOT be liked.");

		core.like_frames(vec{ fm->frame_ID });
		do_assert(likes.count(fm->frame_ID) == 1,
		       "Frame SHOULD be liked.");
		core.like_frames(std::vector<ImageId>{ fm->frame_ID });
		do_assert(likes.count(fm->frame_ID) == 0,
		       "Frame SHOULD NOT be liked.");

		core.like_frames(vec{ fl->frame_ID });
		do_assert(likes.count(fl->frame_ID) == 1,
		       "Frame SHOULD be liked.");
		core.like_frames(std::vector<ImageId>{ fl->frame_ID });
		do_assert(likes.count(fl->frame_ID) == 0,
		       "Frame SHOULD NOT be liked.");

		vec all;
		for (auto &&f : disp) {
			all.emplace_back(f->frame_ID);
		}
		core.like_frames(all);
		do_assert(likes.size() == size, "All frames SHOULD be liked.");

		core.like_frames(all);
		do_assert(likes.size() == 0, "All frames SHOULD NOT be liked.");

		SHLOG("\t Testing `SomHunter::like_frames` finished.");
	}

	static void TEST_bookmark_frames(SomHunter& core)
	{
		SHLOG("\t Testing `SomHunter::bookmark_frames` method...");

		auto [disp, likes, bookmarks] { core.get_display(DisplayType::DTopN, 0, 0) };
		size_t size{ disp.size() };
		do_assert(size > 0, "Top N display is empty!");

		auto ff = *(disp.begin());
		auto fm = *(disp.begin() + utils::irand(1_z, size - 1));
		auto fl = *(--(disp.end()));

		using vec = std::vector<ImageId>;

		core.bookmark_frames(vec{ ff->frame_ID });
		do_assert(bookmarks.count(ff->frame_ID) == 1,
			"Frame SHOULD be bookmarked.");
		core.bookmark_frames(std::vector<ImageId>{ ff->frame_ID });
		do_assert(bookmarks.count(ff->frame_ID) == 0,
			"Frame SHOULD NOT be bookmarked.");

		core.bookmark_frames(vec{ fm->frame_ID });
		do_assert(bookmarks.count(fm->frame_ID) == 1,
			"Frame SHOULD be bookmarked.");
		core.bookmark_frames(std::vector<ImageId>{ fm->frame_ID });
		do_assert(bookmarks.count(fm->frame_ID) == 0,
			"Frame SHOULD NOT be bookmarked.");

		core.bookmark_frames(vec{ fl->frame_ID });
		do_assert(bookmarks.count(fl->frame_ID) == 1,
			"Frame SHOULD be bookmarked.");
		core.bookmark_frames(std::vector<ImageId>{ fl->frame_ID });
		do_assert(bookmarks.count(fl->frame_ID) == 0,
			"Frame SHOULD NOT be bookmarked.");

		vec all;
		for (auto&& f : disp) {
			all.emplace_back(f->frame_ID);
		}
		core.bookmark_frames(all);
		do_assert(bookmarks.size() == size, "All frames SHOULD be bookmarked.");

		core.bookmark_frames(all);
		do_assert(bookmarks.size() == 0, "All frames SHOULD NOT be bookmarked.");

		SHLOG("\t Testing `SomHunter::bookmark_frames` finished.");
	}

	static void TEST_autocomplete_keywords(SomHunter &core)
	{
		SHLOG(
		  "\t Testing `SomHunter::autocomplete_keywords` method...");

		/*
		 * Non-empty cases
		 */
#ifdef TESTING_BOW_W2VV
		std::map<std::string, std::vector<KeywordId>> correct{
			{ "cat", { 44, 7725, 8225, 9712 } },
			{ "z", { 1615, 9127, 8767, 4316 } }
		};
#else
		std::vector<KeywordId> correct{};
		SHLOG_E("No test values for this dataset.");
#endif
		for (auto &&[key, val] : correct) {
			auto ac_res{ core.autocomplete_keywords(key, 10) };

			for (size_t i{ 0 }; i < val.size(); ++i) {
				do_assert(ac_res[i]->synset_ID == val[i],
				       "Incorrect keyword");
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

		SHLOG(
		  "\t Testing `SomHunter::autocomplete_keywords` finished.");
	}

	static void TEST_rescore(SomHunter &core)
	{
		SHLOG("\t Testing `SomHunter::TEST_rescore` method...");

		FramePointerRange disp{};

		/*
		 * #1 Text
		 */
		auto h{ core.rescore("cat").history };
		auto state1{ core.user.ctx };
		do_assert(h.back() == core.user.ctx, "Inconsistent data.");

#ifdef TESTING_ITEC_DATASET
		disp = core.get_display(DisplayType::DTopN, 0, 0).frames;
		do_assert(disp[0]->frame_ID == 80,
		       "Incorrect frame in the display.");
		do_assert(disp[1]->frame_ID == 130,
		       "Incorrect frame in the display.");

#endif

		/*
		 * #2 Temporal text
		 */
		core.like_frames(std::vector<ImageId>{ 80 });
		h = core.rescore("dog catalog >> habitat ").history;
		auto state2{ core.user.ctx };
		do_assert(h.back() == core.user.ctx, "Inconsistent data.");

#ifdef TESTING_ITEC_DATASET
		disp = core.get_display(DisplayType::DTopN, 0, 0).frames;
		do_assert(disp[0]->frame_ID == 74,
		       "Incorrect frame in the display.");
		do_assert(disp[1]->frame_ID == 224,
		       "Incorrect frame in the display.");

		do_assert(disp[7]->frame_ID == 109,
		       "Incorrect frame in the display.");
		do_assert(disp[8]->frame_ID == 35,
		       "Incorrect frame in the display.");
#endif

		/*
		 * #3: Text & likes
		 */
		core.like_frames(std::vector<ImageId>{ 187 });
		core.like_frames(std::vector<ImageId>{ 217 });
		core.like_frames(std::vector<ImageId>{ 581 });
		h = core.rescore("chicken").history;
		auto state3{ core.user.ctx };
		do_assert(h.back() == core.user.ctx, "Inconsistent data.");
		do_assert(h.back().likes.size() == 0,
		       "Likes should be reset with rescore.");

#ifdef TESTING_ITEC_DATASET
		disp = core.get_display(DisplayType::DTopN, 0, 0).frames;
		do_assert(disp[0]->frame_ID == 489,
		       "Incorrect frame in the display.");
		do_assert(disp[1]->frame_ID == 221,
		       "Incorrect frame in the display.");

		do_assert(disp[7]->frame_ID == 372,
		       "Incorrect frame in the display.");
		do_assert(disp[8]->frame_ID == 267,
		       "Incorrect frame in the display.");
#endif

		/*
		 * #4: `conext_switch`
		 */
		core.switch_search_context(1);
		do_assert(state1 == core.user.ctx, "State SHOULD BE equal.");
#ifdef TESTING_ITEC_DATASET
		disp = core.get_display(DisplayType::DTopN, 0, 0).frames;
		do_assert(disp[0]->frame_ID == 80,
		       "Incorrect frame in the display.");
		do_assert(disp[1]->frame_ID == 130,
		       "Incorrect frame in the display.");
#endif

		core.switch_search_context(2);
		do_assert(state2 == core.user.ctx, "State SHOULD BE equal.");
#ifdef TESTING_ITEC_DATASET
		disp = core.get_display(DisplayType::DTopN, 0, 0).frames;
		do_assert(disp[0]->frame_ID == 74,
			"Incorrect frame in the display.");
		do_assert(disp[1]->frame_ID == 224,
			"Incorrect frame in the display.");

		do_assert(disp[7]->frame_ID == 109,
			"Incorrect frame in the display.");
		do_assert(disp[8]->frame_ID == 35,
			"Incorrect frame in the display.");
#endif

		core.switch_search_context(3);
		do_assert(state3 == core.user.ctx, "State SHOULD BE equal.");
#ifdef TESTING_ITEC_DATASET
		disp = core.get_display(DisplayType::DTopN, 0, 0).frames;
		do_assert(disp[0]->frame_ID == 489,
			"Incorrect frame in the display.");
		do_assert(disp[1]->frame_ID == 221,
			"Incorrect frame in the display.");

		do_assert(disp[7]->frame_ID == 372,
			"Incorrect frame in the display.");
		do_assert(disp[8]->frame_ID == 267,
			"Incorrect frame in the display.");
#endif

		SHLOG("\t Testing `SomHunter::TEST_rescore` finished.");
	}

	static void TEST_rescore_filters(SomHunter& core)
	{
		SHLOG("\t Testing `SomHunter::TEST_rescore` score filter...");

		std::vector<std::tuple<std::string, Hour, Hour, uint8_t>> input{
			{ "", Hour(0), Hour(24), uint8_t(0x00) }, // Empty result
			{ "cat", Hour(0), Hour(0), uint8_t(0x3F) }, // Only 00:xx 
			{ "", Hour(9), Hour(17), uint8_t(0x01) }, // 9-17h, mondays
			{ "", Hour(17), Hour(17), uint8_t(0x01) }, // 9-17h, mondays
			{ "", Hour(12), Hour(12), uint8_t(0x03) }, // 12h, mondays, wednesdays
			{ "", Hour(0), Hour(24), uint8_t(0x03) }, // all hours, mondays, wednesdays, fridays
		};

		std::vector<std::vector<ImageId>> spec_tests;
#ifdef TESTING_LSC5DAYS_DATASET
		spec_tests.emplace_back(std::vector<ImageId>{});
		spec_tests.emplace_back(std::vector<ImageId>{ 1769 });
		spec_tests.emplace_back(std::vector<ImageId>{ 356 });
		spec_tests.emplace_back(std::vector<ImageId>{ 356 });
		spec_tests.emplace_back(std::vector<ImageId>{ 752, 193 });
		spec_tests.emplace_back(std::vector<ImageId>{ 977,508 });
#endif

		size_t t_i{ 0 };
		for (auto&& [tq, fr, to, days_mask] : input) {

			Filters fs{
				TimeFilter{fr,to},
				WeekDaysFilter{days_mask}
			};

			core.rescore(tq, &fs);
			auto disp{ core.get_topn_display(0) };

			size_t i{ 0 };
			for (auto&& f : disp) {
				do_assert(fr <= f->hour && f->hour <= to, "Should not be in the result.");
				do_assert(utils::is_set(days_mask, f->weekday), "Should not be in the result.");

				// Dataset specific tests
				if (spec_tests[t_i].size() > i) {
					do_assert(spec_tests[t_i][i] == f->frame_ID, "Incorrect frame ID.");
				}

				//std::cout << "H: " << size_t(f->hour) << " WD: " << size_t(f->weekday) << ", ID: " << f->frame_ID << std::endl;
				++i;
			}
			
			// Empty case
			if (spec_tests[t_i].empty()) {
				do_assert(disp.begin() == disp.end(), "Should be empty.");
			}

			//std::cout << "\n ================================== \n" << std::endl;
			++t_i;
		}

		SHLOG("\t Testing `SomHunter::TEST_rescore` score filter finished...");
	}

};

const char *json_contents = R"(
{ "core": {
  "user_token": "admin",
  "submitter_config":{
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
	  
		"log_submitted_dir": "logs/submitted_logs/",
		"log_actions_dir": "logs/actions/",
		"log_queries_dir": "logs/collages/",
		"log_requests_dir": "logs/requests/",
		"log_file_suffix": ".json",
		"extra_verbose_log": false,
	  
		"send_logs_to_server_period": 10000,
		"apply_log_action_timeout_in_core": false,
		"log_action_timeout": 500
	},

	"max_frame_filename_len": 64,
	"display_page_size": 128,
	"topn_frames_per_video": 3,
	"topn_frames_per_shot": 1,
	
	"filename_offsets": {
		"fr_filename_off": 6,
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

class TESTER_Config
{
public:
	static void run_all_tests(const std::string &/*cfg_fpth*/)
	{
		SHLOG("====================================================");
		SHLOG("\tInitializing the `Config` struct tests...");
		SHLOG("====================================================");

		// Parse config file
		Config config = Config::parse_json_config_string(json_contents);

		TEST_parse_json_config(config);
		TEST_LSC_addition(config);

		SHLOG("====================================================");
		SHLOG("\tIf you got here, all `Config` tests were OK...");
		SHLOG("====================================================");
	}

private:
	static void TEST_parse_json_config(const Config &c)
	{
		SHLOG("\t Testing `Config::parse_json_config`...");

		do_assert(c.user_token == "admin", "Incorrect parse.");

		const auto &sbc{ c.submitter_config };
		do_assert(sbc.submit_to_VBS == true, "Incorrect parse.");
		do_assert(sbc.team_ID == 4_z, "Incorrect parse.");
		do_assert(sbc.member_ID == 1_z, "Incorrect parse.");
		do_assert(sbc.log_submitted_dir == "logs/submitted_logs/", "Incorrect parse.");
		do_assert(sbc.log_actions_dir == "logs/actions/", "Incorrect parse.");
		do_assert(sbc.log_queries_dir == "logs/collages/", "Incorrect parse.");
		do_assert(sbc.log_requests_dir == "logs/requests/", "Incorrect parse.");
		do_assert(sbc.log_file_suffix == ".json", "Incorrect parse.");
		do_assert(sbc.extra_verbose_log == false, "Incorrect parse.");
		do_assert(sbc.send_logs_to_server_period == 10000_z, "Incorrect parse.");
		do_assert(sbc.apply_log_action_timeout == false, "Incorrect parse.");
		do_assert(sbc.log_action_timeout == 500_z, "Incorrect parse.");
		do_assert(sbc.server_type == "dres", "Incorrect parse.");

		const auto &sc{ std::get<ServerConfigDres>(c.submitter_config.server_cfg) };
		do_assert(sc.submit_URL == "http://localhost:8080/submit", "Incorrect parse.");
		do_assert(sc.submit_rerank_URL == "http://localhost:8080/log/result", "Incorrect parse.");
		do_assert(sc.submit_interaction_URL == "http://localhost:8080/log/query", "Incorrect parse.");
		do_assert(sc.cookie_file == "cookie.txt", "Incorrect parse.");
		do_assert(sc.login_URL == "http://localhost:8080/api/login", "Incorrect parse.");
		do_assert(sc.username == "admin", "Incorrect parse.");
		do_assert(sc.password == "adminadmin", "Incorrect parse.");

		do_assert(c.max_frame_filename_len == 64_z, "Incorrect parse.");

		do_assert(c.filename_offsets.filename_off == 6_z, "Incorrect parse.");
		do_assert(c.filename_offsets.vid_ID_off == 7_z,"Incorrect parse.");
		do_assert(c.filename_offsets.vid_ID_len == 5_z, "Incorrect parse.");
		do_assert(c.filename_offsets.shot_ID_off == 14_z, "Incorrect parse.");
		do_assert(c.filename_offsets.shot_ID_len == 5_z, "Incorrect parse.");
		do_assert(c.filename_offsets.frame_num_off == 42_z, "Incorrect parse.");
		do_assert(c.filename_offsets.frame_num_len == 8_z, "Incorrect parse.");

		do_assert(c.frames_list_file == "data/LSC2020_5days/LSC-5days.keyframes.dataset", "Incorrect parse.");
		do_assert(c.frames_dir == "data/ITEC_w2vv/frames/", "Incorrect parse.");
		do_assert(c.thumbs_dir == "data/ITEC_w2vv/thumbs/", "Incorrect parse.");

		do_assert(c.features_file_data_off == 0_z, "Incorrect parse.");
		do_assert(c.features_file == "data/LSC2020_5days/LSC-5days.w2vv.bin", "Incorrect parse.");
		do_assert(c.features_dim == 128, "Incorrect parse.");

		do_assert(c.pre_PCA_features_dim == 2048, "Incorrect parse.");
		do_assert(c.kw_scores_mat_file == "data/LSC2020_5days/txt_weight-11147x2048floats.bin", "Incorrect parse.");
		do_assert(c.kw_bias_vec_file == "data/LSC2020_5days/txt_bias-2048floats.bin", "Incorrect parse.");
		do_assert(c.kw_PCA_mean_vec_file == "data/LSC2020_5days/LSC-5days.w2vv.pca.mean.bin", "Incorrect parse.");
		do_assert(c.kw_PCA_mat_file == "data/LSC2020_5days/LSC-5days.w2vv.pca.matrix.bin", "Incorrect parse.");
		do_assert(c.kw_PCA_mat_dim == 128, "Incorrect parse.");

		do_assert(c.kws_file == "data/LSC2020_5days/word2idx.txt", "Incorrect parse.");

		do_assert(c.display_page_size == 128_z, "Incorrect parse.");
		do_assert(c.topn_frames_per_video == 3, "Incorrect parse.");
		do_assert(c.topn_frames_per_shot == 1, "Incorrect parse.");

		SHLOG("\t Finishing `Config::parse_json_config`...");
	}

	static void TEST_LSC_addition(const Config &config)
	{
		SHLOG("\t Testing `Config::parse_json_config` for LSC changes...");

		do_assert(config.LSC_metadata_file == "data/LSC2020_5days/lsc2020-metadata.csv", "Incorrect parse.");

		SHLOG("\t Finishing `Config::parse_json_config` for LSC changes...");
	}

	static void TEST_collage_addition(const Config &config)
	{
		SHLOG("\t Testing `Config::parse_json_config` for collage changes...");

		do_assert(config.model_W2VV_img_bias == "data/LSC2020_5days/nn_models/w2vv-img_bias-2048floats.bin", "Incorrect parse.");
		do_assert(config.model_W2VV_img_weigths == "data/LSC2020_5days/nn_models/w2vv-img_weight-2048x4096floats.bin", "Incorrect parse.");
		do_assert(config.model_ResNet_file == "data/LSC2020_5days/nn_models/traced_Resnet152.pt", "Incorrect parse.");
		do_assert(config.model_ResNext_file == "data/LSC2020_5days/nn_models/traced_Resnext101.pt", "Incorrect parse.");

		SHLOG("\t Finishing `Config::parse_json_config` for collage changes...");
	}
};

// clang-format on

};  // namespace tests
};  // namespace sh