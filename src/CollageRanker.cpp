
#include "CollageRanker.h"

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
Collage::RGBA_to_BGR()
{
	if (channels == 3)
		return;

	std::vector<std::vector<float>> rgb_images;

	for (size_t i = 0; i < images.size(); i++) {
		std::vector<float> image;
		for (size_t j = 0; j < images[i].size(); j += 4) {
			image.push_back(images[i][j + 2]);
			image.push_back(images[i][j + 1]);
			image.push_back(images[i][j + 0]);
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
}

void
CollageRanker::score(Collage& collage,
                     ScoreModel& /*model*/,
                     const DatasetFeatures& features,
                     const DatasetFrames& frames)
{
	if (collage.images.size() > 0) {
		collage.RGBA_to_BGR();
		collage.resize_all(224, 224);

		at::Tensor tensor_features = get_features(collage);

		// Convert tensor to STD containered matrix
		auto collage_vectors{ to_std_matrix<float>(tensor_features) };
		// print_matrix(mat);

		// Split by `break` index
		StdMatrix<float> q0(collage_vectors.begin(), collage_vectors.begin() + collage.break_point);
		StdMatrix<float> q1(collage_vectors.begin() + collage.break_point, collage_vectors.end());

		// ========================================
		// \todo

		// Aggregate multiple vectors per query into single one
		StdMatrix<float> query_vectors;
		if (!q0.empty())
			query_vectors.emplace_back(q0.front()); // Dummy
		if (!q1.empty())
			query_vectors.emplace_back(q1.front()); // Dummy

		/* `features` is matrix of dataset frames it is going to score against
		 *		- for each subregion we need to send in different variable
		 * `frames` is just structure describing each frame
		 * 		- it can stay the same for each subregion
		 */
		auto scores{ KeywordRanker::score_vectors(query_vectors, features, frames) };
		print_vector(scores);

		auto sorted_results{ KeywordRanker::sort_by_score(scores) };
		KeywordRanker::report_results(sorted_results, frames);

		// \todo The scores in the `model` should be updated somewhere here

		// \todo
		// ========================================
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
