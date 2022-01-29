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

#ifndef COMMON_H_
#define COMMON_H_

// Forward declare tester classes.
namespace sh {
namespace tests {

class TESTER_Somhunter;

};  // namespace tests
};  // namespace sh

// *** CONFIGS ***

#include "intrinsics.h"
#include "static-config.h"

// *** TYPES ***

#include "common-types.h"
#include "settings.h"

// *** TOOLS ***
#include "static-logger.hpp"

// helper type for the visitor #4
template <class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};

// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

#include <string>
using namespace std::literals;

/**
 * Until C++20 and ranges come.
 *
 * Copied from here: https://stackoverflow.com/a/63341946/5481153
 */
template <typename NumericType>
struct ioterable {
	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = NumericType;
	using difference_type = NumericType;
	using pointer = std::add_pointer_t<NumericType>;
	using reference = NumericType;

	explicit ioterable(NumericType n) : val_(n) {}

	ioterable() = default;
	ioterable(ioterable&&) = default;
	ioterable(ioterable const&) = default;
	ioterable& operator=(ioterable&&) = default;
	ioterable& operator=(ioterable const&) = default;

	ioterable& operator++() {
		++val_;
		return *this;
	}
	ioterable operator++(int) {
		ioterable tmp(*this);
		++val_;
		return tmp;
	}
	bool operator==(ioterable const& other) const { return val_ == other.val_; }
	bool operator!=(ioterable const& other) const { return val_ != other.val_; }

	value_type operator*() const { return val_; }

private:
	NumericType val_{ std::numeric_limits<NumericType>::max() };
};

#endif  // COMMON_H_
