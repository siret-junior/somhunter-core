/* This file is part of SOMHunter.
 *
 * Copyright (C) 2021 Frantisek Mejzlik <frankmejzlik@protonmail.com>
 *                    Mirek Kratochvil <exa.exa@gmail.com>
 *                    Patrik Vesely <prtrikvesely@gmail.com>
 * 									  Marek Dobransky <marekdobr@gmail.com>
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

#ifndef EXTRACTOR_H_
#define EXTRACTOR_H_

#include "log.h"

#include <torch/jit.h>
#include <torch/types.h>

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
