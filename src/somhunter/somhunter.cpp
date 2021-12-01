
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
#include <stdexcept>
#include <tuple>

#include "somhunter.h"

#include "common.h"
#include "tests.h"
#include "utils.hpp"

using namespace sh;

GetDisplayResult Somhunter::get_display(DisplayType d_type, FrameId selected_image, PageId page, bool log_it) {
	_user_context._logger.poll();

	auto prev_display{ _user_context.ctx.curr_disp_type };

	FramePointerRange frs{};

	switch (d_type) {
		case DisplayType::DRand:
			frs = get_random_display();
			break;

		case DisplayType::DTopN:
			frs = get_topn_display(page);
			break;

		case DisplayType::DTopNContext:
			frs = get_topn_context_display(page);
			break;

		case DisplayType::DSom:
			frs = get_som_display();
			break;

		case DisplayType::DRelocation:
			frs = get_som_relocation_display(page);
			break;

		case DisplayType::DVideoDetail:
			frs = get_video_detail_display(selected_image, log_it);
			break;

		case DisplayType::DTopKNN:
			frs = get_topKNN_display(selected_image, page);
			break;

		default:
			std::string msg{ "Unsupported display requested: " + std::to_string(int(d_type)) };
			SHLOG_E(msg);
			throw std::runtime_error(msg);
			break;
	}

	auto curr_display{ _user_context.ctx.curr_disp_type };

	/* If we're going back from the nearest neighbours display, we log the results

	    Trigger combos are:
	    - KNN -> TopN
	    - KNN -> TopNContext
	    - KNN -> DRand
	    - KNN -> DSom

	    The rest are popups.
	 */

	const auto& ss{ _settings.presentation_views };

	if ((_user_context._force_result_log) ||
	    (prev_display == DisplayType::DTopKNN &&
	     (curr_display == DisplayType::DTopN || curr_display == DisplayType::DTopNContext ||
	      curr_display == DisplayType::DRand || curr_display == DisplayType::DSom))) {
		const auto& top_n = _user_context.ctx.scores.top_n(_dataset_frames, TOPN_LIMIT, ss.topn_frames_per_video,
		                                                   ss.topn_frames_per_shot);

		_user_context._force_result_log = false;

		_user_context._logger.log_results(
		    _dataset_frames, _user_context.ctx.scores, _user_context.ctx._prev_query.relevance_feeedback,
		    _user_context.ctx.used_tools, _user_context.ctx.curr_disp_type, top_n,
		    _user_context.ctx._prev_query.get_plain_text_query(), ss.topn_frames_per_video, ss.topn_frames_per_shot);
	}

	return GetDisplayResult{ frs, _user_context.ctx.likes, _user_context._bookmarks, _user_context._videos_seen };
}

std::vector<bool> Somhunter::like_frames(const std::vector<FrameId>& new_likes) {
	_user_context._logger.poll();

	// Prepare the result flags vector
	std::vector<bool> res;
	res.reserve(new_likes.size());

	for (auto&& fr_ID : new_likes) {
		// Find the item in the set
		size_t count{ _user_context.ctx.likes.count(fr_ID) };

		// If item is not present (NOT LIKED)
		if (count == 0) {
			// Like it
			_user_context.ctx.likes.insert(fr_ID);
			res.emplace_back(true);

			_user_context._logger.log_like(fr_ID);
		}
		// If the item is present (LIKED)
		else {
			// Unlike it
			_user_context.ctx.likes.erase(fr_ID);
			res.emplace_back(false);

			_user_context._logger.log_unlike(fr_ID);
		}
	}

	return res;
}

std::vector<bool> Somhunter::bookmark_frames(const std::vector<FrameId>& new_bookmarks) {
	_user_context._logger.poll();

	// Prepare the result flags vector
	std::vector<bool> res;
	res.reserve(new_bookmarks.size());

	for (auto&& fr_ID : new_bookmarks) {
		// Find the item in the set
		size_t count{ _user_context._bookmarks.count(fr_ID) };

		// If item is not present (NOT LIKED) -> bookmark it
		if (count == 0) {
			_user_context._bookmarks.insert(fr_ID);
			res.emplace_back(true);
			_user_context._logger.log_bookmark(fr_ID);
			// \todo Log it?
		}
		// If the item is present (LIKED) -> unbookmark it
		else {
			_user_context._bookmarks.erase(fr_ID);
			res.emplace_back(false);
			_user_context._logger.log_unbookmark(fr_ID);

			// \todo Log it?
		}
	}

	return res;
}

std::vector<const Keyword*> Somhunter::autocomplete_keywords(const std::string& prefix, size_t count) const {
	// Trivial case
	if (prefix.empty()) return std::vector<const Keyword*>{};

	auto lowercase_prefix{ utils::to_lowercase(prefix) };

	// Get the keywrods IDs
	auto kw_IDs{ _keyword_ranker.find(lowercase_prefix, count) };

	// Create vector of ptrs to corresponding keyword instances
	std::vector<const Keyword*> res;
	res.reserve(kw_IDs.size());
	for (auto&& kw_ID : kw_IDs) {
		res.emplace_back(&_keyword_ranker[kw_ID.first]);
	}

	return res;
}

bool Somhunter::has_metadata() const { return _settings.datasets.LSC_metadata_file.has_value(); }

void Somhunter::apply_filters() {
	const Filters& filters{ _user_context.ctx.filters };

	// If no filters set up
	if (!has_metadata() && (filters.dataset_parts[0] && filters.dataset_parts[1])) {
		return;
	}

	// Make sure to reset the previous mask on the scores
	_user_context.ctx.scores.reset_mask();

	const auto& days{ filters.days };
	Hour t_from{ filters.time.from };
	Hour t_to{ filters.time.to };
	Year y_from{ filters.years.from };
	Year y_to{ filters.years.to };

	auto ds_valid_interval{ filters.get_dataset_parts_valid_interval(_dataset_frames.size()) };

	// std::cout << "[" << ds_valid_interval.first << ", " << ds_valid_interval.second << ")" << std::endl;

	// A closure that determines if the frame should be filtered out
	auto is_out{ [&days, t_from, t_to, &ds_valid_interval, y_from, y_to](const VideoFrame& f) {
		// If NOT within the selected days
		if (!days[f.weekday]) return true;

		// If NOT within the hour range
		if (t_from > f.hour || f.hour > t_to) return true;

		// If NOT within the years range
		if (y_from > f.year || f.year > y_to) return true;

		// Dataset part filter
		if (!(ds_valid_interval.first <= f.frame_ID && f.frame_ID < ds_valid_interval.second)) return true;

		return false;
	} };

	FrameId frame_ID{ 0 };
	for (auto&& f : _dataset_frames) {
		// If should be filtered out
		if (is_out(f)) {
			_user_context.ctx.scores.set_mask(frame_ID, false);
		}

		++frame_ID;
	}
}

void Somhunter::run_basic_test() {
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
	std::cout << "SHA256: " << utils::SHA256_sum("config/config-json") << std::endl;

	// Try autocomplete
	auto ac_res{ autocomplete_keywords("Cat", 30) };
	for (auto&& p_kw : ac_res) {
		std::cout << p_kw->synset_strs.front() << std::endl;
	}

	// Try different displays
	{
		Query q{ std::vector({ "dog park" }) };
		rescore(q);

		auto d_topn = get_display(DisplayType::DTopN, 0, 0)._dataset_frames;
		std::cout << "TOP N\n";
		d_topn.print_display();

		auto d_topknn = get_display(DisplayType::DTopKNN, 2, 0)._dataset_frames;
		std::cout << "TOP KNN\n";
		d_topknn.print_display();

		auto d_rand = get_display(DisplayType::DRand)._dataset_frames;
		std::cout << "RANDOM\n";
		d_rand.print_display();
	}

	// Try keyword rescore
	{
		Query q{ std::vector({ "dog park" }) };
		rescore(q);
		auto d_topn1 = get_display(DisplayType::DTopN, 0, 0)._dataset_frames;
		std::cout << "TOP N\n";
		d_topn1.print_display();
	}

	// Try reset session
	reset_search_session();

	// Try relevance feedback
	{
		auto d_rand1 = get_display(DisplayType::DRand)._dataset_frames;
		std::vector<FrameId> likes;
		auto d_rand_b = d_rand1.begin();
		likes.push_back((*d_rand_b)->frame_ID);
		d_rand_b++;
		likes.push_back((*d_rand_b)->frame_ID);

		like_frames(likes);
		likes.resize(1);
		like_frames(likes);
		std::cout << "Like " << likes[0] << std::endl;

		Query q{ std::vector({ "\\/?!,.'\"" }) };
		rescore(q);
	}

	{
		auto d_topn2 = get_display(DisplayType::DTopN, 0, 0)._dataset_frames;
		d_topn2.print_display();
		std::cout << "Len of top n page 0 " << d_topn2.size() << std::endl;
	}
	{
		auto d_topn2 = get_display(DisplayType::DTopN, 0, 1)._dataset_frames;
		std::cout << "Len of top n page 1 " << d_topn2.size() << std::endl;
	}
	{
		auto d_topn2 = get_display(DisplayType::DTopN, 0, 2)._dataset_frames;
		std::cout << "Len of top n page 2 " << d_topn2.size() << std::endl;
	}

	// Try SOM
	{
		while (!som_ready()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
		std::cout << "SOM is ready now!" << std::endl;

		auto d_som = get_display(DisplayType::DSom);
	}

	SHLOG_E("this is an error log");
	SHLOG_W("this is a warning log");
	SHLOG_I("this is an info log");
	SHLOG_S("this is a success log");
	SHLOG_D("this is a debug log");
	SHLOG_REQ("123.0.0.1", "this is an API request");
}

void Somhunter::run_generators() {
	/* ***
	 * Dataset generators
	 */
	// core.generate_example_images_for_keywords();

	/* ***
	 * Benchmarks
	 */
	// core.benchmark_native_text_queries(R"(data\v3c1-20k\native-queries.csv)", "bench-out");
	// core.benchmark_canvas_queries("saved-queries", "saved-queries-out");
	// core.benchmark_real_queries("data-logs", "data-logs/tasks.csv", "saved-queries-out");
	// std::cout << "DONE!" << std::endl;
}

RescoreResult Somhunter::rescore(Query& query, bool benchmark_run) {
	auto ts_start{ std::chrono::high_resolution_clock::now() };

	// If benchmarking, always reset
	if (benchmark_run) {
		reset_search_session();
	}

	const std::vector<TemporalQuery>& temporal_query{ query.temporal_queries };

	// Add the internal state likes to it
	_user_context.ctx.likes.insert(query.relevance_feeedback.begin(), query.relevance_feeedback.end());

	const Filters* p_filters{ &query.filters };
	size_t src_search_ctx_ID{ query.metadata.srd_search_ctx_ID };
	const std::string& screenshot_fpth{ query.metadata.screenshot_filepath };
	const std::string& label{ query.metadata.time_label };

	/* ***
	 * Save provided screenshot filepath if needed
	 */
	if (!benchmark_run) {
		// Catch occasional "inconsistent state" that happens due to old front-end state (e.g. after core restart)
		if (_user_context._history.size() < src_search_ctx_ID) {
			return RescoreResult{ _user_context.ctx.ID, _user_context._history, _user_context.ctx.curr_targets };
		}

		if (src_search_ctx_ID > 0 && src_search_ctx_ID != SIZE_T_ERR_VAL &&
		    _user_context._history[src_search_ctx_ID].screenshot_fpth.empty()) {
			_user_context._history[src_search_ctx_ID].label = label;
			_user_context._history[src_search_ctx_ID].screenshot_fpth = screenshot_fpth;
		}
	}

	/* ***
	 * Do all the needed rescore steps
	 */
	// Store likes for the logging purposees
	auto old_likes{ _user_context.ctx.likes };

	auto& features{ _dataset_features.primary };

	// Check if temporal queries has changed
	if (_user_context.ctx.last_temporal_queries != temporal_query) {
		reset_scores();  //< Resets scores & used tools
		size_t moment = 0;

		/* ***
		 * Set the filters to the context
		 */
		if (p_filters != nullptr) {
			_user_context.ctx.filters = *p_filters;

			// If filters used
			if (!_user_context.ctx.filters.is_default()) {
				_user_context.ctx.used_tools.filters = &_user_context.ctx.filters;
			}
			// Else reset filters used
			else {
				_user_context.ctx.used_tools.filters = nullptr;
			}
		}

		for (size_t mi = 0; mi < temporal_query.size(); ++mi) {
			auto&& moment_query = temporal_query[mi];

			if (moment_query.empty()) {
				continue;
			} else {
				// Mark temporality of the query
				if (mi > 0) {
					_user_context.ctx.used_tools.temporal_query_used = true;
				}
			}

			// ***
			// Relocation
			if (moment_query.is_relocation()) {
				SHLOG_D("Running the relocation query model...");

				// Set used tool
				_user_context.ctx.used_tools.relocation_used = true;

				_relocation_ranker.score(moment_query.relocation, _user_context.ctx.scores, moment, features);
			}
			// ***
			// Canvas
			else if (moment_query.is_canvas()) {
				SHLOG_D("Running the canvas query model...");

				_collage_ranker.score(moment_query.canvas, _user_context.ctx.scores, moment,
				                      _user_context.ctx.used_tools, features, _dataset_frames);

			}
			// ***
			// Plain text
			else if (moment_query.is_text()) {
				// If secondary features should be used
				if (query.score_secondary()) {
					SHLOG_D("Running plain texual model << SECONDARY SCORING >>...");
					rescore_keywords(_secondary_keyword_ranker, moment_query.textual, moment,
					                 _dataset_features.secondary);
				} else {
					SHLOG_D("Running plain texual model << PRIMARY SCORING >>...");
					rescore_keywords(_keyword_ranker, moment_query.textual, moment, features);
				}
			}
			++moment;
		}

		_user_context.ctx.temporal_size = moment;
		// Cache the appliend temporal queries
		_user_context.ctx.last_temporal_queries = temporal_query;
		// Normalize the inverse scores
		_user_context.ctx.scores.normalize(_user_context.ctx.temporal_size);

		// Power of query initialization
		const float power = 50;
		// Apply temporal fusion and trnsform inv. scores to scores
		_user_context.ctx.scores.apply_temporals(_user_context.ctx.temporal_size, _dataset_frames, power);

		// Normalize the scores
		_user_context.ctx.scores.normalize(_user_context.ctx.temporal_size);
	}

	// Cancel the effect of returning from KNN
	_user_context.ctx.curr_disp_type = DisplayType::DTopN;

	apply_filters();
	// Apply feedback and normalize
	rescore_feedback();

	// If SOM required
	std::thread som_t;
	if (!benchmark_run) {
		// Notify the SOM worker thread
		som_t = std::thread{ [this]() { som_start(_user_context.ctx.temporal_size); } };
	}

	// Reset the "seen frames" constext for the Bayes
	_user_context.ctx.shown_images.clear();

	// Reset likes
	_user_context.ctx.likes.clear();

	// Start the new search context
	if (!benchmark_run) {
		push_search_ctx();
	}

	/* ***
	 * Logging
	 */
	size_t tar_pos{ _dataset_frames.size() };
	if (!benchmark_run) {
		const auto& targets{ _user_context.ctx.curr_targets };

		/*for (auto&& t : targets) {
		    size_t r{ _user_context.ctx.scores.frame_rank(t.frame_ID) + 1 };

		    tar_pos = std::min(r, tar_pos);
		}*/

		// Flush the backlog
		_user_context._logger.poll();

		// Add debug targets if saving the one
		if (query.is_save) {
			query.set_targets(targets);
		}

		_user_context._logger.log_rescore(_user_context.ctx._prev_query, query);

		const auto& ss{ _settings.presentation_views };

		const auto& top_n = _user_context.ctx.scores.top_n(_dataset_frames, TOPN_LIMIT, ss.topn_frames_per_video,
		                                                   ss.topn_frames_per_shot);

		// Log this rescore result
		_user_context._logger.log_results(_dataset_frames, _user_context.ctx.scores, old_likes,
		                                  _user_context.ctx.used_tools, _user_context.ctx.curr_disp_type, top_n,
		                                  query.get_plain_text_query(), ss.topn_frames_per_video,
		                                  ss.topn_frames_per_shot, p_filters->dataset_parts);

		// SHLOG_S("Target position is " << tar_pos << ".");
	}

	// Store this query
	_user_context.ctx._prev_query = query;

	auto res{ RescoreResult{ _user_context.ctx.ID, _user_context._history, _user_context.ctx.curr_targets, tar_pos } };

	// auto ts2{ std::chrono::high_resolution_clock::now() };
	if (!benchmark_run) {
		som_t.join();
	}
	// auto ts_end{ std::chrono::high_resolution_clock::now() };

	// auto d1{ std::chrono::duration_cast<std::chrono::milliseconds>(ts_end - ts2).count() };
	// auto d2{ std::chrono::duration_cast<std::chrono::milliseconds>(ts_end - ts_start).count() };

	// std::cout << "Blocked by `som_t` thread: " << d1 << std::endl;
	// std::cout << "`rescore()` took: " << d2 << std::endl;
	return res;
}

bool Somhunter::som_ready() const { return _user_context._async_SOM.map_ready(); }

bool Somhunter::som_ready(size_t temp_id) const { return _user_context._temp_async_SOM[temp_id]->map_ready(); }

bool Somhunter::login_to_eval_server() { return _user_context._eval_server.login(); }
bool Somhunter::logout_from_eval_server() { return _user_context._eval_server.logout(); }

SubmitResult Somhunter::submit_to_eval_server(FrameId frame_ID) {
	// Submit
	auto vf = _dataset_frames.get_frame(frame_ID);
	try {
		bool submit_res{ _user_context._eval_server.submit(vf) };

		// Log it
		_user_context._logger.log_submit(vf, submit_res);
		return (submit_res ? SubmitResult::CORRECT : SubmitResult::INCORRECT);
	} catch (const NotLoggedInEx& ex) {
		SHLOG_W("Not logged in...");
		return SubmitResult::NOT_LOGGED_IN;
	}
}

void Somhunter::reset_search_session() {
	_user_context._logger.poll();

	_user_context.ctx.shown_images.clear();
	_user_context.ctx.likes.clear();
	_user_context.ctx.last_temporal_queries.clear();

	reset_scores();

	_user_context._logger.log_reset_search();
	som_start(MAX_TEMPORAL_SIZE);

	// Reset UserContext
	_user_context.reset();

	generate_new_targets();

	// Trigger the initial display as "rescore" just to have complete logs
	Query phony_query{};
	rescore(phony_query);
}

void Somhunter::log_video_replay(FrameId frame_ID, float delta_X) {
	_user_context._logger.log_show_video_replay(_dataset_frames, frame_ID, delta_X);
}

void Somhunter::log_scroll(DisplayType t, float dir_Y) { _user_context._logger.log_scroll(_dataset_frames, t, dir_Y); }

void Somhunter::log_text_query_change(const std::string& text_query) {
	_user_context._logger.log_text_query_change(text_query);
}

void Somhunter::log_canvas_query_change() { _user_context._logger.log_canvas_query_change(); }

std::string Somhunter::store_rescore_screenshot(const std::string& /*filepath*/) {
	// SHLOG_W("Simulating the screenshot saving...");

	std::string UI_filepath{ "/assets/img/history_screenshot.jpg" };

	// Return the filepath the UI can use to render it
	return UI_filepath;
}

const std::vector<FrameId>& Somhunter::get_top_scored(size_t max_count, size_t from_video, size_t from_shot) const {
	return _user_context.ctx.scores.top_n(_dataset_frames, max_count, from_video, from_shot);
}

std::vector<VideoFramePointer> Somhunter::get_top_scored_frames(size_t max_count, size_t from_video,
                                                                size_t from_shot) const {
	auto fs = _user_context.ctx.scores.top_n(_dataset_frames, max_count, from_video, from_shot);
	return _dataset_frames.ids_to_video_frame(fs);
}

std::vector<float> Somhunter::get_top_scored_scores(std::vector<FrameId>& top_scored_frames) const {
	std::vector<float> res;
	for (auto&& frame_ID : top_scored_frames) {
		res.emplace_back(_user_context.ctx.scores[frame_ID]);
	}

	return res;
}

size_t sh::Somhunter::find_targets(const std::vector<FrameId>& top_scored, const std::vector<FrameId>& targets) const {
	size_t i{ 0 };
	for (auto it{ top_scored.begin() }; it != top_scored.end(); ++it) {
		FrameId curr_ID{ *it };

		for (auto&& t : targets) {
			if (t == curr_ID) {
				return i;
				break;
			}
		}
		++i;
	}

	return ERR_VAL<size_t>();
}

void Somhunter::benchmark_native_text_queries(const std::string& queries_filepath, const std::string& out_dir) {
	SHLOG_I("Running benchmark on file '" << queries_filepath << "'...");

	std::ifstream ifs(queries_filepath, std::ios::in);
	if (!ifs) {
		std::string msg{ "Error opening file: " + queries_filepath };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	std::vector<PlainTextBenchmarkQuery> bench_queries;

	std::string line;
	// Ignore the header line
	std::getline(ifs, line);

	// Read the file line by line
	std::string delimiter(">>");
	for (; std::getline(ifs, line);) {
		std::stringstream line_ss(line);

		// Tokenize this line with ';'
		std::vector<std::string> tokens;
		for (std::string token; std::getline(line_ss, token, ';');) {
			tokens.push_back(token);
		}

		// Parse wordnet synset ID
		FrameId target_frame_ID{ utils::str2<FrameId>(tokens[0]) };
		std::string text_query{ std::move(tokens[1]) };

		// Split query into temporal one
		size_t pos = 0;
		std::string token;
		std::vector<TextualQuery> tempQuery;
		while ((pos = text_query.find(delimiter)) != std::string::npos) {
			tempQuery.push_back(text_query.substr(0, pos));
			text_query.erase(0, pos + delimiter.length());
		}

		bench_queries.emplace_back(
		    PlainTextBenchmarkQuery{ std::vector<FrameId>{ target_frame_ID }, std::move(tempQuery) });
	}

	std::vector<std::vector<float>> query_results;
	std::vector<std::vector<FrameId>> query_results_IDs;

	std::vector<size_t> ranks;
	for (size_t i{ 0 }; i < bench_queries.size(); ++i) {
		auto& bq{ bench_queries[i] };

		reset_scores();
		Query q{ bq.text_query };
		rescore(q, true);

		auto& IDs{ query_results_IDs.emplace_back(get_top_scored()) };
		query_results.emplace_back(get_top_scored_scores(IDs));

		size_t rank{ find_targets(IDs, bq.targets) };

		ranks.emplace_back(rank);
	}

	// If print to file required
	if (!out_dir.empty()) {
		utils::dir_create(out_dir);

		std::string query_scores_file{ out_dir + "/native-query-scores.bin" };
		std::string query_IDs_file{ out_dir + "/native-query-frame-IDs.bin" };

		utils::to_file(query_results, query_scores_file);
		utils::to_file(query_results_IDs, query_IDs_file);
	}

	//
	// Summarize
	//

	size_t num_ticks{ 100 };
	size_t num_frames{ get_num_frames() };

	float scale{ num_ticks / float(num_frames) };

	std::vector<size_t> hist;
	hist.resize(num_ticks, 0);
	{
		size_t i{ 0 };
		for (auto&& r : ranks) {
			size_t scaled_rank{ static_cast<size_t>(r * scale) };
			++hist[scaled_rank];

			++i;
		}
	}

	size_t acc{ 0 };
	for (auto& i : hist) {
		auto xx = i;
		i = acc;
		acc += xx;
	}

	for (size_t i = 0; i < hist.size(); i++) {
		SHLOG(i << "," << hist[i]);
	}
}

void Somhunter::benchmark_canvas_queries(const std::string& queries_dir, const std::string& out_dir) {
#if 1  // Rewite
	using directory_iterator = std::filesystem::directory_iterator;

	// ***
	// Prepare output
	utils::dir_create(out_dir);
	std::string filename{ "canvas_benchmark_" + std::to_string(utils::timestamp()) + ".txt" };
	auto out_filepath{ std::filesystem::path{ out_dir } / filename };

	std::ofstream ofs{ out_filepath.string() };
	if (!ofs.is_open()) {
		throw std::runtime_error{ "Unable to open file for writing: "s + out_filepath.string() };
	}
	SHLOG("Results will be printed to '" << out_filepath << "'...");

	std::vector<std::size_t> ranks_positioned;
	std::vector<std::size_t> ranks_unpositioned;
	std::vector<std::string> queries;

	// ***
	// Load the filenames from the directory
	std::vector<std::string> serialized_queries;
	std::vector<std::string> serialized_queries_infos;

	SHLOG("Loading queries from the directory '" << queries_dir << "'...");
	for (const auto& dir_entry : directory_iterator(queries_dir)) {
		SHLOG("\t - " << dir_entry.path());

		for (const auto& file : std::filesystem::directory_iterator(dir_entry)) {
			std::string filepath{ file.path().string() };

			std::string suffix{ filepath.substr(filepath.length() - 3) };
			if (suffix == "bin") {
				serialized_queries.emplace_back(filepath);
			}

			if (suffix == "son") {
				serialized_queries_infos.emplace_back(filepath);
			}

			SHLOG("\t" << file.path());
		}
		do_assert_equals(serialized_queries.size(), serialized_queries_infos.size(),
		                 "There must be the same number of infos & bin files.");
	}

	// ***
	// Rank them
	{
		size_t q_idx{ 0 };
		for (auto&& f : serialized_queries) {
			using namespace nlohmann;
			SHLOG("Running query from '" << f << "' file...");

			const auto& f_info{ serialized_queries_infos[q_idx] };
			std::ifstream ifs_info(f_info);
			json info_json;
			ifs_info >> info_json;

			std::vector<TemporalQuery> qs{ utils::deserialize_from_file<std::vector<TemporalQuery>>(f) };
			auto targets = info_json["targets"].get<std::vector<std::size_t>>();

			Query q;
			q.temporal_queries = qs;

			// ***
			// POSITIONAL
			{
				rescore(q, true);
				auto disp = get_top_scored();

				size_t i{ 0 };
				for (auto it{ disp.begin() }; it != disp.end(); ++it) {
					if (std::find(targets.begin(), targets.end(), *it) != targets.end()) {
						ranks_positioned.push_back(i);
						break;
					}
					++i;
				}
			}

			// ***
			// Decay to unpositioned
			Query qq{ q };
			qq.transform_to_no_pos_queries();

			std::string this_query;
			for (auto&& temp_q : qq.temporal_queries) {
				this_query.append(temp_q.textual).append(" >> ");
			}
			for (size_t i = 0; i < 4; ++i) this_query.pop_back();

			queries.emplace_back(this_query);

			// ***
			// UNPOSITIONAL
			{
				rescore(qq, true);
				auto disp = get_top_scored();

				size_t i{ 0 };
				for (auto it{ disp.begin() }; it != disp.end(); ++it) {
					if (std::find(targets.begin(), targets.end(), *it) != targets.end()) {
						ranks_unpositioned.push_back(i);
						break;
					}
					++i;
				}
			}

			do_assert_equals(ranks_positioned.size(), ranks_unpositioned.size(), "Numbers must match!");
			do_assert_equals(queries.size(), ranks_unpositioned.size(), "Numbers must match!");

			// SHLOG("\t RANKS: " << ranks_positioned[q_idx] << ", " << ranks_unpositioned[q_idx]);
			++q_idx;
		}
	}
	// #####################################

	ofs << "query_idx;positioned;unpositioned;query" << std::endl;
	for (size_t i = 0; i < ranks_positioned.size(); i++) {
		ofs << i << ";" << ranks_positioned[i] << ";" << ranks_unpositioned[i] << ";" << queries[i] << std::endl;
	}

	// print it
	// std::sort(ranks.begin(), ranks.end());

	size_t num_ticks{ 500 };
	size_t num_frames{ get_num_frames() };

	float scale{ num_ticks / float(num_frames) };

	// POS
	std::vector<size_t> hist_positioned;
	hist_positioned.resize(num_ticks, 0);
	{
		{
			size_t i{ 0 };
			for (auto&& r : ranks_positioned) {
				size_t scaled_rank{ static_cast<size_t>(r * scale) };
				++hist_positioned[scaled_rank];

				++i;
			}
		}

		size_t acc{ 0 };
		for (auto& i : hist_positioned) {
			auto xx = i;
			i = acc;
			acc += xx;
		}
	}

	// NPOS
	std::vector<size_t> hist_unpositioned;
	hist_unpositioned.resize(num_ticks, 0);
	{
		{
			size_t i{ 0 };
			for (auto&& r : ranks_unpositioned) {
				size_t scaled_rank{ static_cast<size_t>(r * scale) };
				++hist_unpositioned[scaled_rank];

				++i;
			}
		}

		size_t acc{ 0 };
		for (auto& i : hist_unpositioned) {
			auto xx = i;
			i = acc;
			acc += xx;
		}
	}

	ofs << "tick;positioned;unpositioned;query" << std::endl;
	for (size_t i = 0; i < hist_positioned.size(); i++) {
		ofs << i << ";" << hist_positioned[i] << ";" << hist_unpositioned[i] << std::endl;
	}

#endif
}

void Somhunter::benchmark_real_queries(const std::string& queries_dir, const std::string& targets_fpth,
                                       const std::string& out_dir) {
	using directory_iterator = std::filesystem::directory_iterator;
	using namespace nlohmann;

	// Query xx1 = utils::deserialize_from_file<Query>(queries_dir + "/sh-patrik/admin#1624280823952.bin");
	// Query xx2 = utils::deserialize_from_file<Query>(queries_dir + "/sh-patrik/admin#1624280769587.bin");
	// // std::cout << jxx.to_JSON().dump(4) << std::endl;
	// rescore(xx1, true);
	// auto disp = get_top_scored_frames(0, 3, 1);
	// auto it = disp.begin();
	// for (std::size_t i = 0; i < 10; ++i, ++it) {
	// 	std::cout << (*it)->frame_ID << std::endl;
	// 	std::cout << _user_context.ctx.scores[i] << std::endl;
	// }
	// std::cout << "---" << std::endl;

	// rescore(xx2, true);
	// disp = get_top_scored_frames(0, 3, 1);
	// it = disp.begin();
	// for (std::size_t i = 0; i < 10; ++i, ++it) {
	// 	std::cout << (*it)->frame_ID << std::endl;
	// 	std::cout << _user_context.ctx.scores[i] << std::endl;
	// }

	// ***
	// Config
	constexpr std::size_t from_video = 3;
	constexpr std::size_t from_shot = 1;
	constexpr std::size_t num_frames = 1154038;
	std::array types{ "text-canvas"s, "bitmap-canvas"s, "relocation"s };

	TaskTargetHelper tar_helper(targets_fpth);

	// ***
	// Prepare output
	utils::dir_create(out_dir);
	std::map<std::string, std::ofstream> ofss;
	for (auto&& type : types) {
		std::string filename = type + "-benchmark.csv";

		utils::dir_create((std::filesystem::path(out_dir) / type).string());
		auto out_filepath{ std::filesystem::path(out_dir) / filename };

		auto&& [item, ins] = ofss.emplace(type, out_filepath.string());
		if (!(item->second)) {
			throw std::runtime_error{ "Unable to open file for writing: "s + out_filepath.string() };
		}

		SHLOG("Results for '" << type << "' will be printed to '" << out_filepath << "'...");
	}

	using QueryResults = std::vector<std::tuple<std::size_t, std::size_t>>;

	std::map<std::string, std::vector<std::tuple<std::string, std::string>>> query_infos;
	std::map<std::string, QueryResults> ranks_positioned;
	std::map<std::string, QueryResults> ranks_unpositioned;

	for (auto&& type : types) {
		query_infos.emplace(type, std::vector<std::tuple<std::string, std::string>>());
		ranks_positioned.emplace(type, QueryResults());
		ranks_unpositioned.emplace(type, QueryResults());

		ofss[type] << "ID,user,pos_video,pos_frame,unpos_video,unpos_frame" << std::endl;
	}

	// ***
	// Load the filenames from the directory
	std::vector<std::pair<std::string, std::string>> query_filepaths;

	// For each user
	SHLOG("Loading queries from the directory '" << queries_dir << "'...");
	for (const auto& dir_entry : directory_iterator(queries_dir)) {
		SHLOG("\t USER: " << dir_entry.path());

		// Ignore non-directories
		if (!std::filesystem::is_directory(dir_entry)) continue;

		// For each query of the user
		for (const auto& file : std::filesystem::directory_iterator(dir_entry)) {
			std::string filepath{ file.path().string() };

			std::string suffix{ filepath.substr(filepath.length() - 3) };
			if (suffix == "bin") {
				query_filepaths.emplace_back(dir_entry.path().filename().string(), filepath);
			}
		}
	}

	// ***
	// Rank them
	{
		std::size_t q_idx = 0;
		for (auto&& [user, f] : query_filepaths) {
			Query q(utils::deserialize_from_file<Query>(f));

			std::string type = "";
			if (q.is_relocation())
				type = "relocation";
			else if (q.is_bitmap_canvas())
				type = "bitmap-canvas";
			else if (q.is_text_canvas())
				type = "text-canvas";

			// Ignore other types
			if (type.empty()) continue;
			SHLOG("Running '" << type << "' query from '" << f << "' file...");

			std::string ID = f.substr(f.length() - 23);
			auto task_dir{ std::filesystem::path(out_dir) / type / ID };
			std::string file_base = task_dir.string();

			// Extract timestamp from the filename
			std::size_t timestamp = ERR_VAL<std::size_t>();
			{
				// \todo Generalize...
				// admin#1624275276932.bin
				auto l = f.length();
				timestamp = utils::str2<std::size_t>(f.substr(l - 17, l - 4));
			}

			// Get target for the task
			std::tuple<VideoId, FrameId, FrameId> targets;
			try {
				auto [v_ID, fr, to] = tar_helper.target(timestamp);
				targets = std::tuple(v_ID, fr, to);
			}
			// If not inside any task
			catch (...) {
				continue;
			}
			query_infos[type].emplace_back(ID, user);

			// ***
			// POSITIONAL
			{
				Somhunter::write_query(file_base + ".q1.json", q);
				rescore(q, true);
				auto disp = get_top_scored_frames(0, from_video, from_shot);
				Somhunter::write_resultset(file_base + ".q1.resultset.json", disp);

				std::size_t v_min = num_frames;
				std::size_t f_min = num_frames;

				std::size_t i = 0;
				for (auto&& vf : disp) {
					++i;
					auto v_ID = vf->video_ID;
					auto fn = vf->frame_number;

					if (v_ID == std::get<0>(targets)) {
						v_min = std::min(i, v_min);
						if (std::get<1>(targets) <= fn && fn <= std::get<2>(targets)) {
							f_min = std::min(i, f_min);
							break;
						}
					}
				}

				std::cout << v_min << ", " << f_min << std::endl;
				ranks_positioned[type].emplace_back(v_min, f_min);
			}

			// ***
			// Decay the query
			Query qq(q);
			qq.transform_to_no_pos_queries();

			// ***
			// UNPOSITIONAL
			{
				Somhunter::write_query(file_base + ".q2.json", qq);
				rescore(qq, true);
				auto disp = get_top_scored_frames(0, from_video, from_shot);
				Somhunter::write_resultset(file_base + ".q2.resultset.json", disp);

				std::size_t v_min = num_frames;
				std::size_t f_min = num_frames;

				std::size_t i = 0;
				for (auto&& vf : disp) {
					++i;
					auto v_ID = vf->video_ID;
					auto fn = vf->frame_number;

					if (v_ID == std::get<0>(targets)) {
						v_min = std::min(i, v_min);
						if (std::get<1>(targets) <= fn && fn <= std::get<2>(targets)) {
							f_min = std::min(i, f_min);
							break;
						}
					}
				}

				std::cout << v_min << ", " << f_min << std::endl;
				ranks_unpositioned[type].emplace_back(v_min, f_min);
			}
			std::cout << "------------------ " << std::endl;

			Somhunter::write_query_info(
			    file_base + ".info.json", ID, user, targets, std::get<0>(ranks_positioned[type].back()),
			    std::get<1>(ranks_positioned[type].back()), std::get<0>(ranks_unpositioned[type].back()),
			    std::get<1>(ranks_unpositioned[type].back()));

			++q_idx;
		}
	}

	for (auto&& type : types) {
		auto& ofs = ofss[type];
		for (size_t i = 0; i < query_infos[type].size(); ++i) {
			ofs << std::get<0>(query_infos[type][i]) << "," << std::get<1>(query_infos[type][i]) << ","
			    << std::get<0>(ranks_positioned[type][i]) << "," << std::get<1>(ranks_positioned[type][i]) << ","
			    << std::get<0>(ranks_unpositioned[type][i]) << "," << std::get<1>(ranks_unpositioned[type][i])
			    << std::endl;
		}
	}
}

void Somhunter::generate_new_targets() {
	constexpr std::size_t num_seq{ 5 };
	std::size_t num_frames{ _dataset_frames.size() };

	std::vector<VideoFrame> targets;
	targets.reserve(num_seq);

	while (true) {
		FrameId target_ID{ utils::irand<FrameId>(0, num_frames - 5) };

		const auto& f{ _dataset_frames.get_frame(target_ID) };

		for (std::size_t i{ 1 }; i < num_seq; ++i) {
			const auto& ff{ _dataset_frames.get_frame(target_ID + i) };

			if (f.video_ID != ff.video_ID) {
				continue;
			}
		}

		// Done!
		for (std::size_t i{ 0 }; i < num_seq; ++i) {
			const auto& fr{ _dataset_frames.get_frame(target_ID + i) };

			targets.emplace_back(fr);
		}
		break;
	}

	_user_context.ctx.curr_targets = std::move(targets);
}

void Somhunter::rescore_feedback() {
	if (_user_context.ctx.likes.empty()) return;

	// Make sure some frames are set as seen
	if (_user_context.ctx.shown_images.empty()) {
		// This fills in the shown images with the first page of top-scored display
		get_topn_display(0);
	}

	_user_context.ctx.scores.apply_bayes(_user_context.ctx.likes, _user_context.ctx.shown_images,
	                                     _dataset_features.primary);
	_user_context.ctx.used_tools.bayes_used = true;
}

void Somhunter::som_start(size_t temporal) {
	_user_context._async_SOM.start_work(_dataset_features.primary, _user_context.ctx.scores,
	                                    _user_context.ctx.scores.v());
	for (size_t i = 0; i < temporal; ++i) {
		_user_context._temp_async_SOM[i]->start_work(_dataset_features.primary, _user_context.ctx.scores,
		                                             _user_context.ctx.scores.temp(i));
	}
}

FramePointerRange Somhunter::get_random_display() {
	// Get ids
	auto ids =
	    _user_context.ctx.scores.weighted_sample(DISPLAY_GRID_WIDTH * DISPLAY_GRID_HEIGHT, RANDOM_DISPLAY_WEIGHT);

	// Log
	_user_context._logger.log_show_random_display(_dataset_frames, ids);
	// Update context
	for (auto id : ids) _user_context.ctx.shown_images.insert(id);
	_user_context.ctx.current_display = _dataset_frames.ids_to_video_frame(ids);
	_user_context.ctx.curr_disp_type = DisplayType::DRand;

	return FramePointerRange(_user_context.ctx.current_display);
}

FramePointerRange Somhunter::get_topn_display(PageId page) {
	// Another display or first page -> load
	if (_user_context.ctx.curr_disp_type != DisplayType::DTopN || page == 0) {
		SHLOG_D("Loading top n display first page");

		const auto& ss{ _settings.presentation_views };

		// Get ids
		const auto& ids = _user_context.ctx.scores.top_n(_dataset_frames, TOPN_LIMIT, ss.topn_frames_per_video,
		                                                 ss.topn_frames_per_shot);

		// Log only if page 0
		if (page == 0) _user_context._logger.log_show_topn_display(_dataset_frames, ids);

		// Update context
		_user_context.ctx.current_display = _dataset_frames.ids_to_video_frame(ids);
		_user_context.ctx.curr_disp_type = DisplayType::DTopN;
	}

	return get_page_from_last(page);
}

FramePointerRange Somhunter::get_topn_context_display(PageId page) {
	// Another display or first page -> load
	if (_user_context.ctx.curr_disp_type != DisplayType::DTopNContext || page == 0) {
		SHLOG_D("Loading top n context display first page");
		const auto& ss{ _settings.presentation_views };

		// Get ids
		auto ids = _user_context.ctx.scores.top_n_with_context(_dataset_frames, TOPN_LIMIT, ss.topn_frames_per_video,
		                                                       ss.topn_frames_per_shot);

		// Log
		if (page == 0) _user_context._logger.log_show_topn_context_display(_dataset_frames, ids);

		// Update context
		_user_context.ctx.current_display = _dataset_frames.ids_to_video_frame(ids);
		_user_context.ctx.curr_disp_type = DisplayType::DTopNContext;
	}

	return get_page_from_last(page);
}

FramePointerRange Somhunter::get_som_display() {
	if (!_user_context._async_SOM.map_ready()) {
		return FramePointerRange();
	}

	auto ids{ _user_context._async_SOM.get_display(_user_context.ctx.scores) };

	// Log
	_user_context._logger.log_show_som_display(_dataset_frames, ids);

	// Update context
	for (auto id : ids) {
		if (id == IMAGE_ID_ERR_VAL) continue;

		_user_context.ctx.shown_images.insert(id);
	}
	_user_context.ctx.current_display = _dataset_frames.ids_to_video_frame(ids);
	_user_context.ctx.curr_disp_type = DisplayType::DSom;

	return FramePointerRange(_user_context.ctx.current_display);
}

FramePointerRange Somhunter::get_som_relocation_display(size_t temp_id) {
	assert(temp_id < _user_context._temp_async_SOM.size());

	if (!_user_context._temp_async_SOM[temp_id]->map_ready()) {
		return FramePointerRange();
	}

	auto ids{ _user_context._temp_async_SOM[temp_id]->get_display(_user_context.ctx.scores) };

	// Log
	_user_context._logger.log_show_som_relocation_display(_dataset_frames, ids);

	// Update context
	for (auto id : ids) {
		if (id == IMAGE_ID_ERR_VAL) continue;

		_user_context.ctx.shown_images.insert(id);
	}
	_user_context.ctx.current_display = _dataset_frames.ids_to_video_frame(ids);
	_user_context.ctx.curr_disp_type = DisplayType::DRelocation;

	return FramePointerRange(_user_context.ctx.current_display);
}

FramePointerRange Somhunter::get_video_detail_display(FrameId selected_image, bool log_it) {
	VideoId v_id = _dataset_frames.get_video_id(selected_image);

	if (v_id == VIDEO_ID_ERR_VAL) {
		SHLOG_E("Video for " << selected_image << " not found");
		return std::vector<VideoFramePointer>();
	}

	// Get ids
	FrameRange video_frames = _dataset_frames.get_all_video_frames(v_id);

	// Log
	if (log_it) _user_context._logger.log_show_detail_display(_dataset_frames, selected_image);

	// Update context
	for (auto iter = video_frames.begin(); iter != video_frames.end(); ++iter) {
		_user_context.ctx.shown_images.insert(iter->frame_ID);
	}

	_user_context.ctx.current_display = _dataset_frames.range_to_video_frame(video_frames);
	_user_context.ctx.curr_disp_type = DisplayType::DVideoDetail;
	_user_context._videos_seen.insert(v_id);

	return FramePointerRange(_user_context.ctx.current_display);
}

FramePointerRange Somhunter::get_topKNN_display(FrameId selected_image, PageId page) {
	// Another display or first page -> load
	if (_user_context.ctx.curr_disp_type != DisplayType::DTopKNN || page == 0) {
		const auto& ss{ _settings.presentation_views };
		// Get ids
		auto ids = _dataset_features.primary.get_top_knn(_dataset_frames, selected_image, ss.topn_frames_per_video,
		                                                 ss.topn_frames_per_shot);

		// Log only if the first page
		if (page == 0) {
			_user_context._logger.log_show_topknn_display(_dataset_frames, selected_image, ids);
		}

		// Update context
		_user_context.ctx.current_display = _dataset_frames.ids_to_video_frame(ids);
		_user_context.ctx.curr_disp_type = DisplayType::DTopKNN;

		// KNN is query by example so we NEED to log a rerank
		UsedTools ut;
		ut.topknn_used = true;

		_user_context._logger.log_results(_dataset_frames, _user_context.ctx.scores, _user_context.ctx.likes, ut,
		                                  _user_context.ctx.curr_disp_type, ids, "", ss.topn_frames_per_video,
		                                  ss.topn_frames_per_shot);
	}

	return get_page_from_last(page);
}

FramePointerRange Somhunter::get_page_from_last(PageId page) {
	SHLOG_D("Getting page " << page << ", page size " << _settings.presentation_views.display_page_size);

	const auto& ss{ _settings.presentation_views };

	size_t begin_off{ std::min(_user_context.ctx.current_display.size(), page * ss.display_page_size) };
	size_t end_off{ std::min(_user_context.ctx.current_display.size(),
		                     page * ss.display_page_size + ss.display_page_size) };

	FramePointerRange res(_user_context.ctx.current_display.cbegin() + begin_off,
	                      _user_context.ctx.current_display.cbegin() + end_off);

	// Update context
	for (auto iter = res.begin(); iter != res.end(); ++iter)
		// Skip "empty" frames
		if (*iter != nullptr) _user_context.ctx.shown_images.insert((*iter)->frame_ID);

	return res;
}

void Somhunter::reset_scores(float val) {
	_user_context.ctx.used_tools.reset();
	_user_context.ctx.scores.reset(val);
}

const UserContext& Somhunter::switch_search_context(size_t index, size_t src_search_ctx_ID,
                                                    const std::string& screenshot_fpth, const std::string& label) {
	/*
	 * Save provided screenshot filepath if needed
	 */
	if (src_search_ctx_ID != SIZE_T_ERR_VAL && _user_context._history[src_search_ctx_ID].screenshot_fpth.empty()) {
		_user_context._history[src_search_ctx_ID].label = label;
		_user_context._history[src_search_ctx_ID].screenshot_fpth = screenshot_fpth;
	}

	// Range check
	if (index >= _user_context._history.size()) {
		std::string msg{ "Index is out of bounds: " + index };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	SHLOG_I("Switching to context '" << index << "'...");
	_user_context._logger.log_search_context_switch(index, src_search_ctx_ID);

	// SOM must stop first
	while (!_user_context._async_SOM.map_ready()) {
		std::this_thread::sleep_for(10ms);
	}

	// Get the desired state
	const auto& destContext{ _user_context._history[index] };

	// Copy the history state into the current one
	_user_context.ctx = SearchContext{ destContext };

	// Kick-off the SOM for the old-new state
	som_start(_user_context.ctx.temporal_size);

	// This action forces the result log to be send again
	_user_context._force_result_log = true;

	// Returnp ptr to it
	return _user_context;
}

const SearchContext& Somhunter::get_search_context() const { return _user_context.ctx; }

const UserContext& Somhunter::get_user_context() const { return _user_context; }
