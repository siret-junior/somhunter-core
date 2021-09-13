
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

#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <thread>
#include <vector>

#if defined WIN32 || defined _WIN32 || defined WIN64 || defined _WIN64
#	include "Windows.h"
#endif  // defined WIN32 || defined _WIN32 || defined WIN64 || defined _WIN64

// !!!
#include "somhunter.h"  // Do not move this beloe other includes
                        // -> otherwise the libtorch compile error will (or may) bite you
#include "network-api.h"
// !!!

#include "common.h"
#include "utils.hpp"

#include "tests.h"

using namespace sh;

namespace sh
{
/**
 * Does the global application initialization.
 */
static void initialize_aplication();
};  // namespace sh

int main()
{
	initialize_aplication();

	// Instantiate the SOMHunter
	Somhunter core{ "config/config-core.json" };

	/* ***
	 * Benchmarks
	 */
	// core.benchmark_native_text_queries(R"(data\v3c1-20k\native-queries.csv)", "bench-out");
	// core.benchmark_canvas_queries("saved-queries", "saved-queries-out");
	// core.benchmark_real_queries("data-logs", "data-logs/tasks.csv", "saved-queries-out");
	// std::cout << "DONE!" << std::endl;
	// return 0;

	NetworkApi api{ core.settings().API, &core };
	api.run();

#if 0
	/* ***
	 * Test features here...
	 */

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

	// *** SHA file checksum ***
	std::cout << "SHA256: " << utils::SHA256_sum("config/config-core.json") << std::endl;

	// Try autocomplete
	auto ac_res{ core.autocomplete_keywords("Cat", 30) };
	for (auto&& p_kw : ac_res) {
		std::cout << p_kw->synset_strs.front() << std::endl;
	}

	// Try different displays
	{
		Query q{ std::vector({ "dog park" }) };
		core.rescore(q);

		auto d_topn = core.get_display(DisplayType::DTopN, 0, 0)._dataset_frames;
		std::cout << "TOP N\n";
		d_topn.print_display();

		auto d_topknn = core.get_display(DisplayType::DTopKNN, 2, 0)._dataset_frames;
		std::cout << "TOP KNN\n";
		d_topknn.print_display();

		auto d_rand = core.get_display(DisplayType::DRand)._dataset_frames;
		std::cout << "RANDOM\n";
		d_rand.print_display();
	}

	// Try keyword rescore
	{
		Query q{ std::vector({ "dog park" }) };
		core.rescore(q);
		auto d_topn1 = core.get_display(DisplayType::DTopN, 0, 0)._dataset_frames;
		std::cout << "TOP N\n";
		d_topn1.print_display();
	}

	// Try reset session
	core.reset_search_session();

	// Try relevance feedback
	{
		auto d_rand1 = core.get_display(DisplayType::DRand)._dataset_frames;
		std::vector<FrameId> likes;
		auto d_rand_b = d_rand1.begin();
		likes.push_back((*d_rand_b)->frame_ID);
		d_rand_b++;
		likes.push_back((*d_rand_b)->frame_ID);

		core.like_frames(likes);
		likes.resize(1);
		core.like_frames(likes);
		std::cout << "Like " << likes[0] << std::endl;

		Query q{ std::vector({ "\\/?!,.'\"" }) };
		core.rescore(q);
	}

	{
		auto d_topn2 = core.get_display(DisplayType::DTopN, 0, 0)._dataset_frames;
		d_topn2.print_display();
		std::cout << "Len of top n page 0 " << d_topn2.size() << std::endl;
	}
	{
		auto d_topn2 = core.get_display(DisplayType::DTopN, 0, 1)._dataset_frames;
		std::cout << "Len of top n page 1 " << d_topn2.size() << std::endl;
	}
	{
		auto d_topn2 = core.get_display(DisplayType::DTopN, 0, 2)._dataset_frames;
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
#endif

	return 0;
}

static void sh::initialize_aplication()
{
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
	std::cout << path << std::endl;
	std::filesystem::current_path(path.parent_path());

	SHLOG("ISA capibilites:");

#if __SSE__
	SHLOG("__SSE__: true");
#else
	SHLOG("__SSE__: false");
#endif  // __SSE__

#if __SSE2__
	SHLOG("__SSE2__: true");
#else
	SHLOG("__SSE2__: false");
#endif  // __SSE2__

#if __SSE3__
	SHLOG("__SSE3__: true");
#else
	SHLOG("__SSE3__: false");
#endif  // __SSE3__

#if __SSE4_2__
	SHLOG("__SSE4_2__ : true");
#else
	SHLOG("__SSE4_2__ : false");
#endif  // __SSE4_2__

#if __AVX__
	SHLOG("__AVX__: true");
#else
	SHLOG("__AVX__: false");
#endif  // __AVX__

#if __AVX2__
	SHLOG("__AVX2__: true");
#else
	SHLOG("__AVX2__: false");
#endif  // __AVX2__

#if __AVX512BW__
	SHLOG("__AVX512BW__ : true");
#else
	SHLOG("__AVX512BW__ : false");
#endif  // __AVX512BW__

#if __AVX512CD__
	SHLOG("__AVX512CD__  : true");
#else
	SHLOG("__AVX512CD__  : false");
#endif  // __AVX512CD__

#if __AVX512DQ__
	SHLOG("__AVX512DQ__  : true");
#else
	SHLOG("__AVX512DQ__  : false");
#endif  // __AVX512DQ__

#if __AVX512F__
	SHLOG("__AVX512F__  : true");
#else
	SHLOG("__AVX512F__  : false");
#endif  // __AVX512F__

#if __AVX512VL__
	SHLOG("__AVX512VL__  : true");
#else
	SHLOG("__AVX512VL__  : false");
#endif  // __AVX512VL__

	SHLOG_I("The binary is running from the directory " << std::filesystem::current_path() << "...");
}