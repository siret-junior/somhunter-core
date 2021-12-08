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

#ifndef IMAGE_KEYWORDS_W2VV_H_
#define IMAGE_KEYWORDS_W2VV_H_

#include <cassert>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "common.h"

#include "utils.hpp"

#include "dataset-frames.h"
#include "embedding-ranker.h"
#include "scores.h"
#include "settings.h"

namespace sh {
struct Keyword {
	KeywordId kw_ID;
	SynsetId synset_ID;
	SynsetStrings synset_strs;
	KwDescription desc;

	/** Best representative images for this keyword */
	std::vector<const VideoFrame*> top_ex_imgs;
};

class KeywordRanker : public EmbeddingRanker<PrimaryFrameFeatures> {
	std::vector<Keyword> _keyword_ranker;
	FeatureMatrix kw_features;
	FeatureVector kw_features_bias_vec;
	FeatureMatrix kw_pca_mat;
	FeatureVector kw_pca_mean_vec;

public:
	static std::vector<Keyword> parse_kw_classes_text_file(const std::string& filepath,
	                                                       const DatasetFrames& _dataset_frames);

	/**
	 * Parses float matrix from a binary file that is written in row-major
	 * format and starts at `begin_offset` offset.k FORMAT: Matrix of 4B
	 * floats, row - major:
	 *    - each line is dim_N * 4B floats
	 *    - number of lines is number of selected frames
	 */
	// @todo Make this template and inside some `Parsers` class
	static FeatureMatrix parse_float_matrix(const std::string& filepath, size_t row_dim, size_t begin_offset = 0);
	/**
	 * FORMAT:
	 *    Matrix of 4B floats:
	 *    - each line is dim * 4B floats
	 *    - number of lines is number of selected frames
	 */
	// @todo Make this template and inside some `Parsers` class
	static FeatureVector parse_float_vector(const std::string& filepath, size_t dim, size_t begin_offset = 0);

	inline KeywordRanker(const Settings& config, const DatasetFrames& _dataset_frames)
	    : _keyword_ranker(parse_kw_classes_text_file(config.datasets.primary_features.kws_file, _dataset_frames)),

	      kw_features(parse_float_matrix(config.datasets.primary_features.kw_scores_mat_file,
	                                     config.datasets.primary_features.pre_PCA_features_dim)),
	      kw_features_bias_vec(parse_float_vector(config.datasets.primary_features.kw_bias_vec_file,
	                                              config.datasets.primary_features.pre_PCA_features_dim)),

	      kw_pca_mat(parse_float_matrix(config.datasets.primary_features.kw_PCA_mat_file,
	                                    config.datasets.primary_features.pre_PCA_features_dim)),
	      kw_pca_mean_vec(parse_float_vector(config.datasets.primary_features.kw_PCA_mean_vec_file,
	                                         config.datasets.primary_features.pre_PCA_features_dim)) {
		assert(kw_pca_mat.size() > 1);  // Make sure it's a matrix!

		SHLOG_S("Keyword features loaded from '" << config.datasets.primary_features.kw_scores_mat_file
		                                         << "' with dimension ("
		                                         << config.datasets.primary_features.pre_PCA_features_dim << ").");
		SHLOG_S("Keyword bias loaded from '" << config.datasets.primary_features.kw_bias_vec_file
		                                     << "' with dimension ("
		                                     << config.datasets.primary_features.pre_PCA_features_dim << ").");

		SHLOG_S("Loaded PCA matrix from '" << config.datasets.primary_features.kw_PCA_mat_file << "' with dimension ("
		                                   << config.datasets.primary_features.pre_PCA_features_dim << " ,"
		                                   << config.datasets.primary_features.kw_PCA_mat_dim << ").");
		SHLOG_S("Loaded PCA mean vector from '" << config.datasets.primary_features.kw_bias_vec_file
		                                        << "' with dimension ("
		                                        << config.datasets.primary_features.pre_PCA_features_dim << ").");
	}

	KeywordRanker(const KeywordRanker&) = delete;
	KeywordRanker& operator=(const KeywordRanker&) = delete;

	KeywordRanker(KeywordRanker&&) = default;
	KeywordRanker& operator=(KeywordRanker&&) = default;
	~KeywordRanker() noexcept = default;

	static void report_results(const StdVector<std::pair<FrameId, float>>& sorted_results,
	                           const DatasetFrames& _dataset_frames, size_t num = 10);

	StdVector<float> embedd_text_queries(const StdVector<KeywordId>& kws) const;

	/**
	 * Gets all string representants of this keyword.
	 */
	const Keyword& operator[](KeywordId idx) const {
		// Get all keywords with this Keyword ID
		return _keyword_ranker[idx];
	}

	KwSearchIds find(const std::string& search, size_t num_limit) const;

	void rank_sentence_query(const std::string& sentence_query_raw, ScoreModel& model,
	                         const PrimaryFrameFeatures& _dataset_features, size_t temporal) const;

	// ----
	StdVector<float> get_text_query_feature(const std::string& query);
	std::vector<std::string> tokenize_textual_query(const std::string& sentence_query_raw) const;
	std::vector<KeywordId> decode_keywords(const std::vector<std::string>& query) const;

	// -----
};
};  // namespace sh

#endif  // IMAGE_KEYWORDS_W2VV_H_
