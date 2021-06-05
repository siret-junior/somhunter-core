
#include "settings.h"

#include "json-helpers.hpp"
#include "utils.hpp"

using namespace sh;

/** Parsees the JSON config file that holds initial config.
 *
 * That means basically what we have in config.h now (paths etc.)
 */
Config Config::parse_json_config(const std::string& filepath) {
	std::string cfg_file_contents(utils::read_whole_file(filepath));
	return parse_json_config_string(cfg_file_contents);
}

Config Config::parse_json_config_string(const std::string& cfg_file_contents) {
	std::string err;
	auto json_all{ json11::Json::parse(cfg_file_contents, err) };

	auto json{ json_all["core"] };

	if (!err.empty()) {
		std::string msg{ "Error parsing JSON config string." };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	std::string msg_missing_value{ "Missing config value" };

	// clang-format off
	auto cfg = Config{ 
		// .API_config
		Config::parse_API_config(json["API"]),
		// .user_token
		require_string_value(json, "user_token"),
		// .submitter_config
		parse_submitter_config(json["submitter_config"]),
		// .max_frame_filename_len
		require_int_value<size_t>(json, "max_frame_filename_len"),
		
		// .filename_offsets
		VideoFilenameOffsets{
			
			// .vid_ID_off
			require_int_value<size_t>(json["filename_offsets"], "fr_filename_vid_ID_off"),
			// .vid_ID_len
			require_int_value<size_t>(json["filename_offsets"], "fr_filename_vid_ID_len"),
			// .shot_ID_off
			require_int_value<size_t>(json["filename_offsets"], "fr_filename_shot_ID_off"),
			// .shot_ID_len
			require_int_value<size_t>(json["filename_offsets"], "fr_filename_shot_ID_len"),
			// .frame_num_off
			require_int_value<size_t>(json["filename_offsets"], "fr_filename_frame_num_off"),
			// .frame_num_len
			require_int_value<size_t>(json["filename_offsets"], "fr_filename_frame_num_len"),

		},

		// .frames_list_file
		require_string_value(json, "frames_list_file"),
		// .frames_dir
		require_string_value(json, "frames_dir"),
		// .thumbs_dir
		require_string_value(json, "thumbs_dir"),

		// .features_file_data_off
		require_int_value<size_t>(json, "features_file_data_off"),
		// .features_file
		require_string_value(json, "features_file"),
		// .features_dim
		require_int_value<size_t>(json, "features_dim"),

		// .pre_PCA_features_dim
		require_int_value<size_t>(json, "pre_PCA_features_dim"),
		// .kw_bias_vec_file
		require_string_value(json, "kw_bias_vec_file"),
		// .kw_scores_mat_file
		require_string_value(json, "kw_scores_mat_file"),
		// .kw_PCA_mean_vec_file
		require_string_value(json, "kw_PCA_mean_vec_file"),
		// .kw_PCA_mat_file
		require_string_value(json, "kw_PCA_mat_file"),
		// .kw_PCA_mat_dim
		require_int_value<size_t>(json, "kw_PCA_mat_dim"),

		// .kws_file
		require_string_value(json, "kws_file"),

		// .display_page_size
		require_int_value<size_t>(json, "display_page_size"),
		// .topn_frames_per_video
		require_int_value<size_t>(json, "topn_frames_per_video"),
		// .topn_frames_per_shot
		require_int_value<size_t>(json, "topn_frames_per_shot"),

		// .LSC_metadata_file
		json["LSC_metadata_file"].string_value(),
		
		// .model_W2VV_img_bias
		require_string_value(json, "model_W2VV_img_bias"),
		// .model_W2VV_img_weigths
		require_string_value(json, "model_W2VV_img_weigths"),
		// .model_ResNet_file
		require_string_value(json, "model_ResNet_file"),
		// .model_ResNext_file
		require_string_value(json, "model_ResNext_file"),

		// .collage_region_file_prefix
		require_string_value(json, "collage_region_file_prefix"),
		// .collage_regions
		require_int_value<size_t>(json, "collage_regions"),

		require_string_value(json, "test_data_root")
	};
	// clang-format on

	return cfg;
}

SubmitterConfig Config::parse_submitter_config(const json11::Json& json) {
	SubmitterConfig res;

	res.submit_to_VBS = json["submit_to_VBS"].bool_value();

	res.team_ID = size_t(json["team_ID"].int_value());
	res.member_ID = size_t(json["member_ID"].int_value());

	res.log_submitted_dir = json["log_submitted_dir"].string_value();
	res.log_actions_dir = json["log_actions_dir"].string_value();
	res.log_queries_dir = json["log_queries_dir"].string_value();
	res.log_requests_dir = json["log_requests_dir"].string_value();
	res.log_file_suffix = json["log_file_suffix"].string_value();
	res.extra_verbose_log = json["extra_verbose_log"].bool_value();

	res.send_logs_to_server_period = size_t(json["send_logs_to_server_period"].int_value());

	res.apply_log_action_timeout = json["apply_log_action_timeout_in_core"].bool_value();
	res.log_action_timeout = size_t(json["log_action_timeout"].int_value());

	// Parse a type of the submit server
	res.server_type = json["submit_server"].string_value();

	// Parse the correct JSON format based on the server type
	if (res.server_type == "vbs") {
		res.server_cfg = parse_vbs_config(json["server_config"][res.server_type]);
	} else if (res.server_type == "dres") {
		res.server_cfg = parse_dres_config(json["server_config"][res.server_type]);
	}
	// If error value
	else {
		std::string msg{ "Uknown submit server type: " + res.server_type };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	return res;
}

ApiConfig Config::parse_API_config(const json11::Json& json) {
	auto docs_dir{ json["docs_dir"].string_value() };

	if (docs_dir.back() != '/') {
		docs_dir.append("/");
		SHLOG_W("Appending '/' to the `docs_dir` value - '" << docs_dir << "'.");
	}

	return ApiConfig{ static_cast<size_t>(json["port"].int_value()), json["config_filepath"].string_value(), docs_dir };
}

ServerConfigVbs Config::parse_vbs_config(const json11::Json& json) {
	return ServerConfigVbs{ json["submit_URL"].string_value(), json["submit_rerank_URL"].string_value(),
		                    json["submit_interaction_URL"].string_value() };
}

ServerConfigDres Config::parse_dres_config(const json11::Json& json) {
	return ServerConfigDres{ json["submit_URL"].string_value(),
		                     json["submit_rerank_URL"].string_value(),
		                     json["submit_interaction_URL"].string_value(),

		                     json["cookie_file"].string_value(),

		                     json["login_URL"].string_value(),
		                     json["username"].string_value(),
		                     json["password"].string_value() };
}