
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

#ifndef TEST_UTILS_HPP_
#define TEST_UTILS_HPP_

// ---
#include <nlohmann/json.hpp>
// ---

using json = nlohmann::json;

namespace sh
{
namespace tests
{
inline void assert_contains_key_with_value(const json& data, const std::string& key, const json& value)
{
	if (!data.is_object()) {
		do_assert(false, "Must be an object.");
	}
	for (auto& [k, v] : data.items()) {
		if (k == key) {
			if (v == value) {
				return;
			}
		}
	}
	do_assert(false, "Key-value " + key + " -> " + value.dump(4) + "not found.");
}

inline void assert_column_contains(const std::string& line, std::size_t idx, const std::string& val)
{
	std::stringstream ss{ line };
	std::string tok;
	for (size_t i = 0; i <= idx; i++) {
		ss >> tok;
	}
	if (tok != val) {
		do_assert(false, "Column " + std::to_string(idx) + " contains '" + tok + "'.");
	}
}

};  // namespace tests
};  // namespace sh

#endif