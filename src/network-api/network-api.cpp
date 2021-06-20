
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
// This include has to be first
// otherwise Torch will not build
// !!!
#include "somhunter.h"

#include "network-api.h"

#include <algorithm>

using namespace utility::conversions;  //< cpprest

using namespace sh;

static http_response construct_error_res(status_code code, const std::string& msg)
{
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
 * Writes the provided image as JPEG file placed at `filepath`, returns number of written bytes.
 */
static size_t store_JPEG_from_base64(const std::string& /*filepath*/, const std::string& /*base64_data*/)
{
	// \todo
	SHLOG_D("Simulating the JPEG screenshot write to '" << "filepath" << "'...");
	return 128;
}

template <typename _ElemType>
std::vector<_ElemType> from_JSON_array(json::value x)
{
	auto arr{ x.as_array() };

	std::vector<_ElemType> res;
	res.reserve(arr.size());
	try {
		for (size_t i{ 0 }; i < arr.size(); ++i) {
			res.emplace_back(static_cast<_ElemType>(arr.at(i).as_integer()));
		}

	} catch (std::exception& e) {
		SHLOG_D(e.what());
	}

	return res;
}

template <typename _ElemType>
std::vector<_ElemType> from_double_array(json::value x)
{
	auto arr{ x.as_array() };

	std::vector<_ElemType> res;
	res.reserve(arr.size());
	try {
		for (size_t i{ 0 }; i < arr.size(); ++i) {
			res.emplace_back(static_cast<_ElemType>(arr.at(i).as_double()));
		}

	} catch (std::exception& e) {
		SHLOG_D(e.what());
	}

	return res;
}

/**
 *
 * OpenAPI: QueryFilters
 */
json::value to_QueryFilters(Somhunter* /*p_core*/, const SearchContext& search_ctx)
{
	json::value result_obj = json::value::object();

	{ /* *** weekdays *** */
		json::value weekdaysArr = json::value::array(7);
		for (size_t i{ 0 }; i < 7; ++i) {
			weekdaysArr[i] = json::value::boolean(search_ctx.filters.days[i]);
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
json::value to_FrameReference(Somhunter* /*p_core*/, const VideoFrame* p_frame, const LikesCont& likes,
                              const BookmarksCont& _bookmarks, const std::string& path_prefix)
{
	json::value result_obj = json::value::object();
	{
		FrameId ID{ IMAGE_ID_ERR_VAL };
		FrameId v_ID{ IMAGE_ID_ERR_VAL };
		FrameId s_ID{ IMAGE_ID_ERR_VAL };

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

			is_bookmarked = (_bookmarks.count(ID) > 0 ? true : false);
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

json::value canvas_to_json(const CanvasSubqueryBitmap& q)
{
	json::value res = json::value::object();
	res[U("rect")] = json::value::array({ q.rect().left, q.rect().top, q.rect().right, q.rect().bottom });
	res[U("type")] = json::value::string(U("bitmap"));
	res[U("width_pixels")] = json::value::number(q.width_pixels());
	res[U("height_pixels")] = json::value::number(q.height_pixels());
	res[U("num_channels")] = json::value::number(q.num_channels());

	{  // Copy image data
		json::value arr = json::value::array(q.data().size());
		for (std::size_t i = 0; i < q.data().size(); ++i) {
			arr[i] = json::value::number(q.data()[i]);
		}
		res[U("bitmap_data")] = arr;
	}

	return res;
}

json::value canvas_to_json(const CanvasSubqueryText& q)
{
	json::value res = json::value::object();
	res[U("rect")] = json::value::array({ q.rect().left, q.rect().top, q.rect().right, q.rect().bottom });
	res[U("type")] = json::value::string(U("text"));
	res[U("text_query")] = json::value::string(to_string_t(q.query()));
	return res;
}

json::value to_SearchContext(Somhunter* p_core, const UserContext& ctx)
{
	auto search_ctx{ ctx.ctx };
	auto _bookmarks{ ctx._bookmarks };

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
			if (search_ctx.last_temporal_queries.size() > 0) q0 = search_ctx.last_temporal_queries[0].textual;
			if (search_ctx.last_temporal_queries.size() > 1) q1 = search_ctx.last_temporal_queries[1].textual;

			json::value q0_napi = json::value::string(to_string_t(q0));
			json::value q1_napi = json::value::string(to_string_t(q1));
			value_arr[0] = q0_napi;
			value_arr[1] = q1_napi;
		}

		result_obj[U("textQueries")] = value_arr;
	}

	{ /* *** relocation *** */

		json::value value_arr{ json::value::array(2) };
		{
			// \todo This should be generalized in the
			// future

			FrameId q0{ IMAGE_ID_ERR_VAL };
			FrameId q1{ IMAGE_ID_ERR_VAL };

			// Scan the query string
			if (search_ctx.last_temporal_queries.size() > 0) q0 = search_ctx.last_temporal_queries[0].relocation;
			if (search_ctx.last_temporal_queries.size() > 1) q1 = search_ctx.last_temporal_queries[1].relocation;

			json::value q0_napi = to_FrameReference(p_core, p_core->get_frame_ptr(q0), {}, {}, "");
			json::value q1_napi = to_FrameReference(p_core, p_core->get_frame_ptr(q1), {}, {}, "");
			value_arr[0] = q0_napi;
			value_arr[1] = q1_napi;
		}

		result_obj[U("relocation")] = value_arr;
	}

	{ /* *** canvas *** */

		json::value value_arr{ json::value::array(2) };
		for (std::size_t i = 0; i < 2; ++i) {
			if (i < search_ctx.last_temporal_queries.size() && search_ctx.last_temporal_queries[i].is_canvas()) {
				// Push canvas query
				TemporalQuery& tempQ = search_ctx.last_temporal_queries[i];
				value_arr[i] = json::value::array(tempQ.canvas.size());
				for (std::size_t j = 0; j < tempQ.canvas.size(); ++j) {
					// Push canvas subquery
					value_arr[i][j] = std::visit(
					    overloaded{
					        [](auto sq) { return canvas_to_json(sq); },
					    },
					    tempQ.canvas[j]);
				}
			} else {
				// Create empty query
				value_arr[i] = json::value::array(0);
			}
		}

		result_obj[U("canvas")] = value_arr;
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
			auto fr{ to_FrameReference(p_core, &f, search_ctx.likes, _bookmarks, "") };

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

json::value to_HistoryArray(Somhunter* /*p_core*/, const std::vector<SearchContext>& _history)
{
	json::value history_arr{ json::value::array(_history.size()) };

	size_t i{ 0 };
	for (auto&& ctx : _history) {
		json::value hist_point{ json::value::object() };

		{  // *** id ***
			hist_point[U("id")] = json::value::number(uint32_t(ctx.ID));
		}

		{  // *** screenshotFilepath ***
			hist_point[U("screenshotFilepath")] = json::value::string(to_string_t(ctx.screenshot_fpth));
		}

		{  // *** time ***
			hist_point[U("time")] = json::value::string(to_string_t(ctx.label));
		}
		history_arr[i] = hist_point;
		++i;
	}

	return history_arr;
}

/**
 *
 * OpenAPI: Response__User__Context__Get
 */
json::value to_Response__User__Context__Get(Somhunter* p_core, const UserContext& ctx)
{
	auto search_ctx{ ctx.ctx };
	auto _bookmarks{ ctx._bookmarks };

	// Return structure
	json::value result_obj = json::value::object();

	{ /* *** search *** */
		result_obj[U("search")] = to_SearchContext(p_core, ctx);
	}

	{ /* *** history *** */
		result_obj[U("history")] = to_HistoryArray(p_core, ctx._history);
	}

	{ /* *** bookmarkedFrames *** */
		auto bookmarked_arr{ json::value::array(ctx._bookmarks.size()) };

		size_t i{ 0 };
		for (auto&& b : ctx._bookmarks) {
			bookmarked_arr[i] = to_FrameReference(p_core, p_core->get_frame_ptr(b), search_ctx.likes, _bookmarks, "");
			++i;
		}
		result_obj[U("bookmarkedFrames")] = bookmarked_arr;
	}

	{ /* *** targets *** */
		json::value arr{ json::value::array(2) };
		size_t i{ 0 };
		for (auto&& f : ctx.ctx.curr_targets) {
			auto fr{ to_FrameReference(p_core, &f, {}, {}, "") };

			arr[i] = fr;
			++i;
		}
		result_obj[U("targets")] = arr;
	}

	return result_obj;
}

/**
 *
 * OpenAPI: Response__GetTopScreen__Post
 */
json::value to_Response__GetTopScreen__Post(Somhunter* p_core, const GetDisplayResult& res, size_t page_num,
                                            const std::string& type, const std::string& path_prefix)
{
	const auto& _dataset_frames{ res._dataset_frames };
	const auto& likes{ res.likes };
	const auto& _bookmarks{ res._bookmarks };

	// Return structure
	json::value result = json::value::object();

	{ /* *** page *** */
		result[U("page")] = json::value::number(uint32_t(page_num));
	}

	{ /* *** type *** */
		result[U("type")] = json::value::string(to_string_t(type));
	}

	{ /* *** frames *** */
		auto s{ _dataset_frames.end() - _dataset_frames.begin() };
		json::value arr{ json::value::array(s) };

		size_t i{ 0 };
		for (auto it{ _dataset_frames.begin() }; it != _dataset_frames.end(); ++it) {
			auto fr{ to_FrameReference(p_core, *it, likes, _bookmarks, path_prefix) };

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

json::value to_Response__GetDetailScreen__Post(Somhunter* p_core, const GetDisplayResult& res, size_t page_num,
                                               const std::string& /*type*/, const std::string& path_prefix)
{
	const auto& _dataset_frames{ res._dataset_frames };
	const auto& likes{ res.likes };
	const auto& _bookmarks{ res._bookmarks };

	// Return structure
	json::value result = json::value::object();

	{ /* *** page *** */
		result[U("page")] = json::value::number(uint32_t(page_num));
	}

	{ /* *** frames *** */
		auto s{ _dataset_frames.end() - _dataset_frames.begin() };
		json::value arr{ json::value::array(s) };

		size_t i{ 0 };
		for (auto it{ _dataset_frames.begin() }; it != _dataset_frames.end(); ++it) {
			auto fr{ to_FrameReference(p_core, *it, likes, _bookmarks, path_prefix) };

			arr[i] = fr;
			++i;
		}

		result[U("frames")] = arr;
	}

	return result;
}

json::value to_Response__GetAutocompleteResults__Get(Somhunter* /*p_core*/, const std::vector<const Keyword*>& kws,
                                                     size_t example_count, const std::string& path_prefix)
{
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

json::value to_Response__Rescore__Post(Somhunter* p_core, const RescoreResult& rescore_res)
{
	size_t curr_ctx_ID{ rescore_res.curr_ctx_ID };
	const auto& _history{ rescore_res._history };

	json::value result_obj = json::value::object();

	{ /* *** id *** */
		result_obj[U("currId")] = json::value::number(uint32_t(curr_ctx_ID));
	}

	{ /* *** history *** */
		result_obj[U("history")] = to_HistoryArray(p_core, _history);
	}

	{ /* *** targets *** */
		json::value arr{ json::value::array(2) };
		size_t i{ 0 };
		for (auto&& f : rescore_res.targets) {
			auto fr{ to_FrameReference(p_core, &f, {}, {}, "") };

			arr[i] = fr;
			++i;
		}
		result_obj[U("targets")] = arr;
	}
	{ /* *** target_position *** */
		result_obj[U("target_position")] = rescore_res.target_pos;
	}
	return result_obj;
}

void NetworkApi::add_CORS_headers(http_response& res)
{
	// Let the client know we approve of this
	res.headers().add(U("Access-Control-Allow-Origin"), U("*"));
}

void handle_options(http_request request)
{
	auto remote_addr{ to_utf8string(request.remote_address()) };
	SHLOG_REQ(remote_addr, "handle_options");

	http_response response(status_codes::OK);
	response.headers().add(U("Allow"), U("GET, POST, OPTIONS"));
	response.headers().add(U("Access-Control-Allow-Origin"), U("*"));
	response.headers().add(U("Access-Control-Allow-Methods"), U("GET, POST, OPTIONS"));
	response.headers().add(U("Access-Control-Allow-Headers"), U("Content-Type"));
	request.reply(response);
}

NetworkApi::NetworkApi(const ApiConfig& API_config, Somhunter* p_core)
    : _API_config{ API_config },
      _p_core{ p_core },
      _base_addr{ (API_config.local_only ? "http://127.0.0.1:" :
#ifdef WIN32  //< Windows won't accept zeroes
	                                     "http://*:"
#else  // UNIX
	                                     "http://0.0.0.0:"
#endif
	               ) +
	              std::to_string(API_config.port) }
{
}

void NetworkApi::initialize()
{
	uri_builder endpoint(utility::conversions::to_string_t(_base_addr));
	http_listener ep_listener{ endpoint.to_uri().to_string() };
	ep_listener.support(methods::OPTIONS, handle_options);

	// Add all desired endpoints
	push_endpoint("api", &NetworkApi::handle__api__GET);
	push_endpoint("api/config", &NetworkApi::handle__api__config__GET);
	push_endpoint("config", &NetworkApi::handle__config__GET);

	push_endpoint("user/context", &NetworkApi::handle__user__context__GET);

	push_endpoint("dataset/video-detail", &NetworkApi::handle__dataset__video_detail__GET);

	push_endpoint("search/get-top-display", {}, &NetworkApi::handle__search__get_top_display__POST);
	push_endpoint("search/get-som-display", {}, &NetworkApi::handle__search__get_som_display__POST);
	push_endpoint("search/get-som-relocation-display", {},
	              &NetworkApi::handle__search__get_som_relocation_display__POST);
	push_endpoint("search/keyword-autocomplete", &NetworkApi::handle__search__keyword_autocomplete__GET);
	push_endpoint("search/reset", {}, &NetworkApi::handle__search__reset__POST);
	push_endpoint("search/rescore", {}, &NetworkApi::handle__search__rescore__POST);
	push_endpoint("search/like-frame", {}, &NetworkApi::handle__search__like_frame__POST);
	push_endpoint("search/bookmark-frame", {}, &NetworkApi::handle__search__bookmark_frame__POST);
	push_endpoint("search/context", &NetworkApi::handle__search__context__GET,
	              &NetworkApi::handle__search__context__POST);

	push_endpoint("log/scroll", &NetworkApi::handle__log__scroll__GET);
	push_endpoint("log/text-query-change", &NetworkApi::handle__log__text_query_change__GET);

	push_endpoint("eval-server/submit", {}, &NetworkApi::handle__eval_server__submit__POST);
	push_endpoint("eval-server/login", {}, &NetworkApi::handle__eval_server__login__POST);
	push_endpoint("eval-server/logout", {}, &NetworkApi::handle__eval_server__logout__POST);

	SHLOG_S("Listening for requests at '" << _base_addr << "' at the API endpoints.");
}

void NetworkApi::terminate()
{
	for (auto&& ep : _endpoints) {
		ep.close();
	}
}

void NetworkApi::run()
{
	initialize();

	SHLOG("Type in \"exit\" to exit.");

	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		std::string line;
		std::getline(std::cin, line);

		if (line == "exit") break;
	}

	terminate();
}

void NetworkApi::push_endpoint(const std::string& path, std::function<void(NetworkApi*, http_request)> GET_handler,
                               std::function<void(NetworkApi*, http_request)> POST_handler,
                               std::function<void(NetworkApi*, http_request)> PUT_handler,
                               std::function<void(NetworkApi*, http_request)> DEL_handler)
{
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

		// For CORS
		ep_listener.support(methods::OPTIONS, std::bind(handle_options, std::placeholders::_1));

		auto res = ep_listener.open().wait();

		// Check if failed
		if (res != pplx::completed) {
			std::string msg{ "Unable to set the HTTP listener for '" + path + ".!" };

			SHLOG_E(msg);
			throw std::runtime_error{ msg };
		}

		_endpoints.emplace_back(std::move(ep_listener));
	} catch (const std::exception& e) {
		std::string msg{ "Unable to set the HTTP listener for '" + path + "'!" };

		msg.append("\n\nIf these are access problems, try running it as an administrator.\n\n");

		msg.append("\nMESSAGE: \n");
		msg.append(e.what());

		SHLOG_E(msg);
		throw std::runtime_error{ msg };
	}
}

/**
 * This handles request to `/api/` endpoint - it serves OpenAPI HTML docs.
 */
void NetworkApi::handle__api__GET(http_request message)
{
	auto lck{ exclusive_lock() };  //< (#)
	auto remote_addr{ to_utf8string(message.remote_address()) };
	SHLOG_REQ(remote_addr, __func__);

	auto paths = http::uri::split_path(http::uri::decode(message.relative_uri().path()));
	message.relative_uri().path();

	auto filepath{ (paths.empty() ? _API_config.docs_dir + "index.html"
		                          : _API_config.docs_dir + to_utf8string(paths.front())) };

	string_t mime{ U("text/html") };
	auto sub3{ filepath.substr(filepath.length() - 3) };
	if (sub3 == "css") {
		mime = U("text/css");
	} else if (sub3 == ".js") {
		mime = U("application/javascript");
	}

	// More examples: https://github.com/Meenapintu/Restweb/blob/master/src/handler.cpp
	concurrency::streams::fstream::open_istream(to_string_t(filepath), std::ios::in)
	    .then([=](concurrency::streams::istream is) {
		    message.reply(status_codes::OK, is, mime).then([](pplx::task<void> t) {
			    try {
				    t.get();
			    } catch (...) {
				    SHLOG_W("Error serving `/api/` files...");
			    }
		    });
	    })
	    .then([=](pplx::task<void> t) {
		    try {
			    t.get();
		    } catch (...) {
			    message.reply(status_codes::InternalError, U("INTERNAL ERROR "));
		    }
	    });

	SHLOG_UNREQ(remote_addr, __func__);
	return;
}
void NetworkApi::handle__api__config__GET(http_request req)
{
	auto lck{ exclusive_lock() };  //< (#)
	auto remote_addr{ to_utf8string(req.remote_address()) };
	SHLOG_REQ(remote_addr, __func__);

	// auto b = message.extract_json().get();

	std::error_code ec;
	auto j{ json::value::parse(utils::read_whole_file(_p_core->get_API_config_filepath()), ec) };
	if (ec) {
		std::string msg{ ec.message() };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	http_response response(status_codes::OK);
	response.set_body(j);
	NetworkApi::add_CORS_headers(response);
	req.reply(response);
}

void NetworkApi::handle__config__GET(http_request req)
{
	auto lck{ exclusive_lock() };  //< (#)
	auto remote_addr{ to_utf8string(req.remote_address()) };
	SHLOG_REQ(remote_addr, __func__);

	// auto b = message.extract_json().get();

	std::error_code ec;
	auto j_API{ json::value::parse(utils::read_whole_file(_p_core->get_API_config_filepath()), ec) };

	auto j{ json::value::parse(utils::read_whole_file(_p_core->get_config_filepath()), ec) };
	if (ec) {
		std::string msg{ ec.message() };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	// Append API config to the core one
	j[U("API")] = j_API;

	http_response response(status_codes::OK);
	response.set_body(j);
	NetworkApi::add_CORS_headers(response);
	req.reply(response);
}

void NetworkApi::handle__user__context__GET(http_request req)
{
	auto lck{ exclusive_lock() };  //< (#)
	auto remote_addr{ to_utf8string(req.remote_address()) };
	SHLOG_REQ(remote_addr, __func__);

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

void NetworkApi::handle__search__get_top_display__POST(http_request req)
{
	auto lck{ exclusive_lock() };  //< (#)
	auto remote_addr{ to_utf8string(req.remote_address()) };
	SHLOG_REQ(remote_addr, __func__);

	auto body = req.extract_json().get();

	FrameId frame_ID{ ERR_VAL<FrameId>() };
	if (body.has_field(U("frameId")) && !body[U("frameId")].is_null()) {
		frame_ID = static_cast<FrameId>(body[U("frameId")].as_integer());
	}
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

void NetworkApi::handle__search__get_som_display__POST(http_request req)
{
	auto lck{ exclusive_lock() };  //< (#)
	auto remote_addr{ to_utf8string(req.remote_address()) };
	SHLOG_REQ(remote_addr, __func__);

	auto body = req.extract_json().get();

	auto dtype{ str_to_disp_type("SOM_display") };

	if (!_p_core->som_ready()) {
		http_response res{ construct_error_res(status_codes::BadRequest, "SOM not ready!") };
		res.set_status_code(222);
		NetworkApi::add_CORS_headers(res);
		req.reply(res);
		return;
	}

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

void NetworkApi::handle__search__get_som_relocation_display__POST(http_request req)
{
	auto lck{ exclusive_lock() };  //< (#)
	auto remote_addr{ to_utf8string(req.remote_address()) };
	SHLOG_REQ(remote_addr, __func__);

	auto body = req.extract_json().get();
	size_t temporal_id{ static_cast<size_t>(body[U("temporalId")].as_integer()) };

	if (!_p_core->som_ready(temporal_id)) {
		http_response res{ construct_error_res(status_codes::BadRequest, "SOM relocation not ready!") };
		res.set_status_code(222);
		NetworkApi::add_CORS_headers(res);
		req.reply(res);
		return;
	}

	// Fetch the data
	auto display_frames{ _p_core->get_display(DisplayType::DRelocation, ERR_VAL<FrameId>(), temporal_id) };
	json::value res_data{ to_Response__GetTopScreen__Post(_p_core, display_frames, 0, "SOM_relocation_display", "") };

	// Construct the response
	http_response res(status_codes::OK);
	res.set_body(res_data);

	// Send the response
	NetworkApi::add_CORS_headers(res);
	req.reply(res);
}

void NetworkApi::handle__dataset__video_detail__GET(http_request req)
{
	auto lck{ exclusive_lock() };  //< (#)
	auto remote_addr{ to_utf8string(req.remote_address()) };
	SHLOG_REQ(remote_addr, __func__);

	// auto paths = http::uri::split_path(http::uri::decode(req.relative_uri().path()));

	auto query{ req.relative_uri().query() };
	auto query_map{ web::uri::split_query(query) };

	size_t frame_ID{ ERR_VAL<size_t>() };
	try {
		frame_ID = static_cast<size_t>(utils::str_to_int(to_utf8string(query_map[U("frameId")])));
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

void NetworkApi::handle__search__keyword_autocomplete__GET(http_request req)
{
	auto lck{ exclusive_lock() };  //< (#)
	auto remote_addr{ to_utf8string(req.remote_address()) };
	SHLOG_REQ(remote_addr, __func__);

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
	size_t num_suggestions{ 5 };
	size_t example_frames_count{ 5 };
	auto record_count{ query_map.find(U("count")) };

	// Fetch the data
	auto _keyword_ranker{ _p_core->autocomplete_keywords(prefix, num_suggestions) };
	json::value res_data{ to_Response__GetAutocompleteResults__Get(_p_core, _keyword_ranker, example_frames_count,
		                                                           "") };

	// Construct the response
	http_response res(status_codes::OK);
	res.set_body(res_data);

	// Send the response
	NetworkApi::add_CORS_headers(res);
	req.reply(res);
}

void NetworkApi::handle__log__scroll__GET(http_request req)
{
	auto lck{ exclusive_lock() };  //< (#)
	auto remote_addr{ to_utf8string(req.remote_address()) };
	SHLOG_REQ(remote_addr, __func__);

	auto query{ req.relative_uri().query() };
	auto query_map{ web::uri::split_query(query) };

	auto scroll_area_record{ query_map.find(U("scrollArea")) };
	auto delta_record{ query_map.find(U("delta")) };
	auto frame_ID_record{ query_map.find(U("frameId")) };

	if (scroll_area_record == query_map.end() || delta_record == query_map.end()) {
		http_response res{ construct_error_res(status_codes::BadRequest, "Invalid parameters.") };
		NetworkApi::add_CORS_headers(res);
		req.reply(res);
		return;
	}

	try {
		FrameId frame_ID{ ERR_VAL<FrameId>() };
		if (frame_ID_record != query_map.end()) {
			frame_ID = static_cast<FrameId>(utils::str_to_int(to_utf8string(frame_ID_record->second)));
		}

		auto scroll_area{ to_utf8string(scroll_area_record->second) };
		auto disp{ str_to_disp_type(scroll_area) };
		float delta{ (utils::str2<float>(to_utf8string(delta_record->second)) > 0 ? 1.0F : -1.0F) };

		// If normal scroll
		if (frame_ID == ERR_VAL<FrameId>()) {
			_p_core->log_scroll(disp, delta);
		}
		// Else replay scroll over the frrame_ID frame
		else {
			_p_core->log_video_replay(frame_ID, delta);
		}

		// Construct the response
		http_response res(status_codes::OK);
		NetworkApi::add_CORS_headers(res);
		res.set_body(json::value::object());
		req.reply(res);
	} catch (...) {
		http_response res{ construct_error_res(status_codes::BadRequest, "Invalid parameters.") };
		NetworkApi::add_CORS_headers(res);
		res.set_body(json::value::object());
		req.reply(res);
		return;
	}
}

void NetworkApi::handle__log__text_query_change__GET(http_request req)
{
	auto lck{ exclusive_lock() };  //< (#)
	auto remote_addr{ to_utf8string(req.remote_address()) };
	SHLOG_REQ(remote_addr, __func__);

	auto query{ req.relative_uri().query() };
	auto query_map{ web::uri::split_query(query) };

	auto record{ query_map.find(U("query")) };
	if (record == query_map.end()) {
		http_response res{ construct_error_res(status_codes::BadRequest, "Invalid parameters.") };
		NetworkApi::add_CORS_headers(res);
		req.reply(res);
		return;
	}
	std::string prefix{ to_utf8string(web::http::uri::decode(record->second)) };

	_p_core->log_text_query_change(prefix);

	// Construct the response
	http_response res(status_codes::OK);
	NetworkApi::add_CORS_headers(res);
	res.set_body(json::value::object());
	req.reply(res);
}

void NetworkApi::handle__eval_server__submit__POST(http_request req)
{
	auto lck{ exclusive_lock() };  //< (#)
	auto remote_addr{ to_utf8string(req.remote_address()) };
	SHLOG_REQ(remote_addr, __func__);

	FrameId frame_ID{ ERR_VAL<FrameId>() };
	auto body = req.extract_json().get();
	try {
		frame_ID = static_cast<FrameId>(body[U("frameId")].as_integer());
	} catch (...) {
		http_response res{ construct_error_res(status_codes::BadRequest, "Invalid `frameId` parameter.") };
		NetworkApi::add_CORS_headers(res);
		req.reply(res);
		return;
	}

	// Submit it!
	SubmitResult correct{ _p_core->submit_to_eval_server(frame_ID) };

	// 401 for Unauthorized
	if (correct != SubmitResult::CORRECT && correct != SubmitResult::INCORRECT) {
		http_response res{ construct_error_res(status_codes::Unauthorized,
			                                   "You are not logged in to the evaluation server.") };
		NetworkApi::add_CORS_headers(res);
		req.reply(res);
		return;
	}

	auto res_obj{ json::value::object() };
	res_obj[U("result")] = json::value::boolean(correct == SubmitResult::CORRECT ? true : false);

	// Construct the response
	http_response res(status_codes::OK);
	res.set_body(res_obj);
	NetworkApi::add_CORS_headers(res);
	req.reply(res);
}

void NetworkApi::handle__eval_server__login__POST(http_request req)
{
	auto lck{ exclusive_lock() };  //< (#)
	auto remote_addr{ to_utf8string(req.remote_address()) };
	SHLOG_REQ(remote_addr, __func__);

	bool result{ _p_core->login_to_eval_server() };

	auto res_obj{ json::value::object() };
	res_obj[U("result")] = json::value::boolean(result);

	// Construct the response
	http_response res(status_codes::OK);
	res.set_body(res_obj);
	NetworkApi::add_CORS_headers(res);
	req.reply(res);
}

void NetworkApi::handle__eval_server__logout__POST(http_request req)
{
	auto lck{ exclusive_lock() };  //< (#)
	auto remote_addr{ to_utf8string(req.remote_address()) };
	SHLOG_REQ(remote_addr, __func__);

	bool result{ _p_core->logout_from_eval_server() };

	auto res_obj{ json::value::object() };
	res_obj[U("result")] = json::value::boolean(result);

	// Construct the response
	http_response res(status_codes::OK);
	res.set_body(res_obj);
	NetworkApi::add_CORS_headers(res);
	req.reply(res);
}

void NetworkApi::handle__search__reset__POST(http_request req)
{
	auto lck{ exclusive_lock() };  //< (#)
	auto remote_addr{ to_utf8string(req.remote_address()) };
	SHLOG_REQ(remote_addr, __func__);

	// Reset
	_p_core->reset_search_session();

	// Construct the response
	http_response res(status_codes::OK);
	res.set_body(json::value::object());
	NetworkApi::add_CORS_headers(res);
	req.reply(res);
}

RescoreMetadata NetworkApi::extract_rescore_metadata(web::json::value& body)
{
	/*
	 * Extract from the body
	 */
	size_t src_ctx_ID{ static_cast<size_t>(body[U("srcSearchCtxId")].as_integer()) };
	std::string screenshot_data{ to_utf8string(body[U("screenshotData")].as_string()) };
	std::string _username{ "matfyz" };

	/*
	 * Process it
	 */

	// \todo Validate the user
	// p_core->auth_user()

	std::string time_label{ utils::get_formated_timestamp("%H:%M:%S") };

	RescoreMetadata md;
	md._username = _username;
	md.screenshot_filepath = _p_core->store_rescore_screenshot(screenshot_data);
	md.srd_search_ctx_ID = src_ctx_ID;
	md.time_label = time_label;
	return md;
}

std::vector<TextualQuery> NetworkApi::extract_textual_query(web::json::value& body)
{
	/*
	 * Extract from the body
	 */
	std::string q0{ to_utf8string(body[U("q0")].as_string()) };
	std::string q1{ to_utf8string(body[U("q1")].as_string()) };

	/*
	 * Process it
	 */
	return { q0, q1 };
}

std::vector<RelocationQuery> NetworkApi::extract_relocation_query(web::json::value& body)
{
	/*
	 * Extract from the body
	 */
	RelocationQuery relocation0{ static_cast<RelocationQuery>(body[U("relocation0")].as_integer()) };
	RelocationQuery relocation1{ static_cast<RelocationQuery>(body[U("relocation1")].as_integer()) };

	return { relocation0, relocation1 };
}

std::vector<CanvasQuery> NetworkApi::extract_canvas_query(web::json::value& body)
{
	/*
	 * Extract from the body
	 */
	json::value outer_array{ body[U("canvas_query")] };

	if (outer_array.is_null()) {
		return { DEFAULT_COLLAGE };
	}

	/*
	 * Process it
	 */
	std::vector<CanvasQuery> canvas_query;

	/* outer_array:
	[
	    [inner_arr #0], < The first scene described
	    [inner_arr #1], < The following scene
	    [inner_arr #2], < The third scene
	    ...
	] */

	// For each temporal query provided by the user
	for (size_t temp_i{ 0 }; temp_i < outer_array.size(); ++temp_i) {
		canvas_query.push_back(CanvasQuery());

		auto inner_arr{ outer_array[temp_i] };
		if (inner_arr.is_null()) {
			return { DEFAULT_COLLAGE };
		}

		/* inner_arr:
		[
		    { "bitmap" OR "text" subquery #0 },
		    { "bitmap" OR "text" subquery #1 },
		    { "bitmap" OR "text" subquery #2 },
		    { "bitmap" OR "text" subquery #3 },
		    ...
		] */
		// For each query placed on this temporal canvas
		for (size_t canvas_query_i{ 0 }; canvas_query_i < inner_arr.size(); ++canvas_query_i) {
			auto subquery_JSON{ inner_arr[canvas_query_i].as_object() };

			// Determine the type
			std::string subquery_type{ to_utf8string(subquery_JSON[U("type")].as_string()) };

			// Parse the rect
			std::vector<float> rect_vals = from_double_array<float>(subquery_JSON[U("rect")]);
			if (rect_vals.size() != 4) {
				throw std::runtime_error("Invalid `rect` property.");
			}
			RelativeRect rect{ rect_vals[0], rect_vals[1], rect_vals[2], rect_vals[3] };

			// If textual query on the canvas
			if (subquery_type == "text") {
				std::string text_query{ to_utf8string(subquery_JSON[U("text_query")].as_string()) };
				canvas_query[temp_i].emplace_back(rect, text_query);
			}
			// Else bitmap
			else {
				std::vector<uint8_t> bitmap_data{ from_JSON_array<uint8_t>(subquery_JSON[U("bitmap_data")]) };
				size_t width{ static_cast<size_t>(subquery_JSON[U("width_pixels")].as_integer()) };
				size_t height{ static_cast<size_t>(subquery_JSON[U("height_pixels")].as_integer()) };
				size_t num_channels{ static_cast<size_t>(subquery_JSON[U("num_channels")].as_integer()) };
				canvas_query[temp_i].emplace_back(rect, width, height, num_channels, bitmap_data.data());
			}
		}
	}

	return canvas_query;
}

Filters NetworkApi::extract_filters(web::json::value& body)
{
	/*
	 * Extract from the body
	 */
	json::value weekdays_JSON{ body[U("filters")][U("weekdays")] };
	Hour hourFrom{ static_cast<Hour>(body[U("filters")][U("hoursFrom")].as_integer()) };
	Hour hourTo{ static_cast<Hour>(body[U("filters")][U("hoursTo")].as_integer()) };
	json::value dataset_parts_JSON{ body[U("filters")][U("datasetFilter")] };

	/*
	 * Process it
	 */
	uint8_t weekdays_mask{ 0 };
	for (size_t i{ 0 }; i < 7; ++i) {
		bool flag{ weekdays_JSON.as_array().at(i).as_bool() };
		if (flag) {
			// LSb is day 0, MSb is day 6
			weekdays_mask = weekdays_mask | (1 << i);
		}
	}

	auto xx{ dataset_parts_JSON.as_array() };
	std::vector<bool> dataset_parts_mask;
	for (size_t i{ 0 }; i < xx.size(); ++i) {
		bool flag{ xx.at(i).as_bool() };

		dataset_parts_mask.emplace_back(flag);
	}

	return Filters{ TimeFilter{ hourFrom, hourTo }, WeekDaysFilter{ weekdays_mask }, dataset_parts_mask };
}

void NetworkApi::handle__search__rescore__POST(http_request req)
{
	auto lck{ exclusive_lock() };  //< (#)
	auto remote_addr{ to_utf8string(req.remote_address()) };
	SHLOG_REQ(remote_addr, __func__);

	auto body = req.extract_json().get();

	Query query;
	try {
		// Metadata
		RescoreMetadata rescore_metadata{ extract_rescore_metadata(body) };

		// Relevance feedbask query
		RelevanceFeedbackQuery relevance_query{ _p_core->get_search_context().likes };

		// Text queries
		std::vector<TextualQuery> textual_query{ extract_textual_query(body) };

		// Relocation query
		std::vector<RelocationQuery> relocation_query{ extract_relocation_query(body) };

		// Filters
		Filters filters{ extract_filters(body) };

		// Canvas queries
		std::vector<CanvasQuery> canvas_query{ extract_canvas_query(body) };

		bool is_save{ body[U("is_save")].as_bool() };
		query.is_save = is_save;

		// Is the query non-empty?
		if (textual_query.empty() && relevance_query.empty() && canvas_query.empty()) {
			http_response res{ construct_error_res(status_codes::BadRequest, "Empty queries.") };
			NetworkApi::add_CORS_headers(res);
			req.reply(res);
			return;
		}

		query.metadata = std::move(rescore_metadata);
		query.filters = std::move(filters);
		query.relevance_feeedback = std::move(relevance_query);
		size_t temp_length = std::max(textual_query.size(), canvas_query.size());
		for (size_t i = 0; i < temp_length; ++i) {
			TemporalQuery tq;
			if (i < textual_query.size()) tq.textual = textual_query[i];
			if (i < canvas_query.size()) tq.canvas = canvas_query[i];
			if (i < relocation_query.size()) tq.relocation = relocation_query[i];

			query.temporal_queries.push_back(tq);
		}
	} catch (...) {
		http_response res{ construct_error_res(status_codes::BadRequest, "Invalid parameters.") };
		NetworkApi::add_CORS_headers(res);
		req.reply(res);
		return;
	}
	// Rescore
	auto rescore_result{ _p_core->rescore(query) };
	json::value _history{ to_Response__Rescore__Post(_p_core, rescore_result) };

	// Construct the response
	http_response res(status_codes::OK);
	res.set_body(_history);
	NetworkApi::add_CORS_headers(res);
	req.reply(res);
}

void NetworkApi::handle__search__like_frame__POST(http_request req)
{
	auto lck{ exclusive_lock() };  //< (#)
	auto remote_addr{ to_utf8string(req.remote_address()) };
	SHLOG_REQ(remote_addr, __func__);

	FrameId frame_ID{ ERR_VAL<FrameId>() };
	auto body = req.extract_json().get();
	try {
		frame_ID = static_cast<FrameId>(body[U("frameId")].as_integer());
	} catch (...) {
		http_response res{ construct_error_res(status_codes::BadRequest, "Invalid `frameId` parameter.") };
		NetworkApi::add_CORS_headers(res);
		req.reply(res);
		return;
	}

	// Like it.
	bool like_flag{ _p_core->like_frames({ frame_ID }).front() };

	json::value result_obj{ json::value::object() };
	result_obj[U("frameId")] = json::value::number(uint32_t(frame_ID));
	result_obj[U("isLiked")] = json::value::boolean(like_flag);

	// Construct the response
	http_response res(status_codes::OK);
	res.set_body(result_obj);
	NetworkApi::add_CORS_headers(res);
	req.reply(res);
}

void NetworkApi::handle__search__bookmark_frame__POST(http_request req)
{
	auto lck{ exclusive_lock() };  //< (#)
	auto remote_addr{ to_utf8string(req.remote_address()) };
	SHLOG_REQ(remote_addr, __func__);

	FrameId frame_ID{ ERR_VAL<FrameId>() };
	auto body = req.extract_json().get();
	try {
		frame_ID = static_cast<FrameId>(body[U("frameId")].as_integer());
	} catch (...) {
		http_response res{ construct_error_res(status_codes::BadRequest, "Invalid `frameId` parameter.") };
		NetworkApi::add_CORS_headers(res);
		req.reply(res);
		return;
	}

	// Like it.
	bool like_flag{ _p_core->bookmark_frames({ frame_ID }).front() };

	json::value result_obj{ json::value::object() };
	result_obj[U("frameId")] = json::value::number(uint32_t(frame_ID));
	result_obj[U("isBookmarked")] = json::value::boolean(like_flag);

	// Construct the response
	http_response res(status_codes::OK);
	res.set_body(result_obj);
	NetworkApi::add_CORS_headers(res);
	req.reply(res);
}

void NetworkApi::handle__search__context__POST(http_request req)
{
	auto lck{ exclusive_lock() };  //< (#)
	auto remote_addr{ to_utf8string(req.remote_address()) };
	SHLOG_REQ(remote_addr, __func__);

	try {
		auto body = req.extract_json().get();

		size_t src_search_context_idx{ static_cast<size_t>(body[U("srcSearchCtxId")].as_integer()) };
		size_t dest_search_context_idx{ static_cast<size_t>(body[U("id")].as_integer()) };
		std::string screenshot_base64{ to_utf8string(body[U("screenshotData")].as_string()) };

		// \todo
		std::string time_label{ "1:20 PM" };
		std::string screenshot_fpth{ "assets/img/history_screenshot.jpg" };

		if (store_JPEG_from_base64(screenshot_fpth, screenshot_base64) == 0) {
			SHLOG_W("Failed to write screenshot to '" << screenshot_fpth << "'.");
		}

		// Fetch the data
		const UserContext& user_ctx{ _p_core->switch_search_context(dest_search_context_idx, src_search_context_idx,
			                                                        screenshot_fpth, time_label) };
		json::value res_data{ to_Response__User__Context__Get(_p_core, user_ctx) };

		// Construct the response
		http_response response(status_codes::OK);
		response.set_body(res_data);

		// Send the response
		NetworkApi::add_CORS_headers(response);
		req.reply(response);

	} catch (...) {
		http_response res{ construct_error_res(status_codes::BadRequest, "Invalid `frameId` parameter.") };
		NetworkApi::add_CORS_headers(res);
		req.reply(res);
		return;
	}
}

void NetworkApi::handle__search__context__GET(http_request req)
{
	auto lck{ exclusive_lock() };  //< (#)
	// Construct the response
	http_response res(status_codes::OK);
	NetworkApi::add_CORS_headers(res);
	req.reply(res);
}
