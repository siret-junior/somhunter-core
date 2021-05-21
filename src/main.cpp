
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

#include <stdio.h>
#include <chrono>
#include <filesystem>
#include <thread>
#include <vector>

#if defined WIN32 || defined _WIN32 || defined WIN64 || defined _WIN64
#	include "Windows.h"
#endif  // defined WIN32 || defined _WIN32 || defined WIN64 || defined _WIN64

// !!!
#include "SomHunter.h"  // Do not move this beloe other includes
                        // -> otherwise the libtorch compile error wil bite you
// !!!

#include "config-tests.h"  //< Comment this to disable testing behaviour

#include "log.h"
#include "utils.h"

#include "NetworkApi.h"

using namespace sh;

/**
 * Does the global application initialization.
 */
static void initialize_aplication();

int main() {
	initialize_aplication();

#if 0
	Query q{ utils::deserialize_from_file<CanvasQuery>("cq.bin") };
	core.rescore(q);
	CanvasSubqueryBitmap& cq{ std::get<CanvasSubqueryBitmap>(q.canvas_query[1]) };
	auto img = cq.data();
	auto img2 = cq.data_std();

	auto img_new{ ImageManipulator::resize(img, cq.width_pixels(), cq.height_pixels(), 224, 224, 3) };
	// auto img2_new {ImageManipulator::resize(img2, cq.width_pixels(), cq.height_pixels(), 224, 224, 3)};

	{  // *** PNGs ***
		cv::Mat cv_img{ ImageManipulator::load_image<cv::Mat>(TEST_PNGS[0]) };
		BitmapImage<uint8_t> std_img_u8{ ImageManipulator::load_image<BitmapImage<uint8_t>>(TEST_PNGS[0]) };
		BitmapImage<float> std_img_f32{ ImageManipulator::load_image<BitmapImage<float>>(TEST_PNGS[0]) };

		ImageManipulator::show_image(TEST_PNGS[0]);
		ImageManipulator::show_image(cv_img);
	}

	{  // *** JPEGs ***
		cv::Mat cv_img{ ImageManipulator::load_image<cv::Mat>(TEST_JPEGS[0]) };
		BitmapImage<uint8_t> std_img_u8{ ImageManipulator::load_image<BitmapImage<uint8_t>>(TEST_JPEGS[0]) };
		BitmapImage<float> std_img_f32{ ImageManipulator::load_image<BitmapImage<float>>(TEST_JPEGS[0]) };

		ImageManipulator::show_image(TEST_JPEGS[0]);
		ImageManipulator::show_image(cv_img);
	}
#endif
	const std::string cfg_fpth{ "config/config-core.json" };

	// Parse config file
	auto config = Config::parse_json_config(cfg_fpth);
	// Instantiate the SOMHunter
	SomHunter core{ config, cfg_fpth };

	// core.benchmark_native_text_queries(R"(data\v3c1-20k\native-queries.csv)", "bench-out");

	std::vector<size_t> ranks;

#if 0  // Run CanvasQuery benchmark
	// #####################################
	// Run the serialized Canvas query

	using directory_iterator = std::filesystem::directory_iterator;

	std::vector<std::string> serialized_queries;
	for (const auto& dirEntry : directory_iterator("saved-queries")) {
		std::cout << dirEntry << std::endl;

		for (const auto& file : std::filesystem::directory_iterator(dirEntry)) {
			std::string filepath{ file.path().string() };

			std::string suffix{ filepath.substr(filepath.length() - 3) };
			if (suffix == "bin") {
				serialized_queries.emplace_back(filepath);
			}
			std::cout << "\t" << file << std::endl;
		}
	}

	size_t q_idx{0};

	for (auto&& f : serialized_queries) {
		std::cout << "Running query from '" << f << "' file..." << std::endl;

		Query q{ utils::deserialize_from_file<CanvasQuery>(f) };
		auto targets = q.canvas_query.get_targets();

		// !!!!!
		q.transform_to_no_pos_queries();
		// !!!!!

		core.rescore(q);
		auto disp = core.get_display(DisplayType::DTopN, 0, 0).frames;

		size_t res = 0;

		

		size_t i{ 0 };
		for (auto it{ disp.begin() }; it != disp.end(); ++it) {
			if (targets[0] == (*it)->frame_ID || targets[1] == (*it)->frame_ID) {

				ranks.push_back(i);
				break;
			}
			++i;
		}

		++q_idx;
	}
	// #####################################

	// print it
	//std::sort(ranks.begin(), ranks.end());


	size_t num_ticks{100};
	size_t num_frames{core.get_num_frames()};

	float scale{num_ticks / float(num_frames)};

	std::vector<size_t> hist;
	hist.resize(num_ticks, 0);
	{
		size_t i{ 0 };
		for (auto&& r : ranks) {
			size_t scaled_rank{static_cast<size_t>(r * scale)};
			++hist[scaled_rank];

			++i;
		}
	}

	size_t acc{0};
	for (auto& i : hist) {
		auto xx = i;
		i = acc;
		acc += xx;
	}

	for (size_t i = 0; i < hist.size(); i++) {
		std::cout << i << ","<< hist[i] << std::endl;
	}

#endif  // Run CanvasQuery benchmark

	NetworkApi api{ config.API_config, &core };
	api.run();

#ifdef DO_TESTS
#	if 0  // Serialized CanvasQueries
	using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;

	std::vector<std::string> serialized_queries;
	for (const auto& dirEntry : recursive_directory_iterator("logs/queries/")) {
		std::cout << dirEntry << std::endl;

		for (auto&& file : std::filesystem::directory_iterator(dirEntry)) {
			std::string filepath{ file.path().string() };

			std::string suffix{ filepath.substr(filepath.length() - 3) };
			if (suffix == "bin") {
				serialized_queries.emplace_back(filepath);
			}
			std::cout << "\t" << file << std::endl;
		}
	}

#	endif  // Serialized CanvasQueries

	/* ********************************
	 * Test features here...
	 * ******************************** */

#	if 0
	/* !!!!!!!!!!!!!!!!!!!!!!!!!!
	 * Test collage queries
	 * !!!!!!!!!!!!!!!!!!!!!!!!!! */
	namespace fs = std::filesystem;

	for (auto& p : fs::directory_iterator(TEST_COLLAGE_DATA_DIR)) {
		// Skip directories
		if (p.is_directory()) continue;

		std::cout << "Running collage query from: " << p.path() << std::endl;
		Collage c{ deserialize_from_file<Collage>(p.path().string()) };
		core.rescore(c, nullptr);
	}

#	endif  // TEST_COLLAGE_QUERIES

	// *** SHA file checksum ***
	std::cout << "SHA256: " << utils::SHA256_sum("config/config-core.json") << std::endl;

	// Try autocomplete
	auto ac_res{ core.autocomplete_keywords("Cat", 30) };
	for (auto&& p_kw : ac_res) {
		std::cout << p_kw->synset_strs.front() << std::endl;
	}

	// Try different displays
	{
		core.rescore("dog park");

		auto d_topn = core.get_display(DisplayType::DTopN, 0, 0).frames;
		std::cout << "TOP N\n";
		d_topn.print_display();

		auto d_topknn = core.get_display(DisplayType::DTopKNN, 2, 0).frames;
		std::cout << "TOP KNN\n";
		d_topknn.print_display();

		auto d_rand = core.get_display(DisplayType::DRand).frames;
		std::cout << "RANDOM\n";
		d_rand.print_display();
	}

	// Try keyword rescore
	{
		core.rescore("dog park");
		auto d_topn1 = core.get_display(DisplayType::DTopN, 0, 0).frames;
		std::cout << "TOP N\n";
		d_topn1.print_display();
	}

	// Try reset session
	core.reset_search_session();

	// Try relevance feedback
	{
		auto d_rand1 = core.get_display(DisplayType::DRand).frames;
		std::vector<ImageId> likes;
		auto d_rand_b = d_rand1.begin();
		likes.push_back((*d_rand_b)->frame_ID);
		d_rand_b++;
		likes.push_back((*d_rand_b)->frame_ID);

		core.like_frames(likes);
		likes.resize(1);
		core.like_frames(likes);
		std::cout << "Like " << likes[0] << std::endl;

		core.rescore("\\/?!,.'\"");
	}

	{
		auto d_topn2 = core.get_display(DisplayType::DTopN, 0, 0).frames;
		d_topn2.print_display();
		std::cout << "Len of top n page 0 " << d_topn2.size() << std::endl;
	}
	{
		auto d_topn2 = core.get_display(DisplayType::DTopN, 0, 1).frames;
		std::cout << "Len of top n page 1 " << d_topn2.size() << std::endl;
	}
	{
		auto d_topn2 = core.get_display(DisplayType::DTopN, 0, 2).frames;
		std::cout << "Len of top n page 2 " << d_topn2.size() << std::endl;
	}

	// Try SOM
	{
		while (!core.som_ready()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
		std::cout << "SOM is ready now!" << std::endl;

		auto d_som = core.get_display(DisplayType::DSom);
	}

	SHLOG_E("this is an error log");
	SHLOG_W("this is a warning log");
	SHLOG_I("this is an info log");
	SHLOG_S("this is a success log");
	SHLOG_D("this is a debug log");
	SHLOG_REQ("123.0.0.1", "this is an API request");

	tests::TESTER_SomHunter::run_all_tests(cfg_fpth);
	tests::TESTER_Config::run_all_tests(cfg_fpth);

#endif  // DO_TESTS

	return 0;
}

static void initialize_aplication() {
	// Enable ANSII colored output if not enabled by default
#if defined WIN32 || defined _WIN32 || defined WIN64 || defined _WIN64
	// From: https://superuser.com/a/1529908

#	include "Windows.h"

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = 0;
	GetConsoleMode(hOut, &dwMode);
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(hOut, dwMode);

	// References:
	// SetConsoleMode() and ENABLE_VIRTUAL_TERMINAL_PROCESSING?
	// https://stackoverflow.com/questions/38772468/setconsolemode-and-enable-virtual-terminal-processing

	// Windows console with ANSI colors handling
	// https://superuser.com/questions/413073/windows-console-with-ansi-colors-handling
#endif

	/*	cd to the parent dir (root of the project)
	 *  `cd ..`
	 *	Change this accordingly. 	*/
	auto path = std::filesystem::current_path();
	std::filesystem::current_path(path.parent_path());
	SHLOG_I("The binary is running from the directory " << std::filesystem::current_path() << "...");
}