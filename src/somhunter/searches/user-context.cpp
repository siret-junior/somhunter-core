
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

#include "user-context.h"

#include "dataset-features.h"
#include "dataset-frames.h"

using namespace sh;

UserContext::UserContext(const Settings& settings, const std::string& username, const DatasetFrames& dataset_frames,
                         const DatasetFeatures& dataset_features)
    : ctx(0, settings, dataset_frames),
      _username(username),
      _eval_server{ settings.eval_server },
      _logger(settings.eval_server, this, &_eval_server),
      _async_SOM(settings, SOM_DISPLAY_GRID_WIDTH, SOM_DISPLAY_GRID_HEIGHT)
{
	SHLOG_D("Triggering main SOM worker");
	_async_SOM.start_work(dataset_features, ctx.scores, ctx.scores.v());

	// Temporal query SOMs
	for (size_t i = 0; i < MAX_NUM_TEMP_QUERIES; ++i) {
		SHLOG_D("Triggering " << i << " SOM worker");
		_temp_async_SOM.push_back(std::make_unique<AsyncSom>(settings, RELOCATION_GRID_WIDTH, RELOCATION_GRID_HEIGHT));
		_temp_async_SOM[i]->start_work(dataset_features, ctx.scores, ctx.scores.temp(i));
	}

	/* ***
	 * Store this initial state into the history
	 */
	ctx.screenshot_fpth = "";
	_history.emplace_back(ctx);
}

bool UserContext::operator==(const UserContext& other) const
{
	return (ctx == other.ctx && _username == other._username && _history == other._history);
}
