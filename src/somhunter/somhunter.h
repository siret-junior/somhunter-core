/* This file is part of SOMHunter.
 *
 * Copyright (C) 2021 František Mejzlík <frankmejzlik@gmail.com>
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

#ifndef SOMHUNTER_H_
#define SOMHUNTER_H_

#include <cstddef>
#include <string>
#include <vector>
// ---
#include "async-som.h"
#include "canvas-query-ranker.h"
#include "common.h"
#include "dataset-features.h"
#include "dataset-frames.h"
#include "image-processor.h"
#include "keyword-clip-ranker.h"
#include "keyword-ranker.h"
#include "logger.h"
#include "query-types.h"
#include "relocation-ranker.h"
#include "scores.h"
#include "search-context.h"
#include "task-target-helper.h"
#include "user-context.h"
#include "utils.hpp"

namespace sh {

/**
 * The main C++ API of the SOMHunter Core.
 *
 * Every comunication from the outside world goes through this API.
 * This API can be called directly from other C++ code or can be called
 * by the HTTP API provided inside \ref `class NetworkApi`.
 */
class Somhunter {
public:
	
	Somhunter() = delete;
	/** The main ctor with the config from the JSON config file. */
	Somhunter(const std::string& config_filepath);

	// ---

	/**
	 * Returns display of the desired type.
	 *
	 *	Some diplays may even support paging (e.g. DISP_TOP_N) or selection of one frame (e.g. DISP_KNN).
	 */
	GetDisplayResult get_display(DisplayType d_type, FrameId selected_image = 0, PageId page = 0, bool log_it = true);

	/** Inverts the like states of the provided frames and returns the new states. */
	std::vector<bool> like_frames(const std::vector<FrameId>& new_likes);

	/** (De)selects the provided frames from the bookmark list. */
	std::vector<bool> bookmark_frames(const std::vector<FrameId>& new_bookmarks);

	/** Returns the nearest supported keyword matches to the provided prefix. */
	std::vector<const Keyword*> autocomplete_keywords(const std::string& prefix, size_t count = 5) const;

	/**
	 * Applies all algorithms for score computation and updates context.
	 *
	 * Returns references to existing history states that we can go back to (including the current one).
	 */
	RescoreResult rescore(Query& query, bool benchmark_run = false);

	/**
	 * Switches the search context for the user to the provided index in the history and returns reference to it.
	 */
	const UserContext& switch_search_context(size_t index, size_t src_search_ctx_ID = SIZE_T_ERR_VAL,
	                                         const std::string& screenshot_fpth = "", const std::string& label = "");

	/**
	 * Returns a reference to the current user's search context.
	 */
	const SearchContext& get_search_context() const;

	/**
	 * Returns a reference to the current user's context.
	 */
	const UserContext& get_user_context() const;

	const VideoFrame& get_frame(FrameId ID) const;

	FrameRange get_frames(VideoId video_ID, FrameNum fr, FrameNum to) const;

	VideoFramePointer get_frame_ptr(FrameId img) const;

	/** Returns true if the user's SOM is ready */
	bool som_ready() const;

	bool som_ready(size_t temp_id) const;

	/** Resets current search context and starts new search */
	void reset_search_session();

	// ********************************
	// Remote server related calls
	// ********************************

	/**
	 * Tries to login into the remote evaluation server (competition one).
	 */
	bool login_to_eval_server();

	/**
	 * Tries to logout from the remote evaluation server (competition one).
	 */
	bool logout_from_eval_server();

	/** Sumbits frame with given id to VBS server */
	SubmitResult submit_to_eval_server(FrameId frame_id);

	// ********************************
	// Logging calls
	// ********************************

	/*
	 * Log events that need to be triggered from the outside (e.g. the UI).
	 */
	void log_video_replay(FrameId frame_ID, float delta_X);
	void log_scroll(DisplayType t, float delta_Y);
	void log_text_query_change(const std::string& text_query);
	void log_canvas_query_change();

	// ********************************
	// Image manipulation utilites
	// ********************************

	std::string store_rescore_screenshot(const std::string& filepath);

	size_t get_num_frames() const { return _dataset_frames.size(); }
	const std::vector<FrameId>& get_top_scored(size_t max_count = 0, size_t from_video = 0, size_t from_shot = 0) const;
	std::vector<VideoFramePointer> get_top_scored_frames(size_t max_count = 0, size_t from_video = 0,
	                                                     size_t from_shot = 0) const;
	std::vector<float> get_top_scored_scores(std::vector<FrameId>& top_scored_frames) const;
	size_t find_targets(const std::vector<FrameId>& top_scored, const std::vector<FrameId>& targets) const;

	// ********************************
	// Other
	// ********************************
	const Settings& settings() const { return _settings; }
	const std::string& get_config_filepath() { return _core_settings_filepath; }
	const std::string& get_API_config_filepath() { return _API_settings_filepath; }
	const KeywordRanker* textual_model() { return &_keyword_ranker; };

	void benchmark_native_text_queries(const std::string& queries_filepath, const std::string& out_dir);
	void benchmark_canvas_queries(const std::string& queries_dir, const std::string& out_dir);
	void benchmark_real_queries(const std::string& queries_dir, const std::string& targets_fpth,
	                            const std::string& out_dir);

	/**
	 * Generates the top example images from the database for all supported W2VV keywords.
	 *
	 * For examples used as `keyword-to-ID.W2VV-BoW.csv` file.
	 */
	void generate_example_images_for_keywords();

	static void write_resultset(const std::string& file, const std::vector<VideoFramePointer>& results);
	static void write_query(const std::string& file, const Query& q);
	static void write_query_info(const std::string& file, const std::string& ID, const std::string& user,
	                             const std::tuple<VideoId, FrameId, FrameId>& target, std::size_t pos_vid,
	                             std::size_t pos_fr, std::size_t unpos_vid, std::size_t unpos_fr);

	void run_basic_test();
	void run_generators();

private:
	void apply_filters();

	/**
	 *	Generates the new debug targets.
	 */
	void generate_new_targets();

	/**
	 *	Applies text query from the user.
	 */
	template <typename SpecificKWRanker, typename SpecificFrameFeatures>
	void rescore_keywords(SpecificKWRanker& kw_ranker, const TextualQuery& query, size_t temporal,
	                      const SpecificFrameFeatures& features);

	/**
	 * Applies the relevance feedback from the user based on images
	 * the user already saw (as implicit negative examples).
	 */
	void rescore_feedback();

	/**
	 *	Gives the SOM worker the new work.
	 */
	void som_start(size_t temporal);

	FramePointerRange get_random_display();

	FramePointerRange get_topn_display(PageId page);

	FramePointerRange get_topn_context_display(PageId page);

	FramePointerRange get_som_display();

	FramePointerRange get_som_relocation_display(size_t temp_id);

	FramePointerRange get_video_detail_display(FrameId selected_image, bool log_it = true);

	FramePointerRange get_topKNN_display(FrameId selected_image, PageId page);

	// Gets only part of last display
	FramePointerRange get_page_from_last(PageId page);

	void reset_scores(float val = 1.0F);

	/**
	 * Adds the currently active search context to the history and starts a new
	 * context (with next contiguous ID number)
	 */
	void push_search_ctx();

	/**
	 * Returns true if LSC metadata file provided inside the config.
	 */
	bool has_metadata() const;

	/** The tester class for this one. */
	friend sh::tests::TESTER_Somhunter;

	// ---
	/** Current application settings. */
	const Settings _settings;

	// ********************************
	// Loaded dataset
	//		(shared for all the users)
	// ********************************
	const DatasetFrames _dataset_frames;
	const DatasetFeatures _dataset_features;

	// ********************************
	// User contexts
	//		(private for each unique user session)
	// ********************************
	UserContext _user_context;

	// ********************************
	// Services
	//		(shared for all the users)
	// ********************************
	const std::string _core_settings_filepath;
	const std::string _API_settings_filepath;

	KeywordRanker _keyword_ranker;
	KeywordClipRanker _secondary_keyword_ranker;
	CanvasQueryRanker _collage_ranker;
	const RelocationRanker _relocation_ranker;
};

// ---

template <typename SpecificKWRanker, typename SpecificFrameFeatures>
void Somhunter::rescore_keywords(SpecificKWRanker& kw_ranker, const TextualQuery& query, size_t temporal,
                                 const SpecificFrameFeatures& features) {
	kw_ranker.rank_sentence_query(query, _user_context.ctx.scores, features, temporal);

	_user_context.ctx.used_tools.text_search_used = true;
}

};      // namespace sh
#endif  // SOMHUNTER_H_
