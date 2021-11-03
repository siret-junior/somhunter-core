
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

#ifndef KEYWORD_CLIP_RANKER_H_
#define KEYWORD_CLIP_RANKER_H_

#include <cassert>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <string>
#include <vector>
// ---
#include "common.h"
#include "embedding-ranker.h"
#include "http.h"
#include "utils.hpp"

namespace sh
{
class KeywordClipRanker : public EmbeddingRanker<SecondaryFrameFeatures>
{
public:
	inline KeywordClipRanker(const Settings& config) : server_url(config.remote_services.CLIP_query_to_vec.address) {}

	void rank_sentence_query(const std::string& sentence_query, ScoreModel& model,
	                         const SecondaryFrameFeatures& _dataset_features, size_t temporal);

private:
	Http _http;
	std::string server_url;
};
};  // namespace sh

#endif  // KEYWORD_CLIP_RANKER_H_
