
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

#if defined WIN32 || defined _WIN32 || defined WIN64 || defined _WIN64
#	include "Windows.h"
#endif  // defined WIN32 || defined _WIN32 || defined WIN64 || defined _WIN64

// !!!
#include "SomHunter.h"  // Do not move this beloe other includes
                        // -> otherwise the libtorch compile error wil bite you
// !!!

#include "NetworkApi.h"
#include "utils.h"

using namespace sh;

// If the `TESTER_SomHunter` should do its job.
//#define RUN_TESTER
//#define TEST_COLLAGE_QUERIES

#define TEST_DATA_DIR "../../tests/data"
#define TEST_COLLAGE_DATA_DIR TEST_DATA_DIR "/collages/"

#ifdef RUN_TESTER

/*
 * What dataset are we testing?
 */
#	define TESTING_ITEC_DATASET
//#	define TESTING_LSC5DAYS_DATASET

/** Run filter tests on on datasets supporting it */
#	ifdef TESTING_LSC5DAYS_DATASET
#		define TEST_FILTERS
#	endif

/*
 * What keywords are we testing?
 */
#	define TESTING_BOW_W2VV

#	include "tests.hpp"

#endif

void print_display(const FramePointerRange& d) {
	for (auto iter = d.begin(); iter != d.end(); iter++) std::cout << (*iter)->frame_ID << std::endl;
}

/**
 * Does the global application initialization.
 */
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
}

int main() {
	initialize_aplication();

	/*	cd to the parent dir (root of the project)
	 *  `cd ..`
	 *	Change this accordingly. 	*/
	auto path = std::filesystem::current_path();
	std::filesystem::current_path(path.parent_path());
	std::cout << "Running from the directory " << std::filesystem::current_path() << "..." << std::endl;

	const std::string cfg_fpth{ "config/config-core.json" };

#ifdef RUN_TESTER
	TESTER_SomHunter::run_all_tests(cfg_fpth);
	TESTER_Config::run_all_tests(cfg_fpth);
#endif

#if 1

	// Parse config file
	auto config = Config::parse_json_config(cfg_fpth);

	// Example of JSON lib from network lib
	/*std::ifstream ifs{ cfg_fpth };
	auto d{ web::json::value::parse(ifs) };
	std::cout << d.as_object()[U("api")].as_object()[U("port")].as_integer() << std::endl;*/

	// Instantiate the SOMHunter
	SomHunter core{ config, cfg_fpth };

	CanvasQuery q;
	q.emplace_back(RelativeRect{ 0.2F, 0.1F, 0.5F, 0.5F }, "Lojza");
	// std::cout << q.to_JSON().dump() << std::endl;
	utils::serialize_to_file(q, "test.bin");

	CanvasQuery qs{ utils::deserialize_from_file<CanvasQuery>("test.bin") };
	// std::cout << qs.to_JSON().dump() << std::endl;

	NetworkApi api{ config.API_config, &core };
	api.run();

	/* ********************************
	 * Test features here...
	 * ******************************** */

	// *** SHA file checksum ***
	std::cout << "SHA256: " << utils::SHA256_sum("config/config-core.json") << std::endl;

	/* !!!!!!!!!!!!!!!!!!!!!!!!!!
	 * Test collage queries
	 * !!!!!!!!!!!!!!!!!!!!!!!!!! */
#	ifdef TEST_COLLAGE_QUERIES
	namespace fs = std::filesystem;

	for (auto& p : fs::directory_iterator(TEST_COLLAGE_DATA_DIR)) {
		// Skip directories
		if (p.is_directory()) continue;

		std::cout << "Running collage query from: " << p.path() << std::endl;
		Collage c{ deserialize_from_file<Collage>(p.path().string()) };
		core.rescore(c, nullptr);
	}

	/* -------------------------------- */
#	endif  // TEST_COLLAGE_QUERIES

	/*
	 * Test ImageManipulator
	 */
#	if 0
	std::string orig_img{ "testimg.jpg" };
	auto img{ core.load_image(orig_img) };

	for (size_t i{ 0 }; i < 10; ++i) {
	  size_t new_w{ irand(10_z, 2000_z) };
	  size_t new_h{ irand(10_z, 2000_z) };

	  std::string new_file{ "testimg_edited_" + std::to_string(i) + ".jpg" };

	  auto tmp_data{ core.resize_image(img.data, img.w, img.h, new_w, new_h, img.num_channels) };
	  core.store_jpg_image(new_file, tmp_data, new_w, new_h, 50, img.num_channels);

	  // Check!
	  auto tmp_img{ core.load_image(new_file) };

	  if (tmp_img.w != new_w)
			throw std::logic_error("Width does not match!");

	  if (tmp_img.h != new_h)
	    throw std::logic_error("Height does not match!");
	}

#	endif

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
		print_display(d_topn);

		auto d_topknn = core.get_display(DisplayType::DTopKNN, 2, 0).frames;
		std::cout << "TOP KNN\n";
		print_display(d_topknn);

		auto d_rand = core.get_display(DisplayType::DRand).frames;
		std::cout << "RANDOM\n";
		print_display(d_rand);
	}

	// Try keyword rescore
	{
		core.rescore("dog park");
		auto d_topn1 = core.get_display(DisplayType::DTopN, 0, 0).frames;
		std::cout << "TOP N\n";
		print_display(d_topn1);
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
		print_display(d_topn2);
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

	LOG_E("this is an error log");
	LOG_W("this is a warning log");
	LOG_I("this is an info log");
	LOG_S("this is a success log");
	LOG_D("this is a debug log");
	LOG_REQUEST("123.0.0.1", "this is an API request");
#endif
	return 0;
}
