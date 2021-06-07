
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

#ifndef EVAL_SERVER_CLIENT_H_
#define EVAL_SERVER_CLIENT_H_

#include <memory>
#include <string>
#include <vector>
// ---
#include "settings.h"
#include "client-dres.h"


namespace sh {

struct VideoFrame;

class EvalServerClient {
	// *** METHODS ***
public:
	EvalServerClient(const SubmitterConfig& settings);
	// ---
	bool login();
	bool logout();
	bool submit(const VideoFrame& frame);

	// *** MEMBER VARIABLES ***
private:
	SubmitterConfig _submitter_settings;
	std::unique_ptr<IServerClient> _p_client;
	std::string _dummy{ "" };
};

};  // namespace sh

#endif  // EVAL_SERVER_CLIENT_H_
