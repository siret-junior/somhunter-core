
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

#include "settings.h"

#include "http.h"

namespace sh {

struct VideoFrame;

/** Unified interface for all remote evaluation servers. */
class IServerClient {
	// *** METHODS ***
public:
	IServerClient(const SubmitterConfig& settings)
	    : _eval_server_settings{ settings }, _do_requests{ false }, _http{}, _user_token{ "" } {};
	// ---
	virtual bool login() = 0;
	virtual bool logout() = 0;
	virtual bool submit(const VideoFrame& frame) = 0;

	virtual bool is_logged_in() const { return !_user_token.empty(); }
	virtual const std::string& get_user_token() const {
		if (_user_token.empty()) {
			std::string msg{ "User token is not valid" };
			throw std::runtime_error{ msg };
		}
		return _user_token;
	};
	virtual void set_do_requests(bool val) { _do_requests = val; };
	virtual bool get_do_requests() const { return _do_requests; };

protected:
	virtual void set_user_token(const std::string& val) {
		SHLOG_D("Setting `_user_token` to " << val);
		_user_token = val;
	};

	// *** MEMBER VARIABLES ***
protected:
	SubmitterConfig _eval_server_settings;
	bool _do_requests;
	Http _http;
	std::string _user_token;
};

/**
 * Specific DRES server implementation.
 *
 * https://github.com/dres-dev/DRES
 */
class ClientDres final : public IServerClient {
	// *** METHODS ***
public:
	ClientDres(const SubmitterConfig& settings);
	~ClientDres() noexcept;
	// ---
	virtual bool login() override;
	virtual bool logout() override;
	virtual bool submit(const VideoFrame& frame) override;

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

	void write_log(LogType type, Timestamp ts, const nlohmann::json& req, ReqCode code, nlohmann::json& res) const;

	// *** MEMBER VARIABLES ***
protected:
	ServerConfigDres _settings;
};

};  // namespace sh

#endif  // CLIENT_DRES_H_
