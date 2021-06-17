
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

namespace sh
{
/**
 * Internal logger lever.
 *
 * 4 = +debug, 3 = +info, 2 = +warnings, 1 = +errors 0 = none
 */
#define LOGLEVEL 4
/** If verbose CURL debug request log should be printed. */
#define DEBUG_CURL_REQUESTS 0

/** If 1, each call to Core API will be logged. */
#define LOG_API_CALLS 1

/** How many following frames we inspect during the ranking. */
#define KW_TEMPORAL_SPAN 5

/** Zero scores will be substituted with this value. */
#define MINIMAL_SCORE 1e-18f

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
namespace LOGGING_STRINGS
{
namespace STD_CATEGORIES
{
constexpr char* BROWSING = "BROWSING";
constexpr char* IMAGE = "IMAGE";
constexpr char* TEXT = "TEXT";
};  // namespace STD_CATEGORIES

namespace STD_TYPES
{
constexpr char* EXPLORATION = "exploration";
constexpr char* RANKED_LIST = "rankedList";
constexpr char* RANDOM_SELECTION = "randomSelection";
};  // namespace STD_TYPES

namespace STD_VALUES
{
constexpr char* RANDOM_DISPLAY = "randomDisplay";
constexpr char* SOM_DISPLAY = "somDisplay";

constexpr char* TOP_SCORED_DISPLAY = "topScoredDisplay";
constexpr char* SOM_RELOC_DISPLAY = "somRelocationDisplay";
constexpr char* TOP_SCORED_CONTEXT_DISPLAY = "topnContextDisplay";
};  // namespace STD_VALUES

namespace STD_KEYS
{
constexpr char* ACTION_NAME = "actionName";
};

namespace ACTION_NAMES
{
constexpr char* SHOW_RANDOM_DISPLAY = "showRandomDisplay";
constexpr char* SHOW_SOM_DISPLAY = "showSomDisplay";
constexpr char* SHOW_SOM_RELOC_DISPLAY = "showSomRelocationDisplay";
constexpr char* SHOW_TOP_SCORED_DISPLAY = "showTopScoredDisplay";
constexpr char* SHOW_TOP_SCORED_CONTEXT_DISPLAY = "showTopScoredContextDisplay";

constexpr char* RESET_ALL = "resetAll";

};  // namespace ACTION_NAMES
};  // namespace LOGGING_STRINGS

};  // namespace sh

#endif  // CONFIG_H_
