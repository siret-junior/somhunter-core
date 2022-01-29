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

#include "config-tests.h"

namespace sh {
class Somhunter;
struct Settings;

namespace tests {
class TESTER_Somhunter {
public:
	static void run_all_tests(const std::string &cfg_fpth);

private:
	static void TEST_like_frames(Somhunter &core);
	static void TEST_bookmark_frames(Somhunter &core);
	static void TEST_autocomplete_keywords(Somhunter &core);
	static void TEST_rescore(Somhunter &core);
	static void TEST_rescore_filters(Somhunter &core);
	static void TEST_canvas_queries(Somhunter &core);

	static void TEST_log_results(Somhunter &core);
};

class TESTER_Config {
public:
	static void run_all_tests(const std::string & /*cfg_fpth*/);

private:
	static void TEST_parse_JSON_config(const Settings &c);

	static void TEST_LSC_addition(const Settings &config);

	static void TEST_collage_addition(const Settings &config);
};

};  // namespace tests
};  // namespace sh
