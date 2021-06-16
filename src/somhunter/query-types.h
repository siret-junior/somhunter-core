

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

#include <cereal/types/variant.hpp>
#include <json11.hpp>

#include "common.h"

#include "dataset-frames.h"
#include "utils.hpp"

namespace sh
{
/** Container for information about days filtering */
class WeekDaysFilter
{
	std::array<bool, 7> _days;

public:
	/** Default state is all dayes */
	WeekDaysFilter() { _days.fill(true); }

	/** Construct from the bit mask */
	WeekDaysFilter(uint8_t mask)
	{
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
	std::string _username;
	std::string screenshot_filepath;
	size_t srd_search_ctx_ID;
	std::string time_label;
};

using RelevanceFeedbackQuery = LikesCont;

using TextualQuery = std::string;

using RelocationQuery = FrameId;

struct RelativeRect {
	float left, top, right, bottom;

	float width_norm() const
	{
		do_assert_debug(right >= right);
		return right - left;
	};

	float height_norm() const
	{
		do_assert_debug(bottom >= top);
		return bottom - top;
	};

	template <class Archive>
	void serialize(Archive& archive)
	{
		archive(left, top, right, bottom);
	}

	bool operator==(const RelativeRect& b) const
	{
		return left == b.left && top == b.top && right == b.right && bottom == b.bottom;
	}
};

class CanvasSubqueryBase
{
protected:
	RelativeRect _rect;

public:
	CanvasSubqueryBase() = default;
	CanvasSubqueryBase(const RelativeRect& rect) : _rect{ rect } {};
	const RelativeRect& rect() const { return _rect; };
};

class CanvasSubqueryBitmap : public CanvasSubqueryBase
{
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

	bool operator!=(const CanvasSubqueryBitmap& b) const { return !((*this) == b); }

	bool operator==(const CanvasSubqueryBitmap& b) const
	{
		return _num_channels == b._num_channels && _width == b._width && _height == b._height &&
		       _data_int == b._data_int && _rect == b._rect;
	}

	json11::Json to_JSON() const
	{
		json11::Json::object res{ { "rect", json11::Json::array{ _rect.left, _rect.top, _rect.right, _rect.bottom } },
			                      { "bitmap_filename", jpeg_filename },
			                      { "width_pixels", static_cast<int>(_width) },
			                      { "height_pixels", static_cast<int>(_height) } };

		return res;
	}

	template <class Archive>
	void serialize(Archive& archive)
	{
		archive(_rect, _num_channels, _width, _height, _data_int);
	}

	friend std::ostream& operator<<(std::ofstream& os, CanvasSubqueryBitmap x);
};

inline std::ostream& operator<<(std::ofstream& os, CanvasSubqueryBitmap x)
{
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

class CanvasSubqueryText : public CanvasSubqueryBase
{
	TextualQuery _text_query;

public:
	CanvasSubqueryText() = default;
	CanvasSubqueryText(const RelativeRect& rect, const TextualQuery query)
	    : CanvasSubqueryBase{ rect }, _text_query{ query }
	{
	}

	const TextualQuery& query() const { return _text_query; };

	json11::Json to_JSON() const
	{
		json11::Json::object res{ { "rect", json11::Json::array{ _rect.left, _rect.top, _rect.right, _rect.bottom } },
			                      { "text_query", _text_query } };

		return res;
	}
	template <class Archive>
	void serialize(Archive& archive)
	{
		archive(_rect, _text_query);
	}

	bool operator!=(const CanvasSubqueryText& b) const { return !((*this) == b); }

	bool operator==(const CanvasSubqueryText& b) const { return _text_query == b._text_query && _rect == b._rect; }

	friend std::ostream& operator<<(std::ofstream& os, CanvasSubqueryText x);
};

inline std::ostream& operator<<(std::ofstream& os, CanvasSubqueryText x)
{
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
class CanvasQuery
{
	// *** METHODS ***
public:
	/** Emplace new subregion TEXT query. */
	void emplace_back(const RelativeRect& rect, const std::string& text_query);

	/** Emplace new subregion BITMAP query.
	 *	During this function the input integral RGBA data are converted to RGB float data. */
	void emplace_back(const RelativeRect& rect, size_t bitmap_w, size_t bitmap_h, size_t num_channels,
	                  uint8_t* bitmap_RGBA_data);

	size_t size() const { return _subqueries.size(); }
	bool empty() const { return (size() == 0); }
	const std::vector<CanvasSubquery>& subqueries() const { return _subqueries; };

	/**
	 * This allows portable binary serialization of Collage instances to files.
	 *
	 * by Cereal header-only lib
	 * https://uscilab.github.io/cereal/quickstart.html
	 * https://uscilab.github.io/cereal/stl_support.html
	 */
	template <class Archive>
	void serialize(Archive& archive)
	{
		archive(_subqueries);
	}

	json11::Json to_JSON() const;

	CanvasSubquery& operator[](size_t idx) { return _subqueries[idx]; }
	const CanvasSubquery& operator[](size_t idx) const { return _subqueries[idx]; }

	bool is_save{ false };

	/** Parses JSON file created by to_JSON */
	static std::vector<CanvasQuery> parse_json(const std::string& filepath);

	/** Parses JSON string created by to_JSON */
	static std::vector<CanvasQuery> parse_json_contents(const std::string& contents,
	                                                    const std::filesystem::path parentPath);

	bool operator==(const CanvasQuery& b) const
	{
		if (b.size() != size()) return false;

		for (size_t i = 0; i < size(); ++i) {
			auto&& s1 = _subqueries[i];
			auto&& s2 = b._subqueries[i];

			if (std::holds_alternative<CanvasSubqueryText>(s1) && std::holds_alternative<CanvasSubqueryText>(s2)) {
				auto&& s1t = std::get<CanvasSubqueryText>(s1);
				auto&& s2t = std::get<CanvasSubqueryText>(s2);
				if (s1t != s2t) return false;
			} else if (std::holds_alternative<CanvasSubqueryBitmap>(s1) &&
			           std::holds_alternative<CanvasSubqueryBitmap>(s2)) {
				auto&& s1b = std::get<CanvasSubqueryBitmap>(s1);
				auto&& s2b = std::get<CanvasSubqueryBitmap>(s2);
				if (s1b != s2b) return false;
			} else {
				return false;
			}
		}

		return true;
	}
	// *** MEMBER VARIABLES ***
private:
	/** Subregion queries */
	std::vector<CanvasSubquery> _subqueries;
};

struct TemporalQuery {
	// *** METHODS ***
public:
	TemporalQuery() : textual{}, canvas{}, relocation{ ERR_VAL<FrameId>() } {}
	TemporalQuery(TextualQuery tq) : textual{ tq }, canvas{}, relocation{ ERR_VAL<FrameId>() } {}
	TemporalQuery(CanvasQuery cq) : textual{}, canvas{ cq }, relocation{ ERR_VAL<FrameId>() } {}
	TemporalQuery(RelocationQuery rq) : textual{}, canvas{}, relocation{ rq } {}
	TemporalQuery(TextualQuery tq, CanvasQuery cq, RelocationQuery rq) : textual{ tq }, canvas{ cq }, relocation{ rq }
	{
	}
	// ---
	const bool is_relocation() const { return relocation != ERR_VAL<FrameId>(); }
	const bool is_canvas() const { return !canvas.empty(); }
	const bool is_text() const { return !textual.empty(); }
	const bool empty() const { return !is_relocation() && !is_canvas() && !is_text(); }

	template <class Archive>
	void serialize(Archive& archive)
	{
		archive(textual, canvas, relocation);
	}
	// ---
	bool operator!=(const TemporalQuery& b) const { return !((*this) == b); }
	bool operator==(const TemporalQuery& b) const
	{
		return textual == b.textual && canvas == b.canvas && relocation == b.relocation;
	}

	// *** MEMBER VARIABLES ***
public:
	TextualQuery textual;
	CanvasQuery canvas;
	RelocationQuery relocation;
};

/** The type representing the whole query. */
struct Query {
	// *** METHODS ***
public:
	Query() = default;
	template <typename Q>
	Query(const std::vector<Q>& temp_queries) : metadata{}, filters{}, relevance_feeedback{}
	{
		for (auto&& q : temp_queries) {
			temporal_queries.emplace_back(q);
		}
	}
	// ---
	std::string get_plain_text_query() const
	{
		std::string text2;

		for (auto&& tq : temporal_queries) {
			TextualQuery new_query;

			const auto& sqs{ tq.canvas.subqueries() };
			std::string text{ tq.textual };
			for (size_t idx{ 0 }; idx < sqs.size(); ++idx) {
				const auto& sq{ sqs[idx] };

				if (!std::holds_alternative<CanvasSubqueryText>(sq)) {
					continue;
				}

				const CanvasSubqueryText& s{ std::get<CanvasSubqueryText>(sq) };
				text.append(" [").append(s.query()).append("] ");
			}
			if (!text.empty()) {
				text2.append(text).append(" >> ");
			}
		}

		if (text2.length() > 0) {
			text2.pop_back();
			text2.pop_back();
			text2.pop_back();
			text2.pop_back();
		}

		return text2;
	}

	void transform_to_no_pos_queries()
	{
		for (auto&& tq : temporal_queries) {
			TextualQuery new_query;

			const auto& sqs{ tq.canvas.subqueries() };
			std::string text;
			for (size_t idx{ 0 }; idx < sqs.size(); ++idx) {
				const auto& sq{ sqs[idx] };

				do_assert(std::holds_alternative<CanvasSubqueryText>(sq), "Text canvases only!");

				const CanvasSubqueryText& s{ std::get<CanvasSubqueryText>(sq) };
				text.append(s.query()).append(" ");
			}
			tq.textual = text;
			tq.canvas = CanvasQuery{};  //< Empty canvas query
		}
	}

	// *** MEMBER VARIABLES ***
public:
	RescoreMetadata metadata;
	Filters filters;
	RelevanceFeedbackQuery relevance_feeedback;
	std::vector<TemporalQuery> temporal_queries;
};

struct BaseBenchmarkQuery {
	std::vector<FrameId> targets;
};

struct PlainTextBenchmarkQuery : public BaseBenchmarkQuery {
	std::vector<TextualQuery> text_query;
};

struct CanvasBenchmarkQuery : public BaseBenchmarkQuery {
	std::vector<CanvasQuery> canvas_query;
};

using BenchmarkQuery = std::variant<PlainTextBenchmarkQuery>;
};  // namespace sh

#endif  // QUERY_TYPES_H_
