/* This file is part of SOMHunter.
 *
 * Copyright (C) 2021 František Mejzlík <frankmejzlik@gmail.com>
 *                    Mirek Kratochvil <exa.exa@gmail.com>
 *                    Patrik Veselý <prtrikvesely@gmail.com>
 *                    Vít Škrhák <v.skrhak@gmail.com>
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

#ifndef HTTP_H_
#define HTTP_H_

#include <memory>
#include <string>
#include <thread>
#include <vector>
// ---
#include <nlohmann/json.hpp>
// ---
#include "common.h"

namespace sh {
using ReqCode = std::size_t;
enum class RequestType { GET, POST };

class Http {
	// *** METHODS ***
public:
	~Http() noexcept;
	// ---
	void do_POST_async(const std::string& URL, const nlohmann::json& body, const nlohmann::json& headers = {},
	                   std::function<void(ReqCode, nlohmann::json)> cb_succ = {}, std::function<void()> cb_err = {});
	void do_GET_async(const std::string& URL, const nlohmann::json& query, const nlohmann::json& headers = {},
	                  std::function<void(ReqCode, nlohmann::json)> cb_succ = {}, std::function<void()> cb_err = {});

	std::pair<ReqCode, std::vector<uint8_t>> do_request_sync(const RequestType request_method, const std::string& URL,
	                                                         const nlohmann::json& body,
	                                                         const nlohmann::json& headers = {});
	std::pair<ReqCode, nlohmann::json> do_POST_sync_json(const std::string& URL, const nlohmann::json& body,
	                                                     const nlohmann::json& headers = {});
	std::pair<ReqCode, nlohmann::json> do_GET_sync_json(const std::string& URL, const nlohmann::json& body,
	                                                    const nlohmann::json& headers = {});
	std::pair<ReqCode, std::vector<float>> do_GET_sync_floats(const std::string& URL, const nlohmann::json& body,
	                                                          const nlohmann::json& headers);
	std::tuple<ReqCode, std::vector<float>, std::vector<int>> do_GET_sync_distances(const std::string& URL,
	                                                                                const nlohmann::json& body,
	                                                                                const nlohmann::json& headers);

	void set_allow_insecure(bool val) { _allow_insecure = val; };
	bool get_allow_insecure() const { return _allow_insecure; };

private:
	void common_finish() { prune_threads(); }

	void prune_threads() {
		for (size_t i = 0; i < submit_threads.size();) {
			if (*finish_flags[i]) {
				submit_threads[i].join();
				submit_threads.erase(submit_threads.begin() + i);
				finish_flags.erase(finish_flags.begin() + i);
			} else {
				++i;
			}
		}
	}

	// *** MEMBER VARIABLES  ***
private:
	std::vector<std::thread> submit_threads;
	std::vector<std::unique_ptr<bool>> finish_flags;

	bool _allow_insecure;
};

};  // namespace sh

#endif  // HTTP_H_