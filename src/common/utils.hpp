
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

namespace sh
{
namespace utils
{
template <typename DataType>
void serialize_to_file(DataType data, const std::string filepath)
{
	std::ofstream ofs(filepath, std::ios::out | std::ios::binary);
	if (!ofs) {
		std::string msg{ "Error openning file: " + filepath };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
		return;
	}

	cereal::BinaryOutputArchive out_archive(ofs);
	out_archive(data);
}

template <typename DataType>
DataType deserialize_from_file(const std::string filepath)
{
	std::ifstream ifs(filepath, std::ios::in | std::ios::binary);
	if (!ifs) {
		std::string msg{ "Error openning file: " + filepath };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	cereal::BinaryInputArchive in_archive(ifs);

	DataType data;
	in_archive(data);
	return data;
}

/*!
 * Returns string representing current time and date in formated
 * string based on provided format.
 *
 * @remarks
 *  Use format string as for put_time method
 *  (https://en.cppreference.com/w/cpp/io/manip/put_time)
 *
 *  "%d-%m-%Y_%H-%M-%S" => e.g. "16-11-2019_13-26-45:
 *
 *  @param fmt  Format string using the same rules as put_time method.
 *  @return   String representing current date and time in desired format.
 */
inline std::string get_formated_timestamp(const std::string& fmt)
{
	auto ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

	std::stringstream ss;
	ss << std::put_time(std::localtime(&ts), fmt.data());
	return ss.str();
}

inline std::string get_formated_timestamp(const std::string& fmt, UnixTimestamp ts)
{
	auto x = std::chrono::duration<std::size_t, std::milli>(ts);
	std::chrono::time_point<std::chrono::system_clock> tp{ x };
	auto tts{ std::chrono::system_clock::to_time_t(tp) };

	std::stringstream ss;
	ss << std::put_time(std::localtime(&tts), fmt.data());
	return ss.str();
}

inline int str_to_int(const std::string& str)
{
	int result = 0;

	// Convert and check if successful
	auto conv_res = std::from_chars(str.data(), str.data() + str.size(), result);

	if (conv_res.ptr != (str.data() + str.size())) {
		std::string msg{ "Incorrect string to covnert: " + str };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	return result;
}

template <typename T, typename S>
static inline T str2(const S& s)
{
	std::stringstream ss(s);
	T r;
	ss >> r;
	return r;
}

template <typename T>
inline float d_manhattan(const std::vector<T>& left, const std::vector<T>& right)
{
	if (left.size() != right.size()) {
		SHLOG_E("Vectors have different sizes.");
#ifndef NDEBUG
		throw std::runtime_error("Vectors have different sizes.");
#endif
	}

	float s = 0;

	for (size_t d = 0; d < left.size(); ++d) {
		s += abs(left[d] - right[d]);
	}
	return s;
}

template <typename T>
inline float d_sqeucl(const std::vector<T>& left, const std::vector<T>& right)
{
	if (left.size() != right.size()) {
		SHLOG_E("Vectors have different sizes.");
#ifndef NDEBUG
		throw std::runtime_error("Vectors have different sizes.");
#endif
	}

	float s = 0;
	for (size_t d = 0; d < left.size(); ++d) {
		s += squaref(left[d] - right[d]);
	}
	return s;
}

template <typename T>
inline float d_eucl(const std::vector<T>& left, const std::vector<T>& right)
{
	return sqrtf(d_sqeucl(left, right));
}

inline static float squaref(float a) { return a * a; }

inline float d_cos(const std::vector<float>& left, const std::vector<float>& right)
{
	float s = 0.0f;
	float w1 = 0.0f;
	float w2 = 0.0f;

	for (size_t i = 0; i < left.size(); ++i) {
		s += left[i] * right[i];

		w1 += squaref(left[i]);
		w2 += squaref(right[i]);
	}
	if (w1 == 0 && w2 == 0) return 0;
	return 1.0f - (s / (sqrtf(w1) * sqrtf(w2)));
}

template <typename T>
inline std::vector<T> VecSub(const std::vector<T>& left, const std::vector<T>& right)
{
	if (left.size() != right.size()) {
		SHLOG_E("Vectors have different sizes.");
#ifndef NDEBUG
		throw std::runtime_error("Vectors have different sizes.");
#endif
	}

	std::vector<T> result;
	result.resize(left.size());

	size_t i = 0;
	for (auto& v : result) {
		v = left[i] - right[i];
		++i;
	}

	return result;
}

template <typename T>
inline std::vector<T> VecAdd(const std::vector<T>& left, const std::vector<T>& right)
{
	if (left.size() != right.size()) {
		SHLOG_E("Vectors have different sizes.");
#ifndef NDEBUG
		throw std::runtime_error("Vectors have different sizes.");
#endif
	}

	std::vector<T> result;
	result.resize(left.size());

	size_t i = 0;
	for (auto& v : result) {
		v = left[i] + right[i];
		++i;
	}

	return result;
}

template <typename T, typename S>
inline std::vector<T> VecMult(const std::vector<T>& left, S right)
{
	std::vector<T> result(left.size());

	std::transform(left.begin(), left.end(), result.begin(), [right](const T& l) { return l * right; });

	return result;
}

template <typename T>
inline std::vector<T> VecMult(const std::vector<T>& left, const std::vector<T>& right)
{
	if (left.size() != right.size()) {
		SHLOG_E("Vectors have different sizes.");
#ifndef NDEBUG
		throw std::runtime_error("Vectors have different sizes.");
#endif
	}

	std::vector<T> result;

	std::transform(left.begin(), left.end(), right.begin(), std::back_inserter(result),
	               [](const T& l, const T& r) { return l * r; });

	return result;
}

template <typename T>
inline T VecDot(const std::vector<T>& left, const std::vector<T>& right)
{
	if (left.size() != right.size()) {
		SHLOG_E("Vectors have different sizes.");
#ifndef NDEBUG
		throw std::runtime_error("Vectors have different sizes.");
#endif
	}

	std::vector<T> sum = VecMult<T>(left, right);

	return std::accumulate(sum.begin(), sum.end(), 0.0f, std::plus<T>());
}

template <typename T>
inline std::vector<T> MatVecProd(const std::vector<std::vector<T>>& mat, const std::vector<T>& vec)
{
	if (mat.empty() || mat[0].size() != vec.size()) {
		SHLOG_E("Vectors have different sizes.");
#ifndef NDEBUG
		throw std::runtime_error("Vectors have different sizes.");
#endif
	}

	std::vector<T> result;
	result.resize(mat.size());

	size_t i = 0;
	for (auto&& mat_row_vec : mat) result[i++] = VecDot(mat_row_vec, vec);

	return result;
}

template <typename T>
inline float VecLen(const std::vector<T>& left)
{
	return sqrtf(VecDot(left, left));
}

template <typename T>
inline std::vector<T> VecNorm(const std::vector<T>& left)
{
	float vec_size = VecLen(left);

	if (vec_size > 0.0f)
		return VecMult(left, (1.0f / vec_size));
	else {
		SHLOG_E("Zero vector!");
#ifndef NDEBUG
		throw std::runtime_error("Zero vector!");
#else
		return std::vector<T>{};
#endif
	}
}

/**
 * Vectors must have unit size!
 */
inline float d_cos_normalized(const std::vector<float>& left, const std::vector<float>& right)
{
	return 1.0f - VecDot(left, right);
}

/**
 * Vectors must have unit size!
 */

inline float d_cos_normalized(const float* left, const float* right, size_t dim)
{
	float s = 0.0f;
	const float* iv = left;
	const float* jv = right;

	for (size_t d = 0; d < dim; ++d) {
		s += iv[d] * jv[d];
	}

	return 1.0f - s;
}

inline float d_cos_normalized(const std::vector<float>& left, const float* right, size_t dim)
{
	return d_cos_normalized(left.data(), right, dim);
}

/**
 * Vectors must have unit size!
 */
inline float cos_sim_normalized(const std::vector<float>& left, const float* right, size_t dim)
{
	float s = 0.0f;
	const float* iv = left.data();
	const float* jv = right;

	for (size_t d = 0; d < dim; ++d) {
		s += iv[d] * jv[d];
	}

	return s;
}

inline static float square(float a) { return a * a; }

inline int64_t timestamp()
{
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

/** Returns pseudorandom integral number sampled from
 *  the uniform distribution [from, to]. */
template <typename T>
T irand(T from, T to)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<T> dist(from, to);

	return dist(gen);
}

/** Returns pseudorandom floating point number sampled from
 *  the uniform distribution [from, to). */
template <typename T>
T frand(T from, T to)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<T> dist(from, to);

	return dist(gen);
}

inline std::vector<std::string> split(const std::string& str, char delim)
{
	std::vector<std::string> result;
	std::stringstream ss(str);
	std::string item;

	while (getline(ss, item, delim)) {
		result.emplace_back(item);
	}

	return result;
}

template <typename T>
bool is_set(T mask, size_t i)
{
	return ((mask >> i) & 0x01) == 1;
}

template <typename Container>
void print_matrix(const Container& mat)
{
	for (auto&& row : mat) {
		print_vector(row);
	}
}

template <typename Container>
void print_vector(const Container& row)
{
	for (auto&& v : row) {
		std::cout << "\t" << std::fixed << std::setprecision(4) << v;
	}
	std::cout << std::endl;
}

inline std::string to_lowercase(const std::string& old)
{
	std::string transformed;

	std::transform(old.begin(), old.end(), std::back_inserter(transformed), ::tolower);

	return transformed;
}

/**
 * Computes the SHA256 hash for the given file and returns it.
 */
inline std::string SHA256_sum(const std::string& filepath)
{
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

inline std::string read_whole_file(const std::string& filepath)
{
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
inline std::string trim_left(std::string s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
	return s;
}

/** Right trim. */
inline std::string trim_right(std::string s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
	return s;
}

/** Bi-trim. */
inline std::string trim(std::string s) { return trim_right(trim_left(s)); }

inline bool file_exists(const std::string& filepath) { return std::filesystem::exists(filepath); }

inline bool dir_exists(const std::string& path) { return std::filesystem::is_directory(path); }

inline bool dir_create(const std::string& path)
{
	try {
		if (!std::filesystem::is_directory(path)) {
			std::filesystem::create_directories(path);
		}
	} catch (...) {
		return false;
	}

	return true;
}

template <typename DType_>
void to_file(const std::vector<DType_>& vec, const std::string filepath)
{
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
void to_file(const std::vector<std::vector<DType_>>& mat, const std::string filepath)
{
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
template <typename T>
T ipow(T b, std::size_t p)
{
	T r = 1;
	for (std::size_t i = 0; i < p; ++i) {
		r = r * b;
	}
	return r;
}

/** Runds the number to the specified decimal places. */
template <typename T>
T round_decimal(T x, std::size_t places)
{
	std::size_t b = ipow(10, places);
	return static_cast<T>(std::round(b * x) / b);
}

}  // namespace utils
}  // namespace sh
#endif  // UTILS_H_
