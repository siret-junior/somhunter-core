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

#include "client-dres.h"

#include "common.h"
#include "dataset-frames.h"
#include "os-utils.hpp"

using namespace sh;

ClientDres::ClientDres(const EvalServerSettings& eval_server_settings)
    : IServerClient{ eval_server_settings },
      _settings{ std::get<EvalServerSettings::ServerConfigDres>(eval_server_settings.server_cfg) },
      _synced{ false } {
	// Initial setup
	set_do_requests(eval_server_settings.do_network_requests);

	// If we should accept insecure connections
	_http.set_allow_insecure(eval_server_settings.allow_insecure);

	_t_sync_worker = std::thread{ [this]() {
		if (!_do_requests) {
			return;
		}

		while (true) {
			auto ts_our_pre{ utils::timestamp() };
			auto res{ _http.do_GET_sync_json(_settings.server_time_URL, {}) };
			auto ts_our_post{ utils::timestamp() };

			if (res.first != 200) {
				SHLOG_W("Evaluation server at '" << _settings.server_time_URL << "' is unavailable! Unable to sync.");
				_synced = false;
			} else {
				auto ts{ res.second["timeStamp"].get<UnixTimestamp>() };

				std::ptrdiff_t half_trip{ (ts_our_post - ts_our_pre) / 2 };
				_diff = ((ts - half_trip) - ts_our_pre);

				SHLOG_D("Synced with diff " << _diff << ", half_trip = " << half_trip);
				_synced = true;
			}
			std::this_thread::sleep_for(_sync_period);
		}
	} };

	_t_sync_worker.detach();
}

sh::ClientDres::~ClientDres() noexcept { logout(); }

bool ClientDres::login() {
	auto ts{ utils::timestamp() };

	nlohmann::json headers;

	// clang-format off
	nlohmann::json body{ 
		{ "username", _settings.username }, 
		{ "password", _settings.password } 
	};
	// clang-format on

	bool success{ false };

	nlohmann::json res;
	ReqCode code{ 0 };
	const auto& URL{ _settings.login_URL };

	if (_do_requests) {
		try {
			// Do the blocking request
			std::tie(code, res) = _http.do_POST_sync_json(URL, body, headers);

			// If failed
			if (code != 200) {
				std::string msg{ "Login request unsuccessfull, return code: " + std::to_string(code) };
				msg.append("\n\nres:\n");
				msg.append(res.dump(4));
				SHLOG_W(msg);
			}
			// 2XX
			else {
				auto user_token{ res["sessionId"].get<std::string>() };
				set_user_token(user_token);

				std::string msg{ "Login request successfull, user_token: " + user_token };
				SHLOG_S(msg);
				success = true;
			}

		} catch (const std::exception& ex) {
			SHLOG_W("Login request failed!" << std::endl << ex.what());
		}
	} else {
		std::string msg{ "Login request wasn't actually made. Turn it on if unintended." };
		SHLOG_W(msg);
		code = 200;
		success = true;
	}

	write_log(LogType::LOGIN, ts, URL, body, code, res);
	return success;
}

bool ClientDres::logout() {
	auto ts{ utils::timestamp() };

	nlohmann::json headers;

	// clang-format off
	nlohmann::json params{ 
		{ "session", _username }
	};
	// clang-format on

	bool success{ false };
	nlohmann::json res;
	ReqCode code{ 0 };
	const auto& URL{ _settings.logout_URL };

	if (_do_requests) {
		try {
			// Do the blocking request
			std::tie(code, res) = _http.do_GET_sync_json(_settings.logout_URL, params, headers);

			// If failed
			if (code != 200) {
				std::string msg{ "Logout request unsuccessfull, return code: " + std::to_string(code) };
				msg.append("\n\nres:\n");
				msg.append(res.dump(4));
				SHLOG_W(msg);
			}
			// 2XX
			else {
				std::string msg{ "Logout request successfull, user_token: " + _username };
				SHLOG_S(msg);

				set_user_token("");
				success = true;
			}

		} catch (const std::exception& ex) {
			SHLOG_W("Logout request failed!" << std::endl << ex.what());
		}
	} else {
		std::string msg{ "Logout request wasn't actually made. Turn it on if unintended." };
		SHLOG_W(msg);
		code = 200;
		success = true;
	}

	_username = "";

	write_log(LogType::LOGOUT, ts, URL, params, code, res);
	return success;
}

bool sh::ClientDres::submit(const VideoFrame& frame) {
	// Login check
	if (!is_logged_in()) {
		throw NotLoggedInEx("You must be logged in to the evaluation server to submit.");
	}

	auto ts{ utils::timestamp() };

	// If LSC format submit should be used
	std::stringstream item_ss;
	if (_eval_server_settings.submit_LSC_IDs) {
		item_ss << frame.LSC_id;
	}
	// Else the "usual" video ID should be used
	else {
		// It must be STRING representation of 5-digit video ID
		item_ss << std::setfill('0') << std::setw(5) << (frame.video_ID + 1);  //< !! VBS videos start from 1
	}

	// clang-format off
	nlohmann::json headers;
	nlohmann::json params{ { "session", _username }, { "item", item_ss.str() }, { "frame", frame.frame_number } };
	// clang-format on

	bool success{ false };
	nlohmann::json res;
	ReqCode code{ 0 };
	const auto& URL{ _settings.submit_URL };

	if (_do_requests) {
		try {
			// Do the blocking request
			std::tie(code, res) = _http.do_GET_sync_json(URL, params, headers);

			// If failed
			if (code != 200 && code != 202) {
				std::string msg{ "Submit request unsuccessfull, return code: " + std::to_string(code) };
				msg.append("\n\nres:\n");
				msg.append(res.dump(4));
				SHLOG_W(msg);
			}
			// 2XX
			else {
				auto sub_string{ res["submission"].get<std::string>() };

				if (sub_string == "CORRECT") {
					success = true;
				}

				std::string msg{ "Submit request successfull, result: " + sub_string };
				SHLOG_S(msg);
			}

		} catch (const std::exception& ex) {
			SHLOG_W("Submit request failed!" << std::endl << ex.what());
			return false;
		}
	} else {
		std::string msg{ "Submit request wasn't actually made. Turn it on if unintended." };
		SHLOG_W(msg);
		code = 200;
	}

	write_log(LogType::SUBMIT, ts, URL, params, code, res);
	return success;
}

bool ClientDres::send_results_log(const nlohmann::json& log_JSON) {
	// \todo Actually send the log to the server.

	bool result{ true };
	auto ts{ utils::timestamp() };
	ReqCode code{ 0 };
	const auto& URL{ "nowhere" };
	nlohmann::json res;

	write_log(LogType::RESULT, ts, URL, log_JSON, code, res);

	return result;
}

bool ClientDres::send_interactions_log(const nlohmann::json& log_JSON) {
	// \todo Actually send the log to the server.

	bool result{ true };
	auto ts{ utils::timestamp() };
	ReqCode code{ 0 };
	const auto& URL{ "nowhere" };

	nlohmann::json res;

	write_log(LogType::INTERACTION, ts, URL, log_JSON, code, res);

	return result;
}

UnixTimestamp ClientDres::get_server_ts() {
	if (!_synced) {
		return 0;
	}

	// Shift the timestamp to the server
	return utils::timestamp() + _diff;
}

nlohmann::json ClientDres::get_current_task() {
	// \todo Implement...
	// clang-format off
	nlohmann::json task_JSON{
		{ "id", "taskid" },
		{ "name", "taskname"},
		{ "taskGroup", "taskgroup" },
		{ "remainingTime", 123 }
	};
	// clang-format on

	return task_JSON;
}

void ClientDres::write_log(LogType type, UnixTimestamp ts, const std::string& URL, const nlohmann::json& req,
                           ReqCode code, nlohmann::json& res) const {
	std::string log_filepath = _eval_server_settings.log_dir_eval_server_requests + std::string("/") +
	                           std::to_string(ts) + std::string("__") + log_type_to_str(type) +
	                           _eval_server_settings.log_file_suffix;

	bool file_exists{ osutils::file_exists(log_filepath) };

	std::ofstream ofs(log_filepath, std::ios::app);
	if (file_exists) {
		ofs << ",";
	}

	if (!ofs) {
		SHLOG_E("Could not write a log file '" << log_filepath << "'!");
	}

	nlohmann::json log{ { "timestamp", ts },
		                { "datetime", utils::get_formated_timestamp("%d-%m-%Y_%H-%M-%S", ts) },
		                { "type", log_type_to_str(type) },
		                { "URL", URL },
		                { "request", req },
		                { "response_code", code },
		                { "response", res } };
	ofs << log.dump(4) << std::endl;
}
