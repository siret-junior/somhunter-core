
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

#ifndef scores_h
#define scores_h

#include <map>
#include <set>
#include <vector>

#include "common.h"

#include "DatasetFeatures.h"
#include "DatasetFrames.h"

namespace sh {

class ScoreModel {
	/** Current score distribution for the frames. */
	std::vector<float> _scores;

	StdMatrix<float> _temporal_scores;

	/**
	 * Frames mask telling what frames should be placed inside the
	 * results.
	 *
	 * true <=> present in the result set
	 * false <=> filtered out
	 */
	std::vector<bool> _mask;

	// *** CACHING VARIABLES ***
	mutable std::vector<ImageId> _topn_cache;
	mutable bool _cache_dirty;
	mutable std::vector<ImageId> _topn_ctx_cache;
	mutable bool _cache_ctx_dirty;

public:
	ScoreModel(const DatasetFrames& p)
	    : _scores(p.size(), 1.0F),
	      _temporal_scores(MAX_TEMPORAL_SIZE, _scores),
	      _mask(p.size(), true),
	      _cache_dirty{ true },
	      _cache_ctx_dirty{ true } {}

	bool operator==(const ScoreModel& other) const;
	float operator[](ImageId i) const { return _scores[i]; }

	void reset(float val = 1.0F);

	/** Multiplies the relevance score with the provided value.
	 * Does not update temporal scores.
	 */
	float adjust(ImageId i, float prob);

	/** Multiplies the relevance score of temporal part with the provided value. */
	float adjust(size_t temp, ImageId i, float prob);

	/** Hard-sets the score with the provided value (normalization
	 * required). */
	float set(ImageId i, float prob);

	/** Pointer to the begin of the data. */
	const float* v() const { return _scores.data(); }

	const float* temp(size_t temp) const { return _temporal_scores[temp].data(); }

	/** Returns number of scores stored. */
	size_t size() const { return _scores.size(); }

	/** Transforms temporal inverse score into temporal
	 * scores and aggregates into full image scores.
	 * Depth parameter defines depth of temporal query
	 */
	void apply_temporals(size_t depth, const DatasetFrames& frames);

	/** Normalizes the score distribution. */
	void normalize(size_t depth = MAX_NUM_TEMP_QUERIES);
	void normalize(float* scores, size_t size);

	void invalidate_cache() {
		_cache_dirty = true;
		_cache_ctx_dirty = true;
	}

	void reset_mask() {
		invalidate_cache();
		std::transform(_mask.begin(), _mask.end(), _mask.begin(), [](const bool&) { return true; });
	};

	/** Returns the current value for the frame */
	bool is_masked(ImageId ID) const { return _mask[ID]; }

	/** Sets the mask value for the frame. */
	bool set_mask(ImageId ID, bool val) {
		invalidate_cache();
		return _mask[ID] = val;
	}

	/**
	 * Applies relevance feedback rescore based on the Bayesian update rule.
	 */
	void apply_bayes(std::set<ImageId> likes, const std::set<ImageId>& screen, const DatasetFeatures& features);

	/**
	 * Gets the images with the highest scores but respecting the provided
	 * limits. */
	std::vector<ImageId> top_n(const DatasetFrames& frames, size_t n, size_t from_vid_limit = 0,
	                           size_t from_shot_limit = 0) const;

	/**
	 * Gets the images with the highest scores while respecting the
	 * provided limits and each frame is wrapped by it's context based on
	 * the number of frames per line. */
	std::vector<ImageId> top_n_with_context(const DatasetFrames& frames, size_t n, size_t from_vid_limit,
	                                        size_t from_shot_limit) const;

	/** Samples `n` random frames from the current scores distribution. */
	std::vector<ImageId> weighted_sample(size_t n, float pow = 1) const;

	/** Samples a random frame from the current scores distribution. */
	ImageId weighted_example(const std::vector<ImageId>& subset) const;

	/** Returns the current rank of the provided frame (starts from 0). */
	size_t frame_rank(ImageId i) const;

	/** Sorts images by given score vector */
	static StdVector<std::pair<ImageId, float>> sort_by_score(const StdVector<float>& scores);
};

};  // namespace sh
#endif
