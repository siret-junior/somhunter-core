
/* This file is part of SOMHunter.
 *
 * Copyright (C) 2021 František Mejzlík <frankmejzlik@gmail.com>
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

#ifndef DATASET_FEATURES_H_
#define DATASET_FEATURES_H_

#include "dataset-frames.h"
// ---
#include <cmath>
#include <map>
#include <queue>
// ---
#include "common.h"
#include "distances.hpp"

namespace sh
{
/**
 * Represents one set of features for the given dataset.
 */
class FrameFeatures
{
	// *** METHODS ***
public:
	FrameFeatures() = delete;
	FrameFeatures(const DatasetFrames& frames, const Settings& config);

	// ---

	size_t size() const { return _size; }
	size_t dim() const { return _dim; }
	const float* fv(size_t i) const { return _data.data() + _dim * i; }

	std::vector<FrameId> get_top_knn(const DatasetFrames& _dataset_frames, FrameId id, size_t per_vid_limit = 0,
	                                 size_t from_shot_limit = 0) const;

	std::vector<FrameId> get_top_knn(const DatasetFrames& _dataset_frames, FrameId id,
	                                 std::function<bool(FrameId ID)> pred, size_t per_vid_limit = 0,
	                                 size_t from_shot_limit = 0) const;

	float d_manhattan(size_t i, size_t j) const;
	float d_sqeucl(size_t i, size_t j) const;
	float d_eucl(size_t i, size_t j) const;
	float d_dot_normalized(size_t i, size_t j) const;
	float d_cos(size_t i, size_t j) const;

	// *** MEMBER VARIABLES  ***
private:
	/** Number of rows (i.e. number of feature vectors). */
	std::size_t _size;
	/** Number of vector components. */
	std::size_t _dim;
	/** Raw flat data matrix (row-wise). */
	std::vector<float> _data;
};

/**
 * Represents all available feature sets.
 */
class DatasetFeatures
{
public:
	DatasetFeatures(const DatasetFrames& frames, const Settings& config)
	    : primary{ frames, config }, secondary{ frames, config } {};

public:
	FrameFeatures primary;    //< e.g. W2VV
	FrameFeatures secondary;  //< e.g. CLIP
};

};  // namespace sh

#endif  // DATASET_FEATURES_H_
