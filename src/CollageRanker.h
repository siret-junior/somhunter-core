#ifndef COLLAGE_RANKER_H_
#define COLLAGE_RANKER_H_

#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <cereal/types/complex.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <torch/script.h>
#include <torch/torch.h>

#include "log.h"

#include "ImageManipulator.h"
#include "KeywordRanker.h"

template<typename DType>
std::vector<std::vector<DType>>
to_std_matrix(const at::Tensor& tensor_features)
{
	if (tensor_features.sizes().size() != 2) {
		throw std::runtime_error("Not 2x<dim> matrix.");
	}

	size_t num_rows{ size_t(tensor_features.sizes()[0]) };
	size_t num_cols{ size_t(tensor_features.sizes()[1]) };

	std::vector<std::vector<DType>> mat;
	mat.reserve(num_rows);

	// Iterate over the rows
	float* data_ptr = static_cast<float*>(tensor_features.data_ptr());
	for (int ir = 0; ir < num_rows; ++ir) {
		std::vector<DType> row;
		row.assign(data_ptr, data_ptr + num_cols);
		data_ptr += num_cols;

		mat.emplace_back(std::move(row));
	}

	//// iterate through all elements
	// for (int i = 0; i < tensor_features.numel(); ++i)
	//{
	//	printf("%dth Element: %f\n", i, *data_ptr++);
	//}

	return mat;
}

class Collage
{
public:
	std::vector<float> lefts;
	std::vector<float> tops;
	std::vector<float> relative_heights;
	std::vector<float> relative_widths;
	std::vector<unsigned int> pixel_heights;
	std::vector<unsigned int> pixel_widths;

	// format from js: [RGBARGBA.....]
	std::vector<std::vector<float>> images;

	// temporal query delimiter
	int break_point = 0;
	int channels = 0;

	void print() const;
	void RGBA_to_BGR();
	void resize_all(int W = 224, int H = 224);
	void save_all(const std::string& prefix = "");

	/**
	 * This allows portable binary serialization of Collage instances to files.
	 *
	 * by Cereal header-only lib
	 * https://uscilab.github.io/cereal/quickstart.html
	 * https://uscilab.github.io/cereal/stl_support.html
	 */
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(lefts,
		        tops,
		        relative_heights,
		        relative_widths,
		        pixel_heights,
		        pixel_widths,
		        images,
		        break_point,
		        channels);
	}
};

class CollageRanker
{
public:
	CollageRanker(const Config& config);
	void score(Collage&, ScoreModel& model, const DatasetFeatures& features, const DatasetFrames& frames);

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

// This serves for default parameters of type Collage&
static Collage DEFAULT_COLLAGE{};

#endif // COLLAGE_RANKER_H_
