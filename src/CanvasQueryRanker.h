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

#include "ImageManipulator.h"
#include "KeywordRanker.h"
#include "common.h"
#include "query_types.h"

namespace sh {

class SomHunter;

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

	return mat;
}

template <c10::ScalarType TensorDType_ = at::kFloat, typename OrigDType_ = float>
at::Tensor to_tensor(std::vector<OrigDType_>& orig_vec) {
	do_assert_debug(orig_vec.size() > 0, "Vector cannot be empty.");
	// SHLOG_D("shape = (" << orig_vec.size() << ")");

	return torch::tensor(orig_vec, TensorDType_);
}

template <c10::ScalarType TensorDType_ = at::kFloat, typename OrigDType_ = float>
at::Tensor to_tensor(std::vector<std::vector<OrigDType_>>& orig_mat) {
	do_assert_cond(orig_mat.size() > 0, "Matrix cannot be empty.");
	// SHLOG_D("shape = (" << orig_vec.size() << ", " << orig_vec.front(0).size() << ")");

	std::vector<at::Tensor> meta;
	meta.reserve(orig_mat.size());

	for (auto&& vec : orig_mat) {
		meta.emplace_back(torch::tensor(vec.data(), TensorDType_));
	}

	return torch::cat(meta, 0);
}

class CanvasQueryRanker {
	KeywordRanker* _p_core;
	bool _loaded;

	torch::jit::script::Module resnet152;
	torch::jit::script::Module resnext101;
	torch::Tensor bias;
	torch::Tensor weights;
	torch::Tensor kw_pca_mat;
	torch::Tensor kw_pca_mean_vec;

	const std::vector<std::vector<float>> RoIs = {
		{ 0.0, 0.0, 1.0, 1.0 }, { 0.1, 0.2, 0.4, 0.6 }, { 0.3, 0.2, 0.4, 0.6 }, { 0.5, 0.2, 0.4, 0.6 },

		{ 0.0, 0.0, 0.4, 0.6 }, { 0.2, 0.0, 0.4, 0.6 }, { 0.4, 0.0, 0.4, 0.6 }, { 0.6, 0.0, 0.4, 0.6 },

		{ 0.0, 0.4, 0.4, 0.6 }, { 0.2, 0.4, 0.4, 0.6 }, { 0.4, 0.4, 0.4, 0.6 }, { 0.6, 0.4, 0.4, 0.6 },
	};

public:
	static const size_t models_input_width{ 224 };
	static const size_t models_input_height{ 224 };
	static const size_t models_num_channels{ 3 };

	CanvasQueryRanker(const Config& config, KeywordRanker* p_core);
	void score(const CanvasQuery&, ScoreModel& model, size_t temporal, const DatasetFeatures& features,
	           const DatasetFrames& frames);

private:
	std::vector<FeatureMatrix> region_data;

	at::Tensor get_features(const CanvasQuery&);
	at::Tensor get_L2norm(const at::Tensor& data) const;

	std::vector<std::size_t> get_RoIs(const CanvasQuery& collage) const;
	std::size_t get_RoI(const CanvasSubquery& image) const;

	std::vector<float> score_image(const std::vector<float>& feature, std::size_t region) const;
	std::vector<float> average_scores(const std::vector<std::vector<float>>& scores) const;
};

// This serves for default parameters of type Collage&
static CanvasQuery DEFAULT_COLLAGE{};

};  // namespace sh

#endif  // COLLAGE_RANKER_H_
