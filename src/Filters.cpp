
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

#include <filesystem>

#include "json11.hpp"

#include "Filters.h"
#include "ImageManipulator.h"


using namespace sh;

namespace fs = std::filesystem;

std::vector<std::uint8_t> CanvasSubqueryBitmap::get_scaled_bitmap(size_t w, size_t h) const {
	return ImageManipulator::resize(_data_int, width_pixels(), height_pixels(), w, h, _num_channels);
}

CanvasQuery CanvasQuery::parse_json_contents(
	const std::string& contents, 
	const fs::path parentPath) 
	{
	std::string err;
	auto json_all{ json11::Json::parse(contents, err) };

	if (!err.empty()) {
		std::string msg{ "Error parsing JSON CanvasQuery string." };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	CanvasQuery q;

	json11::Json cqJson { json_all["canvas_query"] };

	for (auto&& tempQuery : cqJson.array_items()) {
		q.push_query_begin();

		for (auto&& obj : tempQuery.array_items()) {
			require_value(obj, "rect");
			auto&& positions = obj["rect"].array_items();
			RelativeRect rect { 
				require_float_value<float>(positions[0]),
				require_float_value<float>(positions[1]),
				require_float_value<float>(positions[2]),
				require_float_value<float>(positions[3])
			 };


			if (!obj["text_query"].is_null()) { // Text query on canvas
				q.emplace_back(rect, require_string_value(obj, "text_query"));
			} else { // Image on canvas
				fs::path p(parentPath / require_string_value(obj, "bitmap_filename"));
				auto image = ImageManipulator::load_image<BitmapImage<uint8_t>>(p);
				q.emplace_back(rect, image.w, image.h, image.num_channels, image.data.data());
			}
		}
	}
	q.push_query_begin();

	return q;
}

CanvasQuery CanvasQuery::parse_json(const std::string& filepath) {
	std::string file_contents(utils::read_whole_file(filepath));
	fs::path p(filepath);
	return parse_json_contents(file_contents, p.parent_path());
}
