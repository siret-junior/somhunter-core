/* This file is part of SOMHunter.
 *
 * Copyright (C) 2021 Frantisek Mejzlik <frankmejzlik@gmail.com>
 *                    Mirek Kratochvil <exa.exa@gmail.com>
 *                    Patrik Vesely <prtrikvesely@gmail.com>
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

#ifndef SEARCH_CONTEXT_H_
#define SEARCH_CONTEXT_H_

#include <optional>
#include <string>
#include <vector>

#include "common.h"
#include "utils.hpp"

#include "dataset-frames.h"
#include "query-types.h"
#include "scores.h"

namespace sh {
class DatasetFrames;
class DatasetFeatures;

/**
 * Represents exactly one momentary state of a search session.
 *
 * It can be ome point in HISTORY.
 */
struct SearchContext {
	// *** METHODS ***
public:
	SearchContext() = delete;
	SearchContext(size_t ID, const Settings& _logger_settings, const DatasetFrames& _dataset_frames);
	bool operator==(const SearchContext& other) const;
	// ---

	void reset() {
		scores.reset_mask();
		reset_filters();
	}
	void reset_filters() { filters = Filters{}; }

	// *** MEMBER VARIABLES ***
public:
	std::size_t ID;

	// VBS logging
	UsedTools used_tools;

	// Current display context
	std::vector<VideoFramePointer> current_display;
	DisplayType curr_disp_type{ DisplayType::DTopN };

	// Relevance scores
	ScoreModel scores;

	// Size of temporal query
	size_t temporal_size;

	// Used temporal queries
	std::vector<TemporalQuery> last_temporal_queries;

	// Relevance feedback context
	LikesCont likes;

	/** Frames that were seen since the last rescore. */
	ShownFramesCont shown_images;

	// Filepath to screenshot repesenting this screen
	std::string screenshot_fpth{};

	std::string label;

	/** Filters based on metadata (hour, weekday). */
	Filters filters;

	/** Target we were looking for. (Data collection.) */
	std::vector<VideoFrame> curr_targets;

	Query _prev_query;
};

};  // namespace sh

#endif  // SEARCH_CONTEXT_H_
