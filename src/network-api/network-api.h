
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

#ifndef NETWORK_API_H_
#define NETWORK_API_H_

#include <mutex>
// ---
#include <cpprest/asyncrt_utils.h>
#include <cpprest/containerstream.h>
#include <cpprest/filestream.h>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <cpprest/producerconsumerstream.h>
#include <cpprest/uri.h>
// ---
#include "common.h"
#include "query-types.h"

using namespace web;                           //< cpprest
using namespace http;                          //< cpprest
using namespace utility;                       //< cpprest
using namespace http::experimental::listener;  //< cpprest

namespace sh
{
class Somhunter;

class NetworkApi
{
public:
	static void add_CORS_headers(http_response& res);

	NetworkApi() = delete;
	NetworkApi(const ApiConfig& API, Somhunter* p_core);

	void run();
	void initialize();
	void terminate();

private:
	void push_endpoint(
	    const std::string& path,
	    std::function<void(NetworkApi*, http_request)> GET_handler = std::function<void(NetworkApi*, http_request)>{},
	    std::function<void(NetworkApi*, http_request)> POST_handler = std::function<void(NetworkApi*, http_request)>{},
	    std::function<void(NetworkApi*, http_request)> PUT_handler = std::function<void(NetworkApi*, http_request)>{},
	    std::function<void(NetworkApi*, http_request)> DEL_handler = std::function<void(NetworkApi*, http_request)>{});

	// *** Handlers ***
	void handle__api__GET(http_request req);
	void handle__api__config__GET(http_request req);

	void handle__config__GET(http_request req);
	void handle__user__context__GET(http_request req);

	void handle__dataset__video_detail__GET(http_request req);

	void handle__search__get_top_display__POST(http_request req);
	void handle__search__get_som_display__POST(http_request req);
	void handle__search__get_som_relocation_display__POST(http_request req);
	void handle__search__keyword_autocomplete__GET(http_request req);

	void handle__search__reset__POST(http_request req);
	void handle__search__rescore__POST(http_request req);
	void handle__search__like_frame__POST(http_request req);
	void handle__search__bookmark_frame__POST(http_request req);
	void handle__search__context__POST(http_request req);
	void handle__search__context__GET(http_request req);

	void handle__log__scroll__GET(http_request req);
	void handle__log__text_query_change__GET(http_request req);
	void handle__log__canvas_query_change__GET(http_request req);

	void handle__eval_server__submit__POST(http_request req);
	void handle__eval_server__login__POST(http_request req);
	void handle__eval_server__logout__POST(http_request req);

	// *** Helpers ***
	RescoreMetadata extract_rescore_metadata(web::json::value& body);
	std::vector<TextualQuery> extract_textual_query(web::json::value& body);
	std::vector<RelocationQuery> extract_relocation_query(web::json::value& body);
	std::vector<CanvasQuery> extract_canvas_query(web::json::value& body);
	Filters extract_filters(web::json::value& body);

private:
	ApiConfig _API_config;
	Somhunter* _p_core;
	std::string _base_addr;
	std::vector<http_listener> _endpoints;

	mutable std::mutex _req_mtx;
	std::lock_guard<std::mutex> exclusive_lock() const { return std::lock_guard<std::mutex>{ _req_mtx }; };
};

};  // namespace sh

#endif  // NETWORK_API_H_
