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

#ifndef OS_UTILS_H_
#define OS_UTILS_H_

#if defined WIN32 || defined _WIN32 || defined WIN64 || defined _WIN64
#	include <Windows.h>
#endif  // defined WIN32 || defined _WIN32 || defined WIN64 || defined _WIN64

#include <filesystem>

namespace osutils {

/**
 * Setups the attached terminal accordingly.
 */
inline void setup_terminal() {
	// Enable ANSII colored output if not enabled by default
#if defined WIN32 || defined _WIN32 || defined WIN64 || defined _WIN64
	// From: https://superuser.com/a/1529908

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = 0;
	GetConsoleMode(hOut, &dwMode);
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(hOut, dwMode);

	// References:
	// SetConsoleMode() and ENABLE_VIRTUAL_TERMINAL_PROCESSING?
	// https://stackoverflow.com/questions/38772468/setconsolemode-and-enable-virtual-terminal-processing

	// Windows console with ANSI colors handling
	// https://superuser.com/questions/413073/windows-console-with-ansi-colors-handling
#endif
}

/**
 * Prints available instruction set extensions (not working with MSVC).
 */
inline void print_ISA_capibilites() {
	SHLOG("ISA capibilites: (does not work on Windows)");

#ifdef __SSE__
	SHLOG("__SSE__: true");
#else
	SHLOG("__SSE__: false");
#endif  // __SSE__

#ifdef __SSE2__
	SHLOG("__SSE2__: true");
#else
	SHLOG("__SSE2__: false");
#endif  // __SSE2__

#ifdef __SSE3__
	SHLOG("__SSE3__: true");
#else
	SHLOG("__SSE3__: false");
#endif  // __SSE3__

#ifdef __SSE4_2__
	SHLOG("__SSE4_2__ : true");
#else
	SHLOG("__SSE4_2__ : false");
#endif  // __SSE4_2__

#ifdef __AVX__
	SHLOG("__AVX__: true");
#else
	SHLOG("__AVX__: false");
#endif  // __AVX__

#ifdef __AVX2__
	SHLOG("__AVX2__: true");
#else
	SHLOG("__AVX2__: false");
#endif  // __AVX2__

#ifdef __AVX512BW__
	SHLOG("__AVX512BW__ : true");
#else
	SHLOG("__AVX512BW__ : false");
#endif  // __AVX512BW__

#ifdef __AVX512CD__
	SHLOG("__AVX512CD__  : true");
#else
	SHLOG("__AVX512CD__  : false");
#endif  // __AVX512CD__

#ifdef __AVX512DQ__
	SHLOG("__AVX512DQ__  : true");
#else
	SHLOG("__AVX512DQ__  : false");
#endif  // __AVX512DQ__

#ifdef __AVX512F__
	SHLOG("__AVX512F__  : true");
#else
	SHLOG("__AVX512F__  : false");
#endif  // __AVX512F__

#ifdef __AVX512VL__
	SHLOG("__AVX512VL__  : true");
#else
	SHLOG("__AVX512VL__  : false");
#endif  // __AVX512VL__
}

inline void cd_back(std::size_t count = 1) {
	auto path = std::filesystem::current_path();

	for (std::size_t i{ 0 }; i < count; ++i) {
		std::filesystem::current_path(path.parent_path());
	}
}

/**
 * Runs all initialization routines related to the OS.
 */
inline void initialize_aplication() {
	setup_terminal();
	// print_ISA_capibilites();

	// Change binary directory to the parent one.
	cd_back();
	SHLOG_I("The binary is running from the directory " << std::filesystem::current_path() << "...");
}

inline bool file_exists(const std::string& filepath) { return std::filesystem::exists(filepath); }

inline bool dir_exists(const std::string& path) { return std::filesystem::is_directory(path); }

inline bool dir_create(const std::string& path) {
	try {
		if (!std::filesystem::is_directory(path)) {
			std::filesystem::create_directories(path);
		}
	} catch (...) {
		return false;
	}

	return true;
}

}  // namespace osutils

#endif  // OS_UTILS_H_
