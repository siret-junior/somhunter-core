
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
#ifndef UTILS_JSON_H_
#define UTILS_JSON_H_

#include "json11.hpp"

#include "log.h"

namespace sh {

inline void require_value(const json11::Json& json, std::string msg = "Missing JSON value") {
	if (json.is_null()) {
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}
}

inline void require_key(const json11::Json& json, const std::string& key) {
	std::string msg{ "Missing JSON key: " + key };
	require_value(json[key], msg);
}

inline std::string require_string_value(const json11::Json& json, const std::string& key) {
	require_key(json, key);

	return json[key].string_value();
}

template <typename T_>
inline T_ require_int_value(const json11::Json& json, const std::string& key) {
	require_key(json, key);

	return static_cast<T_>(json[key].int_value());
}

template <typename T_>
inline T_ require_float_value(const json11::Json& json, const std::string& key) {
	require_key(json, key);

	return static_cast<T_>(json[key].number_value());
}

template <typename T_>
inline T_ require_float_value(const json11::Json& json) {
	require_value(json);

	return static_cast<T_>(json.number_value());
}

inline bool require_bool_value(const json11::Json& json, const std::string& key) {
	require_key(json, key);

	return json[key].bool_value();
}

}  // namespace sh

#endif  // UTILS_JSON_H_
