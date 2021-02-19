#include "CollageRanker.h"
#include "ImageManipulator.h"
#include <math.h>
#include <string>

CollageRanker::CollageRanker(const Config& config)
{

	try {
		resnet152 = torch::jit::load(config.model_ResNet_file);
	} catch (const c10::Error& e) {
		std::string msg{ "Error openning ResNet model file: " + config.model_ResNet_file };
		warn_d(msg);

#ifndef NDEBUG
		throw std::runtime_error(msg);
#endif // NDEBUG
	}

	try {
		resnext101 = torch::jit::load(config.model_ResNext_file);
	} catch (const c10::Error& e) {
		std::string msg{ "Error openning ResNext model file: " + config.model_ResNext_file };
		warn_d(msg);

#ifndef NDEBUG
		throw std::runtime_error(msg);
#endif // NDEBUG
	}

	try {
		bias = torch::tensor(KeywordRanker::parse_float_vector(config.model_W2VV_img_bias, 2048));
	} catch (const c10::Error& e) {
		std::string msg{ "Error loading W2VV FC bias: " + config.model_W2VV_img_bias };
		warn_d(msg);

#ifndef NDEBUG
		throw std::runtime_error(msg);
#endif // NDEBUG
	}

	try {
		weights = torch::tensor(KeywordRanker::parse_float_vector(config.model_W2VV_img_weigths, 4096 * 2048))
		            .reshape({ 2048, 4096 })
		            .permute({ 1, 0 });
	} catch (const c10::Error& e) {
		std::string msg{ "Error loading W2VV FC weights: " + config.model_W2VV_img_weigths };
		warn_d(msg);

#ifndef NDEBUG
		throw std::runtime_error(msg);
#endif // NDEBUG
	}

	kw_pca_mat = torch::tensor(KeywordRanker::parse_float_vector(
	                             config.kw_PCA_mat_file, config.pre_PCA_features_dim * config.kw_PCA_mat_dim))
	               .reshape({ (long long)(config.kw_PCA_mat_dim), (long long)(config.pre_PCA_features_dim) })
	               .permute({ 1, 0 });
	kw_pca_mean_vec =
	  torch::tensor(KeywordRanker::parse_float_vector(config.kw_bias_vec_file, config.pre_PCA_features_dim));
}

void
CollageRanker::score(Collage& collage)
{
	if (collage.images.size() > 0) {
		collage.RGBA_to_BGR();
		collage.resize_all(224, 224);

		at::Tensor feature = get_features(collage);
	}
}

// in 1st dim
at::Tensor
CollageRanker::get_L2norm(at::Tensor data)
{
	at::Tensor norm = torch::zeros({ data.sizes()[0], 1 });

	for (size_t i = 0; i < data.sizes()[0]; i++)
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

	float a[] = { 1.0, 2, 3, 5, 6, 7 };
	torch::Tensor b = torch::from_blob(a, { 2, 3 });

	std::cout << b << std::endl;

	debug_d("normalizing\n");
	try {
		auto norm = torch::linalg::linalg_norm(b, 2, { 1 }, true, torch::kFloat32);
	} catch (const c10::IndexError& e) {
		std::cout << e.what() << std::endl;
	}

	return feature;
}
