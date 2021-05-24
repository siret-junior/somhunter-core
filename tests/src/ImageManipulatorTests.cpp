
#include <gtest/gtest.h>

#include "ImageManipulator.h"
#include "config-tests.h"

using namespace sh;

#define SUITE_NAME ImageManipulatorTests

TEST(SUITE_NAME, cvMat__correct_PNG_loaded) {
	{
		cv::Mat cv_img{ ImageManipulator::load_image<cv::Mat>(TEST_PNGS[0]) };
		EXPECT_EQ(cv_img.rows, 200);
		EXPECT_EQ(cv_img.cols, 100);
		EXPECT_EQ(cv_img.channels(), 3);

		// auto c{ cv_img.channels() };

		// RED
		// BGR: (0, 0, 255)
		cv::Vec3b top_left = cv_img.at<cv::Vec3b>(0, 0);
		EXPECT_EQ(top_left[0], 0);
		EXPECT_EQ(top_left[1], 0);
		EXPECT_EQ(top_left[2], 255);

		// RED
		// BGR: (0, 0, 255)
		cv::Vec3b bottom_right = cv_img.at<cv::Vec3b>(cv_img.rows - 1, cv_img.cols - 1);
		EXPECT_EQ(bottom_right[0], 0);
		EXPECT_EQ(bottom_right[1], 0);
		EXPECT_EQ(bottom_right[2], 255);
	}
	{
		cv::Mat cv_img{ ImageManipulator::load_image<cv::Mat>(TEST_PNGS[1]) };
		EXPECT_EQ(cv_img.rows, 200);
		EXPECT_EQ(cv_img.cols, 100);
		EXPECT_EQ(cv_img.channels(), 4);

		// RED
		// BGR: (0, 0, 255)
		cv::Vec4b top_left = cv_img.at<cv::Vec4b>(0, 0);
		EXPECT_EQ(top_left[0], 0);
		EXPECT_EQ(top_left[1], 0);
		EXPECT_EQ(top_left[2], 255);
		EXPECT_EQ(top_left[3], 255);

		// RED
		// BGRA: (0, 0, 255)
		cv::Vec4b bottom_right = cv_img.at<cv::Vec4b>(cv_img.rows - 1, cv_img.cols - 1);
		EXPECT_EQ(bottom_right[0], 0);
		EXPECT_EQ(bottom_right[1], 0);
		EXPECT_EQ(bottom_right[2], 255);
		EXPECT_EQ(bottom_right[3], 255);
	}
	{
		cv::Mat cv_img{ ImageManipulator::load_image<cv::Mat>(TEST_PNGS[2]) };
		EXPECT_EQ(cv_img.rows, 200);
		EXPECT_EQ(cv_img.cols, 100);
		EXPECT_EQ(cv_img.channels(), 1);
	}
	{
		cv::Mat cv_img{ ImageManipulator::load_image<cv::Mat>(TEST_PNGS[3]) };
		EXPECT_EQ(cv_img.rows, 200);
		EXPECT_EQ(cv_img.cols, 100);
		EXPECT_EQ(cv_img.channels(), 4);
	}
}

TEST(SUITE_NAME, BitmapImage_uint8_t__correct_JPEGs_loaded) {
	{
		BitmapImage<uint8_t> std_img_u8{ ImageManipulator::load_image<BitmapImage<uint8_t>>(TEST_JPEGS[0]) };
		EXPECT_EQ(std_img_u8.w, 320);
		EXPECT_EQ(std_img_u8.h, 180);
		EXPECT_EQ(std_img_u8.num_channels, 3);
	}
}
