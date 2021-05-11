
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

#include "DatasetFeatures.h"

#include <exception>
#include <fstream>

#include "config_json.h"
#include "log.h"

using namespace sh;

DatasetFeatures::DatasetFeatures(const DatasetFrames& p, const Config& config)
    : n(p.size()), features_dim(config.features_dim) {
	data.resize(features_dim * n);
	std::ifstream in(config.features_file, std::ios::binary);
	if (!in.good()) {
		std::string msg{ "Error opening file with features: " + config.features_file };
		LOG_E(msg);
		throw std::runtime_error(msg);
	}

	// Skip the header
	in.ignore(config.features_file_data_off);

	if (!in.read(reinterpret_cast<char*>(data.data()), sizeof(float) * data.size())) {
		std::string msg{ "Feature matrix reading problems: " + config.features_file };
		LOG_E(msg);
		throw std::runtime_error(msg);
	} else {
		LOG_I("Feature matrix loaded OK");
	}
}
