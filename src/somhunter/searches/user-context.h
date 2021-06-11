

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

#ifndef USER_CONTEXT_H_
#define USER_CONTEXT_H_

#include <array>
#include <memory>
#include <optional>
#include <vector>

#include "common.h"

#include "async-som.h"
#include "logger.h"
#include "search-context.h"
#include "eval-server-client.h"

namespace sh {

class DatasetFrames;
class DatasetFeatures;

/** Represents exactly one state of ONE user that uses this core. */
class UserContext {
public:
	UserContext() = delete;
	UserContext(const std::string& _user_token, const Settings& cfg, const DatasetFrames& _dataset_frames,
	            const DatasetFeatures _dataset_features);

	bool operator==(const UserContext& other) const;
	void reset() {
		// Reset SearchContext
		ctx.reset();
		// Make sure we're not pushing in any old screenshot
		ctx.screenshot_fpth = "";
		ctx.ID = 0;

		// Reset bookmarks
		_bookmarks.clear();

		_history.clear();
		_history.emplace_back(ctx);
	}

	
public:  //< This is temporary, until we support multiple users
	// *** SEARCH CONTEXT ***
	SearchContext ctx;

	// *** USER SPECIFIC ***
	std::string _user_token;
	std::string _user_eval_server_token;  //< For remote auth
	std::vector<SearchContext> _history;

	EvalServerClient _eval_server;
	Logger _logger;
	AsyncSom _async_SOM;
	std::vector<std::unique_ptr<AsyncSom>> _temp_async_SOM;

	/** Frames selected as important. */
	BookmarksCont _bookmarks;
};

/** Result type `get_display` returns */
struct GetDisplayResult {
	FramePointerRange _dataset_frames;
	const LikesCont& likes;
	const LikesCont& _bookmarks;
};

/** Result type `rescore` returns */
struct RescoreResult {
	size_t curr_ctx_ID;
	const std::vector<SearchContext>& _history;
	const std::vector<VideoFrame> targets;
	size_t target_pos;
};

};  // namespace sh

#endif  // USER_CONTEXT_H_
