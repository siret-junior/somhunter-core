

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

#ifndef IMAGE_MANIPULATOR_H_
#define IMAGE_MANIPULATOR_H_

#include "common.h"
#include "config.h"
#include "log.h"

namespace sh {

struct LoadedImage {
	size_t w;
	size_t h;
	size_t num_channels;
	std::vector<float> data;
};

/** Provides utilities for image manipulation and processing. */
class ImageManipulator {
public:
	/**
	 * Loads the image from the provided filepath.
	 *
	 * \exception std::runtime_error If the loading fails.
	 */
	static LoadedImage load(const std::string& file);

	/**
	 * Writes the provided image into the JPG file.
	 *
	 * \exception std::runtime_error If the writing fails.
	 */
	static void store_jpg(const std::string& filepath, const std::vector<float>& in, size_t w, size_t h,
	                      size_t quality = 100, size_t num_channels = 3, bool are_ints = false);

	/**
	 * Creates a new resized copy of the provided image matrix.
	 *
	 * \exception std::runtime_error If the resizing fails.
	 */
	static std::vector<float> resize(const std::vector<float>& in, size_t orig_w, size_t orig_h, size_t new_w,
	                                 size_t new_h, size_t num_channels = 3);
};

};  // namespace sh

#endif  // IMAGE_MANIPULATOR_
