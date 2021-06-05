
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

#include "scores.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <map>
#include <random>
#include <set>
#include <thread>
#include <vector>

#include "common.h"

using namespace sh;

struct FrameScoreIdPair {
	float score;
	ImageId id;

	inline bool operator==(const FrameScoreIdPair& a) const { return score == a.score && id == a.id; }

	inline bool operator<(const FrameScoreIdPair& a) const {
		if (score < a.score) return true;
		if (score > a.score) return false;
		return id < a.id;
	}

	inline bool operator>(const FrameScoreIdPair& a) const {
		if (score > a.score) return true;
		if (score < a.score) return false;
		return id > a.id;
	}
};

// the dank C++ standard is still missing adjust_heap!
// (this is maximum heap w.r.t. the supplied `less`)
template <typename T, typename C>
static void heap_down(T* heap, size_t start, size_t lim, C less = std::less<T>()) {
	for (;;) {
		size_t L = 2 * start + 1;
		size_t R = L + 1;
		if (R < lim) {
			if (less(heap[L], heap[R])) {
				if (less(heap[start], heap[R])) {
					std::swap(heap[start], heap[R]);
					start = R;
				} else
					break;
			} else {
				if (less(heap[start], heap[L])) {
					std::swap(heap[start], heap[L]);
					start = L;
				} else
					break;
			}
		} else if (L < lim) {
			if (less(heap[start], heap[L])) std::swap(heap[start], heap[L]);
			break;  // exit safely!
		} else
			break;
	}
}

bool ScoreModel::operator==(const ScoreModel& other) const { return (_scores == other._scores); }

void ScoreModel::reset(float val) {
	invalidate_cache();

	for (auto& i : _scores) i = val;
	for (auto&& moment_score : _temporal_scores) {
		for (auto& ms : moment_score) ms = val;
	}
}

float ScoreModel::adjust(ImageId i, float prob) {
	invalidate_cache();

	return _scores[i] *= prob;
}

float ScoreModel::adjust(size_t temp, ImageId i, float prob) {
	invalidate_cache();

	return _temporal_scores[temp][i] *= prob;
}

float ScoreModel::set(ImageId i, float prob) {
	invalidate_cache();

	return _scores[i] = prob;
}

std::vector<ImageId> ScoreModel::top_n_with_context(const DatasetFrames& frames, size_t n, size_t from_vid_limit,
                                                    size_t from_shot_limit) const {
	// Is this cached
	// !! We assume that vid/shot limits do not change during the runtime.
	if (!_cache_ctx_dirty) {
		return _topn_ctx_cache;
	}

	/* This display needs to have `GUI_IMG_GRID_WIDTH`-times more images
	if we want to keep reporting `n` unique results. */
	n = n * DISPLAY_GRID_WIDTH;

	auto to_show = top_n(frames, n / DISPLAY_GRID_WIDTH, from_vid_limit, from_shot_limit);

	_topn_ctx_cache.clear();
	_topn_ctx_cache.reserve(n);
	for (auto&& selected : to_show) {
		auto video_id = frames.get_video_id(selected);
		for (int i = -TOP_N_SELECTED_FRAME_POSITION; i < DISPLAY_GRID_WIDTH - TOP_N_SELECTED_FRAME_POSITION; ++i) {
			if (frames.get_video_id(selected + i) == video_id) {
				_topn_ctx_cache.push_back(selected + i);
			} else {
				_topn_ctx_cache.push_back(IMAGE_ID_ERR_VAL);
			}
		}
	}

	_cache_ctx_dirty = false;
	return _topn_ctx_cache;
}

std::vector<ImageId> ScoreModel::top_n(const DatasetFrames& frames, size_t n, size_t from_vid_limit,
                                       size_t from_shot_limit) const {
	// Is this cached
	// !! We assume that vid/shot limits do not change during the runtime.
	if (!_cache_dirty) {
		return _topn_cache;
	}

	if (from_vid_limit == 0) from_vid_limit = _scores.size();

	if (from_shot_limit == 0) from_shot_limit = _scores.size();

	if (n > _scores.size() || n == 0) n = _scores.size();

	std::vector<FrameScoreIdPair> score_ids;
	for (ImageId i = 0; i < _scores.size(); ++i) {
		auto mask{ _mask[i] };

		// Filter out masked values
		if (mask) {
			score_ids.emplace_back(FrameScoreIdPair{ _scores[i], i });
		}
	}

	std::sort(score_ids.begin(), score_ids.end(), std::greater<FrameScoreIdPair>());

	std::map<VideoId, size_t> frames_per_vid;
	std::map<VideoId, std::map<ShotId, size_t>> frames_per_shot;

	_topn_cache.clear();
	_topn_cache.reserve(n);
	size_t t = 0;
	for (ImageId i = 0; t < n && i < score_ids.size(); ++i) {
		ImageId frame = score_ids[i].id;
		auto vf = frames.get_frame(frame);

		// If we have already enough from this video
		if (frames_per_vid[vf.video_ID]++ >= from_vid_limit) continue;

		// If we have already enough from this shot
		if (frames_per_shot[vf.video_ID][vf.shot_ID]++ >= from_shot_limit) continue;

		_topn_cache.push_back(frame);
		++t;
	}

	_cache_dirty = false;
	return _topn_cache;
}

std::vector<ImageId> ScoreModel::weighted_sample(size_t k, float pow) const {
	size_t n = _scores.size();

	assert(n >= 2);
	assert(k < n);

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> real_dist(0.0f, 1.0f);

	size_t branches = n - 1;
	std::vector<float> tree(branches + n, 0);
	float sum = 0;
	for (size_t i = 0; i < n; ++i) sum += tree[branches + i] = powf(_scores[i], pow);

	auto upd = [&tree, branches, n](size_t i) {
		const size_t l = 2 * i + 1;
		const size_t r = 2 * i + 2;
		if (i < branches + n) tree[i] = ((l < branches + n) ? tree[l] : 0) + ((r < branches + n) ? tree[r] : 0);
	};

	auto updb = [&tree, branches, n, upd](size_t i) {
		for (;;) {
			upd(i);
			if (i != 0u)
				i = (i - 1) / 2;
			else
				break;
		}
	};

	for (size_t i = branches; i > 0; --i) upd(i - 1);

	std::vector<ImageId> res(k, 0);

	for (ImageId& rei : res) {
		float x = real_dist(gen) * tree[0];
		size_t i = 0;
		for (;;) {
			const size_t l = 2 * i + 1;
			const size_t r = 2 * i + 2;

			// cout << "at i " << i << " x: " << x << " in tree: "
			// << tree[i] << endl;

			if (i >= branches) break;
			if (r < branches + n && x >= tree[l]) {
				x -= tree[l];
				i = r;
			} else
				i = l;
		}

		tree[i] = 0;
		updb(i);
		rei = ImageId(i - branches);
	}

	return res;
}

ImageId ScoreModel::weighted_example(const std::vector<ImageId>& subset) const {
	std::vector<float> fs(subset.size());
	for (size_t i = 0; i < subset.size(); ++i) fs[i] = _scores[subset[i]];

	std::random_device rd;
	std::mt19937 gen(rd());
	std::discrete_distribution<ImageId> dist(fs.begin(), fs.end());
	return subset[dist(gen)];
}

void ScoreModel::apply_bayes(std::set<ImageId> likes, const std::set<ImageId>& screen,
                             const DatasetFeatures& features) {
	if (likes.empty()) return;

	invalidate_cache();

	constexpr float Sigma = .1f;
	constexpr size_t max_others = 64;

	std::vector<ImageId> others;
	others.reserve(screen.size());
	for (ImageId id : screen)
		if (likes.count(id) == 0) others.push_back(id);

	if (others.size() > max_others) {
		// drop random overflow
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<size_t> rng;

		for (size_t i = 0; (i + 1) < max_others; ++i)
			std::swap(others[i], others[i + 1 + rng(gen) % (max_others - i - 1)]);

		others.resize(max_others);
	}

	auto start = std::chrono::high_resolution_clock::now();

	{
		size_t n_threads = std::thread::hardware_concurrency();
		std::vector<std::thread> threads(n_threads);

		auto worker = [&](size_t threadID) {
			const ImageId first = ImageId(threadID * _scores.size() / n_threads);
			const ImageId last = ImageId((threadID + 1) * _scores.size() / n_threads);

			for (ImageId ii = first; ii < last; ++ii) {
				if (_mask[ii]) {
					float divSum = 0;

					for (ImageId oi : others) divSum += expf(-features.d_dot(ii, oi) / Sigma);

					for (auto&& like : likes) {
						const float likeValTmp = expf(-features.d_dot(ii, like) / Sigma);
						_scores[ii] *= likeValTmp / (likeValTmp + divSum);
					}
				}
			}
		};
		for (size_t i = 0; i < threads.size(); ++i) threads[i] = std::thread(worker, i);
		for (auto& t : threads) t.join();
	}

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = end - start;
	SHLOG_D("Bayes took " << elapsed.count());

	normalize();
}

void ScoreModel::apply_temporals(size_t depth, const DatasetFrames& frames) {
	if (depth == 0) return;

	depth = std::min(depth, _temporal_scores.size());

	// Last level copy to main scores
	for (size_t j = 0; j < _scores.size(); ++j) {
		_scores[j] = _temporal_scores[depth - 1][j];
	}

	// At this point the _temporal_scores contains proportional inverse scores
	// Other levels multiply with minimal inverse scores from window
	for (long i = depth - 2; i >= 0; --i) {
		for (size_t j = 0; j < _scores.size(); ++j) {
			auto img_it = frames.get_frame_it(j);
			VideoId vid_ID = img_it->video_ID;

			// Select minimal proportional inverse score
			float min = 1;
			for (size_t k = 1; k < KW_TEMPORAL_SPAN && j + k < _scores.size(); ++k) {
				img_it++;
				if (img_it->video_ID != vid_ID) break;
				min = std::min(min, _scores[j + k]);
			}
			_scores[j] = _temporal_scores[i][j] * min;
		}
	}

	// Apply exponential
	for (size_t j = 0; j < _scores.size(); ++j) {
		_scores[j] = std::exp(_scores[j] * -50);
	}

	for (size_t i = 0; i < depth; ++i) {
		for (size_t j = 0; j < _scores.size(); ++j) {
			_temporal_scores[i][j] = std::exp(_temporal_scores[i][j] * -50);
		}
	}
}

void ScoreModel::normalize(size_t depth) {
	depth = std::min<size_t>(depth, MAX_TEMPORAL_SIZE);

	normalize(_scores.data(), _scores.size());
	for (size_t i = 0; i < depth; ++i) normalize(_temporal_scores[i].data(), _temporal_scores[i].size());
}

void ScoreModel::normalize(float* scores, size_t size) {
	float smax = 0;

	for (ImageId ii = 0; ii < size; ++ii)
		if (scores[ii] > smax) smax = scores[ii];

	if (smax < MINIMAL_SCORE) {
		SHLOG_E("all images have negligible score!");
		smax = MINIMAL_SCORE;
	}

	size_t n = 0;
	for (ImageId ii = 0; ii < size; ++ii) {
		if (_mask[ii]) {
			scores[ii] /= smax;
			if (scores[ii] < MINIMAL_SCORE) ++n, scores[ii] = MINIMAL_SCORE;
		}
	}
}

size_t ScoreModel::frame_rank(ImageId i) const {
	size_t rank{ 0 };
	float tar_score = _scores[i];
	for (float s : _scores) {
		if (s > tar_score) ++rank;
	}
	return rank;
}

std::vector<std::pair<ImageId, float>> ScoreModel::sort_by_score(const StdVector<float>& scores) {
	// Zip scores with frame IDs
	std::vector<std::pair<ImageId, float>> id_scores;
	id_scores.reserve(scores.size());
	{
		size_t i{ 0 };
		std::transform(scores.begin(), scores.end(), std::back_inserter(id_scores), [&i](const float& x) {
			return std::pair{ i++, x };
		});
	}

	// Sort results
	std::sort(id_scores.begin(), id_scores.end(),
	          [](const std::pair<size_t, float>& left, const std::pair<size_t, float>& right) {
		          return left.second < right.second;
	          });

	return id_scores;
}