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

#ifndef EMBEDDING_RANKER_H_
#define EMBEDDING_RANKER_H_

#include <cassert>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "common.h"

#include "dataset-frames.h"
#include "scores.h"
#include "settings.h"
#include "utils.hpp"

namespace sh {

class EmbeddingRanker {
public:
	virtual ~EmbeddingRanker() noexcept {}

protected:
	std::vector<float> inverse_score_vector(const std::vector<float>& query_vecs,
	                                        const DatasetFeatures& features) const;

	std::vector<float> inverse_score_vector(const float* query_vecs, const DatasetFeatures& features) const;
};

}  // namespace sh

#endif  // EMBEDDING_RANKER_H_
