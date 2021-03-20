
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

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#if LOGLEVEL > 0

#	define _dont_write_log_d                                                                                      \
		do {                                                                                                   \
		} while (0)
#	define _write_log_d(level, x)                                                                                 \
		do {                                                                                                   \
			std::cerr << level << x << "\n\t(" << __func__ << " in " __FILE__ " :" << __LINE__ << ")"      \
			          << std::endl;                                                                        \
		} while (0)

#	define warn_d(x) _write_log_d("* ", x)
#else
#	define warn_d(x)
#	define _write_log_d(level, x)
#endif

#if LOGLEVEL > 1
#	define info_d(x) _write_log_d("- ", x)
#else
#	define info_d(x) _dont_write_log_d
#endif

#if LOGLEVEL > 2
#	define debug_d(x) _write_log_d(". ", x)
#else
#	define debug_d(x) _dont_write_log_d
#endif

#define print_d(x) std::cout << x << std::endl;

#define ASSERT(cond, msg)                                                                                              \
	do {                                                                                                           \
		if (!(cond)) {                                                                                         \
			std::cerr << msg << std::endl;                                                                 \
			throw std::logic_error(msg);                                                                   \
		}                                                                                                      \
	} while (false);

#endif // log_h
