
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

#ifndef NETWORK_API_H_
#define NETWORK_API_H_

#include "cpprest/asyncrt_utils.h"
#include "cpprest/containerstream.h"
#include "cpprest/filestream.h"
#include "cpprest/http_listener.h"
#include "cpprest/json.h"
#include "cpprest/producerconsumerstream.h"
#include "cpprest/uri.h"

using namespace web;
using namespace http;
using namespace utility;
using namespace http::experimental::listener;

#include "config_json.h"
#include "log.h"

namespace sh {

class SomHunter;

class NetworkApi {
public:
	static void add_CORS_headers(http_response& res);

	NetworkApi() = delete;
	NetworkApi(const ApiConfig& API_config, SomHunter* p_core);

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

	void handle__get_top_screen__POST(http_request req);
	void handle__get_SOM_screen__POST(http_request req);
	void handle__get_SOM_relocation_screen__POST(http_request req);
	void handle__get_frame_detail_data__GET(http_request req);
	void handle__get_autocomplete_results__GET(http_request req);

	void handle__log_scroll__GET(http_request req);
	void handle__log_text_query_change__GET(http_request req);

	void handle__submit_frame__POST(http_request req);
	void handle__login_to_DRES__POST(http_request req);

	void handle__reset_search_session__POST(http_request req);
	void handle__rescore__POST(http_request req);

	void handle__like_frame__POST(http_request req);
	void handle__search__bookmark__POST(http_request req);

	void handle__search__context__POST(http_request req);
	void handle__search__context__GET(http_request req);

	// *** Helpers ***
	RescoreMetadata extract_rescore_metadata(web::json::value& body);
	std::vector<TextualQuery> extract_textual_query(web::json::value& body);
	std::vector<RelocationQuery> extract_relocation_query(web::json::value& body);
	std::vector<CanvasQuery> extract_canvas_query(web::json::value& body);
	Filters extract_filters(web::json::value& body);

private:
	ApiConfig _API_config;
	SomHunter* _p_core;
	std::string _base_addr;
	std::vector<http_listener> _endpoints;
};

};  // namespace sh

#endif  // NETWORK_API_H_
