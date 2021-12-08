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

#ifndef CONFIG_H_
#define CONFIG_H_

#include <cstddef>

namespace sh {
namespace sconfig {

/**
 * Internal logger lever.
 *
 * 4 = +debug, 3 = +info, 2 = +warnings, 1 = +errors 0 = none
 */
#define LOGLEVEL 3

/** If STDOUT/STDERR streams should be synchronized. */
#define LOCK_STDOUT 0

/** If verbose CURL debug request log should be printed. */
#define DEBUG_CURL_REQUESTS 0

/** If 1, each call to Core API will be logged. */
#define LOG_API_CALLS 0

/** How many following frames we inspect during the ranking. */
#define KW_TEMPORAL_SPAN 5

/** Zero scores will be substituted with this value. */
#define MINIMAL_SCORE 1e-18f

/** Maximum number of threads to spawn for short time jobs. */
#define MAX_NUM_TEMP_WORKERS 4

/** Keyword to type in to end the HTTP API listening loop. */
constexpr const char* EXIT_HTTP_API_LOOP_KEYWORD = "exit";

/** Sleep interval for the  the HTTP API listening loop. */
constexpr std::size_t HTTP_API_LOOP_SLEEP = 1000;

};  // namespace sconfig

// ----------------------------------------
// Old

/*
 * Scoring/SOM stuff
 */
#define TOPN_LIMIT 20000
#define TOPKNN_LIMIT 10000
#define SOM_ITERS 30000

/** Pop-up window image grid width */
#define DISPLAY_GRID_WIDTH 6

/** Pop-up window image grid height */
#define DISPLAY_GRID_HEIGHT 6

constexpr int TOP_N_SELECTED_FRAME_POSITION = 2;
constexpr float RANDOM_DISPLAY_WEIGHT = 3.0f;

/** SOM window image grid width */
constexpr size_t SOM_DISPLAY_GRID_WIDTH = 8;

/** SOM window image grid height */
constexpr size_t SOM_DISPLAY_GRID_HEIGHT = 8;

/** SOM relocation grid width */
constexpr size_t RELOCATION_GRID_WIDTH = 5;

/** SOM relocation grid height */
constexpr size_t RELOCATION_GRID_HEIGHT = 5;

/** Maximal size of temporal query */
constexpr int MAX_TEMPORAL_SIZE = 2;

/* ***********************************
 * Logging names
 * *********************************** */
namespace LOGGING_STRINGS {
namespace STD_CATEGORIES {
constexpr const char* BROWSING = "BROWSING";
constexpr const char* IMAGE = "IMAGE";
constexpr const char* TEXT = "TEXT";
};  // namespace STD_CATEGORIES

namespace STD_TYPES {
constexpr const char* EXPLORATION = "exploration";
constexpr const char* RANKED_LIST = "rankedList";
constexpr const char* RANDOM_SELECTION = "randomSelection";
};  // namespace STD_TYPES

namespace STD_VALUES {
constexpr const char* RANDOM_DISPLAY = "randomDisplay";
constexpr const char* SOM_DISPLAY = "somDisplay";

constexpr const char* TOP_SCORED_DISPLAY = "topScoredDisplay";
constexpr const char* SOM_RELOC_DISPLAY = "somRelocationDisplay";
constexpr const char* TOP_SCORED_CONTEXT_DISPLAY = "topnContextDisplay";
};  // namespace STD_VALUES

namespace STD_KEYS {
constexpr const char* ACTION_NAME = "actionName";
};

namespace ACTION_NAMES {
constexpr const char* SHOW_RANDOM_DISPLAY = "showRandomDisplay";
constexpr const char* SHOW_SOM_DISPLAY = "showSomDisplay";
constexpr const char* SHOW_SOM_RELOC_DISPLAY = "showSomRelocationDisplay";
constexpr const char* SHOW_TOP_SCORED_DISPLAY = "showTopScoredDisplay";
constexpr const char* SHOW_TOP_SCORED_CONTEXT_DISPLAY = "showTopScoredContextDisplay";

constexpr const char* RESET_ALL = "resetAll";

};  // namespace ACTION_NAMES
};  // namespace LOGGING_STRINGS

};  // namespace sh

#endif  // CONFIG_H_
