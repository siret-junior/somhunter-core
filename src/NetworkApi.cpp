
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
#include "SomHunter.h"

#include "NetworkApi.h"
using namespace utility::conversions;
using namespace sh;

static http_response construct_error_res(status_code code, const std::string& msg) {
	json::value o_msg = json::value::string(to_string_t(msg));
	json::value e = json::value::object();
	e[U("message")] = o_msg;
	json::value r = json::value::object();
	r[U("error")] = e;

	http_response res;
	res.set_status_code(code);
	res.set_body(r);
	return res;
}

/**
 *
 * OpenAPI: QueryFilters
 */
json::value to_QueryFilters(SomHunter* /*p_core*/, const SearchContext& search_ctx) {
	json::value result_obj = json::value::object();

	{ /* *** weekdays *** */
		json::value weekdaysArr = json::value::array(7);
		for (size_t i{ 0 }; i < 7; ++i) {
			weekdaysArr[i] = search_ctx.filters.days[i];
		}
		result_obj[U("weekdays")] = weekdaysArr;
	}

	{ /* *** hourFrom *** */
		result_obj[U("hourFrom")] = json::value::number(uint32_t(search_ctx.filters.time.from));
	}

	{ /* *** hourTo *** */
		result_obj[U("hourTo")] = json::value::number(uint32_t(search_ctx.filters.time.to));
	}

	return result_obj;
}

/**
 *
 * OpenAPI: FrameReference
 */
json::value to_FrameReference(SomHunter* /*p_core*/, const VideoFrame* p_frame, const LikesCont& likes,
                              const BookmarksCont& bookmarks, const std::string& path_prefix) {
	json::value result_obj = json::value::object();
	{
		ImageId ID{ IMAGE_ID_ERR_VAL };
		ImageId v_ID{ IMAGE_ID_ERR_VAL };
		ImageId s_ID{ IMAGE_ID_ERR_VAL };

		Hour hour{ ERR_VAL<Hour>() };
		Weekday weekday{ ERR_VAL<Weekday>() };
		std::string LSC_ID{ "" };

		bool is_liked{ false };
		bool is_bookmarked{ false };
		std::string filename{};

		if (p_frame != nullptr) {
			ID = p_frame->frame_ID;
			v_ID = p_frame->video_ID;
			s_ID = p_frame->shot_ID;

			hour = p_frame->hour;
			weekday = p_frame->weekday;

			LSC_ID = p_frame->LSC_id;

			is_liked = (likes.count(ID) > 0 ? true : false);
			filename = path_prefix + p_frame->filename;

			is_bookmarked = (bookmarks.count(ID) > 0 ? true : false);
		}

		{ /* *** id *** */
			if (ID == IMAGE_ID_ERR_VAL) {
				result_obj[U("id")] = json::value::null();
			} else {
				result_obj[U("id")] = json::value::number(uint32_t(ID));
			}
		}

		{ /* *** vId *** */
			if (ID == IMAGE_ID_ERR_VAL) {
				result_obj[U("vId")] = json::value::null();
			} else {
				result_obj[U("vId")] = json::value::number(uint32_t(v_ID));
			}
		}

		{ /* *** sId *** */
			if (ID == IMAGE_ID_ERR_VAL) {
				result_obj[U("sId")] = json::value::null();
			} else {
				result_obj[U("sId")] = json::value::number(uint32_t(s_ID));
			}
		}

		{ /* *** hour *** */
			if (hour == ERR_VAL<Hour>()) {
				result_obj[U("hour")] = json::value::null();
			} else {
				result_obj[U("hour")] = json::value::number(uint32_t(hour));
			}
		}

		{ /* *** weekday *** */
			if (weekday == ERR_VAL<Weekday>()) {
				result_obj[U("weekday")] = json::value::null();
			} else {
				result_obj[U("weekday")] = json::value::number(uint32_t(weekday));
			}
		}

		{ /* *** lscId *** */
			if (LSC_ID.empty()) {
				result_obj[U("lscId")] = json::value::null();
			} else {
				result_obj[U("lscId")] = json::value::string(to_string_t(LSC_ID));
			}
		}

		{ /* *** liked *** */
			result_obj[U("liked")] = json::value::boolean(is_liked);
		}

		{  // *** bookmarked ***
			result_obj[U("bookmarked")] = json::value::boolean(is_bookmarked);
		}

		{ /* *** src *** */
			result_obj[U("src")] = json::value::string(to_string_t(filename));
		}
	}
	return result_obj;
}

/**
 *
 * OpenAPI: Response__User__Context__Get
 */
json::value to_Response__User__Context__Get(SomHunter* p_core, const UserContext& ctx) {
	auto search_ctx{ ctx.ctx };
	auto bookmarks{ ctx.bookmarks };

	// Return structure
	json::value result_obj = json::value::object();

	{ /* *** textQueries *** */

		json::value value_arr{ json::value::array(2) };
		{
			// \todo This should be generalized in the
			// future

			std::string q0{};
			std::string q1{};

			// Scan the query string
			const auto& query{ search_ctx.last_text_query };
			auto idx{ query.find(">>") };

			// If temporal
			if (idx != std::string::npos) {
				q0 = query.substr(0, idx);
				q1 = query.substr(idx + 2);
			}
			// Else simple
			else {
				q0 = query;
			}

			json::value q0_napi = json::value::string(to_string_t(q0));
			json::value q1_napi = json::value::string(to_string_t(q1));
			value_arr[0] = q0_napi;
			value_arr[1] = q1_napi;
		}

		result_obj[U("textQueries")] = value_arr;
	}

	{ /* *** displayType *** */
		auto curr_disp_type{ search_ctx.curr_disp_type };
		auto disp_string{ disp_type_to_str(curr_disp_type) };

		json::value disp_string_napi = json::value::string(to_string_t(disp_string));
		result_obj[U("displayType")] = disp_string_napi;
	}

	{ /* *** screenshotFilepath *** */
		auto str{ search_ctx.screenshot_fpth };

		json::value disp_string_napi = json::value::string(to_string_t(str));
		result_obj[U("screenshotFilepath")] = disp_string_napi;
	}

	{ /* *** ID *** */
		size_t ID{ search_ctx.ID };

		json::value disp_string_napi = json::value::number(ID);
		result_obj[U("id")] = disp_string_napi;
	}

	{ /* *** likedFrames *** */

		json::value likes_arr{ json::value::array(search_ctx.likes.size()) };

		size_t i{ 0 };
		for (auto&& f_ID : search_ctx.likes) {
			const VideoFrame& f{ p_core->get_frame(f_ID) };
			auto fr{ to_FrameReference(p_core, &f, search_ctx.likes, bookmarks, "") };

			likes_arr[i] = fr;
			++i;
		}

		result_obj[U("likedFrames")] = likes_arr;
	}

	{ /* *** filters *** */
		auto fiters{ to_QueryFilters(p_core, search_ctx) };
		result_obj[U("filters")] = fiters;
	}

	return result_obj;
}

/**
 *
 * OpenAPI: Response__GetTopScreen__Post
 */
json::value to_Response__GetTopScreen__Post(SomHunter* p_core, const GetDisplayResult& res, size_t page_num,
                                            const std::string& type, const std::string& path_prefix) {
	const auto& frames{ res.frames };
	const auto& likes{ res.likes };
	const auto& bookmarks{ res.bookmarks };

	// Return structure
	json::value result = json::value::object();

	{ /* *** page *** */
		result[U("page")] = json::value::number(uint32_t(page_num));
	}

	{ /* *** type *** */
		result[U("type")] = json::value::string(to_string_t(type));
	}

	{ /* *** frames *** */
		auto s{ frames.end() - frames.begin() };
		json::value arr{ json::value::array(s) };

		size_t i{ 0 };
		for (auto it{ frames.begin() }; it != frames.end(); ++it) {
			auto fr{ to_FrameReference(p_core, *it, likes, bookmarks, path_prefix) };

			arr[i] = fr;
			++i;
		}

		result[U("frames")] = arr;
	}
	json::value result4 = json::value::object();
	result4[U("screen")] = result;

	json::value result3 = json::value::object();
	result3[U("somhunter")] = result4;

	json::value result2 = json::value::object();
	result2[U("viewData")] = result3;

	return result2;
}

json::value to_Response__GetDetailScreen__Post(SomHunter* p_core, const GetDisplayResult& res, size_t page_num,
                                               const std::string& type, const std::string& path_prefix) {
	const auto& frames{ res.frames };
	const auto& likes{ res.likes };
	const auto& bookmarks{ res.bookmarks };

	// Return structure
	json::value result = json::value::object();

	{ /* *** page *** */
		result[U("page")] = json::value::number(uint32_t(page_num));
	}

	{ /* *** frames *** */
		auto s{ frames.end() - frames.begin() };
		json::value arr{ json::value::array(s) };

		size_t i{ 0 };
		for (auto it{ frames.begin() }; it != frames.end(); ++it) {
			auto fr{ to_FrameReference(p_core, *it, likes, bookmarks, path_prefix) };

			arr[i] = fr;
			++i;
		}

		result[U("frames")] = arr;
	}

	return result;
}

json::value to_Response__GetAutocompleteResults__Get(SomHunter* /*p_core*/, const std::vector<const Keyword*>& kws,
                                                     size_t example_count, const std::string& path_prefix) {
	json::value result_arr{ json::value::array(kws.size()) };

	size_t i = 0ULL;
	// Iterate through all results
	for (auto&& p_kw : kws) {
		json::value result_obj = json::value::object();
		{ /* *** id *** */
			result_obj[U("id")] = json::value::number(uint32_t(p_kw->kw_ID));
		}
		{ /* *** lscId *** */
			result_obj[U("wordString")] = json::value::string(to_string_t(p_kw->synset_strs.front()));
		}
		{ /* *** description *** */
			result_obj[U("description")] = json::value::string(to_string_t(p_kw->desc));
		}
		{ /* *** exampleFrames *** */
			json::value examples_arr{ json::value::array(p_kw->top_ex_imgs.size()) };

			size_t ii{ 0 };
			for (auto&& p_frame : p_kw->top_ex_imgs) {
				if (ii >= example_count) {
					break;
				}

				examples_arr[ii] = json::value::string(to_string_t(path_prefix + p_frame->filename));
				++ii;
			}
			result_obj[U("exampleFrames")] = examples_arr;
		}

		result_arr[i] = result_obj;
		++i;
	}

	return result_arr;
}

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

	push_endpoint("get_top_screen", {}, &NetworkApi::handle__get_top_screen__POST);
	push_endpoint("get_som_screen", {}, &NetworkApi::handle__get_SOM_screen__POST);
	push_endpoint("get_frame_detail_data", &NetworkApi::handle__get_frame_detail_data__GET);

	push_endpoint("get_autocomplete_results", &NetworkApi::handle__get_autocomplete_results__GET);

	push_endpoint("log_scroll", &NetworkApi::handle__log_scroll__GET);
	push_endpoint("log_test_query_change", &NetworkApi::handle__log_test_query_change__GET);
	push_endpoint("submit_frame", {}, &NetworkApi::handle__submit_frame__POST);
	push_endpoint("login_to_DRES", {}, &NetworkApi::handle__login_to_DRES__POST);

	push_endpoint("reset_search_session", {}, &NetworkApi::handle__reset_search_session__POST);
	push_endpoint("rescore", {}, &NetworkApi::handle__rescore__POST);

	push_endpoint("like_frame", {}, &NetworkApi::handle__like_frame__POST);
	push_endpoint("search/bookmark", {}, &NetworkApi::handle__search__bookmark__POST);

	push_endpoint("search/context", &NetworkApi::handle__search__context__GET,
	              &NetworkApi::handle__search__context__POST);

	LOG_S("Listening for requests at: " << _base_addr);
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

void NetworkApi::push_endpoint(const std::string& path, std::function<void(NetworkApi*, http_request)> GET_handler,
                               std::function<void(NetworkApi*, http_request)> POST_handler,
                               std::function<void(NetworkApi*, http_request)> PUT_handler,
                               std::function<void(NetworkApi*, http_request)> DEL_handler) {
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
		LOG_D("Listener for '" << path << "' set.");
	} catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
	}
}

void NetworkApi::handle__settings__GET(http_request req) {
	auto remote_addr{ to_utf8string(req.remote_address()) };
	LOG_REQUEST(remote_addr, "handle__settings__GET");

	// auto b = message.extract_json().get();

	std::error_code ec;
	auto j{ json::value::parse(read_whole_file(_p_core->get_config_filepath()), ec) };
	if (ec) {
		std::string msg{ ec.message() };
		LOG_E(msg);
		throw std::runtime_error(msg);
	}

	http_response response(status_codes::OK);
	response.set_body(j);
	NetworkApi::add_CORS_headers(response);
	req.reply(response);
}

void NetworkApi::handle__user__context__GET(http_request req) {
	auto remote_addr{ to_utf8string(req.remote_address()) };
	LOG_REQUEST(remote_addr, "handle__user__context__GET");

	auto body = req.extract_json().get();
	// \ytbi
	// size_t user_ID{ body[U("user_ID")].as_integer() };
	// std::string user_token{ to_utf8string(body[U("auth_token")].as_string()) };

	// Fetch the data
	const UserContext& user_ctx{ _p_core->get_user_context() };
	json::value res_data{ to_Response__User__Context__Get(_p_core, user_ctx) };

	// Construct the response
	http_response response(status_codes::OK);
	response.set_body(res_data);

	// Send the response
	NetworkApi::add_CORS_headers(response);
	req.reply(response);
}

void NetworkApi::handle__get_top_screen__POST(http_request req) {
	auto remote_addr{ to_utf8string(req.remote_address()) };
	LOG_REQUEST(remote_addr, "handle__get_top_screen__POST");

	auto body = req.extract_json().get();

	ImageId frame_ID{ static_cast<ImageId>(body[U("frameId")].as_integer()) };
	size_t page_idx{ static_cast<size_t>(body[U("pageId")].as_integer()) };
	std::string type{ to_utf8string(body[U("type")].as_string()) };

	auto dtype{ str_to_disp_type(type) };

	// Fetch the data
	auto display_frames{ _p_core->get_display(dtype, frame_ID, page_idx) };
	json::value res_data{ to_Response__GetTopScreen__Post(_p_core, display_frames, page_idx, type, "") };

	// Construct the response
	http_response res(status_codes::OK);
	res.set_body(res_data);

	// Send the response
	NetworkApi::add_CORS_headers(res);
	req.reply(res);
}

void NetworkApi::handle__get_SOM_screen__POST(http_request req) {
	auto remote_addr{ to_utf8string(req.remote_address()) };
	LOG_REQUEST(remote_addr, "handle__get_SOM_screen__POST");

	auto body = req.extract_json().get();

	/*ImageId frame_ID{ body[U("frameId")].as_integer() };
	size_t page_idx{ body[U("pageId")].as_integer() };*/
	// std::string type{ to_utf8string(body[U("type")].as_string()) };

	auto dtype{ str_to_disp_type("SOM_display") };

	// Fetch the data
	auto display_frames{ _p_core->get_display(dtype) };
	json::value res_data{ to_Response__GetTopScreen__Post(_p_core, display_frames, 0, "SOM_display", "") };

	// Construct the response
	http_response res(status_codes::OK);
	res.set_body(res_data);

	// Send the response
	NetworkApi::add_CORS_headers(res);
	req.reply(res);
}

void NetworkApi::handle__get_frame_detail_data__GET(http_request req) {
	auto remote_addr{ to_utf8string(req.remote_address()) };
	LOG_REQUEST(remote_addr, "handle__get_frame_detail_data__GET");

	// auto paths = http::uri::split_path(http::uri::decode(req.relative_uri().path()));

	auto query{ req.relative_uri().query() };
	auto query_map{ web::uri::split_query(query) };

	size_t frame_ID{ ERR_VAL<size_t>() };
	try {
		frame_ID = static_cast<size_t>(str_to_int(to_utf8string(query_map[U("frameId")])));
	} catch (...) {
	}

	if (frame_ID == ERR_VAL<size_t>()) {
		http_response res{ construct_error_res(status_codes::BadRequest, "Invalid `frameId` parameter.") };
		NetworkApi::add_CORS_headers(res);
		req.reply(res);
		return;
	}

	bool log_it{ true };
	if (query_map.count(U("logIt")) > 0) {
		log_it = (to_utf8string(query_map[U("logIt")]) == "true" ? true : false);
	}

	// Fetch the data
	auto display_frames{ _p_core->get_display(DisplayType::DVideoDetail, frame_ID, 0, log_it) };
	json::value res_data{ to_Response__GetDetailScreen__Post(_p_core, display_frames, 0,
		                                                     disp_type_to_str(DisplayType::DVideoDetail), "") };

	// Construct the response
	http_response res(status_codes::OK);
	res.set_body(res_data);

	// Send the response
	NetworkApi::add_CORS_headers(res);
	req.reply(res);
}

void NetworkApi::handle__get_autocomplete_results__GET(http_request req) {
	auto remote_addr{ to_utf8string(req.remote_address()) };
	LOG_REQUEST(remote_addr, "handle__get_autocomplete_results__GET");

	auto query{ req.relative_uri().query() };
	auto query_map{ web::uri::split_query(query) };

	auto record{ query_map.find(U("queryValue")) };
	std::string prefix{ (record != query_map.end()) ? to_utf8string(record->second) : "" };

	// Empty body
	if (prefix.empty()) {
		http_response res{ construct_error_res(status_codes::BadRequest, "Invalid `queryValue` parameter.") };
		NetworkApi::add_CORS_headers(res);
		req.reply(res);
		return;
	}

	// \todo
	size_t count{ 10 };
	auto record_count{ query_map.find(U("count")) };

	// Fetch the data
	auto keywords{ _p_core->autocomplete_keywords(prefix, count) };
	json::value res_data{ to_Response__GetAutocompleteResults__Get(_p_core, keywords, count, "") };

	// Construct the response
	http_response res(status_codes::OK);
	res.set_body(res_data);

	// Send the response
	NetworkApi::add_CORS_headers(res);
	req.reply(res);
}

void NetworkApi::handle__log_scroll__GET(http_request req) {}

void NetworkApi::handle__log_test_query_change__GET(http_request req) {}

void NetworkApi::handle__submit_frame__POST(http_request req) {
	auto remote_addr{ to_utf8string(req.remote_address()) };
	LOG_REQUEST(remote_addr, "handle__submit_frame__POST");

	ImageId frame_ID{ ERR_VAL<ImageId>() };
	auto body = req.extract_json().get();
	try {
		frame_ID = static_cast<ImageId>(body[U("frameId")].as_integer());
	} catch (...) {
		http_response res{ construct_error_res(status_codes::BadRequest, "Invalid `frameId` parameter.") };
		NetworkApi::add_CORS_headers(res);
		req.reply(res);
		return;
	}

	// Submit it!
	_p_core->submit_to_server(frame_ID);

	// Construct the response
	http_response res(status_codes::OK);
	NetworkApi::add_CORS_headers(res);
	req.reply(res);
}

void NetworkApi::handle__login_to_DRES__POST(http_request req) {}

void NetworkApi::handle__reset_search_session__POST(http_request req) {}

void NetworkApi::handle__rescore__POST(http_request req) {}

void NetworkApi::handle__like_frame__POST(http_request req) {}

void NetworkApi::handle__search__bookmark__POST(http_request req) {}

void NetworkApi::handle__search__context__POST(http_request req) {}

void NetworkApi::handle__search__context__GET(http_request req) {}
