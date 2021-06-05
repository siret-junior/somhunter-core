
#ifndef JSON_HELPERS_H_
#define JSON_HELPERS_H_

#include "json11.hpp"

#include "common.h"

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

#endif  // JSON_HELPERS_H_
