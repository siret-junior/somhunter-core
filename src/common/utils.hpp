
/* This file is part of SOMHunter.
 *
 * Copyright (C) 2021 František Mejzlík <frankmejzlik@gmail.com>
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

#ifndef UTILS_H_
#define UTILS_H_

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <sha256.h>
#include <cereal/archives/binary.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/types/vector.hpp>

#include "common.h"

namespace sh {
namespace utils {

/**
 * Serializes the given data into the file using cereal lib.
 *
 * https://uscilab.github.io/cereal/
 */
template <typename DataType_>
void serialize_to_file(DataType_ data, const std::string filepath) {
	std::ofstream ofs(filepath, std::ios::out | std::ios::binary);
	if (!ofs) {
		SHLOG_E_THROW("Error openning file: " << filepath);
	}

	cereal::BinaryOutputArchive out_archive(ofs);
	out_archive(data);
}

/**
 * Deserializes the data from the provided file using cereal lib.
 *
 * https://uscilab.github.io/cereal/
 */
template <typename DataType_>
DataType_ deserialize_from_file(const std::string filepath) {
	std::ifstream ifs(filepath, std::ios::in | std::ios::binary);
	if (!ifs) {
		SHLOG_E_THROW("Error openning file: " << filepath);
	}

	cereal::BinaryInputArchive in_archive(ifs);

	DataType_ data;
	in_archive(data);

	return data;
}

/**
 * Returns the actual UNIX timestamp (ms).
 */
inline int64_t timestamp() {
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

/**
 * Returns string representing current time and date in formated
 * string based on provided format.
 *
 * \remarks
 *  Use format string as for put_time method
 *  (https://en.cppreference.com/w/cpp/io/manip/put_time)
 *
 *  "%d-%m-%Y_%H-%M-%S" => e.g. "16-11-2019_13-26-45:
 *
 *  @param fmt  Format string using the same rules as put_time method.
 *  @return   String representing current date and time in desired format.
 */
inline std::string get_formated_timestamp(const std::string& fmt, UnixTimestamp ts = timestamp()) {
	auto x = std::chrono::duration<std::size_t, std::milli>(ts);
	std::chrono::time_point<std::chrono::system_clock> tp{ x };
	auto tts{ std::chrono::system_clock::to_time_t(tp) };

	std::stringstream ss;
	ss << std::put_time(std::localtime(&tts), fmt.data());
	return ss.str();
}

/**
 * Tries to parse the integer from the string.
 *
 * \throws std::runtime_error	If unable to parse from the string.
 *
 * \param	str		String to be parsed as an int.
 * \returns		Parsed integer.
 */
inline int str_to_int(const std::string& str) {
	int result = 0;

	// Convert and check if successful
	auto conv_res = std::from_chars(str.data(), str.data() + str.size(), result);

	if (conv_res.ptr != (str.data() + str.size())) {
		SHLOG_E_THROW("Incorrect string to covnert: " << str);
	}

	return result;
}

/**
 * Parses the `T_` type from the string using std streams.
 *
 * \remark Because it uses streams, it is quite slow
 * \param	s		String to be parsed as `T_`.
 * \returns		Parsed integer.
 */
template <typename T_, typename S_>
static inline T_ str2(const S_& s) {
	std::stringstream ss(s);
	T_ r;
	ss >> r;
	return r;
}

/**
 * Returns the squared parameter.
 *
 * \param	x	Number to be squared.
 * \returns		The squared number.
 */
template <typename T_>
inline static T_ square(T_ a) {
	return a * a;
}

/**
 * Returns a pseudorandom integral number sampled from the uniform distribution [from, to].
 *
 * \param	from	Lower distribution bound.
 * \param	to	Upper distribution bound (inclusive).
 * \returns		The sampled number.
 */
template <typename T_>
T_ irand(T_ from, T_ to) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<T_> dist(from, to);

	return dist(gen);
}

/**
 * Returns a pseudorandom floating point number sampled from the uniform distribution [from, to).
 *
 * \param	from	Lower distribution bound.
 * \param	to	Upper distribution bound (exclusive).
 * \returns		The sampled number.
 */
template <typename T_>
T_ frand(T_ from, T_ to) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<T_> dist(from, to);

	return dist(gen);
}

/**
 * Splits the string with the provided character delimiter.
 *
 * \param	str		String to be split.
 * \param	delim		Delimiter to split with.
 * \returns	Vector of resulting strings.
 */
inline std::vector<std::string> split(const std::string& str, char delim) {
	std::vector<std::string> result;
	std::stringstream ss(str);
	std::string item;

	while (getline(ss, item, delim)) {
		result.emplace_back(item);
	}

	return result;
}

/**
 * Tests whether the `i`-th lowest significant bit  is set.
 *
 * \param	mask	Bitfield to test in.
 * \param	i	Index of the LSb in question.
 * \returns		True if the bit is high.
 */
template <typename T_>
bool is_set(T_ mask, size_t i) {
	return ((mask >> i) & 0x01) == 1;
}

template <typename Container>
void print_matrix(const Container& mat) {
	for (auto&& row : mat) {
		print_vector(row);
	}
}

template <typename Container>
void print_vector(const Container& row) {
	for (auto&& v : row) {
		std::cout << "\t" << std::fixed << std::setprecision(4) << v;
	}
	std::cout << std::endl;
}

inline std::string to_lowercase(const std::string& old) {
	std::string transformed;

	std::transform(old.begin(), old.end(), std::back_inserter(transformed), ::tolower);

	return transformed;
}

/**
 * Computes the SHA256 hash for the given file and returns it.
 */
inline std::string SHA256_sum(const std::string& filepath) {
	// \todo test with large files
	third_party::SHA256 hash;

	std::ifstream f(filepath, std::ios::binary);
	if (!f.is_open()) {
		std::string msg{ "Unable to open file '" + filepath + "'." };
		SHLOG_E(msg);
		throw std::runtime_error{ msg };
	}

	const size_t buff_size = 4096;
	char* buffer = new char[buff_size];

	while (f) {
		f.read(buffer, buff_size);
		size_t bytes_read = size_t(f.gcount());

		hash.add(buffer, bytes_read);
	}

	return hash.getHash();
}

inline std::string read_whole_file(const std::string& filepath) {
	std::ifstream ifs{ filepath };
	if (!ifs.is_open()) {
		std::string msg{ "Error opening file: " + filepath };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	// Rad the whole file
	ifs.seekg(0, std::ios::end);
	size_t size = ifs.tellg();

	std::string file_content(size, ' ');

	ifs.seekg(0);
	ifs.read(&file_content[0], size);

	return file_content;
}

/** Left trim. */
inline std::string trim_left(std::string s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
	return s;
}

/** Right trim. */
inline std::string trim_right(std::string s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
	return s;
}

/** Bi-trim. */
inline std::string trim(std::string s) { return trim_right(trim_left(s)); }

template <typename DType_>
void to_file(const std::vector<DType_>& vec, const std::string filepath) {
	std::ofstream ofs(filepath, std::ios::out | std::ios::binary);
	if (!ofs) {
		std::string msg{ "Error openning file: " + filepath };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	for (auto&& d : vec) {
		const char* p_d{ reinterpret_cast<const char*>(&d) };
		ofs.write(p_d, sizeof(DType_));
	}
}

template <typename DType_>
void to_file(const std::vector<std::vector<DType_>>& mat, const std::string filepath) {
	std::ofstream ofs(filepath, std::ios::out | std::ios::binary);
	if (!ofs) {
		std::string msg{ "Error openning file: " + filepath };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	for (auto&& vec : mat) {
		for (auto&& d : vec) {
			const char* p_d{ reinterpret_cast<const char*>(&d) };
			ofs.write(p_d, sizeof(DType_));
		}
	}
}

/** UInteger power function. */
template <typename T_>
T_ ipow(T_ b, std::size_t p) {
	T_ r = 1;
	for (std::size_t i = 0; i < p; ++i) {
		r = r * b;
	}
	return r;
}

/** Rounds the number to the specified decimal places. */
template <typename T_>
T_ round_decimal(T_ x, std::size_t places) {
	std::size_t b = ipow(10, places);
	return static_cast<T_>(std::round(b * x) / b);
}

/**
 * Returns a string view describing the type T_.
 *
 * Code from: https://stackoverflow.com/a/20170989/5481153
 */
template <class T_>
constexpr std::string_view type_name() {
	using namespace std;
#ifdef __clang__
	string_view p = __PRETTY_FUNCTION__;
	return string_view(p.data() + 34, p.size() - 34 - 1);
#elif defined(__GNUC__)
	string_view p = __PRETTY_FUNCTION__;
#	if __cplusplus < 201402
	return string_view(p.data() + 36, p.size() - 36 - 1);
#	else
	return string_view(p.data() + 49, p.find(';', 49) - 49);
#	endif
#elif defined(_MSC_VER)
	string_view p = __FUNCSIG__;
	return string_view(p.data() + 84, p.size() - 84 - 7);
#endif
}

}  // namespace utils
}  // namespace sh
#endif  // UTILS_H_
