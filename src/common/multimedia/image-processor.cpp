
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

#include "image-processor.h"

// Let STB to put it's implementation inside this TU
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

using namespace sh;

std::vector<uint8_t> ImageManipulator::resize(const std::vector<std::uint8_t>& in, size_t /*orig_w*/, size_t orig_h,
                                              size_t new_w, size_t new_h, size_t num_channels)
{
	std::vector<uint8_t> out(new_w * new_h * num_channels, 0);

	cv::Mat bitmap_cv_RGB{ in };
	bitmap_cv_RGB = bitmap_cv_RGB.reshape(3, int(orig_h));

	// cvtColor(bitmap_cv_RGB, bitmap_cv_RGB, cv::COLOR_RGB2BGR); //< Do not bother with converting the space

	cv::Mat scaled_bitmap;
	cv::resize(bitmap_cv_RGB, scaled_bitmap, cv::Size(224, 224), 0, 0, cv::INTER_AREA);

	// ImageManipulator::show_image(scaled_bitmap);
	// cvtColor(bitmap_cv_RGB, bitmap_cv_RGB, cv::COLOR_RGB2BGR); //< Just skip this, just resize

	// Copy the resized data out
	scaled_bitmap = scaled_bitmap.reshape(1, scaled_bitmap.total() * scaled_bitmap.channels());
	scaled_bitmap.copyTo(out);
	return out;
}

std::vector<float> sh::ImageManipulator::to_float32(const std::vector<std::uint8_t>& in)
{
	std::vector<float> out;
	out.reserve(in.size());

	std::transform(in.begin(), in.end(), std::back_inserter(out), [](std::uint8_t x) { return static_cast<float>(x); });

	return out;
}

std::vector<std::uint8_t> sh::ImageManipulator::to_uint8(const std::vector<float>& in)
{
	std::vector<std::uint8_t> out;
	out.reserve(in.size());

	std::transform(in.begin(), in.end(), std::back_inserter(out), [](float x) { return static_cast<std::uint8_t>(x); });

	return out;
}

template <>
cv::Mat ImageManipulator::load_image<cv::Mat>(const std::string& filepath)
{
	using namespace cv;
	Mat image = imread(filepath, IMREAD_COLOR);
	return image;
}

template <>
BitmapImage<float> ImageManipulator::load_image<BitmapImage<float>>(const std::string& filepath)
{
	cv::Mat cv_img{ ImageManipulator::load_image<cv::Mat>(filepath) };

	// Convert to float32 matrix
	cv::Mat cv_fimg;
	cv_img.convertTo(cv_fimg, CV_32F);
	// int total_size{ cv_fimg.rows * cv_fimg.cols * cv_fimg.channels() };

	BitmapImage<float> b_img;
	b_img.w = cv_fimg.cols;
	b_img.h = cv_fimg.rows;
	b_img.num_channels = cv_fimg.channels();

	cv::Mat flat = cv_fimg.reshape(1, cv_fimg.total() * cv_fimg.channels());
	flat.copyTo(b_img.data);

	return b_img;
}

template <>
BitmapImage<uint8_t> ImageManipulator::load_image<BitmapImage<uint8_t>>(const std::string& filepath)
{
	cv::Mat cv_img{ ImageManipulator::load_image<cv::Mat>(filepath) };

	// Convert to uint8_t matrix
	cv::Mat cv_fimg;
	cv_img.convertTo(cv_fimg, CV_8U);
	// int total_size{ cv_fimg.rows * cv_fimg.cols * cv_fimg.channels() };

	BitmapImage<uint8_t> b_img;
	b_img.w = cv_fimg.cols;
	b_img.h = cv_fimg.rows;
	b_img.num_channels = cv_fimg.channels();

	cv::Mat flat = cv_fimg.reshape(1, cv_fimg.total() * cv_fimg.channels());
	flat.copyTo(b_img.data);
	return b_img;
}

void ImageManipulator::store_JPEG(const std::string& filepath, const std::vector<std::uint8_t>& in, size_t w, size_t h,
                                  size_t quality, size_t num_channels)
{
	std::vector<uint8_t> raw_data;
	raw_data.reserve(in.size());

	std::transform(in.begin(), in.end(), std::back_inserter(raw_data), [](const float& x) { return uint8_t(x); });

	auto res{ stbi_write_jpg(filepath.c_str(), w, h, num_channels, raw_data.data(), quality) };

	if (res == 0) {
		std::string msg{ "Writing the '" + filepath + "' image failed!" };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}
}