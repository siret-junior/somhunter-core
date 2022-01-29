/* This file is part of SOMHunter.
 *
 * Copyright (C) 2021 Frantisek Mejzlik <frankmejzlik@protonmail.com>
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

/**
 * \file json-helpers.hpp
 *
 * Implements convenience helper functions for JSON parsing.
 */

#ifndef JSON_HELPERS_H_
#define JSON_HELPERS_H_

#include <sstream>
// ---
#include <nlohmann/json.hpp>
// ---
#include "common.h"

using namespace nlohmann;

namespace sh {

/**
 * Makes sure that the given `json` is defined (not null and not undefined).
 */
inline void require_value(const json& json, const std::string& msg = "Missing JSON value") {
	if (json.is_null()) {
		SHLOG_E_THROW(msg);
	}
}

/**
 * Makes sure that the given `json` has the given key and the key is not null.
 */
inline void require_key(const json& json, const std::string& key) {
	std::string msg{ "Missing JSON key: " + key };
	if (!json.contains(key)) {
		SHLOG_E_THROW(msg);
	}
	require_value(json[key], msg);
}

/**
 * Parses the (potentialy null) value from the given key in JSON structure
 * and if not defined or null uses the `or_val` value.
 */
template <typename T_>
inline T_ optional_value_or(const json& j, const std::string& key, const T_& or_val) {
	if (j[key].is_null()) {
		return or_val;
	} else {
		return j[key].get<T_>();
	}
}

/**
 * Parses the (potentialy null) value from the given key in JSON structure
 * and if not defined or null uses the `or_val` value.
 *
 * Partial specialization of \ref optional_value_or.
 */
template <>
inline std::string optional_value_or(const json& j, const std::string& key, const std::string& or_val) {
	if (j[key].is_null() || j[key].get<std::string>().empty()) {
		return or_val;
	}
	return j[key].get<std::string>();
}

/**
 * Parses the (potentialy null) value from the given key in JSON structure.
 */
template <typename T_>
inline std::optional<T_> optional_value(const json& j, const std::string& key) {
	if (j[key].is_null()) {
		return std::nullopt;
	}
	return std::optional<T_>{ j[key].get<T_>() };
}

/**
 * Parses the (potentialy null or empty) string from the given key in JSON structure.
 *
 * Partial specialization of \ref optional_value.
 */
template <>
inline std::optional<std::string> optional_value(const json& j, const std::string& key) {
	if (j[key].is_null() || j[key].get<std::string>().empty()) {
		return std::nullopt;
	}
	return std::optional<std::string>{ j[key].get<std::string>() };
}

/**
 * Parses the REQUIRED value of type `T_` from the gien key in JSON.
 */
template <typename T_>
inline T_ require_value(const json& json, const std::string& key) {
	require_key(json, key);

	return static_cast<T_>(json[key].get<T_>());
}

inline json wrap_and_parse(std::stringstream& in_stream) {
	std::string s{ in_stream.str() };

	if (s[0] == ',') {
		s[0] = ' ';
	}

	return json::parse("[" + s + "]");
}

}  // namespace sh

#endif  // JSON_HELPERS_H_
