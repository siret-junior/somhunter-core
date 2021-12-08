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

#include <string>
// ---
#include "common-types.h"
#include "common.h"
#include "utils.hpp"

namespace sh {

/**
 * Represents a real competition task and for the given timestamp responds with the target.
 */
class TaskTargetHelper {
public:
	class Task {
	public:
		enum class Type { TEXT, VISUAL, AVS };

		Type type_to_enum(const std::string& s) {
			if (s == "t" || s == "T")
				return Type::TEXT;
			else if (s == "v" || s == "V")
				return Type::VISUAL;
			else
				return Type::AVS;
		}

	public:
		Task() = delete;
		Task(const std::string& name, const std::string& type, std::size_t ts_from, std::size_t ts_to, VideoId video_ID,
		     FrameNum fr_from, FrameNum fr_to)
		    : _name(name),
		      _type(type_to_enum(type)),
		      _timestamps(ts_from, ts_to),
		      _video_ID(video_ID - 1),  // To 0-based
		      _frames_interval(fr_from, fr_to) {}

		std::pair<std::size_t, std::size_t> timestamps() { return _timestamps; }

		VideoId video_ID() { return _video_ID; }

		FrameNum frame_from() { return _frames_interval.first; }

		FrameNum frame_to() { return _frames_interval.second; }

	private:
		std::string _name;
		Type _type;
		std::pair<std::size_t, std::size_t> _timestamps;
		VideoId _video_ID;  // 0-based
		std::pair<FrameNum, FrameNum> _frames_interval;
	};

public:
	TaskTargetHelper() = delete;
	TaskTargetHelper(const std::string& filepath) {
		SHLOG_I("Parsing tasks from '" << filepath << "'...");

		auto ifs = std::ifstream(filepath, std::ios::in);
		if (!ifs) {
			std::string msg{ "Error opening file: " + filepath };
			SHLOG_E(msg);
			throw std::runtime_error(msg);
		}

		// task_name,task_type,timestamp_from,timestamp_to,video_ID_starts_from_1,frame_from,frame_to
		// 01_v21-1,V,1624277458765,1624277758765,4178,1875,2300

		// Read the file line by line until EOF
		std::size_t i_line = 0;
		for (std::string line_text_buffer; std::getline(ifs, line_text_buffer); ++i_line) {
			// Skip the first line
			if (i_line == 0) continue;

			std::stringstream line_buffer_ss(line_text_buffer);

			std::vector<std::string> tokens;

			// Tokenize this line with "," separator
			for (std::string token; std::getline(line_buffer_ss, token, ',');) {
				tokens.push_back(token);
			}

			do_assert(tokens.size() == 7, "Exactly 7 cols in the CSV.");

			std::string& name = tokens[0];
			std::string& type = tokens[1];
			std::size_t ts_from = utils::str2<std::size_t>(tokens[2]);
			std::size_t ts_to = utils::str2<std::size_t>(tokens[3]);
			std::size_t video_ID = utils::str2<std::size_t>(tokens[4]);
			std::size_t fr_from = utils::str2<std::size_t>(tokens[5]);
			std::size_t fr_to = utils::str2<std::size_t>(tokens[6]);

			_tasks.emplace_back(name, type, ts_from, ts_to, video_ID, fr_from, fr_to);
		}

		SHLOG_I("Tasks parsed.");
	}

	// ---
	std::tuple<VideoId, FrameNum, FrameNum> target(std::size_t ts) {
		for (auto&& t : _tasks) {
			auto [ts_fr, ts_to] = t.timestamps();
			if (ts_fr <= ts && ts <= ts_to) {
				return std::tuple(t.video_ID(), t.frame_from(), t.frame_to());
			}
		}

		std::string msg("No task found for timestamp '" + std::to_string(ts) + "'!");
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

private:
	std::vector<Task> _tasks;
};

};  // namespace sh