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

#include "RelocationRanker.h"
#include "DatasetFrames.h"
#include "RelevanceScores.h"
#include "query_types.h"

using namespace sh;

void RelocationRanker::score(const RelocationQuery& query, ScoreModel& model, size_t temporal,
                             const DatasetFeatures& features) const {
	if (query == IMAGE_ID_ERR_VAL) return;

	// Compute inverse scores in for the example query
	auto scores{ inverse_score_vector(features.fv(query), features) };

	// Update the model
	for (size_t i = 0; i < scores.size(); ++i) {
		model.adjust(temporal, i, scores[i]);
	}
}
