
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

EvalServerSettings::ServerConfigVbs parse_VBS_config(const json& json)
{
	return EvalServerSettings::ServerConfigVbs{ // .submit_URL
		                                        json["submit_URL"].get<std::string>(),
		                                        // .submit_rerank_URL
		                                        json["submit_rerank_URL"].get<std::string>(),
		                                        // .submit_interaction_URL
		                                        json["submit_interaction_URL"].get<std::string>()
	};
}

EvalServerSettings::ServerConfigDres parse_DRES_config(const json& json)
{
	return EvalServerSettings::ServerConfigDres{ // .cookie_file
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
		res.server_cfg = parse_VBS_config(json["server_config"][res.server_type]);
	} else if (res.server_type == "dres") {
		res.server_cfg = parse_DRES_config(json["server_config"][res.server_type]);
	}
	// If error value
	else {
		std::string msg{ "Uknown submit server type: " + res.server_type };
		SHLOG_E(msg);
		throw std::runtime_error(msg);
	}

	return res;
}

DatasetsSettings::PrimaryFeaturesSettings parse_primary_features_settings(const json& json)
{
	return DatasetsSettings::PrimaryFeaturesSettings{
		// .features_file_data_off
		require_int_value<size_t>(json, "features_file_data_off"),
		// .features_dim
		require_int_value<size_t>(json, "features_dim"),
		// .features_file
		require_string_value(json, "features_file"),

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
		// .collage_region_file_prefix
		require_string_value(json, "collage_region_file_prefix"),
		// .collage_regions
		require_int_value<size_t>(json, "collage_regions"),
	};
}

DatasetsSettings::SecondaryFeaturesSettings parse_secondary_features_settings(const json& json)
{
	return DatasetsSettings::SecondaryFeaturesSettings{ // .features_file_data_off
		                                                require_int_value<size_t>(json, "features_file_data_off"),
		                                                // .features_dim
		                                                require_int_value<size_t>(json, "features_dim"),
		                                                // .features_file
		                                                optional_value_or<std::string>(json, "features_file", "")
	};
}

DatasetsSettings::VideoFilenameOffsets parse_filename_offsets(const json& json)
{
	return DatasetsSettings::VideoFilenameOffsets{

		// .vid_ID_off
		require_int_value<size_t>(json, "fr_filename_vid_ID_off"),
		// .vid_ID_len
		require_int_value<size_t>(json, "fr_filename_vid_ID_len"),
		// .shot_ID_off
		require_int_value<size_t>(json, "fr_filename_shot_ID_off"),
		// .shot_ID_len
		require_int_value<size_t>(json, "fr_filename_shot_ID_len"),
		// .frame_num_off
		require_int_value<size_t>(json, "fr_filename_frame_num_off"),
		// .frame_num_len
		require_int_value<size_t>(json, "fr_filename_frame_num_len")

	};
}

TestsSettings parse_tests_settings(const json& json)
{
	return TestsSettings{ require_string_value(json, "test_data_root") };
}

PresentationViewsSettings parse_presentation_views_settings(const json& json)
{
	return PresentationViewsSettings{ // .display_page_size
		                              require_int_value<size_t>(json, "display_page_size"),
		                              // .topn_frames_per_video
		                              require_int_value<size_t>(json, "topn_frames_per_video"),
		                              // .topn_frames_per_shot
		                              require_int_value<size_t>(json, "topn_frames_per_shot")
	};
}

LoggerSettings parse_logger_settings(const json& /*json*/)
{
	return LoggerSettings{
		// ...
	};
}

RemoteServicesSettings::ClipQueryToVec parse_clip_settings(const json& json)
{
	return RemoteServicesSettings::ClipQueryToVec{ // .address
		                                           optional_string_value(json, "address")
	};
}
RemoteServicesSettings::MediaServer parse_media_server_settings(const json& json)
{
	return RemoteServicesSettings::MediaServer{ // .address
		                                        optional_string_value(json, "address")
	};
}

RemoteServicesSettings parse_remote_services_settings(const json& json)
{
	return RemoteServicesSettings{ parse_clip_settings(json["CLIP_query_to_vec"]),
		                           parse_media_server_settings(json["media_server"]) };
}

ModelsSettings parse_model_settings(const json& json)
{
	return ModelsSettings{
		// .models_dir
		require_string_value(json, "models_dir"),
		// .model_W2VV_img_bias
		require_string_value(json, "model_W2VV_img_bias"),
		// .model_W2VV_img_weigths
		require_string_value(json, "model_W2VV_img_weigths"),

		// .model_ResNet_file
		optional_string_value(json, "model_ResNet_file"),
		// .model_ResNet_SHA256
		optional_string_value(json, "model_ResNet_SHA256"),
		// .model_ResNext_file
		optional_string_value(json, "model_ResNext_file"),
		// .model_ResNext_SHA256
		optional_string_value(json, "model_ResNext_SHA256"),

	};
}

DatasetsSettings parse_datasets_settings(const json& json)
{
	return DatasetsSettings{ // .data_dir
		                     require_string_value(json, "data_dir"),
		                     // .frames_dir
		                     require_string_value(json, "frames_dir"),
		                     // .thumbs_dir
		                     require_string_value(json, "thumbs_dir"),
		                     // .LSC_metadata_file (optional)
		                     optional_value<std::string>(json, "LSC_metadata_file"),
		                     // .frames_list_file
		                     require_string_value(json, "frames_list_file"),
		                     // .filename_offsets
		                     parse_filename_offsets(json["filename_offsets"]),

		                     // .primary_features
		                     parse_primary_features_settings(json["primary_features"]),
		                     // .secondary_features
		                     parse_secondary_features_settings(json["secondary_features"])
	};
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
	auto ss = Settings{ 
		// .test
		parse_tests_settings(json["tests"]),
		// .presentation_views
		parse_presentation_views_settings(json["presentation_views"]),
		// .logger
		parse_logger_settings(json["logger"]),
		// .API
		parse_API_config(json["API"]),
		// .eval_server
		parse_eval_server(json["eval_server"]),
		// .remote_services
		parse_remote_services_settings(json["remote_services"]),
		// .models
		parse_model_settings(json["models"]),
		// .datasets
		parse_datasets_settings(json["datasets"])
	};
	// clang-format on

	return ss;
}
