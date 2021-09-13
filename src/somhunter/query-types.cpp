
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

#include "query-types.h"

#include <filesystem>

#include <json11.hpp>

#include "image-processor.h"
#include "json-helpers.hpp"

using namespace sh;

namespace fs = std::filesystem;

nlohmann::json Query::to_JSON() const
{
	nlohmann::json o = nlohmann::json::object();

	// o["metadata"] = metadata.to_JSON();
	// o["filters"] = filters.to_JSON();

	o["is_relocation"] = (is_relocation() ? true : false);
	o["is_canvas"] = (is_canvas() ? true : false);
	o["is_text"] = (is_text() ? true : false);
	o["is_bitmap_canvas"] = (is_bitmap_canvas() ? true : false);
	o["is_text_canvas"] = (is_text_canvas() ? true : false);

	// Likes
	nlohmann::json rf_json = nlohmann::json::array();
	for (auto&& like_frame_ID : relevance_feeedback) {
		rf_json.emplace_back(like_frame_ID);
	}
	o["relevance_feeedback"] = rf_json;

	// Queries
	nlohmann::json qs_json = nlohmann::json::array();
	for (auto&& tq : temporal_queries) {
		qs_json.emplace_back(tq.to_JSON());
	}
	o["queries"] = qs_json;

	return o;
}

std::vector<std::uint8_t> CanvasSubqueryBitmap::get_scaled_bitmap(size_t w, size_t h) const
{
	return ImageManipulator::resize(_data_int, width_pixels(), height_pixels(), w, h, _num_channels);
}

nlohmann::json CanvasQuery::to_JSON() const
{
	nlohmann::json arr_temp = nlohmann::json::array();
	for (size_t i{ 0 }; i < _subqueries.size(); ++i) {
		arr_temp.emplace_back(std::visit(
		    overloaded{
		        [](auto sq) { return sq.to_JSON(); },
		    },
		    _subqueries[i]));
	}
	return arr_temp;
}

std::vector<CanvasQuery> CanvasQuery::parse_json_contents(const std::string& /*contents*/,
                                                          const fs::path /*parentPath*/)
{
	// \todo Convert to nlohmann::json
	throw std::runtime_error("...");
	//
	// auto json_all{ json::parse(contents) };

	// std::vector<CanvasQuery> qs;

	// json cqJson{ json_all["canvas_query"] };

	// size_t ti = 0;
	// for (auto&& tempQuery : cqJson) {
	//	qs.push_back(CanvasQuery());

	//	for (auto&& obj : tempQuery) {
	//		require_value(obj, "rect");
	//		auto&& positions = obj["rect"].array_items();
	//		RelativeRect rect{ require_float_value<float>(positions[0]), require_float_value<float>(positions[1]),
	//			               require_float_value<float>(positions[2]), require_float_value<float>(positions[3]) };

	//		if (!obj["text_query"].is_null()) {  // Text query on canvas
	//			qs[ti].emplace_back(rect, require_string_value(obj, "text_query"));
	//		} else {  // Image on canvas
	//			fs::path p(parentPath / require_string_value(obj, "bitmap_filename"));
	//			auto image = ImageManipulator::load_image<BitmapImage<uint8_t>>(p.string());
	//			qs[ti].emplace_back(rect, image.w, image.h, image.num_channels, image.data.data());
	//		}
	//	}

	//	++ti;
	//}

	// return qs;
}

std::vector<CanvasQuery> CanvasQuery::parse_json(const std::string& /*filepath*/)
{
	// \todo Convert to nlohmann::json
	throw std::runtime_error("...");
	/*std::string file_contents(utils::read_whole_file(filepath));
	fs::path p(filepath);
	return parse_json_contents(file_contents, p.parent_path());*/
}

void CanvasQuery::emplace_back(const RelativeRect& rect, const std::string& text_query)
{
	_subqueries.emplace_back(CanvasSubqueryText{ rect, text_query });
}

void CanvasQuery::emplace_back(const RelativeRect& rect, size_t bitmap_w, size_t bitmap_h, size_t num_channels,
                               uint8_t* bitmap_RGBA_data)
{
	std::vector<std::uint8_t> image;

	// DO: RGBA_to_RGB
	size_t total_size{ bitmap_w * bitmap_h * num_channels };
	for (size_t j = 0; j < total_size; j += num_channels) {
		image.push_back(bitmap_RGBA_data[j + 0]);
		image.push_back(bitmap_RGBA_data[j + 1]);
		image.push_back(bitmap_RGBA_data[j + 2]);
	}

	_subqueries.emplace_back(CanvasSubqueryBitmap{ rect, bitmap_w, bitmap_h, 3, std::move(image) });
}
