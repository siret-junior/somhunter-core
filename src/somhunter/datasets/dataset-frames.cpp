/* This file is part of SOMHunter.
 *
 * Copyright (C) 2021 Frantisek Mejzlik <frankmejzlik@gmail.com>
 *                    Mirek Kratochvil <exa.exa@gmail.com>
 *                    Patrik Vesely <prtrikvesely@gmail.com>
 *                    Vit Skrhak <v.skrhak@gmail.com>
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

#include <fstream>

#include "common.h"

#include "dataset-features.h"
#include "somhunter.h"

using namespace sh;

std::vector<std::vector<KeywordId>> DatasetFrames::parse_top_kws_for_imgs_text_file(const std::string& filepath) {
	std::ifstream inFile(filepath.c_str(), std::ios::in);

	SHLOG_D("Loading top image keywords from '" << filepath << "'...");
	if (!inFile) {
		std::string msg{ "Error opening file: " + filepath };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	// pure hypers need to be loaded first because of IDs
	std::vector<std::vector<KeywordId>> result_vec;

	// read the input file by lines
	size_t line_idx = 0;
	for (std::string line_text_buffer; std::getline(inFile, line_text_buffer);) {
		std::stringstream line_buffer_ss(line_text_buffer);

		std::vector<std::string> tokens;

		for (std::string token; std::getline(line_buffer_ss, token, '~');) tokens.push_back(token);

		if (tokens.size() < 2) {
			std::string msg{ "Error parsing file: " + filepath };
			SHLOG_E(msg);
			throw std::runtime_error(msg);
		}

		std::string token;  // tmp

		// Keywrod IDs
		std::vector<KeywordId> kw_IDs;
		for (std::stringstream kw_IDs_ss(tokens[1]); std::getline(kw_IDs_ss, token, '#');)
			kw_IDs.emplace_back(utils::str2<KeywordId>(token));

		// Insert this image's top keywords
		result_vec.emplace_back(std::move(kw_IDs));

		++line_idx;
	}

	SHLOG_S("Textual model keywords loaded.");
	return result_vec;
}

DatasetFrames::DatasetFrames(const Settings& config) {
	const auto& c{ config.datasets };
	// Save the config values
	frames_dir = c.frames_dir;
	thumbs_dir = c.thumbs_dir;

	offs = c.filename_offsets;

	SHLOG_D("Loading frames from '" << c.frames_list_file << "'...");

	// Open the "frames list" file
	std::ifstream in(c.frames_list_file);
	if (!in.good()) {
		auto msg{ "Failed to open " + c.frames_list_file };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	bool parse_metadata{ false };
	std::ifstream ifs_meta;

	// If metadata file available
	if (c.LSC_metadata_file.has_value()) {
		parse_metadata = true;

		ifs_meta.open(c.LSC_metadata_file.value());
		if (!ifs_meta.good()) {
			auto msg{ "Failed to open " + c.LSC_metadata_file.value() };
			SHLOG_E(msg);
			throw std::runtime_error(msg);
		}
	}

	{
		FrameId i = 0;
		size_t prev_frame_vid_ID = SIZE_T_ERR_VAL;

		size_t beg_img_ID = SIZE_T_ERR_VAL;
		std::vector<std::pair<size_t, size_t>> range_pairs;

		std::string md_line;
		for (std::string s; getline(in, s); ++i) {
			std::string tmp;

			auto vf = DatasetFrames::parse_video_filename(std::move(s));

			if (parse_metadata) {
				// Read one metadata line
				if (!std::getline(ifs_meta, md_line)) {
					auto msg{ "Not enough lines in " + c.LSC_metadata_file.value() };
					SHLOG_E(msg);
					throw std::runtime_error(msg);
				}

				// Make sure there is no extra '\r' at the end
				if (md_line.back() == '\r') {
					md_line.pop_back();
				}

				// Parse this line
				auto fd = DatasetFrames::parse_metadata_line(md_line);

				vf.weekday = fd.weekday;
				vf.hour = fd.hour;
				vf.year = fd.year;
				vf.LSC_id = std::move(fd.LSC_id);
			}

			vf.frame_ID = i;

			// Current video ID
			size_t curr_frame_vid_ID = vf.video_ID;

			_frames.emplace_back(std::move(vf));

			// If we parsed all images from given video
			if (prev_frame_vid_ID != curr_frame_vid_ID) {
				size_t last_item_ID = _frames.size() - 1;

				// End previous range
				if (prev_frame_vid_ID != SIZE_T_ERR_VAL) {
					range_pairs.emplace_back(beg_img_ID, last_item_ID);
				}
				// Start the new one
				beg_img_ID = last_item_ID;

				prev_frame_vid_ID = curr_frame_vid_ID;
			}
		}
		// End the last FrameRange
		range_pairs.emplace_back(beg_img_ID, _frames.size());

		/*
		 * Now _frames container won't resize anymore.
		 * Transfer ID pairs into iterators...
		 */
		auto base_it = _frames.begin();
		for (auto&& [from_ID, to_ID] : range_pairs) {
			_video_ID_to_frame_range.emplace_back(base_it + from_ID, base_it + to_ID);
		}
	}

	if (size() == 0_z) {
		SHLOG_E("No frames loaded");
	} else {
		SHLOG_S("Loaded " << size() << " dataset frames.");
	}
}

VideoFrame DatasetFrames::parse_video_filename(std::string&& filename) {
	// Extract string representing video ID
	std::string videoIdString(filename.data() + offs.vid_ID_off, offs.vid_ID_len);

	// Extract string representing shot ID
	std::string shotIdString(filename.data() + offs.shot_ID_off, offs.shot_ID_len);

	// Extract string representing frame number
	std::string frameNumberString(filename.data() + offs.frame_num_off, offs.frame_num_len);

	return VideoFrame(std::move(filename), utils::str_to_int(videoIdString), utils::str_to_int(shotIdString),
	                  utils::str_to_int(frameNumberString), 0);
}

FiltersData DatasetFrames::parse_metadata_line(const std::string& line) {
	// !!! Beware that the data MUST NOT contain the separator char
	// TODO: Use propper CSV parser
	auto tokens{ utils::split(line, ';') };

	auto year_str{ tokens[2].substr(0, 4) };
	auto hour_str{ tokens[2].substr(11, 2) };

	Hour h{ static_cast<Hour>(utils::str2<size_t>(hour_str)) };

	Year y{ static_cast<Year>(utils::str2<size_t>(year_str)) };

	Weekday wd{ static_cast<Weekday>(utils::str2<size_t>(tokens[5])) };

	std::string LSC_id{ tokens[7] };

	return FiltersData{ wd, h, y, LSC_id };
}

std::vector<VideoFramePointer> DatasetFrames::ids_to_video_frame(const std::vector<FrameId>& ids) const {
	std::vector<VideoFramePointer> res;
	res.reserve(ids.size());
	for (FrameId i : ids) {
		if (i == IMAGE_ID_ERR_VAL) {
			res.push_back(nullptr);
			continue;
		}

		res.push_back(&get_frame(i));
	}

	return res;
}

std::vector<VideoFramePointer> DatasetFrames::range_to_video_frame(const FrameRange& ids) {
	std::vector<VideoFramePointer> res;
	res.reserve(ids.size());
	for (auto iter = ids.begin(); iter != ids.end(); ++iter) {
		res.push_back(&(*iter));
	}

	return res;
}
