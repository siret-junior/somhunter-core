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

#include "embedding-ranker.h"

using namespace sh;

std::vector<float> EmbeddingRanker::inverse_score_vector(const std::vector<float>& query_vec,
                                                         const DatasetFeatures& _dataset_features) const
{
	return inverse_score_vector(query_vec.data(), _dataset_features);
}

std::vector<float> EmbeddingRanker::inverse_score_vector(const float* query_vec,
                                                         const DatasetFeatures& _dataset_features) const
{
	size_t target_dim{ _dataset_features.dim() };

	// Result is final score \in [0.0F, 1.0F] of `query_vec` as temporal query
	std::vector<float> scores;
	scores.resize(_dataset_features.size());

	// For all frame_IDs
	for (FrameId frame_ID = 0; frame_ID < _dataset_features.size(); ++frame_ID) {
		const float* raw_frame_features = _dataset_features.fv(frame_ID);

		auto dist = utils::d_cos_normalized(query_vec, raw_frame_features, target_dim) / 2.0f;

		scores[frame_ID] = dist;
	}

	return scores;
}
