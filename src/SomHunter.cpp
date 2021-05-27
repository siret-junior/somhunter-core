
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

#include "SomHunter.h"

#include "log.h"
#include "utils.h"

using namespace sh;

GetDisplayResult SomHunter::get_display(DisplayType d_type, ImageId selected_image, PageId page, bool log_it) {
	user.submitter.poll();

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

	return GetDisplayResult{ frs, user.ctx.likes, user.bookmarks };
}

std::vector<bool> SomHunter::like_frames(const std::vector<ImageId>& new_likes) {
	user.submitter.poll();

	// Prepare the result flags vector
	std::vector<bool> res;
	res.reserve(new_likes.size());

	for (auto&& fr_ID : new_likes) {
		// Find the item in the set
		size_t count{ user.ctx.likes.count(fr_ID) };

		// If item is not present (NOT LIKED)
		if (count == 0) {
			// Like it
			user.ctx.likes.insert(fr_ID);
			res.emplace_back(true);

			user.submitter.log_like(frames, user.ctx.likes, user.ctx.curr_disp_type, fr_ID);
		}
		// If the item is present (LIKED)
		else {
			// Unlike it
			user.ctx.likes.erase(fr_ID);
			res.emplace_back(false);

			user.submitter.log_unlike(frames, user.ctx.likes, user.ctx.curr_disp_type, fr_ID);
		}
	}

	return res;
}

std::vector<bool> SomHunter::bookmark_frames(const std::vector<ImageId>& new_bookmarks) {
	user.submitter.poll();

	// Prepare the result flags vector
	std::vector<bool> res;
	res.reserve(new_bookmarks.size());

	for (auto&& fr_ID : new_bookmarks) {
		// Find the item in the set
		size_t count{ user.bookmarks.count(fr_ID) };

		// If item is not present (NOT LIKED) -> bookmark it
		if (count == 0) {
			user.bookmarks.insert(fr_ID);
			res.emplace_back(true);

			// \todo Log it?
		}
		// If the item is present (LIKED) -> unbookmark it
		else {
			user.bookmarks.erase(fr_ID);
			res.emplace_back(false);

			// \todo Log it?
		}
	}

	return res;
}

std::vector<const Keyword*> SomHunter::autocomplete_keywords(const std::string& prefix, size_t count) const {
	// Trivial case
	if (prefix.empty()) return std::vector<const Keyword*>{};

	auto lowercase_prefix{ utils::to_lowercase(prefix) };

	// Get the keywrods IDs
	auto kw_IDs{ keywords.find(lowercase_prefix, count) };

	// Create vector of ptrs to corresponding keyword instances
	std::vector<const Keyword*> res;
	res.reserve(kw_IDs.size());
	for (auto&& kw_ID : kw_IDs) {
		res.emplace_back(&keywords[kw_ID.first]);
	}

	return res;
}

bool SomHunter::has_metadata() const { return !config.LSC_metadata_file.empty(); }

void SomHunter::apply_filters() {
	// If no filters set up
	if (!has_metadata()) {
		return;
	}

	// Make sure to reset the previous mask on the scores
	user.ctx.scores.reset_mask();

	const Filters& filters{ user.ctx.filters };

	const auto& days{ filters.days };
	Hour t_from{ filters.time.from };
	Hour t_to{ filters.time.to };

	// A closure that determines if the frame should be filtered out
	auto is_out{ [&days, t_from, t_to](const VideoFrame& f) {
		// If NOT within the selected days
		if (!days[f.weekday]) return true;

		// If NOT within the hour range
		if (t_from > f.hour || f.hour > t_to) return true;

		return false;
	} };

	ImageId frame_ID{ 0 };
	for (auto&& f : frames) {
		// If should be filtered out
		if (is_out(f)) {
			user.ctx.scores.set_mask(frame_ID, false);
		}

		++frame_ID;
	}
}

RescoreResult SomHunter::rescore(const Query& query, bool run_SOM) {
	return rescore(query.temporal_queries, query.relevance_feeedback, &query.filters, query.metadata.srd_search_ctx_ID,
	               query.metadata.screenshot_filepath, query.metadata.time_label, run_SOM);
}

RescoreResult SomHunter::rescore(const std::vector<TemporalQuery>& temporal_query,
                                 const RelevanceFeedbackQuery& rfQuery,
                                 const Filters* p_filters, size_t src_search_ctx_ID, const std::string& screenshot_fpth,
                                 const std::string& label, bool run_SOM) {
	/* ***
	 * Set the filters to the context
	 */
	if (p_filters != nullptr && has_metadata()) {
		user.ctx.filters = *p_filters;

		// If filters used
		if (!user.ctx.filters.is_default()) {
			user.ctx.used_tools.filters = &user.ctx.filters;
		}
		// Else reset filters used
		else {
			user.ctx.used_tools.filters = nullptr;
		}
	}

	/* ***
	 * Save provided screenshot filepath if needed
	 */
	if (user.history.size() <= src_search_ctx_ID) {
		return RescoreResult{ user.ctx.ID, user.history, user.ctx.curr_targets };
	}

	if (src_search_ctx_ID != SIZE_T_ERR_VAL && user.history[src_search_ctx_ID].screenshot_fpth.empty()) {
		user.history[src_search_ctx_ID].label = label;
		user.history[src_search_ctx_ID].screenshot_fpth = screenshot_fpth;
	}

	/* ***
	 * Do all the needed rescore steps
	 */
	// Store likes for the logging purposees
	auto old_likes{ rfQuery };

	// Check if temporal queries has changed
	if (user.ctx.last_temporal_queries != temporal_query) {
		reset_scores();
		user.ctx.temporal_size = temporal_query.size();
		size_t moment = 0;

		user.submitter.log_canvas_query(
			temporal_query,
			&user.ctx.curr_targets);  // < This triggers log into the /logs/collage/

		for (size_t mi = 0; mi < temporal_query.size(); ++mi) {
			auto&& moment_query = temporal_query[mi];
			auto&& last_moment_query = user.ctx.last_temporal_queries[mi];

			if (moment_query.isRelocation()) {
				SHLOG_D("Running the relocation query model...");
				relocationRanker.score(moment_query.relocation, user.ctx.scores, moment, features);
			} else if (moment_query.isCanvas()) {
				SHLOG_D("Running the canvas query model...");
				// canvas_query.set_targets(user.ctx.curr_targets);
				collageRanker.score(moment_query.canvas, user.ctx.scores, moment, features, frames);

			} else if (moment_query.isText()) {
				SHLOG_D("Running plain texual model...");
				rescore_keywords(moment_query.textual, moment);
			}
			++moment;
		}
		// Cache the appliend temporal queries
		user.ctx.last_temporal_queries = temporal_query;
		// Apply temporal fusion
		user.ctx.scores.apply_temporals(user.ctx.temporal_size, frames);
		// Normalize the scores
		user.ctx.scores.normalize(user.ctx.temporal_size);
	}

	apply_filters();
	// Apply feedback and normalize
	rescore_feedback();

	// If SOM required
	if (run_SOM) {
		// Notify the SOM worker thread
		som_start(user.ctx.temporal_size);
	}

	// Reset the "seen frames" constext for the Bayes
	user.ctx.shown_images.clear();

	// Reset likes
	user.ctx.likes.clear();

	// Start the new search context
	push_search_ctx();

	/* ***
	 * Logging
	 */
	// Flush the backlog
	user.submitter.poll();
	auto top_n = user.ctx.scores.top_n(frames, TOPN_LIMIT, config.topn_frames_per_video, config.topn_frames_per_shot);

	// Log this rescore result
	user.submitter.submit_and_log_rescore(frames, user.ctx.scores, old_likes, user.ctx.used_tools,
	                                      user.ctx.curr_disp_type, top_n, "TODO add text query logging",
	                                      config.topn_frames_per_video, config.topn_frames_per_shot);

	// !!!!
	// Generate new targets
	// !!!!
	{
		size_t num_frames{ frames.size() };
		ImageId target_ID{ utils::irand<ImageId>(1, num_frames - 2) };

		// Get next or prev frame to create pair from the same video
		const auto& prevf{ frames.get_frame(target_ID - 1) };
		const auto& f{ frames.get_frame(target_ID) };
		const auto& nextf{ frames.get_frame(target_ID + 1) };

		std::vector<VideoFrame> targets;
		targets.reserve(2);
		if (prevf.video_ID == f.video_ID) {
			targets.emplace_back(prevf);
			targets.emplace_back(f);
		} else {
			targets.emplace_back(f);
			targets.emplace_back(nextf);
		}
		user.ctx.curr_targets = std::move(targets);
	}

	return RescoreResult{ user.ctx.ID, user.history, user.ctx.curr_targets };
}

bool SomHunter::som_ready() const { return user.async_SOM.map_ready(); }

bool SomHunter::login_to_dres() const { return user.submitter.login_to_DRES(); }

void SomHunter::submit_to_server(ImageId frame_id) {
	user.submitter.submit_and_log_submit(frames, user.ctx.curr_disp_type, frame_id);
}

void SomHunter::reset_search_session() {
	user.submitter.poll();

	user.ctx.shown_images.clear();
	user.ctx.likes.clear();
	user.ctx.last_temporal_queries.clear();

	reset_scores();

	user.submitter.log_reset_search();
	som_start(MAX_NUM_TEMP_QUERIES);

	// Reset UserContext
	user.reset();
}

void SomHunter::log_video_replay(ImageId frame_ID, float delta_X) {
	user.submitter.log_show_video_replay(frames, frame_ID, delta_X);
}

void SomHunter::log_scroll(DisplayType t, float dir_Y) { user.submitter.log_scroll(frames, t, dir_Y); }

void SomHunter::log_text_query_change(const std::string& text_query) {
	user.submitter.log_text_query_change(text_query);
}

std::string SomHunter::store_rescore_screenshot(const std::string& /*filepath*/) {
	SHLOG_W("Simulating the screenshot saving...");

	std::string UI_filepath{ "/assets/img/history_screenshot.jpg" };

	// Return the filepath the UI can use to render it
	return UI_filepath;
}

std::vector<ImageId> SomHunter::get_top_scored(size_t max_count, size_t from_video, size_t from_shot) const {
	return user.ctx.scores.top_n(frames, max_count, from_video, from_shot);
}

std::vector<float> SomHunter::get_top_scored_scores(std::vector<ImageId>& top_scored_frames) const {
	std::vector<float> res;
	for (auto&& frame_ID : top_scored_frames) {
		res.emplace_back(user.ctx.scores[frame_ID]);
	}

	return res;
}

size_t sh::SomHunter::find_targets(const std::vector<ImageId>& top_scored, const std::vector<ImageId>& targets) const {
	size_t i{ 0 };
	for (auto it{ top_scored.begin() }; it != top_scored.end(); ++it) {
		ImageId curr_ID{ *it };

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

void SomHunter::benchmark_native_text_queries(const std::string& queries_filepath, const std::string& out_dir) {
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
		ImageId target_frame_ID{ utils::str2<ImageId>(tokens[0]) };
		std::string text_query{ std::move(tokens[1]) };

		// Split query into temporal one
		size_t pos = 0;
		std::string token;
		std::vector<TextualQuery> tempQuery;
		while ((pos = text_query.find(delimiter)) != std::string::npos) {
			tempQuery.push_back(text_query.substr(0, pos));
			text_query.erase(0, pos + delimiter.length());
		}

		bench_queries.emplace_back(PlainTextBenchmarkQuery{ std::vector<ImageId>{ target_frame_ID }, std::move(tempQuery) });
	}

	std::vector<std::vector<float>> query_results;
	std::vector<std::vector<ImageId>> query_results_IDs;

	std::vector<size_t> ranks;
	for (size_t i{ 0 }; i < bench_queries.size(); ++i) {
		auto& bq{ bench_queries[i] };

		reset_scores();
		Query q{ bq.text_query };
		rescore(q, false);

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
		std::cout << i << "," << hist[i] << std::endl;
	}
}

void SomHunter::rescore_keywords(const TextualQuery& query, size_t temporal) {

	keywords.rank_sentence_query(query, user.ctx.scores, features, config, temporal);

	user.ctx.used_tools.KWs_used = true;
}

void SomHunter::rescore_feedback() {
	if (user.ctx.likes.empty()) return;

	user.ctx.scores.apply_bayes(user.ctx.likes, user.ctx.shown_images, features);
	user.ctx.used_tools.bayes_used = true;
}

void SomHunter::som_start(size_t temporal) {
	user.async_SOM.start_work(features, user.ctx.scores, user.ctx.scores.v());
	for (size_t i = 0; i < temporal; ++i) {
		user.temp_async_SOM[i]->start_work(features, user.ctx.scores, user.ctx.scores.temp(i));
	}
}

FramePointerRange SomHunter::get_random_display() {
	// Get ids
	auto ids = user.ctx.scores.weighted_sample(DISPLAY_GRID_WIDTH * DISPLAY_GRID_HEIGHT, RANDOM_DISPLAY_WEIGHT);

	// Log
	user.submitter.log_show_random_display(frames, ids);
	// Update context
	for (auto id : ids) user.ctx.shown_images.insert(id);
	user.ctx.current_display = frames.ids_to_video_frame(ids);
	user.ctx.curr_disp_type = DisplayType::DRand;

	return FramePointerRange(user.ctx.current_display);
}

FramePointerRange SomHunter::get_topn_display(PageId page) {
	// Another display or first page -> load
	if (user.ctx.curr_disp_type != DisplayType::DTopN || page == 0) {
		SHLOG_D("Loading top n display first page");
		// Get ids
		auto ids = user.ctx.scores.top_n(frames, TOPN_LIMIT, config.topn_frames_per_video, config.topn_frames_per_shot);

		// Log only if page 0
		if (page == 0) user.submitter.log_show_topn_display(frames, ids);

		// Update context
		user.ctx.current_display = frames.ids_to_video_frame(ids);
		user.ctx.curr_disp_type = DisplayType::DTopN;
	}

	return get_page_from_last(page);
}

FramePointerRange SomHunter::get_topn_context_display(PageId page) {
	// Another display or first page -> load
	if (user.ctx.curr_disp_type != DisplayType::DTopNContext || page == 0) {
		SHLOG_D("Loading top n context display first page");
		// Get ids
		auto ids = user.ctx.scores.top_n_with_context(frames, TOPN_LIMIT, config.topn_frames_per_video,
		                                              config.topn_frames_per_shot);

		// Log
		if (page == 0) user.submitter.log_show_topn_context_display(frames, ids);

		// Update context
		user.ctx.current_display = frames.ids_to_video_frame(ids);
		user.ctx.curr_disp_type = DisplayType::DTopNContext;
	}

	return get_page_from_last(page);
}

FramePointerRange SomHunter::get_som_display() {
	if (!user.async_SOM.map_ready()) {
		return FramePointerRange();
	}

	std::vector<ImageId> ids;
	ids.resize(SOM_DISPLAY_GRID_WIDTH * SOM_DISPLAY_GRID_HEIGHT);

	// Select weighted example from cluster
	for (size_t i = 0; i < SOM_DISPLAY_GRID_WIDTH; ++i) {
		for (size_t j = 0; j < SOM_DISPLAY_GRID_HEIGHT; ++j) {
			if (!user.async_SOM.map(i + SOM_DISPLAY_GRID_WIDTH * j).empty()) {
				ImageId id = user.ctx.scores.weighted_example(user.async_SOM.map(i + SOM_DISPLAY_GRID_WIDTH * j));
				ids[i + SOM_DISPLAY_GRID_WIDTH * j] = id;
			}
		}
	}

	auto begin = std::chrono::steady_clock::now();
	// Fix empty cluster
	std::vector<size_t> stolen_count(SOM_DISPLAY_GRID_WIDTH * SOM_DISPLAY_GRID_HEIGHT, 1);
	for (size_t i = 0; i < SOM_DISPLAY_GRID_WIDTH; ++i) {
		for (size_t j = 0; j < SOM_DISPLAY_GRID_HEIGHT; ++j) {
			if (user.async_SOM.map(i + SOM_DISPLAY_GRID_WIDTH * j).empty()) {
				SHLOG_D("Fixing cluster " << i + SOM_DISPLAY_GRID_WIDTH * j);

				// Get SOM node of empty cluster
				auto k = user.async_SOM.get_koho(i + SOM_DISPLAY_GRID_WIDTH * j);

				// Get nearest cluster with enough elements
				size_t clust = user.async_SOM.nearest_cluster_with_atleast(k, stolen_count);

				stolen_count[clust]++;
				std::vector<ImageId> ci = user.async_SOM.map(clust);

				for (ImageId ii : ids) {
					auto fi = std::find(ci.begin(), ci.end(), ii);
					if (fi != ci.end()) ci.erase(fi);
				}

				// If some frame candidates
				if (!ci.empty()) {
					ImageId id = user.ctx.scores.weighted_example(ci);
					ids[i + SOM_DISPLAY_GRID_WIDTH * j] = id;
				}
				// Subsitute with "empty" frame
				else {
					ids[i + SOM_DISPLAY_GRID_WIDTH * j] = ERR_VAL<ImageId>();
				}
			}
		}
	}
	auto end = std::chrono::steady_clock::now();
	SHLOG_D("Fixing clusters took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
	                                << " [ms]");

	// Log
	user.submitter.log_show_som_display(frames, ids);

	// Update context
	for (auto id : ids) {
		if (id == IMAGE_ID_ERR_VAL) continue;

		user.ctx.shown_images.insert(id);
	}
	user.ctx.current_display = frames.ids_to_video_frame(ids);
	user.ctx.curr_disp_type = DisplayType::DSom;

	return FramePointerRange(user.ctx.current_display);
}

FramePointerRange SomHunter::get_video_detail_display(ImageId selected_image, bool log_it) {
	VideoId v_id = frames.get_video_id(selected_image);

	if (v_id == VIDEO_ID_ERR_VAL) {
		SHLOG_E("Video for " << selected_image << " not found");
		return std::vector<VideoFramePointer>();
	}

	// Get ids
	FrameRange video_frames = frames.get_all_video_frames(v_id);

	// Log
	if (log_it) user.submitter.log_show_detail_display(frames, selected_image);

	// Update context
	for (auto iter = video_frames.begin(); iter != video_frames.end(); ++iter) {
		user.ctx.shown_images.insert(iter->frame_ID);
	}

	user.ctx.current_display = frames.range_to_video_frame(video_frames);
	user.ctx.curr_disp_type = DisplayType::DVideoDetail;

	return FramePointerRange(user.ctx.current_display);
}

FramePointerRange SomHunter::get_topKNN_display(ImageId selected_image, PageId page) {
	// Another display or first page -> load
	if (user.ctx.curr_disp_type != DisplayType::DTopKNN || page == 0) {
		// Get ids
		auto ids =
		    features.get_top_knn(frames, selected_image, config.topn_frames_per_video, config.topn_frames_per_shot);

		// Log only if the first page
		if (page == 0) {
			user.submitter.log_show_topknn_display(frames, selected_image, ids);
		}

		// Update context
		user.ctx.current_display = frames.ids_to_video_frame(ids);
		user.ctx.curr_disp_type = DisplayType::DTopKNN;

		// KNN is query by example so we NEED to log a rerank
		UsedTools ut;
		ut.topknn_used = true;

		user.submitter.submit_and_log_rescore(frames, user.ctx.scores, user.ctx.likes, ut, user.ctx.curr_disp_type, ids,
		                                      "TODO add text query logging", config.topn_frames_per_video,
		                                      config.topn_frames_per_shot);
	}

	return get_page_from_last(page);
}

FramePointerRange SomHunter::get_page_from_last(PageId page) {
	SHLOG_D("Getting page " << page << ", page size " << config.display_page_size);

	size_t begin_off{ std::min(user.ctx.current_display.size(), page * config.display_page_size) };
	size_t end_off{ std::min(user.ctx.current_display.size(),
		                     page * config.display_page_size + config.display_page_size) };

	FramePointerRange res(user.ctx.current_display.cbegin() + begin_off, user.ctx.current_display.cbegin() + end_off);

	// Update context
	for (auto iter = res.begin(); iter != res.end(); ++iter)
		// Skip "empty" frames
		if (*iter != nullptr) user.ctx.shown_images.insert((*iter)->frame_ID);

	return res;
}

void SomHunter::reset_scores(float val) {
	user.ctx.used_tools.reset();
	user.ctx.scores.reset(val);
}

const UserContext& SomHunter::switch_search_context(size_t index, size_t src_search_ctx_ID,
                                                    const std::string& screenshot_fpth, const std::string& label) {
	/*
	 * Save provided screenshot filepath if needed
	 */
	if (src_search_ctx_ID != SIZE_T_ERR_VAL && user.history[src_search_ctx_ID].screenshot_fpth.empty()) {
		user.history[src_search_ctx_ID].label = label;
		user.history[src_search_ctx_ID].screenshot_fpth = screenshot_fpth;
	}

	// Range check
	if (index >= user.history.size()) {
		std::string msg{ "Index is out of bounds: " + index };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	SHLOG_I("Switching to context '" << index << "'...");

	// SOM must stop first
	while (!user.async_SOM.map_ready()) {
		std::this_thread::sleep_for(10ms);
	}

	// Get the desired state
	const auto& destContext{ user.history[index] };

	// Copy the history state into the current one
	user.ctx = SearchContext{ destContext };

	// Kick-off the SOM for the old-new state
	som_start(user.ctx.temporal_size);

	// Returnp ptr to it
	return user;
}

const SearchContext& SomHunter::get_search_context() const { return user.ctx; }

const UserContext& SomHunter::get_user_context() const { return user; }
