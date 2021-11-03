
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

#include "keyword-clip-ranker.h"


using namespace sh;

void KeywordClipRanker::rank_sentence_query(const std::string& sentence_query, ScoreModel& model,
	                         const SecondaryFrameFeatures& _dataset_features, size_t temporal)
{
	if (sentence_query.empty()) return;

    const nlohmann::json headers;

	nlohmann::json body;

	const auto& URL{ server_url + "/" + sentence_query };

	// Compute inverse scores in for the example query
    auto [code, res] = _http.do_GET_sync_floats(URL, body, headers);
	auto scores{ inverse_score_vector(res, _dataset_features) };

	// Update the model
	for (size_t i = 0; i < scores.size(); ++i) {
		model.adjust(temporal, i, scores[i]);
	}
}


