#include "CollageRanker.h"
#include "utils.h"

void
Collage::print() const
{
	std::cout << "COLLAGE BEGIN\n";
	std::cout << "Images: " << images.size() << "\n";
	std::cout << "Break: " << break_point << "\n\n";
	for (size_t i = 0; i < images.size(); i++) {
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

void
Collage::RGBA_to_RGB()
{
	if (channels == 3)
		return;

	std::vector<std::vector<float>> rgb_images;

	for (size_t i = 0; i < images.size(); i++) {
		std::vector<float> image;
		for (size_t j = 0; j < images[i].size(); j += 4) {
			image.push_back(images[i][j + 0]);
			image.push_back(images[i][j + 1]);
			image.push_back(images[i][j + 2]);
		}
		rgb_images.push_back(image);
	}
	images = rgb_images;
	channels = 3;
}

void
Collage::resize_all(int W, int H)
{
	std::vector<std::vector<float>> resized_images;
	for (size_t i = 0; i < images.size(); i++) {
		std::vector<float> image =
		  ImageManipulator::resize(images[i], pixel_widths[i], pixel_heights[i], W, H, channels);
		pixel_widths[i] = W;
		pixel_heights[i] = H;
		resized_images.push_back(image);
	}
	images = resized_images;
}

void
Collage::save_all(const std::string& prefix)
{
	// expects RGB [0,1]
	for (size_t i = 0; i < images.size(); i++) {
		ImageManipulator::store_jpg(prefix + "im" + std::to_string(i) + ".jpg",
		                            images[i],
		                            pixel_widths[i],
		                            pixel_heights[i],
		                            100,
		                            channels);
	}
}

// --------------------------------

CollageRanker::CollageRanker(const Config& config)
{

	try {
		resnet152 = torch::jit::load(config.model_ResNet_file);
	} catch (const c10::Error& e) {
		std::string msg{ "Error openning ResNet model file: " + config.model_ResNet_file + "\n" + e.what() };
		warn_d(msg);
		throw std::runtime_error(msg);
	}

	try {
		resnext101 = torch::jit::load(config.model_ResNext_file);
	} catch (const c10::Error& e) {
		std::string msg{ "Error openning ResNext model file: " + config.model_ResNext_file + "\n" + e.what() };
		warn_d(msg);
		throw std::runtime_error(msg);
	}

	try {
		bias = torch::tensor(KeywordRanker::parse_float_vector(config.model_W2VV_img_bias, 2048));
	} catch (const c10::Error& e) {
		std::string msg{ "Error loading W2VV FC bias: " + config.model_W2VV_img_bias + "\n" + e.what() };
		warn_d(msg);
		throw std::runtime_error(msg);
	}

	try {
		weights = torch::tensor(KeywordRanker::parse_float_vector(config.model_W2VV_img_weigths, 4096 * 2048))
		            .reshape({ 2048, 4096 })
		            .permute({ 1, 0 });
	} catch (const c10::Error& e) {
		std::string msg{ "Error loading W2VV FC weights: " + config.model_W2VV_img_weigths + "\n" + e.what() };
		warn_d(msg);
		throw std::runtime_error(msg);
	}

	kw_pca_mat = torch::tensor(KeywordRanker::parse_float_vector(
	                             config.kw_PCA_mat_file, config.pre_PCA_features_dim * config.kw_PCA_mat_dim))
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
		warn_d(msg);
		throw std::runtime_error(msg);
	}
}

void
CollageRanker::score(Collage& collage,
                     ScoreModel& model,
                     const DatasetFeatures& /*features*/,
                     const DatasetFrames& frames)
{
	if (collage.images.size() > 0) {
		collage.RGBA_to_RGB();
		collage.resize_all(224, 224);

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

		// first query
		StdMatrix<float> s0(scores.begin(), scores.begin() + collage.break_point);
		auto mean0 = average_scores(s0);

		std::vector<float> final_score;

		if (scores.begin() + collage.break_point == scores.end())
			final_score = mean0;
		else // second query
		{
			StdMatrix<float> s1(scores.begin() + collage.break_point, scores.end());
			auto mean1 = average_scores(s1);

			// applied temporal query
			size_t window = 5;
			for (size_t i = 0; i < mean0.size(); i++) {
				auto begin_it = frames.get_frame_it(i);
				begin_it++;
				if (begin_it == frames.end())
					break;

				auto end_it = begin_it;
				VideoId vid_ID = begin_it->video_ID;

				// move iterator window times or stop if end of video/file
				for (size_t j = 0; j < window; j++) {
					end_it++;
					if (end_it == frames.end() || end_it->video_ID != vid_ID)
						break;
				}

				// get min between begin_it and end_it from mean1
				float min;
				if (end_it == frames.end())
					min = *std::min_element(mean1.begin() + begin_it->frame_ID, mean1.end());
				else
					min = *std::min_element(mean1.begin() + begin_it->frame_ID,
					                        mean1.begin() + end_it->frame_ID);

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
at::Tensor
CollageRanker::get_L2norm(at::Tensor data)
{
	at::Tensor norm = torch::zeros({ data.sizes()[0], 1 });

	for (int64_t i = 0; i < data.sizes()[0]; i++)
		norm[i] = torch::sqrt(torch::sum(data[i] * data[i]));

	return norm;
}

// returns 2048 dim normed vector for each image in collage
at::Tensor
CollageRanker::get_features(Collage& collage)
{
	debug_d("Extracting features\n");

	std::vector<torch::Tensor> tensors;
	std::vector<torch::Tensor> tensors_norm;

	float means[] = { 123.68, 116.779, 103.939 };
	torch::Tensor t_means = torch::from_blob(means, { 3 }).unsqueeze_(0).unsqueeze_(0);

	// get data, no adjustements for resnet, normed for resnext
	for (std::size_t i = 0; i < collage.images.size(); i++) {

		at::Tensor tensor_image = torch::from_blob(collage.images[i].data(), { 224, 224, 3 }, at::kFloat);
		at::Tensor tensor_image_norm = tensor_image - t_means;

		tensor_image = tensor_image.permute({ 2, 0, 1 });
		tensor_image_norm = tensor_image_norm.permute({ 2, 0, 1 });

		tensors.push_back(tensor_image.unsqueeze_(0));
		tensors_norm.push_back(tensor_image_norm.unsqueeze_(0));
	}

	at::Tensor batch = torch::cat(tensors, 0);
	at::Tensor batch_norm = torch::cat(tensors_norm, 0);

	at::Tensor resnext101_feature = resnext101.forward({ batch_norm }).toTensor();
	at::Tensor resnet152_feature = resnet152.forward({ batch }).toTensor();

	at::Tensor feature = torch::cat({ resnext101_feature, resnet152_feature }, 1).to(torch::kFloat32).detach();

	// squeeze 4096 to 2048
	feature = feature.unsqueeze(0).permute({ 1, 0, 2 });
	feature = torch::tanh(torch::matmul(feature, weights).squeeze(1) + bias);

	// norm
	feature = torch::div(feature, get_L2norm(feature));

	debug_d("normalized\n");

	// PCA
	feature = feature - kw_pca_mean_vec;
	feature = feature.unsqueeze(0).permute({ 1, 0, 2 });
	feature = torch::matmul(feature, kw_pca_mat).squeeze(1);

	// norm
	feature = torch::div(feature, get_L2norm(feature));

	std::cout << feature << "\n";

	return feature;
}

std::vector<std::size_t>
CollageRanker::get_RoIs(Collage& collage)
{
	std::vector<std::size_t> regions;
	for (std::size_t i = 0; i < collage.size(); i++)
		regions.push_back(get_RoI(collage[i]));
	return regions;
}

std::size_t
CollageRanker::get_RoI(Collage::image image)
{
	std::vector<float> iou;
	for (std::size_t i = 0; i < RoIs.size(); i++) {
		auto& roi = RoIs[i];
		auto int_l = std::max(image.left, roi[0]);
		auto int_t = std::max(image.top, roi[1]);
		auto int_r = std::min(image.left + image.relative_width, roi[0] + roi[2]);
		auto int_b = std::min(image.top + image.relative_height, roi[1] + roi[3]);
		if (int_r < int_l || int_b < int_t)
			iou.push_back(0);
		else {
			auto intersection = (int_r - int_l) * (int_b - int_t);
			auto uni = ((image.relative_width * image.relative_height) + (roi[2] * roi[3])) - intersection;
			iou.push_back(intersection / uni);
		}
	}
	return std::distance(iou.begin(), std::max_element(iou.begin(), iou.end()));
}

std::vector<float>
CollageRanker::score_image(std::vector<float> feature, std::size_t region)
{
	std::vector<float> score;
	for (size_t i = 0; i < region_data[region].size(); i++)
		score.push_back(d_cos_normalized(feature, region_data[region][i]));
	return score;
}

std::vector<float>
CollageRanker::average_scores(std::vector<std::vector<float>> scores)
{
	size_t count = scores.size();
	std::vector<float> result;
	std::cout << "COUNT " << count << "\n";
	if (count == 0)
		return result;

	for (size_t i = 0; i < scores[0].size(); i++) {
		float sum = 0;
		for (size_t j = 0; j < count; j++) {
			sum += scores[j][i];
		}
		result.push_back(sum / count);
	}
	return result;
}