
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

/** \file vector.hpp
 *
 * File implementing oprations with vectors.
 */

#ifndef VECTOR_H_
#define VECTOR_H_

#include <execution>
#include <vector>
// ---
#include "common.h"
#include "distances.hpp"

namespace math {
namespace vector {

/**
 * Computes the substraction of the given vectors (left - right).
 *
 * \todo Vectorize.
 */
template <typename T_>
inline std::vector<T_> sub(const std::vector<T_>& left, const std::vector<T_>& right) {
	if (left.size() != right.size()) {
		SHLOG_E("Vectors have different sizes.");
#ifndef NDEBUG
		throw std::runtime_error("Vectors have different sizes.");
#endif
	}

	std::vector<T_> result;
	result.resize(left.size());

	size_t i = 0;
	for (auto& v : result) {
		v = left[i] - right[i];
		++i;
	}

	return result;
}

/**
 * Computes the addition of the given vectors (left + right).
 *
 * \todo Vectorize.
 */
template <typename T_>
inline std::vector<T_> add(const std::vector<T_>& left, const std::vector<T_>& right) {
	if (left.size() != right.size()) {
		SHLOG_E("Vectors have different sizes.");
#ifndef NDEBUG
		throw std::runtime_error("Vectors have different sizes.");
#endif
	}

	std::vector<T_> result;
	result.resize(left.size());

	size_t i = 0;
	for (auto& v : result) {
		v = left[i] + right[i];
		++i;
	}

	return result;
}

/**
 * Computes the element-wise multiplication of the given vector and constant `right`.
 *
 * \todo Vectorize.
 */
template <typename T_, typename S_>
inline std::vector<T_> mult(const std::vector<T_>& left, S_ right) {
	std::vector<T_> result(left.size());

	std::transform(std::execution::par_unseq, left.begin(), left.end(), result.begin(),
	               [right](const T_& l) { return l * right; });

	return result;
}

/**
 * Computes the element-wise multiplication of the given vectors.
 *
 * \todo Vectorize.
 */
template <typename T_>
inline std::vector<T_> mult(const std::vector<T_>& left, const std::vector<T_>& right) {
	if (left.size() != right.size()) {
		SHLOG_E("Vectors have different sizes.");
#ifndef NDEBUG
		throw std::runtime_error("Vectors have different sizes.");
#endif
	}

	std::vector<T_> result;

	std::transform(left.begin(), left.end(), right.begin(), std::back_inserter(result),
	               [](const T_& l, const T_& r) { return l * r; });

	return result;
}

/**
 * Computes the dot product of the given vectors.
 *
 * \todo Vectorize.
 */
template <typename T_>
inline T_ dot(const std::vector<T_>& left, const std::vector<T_>& right) {
	if (left.size() != right.size()) {
		SHLOG_E("Vectors have different sizes.");
#ifndef NDEBUG
		throw std::runtime_error("Vectors have different sizes.");
#endif
	}

	std::vector<T_> sum = mult<T_>(left, right);

	return std::accumulate(sum.begin(), sum.end(), 0.0f, std::plus<T_>());
}

template <typename T_>
inline std::vector<T_> mat_mult(const std::vector<std::vector<T_>>& mat, const std::vector<T_>& vec) {
	if (mat.empty() || mat[0].size() != vec.size()) {
		SHLOG_E("Vectors have different sizes.");
#ifndef NDEBUG
		throw std::runtime_error("Vectors have different sizes.");
#endif
	}

	std::vector<T_> result;
	result.resize(mat.size());

	size_t i = 0;
	for (auto&& mat_row_vec : mat) result[i++] = dot(mat_row_vec, vec);

	return result;
}

template <typename T_>
inline float length(const std::vector<T_>& left) {
	return sqrtf(dot(left, left));
}

template <typename T_>
inline std::vector<T_> normalize(const std::vector<T_>& left) {
	float vec_size = length(left);

	if (vec_size > 0.0f)
		return mult(left, (1.0f / vec_size));
	else {
		SHLOG_E("Zero vector!");
#ifndef NDEBUG
		throw std::runtime_error("Zero vector!");
#else
		return std::vector<T>{};
#endif
	}
}

};  // namespace vector
};  // namespace math

#endif  // VECTOR_H_
