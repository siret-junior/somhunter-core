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

// !!!
#include "somhunter.h"  // Do NOT move this below other includes -> libtorch hell
// !!!
#include "common.h"
#include "network-api.h"
#include "os-utils.hpp"
#include "utils.hpp"

using namespace sh;

int main() {
	// Some OS preparation may be needed
	osutils::initialize_aplication();

	// Instantiate the SOMHunter
	Somhunter core{ "config/config-core.json" };

	// Start the HTTP API
	NetworkApi api{ core.settings().API, &core };
	api.run();  //< Type "exit" to stop the server

	// Run optional utility stuff
	core.run_generators();  //< Optional
	core.run_basic_test();  //< Optional

	return 0;
}
