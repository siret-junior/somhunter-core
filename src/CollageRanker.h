#include <vector>
#include <cstdint>
#include <torch/torch.h>
#include <torch/script.h>
#include <torch/linalg.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "log.h"
#include <iostream>
#include "ImageManipulator.h"
#include "KeywordRanker.h"


#ifndef COLLAGE
#define COLLAGE

class Collage
{
public:

    std::vector<float> lefts;
	std::vector<float> tops;
	std::vector<float> relative_heights;
	std::vector<float> relative_widths;
	std::vector<unsigned int> pixel_heights;
	std::vector<unsigned int> pixel_widths;

	//format from js: [RGBARGBA.....]
	std::vector<std::vector<float>> images;

	//temporal query delimiter
    int break_point = 0;
	int channels = 0;

	void print() const
	{
		std::cout << "COLLAGE BEGIN\n";
		std::cout << "Images: " << images.size() << "\n";
		std::cout << "Break: " << break_point << "\n\n";
		for(size_t i = 0; i < images.size(); i++)
		{
			std::cout << "\t#" << i << ":\n";
			std::cout << "\t\tLeft: " << lefts[i] << ":\n";
			std::cout << "\t\tTop: " << tops[i] << ":\n";
			std::cout << "\t\tHeight: " << relative_heights[i] << ":\n";
			std::cout << "\t\tWidth: " << relative_widths[i] << ":\n";
			std::cout << "\t\tP_Height: " << pixel_heights[i] << ":\n";
			std::cout << "\t\tP_Width: " << pixel_widths[i] << ":\n";
			std::cout << "\t\tSize: " << images[i].size() << ":\n";
			std::cout << "\n";
		}
		std::cout << "COLLAGE END\n";
	}

	void RGBA_to_BGR()
	{
		if(channels == 3)
			return;

		std::vector<std::vector<float>> rgb_images;
		
		for(size_t i = 0; i < images.size(); i++)
		{
			std::vector<float> image;
			for(size_t j = 0; j < images[i].size(); j+=4)
			{
				image.push_back(images[i][j+2]);
				image.push_back(images[i][j+1]);
				image.push_back(images[i][j+0]);
			}
			rgb_images.push_back(image);
		}
		images = rgb_images;
		channels = 3;
	}

	void resize_all(int W = 224, int H = 224)
	{
		std::vector<std::vector<float>> resized_images;
		for(size_t i = 0; i < images.size(); i++)
		{
			std::vector<float> image = ImageManipulator::resize(images[i], pixel_widths[i], pixel_heights[i], W, H, channels);
			pixel_widths[i] = W;
			pixel_heights[i] = H;
			resized_images.push_back(image);
		}
		images = resized_images;
	}

	void save_all(std::string prefix = "")
	{
		// expects RGB [0,1] 
		for(size_t i = 0; i < images.size(); i++)
		{
			ImageManipulator::store_jpg(prefix + "im"+std::to_string(i)+".jpg",
								images[i], pixel_widths[i], pixel_heights[i], 100, channels);
		}
	}
};

class CollageRanker
{
	public:
		CollageRanker(const Config &config);
		void score(Collage&);

	private:
		torch::jit::script::Module resnet152;
		torch::jit::script::Module resnext101;
		torch::Tensor bias;
    	torch::Tensor weights;
		torch::Tensor kw_pca_mat;
		torch::Tensor kw_pca_mean_vec;

		at::Tensor get_features(Collage&);
		at::Tensor get_L2norm(at::Tensor data);
};

#endif
