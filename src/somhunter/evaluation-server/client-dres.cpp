
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

#include "client-DRES.h"

#include "common.h"
#include "dataset-frames.h"

using namespace sh;

ClientDres::ClientDres(const SubmitterConfig& eval_server_settings)
    : IServerClient{ eval_server_settings }, _settings{ std::get<ServerConfigDres>(eval_server_settings.server_cfg) } {
	// Initial setup
	set_do_requests(eval_server_settings.do_network_requests);

	// If we should accept insecure connections
	_http.set_allow_insecure(eval_server_settings.allow_insecure);
}

sh::ClientDres::~ClientDres() noexcept { logout(); }

bool ClientDres::login() {
	auto ts{ utils::timestamp() };

	nlohmann::json headers{};

	// clang-format off
	nlohmann::json body{ 
		{ "username", _settings.username }, 
		{ "password", _settings.password } 
	};
	// clang-format on

	bool success{ false };

	nlohmann::json res;
	ReqCode code{ 0 };

	if (_do_requests) {
		try {
			// Do the blocking request
			std::tie(code, res) = _http.do_POST_sync(_settings.login_URL, body, headers);

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

		} catch (std::exception ex) {
			SHLOG_W("Login request failed!" << std::endl << ex.what());
		}
	} else {
		std::string msg{ "Logout request wasn't actually made. Turn it on if unintended." };
		SHLOG_W(msg);
		code = 200;
		success = true;
	}

	write_log(LogType::LOGIN, ts, body, code, res);
	return success;
}

bool ClientDres::logout() {
	auto ts{ utils::timestamp() };

	nlohmann::json headers{};

	// clang-format off
	nlohmann::json params{ 
		{ "session", _user_token }
	};
	// clang-format on

	bool success{ false };
	nlohmann::json res;
	ReqCode code{ 0 };

	if (_do_requests) {
		try {
			// Do the blocking request
			std::tie(code, res) = _http.do_GET_sync(_settings.logout_URL, params, headers);

			// If failed
			if (code != 200) {
				std::string msg{ "Logout request unsuccessfull, return code: " + std::to_string(code) };
				msg.append("\n\nres:\n");
				msg.append(res.dump(4));
				SHLOG_W(msg);
			}
			// 2XX
			else {
				std::string msg{ "Logout request successfull, user_token: " + _user_token };
				SHLOG_S(msg);

				set_user_token("");
				success = true;
			}

		} catch (std::exception ex) {
			SHLOG_W("Logout request failed!" << std::endl << ex.what());
		}
	} else {
		std::string msg{ "Logout request wasn't actually made. Turn it on if unintended." };
		SHLOG_W(msg);
		code = 200;
		success = true;
	}

	write_log(LogType::LOGOUT, ts, params, code, res);
	return success;
}

bool sh::ClientDres::submit(const VideoFrame& frame) {
	// Login check
	if (!is_logged_in()) {
		SHLOG_E("Not logged in! User token empty!");
		return false;
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
	nlohmann::json headers{};
	nlohmann::json params{ { "session", _user_token }, { "item", item_ss.str() }, { "frame", frame.frame_number } };
	// clang-format on

	bool success{ false };
	nlohmann::json res;
	ReqCode code{ 0 };

	if (_do_requests) {
		try {
			// Do the blocking request
			std::tie(code, res) = _http.do_GET_sync(_settings.submit_URL, params, headers);

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

		} catch (std::exception ex) {
			SHLOG_W("Submit request failed!" << std::endl << ex.what());
			return false;
		}
	} else {
		std::string msg{ "Submit request wasn't actually made. Turn it on if unintended." };
		SHLOG_W(msg);
		code = 200;
	}

	write_log(LogType::SUBMIT, ts, params, code, res);
	return success;
}

void ClientDres::write_log(LogType type, Timestamp ts, const nlohmann::json& req, ReqCode code,
                           nlohmann::json& res) const {
	std::string log_filepath = _eval_server_settings.log_submitted_dir + std::string("/") + std::to_string(ts) +
	                           std::string("__") + log_type_to_str(type) + _eval_server_settings.log_file_suffix;

	std::ofstream ofs(log_filepath, std::ios::app);
	if (!ofs) {
		SHLOG_E("Could not write a log file '" << log_filepath << "'!");
	}

	nlohmann::json log{
		{ "timestamp", ts },       { "datetime", utils::get_formated_timestamp("%d-%m-%Y_%H-%M-%S", ts) },
		{ "type", "logout" },      { "request", req },
		{ "response_code", code }, { "response", res }
	};

	ofs << log.dump(4);
}
