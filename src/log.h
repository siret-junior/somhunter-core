
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

#ifndef log_h
#define log_h

#include "config.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#define FILE_NAME_TAIL_LEN 50

namespace TermColor {
enum Code {
	FG_RED = 31,
	FG_GREEN = 32,
	FG_YELLOW = 33,
	FG_BLUE = 34,
	FG_CYAN = 36,
	FG_WHITE = 37,
	FG_DEFAULT = 39,

	BG_RED = 41,
	BG_GREEN = 42,
	BG_YELLOW = 43,
	BG_BLUE = 44,
	BG_CYAN = 46,
	BG_WHITE = 47,
	BG_DEFAULT = 49
};
class Modifier {
	Code code;

public:
	Modifier(Code pCode) : code(pCode) {}
	friend std::ostream& operator<<(std::ostream& os, const Modifier& mod) { return os << "\033[" << mod.code << "m"; }
};

static Modifier red{ TermColor::FG_RED };
static Modifier green{ TermColor::FG_GREEN };
static Modifier yellow{ TermColor::FG_YELLOW };
static Modifier blue{ TermColor::FG_BLUE };
static Modifier cyan{ TermColor::FG_CYAN };
static Modifier white{ TermColor::FG_WHITE };
static Modifier def{ TermColor::FG_DEFAULT };

static Modifier redbg{ TermColor::BG_RED };
static Modifier greenbg{ TermColor::BG_GREEN };
static Modifier yellowbg{ TermColor::BG_YELLOW };
static Modifier bluebg{ TermColor::BG_BLUE };
static Modifier cyanbg{ TermColor::BG_CYAN };
static Modifier whitebg{ TermColor::BG_WHITE };
static Modifier defbg{ TermColor::BG_DEFAULT };
}  // namespace TermColor

/**
 * Returns a view pointing to at most `len` long tail of the `str`.
 */
static std::string_view view_tail(const std::string& str, size_t len) {
	return std::string_view{ str.data() + std::max<size_t>(0, str.length() - len) };
}

#define ASSERT(cond, msg)                  \
	do {                                   \
		if (!(cond)) {                     \
			std::cerr << msg << std::endl; \
			throw std::logic_error(msg);   \
		}                                  \
	} while (false);

#if LOGLEVEL > 0

#	define _dont_write_log_d \
		do {                  \
		} while (0)
#	define _write_log_d(level, x)                                                                               \
		do {                                                                                                     \
			std::cerr << level << x << "\n\t" << __func__ << "() in " << view_tail(__FILE__, FILE_NAME_TAIL_LEN) \
			          << " :" << __LINE__ << "" << std::endl;                                                    \
		} while (0)

#	define LOG_E(x) _write_log_d(TermColor::red << "E:", x << TermColor::def)
#else
#	define LOG_E(x)
#	define _write_log_d(level, x)
#endif

#if LOGLEVEL > 1
#	define LOG_W(x) _write_log_d(TermColor::yellow << "W: ", x << TermColor::def)
#else
#	define LOG_W(x) _dont_write_log_d
#endif

#if LOGLEVEL > 2
#	define LOG_I(x) _write_log_d(TermColor::white << "I: ", x << TermColor::def)
#	define LOG_S(x) _write_log_d(TermColor::green << "S: ", x << TermColor::def)
#else
#	define LOG_I(x) _dont_write_log_d
#	define LOG_S(x) _dont_write_log_d
#endif

#if LOGLEVEL > 3
#	define LOG_D(x) _write_log_d(TermColor::def << "D: ", x << TermColor::def)
#else
#	define LOG_D(x) _dont_write_log_d
#endif

#ifdef LOG_API_CALLS

#	define _write_API_log_d(id, x)                                                                      \
		do {                                                                                             \
			std::cout << TermColor::blue << "--> [ " << id << " ] " << TermColor::cyan << x << std::endl \
			          << TermColor::def;                                                                 \
		} while (0)

#	define LOG_REQUEST(id, x) _write_API_log_d(id, x)

#endif  // LOGAPI

#endif  // log_h