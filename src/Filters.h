

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

#ifndef FILTERS_H_
#define FILTERS_H_

#include <array>

#include "common.h"
#include "utils.h"

namespace sh {

class WeekDaysFilter {
public:
	/** Default state is all dayes */
	WeekDaysFilter() { _days.fill(true); }

	/** Construct from the bit mask */
	WeekDaysFilter(uint8_t mask) {
		// Set the according days, ignore the last 2 bits
		for (size_t i{ 0 }; i < 7; ++i) {
			_days[i] = utils::is_set(mask, i);
		}
	}

	const bool& operator[](size_t i) const { return _days[i]; }

	bool& operator[](size_t i) { return _days[i]; }

	bool operator==(const WeekDaysFilter& other) const { return (_days == other._days); }

private:
	std::array<bool, 7> _days;
};

class TimeFilter {
public:
	/** Default state is the whole day */
	TimeFilter() : from(0), to(24){};
	TimeFilter(Hour from, Hour to) : from(from), to(to){};

	bool operator==(const TimeFilter& other) const { return (from == other.from && to == other.to); }

	Hour from;
	Hour to;
};

/** Container for all the available filters for the rescore */
struct Filters {
	TimeFilter time;
	WeekDaysFilter days;

	bool operator==(const Filters& other) const { return (time == other.time && days == other.days); }
	bool is_default() const {
		if (time == TimeFilter{} && days == WeekDaysFilter{}) return true;

		return false;
	}
};

class RescoreMetadata {
public:
	std::string user_token;
	std::string screenshot_filepath;
	size_t srd_search_ctx_ID;
	std::string time_label;
};

class RelevanceFeedbackQuery {
public:
	bool empty() const { return likes.empty(); }

public:
	LikesCont likes;
};

class TextualQuery {
public:
	bool empty() const { return utils::trim(query).empty(); }

public:
	std::string query;
};

class CanvasQuery {
public:
	bool empty() { return (size() == 0); }

public:
	std::vector<float> lefts;
	std::vector<float> tops;
	std::vector<float> relative_heights;
	std::vector<float> relative_widths;
	std::vector<unsigned int> pixel_heights;
	std::vector<unsigned int> pixel_widths;

	// format from js: [RGBARGBA.....]
	std::vector<std::vector<float>> images;

	// temporal query delimiter
	int break_point = 0;

	int channels = 0;
	std::size_t len = 0;
	inline std::size_t size() { return len; }

	void print() const;

	// Images are expected to be in RGB format
	void RGBA_to_RGB();
	void resize_all(int W = 224, int H = 224);
	void save_all(const std::string& prefix = "");

	/**
	 * This allows portable binary serialization of Collage instances to files.
	 *
	 * by Cereal header-only lib
	 * https://uscilab.github.io/cereal/quickstart.html
	 * https://uscilab.github.io/cereal/stl_support.html
	 */
	template <class Archive>
	void serialize(Archive& archive) {
		archive(lefts, tops, relative_heights, relative_widths, pixel_heights, pixel_widths, images, break_point,
		        channels);
	}

	struct image {
		float left;
		float top;
		float relative_height;
		float relative_width;
		unsigned int pixel_height;
		unsigned int pixel_width;
		std::vector<float>& img;
	};

	image operator[](std::size_t idx) {
		return image{ lefts[idx],        tops[idx],  relative_heights[idx], relative_widths[idx], pixel_heights[idx],
			          pixel_widths[idx], images[idx] };
	}
};

/** The type representing the whole query. */
class Query {
public:
	RescoreMetadata metadata;
	Filters filters;
	RelevanceFeedbackQuery relevance_feeedback;
	TextualQuery textual_query;
	CanvasQuery canvas_query;
};

};  // namespace sh

#endif  // FILTERS_H_
