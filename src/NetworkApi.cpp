
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

#include "NetworkApi.h"

using namespace std;

#include "SomHunter.h"
using namespace sh;

void NetworkApi::add_CORS_headers(http_response& res) {
	// Let the client know we approve of this
	res.headers().add(U("Access-Control-Allow-Origin"), U("*"));
}

NetworkApi::NetworkApi(const ApiConfig& API_config, SomHunter* p_core)
    : _API_config{ API_config },
      _p_core{ p_core },
      _base_addr{ "http://127.0.0.1:" + std::to_string(API_config.port) } {}

void NetworkApi::initialize() {
	// Add all desired endpoints
	push_endpoint("settings", &NetworkApi::handle__settings__GET);
	push_endpoint("user/context", &NetworkApi::handle__user__context__GET);

	info_d("Listening for requests at: " << _base_addr);
}

void NetworkApi::terminate() {
	for (auto&& ep : _endpoints) {
		ep.close();
	}
}

void NetworkApi::run() {
	initialize();
	std::cout << "Press ENTER to exit." << std::endl;

	std::string line;
	std::getline(std::cin, line);

	terminate();
}

void NetworkApi::push_endpoint(const std::string& path, std::function<void(NetworkApi*, http_request&)> GET_handler,
                               std::function<void(NetworkApi*, http_request&)> POST_handler,
                               std::function<void(NetworkApi*, http_request&)> PUT_handler,
                               std::function<void(NetworkApi*, http_request&)> DEL_handler) {
	uri_builder endpoint(utility::conversions::to_string_t(_base_addr));
	endpoint.append_path(utility::conversions::to_string_t(path));

	try {
		http_listener ep_listener{ endpoint.to_uri().to_string() };

		if (GET_handler) {
			ep_listener.support(methods::GET, std::bind(GET_handler, this, std::placeholders::_1));
		}

		if (POST_handler) {
			ep_listener.support(methods::POST, std::bind(POST_handler, this, std::placeholders::_1));
		}

		if (PUT_handler) {
			ep_listener.support(methods::PUT, std::bind(PUT_handler, this, std::placeholders::_1));
		}

		if (DEL_handler) {
			ep_listener.support(methods::DEL, std::bind(DEL_handler, this, std::placeholders::_1));
		}

		ep_listener.open().wait();
		_endpoints.emplace_back(std::move(ep_listener));
		debug_d("Listener for '" << path << "' set.");
	} catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
	}
}

void NetworkApi::handle__settings__GET(http_request& message) {
	ucout << message.to_string() << endl;

	auto b = message.extract_json().get();

	std::error_code ec;
	auto j{ json::value::parse(read_whole_file(_p_core->get_config_filepath()), ec) };
	if (ec) {
		std::string msg{ ec.message() };
		warn_d(msg);
		throw runtime_error(msg);
	}

	http_response response(status_codes::OK);
	response.set_body(j);
	NetworkApi::add_CORS_headers(response);
	message.reply(response);
}

void NetworkApi::handle__user__context__GET(http_request& message) {
	ucout << message.to_string() << endl;

	auto b = message.extract_json().get();

	std::error_code ec;
	auto j{ json::value::parse(read_whole_file(_p_core->get_config_filepath()), ec) };
	if (ec) {
		std::string msg{ ec.message() };
		warn_d(msg);
		throw runtime_error(msg);
	}

	http_response response(status_codes::OK);
	response.set_body(j);
	NetworkApi::add_CORS_headers(response);
	message.reply(response);
}