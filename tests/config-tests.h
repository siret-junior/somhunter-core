
#ifndef CONFIG_TESTS_H_
#define CONFIG_TESTS_H_

//#define TEST_COLLAGE_QUERIES

#include <string>
#include <vector>

#include "common.h"



#define DO_TESTS

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

#include "tests.hpp"

/* ***
 * Paths
 */
#define TEST_DATA_DIR "tests/data/"
#define TEST_COLLAGE_DATA_DIR TEST_DATA_DIR "/collages/"

#define TESTS_PNGS_DIR TEST_DATA_DIR "/png/"
#define TESTS_JPEGS_DIR TEST_DATA_DIR "/jpg/"

/* ***
 * Testing data
 */
// clang-format off
std::vector<std::string> TEST_PNGS{ 
    TESTS_PNGS_DIR "bitmap-100x100-4C.png" 
};

std::vector<std::string> TEST_JPEGS{ 
    TESTS_JPEGS_DIR "photo-320x180-3C.jpg" 
};
// clang-format on



#endif  // CONFIG_TESTS_H_
