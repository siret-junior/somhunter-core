/* This file is part of SOMHunter.
 *
 * Copyright (C) 2021 Frantisek Mejzlik <frankmejzlik@protonmail.com>
 *                    Mirek Kratochvil <exa.exa@gmail.com>
 *                    Patrik Vesely <prtrikvesely@gmail.com>
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

#ifndef asyncsom_h
#define asyncsom_h

#include <atomic>
#include <condition_variable>
#include <thread>
#include <vector>

#include "dataset-features.h"
#include "dataset-frames.h"
#include "scores.h"

namespace sh {
class AsyncSom {
	std::thread worker;

	size_t _dim{};

	// worker sync
	std::condition_variable new_data_wakeup;
	std::mutex worker_lock;

	/*
	 * Worker input protocol:
	 *
	 * new_data is set when a new computation is required after
	 * wakeup to process a new dataset. Worker "eats" this flag together
	 * with input data.
	 *
	 * terminate is set when the worker should exit.
	 */
	bool new_data, terminate;

	// Number of floats in features matrix
	std::size_t _features_data_len;
	// Number of floats in scores vector
	std::size_t _scores_data_len;

	std::vector<float> points, scores;
	std::vector<bool> present_mask;

	/*
	 * Worker output protocol:
	 *
	 * m_ready is set when mapping is filled in AND the
	 * memory is fenced correctly.
	 */
	bool m_ready;
	std::vector<std::vector<FrameId>> mapping;
	std::vector<float> koho;

	const size_t width;
	const size_t height;

	static void async_som_worker(AsyncSom* parent, const Settings& _logger_settings);

public:
	AsyncSom() = delete;
	~AsyncSom() noexcept;

	AsyncSom(AsyncSom&& _logger_settings) = default;

	AsyncSom(const Settings& _logger_settings, size_t width, size_t height, const PrimaryFrameFeatures& fs,
	         const ScoreModel& sc);

	void start_work(const PrimaryFrameFeatures& fs, const ScoreModel& sc, const float* scores_orig);

	std::vector<FrameId> get_display(ScoreModel scores) const;

	bool map_ready() const {
		bool t = m_ready;
		std::atomic_thread_fence(std::memory_order_acquire);
		return t;
	}
	const std::vector<FrameId>& map(size_t i) const { return mapping.at(i); }

	const float* get_koho(size_t i) const { return koho.data() + i * _dim; }

	size_t nearest_cluster_with_atleast(const float* vec, const std::vector<size_t>& stolen_count) const {
		float min = std::numeric_limits<float>::max();
		size_t res = 0;
		for (size_t i = 0; i < mapping.size(); ++i) {
			if (mapping[i].size() > stolen_count[i]) {
				float tmp = d_sqeucl(koho.data() + _dim * i, vec, _dim);
				if (min > tmp) {
					min = tmp;
					res = i;
				}
			}
		}

		return res;
	}
};

};  // namespace sh

#endif
