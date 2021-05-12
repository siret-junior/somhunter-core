
#include "CollageRanker.h"

#include <filesystem>

#include "KeywordRanker.h"
#include "utils.h"

using namespace sh;

void CanvasQuery::emplace_back(const RelativeRect& rect, const std::string& text_query) {
	_subqueries.emplace_back(CanvasSubqueryText{ rect, text_query });
}

void CanvasQuery::emplace_back(const RelativeRect& rect, size_t bitmap_w, size_t bitmap_h, size_t num_channels,
                               uint8_t* bitmap_RGBA_data) {
	std::vector<float> image;

	// DO: RGBA_to_RGB
	size_t total_size{ bitmap_w * bitmap_h * num_channels };
	for (size_t j = 0; j < total_size; j += num_channels) {
		image.push_back(bitmap_RGBA_data[j + 0]);
		image.push_back(bitmap_RGBA_data[j + 1]);
		image.push_back(bitmap_RGBA_data[j + 2]);
	}

	// DO: resize_all(int 224, int 224)
	image = ImageManipulator::resize(image, bitmap_w, bitmap_h, CollageRanker::models_input_width,
	                                 CollageRanker::models_input_height, CollageRanker::models_num_channels);

	_subqueries.emplace_back(CanvasSubqueryBitmap{ rect, CollageRanker::models_input_width,
	                                               CollageRanker::models_input_height,
	                                               CollageRanker::models_num_channels, std::move(image) });
}

bool CanvasQuery::empty() const { return (size() == 0); }
size_t CanvasQuery::size() const { return _subqueries.size(); }

CollageRanker::CollageRanker(const Config& config, KeywordRanker* p_core) : _p_core{ p_core } {
	try {
		if (!std::filesystem::exists(config.model_ResNet_file)) {
			std::string msg{ "Unable to open file '" + config.model_ResNext_file + "'." };
			LOG_E(msg);
			throw std::runtime_error{ msg };
		}
		resnet152 = torch::jit::load(config.model_ResNet_file);
	} catch (const c10::Error& e) {
		std::string msg{ "Error openning ResNet model file: " + config.model_ResNet_file + "\n" + e.what() };
		msg.append(
		    "\n\nAre you on Windows? Have you forgotten to rerun CMake with appropriate CMAKE_BUILD_TYPE value? "
		    "Windows libtorch debug/release libs are NOT ABI compatible.");
		LOG_E(msg);
		throw std::runtime_error(msg);
	}

	try {
		if (!std::filesystem::exists(config.model_ResNext_file)) {
			std::string msg{ "Unable to open file '" + config.model_ResNext_file + "'." };
			msg.append(
			    "\n\nAre you on Windows? Have you forgotten to rerun CMake with appropriate CMAKE_BUILD_TYPE value? "
			    "Windows libtorch debug/release libs are NOT ABI compatible.");
			LOG_E(msg);
			throw std::runtime_error{ msg };
		}
		resnext101 = torch::jit::load(config.model_ResNext_file);
	} catch (const c10::Error& e) {
		std::string msg{ "Error openning ResNext model file: " + config.model_ResNext_file + "\n" + e.what() };
		LOG_E(msg);
		throw std::runtime_error(msg);
	}

	try {
		bias = torch::tensor(KeywordRanker::parse_float_vector(config.model_W2VV_img_bias, 2048));
	} catch (const c10::Error& e) {
		std::string msg{ "Error loading W2VV FC bias: " + config.model_W2VV_img_bias + "\n" + e.what() };
		LOG_E(msg);
		throw std::runtime_error(msg);
	}

	try {
		weights = torch::tensor(KeywordRanker::parse_float_vector(config.model_W2VV_img_weigths, 4096 * 2048))
		              .reshape({ 2048, 4096 })
		              .permute({ 1, 0 });
	} catch (const c10::Error& e) {
		std::string msg{ "Error loading W2VV FC weights: " + config.model_W2VV_img_weigths + "\n" + e.what() };
		LOG_E(msg);
		throw std::runtime_error(msg);
	}

	kw_pca_mat = torch::tensor(KeywordRanker::parse_float_vector(config.kw_PCA_mat_file,
	                                                             config.pre_PCA_features_dim * config.kw_PCA_mat_dim))
	                 .reshape({ (long long)(config.kw_PCA_mat_dim), (long long)(config.pre_PCA_features_dim) })
	                 .permute({ 1, 0 });
	kw_pca_mean_vec =
	    torch::tensor(KeywordRanker::parse_float_vector(config.kw_bias_vec_file, config.pre_PCA_features_dim));

	try {
		for (int i = 0; i < config.collage_regions; i++) {
			FeatureMatrix m = KeywordRanker::parse_float_matrix(
			    config.collage_region_file_prefix + std::to_string(i) + ".bin", 128, 0);
			region_data.push_back(m);
		}
	} catch (const c10::Error& e) {
		std::string msg{ "Error loading region data \n" };
		LOG_E(msg);
		throw std::runtime_error(msg);
	}
}

void CollageRanker::score(CanvasQuery& collage, ScoreModel& model, const DatasetFeatures& /*features*/,
                          const DatasetFrames& frames) {
	if (collage.size() > 0) {
		/*collage.RGBA_to_RGB();
		collage.resize_all(224, 224);*/

		at::Tensor tensor_features = get_features(collage);

		// Convert tensor to STD containered matrix
		auto collage_vectors{ to_std_matrix<float>(tensor_features) };
		// print_matrix(mat);

		// get best IOU regions for each image in collage
		auto regions = get_RoIs(collage);

		// get scores for all collage images and whole dataset of region
		std::vector<std::vector<float>> scores;
		for (std::size_t i = 0; i < collage_vectors.size(); i++)
			scores.push_back(score_image(collage_vectors[i], regions[i]));

		// size_t q0_beg{ collage.query_begins(0) };
		size_t q1_beg{ collage.query_begins(1) };

		// first query
		StdMatrix<float> s0(scores.begin(), scores.begin() + q1_beg);
		auto mean0 = average_scores(s0);

		std::vector<float> final_score;

		if (scores.begin() + q1_beg == scores.end())
			final_score = mean0;
		else  // second query
		{
			StdMatrix<float> s1(scores.begin() + q1_beg, scores.end());
			auto mean1 = average_scores(s1);

			// applied temporal query
			size_t window = 5;
			for (size_t i = 0; i < mean0.size(); i++) {
				auto begin_it = frames.get_frame_it(i);
				begin_it++;
				if (begin_it == frames.end()) break;

				auto end_it = begin_it;
				VideoId vid_ID = begin_it->video_ID;

				// move iterator window times or stop if end of video/file
				for (size_t j = 0; j < window; j++) {
					end_it++;
					if (end_it == frames.end() || end_it->video_ID != vid_ID) break;
				}

				// get min between begin_it and end_it from mean1
				float min;
				if (end_it == frames.end())
					min = *std::min_element(mean1.begin() + begin_it->frame_ID, mean1.end());
				else
					min = *std::min_element(mean1.begin() + begin_it->frame_ID, mean1.begin() + end_it->frame_ID);

				mean0[i] = mean0[i] * min;
			}

			final_score = mean0;
		}

		auto sorted_results{ KeywordRanker::sort_by_score(final_score) };
		KeywordRanker::report_results(sorted_results, frames);

		// Update the model
		for (auto&& [frame_ID, dist] : sorted_results) {
			model.adjust(frame_ID, std::exp(dist * -50));
		}

		model.normalize();
	}
}

// in 1st dim
at::Tensor CollageRanker::get_L2norm(at::Tensor data) {
	at::Tensor norm = torch::zeros({ data.sizes()[0], 1 });

	for (int64_t i = 0; i < data.sizes()[0]; i++) norm[i] = torch::sqrt(torch::sum(data[i] * data[i]));

	return norm;
}

// returns 2048 dim normed vector for each image in collage
at::Tensor CollageRanker::get_features(CanvasQuery& collage) {
	LOG_D("Extracting features\n");

	std::vector<torch::Tensor> tensors;
	std::vector<torch::Tensor> tensors_bitmap_norm;

	std::vector<torch::Tensor> features_text;

	float means[] = { 123.68, 116.779, 103.939 };
	torch::Tensor t_means = torch::from_blob(means, { 3 }).unsqueeze_(0).unsqueeze_(0);

	// get data, no adjustements for resnet, normed for resnext
	for (std::size_t i = 0; i < collage.size(); i++) {
		auto subquery{ collage[i] };

		// If bitmap
		if (std::holds_alternative<CanvasSubqueryBitmap>(subquery)) {
			CanvasSubqueryBitmap& subquery_bitmap{ std::get<CanvasSubqueryBitmap>(subquery) };

			at::Tensor tensor_imagex = torch::from_blob(subquery_bitmap.data(), { 224, 224, 3 }, at::kFloat);

			at::Tensor tensor_image = tensor_imagex - 0.0F;
			at::Tensor tensor_image_norm = tensor_imagex - t_means;

			tensor_image = tensor_image.permute({ 2, 0, 1 });

			tensor_image_norm = tensor_image_norm.permute({ 2, 0, 1 });

			tensors.push_back(tensor_image.unsqueeze_(0));

			tensors_bitmap_norm.push_back(tensor_image_norm.unsqueeze_(0));
		}
		// Else text
		else {
			CanvasSubqueryText subquery_text{ std::get<CanvasSubqueryText>(subquery) };
			// \todo Undummy
			// xoxo
			auto fea{ _p_core->get_text_query_feature(subquery_text.query()) };
			features_text.emplace_back(to_tensor<at::kFloat, float>(fea));
		}
	}
	at::Tensor batch = torch::cat(tensors, 0);
	at::Tensor batch_norm = torch::cat(tensors_bitmap_norm, 0);

	at::Tensor resnext101_feature = resnext101.forward({ batch_norm }).toTensor();
	at::Tensor resnet152_feature = resnet152.forward({ batch }).toTensor();
	at::Tensor features_bitmap = torch::cat({ resnext101_feature, resnet152_feature }, 1).to(torch::kFloat32).detach();

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
				mixed_tensor.emplace_back(features_bitmap[bitmap_i]);
				++text_i;
			}
		}
	}

	at::Tensor result_features = torch::stack(mixed_tensor, 0);
	std::cout << "result_features.shape: " << result_features.sizes() << std::endl;

	return result_features;
}

std::vector<std::size_t> CollageRanker::get_RoIs(CanvasQuery& collage) {
	std::vector<std::size_t> regions;
	for (std::size_t i = 0; i < collage.size(); i++) regions.push_back(get_RoI(collage[i]));
	return regions;
}

std::size_t CollageRanker::get_RoI(CanvasSubquery image) {
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

std::vector<float> CollageRanker::score_image(std::vector<float> feature, std::size_t region) {
	std::vector<float> score;
	for (size_t i = 0; i < region_data[region].size(); i++)
		score.push_back(utils::d_cos_normalized(feature, region_data[region][i]));
	return score;
}

std::vector<float> CollageRanker::average_scores(std::vector<std::vector<float>> scores) {
	size_t count = scores.size();
	std::vector<float> result;
	std::cout << "COUNT " << count << "\n";
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