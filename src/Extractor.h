#ifndef EXTRACTOR_H_
#define EXTRACTOR_H_

#include <torch/torch.h>

#include "log.h"

namespace sh {

class Extractor
{
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
	std::string thumbs{ "data/ITEC_w2vv/thšmbs/" };
	std::string frames{ "data/ITEC_w2vv/ITEC.keyframes.dataset" };
};

};
#endif // EXTRACTOR_H_