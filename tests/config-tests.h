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

#ifndef CONFIG_TESTS_H_
#define CONFIG_TESTS_H_

#include <string>
#include <vector>

/*
 * What dataset are we testing?
 */
#define TESTING_ITEC_DATASET
//#	define TESTING_LSC5DAYS_DATASET

/** Run filter tests on on datasets supporting it */
#ifdef TESTING_LSC5DAYS_DATASET
#	define TEST_FILTERS
#endif

/*
 * What keywords are we testing?
 */
#define TESTING_BOW_W2VV

/* ***
 * Paths
 */
#define TEST_DATA_DIR "tests/data/"
#define TEST_COLLAGE_DATA_DIR TEST_DATA_DIR "/collages/"

#define TESTS_PNGS_DIR TEST_DATA_DIR "images/png/"
#define TESTS_JPEGS_DIR TEST_DATA_DIR "images/jpg/"

/* ***
 * Testing data
 */
// clang-format off
inline std::vector<std::string> TEST_PNGS { 
    TESTS_PNGS_DIR "bitmap-200x100-RGB.png",
    TESTS_PNGS_DIR "bitmap-200x100-RGBA.png",
    TESTS_PNGS_DIR "bitmap-200x100-GRAY.png",
    TESTS_PNGS_DIR "bitmap-200x100-GRAYA.png"
};

inline std::vector<std::string> TEST_JPEGS{ 
    TESTS_JPEGS_DIR "photo-320x180-RGB.jpg"
};
// clang-format on

#endif  // CONFIG_TESTS_H_
