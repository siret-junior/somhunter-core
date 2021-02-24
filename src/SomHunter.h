
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

#ifndef somhunter_h
#define somhunter_h

#include <array>
#include <set>
#include <string>
#include <vector>

#include "utils.h"

#include "CollageRanker.h"
#include "DatasetFeatures.h"
#include "DatasetFrames.h"
#include "Filters.h"
#include "ImageManipulator.h"
#include "KeywordRanker.h"
#include "RelevanceScores.h"
#include "UserContext.h"

#include "AsyncSom.h"
#include "SearchContext.h"
#include "Submitter.h"

class TESTER_SomHunter;

/**
 * The main API of the SOMHunter Core.
 *
 * \todo Export for a library compilation.
 */
class SomHunter
{
	// ********************************
	// Loaded dataset
	//		(shared for all the users)
	// ********************************
	const Config config;
	const DatasetFrames frames;
	const DatasetFeatures features;
	const KeywordRanker keywords;
	CollageRanker collageRanker;

	// ********************************
	// User contexts
	//		(private for each unique user session)
	// ********************************
	UserContext user; // This will become std::vector<UserContext>

public:
	SomHunter() = delete;
	/** The main ctor with the config from the JSON config file. */
	inline SomHunter(const Config& cfg)
	  : config(cfg)
	  , frames(cfg)
	  , features(frames, cfg)
	  , keywords(cfg, frames)
	  , collageRanker(cfg)
	  , user(cfg.user_token, cfg, frames, features)
	{}

	// ********************************
	// Interactive search calls
	// ********************************

	/**
	 * Returns display of desired type
	 *
	 *	Some diplays may even support paging (e.g. top_n) or
	 * selection of one frame (e.g. top_knn)
	 */
	GetDisplayResult get_display(DisplayType d_type,
	                             ImageId selected_image = 0,
	                             PageId page = 0,
	                             bool log_it = true);

	/** Inverts the like states of the provided frames and returns the new
	 * states. */
	std::vector<bool> like_frames(const std::vector<ImageId>& new_likes);

	/** (De)selects the provided frames from the bookmark list. */
	std::vector<bool> bookmark_frames(const std::vector<ImageId>& new_bookmarks);

	/** Returns the nearest supported keyword matches to the provided
	 * prefix. */
	std::vector<const Keyword*> autocomplete_keywords(const std::string& prefix, size_t count) const;

	void rescore(Collage& collage);

	/**
	 * Applies all algorithms for score computation and updates context.
	 *
	 * Returns references to existing history states that we can go back to
	 * (including the current one).
	 */
	RescoreResult rescore(const std::string& text_query,
	                      Collage& collage,
	                      const Filters* p_filters = nullptr,
	                      size_t src_search_ctx_ID = SIZE_T_ERR_VAL,
	                      const std::string& screenshot_fpth = ""s,
	                      const std::string& label = ""s);

	RescoreResult rescore(const std::string& text_query,
	                      const Filters* p_filters = nullptr,
	                      size_t src_search_ctx_ID = SIZE_T_ERR_VAL,
	                      const std::string& screenshot_fpth = ""s,
	                      const std::string& label = ""s)
	{
		Collage c; // NULL instance
		return rescore(text_query, c, p_filters, src_search_ctx_ID, screenshot_fpth, label);
	}

	RescoreResult rescore(Collage& collage,
	                      const Filters* p_filters = nullptr,
	                      size_t src_search_ctx_ID = SIZE_T_ERR_VAL,
	                      const std::string& screenshot_fpth = ""s,
	                      const std::string& label = ""s)
	{
		return rescore(""s, collage, p_filters, src_search_ctx_ID, screenshot_fpth, label);
	}

	/** Switches the search context for the user to the provided index in
	 *  the history and returns reference to it.
	 *
	 * To be extended with the `user_token` argument with multiple users
	 * support.
	 */
	const UserContext& switch_search_context(size_t index,
	                                         size_t src_search_ctx_ID = SIZE_T_ERR_VAL,
	                                         const std::string& screenshot_fpth = "",
	                                         const std::string& label = "");

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

	/** Returns true if the user's SOM is ready */
	bool som_ready() const;

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

	/**
	 * Loads the image from the provided filepath.
	 *
	 * \exception std::runtime_error If the loading fails.
	 */
	LoadedImage load_image(const std::string& filepath) const { return ImageManipulator::load(filepath); }

	/**
	 * Writes the provided image into the JPG file.
	 *
	 * \exception std::runtime_error If the writing fails.
	 */
	void store_jpg_image(const std::string& filepath,
	                     const std::vector<float>& in,
	                     size_t w,
	                     size_t h,
	                     size_t quality,
	                     size_t num_channels) const
	{
		return ImageManipulator::store_jpg(filepath, in, w, h, quality, num_channels);
	}

	/**
	 * Creates a new resized copy of the provided image matrix.
	 *
	 * \exception std::runtime_error If the resizing fails.
	 *
	 * \param in	Image pixel matrix.
	 * \param orig_w	Original image width in pixels.
	 * \param orig_h	Original image height in pixels.
	 * \param orig_w	Target image width in pixels.
	 * \param orig_h	Target image height in pixels.
	 * \param num_channels	Number of channels aka number of elements representing one pixel.
	 * \return New copy of resized image.
	 */
	std::vector<float> resize_image(const std::vector<float>& in,
	                                size_t orig_w,
	                                size_t orig_h,
	                                size_t new_w,
	                                size_t new_h,
	                                size_t num_channels = 3) const
	{
		return ImageManipulator::resize(in, orig_w, orig_h, new_w, new_h, num_channels);
	}

private:
	/**
	 *	Applies text query from the user.
	 */
	void rescore_keywords(const std::string& query);

	/**
	 *	Applies feedback from the user based
	 * on shown_images.
	 */
	void rescore_feedback();

	/**
	 *	Gives SOM worker new work.
	 */
	void som_start();

	FramePointerRange get_random_display();

	FramePointerRange get_topn_display(PageId page);

	FramePointerRange get_topn_context_display(PageId page);

	FramePointerRange get_som_display();

	FramePointerRange get_video_detail_display(ImageId selected_image, bool log_it = true);

	FramePointerRange get_topKNN_display(ImageId selected_image, PageId page);

	// Gets only part of last display
	FramePointerRange get_page_from_last(PageId page);

	void reset_scores(float val = 1.0F);

	/** Adds currently active search context to the history and starts a new
	 * context (with next contiguous ID number) */
	void push_search_ctx()
	{
		// Make sure we're not pushing in any old screenshot
		user.ctx.screenshot_fpth = "";

		// Increment context ID
		user.ctx.ID = user.history.size();
		user.history.emplace_back(user.ctx);
	}

	bool has_metadata() const;

	/** The tester class */
	friend TESTER_SomHunter;
};

#endif
