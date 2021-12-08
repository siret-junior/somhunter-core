/* This file is part of SOMHunter.
 *
 * Copyright (C) 2021 František Mejzlík <frankmejzlik@gmail.com>
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

#include "canvas-query-ranker.h"

#include <filesystem>

#include "keyword-ranker.h"
#include "scores.h"
#include "utils.hpp"

using namespace sh;

#ifdef SHLOG_D  // Turn off `SHLOG_D` in this TU
#	undef SHLOG_D
#	define SHLOG_D(x) _dont_write_log_err
#endif

CanvasQueryRanker::CanvasQueryRanker(const Settings& _settings, KeywordRanker* p_core)
    : _p_core{ p_core }, _loaded{ false } {
	SHLOG_D("Initializing CanvasQueryRanker...");

	// Check if we have subregions data
	if (_settings.datasets.primary_features.collage_region_file_prefix.empty()) {
		SHLOG_W("No subregion features! Running without canvas queries.");
		_loaded = false;
		return;
	}

	try {
		if (!_settings.models.model_ResNet_file.empty()) {
			if (!std::filesystem::exists(_settings.models.model_ResNet_file)) {
				std::string msg{ "Unable to open file '" + _settings.models.model_ResNet_file + "'." };
				SHLOG_E(msg);
				throw std::runtime_error{ msg };
			}
			resnet152 = torch::jit::load(_settings.models.model_ResNet_file);
			SHLOG_S("ResNet model loaded from  '" << _settings.models.model_ResNet_file << "'.");
		} else {
			SHLOG_W("ResNet model is ignored!");
		}
	} catch (const c10::Error& e) {
		std::string msg{ "Error openning ResNet model file: " + _settings.models.model_ResNet_file + "\n" + e.what() };
		msg.append(
		    "\n\nAre you on Windows? Have you forgotten to rerun CMake with appropriate CMAKE_BUILD_TYPE value? "
		    "Windows libtorch debug/release libs are NOT ABI compatible.");
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	try {
		if (!_settings.models.model_ResNext_file.empty()) {
			if (!std::filesystem::exists(_settings.models.model_ResNext_file)) {
				std::string msg{ "Unable to open file '" + _settings.models.model_ResNext_file + "'." };
				msg.append(
				    "\n\nAre you on Windows? Have you forgotten to rerun CMake with appropriate CMAKE_BUILD_TYPE "
				    "value? "
				    "Windows libtorch debug/release libs are NOT ABI compatible.");
				SHLOG_E(msg);
				throw std::runtime_error{ msg };
			}
			resnext101 = torch::jit::load(_settings.models.model_ResNext_file);
			SHLOG_S("ResNext model loaded from  '" << _settings.models.model_ResNext_file << "'.");
		} else {
			SHLOG_W("ResNext model is ignored!");
		}
	} catch (const c10::Error& e) {
		std::string msg{ "Error openning ResNext model file: " + _settings.models.model_ResNext_file + "\n" +
			             e.what() };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	try {
		bias = torch::tensor(KeywordRanker::parse_float_vector(_settings.models.model_W2VV_img_bias, 2048));
		SHLOG_S("W2VV bias loaded from '" << _settings.models.model_ResNext_file << "' with dimension (2048).");
	} catch (const c10::Error& e) {
		std::string msg{ "Error loading W2VV FC bias: " + _settings.models.model_W2VV_img_bias + "\n" + e.what() };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	try {
		weights = torch::tensor(KeywordRanker::parse_float_vector(_settings.models.model_W2VV_img_weigths, 4096 * 2048))
		              .reshape({ 2048, 4096 })
		              .permute({ 1, 0 });
		SHLOG_S("W2VV FC weights loaded from '" << _settings.models.model_W2VV_img_weigths
		                                        << "' with dimension (4096, 2048).");
	} catch (const c10::Error& e) {
		std::string msg{ "Error loading W2VV FC weights: " + _settings.models.model_W2VV_img_weigths + "\n" +
			             e.what() };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	kw_pca_mat =
	    torch::tensor(KeywordRanker::parse_float_vector(_settings.datasets.primary_features.kw_PCA_mat_file,
	                                                    _settings.datasets.primary_features.pre_PCA_features_dim *
	                                                        _settings.datasets.primary_features.kw_PCA_mat_dim))
	        .reshape({ (long long)(_settings.datasets.primary_features.kw_PCA_mat_dim),
	                   (long long)(_settings.datasets.primary_features.pre_PCA_features_dim) })
	        .permute({ 1, 0 });
	SHLOG_S("Loaded PCA matrix from '" << _settings.datasets.primary_features.kw_PCA_mat_file << "' with dimension ("
	                                   << _settings.datasets.primary_features.pre_PCA_features_dim << " ,"
	                                   << _settings.datasets.primary_features.kw_PCA_mat_dim << ").");
	kw_pca_mean_vec =
	    torch::tensor(KeywordRanker::parse_float_vector(_settings.datasets.primary_features.kw_bias_vec_file,
	                                                    _settings.datasets.primary_features.pre_PCA_features_dim));

	SHLOG_S("Loaded PCA mean vector from '" << _settings.datasets.primary_features.kw_bias_vec_file
	                                        << "' with dimension ("
	                                        << _settings.datasets.primary_features.pre_PCA_features_dim << ").");

	try {
		for (std::size_t i = 0; i < _settings.datasets.primary_features.collage_regions; i++) {
			FeatureMatrix m = KeywordRanker::parse_float_matrix(
			    _settings.datasets.primary_features.collage_region_file_prefix + std::to_string(i) + ".bin", 128, 0);
			region_data.push_back(m);
		}
		SHLOG_S("Loaded " << _settings.datasets.primary_features.collage_regions << " from prefix  '"
		                  << _settings.datasets.primary_features.collage_region_file_prefix << "'.");

	} catch (const c10::Error& e) {
		std::string msg{ "Error loading region data \n" };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	_loaded = true;
	SHLOG_S("CanvasQueryRanker initialized.");
}

void CanvasQueryRanker::score(const CanvasQuery& canvas_query, ScoreModel& model, size_t temporal,
                              UsedTools& used_tools, const PrimaryFrameFeatures& /*features*/,
                              const DatasetFrames& /*_dataset_frames*/) {
	if (!_loaded) {
		SHLOG_W("Called CanvasQueryRanker::score without available subregion data. Leaving the scores intact.");
		return;
	}

	if (canvas_query.size() > 0) {
		at::Tensor tensor_features = get_features(canvas_query, used_tools);

		// Convert tensor to STD containered matrix
		auto collage_vectors{ to_std_matrix<float>(tensor_features) };

		// get best IOU regions for each image in collage
		auto regions = get_RoIs(canvas_query);

		// get scores for all collage images and whole dataset of region
		std::vector<std::vector<float>> scores;
		for (std::size_t i = 0; i < collage_vectors.size(); i++)
			scores.push_back(score_image(collage_vectors[i], regions[i]));

		auto final_score = average_scores(scores);
		auto sorted_results{ ScoreModel::sort_by_score(final_score) };

		// KeywordRanker::report_results(sorted_results, _dataset_frames);

		// Update the model
		for (size_t i = 0; i < final_score.size(); ++i) {
			model.adjust(temporal, i, final_score[i]);
		}
	}
}

// in 1st dim
at::Tensor CanvasQueryRanker::get_L2norm(const at::Tensor& _data) const {
	at::Tensor norm = torch::zeros({ _data.sizes()[0], 1 });

	for (int64_t i = 0; i < _data.sizes()[0]; i++) norm[i] = torch::sqrt(torch::sum(_data[i] * _data[i]));

	return norm;
}

// returns 2048 dim normed vector for each image in collage
at::Tensor CanvasQueryRanker::get_features(const CanvasQuery& collage, UsedTools& used_tools) {
	SHLOG_D("Extracting features");

	torch::NoGradGuard no_grad;
	std::vector<torch::Tensor> tensors;
	std::vector<torch::Tensor> tensors_bitmap_norm;

	std::vector<torch::Tensor> tensors_text;
	at::Tensor features_text;
	at::Tensor features_bitmap;

	float means[] = { 123.68, 116.779, 103.939 };
	torch::Tensor t_means = torch::from_blob(means, { 3 }).unsqueeze_(0).unsqueeze_(0);

	// get data, no adjustements for resnet, normed for resnext
	for (std::size_t i = 0; i < collage.size(); i++) {
		auto subquery{ collage[i] };

		// If bitmap
		if (std::holds_alternative<CanvasSubqueryBitmap>(subquery)) {
			// Set used tool
			used_tools.canvas_bitmap_used = true;

			CanvasSubqueryBitmap& subquery_bitmap{ std::get<CanvasSubqueryBitmap>(subquery) };
			auto scaled_bitmap{ subquery_bitmap.get_scaled_bitmap(224, 224) };

			/*ImageManipulator::store_PNG("pre-scale-bitmap.png", subquery_bitmap.data(),
			subquery_bitmap.width_pixels(), subquery_bitmap.height_pixels(), 3);
			ImageManipulator::store_PNG("post-scale-bitmap.png", scaled_bitmap, 224, 224, 3);*/

			// !!!
			at::Tensor tensor_imagex = torch::from_blob(scaled_bitmap.data(), { 224, 224, 3 }, at::kByte);
			// This is not needed, torch seems to accept the above data
			/*auto ddata{ImageManipulator::to_float32(scaled_bitmap)};
			at::Tensor tensor_imagex = torch::from_blob(ddata.data(), { 224, 224, 3 }, at::kFloat);*/

			at::Tensor tensor_image = tensor_imagex - 0.0F;
			at::Tensor tensor_image_norm = tensor_imagex - t_means;

			tensor_image = tensor_image.permute({ 2, 0, 1 });

			tensor_image_norm = tensor_image_norm.permute({ 2, 0, 1 });

			tensors.push_back(tensor_image.unsqueeze_(0));

			tensors_bitmap_norm.push_back(tensor_image_norm.unsqueeze_(0));
		}
		// Else text
		else {
			// Set used tool
			used_tools.canvas_text_used = true;

			CanvasSubqueryText subquery_text{ std::get<CanvasSubqueryText>(subquery) };
			auto fea{ _p_core->get_text_query_feature(subquery_text.query()) };
			tensors_text.emplace_back(to_tensor<at::kFloat, float>(fea));
		}
	}

	// Stack the TEXT features into one tensor
	if (tensors_text.size() > 0) {
		features_text = torch::stack(tensors_text, 0);
	}

	// Stack the BITMAP features into one tensor
	if (tensors.size() > 0) {
		at::Tensor batch = torch::cat(tensors, 0);
		at::Tensor batch_norm = torch::cat(tensors_bitmap_norm, 0);

		auto resnext101_forward = resnext101.forward({ batch_norm });
		auto resnet125_forward = resnet152.forward({ batch });
		at::Tensor resnext101_feature = resnext101_forward.toTensor();
		at::Tensor resnet152_feature = resnet125_forward.toTensor();
		features_bitmap = torch::cat({ resnext101_feature, resnet152_feature }, 1).to(torch::kFloat32).detach();

		// squeeze 4096 to 2048
		features_bitmap = features_bitmap.unsqueeze(0).permute({ 1, 0, 2 });
		features_bitmap = torch::tanh(torch::matmul(features_bitmap, weights).squeeze(1) + bias);

		// norm
		features_bitmap = torch::div(features_bitmap, get_L2norm(features_bitmap));

		// PCA
		features_bitmap = features_bitmap - kw_pca_mean_vec;
		features_bitmap = features_bitmap.unsqueeze(0).permute({ 1, 0, 2 });
		features_bitmap = torch::matmul(features_bitmap, kw_pca_mat).squeeze(1);

		// norm
		features_bitmap = torch::div(features_bitmap, get_L2norm(features_bitmap));
	}
	// --------------------------------------------
	// Merge text & bitmap features into one matrix
	std::vector<torch::Tensor> mixed_tensor;
	{
		size_t bitmap_i{ 0 };
		size_t text_i{ 0 };
		for (std::size_t i = 0; i < collage.size(); i++) {
			auto subquery{ collage[i] };

			// If bitmap
			if (std::holds_alternative<CanvasSubqueryBitmap>(subquery)) {
				mixed_tensor.emplace_back(features_bitmap[bitmap_i]);
				++bitmap_i;
			}
			// Else text
			else {
				mixed_tensor.emplace_back(features_text[text_i]);
				++text_i;
			}
		}
	}

	// Final stack
	at::Tensor result_features = torch::stack(mixed_tensor, 0);
	// std::cout << "result_features.shape: " << result_features.sizes() << std::endl;
	// std::cout << result_features << std::endl;

	return result_features;
}

std::vector<std::size_t> CanvasQueryRanker::get_RoIs(const CanvasQuery& collage) const {
	std::vector<std::size_t> regions;
	for (std::size_t i = 0; i < collage.size(); i++) regions.push_back(get_RoI(collage[i]));
	return regions;
}

std::size_t CanvasQueryRanker::get_RoI(const CanvasSubquery& image) const {
	RelativeRect rect{ std::visit(
		overloaded{
		    [](auto sq) { return sq.rect(); },
		},
		image) };

	std::vector<float> iou;
	for (std::size_t i = 0; i < RoIs.size(); i++) {
		auto& roi = RoIs[i];
		auto int_l = std::max(rect.left, roi[0]);
		auto int_t = std::max(rect.top, roi[1]);
		auto int_r = std::min(rect.left + rect.width_norm(), roi[0] + roi[2]);
		auto int_b = std::min(rect.top + rect.height_norm(), roi[1] + roi[3]);
		if (int_r < int_l || int_b < int_t)
			iou.push_back(0);
		else {
			auto intersection = (int_r - int_l) * (int_b - int_t);
			auto uni = ((rect.width_norm() * rect.height_norm()) + (roi[2] * roi[3])) - intersection;
			iou.push_back(intersection / uni);
		}
	}
	return std::distance(iou.begin(), std::max_element(iou.begin(), iou.end()));
}

std::vector<float> CanvasQueryRanker::score_image(const std::vector<float>& feature, std::size_t region) const {
	std::vector<float> score;
	for (size_t i = 0; i < region_data[region].size(); i++)
		score.push_back(d_cos_normalized(feature.data(), region_data[region][i].data(), feature.size()) / 2);
	return score;
}

std::vector<float> CanvasQueryRanker::average_scores(const std::vector<std::vector<float>>& scores) const {
	size_t count = scores.size();
	std::vector<float> result;

	if (count == 0) return result;

	for (size_t i = 0; i < scores[0].size(); i++) {
		float sum = 0;
		for (size_t j = 0; j < count; j++) {
			sum += scores[j][i];
		}
		result.push_back(sum / count);
	}
	return result;
}
