
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
#include <optional>
#include <set>
#include <string>
#include <vector>
// ---

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

namespace sh
{
namespace tests
{
class TESTER_Somhunter;
}  // namespace tests

/**
 * The main API of the SOMHunter Core.
 *
 * \todo Export for a library compilation.
 */
class Somhunter
{
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
	UserContext _user_context;  // This will become std::vector<UserContext>

	// ********************************
	// Services
	//		(shared for all the users)
	// ********************************
	const std::string _core_settings_filepath;
	const std::string _API_settings_filepath;
	const Settings _settings;

	KeywordRanker _keyword_ranker;
	CanvasQueryRanker _collage_ranker;
	const RelocationRanker _relocation_ranker;

public:
	Somhunter() = delete;
	/** The main ctor with the config from the JSON config file. */
	inline Somhunter(const Settings& settings, const std::string& config_filepath)
	    : _core_settings_filepath{ config_filepath },
	      _API_settings_filepath{ settings.API_config.config_filepath },
	      _settings(settings),
	      _dataset_frames(settings),
	      _dataset_features(_dataset_frames, settings),
	      _keyword_ranker(settings, _dataset_frames),
	      _collage_ranker(settings, &_keyword_ranker),
	      _user_context(settings, /* \todo */ "admin", &_dataset_frames, &_dataset_features),
	      _relocation_ranker{}
	{
		generate_new_targets();
		return;
		std::ifstream inFile(settings.kws_file, std::ios::in);
		std::ofstream ofs("wooooords.csv");

		if (!inFile) {
			std::string msg{ "Error opening file: " + settings.kws_file };
			SHLOG_E(msg);
			throw std::runtime_error(msg);
		}

		std::vector<Keyword> result_keywords;

		std::size_t i{0};
		// read the input file by lines
		for (std::string line_text_buffer; std::getline(inFile, line_text_buffer);) {
			std::stringstream line_buffer_ss(line_text_buffer);

			std::vector<std::string> tokens;

			// Tokenize this line
			for (std::string token; std::getline(line_buffer_ss, token, ':');) {
				tokens.push_back(token);
			}

			SynsetId synset_ID{ utils::str2<SynsetId>(tokens[1]) };
			FrameId vec_idx{ FrameId(synset_ID) };

			TemporalQuery tq;
			tq.textual = tokens[0];


			Query q;
			q.temporal_queries.push_back(tq);

			rescore(q, true);
			auto res { get_top_scored(10, 1, 3)};

			ofs << tokens[0] << ":" << synset_ID << ":";

			for (auto&& x : res) {
				ofs << x << "#";
			}

			ofs << std::endl;

			if (i % 100 == 0) {
				SHLOG(i);
			}
			++i;
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
	GetDisplayResult get_display(DisplayType d_type, FrameId selected_image = 0, PageId page = 0, bool log_it = true);

	/** Inverts the like states of the provided frames and returns the new
	 * states. */
	std::vector<bool> like_frames(const std::vector<FrameId>& new_likes);

	/** (De)selects the provided frames from the bookmark list. */
	std::vector<bool> bookmark_frames(const std::vector<FrameId>& new_bookmarks);

	/** Returns the nearest supported keyword matches to the provided
	 * prefix. */
	std::vector<const Keyword*> autocomplete_keywords(const std::string& prefix, size_t count) const;

	/**
	 * Applies all algorithms for score computation and updates context.
	 *
	 * Returns references to existing history states that we can go back to
	 * (including the current one).
	 */
	RescoreResult rescore(Query& query, bool benchmark_run = false);

	RescoreResult rescore(const std::vector<TemporalQuery>& temporal_query, const RelevanceFeedbackQuery& rfQuery,
	                      const Filters* p_filters = nullptr, size_t src_search_ctx_ID = SIZE_T_ERR_VAL,
	                      const std::string& screenshot_fpth = ""s, const std::string& label = ""s,
	                      bool benchmark_run = false);

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

	const VideoFrame& get_frame(FrameId ID) const { return _dataset_frames.get_frame(ID); }

	VideoFramePointer get_frame_ptr(FrameId img) const
	{
		if (img < _dataset_frames.size()) return _dataset_frames.get_frame_ptr(img);
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
	bool login_to_eval_server();
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

	// ********************************
	// Image manipulation utilites
	// ********************************

	std::string store_rescore_screenshot(const std::string& filepath);

	size_t get_num_frames() const { return _dataset_frames.size(); }
	std::vector<FrameId> get_top_scored(size_t max_count = 0, size_t from_video = 0, size_t from_shot = 0) const;
	std::vector<float> get_top_scored_scores(std::vector<FrameId>& top_scored_frames) const;
	size_t find_targets(const std::vector<FrameId>& top_scored, const std::vector<FrameId>& targets) const;

	// ********************************
	// Other
	// ********************************
	const std::string& get_config_filepath() { return _core_settings_filepath; }
	const std::string& get_API_config_filepath() { return _API_settings_filepath; }
	const KeywordRanker* textual_model() { return &_keyword_ranker; };

	void benchmark_native_text_queries(const std::string& queries_filepath, const std::string& out_dir);
	void benchmark_canvas_queries(const std::string& queries_dir, const std::string& out_dir);

private:
	/**
	 *	Generates the new debug targets.
	 */
	void generate_new_targets();

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

	FramePointerRange get_video_detail_display(FrameId selected_image, bool log_it = true);

	FramePointerRange get_topKNN_display(FrameId selected_image, PageId page);

	// Gets only part of last display
	FramePointerRange get_page_from_last(PageId page);

	void reset_scores(float val = 1.0F);

	/** Adds currently active search context to the history and starts a new
	 * context (with next contiguous ID number) */
	void push_search_ctx()
	{
		// Make sure we're not pushing in any old screenshot
		_user_context.ctx.screenshot_fpth = "";

		// Increment context ID
		_user_context.ctx.ID = _user_context._history.size();
		_user_context._history.emplace_back(_user_context.ctx);
	}

	bool has_metadata() const;

	/** The tester class */
	friend sh::tests::TESTER_Somhunter;
};

};      // namespace sh
#endif  // SOMHUNTER_H_
