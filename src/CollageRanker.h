#ifndef COLLAGE_RANKER_H_
#define COLLAGE_RANKER_H_

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <torch/script.h>
#include <torch/torch.h>
#include <cereal/types/complex.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include "log.h"

#include "Filters.h"
#include "ImageManipulator.h"
#include "KeywordRanker.h"
#include "common.h"

namespace sh {

template <typename DType>
std::vector<std::vector<DType>> to_std_matrix(const at::Tensor& tensor_features) {
	if (tensor_features.sizes().size() != 2) {
		throw std::runtime_error("Not 2x<dim> matrix.");
	}

	size_t num_rows{ size_t(tensor_features.sizes()[0]) };
	size_t num_cols{ size_t(tensor_features.sizes()[1]) };

	std::vector<std::vector<DType>> mat;
	mat.reserve(num_rows);

	// Iterate over the rows
	float* data_ptr = static_cast<float*>(tensor_features.data_ptr());
	for (std::size_t ir = 0; ir < num_rows; ++ir) {
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

class CollageRanker {
public:
	CollageRanker(const Config& config);
	void score(CanvasQuery&, ScoreModel& model, const DatasetFeatures& features, const DatasetFrames& frames);

private:
	torch::jit::script::Module resnet152;
	torch::jit::script::Module resnext101;
	torch::Tensor bias;
	torch::Tensor weights;
	torch::Tensor kw_pca_mat;
	torch::Tensor kw_pca_mean_vec;

	std::vector<FeatureMatrix> region_data;

	at::Tensor get_features(CanvasQuery&);
	at::Tensor get_L2norm(at::Tensor data);

	std::vector<std::size_t> get_RoIs(CanvasQuery& collage);
	std::size_t get_RoI(CanvasQuery::image);

	std::vector<float> score_image(std::vector<float> feature, std::size_t region);
	std::vector<float> average_scores(std::vector<std::vector<float>> scores);

	const std::vector<std::vector<float>> RoIs = {
		{ 0.0, 0.0, 1.0, 1.0 }, { 0.1, 0.2, 0.4, 0.6 }, { 0.3, 0.2, 0.4, 0.6 }, { 0.5, 0.2, 0.4, 0.6 },

		{ 0.0, 0.0, 0.4, 0.6 }, { 0.2, 0.0, 0.4, 0.6 }, { 0.4, 0.0, 0.4, 0.6 }, { 0.6, 0.0, 0.4, 0.6 },

		{ 0.0, 0.4, 0.4, 0.6 }, { 0.2, 0.4, 0.4, 0.6 }, { 0.4, 0.4, 0.4, 0.6 }, { 0.6, 0.4, 0.4, 0.6 },
	};
};

// This serves for default parameters of type Collage&
static CanvasQuery DEFAULT_COLLAGE{};

};  // namespace sh

#endif  // COLLAGE_RANKER_H_
