
#ifndef JSON_HELPERS_H_
#define JSON_HELPERS_H_

#include <nlohmann/json.hpp>

#include "common.h"

using namespace nlohmann;

namespace sh
{
inline void require_value(const json& json, std::string msg = "Missing JSON value")
{
	if (json.is_null()) {
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}
}

inline void require_key(const json& json, const std::string& key)
{
	std::string msg{ "Missing JSON key: " + key };
	if (!json.contains(key)) {
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	require_value(json[key], msg);
}

inline std::string require_string_value(const json& json, const std::string& key)
{
	require_key(json, key);

	return json[key].get<std::string>();
}

inline std::string optional_string_value(const json& json, const std::string& key)
{
	return (json[key].is_null() ? "" : json[key].get<std::string>());
}

template <typename T_>
inline T_ require_int_value(const json& json, const std::string& key)
{
	require_key(json, key);

	return static_cast<T_>(json[key].get<T_>());
}

template <typename T_>
inline T_ require_float_value(const json& json, const std::string& key)
{
	require_key(json, key);

	return static_cast<T_>(json[key].number_value());
}

template <typename T_>
inline T_ require_float_value(const json& json)
{
	require_value(json);

	return static_cast<T_>(json.number_value());
}

inline bool require_bool_value(const json& json, const std::string& key)
{
	require_key(json, key);

	return json[key].get<bool>();
}

}  // namespace sh

#endif  // JSON_HELPERS_H_
