/* This file is part of SOMHunter.
 *
 * Copyright (C) 2021 Frantisek Mejzlik <frankmejzlik@gmail.com>
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

#include "keyword-ranker.h"
// ---
#include <cmath>
// ---
#include "vector.hpp"

using namespace sh;

std::vector<Keyword> KeywordRanker::parse_kw_classes_text_file(const std::string& filepath,
                                                               const DatasetFrames& _dataset_frames) {
	std::ifstream inFile(filepath.c_str(), std::ios::in);

	SHLOG_D("Loading supported textual model keywords from '" << filepath << "'...");

	if (!inFile) {
		std::string msg{ "Error opening file: " + filepath };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	std::vector<Keyword> result_keywords;

	// read the input file by lines
	for (std::string line_text_buffer; std::getline(inFile, line_text_buffer);) {
		std::stringstream line_buffer_ss(line_text_buffer);

		std::vector<std::string> tokens;

		// Tokenize this line
		for (std::string token; std::getline(line_buffer_ss, token, ':');) {
			tokens.push_back(token);
		}

		// Parse wordnet synset ID
		SynsetId synset_ID{ utils::str2<SynsetId>(tokens[1]) };
		FrameId vec_idx{ FrameId(synset_ID) };

		// String representations
		SynsetStrings synset_strings{ utils::trim(tokens[0]) };

		do_assert(!synset_strings.empty(), "Cannot be empty!");
		for (auto&& s : synset_strings) {
			do_assert(s != "", "Cannot be empty string!");
		}

		// Top exmaple images
		std::vector<const VideoFrame*> top_ex_imgs;
		if (tokens.size() > 2) {
			{
				std::string token;
				for (std::stringstream top_ex_imgs_ss(tokens[2]); std::getline(top_ex_imgs_ss, token, '#');) {
					FrameId img_ID{ utils::str2<FrameId>(token) };

					top_ex_imgs.push_back(&_dataset_frames.get_frame(img_ID));
				}
			}
		}

		std::string description;
		if (tokens.size() > 3) {
			description = std::move(tokens[3]);
		}

		Keyword k{ vec_idx, synset_ID, std::move(synset_strings), std::move(description), std::move(top_ex_imgs) };

		// Insert this keyword
		result_keywords.emplace_back(std::move(k));
	}

	// Sort them by their ID
	std::sort(result_keywords.begin(), result_keywords.end(),
	          [](const Keyword& l, const Keyword& r) { return l.kw_ID < r.kw_ID; });

	SHLOG_S("Successfully loaded " << result_keywords.size() << " supported keywords.");

	return result_keywords;
}

FeatureVector KeywordRanker::parse_float_vector(const std::string& filepath, size_t dim, size_t begin_offset) {
	// Open file for reading as binary from the end side
	std::ifstream ifs(filepath, std::ios::binary | std::ios::ate);

	// If failed to open file
	if (!ifs) {
		std::string msg{ "Error opening file: " + filepath };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	// Get end of file
	auto end = ifs.tellg();

	// Get iterator to begining
	ifs.seekg(0, std::ios::beg);

	// Compute size of file
	auto size = std::size_t(end - ifs.tellg());

	// If emtpy file
	if (size == 0) {
		std::string msg{ "Empty file opened: " + filepath };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	// Calculate byte length of each row (dim_N * sizeof(float))
	size_t row_byte_len = dim * sizeof(float);

	// Create line buffer
	std::vector<char> line_byte_buffer;
	line_byte_buffer.resize(row_byte_len);

	// Start reading at this offset
	ifs.ignore(begin_offset);

	// Initialize vector of floats for this row
	std::vector<float> features_vector;
	features_vector.reserve(dim);

	// Read binary "lines" until EOF
	while (ifs.read(line_byte_buffer.data(), row_byte_len)) {
		size_t curr_offset = 0;

		// Iterate through all floats in a row
		for (size_t i = 0; i < dim; ++i) {
			features_vector.emplace_back(*reinterpret_cast<float*>(line_byte_buffer.data() + curr_offset));

			curr_offset += sizeof(float);
		}

		// Read just one line
		break;
	}

	return features_vector;
}

FeatureMatrix KeywordRanker::parse_float_matrix(const std::string& filepath, size_t row_dim, size_t begin_offset) {
	// Open file for reading as binary from the end side
	std::ifstream ifs(filepath, std::ios::binary | std::ios::ate);

	// If failed to open file
	if (!ifs) {
		std::string msg{ "Error opening file: " + filepath };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	// Get end of file
	auto end = ifs.tellg();

	// Get iterator to begining
	ifs.seekg(0, std::ios::beg);

	// Compute size of file
	auto size = std::size_t(end - ifs.tellg());

	// If emtpy file
	if (size == 0) {
		std::string msg{ "Empty file opened: " + filepath };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	// Calculate byte length of each row (dim_N * sizeof(float))
	size_t row_byte_len = row_dim * sizeof(float);

	// Create line buffer
	std::vector<char> line_byte_buffer;
	line_byte_buffer.resize(row_byte_len);

	// Start reading at this offset
	ifs.ignore(begin_offset);

	// Declare result structure
	std::vector<std::vector<float>> result_features;

	// Read binary "lines" until EOF
	while (ifs.read(line_byte_buffer.data(), row_byte_len)) {
		// Initialize vector of floats for this row
		std::vector<float> features_vector;
		features_vector.reserve(row_dim);

		size_t curr_offset = 0;

		// Iterate through all floats in a row
		for (size_t i = 0; i < row_dim; ++i) {
			features_vector.emplace_back(*reinterpret_cast<float*>(line_byte_buffer.data() + curr_offset));

			curr_offset += sizeof(float);
		}

		// Insert this row into the result
		result_features.emplace_back(std::move(features_vector));
	}

	return result_features;
}

KwSearchIds KeywordRanker::find(const std::string& search, size_t num_limit) const {
	KwSearchIds r;
	KwSearchIds r2;

	for (KeywordId i = 0; i < _keyword_ranker.size(); ++i)
		for (size_t j = 0; j < _keyword_ranker[i].synset_strs.size(); ++j) {
			auto kw = _keyword_ranker[i];

			auto& s = kw.synset_strs[j];
			auto f = s.find(search);

			if (f == std::basic_string<char>::npos) continue;

			if (f == 0u)
				r.emplace_back(kw.kw_ID, j);
			else
				r2.emplace_back(kw.kw_ID, j);
		}

	std::sort(r.begin(), r.end(), [&](const KwSearchId& a, const KwSearchId& b) {
		return _keyword_ranker[a.first].synset_strs[a.second] < _keyword_ranker[b.first].synset_strs[b.second];
	});

	r.insert(r.end(), r2.begin(), r2.end());

	KwSearchIds res;
	for (size_t i = 0; i < num_limit; ++i) {
		if (i >= r.size()) break;

		res.emplace_back(r[i]);
	}

	return res;
}

StdVector<float> KeywordRanker::get_text_query_feature(const std::string& query_raw) {
	auto tokens{ tokenize_textual_query(query_raw) };

	if (tokens.empty()) {
		// We return uniformly distributed valid vector
		return math::vector::normalize(StdVector<float>(128, 0.3F));
	}

	auto decoded{ decode_keywords(tokens) };
	auto textual_query_vectors{ embedd_text_queries(decoded) };
	return textual_query_vectors;
}

std::vector<std::string> KeywordRanker::tokenize_textual_query(const std::string& sentence_query_raw) const {
	// Copy this sentence
	std::string sentence_query(sentence_query_raw);

	// Remove all unwanted charactes
	std::string illegal_chars = "\\/?!,.'\"";
	std::transform(sentence_query.begin(), sentence_query.end(), sentence_query.begin(), [&illegal_chars](char c) {
		// If found in illegal, make it space
		if (illegal_chars.find(c) != std::string::npos) return ' ';

		return c;
	});

	std::stringstream query_ss(sentence_query);

	std::string token_str;
	std::vector<std::string> query;
	while (query_ss >> token_str) {
		query.emplace_back(token_str);
	}
	return query;
}

std::vector<KeywordId> KeywordRanker::decode_keywords(const std::vector<std::string>& query) const {
	std::vector<KeywordId> pos_one_query;
	// Split tokens into temporal queries
	for (const auto& kw_word : query) {
		auto v = find(kw_word, 10);

		if (!v.empty()) pos_one_query.emplace_back(v.front().first);
	}

	return pos_one_query;
}

void KeywordRanker::rank_sentence_query(const std::string& sentence_query_raw, ScoreModel& model,
                                        const PrimaryFrameFeatures& _dataset_features, size_t temporal) const {
	auto tokens{ tokenize_textual_query(sentence_query_raw) };

	if (tokens.empty()) return;

	auto decoded{ decode_keywords(tokens) };

	// Get the most relevant images for this query
	//  Distance is from [0, 1]
	auto embedded{ embedd_text_queries(decoded) };

	// Compute the scores for each frame for this query
	std::vector<float> scores{ inverse_score_vector(embedded, _dataset_features) };

	// Update the model
	for (size_t i = 0; i < scores.size(); ++i) {
		model.adjust(temporal, i, scores[i]);
	}
}

StdVector<float> KeywordRanker::embedd_text_queries(const StdVector<KeywordId>& kws) const {
	// Initialize zero vector
	std::vector<float> score_vec(kw_pca_mean_vec.size(), 0.0f);

	// Accumuate scores for given keywords
	for (auto&& ID : kws) {
		score_vec = math::vector::add(score_vec, kw_features[ID]);
	}

	// Add bias
	score_vec = math::vector::add(score_vec, kw_features_bias_vec);

	// Apply hyperbolic tangent function
	std::transform(score_vec.begin(), score_vec.end(), score_vec.begin(),
	               [](const float& score) { return std::tanh(score); });

	std::vector<float> sentence_vec =
	    math::vector::mat_mult(kw_pca_mat, math::vector::sub(math::vector::normalize(score_vec), kw_pca_mean_vec));

	sentence_vec = math::vector::normalize(sentence_vec);

	return sentence_vec;
}

void KeywordRanker::report_results(const StdVector<std::pair<FrameId, float>>& sorted_results,
                                   const DatasetFrames& _dataset_frames, size_t num) {
	size_t i{ 0 };
	for (auto&& [frame_ID, score] : sorted_results) {
		++i;

		auto filepath{ _dataset_frames[frame_ID] };
		std::cout << i << "\t ";
		std::cout << std::fixed << std::setprecision(4) << score << " => " << filepath << std::endl;

		if (i >= num) break;
	}
}
