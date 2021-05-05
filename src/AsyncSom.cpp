
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

#include "AsyncSom.h"
#include "SomHunter.h"

#include "SOM.h"

#include <chrono>
#include <random>
#include <thread>

#include "config_json.h"
#include "log.h"

void AsyncSom::async_som_worker(AsyncSom* parent, const Config& cfg) {
	std::random_device rd;
	std::mt19937 rng(rd());

	info_d("SOM worker starting");

	while (!parent->terminate) {
		std::vector<float> points;
		std::vector<float> scores;
		std::vector<bool> present_mask;
		size_t n;

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
			n = scores.size();
			parent->new_data = false;
			parent->m_ready = false;
			info_d("SOM worker got new work");
		}

		if (parent->new_data || parent->terminate) continue;

		// at this point: restart is off, input is ready.

		std::vector<float> nhbrdist(SOM_DISPLAY_GRID_WIDTH * SOM_DISPLAY_GRID_WIDTH * SOM_DISPLAY_GRID_HEIGHT *
		                            SOM_DISPLAY_GRID_HEIGHT);
		for (size_t x1 = 0; x1 < SOM_DISPLAY_GRID_WIDTH; ++x1)
			for (size_t y1 = 0; y1 < SOM_DISPLAY_GRID_HEIGHT; ++y1)
				for (size_t x2 = 0; x2 < SOM_DISPLAY_GRID_WIDTH; ++x2)
					for (size_t y2 = 0; y2 < SOM_DISPLAY_GRID_HEIGHT; ++y2)
						nhbrdist[x1 + SOM_DISPLAY_GRID_WIDTH *
						                  (y1 + SOM_DISPLAY_GRID_HEIGHT * (x2 + SOM_DISPLAY_GRID_WIDTH * y2))] =
						    abs(float(x1) - float(x2)) + abs(float(y1) - float(y2));

		if (parent->new_data || parent->terminate) continue;

		std::vector<float> koho(SOM_DISPLAY_GRID_WIDTH * SOM_DISPLAY_GRID_HEIGHT * cfg.features_dim, 0);
		float negAlpha = -0.01f;
		float negRadius = 1.1f;
		float alphasA[2] = { 0.3f, 0.1f };
		float alphasB[2] = { negAlpha * alphasA[0], negAlpha * alphasA[1] };
		float radiiA[2] = { float(SOM_DISPLAY_GRID_WIDTH + SOM_DISPLAY_GRID_HEIGHT) / 3, 0.1f };
		float radiiB[2] = { negRadius * radiiA[0], negRadius * radiiA[1] };

		std::chrono::high_resolution_clock::time_point begin = std::chrono::high_resolution_clock::now();
		som(n, SOM_DISPLAY_GRID_WIDTH * SOM_DISPLAY_GRID_HEIGHT, cfg.features_dim, SOM_ITERS, points, koho, nhbrdist,
		    alphasA, radiiA, alphasB, radiiB, scores, present_mask, rng);
		std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
		debug_d("SOM took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " [ms]");

		if (parent->new_data || parent->terminate) continue;

		std::vector<size_t> point_to_koho(n);
		begin = std::chrono::high_resolution_clock::now();
		{
			size_t n_threads = std::thread::hardware_concurrency();
			std::vector<std::thread> threads(n_threads);

			auto worker = [&](size_t id) {
				size_t start = id * n / n_threads;
				size_t end = (id + 1) * n / n_threads;
				mapPointsToKohos(start, end, SOM_DISPLAY_GRID_WIDTH * SOM_DISPLAY_GRID_HEIGHT, cfg.features_dim, points,
				                 koho, point_to_koho, present_mask);
			};

			for (size_t i = 0; i < n_threads; ++i) threads[i] = std::thread(worker, i);

			for (size_t i = 0; i < n_threads; ++i) threads[i].join();
		}
		end = std::chrono::high_resolution_clock::now();
		debug_d("Mapping took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
		                        << " [ms]");

		if (parent->new_data || parent->terminate) continue;

		parent->mapping.clear();
		parent->mapping.resize(SOM_DISPLAY_GRID_WIDTH * SOM_DISPLAY_GRID_HEIGHT);
		for (ImageId im = 0; im < point_to_koho.size(); ++im)
			if (present_mask[im]) parent->mapping[point_to_koho[im]].push_back(im);

		parent->koho = std::move(koho);

		std::atomic_thread_fence(std::memory_order_release);
		parent->m_ready = true;

		/* TODO
		 * add the parent->restart checks to SOM functions so that they
		 * may break in the middle
		 */
	}
	info_d("SOM worker terminating");
}

AsyncSom::AsyncSom(const Config& cfg) {
	new_data = m_ready = terminate = false;
	worker = std::thread(async_som_worker, this, cfg);
}

AsyncSom::~AsyncSom() {
	info_d("requesting SOM worker termination");
	terminate = true;
	new_data_wakeup.notify_all();
	worker.join();
	info_d("SOM worker terminated");
}

void AsyncSom::start_work(const DatasetFeatures& fs, const ScoreModel& sc) {
	std::unique_lock lck(worker_lock);
	points = std::vector<float>(fs.fv(0), fs.fv(0) + fs.dim() * sc.size());
	scores = std::vector<float>(sc.v(), sc.v() + sc.size());
	present_mask = std::vector<bool>(sc.size(), false);
	for (ImageId ii = 0; ii < sc.size(); ++ii) present_mask[ii] = sc.is_masked(ii);

	new_data = true;
	lck.unlock();

	new_data_wakeup.notify_all();
}
