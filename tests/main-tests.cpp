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

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "tests.h"

using namespace std;

int main(int /*argc*/, char** /*argv*/) {
	auto path = std::filesystem::current_path();
	std::filesystem::current_path(path.parent_path());
	std::cout << "CD:" << path.string() << std::endl;

	std::cout << "Running main() from " << __FILE__ << std::endl;

	const std::string cfg_fpth{ "config/config-core.json" };

	sh::tests::TESTER_Somhunter::run_all_tests(cfg_fpth);
	// TODO update config tests
	// sh::tests::TESTER_Config::run_all_tests(cfg_fpth);

	return 0;
}
