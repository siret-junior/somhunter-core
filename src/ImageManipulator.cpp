
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

#include "ImageManipulator.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>

// Let STB to put it's implementation inside this TU
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_resize.h>
#include <stb_image_write.h>

#include "log.h"

LoadedImage
ImageManipulator::load(const std::string& filepath)
{
	//#define WITH_HDR

	int w{ 0 };
	int h{ 0 };
	int num_channels{ 0 };

#ifdef WITH_HDR
	/* This applies `stbi_hdr_to_ldr_gamma` and `stbi_hdr_to_ldr_scale` since it's meant for HDR images	 */
	float* raw_data = stbi_loadf(filepath.c_str(), &w, &h, &num_channels, 0);

#else
	/* Do it manually */
	unsigned char* raw_data = stbi_load(filepath.c_str(), &w, &h, &num_channels, 0);
#endif

	if (raw_data == NULL) {
		std::string msg{ "Loading the '" + filepath + "' image failed!" };
		warn_d(msg);
		throw std::runtime_error(msg);
	}

	unsigned char* p_begin{ raw_data };
	unsigned char* p_end{ raw_data + (w * h * num_channels) };
#ifdef WITH_HDR
	// Copy the data into a safer container
	std::vector<float> data{ p_begin, p_end };
#else
	std::vector<float> data;
	data.reserve(w * h * num_channels);

	for (unsigned char* it{ p_begin }; it != p_end; ++it) {
		data.emplace_back(float(*it) / 255);
	}
#endif

	stbi_image_free(raw_data);

	return LoadedImage{ size_t(w), size_t(h), size_t(num_channels), std::move(data) };
}

void
ImageManipulator::store_jpg(const std::string& filepath,
                            const std::vector<float>& in,
                            size_t w,
                            size_t h,
                            size_t quality,
                            size_t num_channels,
                            bool are_ints)
{
	std::vector<uint8_t> raw_data;
	raw_data.reserve(in.size());

	// if \in [0.0, 1.0]
	if (!are_ints) {
		std::transform(
		  in.begin(), in.end(), std::back_inserter(raw_data), [](const float& x) { return uint8_t(x * 255); });
	} else {
		std::transform(
		  in.begin(), in.end(), std::back_inserter(raw_data), [](const float& x) { return uint8_t(x); });
	}

	auto res{ stbi_write_jpg(filepath.c_str(), w, h, num_channels, raw_data.data(), quality) };

	if (res == 0) {
		std::string msg{ "Writing the '" + filepath + "' image failed!" };
		warn_d(msg);
		throw std::runtime_error(msg);
	}
}

std::vector<float>
ImageManipulator::resize(const std::vector<float>& in,
                         size_t orig_w,
                         size_t orig_h,
                         size_t new_w,
                         size_t new_h,
                         size_t num_channels)
{
	std::vector<float> out(new_w * new_h * num_channels, 0.0F);

	auto res{ stbir_resize_float(in.data(), orig_w, orig_h, 0, out.data(), new_w, new_h, 0, num_channels) };

	if (res == 0) {
		std::string msg{ "Resizing the image failed!" };
		warn_d(msg);
		throw std::runtime_error(msg);
	}

	return out;
}