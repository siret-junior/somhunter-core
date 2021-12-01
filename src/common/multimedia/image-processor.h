

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

#include <algorithm>
#include <iterator>
#include <stdexcept>

#include <stb_image.h>
#include <stb_image_write.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

#include "common.h"

#include "utils.hpp"

namespace sh {
/** Basic abstraction for images. */
template <typename DType_>
class BitmapImage {
public:
	size_t w;
	size_t h;
	size_t num_channels;
	std::vector<DType_> _data;
};

/** Provides utilities for image manipulation and processing. */
class ImageManipulator {
public:
	template <typename OutType_>
	static OutType_ load_image(const std::string& filepath);

	static void show_image(const std::string& filepath, size_t delay = 0) {
		if (!utils::file_exists(filepath)) {
			std::string msg{ "Unable to open file '" + filepath + "'." };
			SHLOG_E(msg);
			throw std::runtime_error{ msg };
		}

		cv::Mat image = cv::imread(filepath, cv::IMREAD_UNCHANGED);
		cv::imshow(filepath, image);
		cv::waitKey(delay);
	}

	static void show_image(const cv::Mat image, size_t delay = 0) {
		cv::imshow("IMAGE", image);
		cv::waitKey(delay);
	}

	static std::vector<uint8_t> resize(const std::vector<uint8_t>& in, size_t orig_w, size_t orig_h, size_t new_w,
	                                   size_t new_h, size_t num_channels = 3);

	static void store_JPEG(const std::string& filepath, const std::vector<std::uint8_t>& in, size_t w, size_t h,
	                       size_t quality, size_t num_channels);

	/** Returns a copy of uint8 buffer from the float one. */
	static std::vector<float> to_float32(const std::vector<std::uint8_t>& in);

	/** Returns a copy of float buffer from the uint8 one. */
	static std::vector<std::uint8_t> to_uint8(const std::vector<float>& in);

	// NEW
	// -------------------------------------------------
	// OLD

	/**
	 * Loads the image from the provided filepath.
	 *
	 * \exception std::runtime_error If the loading fails.
	 */

	/**
	 * Writes the provided image into the JPG file.
	 *
	 * \exception std::runtime_error If the writing fails.
	 */
	static void store_PNG(const std::string& filepath, const std::vector<uint8_t>& in, size_t w, size_t h,
	                      size_t num_channels) {
		auto res{ stbi_write_png(filepath.c_str(), w, h, num_channels, (uint8_t*)in.data(), w * num_channels) };

		if (res == 0) {
			std::string msg{ "Writing the '" + filepath + "' image failed!" };
			SHLOG_E(msg);
			throw std::runtime_error(msg);
		}
	}

	static void store_PNG(const std::string& filepath, const std::vector<float>& in, size_t w, size_t h,
	                      size_t num_channels) {
		auto res{ stbi_write_png(filepath.c_str(), w, h, num_channels, (float*)in.data(), w * num_channels) };

		if (res == 0) {
			std::string msg{ "Writing the '" + filepath + "' image failed!" };
			SHLOG_E(msg);
			throw std::runtime_error(msg);
		}
	}

	static void store_JPEG(const std::string& filepath, const std::vector<float>& in, size_t w, size_t h,
	                       size_t quality = 100, size_t num_channels = 3, bool are_ints = false);
#ifdef TESTING
private:
#else
public:
#endif
};

};  // namespace sh

#endif  // IMAGE_MANIPULATOR_
