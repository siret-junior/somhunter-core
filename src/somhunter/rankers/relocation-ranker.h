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

#ifndef RELOCATION_RANKER_H_
#define RELOCATION_RANKER_H_

#include <cassert>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "common.h"

#include "dataset-frames.h"
#include "embedding-ranker.h"
#include "scores.h"
#include "settings.h"
#include "query-types.h"
#include "utils.hpp"

namespace sh {
class RelocationRanker : public EmbeddingRanker {
public:
	void score(const RelocationQuery&, ScoreModel& model, size_t temporal, const DatasetFeatures& _dataset_features) const;
};
}  // namespace sh

#endif  // RELOCATION_H_
