/* This file is part of SOMHunter.
 *
 * Copyright (C) 2021 Frantisek Mejzlik <frankmejzlik@gmail.com>
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

namespace sh {

class Somhunter;

/**
 * Class responsible for listening for HTTP requests and for handlling them using the core.
 *
 * This is the universal API to the SOMHunter Core if you're not able to call the native C++ API of \ref class
 * Somhunter directly.
 *
 * \see class Somhunter
 *
 * \remark If you're on Windows and you want to listen for requests from public network, you need to have Administrator
 * privilages, otherwise the listener initialization will fail.
 */
class NetworkApi {
public:
	/** Only single ctor overload is acceptable. */
	NetworkApi() = delete;

	/** Constructs the HTTP API instance for the given core and with the given API config. */
	NetworkApi(const ApiConfig& API, Somhunter* p_core);

	// ---

	/** Initializes the HTTP listeners on the required endpoints. */
	void initialize();

	/** The "main" function of the HTTP API - calls both `initialize` & `terminate`. */
	void run();

	/** Safely terminates all the endpoint listeners. */
	void terminate();

private:
	/** Adds the CORS headers to the given HTTP response instance. */
	static void add_CORS_headers(http_response& res);

	// ---
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

	/**
	 * Registers the provided handlers for the given `path` in the HTTP listener.
	 *
	 * \param	path	String denoting the path of this endpoint (e.g. `/api/do-something`).
	 * \param	GET_handler	Handler to call in case of GET request.
	 * \param	POST_handler	Handler to call in case of POST request.
	 * \param	PUT_handler	Handler to call in case of PUT request.
	 * \param	DEL_handler	Handler to call in case of DEL request.
	 * \returns		void
	 */
	void push_endpoint(
	    const std::string& path,
	    std::function<void(NetworkApi*, http_request)> GET_handler = std::function<void(NetworkApi*, http_request)>{},
	    std::function<void(NetworkApi*, http_request)> POST_handler = std::function<void(NetworkApi*, http_request)>{},
	    std::function<void(NetworkApi*, http_request)> PUT_handler = std::function<void(NetworkApi*, http_request)>{},
	    std::function<void(NetworkApi*, http_request)> DEL_handler = std::function<void(NetworkApi*, http_request)>{});

	/**
	 * Extracts \ref RescoreMetadata from the `body` variable.
	 *
	 * \param	body	JSON structure to be parsed.
	 * \returns		New instance of parsed \ref RescoreMetadata structure.
	 */
	RescoreMetadata extract_rescore_metadata(web::json::value& body);

	/**
	 * Extracts a vector of \ref TextualQuery from the `body` variable.
	 *
	 * \param	body	JSON structure to be parsed.
	 * \returns		New instance of parsed vector of \ref TextualQuery structure.
	 */
	std::vector<TextualQuery> extract_textual_query(web::json::value& body);

	/**
	 * Extracts a vector of \ref RelocationQuery from the `body` variable.
	 *
	 * \param	body	JSON structure to be parsed.
	 * \returns		New instance of parsed vector of \ref RelocationQuery structure.
	 */
	std::vector<RelocationQuery> extract_relocation_query(web::json::value& body);

	/**
	 * Extracts a vector of \ref CanvasQuery from the `body` variable.
	 *
	 * \param	body	JSON structure to be parsed.
	 * \returns		New instance of parsed vector of \ref CanvasQuery structure.
	 */
	std::vector<CanvasQuery> extract_canvas_query(web::json::value& body);

	/**
	 * Extracts \ref Filters from the `body` variable.
	 *
	 * \param	body	JSON structure to be parsed.
	 * \returns		New instance of parsed \ref Filters structure.
	 */
	Filters extract_filters(web::json::value& body);

	// *** Synchronization ***
	std::lock_guard<std::mutex> exclusive_lock() const { return std::lock_guard<std::mutex>{ _req_mtx }; };

	// ---

	/** A private copy of config. */
	ApiConfig _API_config;

	/** A pointer to the core that this API exposes. */
	Somhunter* _p_core;

	/** Base HTTP API address. */
	std::string _base_addr;

	/** Vector of used endpoints. */
	std::vector<http_listener> _endpoints;

	/** Lock used to synchronize some requests. */
	mutable std::mutex _req_mtx;
};

};  // namespace sh

#endif  // NETWORK_API_H_
