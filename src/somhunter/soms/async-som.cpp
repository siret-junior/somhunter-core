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

#include "async-som.h"

#include <chrono>
#include <random>
#include <thread>

#include "common.h"
#include "som.h"

#include "somhunter.h"

using namespace sh;

#ifdef SHLOG_D  // Turn off `SHLOG_D` in this TU
#	undef SHLOG_D
#	define SHLOG_D(x) _dont_write_log_err
#endif

void AsyncSom::async_som_worker(AsyncSom* parent, const Settings& _logger_settings) {
	std::random_device rd;
	std::mt19937 rng(rd());
	const size_t width = parent->width;
	const size_t height = parent->height;

	SHLOG_D("SOM worker is starting...");

	while (!parent->terminate) {
		std::vector<float> points(parent->_features_data_len);
		std::vector<float> scores(parent->_scores_data_len);
		std::vector<bool> present_mask(parent->_scores_data_len);
		size_t _size;

		{
			// get new data, or wait for them.

			std::unique_lock lck(parent->worker_lock);
			if (parent->terminate) continue;
			if (!parent->new_data) {
				parent->new_data_wakeup.wait(lck);
				continue;
			}

			points.swap(parent->points);
			scores.swap(parent->scores);
			present_mask.swap(parent->present_mask);
			_size = scores.size();
			parent->new_data = false;
			parent->m_ready = false;
			SHLOG_D("SOM worker just got new work...");
		}

		if (parent->new_data || parent->terminate) continue;

		// at this point: restart is off, input is ready.

		std::vector<float> nhbrdist(width * width * height * height);
		for (size_t x1 = 0; x1 < width; ++x1)
			for (size_t y1 = 0; y1 < height; ++y1)
				for (size_t x2 = 0; x2 < width; ++x2)
					for (size_t y2 = 0; y2 < height; ++y2)
						nhbrdist[x1 + width * (y1 + height * (x2 + width * y2))] =
						    abs(float(x1) - float(x2)) + abs(float(y1) - float(y2));

		if (parent->new_data || parent->terminate) continue;

		const auto& pfs{ _logger_settings.datasets.primary_features };

		std::vector<float> koho(width * height * pfs._dim, 0);
		float negAlpha = -0.01f;
		float negRadius = 1.1f;
		float alphasA[2] = { 0.3f, 0.1f };
		float alphasB[2] = { negAlpha * alphasA[0], negAlpha * alphasA[1] };
		float radiiA[2] = { float(width + height) / 3, 0.1f };
		float radiiB[2] = { negRadius * radiiA[0], negRadius * radiiA[1] };

		std::chrono::high_resolution_clock::time_point begin = std::chrono::high_resolution_clock::now();
		fit_SOM(_size, width * height, pfs._dim, SOM_ITERS, points, koho, nhbrdist, alphasA, radiiA, alphasB, radiiB,
		        scores, present_mask, rng);
		std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
		SHLOG_D("SOM took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " [ms]");

		if (parent->new_data || parent->terminate) continue;

		std::vector<size_t> point_to_koho(_size);
		begin = std::chrono::high_resolution_clock::now();
		{
			std::size_t n_threads = std::min<std::size_t>(MAX_NUM_TEMP_WORKERS, std::thread::hardware_concurrency());
			std::vector<std::thread> threads(n_threads);

			auto worker = [&](size_t id) {
				size_t start = id * _size / n_threads;
				size_t end = (id + 1) * _size / n_threads;
				map_points_to_kohos(start, end, width * height, pfs._dim, points, koho, point_to_koho, present_mask);
			};

			for (size_t i = 0; i < n_threads; ++i) threads[i] = std::thread(worker, i);

			for (size_t i = 0; i < n_threads; ++i) threads[i].join();
		}
		end = std::chrono::high_resolution_clock::now();
		SHLOG_D("Mapping took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
		                        << " [ms]");

		if (parent->new_data || parent->terminate) continue;

		parent->mapping.clear();
		parent->mapping.resize(width * height);
		for (FrameId im = 0; im < point_to_koho.size(); ++im)
			if (present_mask[im]) parent->mapping[point_to_koho[im]].push_back(im);

		parent->koho = std::move(koho);

		std::atomic_thread_fence(std::memory_order_release);
		parent->m_ready = true;

		/* TODO
		 * add the parent->restart checks to SOM functions so that they
		 * may break in the middle
		 */
	}
	SHLOG_D("SOM worker finished.");
}

AsyncSom::AsyncSom(const Settings& _logger_settings, size_t w, size_t h, const PrimaryFrameFeatures& fs,
                   const ScoreModel& sc)
    :

      _features_data_len{ fs.dim() * sc.size() },
      _scores_data_len{ sc.size() },
      points(_features_data_len),
      scores(_scores_data_len),

      width(w),
      height(h) {
	new_data = m_ready = terminate = false;
	worker = std::thread(async_som_worker, this, _logger_settings);
}

AsyncSom::~AsyncSom() {
	SHLOG_D("Requesting SOM worker termination...");
	terminate = true;
	new_data_wakeup.notify_all();
	worker.join();
	SHLOG_D("SOM worker terminated.");
}

void AsyncSom::start_work(const PrimaryFrameFeatures& fs, const ScoreModel& sc, const float* scores_orig) {
	{
		std::unique_lock lck(worker_lock);

		// points = std::vector<float>(fs.fv(0), fs.fv(0) + fs.dim() * sc.size());
		// scores = std::vector<float>(scores_orig, scores_orig + sc.size());
		std::memcpy(points.data(), fs.fv(0), _features_data_len * sizeof(float));
		std::memcpy(scores.data(), scores_orig, _scores_data_len * sizeof(float));

		// present_mask = std::vector<bool>(sc.size(), false);
		present_mask.clear();
		present_mask.reserve(sc.size());
		for (FrameId ii = 0; ii < sc.size(); ++ii) {
			present_mask.emplace_back(sc.is_masked(ii));
		}

		new_data = true;
	}

	new_data_wakeup.notify_all();
}

std::vector<FrameId> AsyncSom::get_display(ScoreModel model_scores) const {
	std::vector<FrameId> ids;
	ids.resize(width * height);

	// Select weighted example from cluster
	for (size_t i = 0; i < width; ++i) {
		for (size_t j = 0; j < height; ++j) {
			if (!map(i + width * j).empty()) {
				FrameId id = model_scores.weighted_example(map(i + width * j));
				ids[i + width * j] = id;
			}
		}
	}

	[[maybe_unused]] auto begin = std::chrono::steady_clock::now();
	// Fix empty cluster
	std::vector<size_t> stolen_count(width * height, 1);
	for (size_t i = 0; i < width; ++i) {
		for (size_t j = 0; j < height; ++j) {
			if (map(i + width * j).empty()) {
				SHLOG_D("Fixing cluster " << i + width * j);

				// Get SOM node of empty cluster
				auto k = get_koho(i + width * j);

				// Get nearest cluster with enough elements
				size_t clust = nearest_cluster_with_atleast(k, stolen_count);

				stolen_count[clust]++;
				std::vector<FrameId> ci = map(clust);

				for (FrameId ii : ids) {
					auto fi = std::find(ci.begin(), ci.end(), ii);
					if (fi != ci.end()) ci.erase(fi);
				}

				// If some frame candidates
				if (!ci.empty()) {
					FrameId id = model_scores.weighted_example(ci);
					ids[i + width * j] = id;
				}
				// Subsitute with "empty" frame
				else {
					ids[i + width * j] = ERR_VAL<FrameId>();
				}
			}
		}
	}

	[[maybe_unused]] auto end = std::chrono::steady_clock::now();
	SHLOG_D("Fixing clusters took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
	                                << " [ms]");
	return ids;
}
