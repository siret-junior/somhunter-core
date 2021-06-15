
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

#ifndef CLIENT_DRES_H_
#define CLIENT_DRES_H_

// ---
#include <nlohmann/json.hpp>
// ---
#include "settings.h"
#include "http.h"

namespace sh {

struct VideoFrame;

/** Unified interface for all remote evaluation servers. */
class IServerClient {
	// *** METHODS ***
public:
	IServerClient(const EvalServerSettings& settings)
	    : _eval_server_settings{ settings }, _do_requests{ false }, _http{}, _username{ "not-doing-requests" } {};
	// ---
	virtual bool login() = 0;
	virtual bool logout() = 0;
	virtual bool submit(const VideoFrame& frame) = 0;

	virtual UnixTimestamp get_server_ts() = 0;
	virtual nlohmann::json get_current_task() = 0;

	virtual bool is_logged_in() const { return !_username.empty(); }
	virtual const std::string& get_user_token() const {
		if (_username.empty() && _do_requests) {
			std::string msg{ "User token is not valid" };
			SHLOG_E(msg);
			throw std::runtime_error{ msg };
		}
		return _username;
	};
	virtual void set_do_requests(bool val) { _do_requests = val; };
	virtual bool get_do_requests() const { return _do_requests; };

protected:
	virtual void set_user_token(const std::string& val) {
		SHLOG_D("Setting `_user_token` to " << val);
		_username = val;
	};

	// *** MEMBER VARIABLES ***
protected:
	EvalServerSettings _eval_server_settings;
	bool _do_requests;
	Http _http;
	std::string _username;
};

/**
 * Specific DRES server implementation.
 *
 * https://github.com/dres-dev/DRES
 */
class ClientDres final : public IServerClient {
	// *** METHODS ***
public:
	ClientDres(const EvalServerSettings& settings);
	~ClientDres() noexcept;
	// ---
	virtual bool login() override;
	virtual bool logout() override;
	virtual bool submit(const VideoFrame& frame) override;

	virtual UnixTimestamp get_server_ts() override;
	virtual nlohmann::json get_current_task() override;

private:
	enum class LogType { LOGIN, LOGOUT, SUBMIT, RESULT, INTERACTION };
	std::string log_type_to_str(LogType t) const {
		switch (t) {
			case LogType::LOGIN:
				return "login";
				break;

			case LogType::LOGOUT:
				return "logout";
				break;

			case LogType::SUBMIT:
				return "submit";
				break;

			case LogType::RESULT:
				return "result";
				break;

			case LogType::INTERACTION:
				return "interaction";
				break;

			default:
				std::string msg{"Unknown log type!"};
				throw std::runtime_error{msg};
				break;
		}
	}

	void write_log(LogType type, UnixTimestamp ts, const nlohmann::json& req, ReqCode code, nlohmann::json& res) const;

	// *** MEMBER VARIABLES ***
protected:
	const std::chrono::milliseconds _sync_period{5000ms};
	
	ServerConfigDres _settings;
	std::thread _t_pinger;

	bool _synced;
	std::ptrdiff_t _diff;
	nlohmann::json _current_task;
};

};  // namespace sh

#endif  // CLIENT_DRES_H_
