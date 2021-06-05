
#ifndef TOOL_CONFIG_H
#define TOOL_CONFIG_H

#include <fstream>
#include <stdexcept>
#include <string>
#include <variant>

#include "json11.hpp"

namespace sh {

struct VideoFilenameOffsets {
	size_t vid_ID_off;
	size_t vid_ID_len;
	size_t shot_ID_off;
	size_t shot_ID_len;
	size_t frame_num_off;
	size_t frame_num_len;
};

struct ApiConfig {
	size_t port;
	std::string config_filepath;
	std::string docs_dir;
};

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
	std::string submit_URL;
	std::string submit_rerank_URL;
	std::string submit_interaction_URL;

	std::string cookie_file;

	std::string login_URL;
	std::string username;
	std::string password;
};

using ServerConfig = std::variant<ServerConfigVbs, ServerConfigDres>;

/** Config needed by the Submitter instance.
 *
 * \see ServerConfig
 * \see ServerConfigVbs
 * \see ServerConfigDres
 */
struct SubmitterConfig {
	/** If submit actions will create actual HTTP request */
	bool submit_to_VBS;

	ServerConfig server_cfg;

	size_t team_ID;
	size_t member_ID;

	std::string log_submitted_dir;
	std::string log_actions_dir;
	std::string log_queries_dir;
	std::string log_requests_dir;
	std::string log_file_suffix;
	bool extra_verbose_log;

	size_t send_logs_to_server_period;
	bool apply_log_action_timeout;
	size_t log_action_timeout;
	std::string server_type;
};

/** Parsed current config of the core.
 * \see ParseJsonConfig
 */
struct Config {
	ApiConfig API_config;
	std::string user_token;
	SubmitterConfig submitter_config;

	size_t max_frame_filename_len;
	VideoFilenameOffsets filename_offsets;

	std::string frames_list_file;
	std::string frames_dir;
	std::string thumbs_dir;

	size_t features_file_data_off;
	std::string features_file;
	size_t features_dim;

	size_t pre_PCA_features_dim;
	std::string kw_bias_vec_file;
	std::string kw_scores_mat_file;
	std::string kw_PCA_mean_vec_file;
	std::string kw_PCA_mat_file;
	size_t kw_PCA_mat_dim;

	std::string kws_file;

	size_t display_page_size;
	size_t topn_frames_per_video;
	size_t topn_frames_per_shot;

	std::string LSC_metadata_file;

	std::string model_W2VV_img_bias;
	std::string model_W2VV_img_weigths;
	std::string model_ResNet_file;
	std::string model_ResNext_file;

	std::string collage_region_file_prefix;
	size_t collage_regions;

	std::string test_data_root;

	static Config parse_json_config(const std::string& filepath);
	static Config parse_json_config_string(const std::string& cfg_file_contents);

private:
	static SubmitterConfig parse_submitter_config(const json11::Json& json);
	static ApiConfig parse_API_config(const json11::Json& json);
	static ServerConfigVbs parse_vbs_config(const json11::Json& json);
	static ServerConfigDres parse_dres_config(const json11::Json& json);
};

};  // namespace sh

#endif  // TOOL_CONFIG_H