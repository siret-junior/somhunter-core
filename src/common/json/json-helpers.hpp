
#ifndef JSON_HELPERS_H_
#define JSON_HELPERS_H_

#include <nlohmann/json.hpp>

#include "common.h"

using namespace nlohmann;

namespace sh {
inline void require_value(const json& json, std::string msg = "Missing JSON value") {
	if (json.is_null()) {
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}
}

inline void require_key(const json& json, const std::string& key) {
	std::string msg{ "Missing JSON key: " + key };
	if (!json.contains(key)) {
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	require_value(json[key], msg);
}

inline std::string require_string_value(const json& json, const std::string& key) {
	require_key(json, key);

	return json[key].get<std::string>();
}

/**
 * \deprecated
 */
inline std::string optional_string_value(const json& json, const std::string& key) {
	return (json[key].is_null() ? "" : json[key].get<std::string>());
}

/** Parses the (potentialy null) value from the given key in JSON structure. */
template <typename T_>
inline T_ optional_value_or(const json& j, const std::string& key, const T_& or_val) {
	if (j[key].is_null()) {
		return or_val;
	} else {
		return j[key].get<T_>();
	}
}

/**
 * Parses the (potentialy null or empty) string from the given key in JSON structure.
 *
 * Partial specialization of \ref optional_value_or.
 */
template <>
inline std::string optional_value_or(const json& j, const std::string& key, const std::string& or_val) {
	if (j[key].is_null() || j[key].get<std::string>().empty()) {
		return or_val;
	} else {
		return j[key].get<std::string>();
	}
}

/** Parses the (potentialy null) value from the given key in JSON structure. */
template <typename T_>
inline std::optional<T_> optional_value(const json& j, const std::string& key) {
	if (j[key].is_null()) {
		return std::nullopt;
	} else {
		return std::optional<T_>{ j[key].get<T_>() };
	}
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
	} else {
		return std::optional<std::string>{ j[key].get<std::string>() };
	}
}

template <typename T_>
inline T_ require_int_value(const json& json, const std::string& key) {
	require_key(json, key);

	return static_cast<T_>(json[key].get<T_>());
}

template <typename T_>
inline T_ require_float_value(const json& json, const std::string& key) {
	require_key(json, key);

	return static_cast<T_>(json[key].get<T_>());
}

template <typename T_>
inline T_ require_float_value(const json& json) {
	require_value(json);

	return static_cast<T_>(json.get<T_>());
}

inline bool require_bool_value(const json& json, const std::string& key) {
	require_key(json, key);

	return json[key].get<bool>();
}

}  // namespace sh

#endif  // JSON_HELPERS_H_
