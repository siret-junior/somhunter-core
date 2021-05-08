#ifndef EXTRACTOR_H_
#define EXTRACTOR_H_

#include "log.h"

#include <torch/types.h>
#include <torch/jit.h>

namespace sh {

class Extractor {
public:
	Extractor();

	void run();

private:
	torch::jit::script::Module resnet152;
	torch::jit::script::Module resnext101;

	at::Tensor bias;
	at::Tensor weights;

	at::Tensor kw_pca_mat;
	at::Tensor kw_pca_mean_vec;

	std::string out_dir{ "output/" };
	std::string thumbs{ "data/ITEC_w2vv/thumbs/" };
	std::string frames{ "data/ITEC_w2vv/ITEC.keyframes.dataset" };
};

};      // namespace sh
#endif  // EXTRACTOR_H_