
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

/* ***************************************
      SNIPPETS
   ---------------

// Turn of the logging locally
//#if LOGLEVEL > 3
//#	undef SHLOG_D
//#	define SHLOG_D(x) _dont_write_log_err
//#endif

  ***************************************** */

#ifndef log_h
#define log_h

#include <cmath>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include "common.h"

#define FILE_NAME_TAIL_LEN 50

/** If `true` the assertions will be executed */
constexpr bool RUN_ASSERTS = true;

namespace TermColor {
enum Code {
	FG_RED = 31,
	FG_GREEN = 32,
	FG_YELLOW = 33,
	FG_BLUE = 34,
	FG_CYAN = 36,
	FG_WHITE = 37,
	FG_GREY = 90,
	FG_DEFAULT = 39,

	BG_RED = 41,
	BG_GREEN = 42,
	BG_YELLOW = 43,
	BG_BLUE = 44,
	BG_CYAN = 46,
	BG_WHITE = 47,
	BG_GREY = 100,
	BG_DEFAULT = 49
};
class Modifier {
	Code code;

public:
	Modifier(Code pCode) : code(pCode) {}
	friend std::ostream& operator<<(std::ostream& os, const Modifier& mod) { return os << "\033[" << mod.code << "m"; }
};

static Modifier red{ TermColor::Code::FG_RED };
static Modifier green{ TermColor::Code::FG_GREEN };
static Modifier yellow{ TermColor::Code::FG_YELLOW };
static Modifier blue{ TermColor::Code::FG_BLUE };
static Modifier cyan{ TermColor::Code::FG_CYAN };
static Modifier white{ TermColor::Code::FG_WHITE };
static Modifier grey{ TermColor::Code::FG_GREY };
static Modifier def{ TermColor::Code::FG_DEFAULT };

static Modifier redbg{ TermColor::Code::BG_RED };
static Modifier greenbg{ TermColor::Code::BG_GREEN };
static Modifier yellowbg{ TermColor::Code::BG_YELLOW };
static Modifier bluebg{ TermColor::Code::BG_BLUE };
static Modifier cyanbg{ TermColor::Code::BG_CYAN };
static Modifier whitebg{ TermColor::Code::BG_WHITE };
static Modifier greybg{ TermColor::Code::BG_GREY };
static Modifier defbg{ TermColor::Code::BG_DEFAULT };
}  // namespace TermColor

/**
 * Returns a view pointing to at most `len` long tail of the `str`.
 */
static inline std::string_view view_tail(const std::string& str, size_t len) {
	return std::string_view{ str.data() + std::max<size_t>(0, str.length() - len) };
}

/** Undecorated log to the current log stream */
#define SHLOG(x)                     \
	do {                             \
		std::cout << x << std::endl; \
	} while (0)

#if LOGLEVEL > 0

#	define _dont_write_log_err \
		do {                    \
		} while (0)
#	define _write_log_err(level, x)                                                                         \
		do {                                                                                                 \
			std::cerr << level << x << TermColor::grey << "\n\t" << __func__ << "() in "                     \
			          << view_tail(__FILE__, FILE_NAME_TAIL_LEN) << " :" << __LINE__ << "" << TermColor::def \
			          << std::endl;                                                                          \
		} while (0)

#	define _write_log_out(level, x)                                                                         \
		do {                                                                                                 \
			std::cout << level << x << TermColor::grey << "\n\t" << __func__ << "() in "                     \
			          << view_tail(__FILE__, FILE_NAME_TAIL_LEN) << " :" << __LINE__ << "" << TermColor::def \
			          << std::endl;                                                                          \
		} while (0)

#	define SHLOG_E(x) _write_log_err(TermColor::red << "E:", x << TermColor::def)
#else
#	define SHLOG_E(x)
#	define _write_log_err(level, x)
#endif

#if LOGLEVEL > 1
#	define SHLOG_W(x) _write_log_out(TermColor::yellow << "W: ", x << TermColor::def)
#else
#	define SHLOG_W(x) _dont_write_log_err
#endif

#if LOGLEVEL > 2
#	define SHLOG_I(x) _write_log_out(TermColor::white << "I: ", x << TermColor::def)
#	define SHLOG_S(x) _write_log_out(TermColor::green << "S: ", x << TermColor::def)
#else
#	define SHLOG_I(x) _dont_write_log_err
#	define SHLOG_S(x) _dont_write_log_err
#endif

#if LOGLEVEL > 3
#	define SHLOG_D(x) _write_log_out(TermColor::grey << "D: ", x << TermColor::def)
#else
#	define SHLOG_D(x) _dont_write_log_err
#endif

#ifdef LOG_API_CALLS

#	define _write_API_log_d(id, x)                                                                      \
		do {                                                                                             \
			std::cout << TermColor::blue << "--> [ " << id << " ] " << TermColor::cyan << x << std::endl \
			          << TermColor::def;                                                                 \
		} while (0)

#	define SHLOG_REQ(id, x) _write_API_log_d(id, x)

#endif  // LOGAPI

namespace sh {

/** Assert on equals execuded at all times.
 */
template <typename T1, typename T2>
inline void do_assert_equals(const T1& a, const T2&& b, const std::string_view msg = {}, const char* file = __FILE__,
                             const int line = __LINE__) {
	if (!(a == b)) {
		std::cerr << "ASSERTION FAILED: " << msg << "\n\tEXPECTED: " << a << "\tGOT: " << b << "."
		          << "() in " << view_tail(file, FILE_NAME_TAIL_LEN) << " :" << line << "" << std::endl;
	}
}

/** Assert execuded at all times. */
template <typename T>
inline void do_assert(T&& assertion, const std::string_view msg = {}, const char* file = __FILE__,
                      const int line = __LINE__) {
	if (!assertion) {
		std::cerr << "ASSERTION FAILED: " << msg << "\n\t"
		          << "."
		          << "() in " << view_tail(file, FILE_NAME_TAIL_LEN) << " :" << line << "" << std::endl;
	}
}

/** Assert execuded only if `RUN_ASSERTS` is true. */
template <typename T>
inline void do_assert_debug(T&& assertion, const std::string_view msg = {}, const char* file = __FILE__,
                            const int line = __LINE__) {
	if constexpr (RUN_ASSERTS) {
		do_assert(assertion, msg, file, line);
	}
}

};  // namespace sh

#endif  // log_h