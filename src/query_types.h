

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

#ifndef QUERY_TYPES_H_
#define QUERY_TYPES_H_

#include <array>
#include <sstream>
#include <variant>

#include <json11.hpp>

#include <cereal/types/variant.hpp>
#include "DatasetFrames.h"
#include "common.h"
#include "utils.h"

namespace sh {

/** Container for information about days filtering */
class WeekDaysFilter {
	std::array<bool, 7> _days;

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
};

/** Container for information about time filtering */
struct TimeFilter {
	Hour from;
	Hour to;

	/** Default state is the whole day */
	TimeFilter() : from(0), to(24){};
	TimeFilter(Hour from, Hour to) : from(from), to(to){};

	bool operator==(const TimeFilter& other) const { return (from == other.from && to == other.to); }
};

/** Container for all the available filters for the rescore */
struct Filters {
	TimeFilter time;
	WeekDaysFilter days;

	bool operator==(const Filters& other) const { return (time == other.time && days == other.days); }
	bool is_default() const { return time == TimeFilter{} && days == WeekDaysFilter{}; }
};

struct RescoreMetadata {
	std::string user_token;
	std::string screenshot_filepath;
	size_t srd_search_ctx_ID;
	std::string time_label;
};

using RelevanceFeedbackQuery = LikesCont;

using TextualQuery = std::string;

using RelocationQuery = ImageId;

struct RelativeRect {
	float left, top, right, bottom;

	float width_norm() const {
		do_assert_debug(right >= right);
		return right - left;
	};

	float height_norm() const {
		do_assert_debug(bottom >= top);
		return bottom - top;
	};

	template <class Archive>
	void serialize(Archive& archive) {
		archive(left, top, right, bottom);
	}
};

class CanvasSubqueryBase {
protected:
	RelativeRect _rect;

public:
	CanvasSubqueryBase() = default;
	CanvasSubqueryBase(const RelativeRect& rect) : _rect{ rect } {};
	const RelativeRect& rect() const { return _rect; };
};

class CanvasSubqueryBitmap : public CanvasSubqueryBase {
	size_t _num_channels;
	size_t _width;
	size_t _height;
	std::vector<uint8_t> _data_int;

	// Unique prefix right here!
	std::string jpeg_filename{ "canvas-query-bitmap_" + std::to_string(utils::irand(0, 1000)) + "_" +
		                       std::to_string(utils::irand(0, 1000)) + "_" + std::to_string(utils::timestamp()) +
		                       ".jpg" };

public:
	CanvasSubqueryBitmap() = default;
	CanvasSubqueryBitmap(const RelativeRect& rect, size_t bitmap_w, size_t bitmap_h, size_t num_channels,
	                     std::vector<std::uint8_t>&& data)
	    : CanvasSubqueryBase{ rect },
	      _num_channels{ num_channels },
	      _width{ bitmap_w },
	      _height{ bitmap_h },
	      _data_int{ std::move(data) } {

	      };

	size_t num_channels() const { return _num_channels; };
	size_t width_pixels() const { return _width; };
	size_t height_pixels() const { return _height; };
	std::vector<uint8_t>& data() { return _data_int; };
	const std::vector<uint8_t>& data() const { return _data_int; };

	std::vector<uint8_t> get_scaled_bitmap(size_t w, size_t h) const;

	json11::Json to_JSON() const {
		json11::Json::object res{ { "rect", json11::Json::array{ _rect.left, _rect.top, _rect.right, _rect.bottom } },
			                      { "bitmap_filename", jpeg_filename },
			                      { "width_pixels", static_cast<int>(_width) },
			                      { "height_pixels", static_cast<int>(_height) } };

		return res;
	}
	template <class Archive>
	void serialize(Archive& archive) {
		archive(_rect, _num_channels, _width, _height, _data_int);
	}

	friend std::ostream& operator<<(std::ofstream& os, CanvasSubqueryBitmap x);
};

inline std::ostream& operator<<(std::ofstream& os, CanvasSubqueryBitmap x) {
	os << "---------------------------------" << std::endl;
	os << "CanvasSubqueryBitmap: " << std::endl;
	os << "---" << std::endl;

	os << "\t"
	   << "._rect = (" << x._rect.left << ", " << x._rect.top << ", " << x._rect.right << ", " << x._rect.bottom << ")"
	   << std::endl;
	os << "\t"
	   << "._height = (" << x._height << std::endl;
	os << "\t"
	   << "._width = (" << x._width << std::endl;

	return os;
}

class CanvasSubqueryText : public CanvasSubqueryBase {
	TextualQuery _text_query;

public:
	CanvasSubqueryText() = default;
	CanvasSubqueryText(const RelativeRect& rect, const TextualQuery query)
	    : CanvasSubqueryBase{ rect }, _text_query{ query } {}

	const TextualQuery& query() const { return _text_query; };

	json11::Json to_JSON() const {
		json11::Json::object res{ { "rect", json11::Json::array{ _rect.left, _rect.top, _rect.right, _rect.bottom } },
			                      { "text_query", _text_query } };

		return res;
	}
	template <class Archive>
	void serialize(Archive& archive) {
		archive(_rect, _text_query);
	}

	friend std::ostream& operator<<(std::ofstream& os, CanvasSubqueryText x);
};

inline std::ostream& operator<<(std::ofstream& os, CanvasSubqueryText x) {
	os << "---------------------------------" << std::endl;
	os << "CanvasSubqueryText: " << std::endl;
	os << "---" << std::endl;

	os << "\t"
	   << "._rect = (" << x._rect.left << ", " << x._rect.top << ", " << x._rect.right << ", " << x._rect.bottom << ")"
	   << std::endl;
	os << "\t"
	   << "._text_query = (" << x._text_query << std::endl;

	return os;
}

using CanvasSubquery = std::variant<CanvasSubqueryBitmap, CanvasSubqueryText>;

/**
 * Type representing query related to the canvas (atm text & bitmap) rectangles.
 */
class CanvasQuery {
	/** Subregion queries */
	std::vector<CanvasSubquery> _subqueries;

public:
	/** Emplace new subregion TEXT query. */
	void emplace_back(const RelativeRect& rect, const std::string& text_query);

	/** Emplace new subregion BITMAP query.
	 *	During this function the input integral RGBA data are converted to RGB float data. */
	void emplace_back(const RelativeRect& rect, size_t bitmap_w, size_t bitmap_h, size_t num_channels,
	                  uint8_t* bitmap_RGBA_data);

	size_t size() const { return _subqueries.size(); }
	bool empty() const { return (size() == 0); }

	/**
	 * This allows portable binary serialization of Collage instances to files.
	 *
	 * by Cereal header-only lib
	 * https://uscilab.github.io/cereal/quickstart.html
	 * https://uscilab.github.io/cereal/stl_support.html
	 */
	template <class Archive>
	void serialize(Archive& archive) {
		archive(_subqueries);
	}

	json11::Json to_JSON() const;

	CanvasSubquery& operator[](size_t idx) { return _subqueries[idx]; }
	const CanvasSubquery& operator[](size_t idx) const { return _subqueries[idx]; }

	bool is_save{ false };

	/** Parses JSON file created by to_JSON */
	static CanvasQuery parse_json(const std::string& filepath);

	/** Parses JSON string created by to_JSON */
	static CanvasQuery parse_json_contents(const std::string& contents, const std::filesystem::path parentPath);
};

struct TemporalQuery {
	TextualQuery textual;
	CanvasQuery canvas;
	RelocationQuery relocation;

	TemporalQuery() : textual{}, canvas{}, relocation{IMAGE_ID_ERR_VAL} {}

	const bool isRelocation() const { return relocation != IMAGE_ID_ERR_VAL; }
	const bool isCanvas() const { return !canvas.empty(); }
	const bool isText() const { return !textual.empty(); }
}

/** The type representing the whole query. */
struct Query {
	Query() = default;
	/* TODO
	Query(CanvasQuery&& cq) : metadata{}, filters{}, relevance_feeedback{}, textual_query{}, canvas_query{ cq } {}
	Query(std::string textual_query)
	    : metadata{}, filters{}, relevance_feeedback{}, textual_query{ textual_query }, canvas_query{} {}
	*/

	RescoreMetadata metadata;
	Filters filters;
	RelevanceFeedbackQuery relevance_feeedback;
	std::vector<TemporalQuery> temporal_queries;

	/* TODO
	void transform_to_no_pos_queries() {
	    std::string t0;
	    std::string t1;
	    TextualQuery new_text;
	    CanvasQuery new_canvas;

	    size_t div{ canvas_query._begins[1] };

	    for (size_t idx{ 0 }; idx < canvas_query._subqueries.size(); ++idx) {
	        auto&& sq{ canvas_query._subqueries[idx] };

	        CanvasSubqueryText& s{ std::get<CanvasSubqueryText>(sq) };
	        const std::string& text{ s.query() };
	        // if first
	        if (idx < div) {
	            t0.append(text).append(" ");
	        }
	        // else second
	        else {
	            t1.append(text).append(" ");
	        }
	    }

	    new_text.query = t0.append(" << ").append(t1);

	    // Swap them
	    canvas_query = new_canvas;
	    textual_query = new_text;
	}*/
};

struct BaseBenchmarkQuery {
	std::vector<ImageId> targets;
};

struct PlainTextBenchmarkQuery : public BaseBenchmarkQuery {
	std::string text_query;
};

struct CanvasBenchmarkQuery : public BaseBenchmarkQuery {
	CanvasQuery canvas_query;
};

using BenchmarkQuery = std::variant<PlainTextBenchmarkQuery>;
};  // namespace sh

#endif  // QUERY_TYPES_H_
