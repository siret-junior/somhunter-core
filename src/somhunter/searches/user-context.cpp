
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

UserContext::UserContext(const std::string& user_token, const Config& cfg, const DatasetFrames& frames,
                         const DatasetFeatures features)
    : ctx(0, cfg, frames),
      user_token(user_token),
      submitter(cfg.submitter_config),
      async_SOM(cfg, SOM_DISPLAY_GRID_WIDTH, SOM_DISPLAY_GRID_HEIGHT) {
	SHLOG_D("Triggering main SOM worker");
	async_SOM.start_work(features, ctx.scores, ctx.scores.v());
	for (size_t i = 0; i < MAX_NUM_TEMP_QUERIES; ++i) {
		SHLOG_D("Triggering " << i << " SOM worker");
		temp_async_SOM.push_back(std::make_unique<AsyncSom>(cfg, RELOCATION_GRID_WIDTH, RELOCATION_GRID_HEIGHT));
		temp_async_SOM[i]->start_work(features, ctx.scores, ctx.scores.temp(i));
	}

	/*
	 * Store this initial state into the history
	 */
	ctx.screenshot_fpth = "";
	history.emplace_back(ctx);
}

bool UserContext::operator==(const UserContext& other) const {
	return (ctx == other.ctx && user_token == other.user_token && history == other.history);
}
