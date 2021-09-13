
#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <fstream>
#include <stdexcept>
#include <string>
#include <variant>

namespace sh
{


/** Config needed by the Submitter instance.
 *
 * \see ServerConfig
 * \see ServerConfigVbs
 * \see ServerConfigDres
 */

struct TestsSettings {
	std::string test_data_root;
};

struct PresentationViewsSettings {
	size_t display_page_size;
	size_t topn_frames_per_video;
	size_t topn_frames_per_shot;
};

struct LoggerSettings {
	// ...
};

struct ApiConfig {
	bool local_only;
	size_t port;
	std::string config_filepath;
	std::string docs_dir;
};

struct EvalServerSettings {
	/** Config for submitting to the older server.
	 *
	 * https://github.com/klschoef/vbsserver/
	 */
	struct ServerConfigVbs {
		std::string submit_URL;
		std::string submit_rerank_URL;
		std::string submit_interaction_URL;
	};

	/** Config for submitting to the DRES server.
	 *
	 * https://github.com/lucaro/DRES
	 */
	struct ServerConfigDres {
		std::string cookie_file;

		std::string username;
		std::string password;

		std::string submit_URL;
		std::string submit_rerank_URL;
		std::string submit_interaction_URL;

		std::string login_URL;
		std::string logout_URL;
		std::string session_URL;
		std::string server_time_URL;
	};

	using ServerConfig = std::variant<ServerConfigVbs, ServerConfigDres>;

	// ---

	ServerConfig server_cfg;

	bool do_network_requests;
	bool submit_LSC_IDs;
	bool allow_insecure;

	size_t team_ID;
	size_t member_ID;

	std::string log_dir_eval_server_requests;
	std::string log_dir_user_actions;
	std::string log_dir_user_actions_summary;
	std::string log_dir_results;
	std::string log_dir_queries;
	std::string log_file_suffix;
	bool extra_verbose_log;

	size_t send_logs_to_server_period;
	bool apply_log_action_timeout;
	size_t log_action_timeout;
	std::string server_type;
};

struct RemoteServicesSettings {
	struct ClipQueryToVec {
		std::string address;
	};

	struct MediaServer {
		std::string address;
	};

	ClipQueryToVec CLIP_query_to_vec;
	MediaServer media_server;
};

struct ModelsSettings {
	std::string models_dir;
	std::string model_W2VV_img_bias;
	std::string model_W2VV_img_weigths;
	std::string model_ResNet_file;
	std::string model_ResNet_SHA256;
	std::string model_ResNext_file;
	std::string model_ResNext_SHA256;
};

struct DatasetsSettings {
	struct VideoFilenameOffsets {
		size_t vid_ID_off;
		size_t vid_ID_len;
		size_t shot_ID_off;
		size_t shot_ID_len;
		size_t frame_num_off;
		size_t frame_num_len;
	};
	struct PrimaryFeaturesSettings {
		size_t features_file_data_off;
		size_t features_dim;
		std::string features_file;

		size_t pre_PCA_features_dim;
		std::string kw_bias_vec_file;
		std::string kw_scores_mat_file;
		std::string kw_PCA_mean_vec_file;
		std::string kw_PCA_mat_file;
		size_t kw_PCA_mat_dim;

		std::string kws_file;

		std::string collage_region_file_prefix;
		size_t collage_regions;
	};

	struct SecondaryFeaturesSettings {
		size_t features_file_data_off;
		size_t features_dim;
		std::string features_file;
	};

	// ---

	std::string data_dir;
	std::string frames_dir;
	std::string thumbs_dir;
	/** File with time and position for LSC datasets. For non-LSC could be empty. */
	std::string LSC_metadata_file;
	std::string frames_list_file;
	VideoFilenameOffsets filename_offsets;

	PrimaryFeaturesSettings primary_features;
	SecondaryFeaturesSettings secondary_features;
};

/** Parsed current config of the core.
 * \see ParseJsonConfig
 */
struct Settings {
	static Settings parse_JSON_config(const std::string& filepath);
	static Settings parse_JSON_config_string(const std::string& cfg_file_contents);

	// ---

	TestsSettings tests;
	PresentationViewsSettings presentation_views;
	LoggerSettings logger;
	ApiConfig API;
	EvalServerSettings eval_server;
	RemoteServicesSettings remote_services;
	ModelsSettings models;
	DatasetsSettings datasets;
};

};  // namespace sh

#endif  // SETTINGS_H_
