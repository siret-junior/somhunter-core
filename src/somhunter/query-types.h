

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

#include <cereal/types/array.hpp>
#include <cereal/types/set.hpp>
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

	template <class Archive>
	void serialize(Archive& archive)
	{
		archive(_days);
	}
};

/** Container for information about time filtering */
struct TimeFilter {
	Hour from;
	Hour to;

	/** Default state is the whole day */
	TimeFilter() : from(0), to(24){};
	TimeFilter(Hour from, Hour to) : from(from), to(to){};

	bool operator==(const TimeFilter& other) const { return (from == other.from && to == other.to); }

	template <class Archive>
	void serialize(Archive& archive)
	{
		archive(from, to);
	}
};

/** Container for information about time filtering */
struct YearFilter {
	Year from;
	Year to;

	/** Default state is interval [2000, 2021] */
	YearFilter() : from(2000), to(2021){};
	YearFilter(Year from, Year to) : from(from), to(to){};

	bool operator==(const YearFilter& other) const { return (from == other.from && to == other.to); }

	template <class Archive>
	void serialize(Archive& archive)
	{
		archive(from, to);
	}
};

/** Container for all the available filters for the rescore */
struct Filters {
	TimeFilter time;
	YearFilter years;
	WeekDaysFilter days;
	std::vector<bool> dataset_parts{ true, true };

	bool operator==(const Filters& other) const
	{
		return (time == other.time && days == other.days && dataset_parts == other.dataset_parts &&
		        years == other.years);
	}

	bool is_default() const
	{
		return years == YearFilter{} && time == TimeFilter{} && days == WeekDaysFilter{} &&
		       dataset_parts == std::vector<bool>{ true, true };
	}

	/** Based on `dataset_parts` it returns the allowed frame IDs. */
	std::pair<FrameId, FrameId> get_dataset_parts_valid_interval(std::size_t num_total_frames) const
	{
		std::size_t num_parts{ dataset_parts.size() };
		do_assert(num_parts == 2, "Must be 2 intervals.");
		do_assert(num_total_frames >= 2, "At least 2 frames to it.");

		auto base_size{ num_total_frames / num_parts };
		auto rem{ num_total_frames % num_parts };

		return std::pair{ (dataset_parts[0] ? 0 : base_size + rem),
			              (dataset_parts[1] ? num_total_frames : base_size + rem) };
	}

	template <class Archive>
	void serialize(Archive& archive)
	{
		archive(time, days, years);
	}
};

struct RescoreMetadata {
	std::string _username;
	std::string screenshot_filepath;
	size_t srd_search_ctx_ID{ 0 };
	std::string time_label;

	template <class Archive>
	void serialize(Archive& archive)
	{
		// Some of these dont matter in the future
		archive(_username);
	}
};

using RelevanceFeedbackQuery = LikesCont;

using TextualQuery = std::string;

using RelocationQuery = FrameId;

struct RelativeRect {
	/** Distances from the given edge (e.g. top 0.5 means that it starts in the middle vertically) */
	float left, top, right, bottom;

	/** Set rectangle to cover everything. */
	void to_full()
	{
		left = 0;
		top = 0;
		right = 0;
		left = 0;
	}

	float width_norm() const
	{
		do_assert_debug(right >= right, "");
		return right - left;
	};

	float height_norm() const
	{
		do_assert_debug(bottom >= top, "");
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
	// ---
	/** Set canvas positioned query as if they were across the whole canvas. */
	void unposition() { _rect.to_full(); }
	// ---
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
	                     std::vector<std::uint8_t>&& _data)
	    : CanvasSubqueryBase{ rect },
	      _num_channels{ num_channels },
	      _width{ bitmap_w },
	      _height{ bitmap_h },
	      _data_int{ std::move(_data) } {

	      };

	size_t num_channels() const { return _num_channels; };
	size_t width_pixels() const { return _width; };
	size_t height_pixels() const { return _height; };
	std::vector<uint8_t>& data() { return _data_int; };
	const std::vector<uint8_t>& data() const { return _data_int; };
	bool empty() const { return _data_int.empty(); }

	std::vector<uint8_t> get_scaled_bitmap(size_t w, size_t h) const;

	bool operator!=(const CanvasSubqueryBitmap& b) const { return !((*this) == b); }

	bool operator==(const CanvasSubqueryBitmap& b) const
	{
		return _num_channels == b._num_channels && _width == b._width && _height == b._height &&
		       _data_int == b._data_int && _rect == b._rect;
	}

	nlohmann::json to_JSON() const
	{
		nlohmann::json res = { { "rect", nlohmann::json::array({ _rect.left, _rect.top, _rect.right, _rect.bottom }) },
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
	    : CanvasSubqueryBase{ rect }, _text_query{ utils::trim(query) }
	{
	}
	// ---
	const TextualQuery& query() const { return _text_query; };
	bool empty() const { return _text_query.empty(); }

	nlohmann::json to_JSON() const
	{
		nlohmann::json res = { { "rect", nlohmann::json::array({ _rect.left, _rect.top, _rect.right, _rect.bottom }) },
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
	bool empty() const
	{
		if (size() == 0)
			return true;
		else {
			for (auto&& q : _subqueries) {
				auto r = std::visit(overloaded{ [](auto q) { return q.empty(); } }, q);
				if (r) return true;
			}
		}
		return false;
	}
	const std::vector<CanvasSubquery>& subqueries() const { return _subqueries; };
	std::vector<CanvasSubquery>& subqueries() { return _subqueries; };

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

	nlohmann::json to_JSON() const;

	CanvasSubquery& operator[](size_t idx) { return _subqueries[idx]; }
	const CanvasSubquery& operator[](size_t idx) const { return _subqueries[idx]; }

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
	bool score_secondary() const { return _score_secondary; }
	void score_secondary(bool new_value) { _score_secondary = new_value; }
	const bool is_relocation() const { return relocation != ERR_VAL<FrameId>(); }
	const bool is_canvas() const { return !canvas.empty(); }
	const bool is_text() const { return !is_relocation() && !textual.empty(); }
	const bool empty() const
	{
		return (relocation == ERR_VAL<RelocationQuery>()) && (utils::trim(textual).empty()) && canvas.empty();
	}
	const bool is_bitmap_canvas() const
	{
		if (!is_canvas()) return false;

		const auto& sqs{ canvas.subqueries() };
		return std::holds_alternative<CanvasSubqueryBitmap>(sqs.front());
	}
	const bool is_text_canvas() const
	{
		if (!is_canvas()) return false;

		const auto& sqs{ canvas.subqueries() };
		return std::holds_alternative<CanvasSubqueryText>(sqs.front());
	}

	template <class Archive>
	void serialize(Archive& archive)
	{
		archive(textual, canvas, relocation);
	}
	// ---
	// bool operator!=(const TemporalQuery& b) const { return !((*this) == b); }
	bool operator==(const TemporalQuery& b) const
	{
		return textual == b.textual && canvas == b.canvas && relocation == b.relocation &&
		       score_secondary() == b.score_secondary();
	}

	nlohmann::json to_JSON() const
	{
		nlohmann::json o = nlohmann::json::object();

		o["textual"] = textual;
		o["canvas"] = canvas.to_JSON();

		if (relocation != ERR_VAL<RelocationQuery>()) {
			o["relocation"] = relocation;
		} else {
			o["relocation"] = nullptr;
		}

		return o;
	}

	// *** MEMBER VARIABLES ***
public:
	TextualQuery textual;
	CanvasQuery canvas;
	RelocationQuery relocation;
	bool _score_secondary{ false };
};

/** The type representing the whole query. */
struct Query {
	// *** METHODS ***
public:
	Query() = default;
	template <typename QueryType>
	Query(const std::vector<QueryType>& temp_queries) : metadata{}, filters{}, relevance_feeedback{}
	{
		for (auto&& q : temp_queries) {
			auto& item{ temporal_queries.emplace_back(q) };
			// Make sure correct scoring is set
			item.score_secondary(_score_secondary);
		}
	}

	// ---

	const bool is_relocation() const
	{
		for (auto&& t : temporal_queries) {
			if (t.is_relocation()) return true;
		}

		return false;
	}
	const bool score_secondary() const { return _score_secondary; }
	void score_secondary(bool new_value)
	{
		_score_secondary = new_value;
		for (auto&& q : temporal_queries) {
			q.score_secondary(new_value);
		}
	}
	const bool is_canvas() const { return temporal_queries.front().is_canvas(); }
	const bool is_bitmap_canvas() const { return temporal_queries.front().is_bitmap_canvas(); }
	const bool is_text_canvas() const { return temporal_queries.front().is_text_canvas(); }
	const bool is_temporal_text() const { return !is_relocation() && is_text() && temporal_queries.size() > 1; }
	const bool is_text() const { return temporal_queries.front().is_text(); }
	const bool empty() const
	{
		for (auto&& q : temporal_queries) {
			if (q.empty()) return true;
		}
		return false;
	}

	const std::vector<TemporalQuery>& queries() const { return temporal_queries; }

	void set_targets(const std::vector<VideoFrame>& ts)
	{
		targets.clear();
		for (auto&& t : ts) {
			targets.emplace_back(t.frame_ID);
		}
	}

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
		if (is_temporal_text()) {
			std::cout << ">>> DECAY TEMPORAL TEXT: " << std::endl;
			temporal_queries.erase(temporal_queries.begin() + 1);
			do_assert(temporal_queries.size() == 1, "Must have 1 item.");
		} else if (is_text_canvas()) {
			std::cout << ">>> DECAY TEXT CANVAS: " << std::endl;
			for (auto&& tq : temporal_queries) {
				TextualQuery new_query;

				const auto& sqs{ tq.canvas.subqueries() };
				std::string text;
				for (size_t idx{ 0 }; idx < sqs.size(); ++idx) {
					const auto& sq{ sqs[idx] };

					const CanvasSubqueryText& s{ std::get<CanvasSubqueryText>(sq) };
					text.append(s.query()).append(" ");
				}
				tq.textual = text;
				tq.canvas = CanvasQuery{};  //< Empty canvas query
			}
		} else if (is_bitmap_canvas()) {
			std::cout << ">>> DECAY BITMAP CANVAS: " << std::endl;
			for (auto&& tq : temporal_queries) {
				auto& sqs{ tq.canvas.subqueries() };
				for (size_t idx{ 0 }; idx < sqs.size(); ++idx) {
					auto& sq{ sqs[idx] };

					CanvasSubqueryBitmap& s{ std::get<CanvasSubqueryBitmap>(sq) };
					s.unposition();
				}
			}
		} else if (is_relocation()) {
			std::cout << ">>> DECAY RELOCATION: " << std::endl;
			for (auto&& tq : temporal_queries) {
				tq.relocation = ERR_VAL<FrameId>();
			}
		}
	}

	// --- Helper methods ---
	nlohmann::json to_JSON() const;

	template <class Archive>
	void serialize(Archive& archive)
	{
		archive(metadata, filters, relevance_feeedback, temporal_queries, targets, is_save);
	}

	// --- Operators ---
	// bool operator==(const TemporalQuery& other) const { return (*this) == other; }

	// *** MEMBER VARIABLES ***
public:
	RescoreMetadata metadata;
	Filters filters;
	RelevanceFeedbackQuery relevance_feeedback;
	std::vector<TemporalQuery> temporal_queries;
	std::vector<FrameId> targets;

	bool is_save{ false };
	bool _score_secondary{ false };
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
