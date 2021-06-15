
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

#include "eval-server-client.h"
// ---
#include <memory>
// ---
#include "client-dres.h"
#include "common.h"
#include "dataset-frames.h"

using namespace sh;

EvalServerClient::EvalServerClient(const EvalServerSettings& settings)
    : _submitter_settings{ settings }, _p_client{ std::make_unique<ClientDres>(settings) }
{
	// Make sure that submitted logs directory exists
	utils::dir_create(_submitter_settings.log_dir_eval_server_requests);
}

bool EvalServerClient::login() { return _p_client->login(); }

bool EvalServerClient::logout() { return _p_client->logout(); }

bool EvalServerClient::submit(const VideoFrame& frame) { return _p_client->submit(frame); }

UnixTimestamp EvalServerClient::get_server_ts() { return _p_client->get_server_ts(); }

const std::string& EvalServerClient::get_user_token() const { return _p_client->get_user_token(); }

nlohmann::json EvalServerClient::get_current_task() { return nlohmann::json(); }
