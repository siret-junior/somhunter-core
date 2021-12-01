
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

#include <chrono>

#include "keyword-clip-ranker.h"

using namespace sh;

void KeywordClipRanker::rank_sentence_query(const std::string& sentence_query, ScoreModel& model,
                                            const SecondaryFrameFeatures& _dataset_features, size_t temporal) {
	if (sentence_query.empty()) return;

	const nlohmann::json headers;

	nlohmann::json body;

	const auto& URL{ server_url + "/" + sentence_query };

	// Compute inverse scores in for the example query
	auto start = std::chrono::high_resolution_clock::now();
	auto [code, res] = _http.do_GET_sync_floats(URL, body, headers);
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> diff = end - start;
	SHLOG_D("CLIP request took " << diff.count() << " [s]");

	if (code != 200) {
		SHLOG_E("Could not retrieve text query embedding from remote server!!! Return code: " << code);
		return;
	}

	auto scores{ inverse_score_vector(res, _dataset_features) };

	// Update the model
	for (size_t i = 0; i < scores.size(); ++i) {
		model.adjust(temporal, i, scores[i]);
	}
}
