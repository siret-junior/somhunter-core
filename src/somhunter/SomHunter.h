
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

#ifndef SOMHUNTER_H_
#define SOMHUNTER_H_

#include <array>
#include <set>
#include <string>
#include <vector>

#include "async-som.h"
#include "canvas-query-ranker.h"
#include "dataset-features.h"
#include "dataset-frames.h"
#include "image-processor.h"
#include "keyword-ranker.h"
#include "logger.h"
#include "query-types.h"
#include "relocation-ranker.h"
#include "scores.h"
#include "search-context.h"
#include "user-context.h"
#include "utils.hpp"

namespace sh {

namespace tests {
class TESTER_SomHunter;
}  // namespace tests

/**
 * The main API of the SOMHunter Core.
 *
 * \todo Export for a library compilation.
 */
class SomHunter {
	// ********************************
	// Loaded dataset
	//		(shared for all the users)
	// ********************************
	const std::string _config_filepath;
	const std::string _API_config_filepath;
	const Config config;
	const DatasetFrames frames;
	const DatasetFeatures features;
	KeywordRanker keywords;
	CanvasQueryRanker collageRanker;
	const RelocationRanker relocationRanker;

	// ********************************
	// User contexts
	//		(private for each unique user session)
	// ********************************
public:
	UserContext user;  // This will become std::vector<UserContext>

	SomHunter() = delete;
	/** The main ctor with the config from the JSON config file. */
	inline SomHunter(const Config& cfg, const std::string& config_filepath)
	    : _config_filepath{ config_filepath },
	      _API_config_filepath{ cfg.API_config.config_filepath },
	      config(cfg),
	      frames(cfg),
	      features(frames, cfg),
	      keywords(cfg, frames),
	      collageRanker(cfg, &keywords),
	      user(cfg.user_token, cfg, frames, features),
	      relocationRanker{} {
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
	}

	// ********************************
	// Interactive search calls
	// ********************************

	/**
	 * Returns display of desired type
	 *
	 *	Some diplays may even support paging (e.g. top_n) or
	 * selection of one frame (e.g. top_knn)
	 */
	GetDisplayResult get_display(DisplayType d_type, ImageId selected_image = 0, PageId page = 0, bool log_it = true);

	/** Inverts the like states of the provided frames and returns the new
	 * states. */
	std::vector<bool> like_frames(const std::vector<ImageId>& new_likes);

	/** (De)selects the provided frames from the bookmark list. */
	std::vector<bool> bookmark_frames(const std::vector<ImageId>& new_bookmarks);

	/** Returns the nearest supported keyword matches to the provided
	 * prefix. */
	std::vector<const Keyword*> autocomplete_keywords(const std::string& prefix, size_t count) const;

	/**
	 * Applies all algorithms for score computation and updates context.
	 *
	 * Returns references to existing history states that we can go back to
	 * (including the current one).
	 */
	RescoreResult rescore(const Query& query, bool run_SOM = true);

	RescoreResult rescore(const std::vector<TemporalQuery>& temporal_query, const RelevanceFeedbackQuery& rfQuery,
	                      const Filters* p_filters = nullptr, size_t src_search_ctx_ID = SIZE_T_ERR_VAL,
	                      const std::string& screenshot_fpth = ""s, const std::string& label = ""s,
	                      bool run_SOM = true);

	/** Switches the search context for the user to the provided index in
	 *  the history and returns reference to it.
	 *
	 * To be extended with the `user_token` argument with multiple users
	 * support.
	 */
	const UserContext& switch_search_context(size_t index, size_t src_search_ctx_ID = SIZE_T_ERR_VAL,
	                                         const std::string& screenshot_fpth = "", const std::string& label = "");

	void apply_filters();

	/**
	 * Returns a reference to the current user's search context.
	 *
	 * To be extended with the `user_token` argument with multiple users
	 * support.
	 */
	const SearchContext& get_search_context() const;

	/**
	 * Returns a reference to the current user's context.
	 *
	 * To be extended with the `user_token` argument with multiple users
	 * support.
	 */
	const UserContext& get_user_context() const;

	const VideoFrame& get_frame(ImageId ID) const { return frames.get_frame(ID); }

	VideoFramePointer get_frame_ptr(ImageId img) const {
		if (img < frames.size()) return frames.get_frame_ptr(img);
		return nullptr;
	}

	/** Returns true if the user's SOM is ready */
	bool som_ready() const;

	bool som_ready(size_t temp_id) const;

	/** Resets current search context and starts new search */
	void reset_search_session();

	// ********************************
	// Remote server related calls
	// ********************************

	/**
	 * Tries to login into the DRES evaluation server
	 *		https://github.com/lucaro/DRES
	 */
	bool login_to_dres() const;

	/** Sumbits frame with given id to VBS server */
	void submit_to_server(ImageId frame_id);

	// ********************************
	// Logging calls
	// ********************************

	/*
	 * Log events that need to be triggered from the outside (e.g. the UI).
	 */
	void log_video_replay(ImageId frame_ID, float delta_X);
	void log_scroll(DisplayType t, float delta_Y);
	void log_text_query_change(const std::string& text_query);

	// ********************************
	// Image manipulation utilites
	// ********************************

	std::string store_rescore_screenshot(const std::string& filepath);

	size_t get_num_frames() const { return frames.size(); }
	std::vector<ImageId> get_top_scored(size_t max_count = 0, size_t from_video = 0, size_t from_shot = 0) const;
	std::vector<float> get_top_scored_scores(std::vector<ImageId>& top_scored_frames) const;
	size_t find_targets(const std::vector<ImageId>& top_scored, const std::vector<ImageId>& targets) const;

	// ********************************
	// Other
	// ********************************
	const std::string& get_config_filepath() { return _config_filepath; }
	const std::string& get_API_config_filepath() { return _API_config_filepath; }
	const KeywordRanker* textual_model() { return &keywords; };

	void benchmark_native_text_queries(const std::string& queries_filepath, const std::string& out_dir);

private:
	/**
	 *	Applies text query from the user.
	 */
	void rescore_keywords(const std::string& query, size_t temporal);

	/**
	 *	Applies feedback from the user based
	 * on shown_images.
	 */
	void rescore_feedback();

	/**
	 *	Gives SOM worker new work.
	 */
	void som_start(size_t temporal);

	FramePointerRange get_random_display();

	FramePointerRange get_topn_display(PageId page);

	FramePointerRange get_topn_context_display(PageId page);

	FramePointerRange get_som_display();

	FramePointerRange get_som_relocation_display(size_t temp_id);

	FramePointerRange get_video_detail_display(ImageId selected_image, bool log_it = true);

	FramePointerRange get_topKNN_display(ImageId selected_image, PageId page);

	// Gets only part of last display
	FramePointerRange get_page_from_last(PageId page);

	void reset_scores(float val = 1.0F);

	/** Adds currently active search context to the history and starts a new
	 * context (with next contiguous ID number) */
	void push_search_ctx() {
		// Make sure we're not pushing in any old screenshot
		user.ctx.screenshot_fpth = "";

		// Increment context ID
		user.ctx.ID = user.history.size();
		user.history.emplace_back(user.ctx);
	}

	bool has_metadata() const;

	/** The tester class */
	friend sh::tests::TESTER_SomHunter;
};

};  // namespace sh
#endif // SOMHUNTER_H_
