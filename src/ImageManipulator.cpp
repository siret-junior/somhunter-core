
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

using namespace sh;

std::vector<float> ImageManipulator::resize(const std::vector<float>& in, size_t orig_w, size_t orig_h, size_t new_w,
                                            size_t new_h, size_t num_channels) {
	std::vector<float> out(new_w * new_h * num_channels, 0.0F);

	int original_w{ int(orig_w) };
	int original_h{ int(orig_h) };

	cv::Mat bitmap_cv_RGB{ in };
	bitmap_cv_RGB = bitmap_cv_RGB.reshape(3, original_w);
	cv::Mat bitmap_cv_BRG{ CV_32FC3 };
	cvtColor(bitmap_cv_RGB, bitmap_cv_BRG, cv::COLOR_BGR2RGBA);

	/*cv::imshow("bitmap_cv_RGB", bitmap_cv_RGB);
	cv::waitKey();

	cv::imshow("bitmap_cv_BRG", bitmap_cv_BRG);
	cv::waitKey();*/

	cv::Mat mat(480, 640, CV_8UC4);

	// cv::resize(bitmap_cv, bitmap_cv, cv::Size(224, 224), 0, 0, cv::INTER_AREA);
	return std::vector<float>{};
}

template <>
cv::Mat ImageManipulator::load_image<cv::Mat>(const std::string& filepath) {
	using namespace cv;
	Mat image = imread(filepath, IMREAD_UNCHANGED);
	return image;
}

template <>
BitmapImage<float> ImageManipulator::load_image<BitmapImage<float>>(const std::string& filepath) {
	cv::Mat cv_img{ ImageManipulator::load_image<cv::Mat>(filepath) };

	// Convert to float32 matrix
	cv::Mat cv_fimg;
	cv_img.convertTo(cv_fimg, CV_32F);
	int total_size{ cv_fimg.rows * cv_fimg.cols * cv_fimg.channels() };

	BitmapImage<float> b_img;
	b_img.w = cv_fimg.cols;
	b_img.h = cv_fimg.rows;
	b_img.num_channels = cv_fimg.channels();

	cv::Mat flat = cv_fimg.reshape(1, cv_fimg.total() * cv_fimg.channels());
	flat.copyTo(b_img.data);

	return b_img;
}

template <>
BitmapImage<uint8_t> ImageManipulator::load_image<BitmapImage<uint8_t>>(const std::string& filepath) {
	cv::Mat cv_img{ ImageManipulator::load_image<cv::Mat>(filepath) };

	// Convert to uint8_t matrix
	cv::Mat cv_fimg;
	cv_img.convertTo(cv_fimg, CV_8U);
	int total_size{ cv_fimg.rows * cv_fimg.cols * cv_fimg.channels() };

	BitmapImage<uint8_t> b_img;
	b_img.w = cv_fimg.cols;
	b_img.h = cv_fimg.rows;
	b_img.num_channels = cv_fimg.channels();

	cv::Mat flat = cv_fimg.reshape(1, cv_fimg.total() * cv_fimg.channels());
	flat.copyTo(b_img.data);
	return b_img;
}

// NEW
// -------------------------------------------------
// OLD

//
// LoadedImage ImageManipulator::load(const std::string& filepath) {
//	//#define WITH_HDR
//
//	int w{ 0 };
//	int h{ 0 };
//	int num_channels{ 0 };
//
//#ifdef WITH_HDR
//	/* This applies `stbi_hdr_to_ldr_gamma` and `stbi_hdr_to_ldr_scale` since it's meant for HDR images	 */
//	float* raw_data = stbi_loadf(filepath.c_str(), &w, &h, &num_channels, 0);
//
//#else
//	/* Do it manually */
//	unsigned char* raw_data = stbi_load(filepath.c_str(), &w, &h, &num_channels, 0);
//#endif
//
//	if (raw_data == NULL) {
//		std::string msg{ "Loading the '" + filepath + "' image failed!" };
//		SHLOG_E(msg);
//		throw std::runtime_error(msg);
//	}
//
//	unsigned char* p_begin{ raw_data };
//	unsigned char* p_end{ raw_data + (w * h * num_channels) };
//#ifdef WITH_HDR
//	// Copy the data into a safer container
//	std::vector<float> data{ p_begin, p_end };
//#else
//	std::vector<float> data;
//	data.reserve(w * h * num_channels);
//
//	for (unsigned char* it{ p_begin }; it != p_end; ++it) {
//		data.emplace_back(float(*it) / 255);
//	}
//#endif
//
//	stbi_image_free(raw_data);
//
//	return LoadedImage{ size_t(w), size_t(h), size_t(num_channels), std::move(data) };
//}

void ImageManipulator::store_jpg(const std::string& filepath, const std::vector<float>& in, size_t w, size_t h,
                                 size_t quality, size_t num_channels, bool are_ints) {
	std::vector<uint8_t> raw_data;
	raw_data.reserve(in.size());

	// if \in [0.0, 1.0]
	if (!are_ints) {
		std::transform(in.begin(), in.end(), std::back_inserter(raw_data),
		               [](const float& x) { return uint8_t(x * 255); });
	} else {
		std::transform(in.begin(), in.end(), std::back_inserter(raw_data), [](const float& x) { return uint8_t(x); });
	}

	auto res{ stbi_write_jpg(filepath.c_str(), w, h, num_channels, raw_data.data(), quality) };

	if (res == 0) {
		std::string msg{ "Writing the '" + filepath + "' image failed!" };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}
}
