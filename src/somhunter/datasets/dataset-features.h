
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
#include <exception>
#include <fstream>
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
template <typename SETT>
class FrameFeatures
{
	// *** METHODS ***
public:
	FrameFeatures() = delete;
	FrameFeatures(const DatasetFrames& frames, const SETT& config);

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

using PrimaryFrameFeatures = FrameFeatures<DatasetsSettings::PrimaryFeaturesSettings>;
using SecondaryFrameFeatures = FrameFeatures<DatasetsSettings::SecondaryFeaturesSettings>;

/**
 * Represents all available feature sets.
 */
class DatasetFeatures
{
public:
	DatasetFeatures(const DatasetFrames& frames, const Settings& config)
	    : primary{ frames, config.datasets.primary_features },
	      secondary{ frames, config.datasets.secondary_features } {};

public:
	PrimaryFrameFeatures primary;      //< e.g. W2VV
	SecondaryFrameFeatures secondary;  //< e.g. CLIP
};

template <typename SETT>
FrameFeatures<SETT>::FrameFeatures(const DatasetFrames& p, const SETT& config) : _size(p.size()), _dim(config._dim)
{
	SHLOG_D("Loading dataset features from '" << config.features_file << "'...");

	_data.resize(_dim * _size);
	std::ifstream in(config.features_file, std::ios::binary);
	if (!in.good()) {
		std::string msg{ "Error opening features file '" + config.features_file + "'!" };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	// Skip the header
	in.ignore(config.features_file_data_off);

	if (!in.read(reinterpret_cast<char*>(_data.data()), sizeof(float) * _data.size())) {
		std::string msg{ "Feature matrix reading problems at '" + config.features_file + "'!" };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	} else {
		SHLOG_S("Successfully loaded " << _size << " frame features of dimension " << config._dim << ".");
	}
}

template <typename SETT>
std::vector<FrameId> FrameFeatures<SETT>::get_top_knn(const DatasetFrames& _dataset_frames, FrameId id,
                                                      size_t per_vid_limit, size_t from_shot_limit) const
{
	return get_top_knn(
	    _dataset_frames, id, [](FrameId /*frame_ID*/) { return true; }, per_vid_limit, from_shot_limit);
}

template <typename SETT>
std::vector<FrameId> FrameFeatures<SETT>::get_top_knn(const DatasetFrames& _dataset_frames, FrameId id,
                                                      std::function<bool(FrameId ID)> pred, size_t per_vid_limit,
                                                      size_t from_shot_limit) const
{
	if (per_vid_limit == 0) per_vid_limit = _dataset_frames.size();

	if (from_shot_limit == 0) from_shot_limit = _dataset_frames.size();

	auto cmp = [](const std::pair<FrameId, float>& left, const std::pair<FrameId, float>& right) {
		return left.second > right.second;
	};

	std::priority_queue<std::pair<FrameId, float>, std::vector<std::pair<FrameId, float>>, decltype(cmp)> q3(cmp);

	for (FrameId i{ 0 }; i < _size; ++i) {
		auto d = d_dot_normalized(id, i);
		q3.emplace(i, d);
	}

	std::vector<FrameId> res;
	res.reserve(TOPKNN_LIMIT);

	size_t num_videos = _dataset_frames.get_num_videos();
	std::vector<size_t> per_vid_frame_hist(num_videos, 0);
	std::map<VideoId, std::map<ShotId, size_t>> frames_per_shot;

	while (res.size() < TOPKNN_LIMIT && !q3.empty()) {
		auto [adept_ID, f]{ q3.top() };

		if (q3.empty()) break;

		q3.pop();

		auto vf = _dataset_frames.get_frame(adept_ID);

		// If we have already enough from this video
		if (per_vid_frame_hist[vf.video_ID] >= per_vid_limit) continue;

		// If we have already enough from this shot
		if (frames_per_shot[vf.video_ID][vf.shot_ID] >= from_shot_limit) continue;

		// Only if predicate is true
		if (pred(adept_ID)) {
			res.emplace_back(adept_ID);
			per_vid_frame_hist[vf.video_ID]++;
			frames_per_shot[vf.video_ID][vf.shot_ID]++;
		}
	}

	return res;
}

template <typename SETT>
float FrameFeatures<SETT>::d_manhattan(size_t i, size_t j) const
{
	return ::d_manhattan(fv(i), fv(j), _dim);
}

template <typename SETT>
float FrameFeatures<SETT>::d_sqeucl(size_t i, size_t j) const
{
	return ::d_sqeucl(fv(i), fv(j), _dim);
}

template <typename SETT>
float FrameFeatures<SETT>::d_eucl(size_t i, size_t j) const
{
	return sqrtf(d_sqeucl(i, j));
}

template <typename SETT>
float FrameFeatures<SETT>::d_dot_normalized(size_t i, size_t j) const
{
	return 1 - ::d_dot_normalized(fv(i), fv(j), _dim);
}

template <typename SETT>
float FrameFeatures<SETT>::d_cos(size_t i, size_t j) const
{
	float s = 0, w1 = 0, w2 = 0;
	const float *iv = fv(i), *jv = fv(j);
	for (size_t d = 0; d < _dim; ++d) {
		s += iv[d] * jv[d];
		w1 += utils::square(iv[d]);
		w2 += utils::square(jv[d]);
	}
	if (w1 == 0 && w2 == 0) return 0;
	return 1 - s / sqrtf(w1 * w2);
}

};  // namespace sh

#endif  // DATASET_FEATURES_H_
