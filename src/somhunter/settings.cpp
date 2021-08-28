
#include "settings.h"

#include "json-helpers.hpp"
#include "utils.hpp"

using namespace sh;
using namespace nlohmann;

ApiConfig parse_API_config(const json& json)
{
	auto docs_dir{ require_string_value(json, "docs_dir") };

	if (docs_dir.back() != '/') {
		docs_dir.append("/");
		SHLOG_W("Appending '/' to the `docs_dir` value - '" << docs_dir << "'.");
	}

	return ApiConfig{ // .local_only
		              require_bool_value(json, "local_only"),

		              // .port
		              require_int_value<std::size_t>(json, "port"),

		              // .config_filepath
		              require_string_value(json, "config_filepath"),

		              // .docs_dir
		              docs_dir
	};
}

ServerConfigVbs parse_vbs_config(const json& json)
{
	return ServerConfigVbs{ // .submit_URL
		                    json["submit_URL"].get<std::string>(),
		                    // .submit_rerank_URL
		                    json["submit_rerank_URL"].get<std::string>(),
		                    // .submit_interaction_URL
		                    json["submit_interaction_URL"].get<std::string>()
	};
}

ServerConfigDres parse_dres_config(const json& json)
{
	return ServerConfigDres{ // .cookie_file
		                     json["cookie_file"].get<std::string>(),
		                     // .username
		                     require_string_value(json, "username"),
		                     // .password
		                     require_string_value(json, "password"),
		                     // .submit_URL
		                     require_string_value(json, "submit_URL"),
		                     // .submit_rerank_URL
		                     require_string_value(json, "submit_rerank_URL"),
		                     // .submit_interaction_URL
		                     require_string_value(json, "submit_interaction_URL"),
		                     // .login_URL
		                     require_string_value(json, "login_URL"),
		                     // .logout_URL
		                     require_string_value(json, "logout_URL"),
		                     // .session_URL
		                     require_string_value(json, "session_URL"),
		                     // .server_time_URL
		                     require_string_value(json, "server_time_URL")
	};
}


EvalServerSettings parse_eval_server(const json& json)
{
	EvalServerSettings res;

	// .do_network_requests
	res.do_network_requests = require_bool_value(json, "do_network_requests"),
	// .submit_LSC_IDs
	    res.submit_LSC_IDs = require_bool_value(json, "submit_LSC_IDs"),
	// .allow_insecure
	    res.allow_insecure = require_bool_value(json, "allow_insecure"),

	res.team_ID = size_t(json["team_ID"].get<std::size_t>());
	res.member_ID = size_t(json["member_ID"].get<std::size_t>());

	res.log_dir_eval_server_requests = json["log_dir_eval_server_requests"].get<std::string>();
	res.log_dir_user_actions = json["log_dir_user_actions"].get<std::string>();
	res.log_dir_user_actions_summary = json["log_dir_user_actions_summary"].get<std::string>();
	res.log_dir_results = json["log_dir_results"].get<std::string>();
	res.log_dir_queries = json["log_dir_queries"].get<std::string>();
	res.log_file_suffix = json["log_file_suffix"].get<std::string>();
	res.extra_verbose_log = json["extra_verbose_log"].get<bool>();

	res.send_logs_to_server_period = size_t(json["send_logs_to_server_period"].get<std::size_t>());

	res.apply_log_action_timeout = true;  //< \todo
	res.log_action_timeout = size_t(json["log_action_timeout"].get<std::size_t>());

	// Parse a type of the submit server
	res.server_type = json["submit_server"].get<std::string>();

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

/** Parsees the JSON config file that holds initial config.
 *
 * That means basically what we have in config.h now (paths etc.)
 */
Settings Settings::parse_JSON_config(const std::string& filepath)
{
	std::string cfg_file_contents(utils::read_whole_file(filepath));
	return parse_JSON_config_string(cfg_file_contents);
}

Settings Settings::parse_JSON_config_string(const std::string& cfg_file_contents)
{
	json json_all = json::parse(cfg_file_contents);

	auto& json{ json_all["core"] };

	std::string msg_missing_value{ "Missing config value" };

	// clang-format off
	auto _logger_settings = Settings{ 
		// .API_config
		parse_API_config(json["API"]),
		// .eval_server
		parse_eval_server(json["eval_server"]),
		
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
		json["LSC_metadata_file"].get<std::string>(),
		
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

	return _logger_settings;
}
