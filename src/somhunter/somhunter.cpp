
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

#include "somhunter.h"

#include "common.h"
#include "utils.hpp"

using namespace sh;

GetDisplayResult Somhunter::get_display(DisplayType d_type, FrameId selected_image, PageId page, bool log_it)
{
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
	if ((_user_context._force_result_log) ||
	    (prev_display == DisplayType::DTopKNN &&
	     (curr_display == DisplayType::DTopN || curr_display == DisplayType::DTopNContext ||
	      curr_display == DisplayType::DRand || curr_display == DisplayType::DSom))) {
		const auto& top_n = _user_context.ctx.scores.top_n(_dataset_frames, TOPN_LIMIT, _settings.topn_frames_per_video,
		                                                   _settings.topn_frames_per_shot);

		_user_context._force_result_log = false;

		_user_context._logger.log_results(_dataset_frames, _user_context.ctx.scores,
		                                  _user_context.ctx._prev_query.relevance_feeedback,
		                                  _user_context.ctx.used_tools, _user_context.ctx.curr_disp_type, top_n,
		                                  _user_context.ctx._prev_query.get_plain_text_query(),
		                                  _settings.topn_frames_per_video, _settings.topn_frames_per_shot);
	}

	return GetDisplayResult{ frs, _user_context.ctx.likes, _user_context._bookmarks };
}

std::vector<bool> Somhunter::like_frames(const std::vector<FrameId>& new_likes)
{
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

std::vector<bool> Somhunter::bookmark_frames(const std::vector<FrameId>& new_bookmarks)
{
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

std::vector<const Keyword*> Somhunter::autocomplete_keywords(const std::string& prefix, size_t count) const
{
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

bool Somhunter::has_metadata() const { return !_settings.LSC_metadata_file.empty(); }

void Somhunter::apply_filters()
{
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

	auto ds_valid_interval{ filters.get_dataset_parts_valid_interval(_dataset_frames.size()) };

	std::cout << "[" << ds_valid_interval.first << ", " << ds_valid_interval.second << ")" << std::endl;

	// A closure that determines if the frame should be filtered out
	auto is_out{ [&days, t_from, t_to, &ds_valid_interval](const VideoFrame& f) {
		// If NOT within the selected days
		if (!days[f.weekday]) return true;

		// If NOT within the hour range
		if (t_from > f.hour || f.hour > t_to) return true;

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

RescoreResult Somhunter::rescore(Query& query, bool benchmark_run)
{
	auto ts_start{ std::chrono::high_resolution_clock::now() };
	const std::vector<TemporalQuery>& temporal_query{ query.temporal_queries };

	// Add the internal state likes to it
	RelevanceFeedbackQuery& rel_feedback_query{ query.relevance_feeedback };
	rel_feedback_query.insert(_user_context.ctx.likes.begin(), _user_context.ctx.likes.end());

	const Filters* p_filters{ &query.filters };
	size_t src_search_ctx_ID{ query.metadata.srd_search_ctx_ID };
	const std::string& screenshot_fpth{ query.metadata.screenshot_filepath };
	const std::string& label{ query.metadata.time_label };

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

	/* ***
	 * Save provided screenshot filepath if needed
	 */
	if (!benchmark_run) {
		if (_user_context._history.size() <= src_search_ctx_ID) {
			return RescoreResult{ _user_context.ctx.ID, _user_context._history, _user_context.ctx.curr_targets };
		}

		if (src_search_ctx_ID != SIZE_T_ERR_VAL && _user_context._history[src_search_ctx_ID].screenshot_fpth.empty()) {
			_user_context._history[src_search_ctx_ID].label = label;
			_user_context._history[src_search_ctx_ID].screenshot_fpth = screenshot_fpth;
		}
	}

	/* ***
	 * Do all the needed rescore steps
	 */
	// Store likes for the logging purposees
	auto old_likes{ rel_feedback_query };

	// Check if temporal queries has changed
	if (_user_context.ctx.last_temporal_queries != temporal_query) {
		reset_scores();  //< Resets scores & used tools
		size_t moment = 0;

		for (size_t mi = 0; mi < temporal_query.size(); ++mi) {
			auto&& moment_query = temporal_query[mi];

			if (moment_query.empty()) continue;

			// ***
			// Relocation
			if (moment_query.is_relocation()) {
				SHLOG_D("Running the relocation query model...");

				// Set used tool
				_user_context.ctx.used_tools.relocation_used = true;

				_relocation_ranker.score(moment_query.relocation, _user_context.ctx.scores, moment, _dataset_features);
			}
			// ***
			// Canvas
			else if (moment_query.is_canvas()) {
				SHLOG_D("Running the canvas query model...");

				_collage_ranker.score(moment_query.canvas, _user_context.ctx.scores, moment,
				                      _user_context.ctx.used_tools, _dataset_features, _dataset_frames);

			}
			// ***
			// Plain text
			else if (moment_query.is_text()) {
				SHLOG_D("Running plain texual model...");
				rescore_keywords(moment_query.textual, moment);
			}
			++moment;
		}

		_user_context.ctx.temporal_size = moment;
		// Cache the appliend temporal queries
		_user_context.ctx.last_temporal_queries = temporal_query;
		// Normalize the inverse scores
		_user_context.ctx.scores.normalize(_user_context.ctx.temporal_size);
		// Apply temporal fusion and trnsform inv. scores to scores
		_user_context.ctx.scores.apply_temporals(_user_context.ctx.temporal_size, _dataset_frames);
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

		const auto& top_n = _user_context.ctx.scores.top_n(_dataset_frames, TOPN_LIMIT, _settings.topn_frames_per_video,
		                                                   _settings.topn_frames_per_shot);

		// Log this rescore result
		_user_context._logger.log_results(_dataset_frames, _user_context.ctx.scores, old_likes,
		                                  _user_context.ctx.used_tools, _user_context.ctx.curr_disp_type, top_n,
		                                  query.get_plain_text_query(), _settings.topn_frames_per_video,
		                                  _settings.topn_frames_per_shot, p_filters->dataset_parts);

		// SHLOG_S("Target position is " << tar_pos << ".");
	}

	// Store this query
	_user_context.ctx._prev_query = query;

	auto res{ RescoreResult{ _user_context.ctx.ID, _user_context._history, _user_context.ctx.curr_targets, tar_pos } };

	auto ts2{ std::chrono::high_resolution_clock::now() };
	som_t.join();
	auto ts_end{ std::chrono::high_resolution_clock::now() };

	auto d1{ std::chrono::duration_cast<std::chrono::milliseconds>(ts_end - ts2).count() };
	auto d2{ std::chrono::duration_cast<std::chrono::milliseconds>(ts_end - ts_start).count() };

	std::cout << "Blocked by `som_t` thread: " << d1 << std::endl;
	std::cout << "`rescore()` took: " << d2 << std::endl;
	return res;
}

bool Somhunter::som_ready() const { return _user_context._async_SOM.map_ready(); }

bool Somhunter::som_ready(size_t temp_id) const { return _user_context._temp_async_SOM[temp_id]->map_ready(); }

bool Somhunter::login_to_eval_server() { return _user_context._eval_server.login(); }
bool Somhunter::logout_from_eval_server() { return _user_context._eval_server.logout(); }

SubmitResult Somhunter::submit_to_eval_server(FrameId frame_ID)
{
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

void Somhunter::reset_search_session()
{
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
}

void Somhunter::log_video_replay(FrameId frame_ID, float delta_X)
{
	_user_context._logger.log_show_video_replay(_dataset_frames, frame_ID, delta_X);
}

void Somhunter::log_scroll(DisplayType t, float dir_Y) { _user_context._logger.log_scroll(_dataset_frames, t, dir_Y); }

void Somhunter::log_text_query_change(const std::string& text_query)
{
	_user_context._logger.log_text_query_change(text_query);
}

std::string Somhunter::store_rescore_screenshot(const std::string& /*filepath*/)
{
	// SHLOG_W("Simulating the screenshot saving...");

	std::string UI_filepath{ "/assets/img/history_screenshot.jpg" };

	// Return the filepath the UI can use to render it
	return UI_filepath;
}

const std::vector<FrameId>& Somhunter::get_top_scored(size_t max_count, size_t from_video, size_t from_shot) const
{
	return _user_context.ctx.scores.top_n(_dataset_frames, max_count, from_video, from_shot);
}

std::vector<float> Somhunter::get_top_scored_scores(std::vector<FrameId>& top_scored_frames) const
{
	std::vector<float> res;
	for (auto&& frame_ID : top_scored_frames) {
		res.emplace_back(_user_context.ctx.scores[frame_ID]);
	}

	return res;
}

size_t sh::Somhunter::find_targets(const std::vector<FrameId>& top_scored, const std::vector<FrameId>& targets) const
{
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

void Somhunter::benchmark_native_text_queries(const std::string& queries_filepath, const std::string& out_dir)
{
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

void Somhunter::benchmark_canvas_queries(const std::string& queries_dir, const std::string& out_dir)
{
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

			SHLOG("\t RANKS: " << ranks_positioned[q_idx] << ", " << ranks_unpositioned[q_idx]);
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

void Somhunter::generate_new_targets()
{
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

void Somhunter::rescore_keywords(const TextualQuery& query, size_t temporal)
{
	_keyword_ranker.rank_sentence_query(query, _user_context.ctx.scores, _dataset_features, _settings, temporal);

	_user_context.ctx.used_tools.KWs_used = true;
}

void Somhunter::rescore_feedback()
{
	if (_user_context.ctx.likes.empty()) return;

	_user_context.ctx.scores.apply_bayes(_user_context.ctx.likes, _user_context.ctx.shown_images, _dataset_features);
	_user_context.ctx.used_tools.bayes_used = true;
}

void Somhunter::som_start(size_t temporal)
{
	_user_context._async_SOM.start_work(_dataset_features, _user_context.ctx.scores, _user_context.ctx.scores.v());
	for (size_t i = 0; i < temporal; ++i) {
		_user_context._temp_async_SOM[i]->start_work(_dataset_features, _user_context.ctx.scores,
		                                             _user_context.ctx.scores.temp(i));
	}
}

FramePointerRange Somhunter::get_random_display()
{
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

FramePointerRange Somhunter::get_topn_display(PageId page)
{
	// Another display or first page -> load
	if (_user_context.ctx.curr_disp_type != DisplayType::DTopN || page == 0) {
		SHLOG_D("Loading top n display first page");
		// Get ids
		const auto& ids = _user_context.ctx.scores.top_n(_dataset_frames, TOPN_LIMIT, _settings.topn_frames_per_video,
		                                                 _settings.topn_frames_per_shot);

		// Log only if page 0
		if (page == 0) _user_context._logger.log_show_topn_display(_dataset_frames, ids);

		// Update context
		_user_context.ctx.current_display = _dataset_frames.ids_to_video_frame(ids);
		_user_context.ctx.curr_disp_type = DisplayType::DTopN;
	}

	return get_page_from_last(page);
}

FramePointerRange Somhunter::get_topn_context_display(PageId page)
{
	// Another display or first page -> load
	if (_user_context.ctx.curr_disp_type != DisplayType::DTopNContext || page == 0) {
		SHLOG_D("Loading top n context display first page");
		// Get ids
		auto ids = _user_context.ctx.scores.top_n_with_context(
		    _dataset_frames, TOPN_LIMIT, _settings.topn_frames_per_video, _settings.topn_frames_per_shot);

		// Log
		if (page == 0) _user_context._logger.log_show_topn_context_display(_dataset_frames, ids);

		// Update context
		_user_context.ctx.current_display = _dataset_frames.ids_to_video_frame(ids);
		_user_context.ctx.curr_disp_type = DisplayType::DTopNContext;
	}

	return get_page_from_last(page);
}

FramePointerRange Somhunter::get_som_display()
{
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

FramePointerRange Somhunter::get_som_relocation_display(size_t temp_id)
{
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

FramePointerRange Somhunter::get_video_detail_display(FrameId selected_image, bool log_it)
{
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

	return FramePointerRange(_user_context.ctx.current_display);
}

FramePointerRange Somhunter::get_topKNN_display(FrameId selected_image, PageId page)
{
	// Another display or first page -> load
	if (_user_context.ctx.curr_disp_type != DisplayType::DTopKNN || page == 0) {
		// Get ids
		auto ids = _dataset_features.get_top_knn(_dataset_frames, selected_image, _settings.topn_frames_per_video,
		                                         _settings.topn_frames_per_shot);

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
		                                  _user_context.ctx.curr_disp_type, ids, "", _settings.topn_frames_per_video,
		                                  _settings.topn_frames_per_shot);
	}

	return get_page_from_last(page);
}

FramePointerRange Somhunter::get_page_from_last(PageId page)
{
	SHLOG_D("Getting page " << page << ", page size " << _settings.display_page_size);

	size_t begin_off{ std::min(_user_context.ctx.current_display.size(), page * _settings.display_page_size) };
	size_t end_off{ std::min(_user_context.ctx.current_display.size(),
		                     page * _settings.display_page_size + _settings.display_page_size) };

	FramePointerRange res(_user_context.ctx.current_display.cbegin() + begin_off,
	                      _user_context.ctx.current_display.cbegin() + end_off);

	// Update context
	for (auto iter = res.begin(); iter != res.end(); ++iter)
		// Skip "empty" frames
		if (*iter != nullptr) _user_context.ctx.shown_images.insert((*iter)->frame_ID);

	return res;
}

void Somhunter::reset_scores(float val)
{
	_user_context.ctx.used_tools.reset();
	_user_context.ctx.scores.reset(val);
}

const UserContext& Somhunter::switch_search_context(size_t index, size_t src_search_ctx_ID,
                                                    const std::string& screenshot_fpth, const std::string& label)
{
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
