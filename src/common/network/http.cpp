
#include "http.h"
// ---
#include <functional>
// ---
#include <curl/curl.h>
#include <regex>

using namespace sh;

#if DEBUG_CURL_REQUESTS

static void curl_dump(const char* text, FILE* stream, unsigned char* ptr, size_t size, char nohex) {
	size_t i;
	size_t c;

	unsigned int width = 0x10;

	if (nohex) /* without the hex output, we can fit more on screen */
		width = 0x40;

	fprintf(stream, "%s, %10.10lu bytes (0x%8.8lx)\n", text, (unsigned long)size, (unsigned long)size);

	for (i = 0; i < size; i += width) {
		fprintf(stream, "%4.4lx: ", (unsigned long)i);

		if (!nohex) {
			/* hex not disabled, show it */
			for (c = 0; c < width; c++)
				if (i + c < size)
					fprintf(stream, "%02x ", ptr[i + c]);
				else
					fputs("   ", stream);
		}

		for (c = 0; (c < width) && (i + c < size); c++) {
			/* check for 0D0A; if found, skip past and start a new
			 * line of output */
			if (nohex && (i + c + 1 < size) && ptr[i + c] == 0x0D && ptr[i + c + 1] == 0x0A) {
				i += (c + 2 - width);
				break;
			}
			fprintf(stream, "%c", (ptr[i + c] >= 0x20) && (ptr[i + c] < 0x80) ? ptr[i + c] : '.');
			/* check again for 0D0A, to avoid an extra \n if it's at
			 * width */
			if (nohex && (i + c + 2 < size) && ptr[i + c + 1] == 0x0D && ptr[i + c + 2] == 0x0A) {
				i += (c + 3 - width);
				break;
			}
		}
		fputc('\n', stream); /* newline */
	}
	fflush(stream);
}

static int trace_fn(CURL* handle, curl_infotype type, char* dt, size_t size, void* userp) {
	struct data* config = (struct data*)userp;
	const char* text;

	switch (type) {
		case CURLINFO_TEXT:
			std::cout << dt << std::endl;
			return 0;
			break;
		case CURLINFO_HEADER_OUT:
			text = "=> Send header";
			break;
		case CURLINFO_DATA_OUT:
			text = "=> Send data";
			break;
		case CURLINFO_SSL_DATA_OUT:
			text = "=> Send SSL data";
			break;
		case CURLINFO_HEADER_IN:
			text = "<= Recv header";
			break;
		case CURLINFO_DATA_IN:
			text = "<= Recv data";
			break;
		case CURLINFO_SSL_DATA_IN:
			text = "<= Recv SSL data";
			break;
	}

	curl_dump(text, stderr, (unsigned char*)dt, size, 0);
	return 0;
}

#endif  // DEBUG_CURL_REQUESTS

static size_t res_cb(char* contents, size_t size, size_t nmemb, void* userp) {
	static_cast<std::string*>(userp)->append(contents, size * nmemb);
	return size * nmemb;
}

static void request_worker(RequestType type, const std::string& submit_URL, const nlohmann::json& body,
                           const nlohmann::json& /*headers*/ = {},
                           std::function<void(ReqCode, std::vector<uint8_t>&&)> cb_succ = {},
                           std::function<void()> cb_err = {}, const std::string& cookie_filepath = "",
                           bool allow_insecure = false) {
	CURL* curl = curl_easy_init();
	// std::string res_buffer;

	std::string url = std::regex_replace(submit_URL, std::regex("\\s"), "%20");
	static std::string hdr = "Content-type: application/json";
	static struct curl_slist reqheader = { hdr.data(), nullptr };
	auto data_serialized{ body.dump() };

	// POST/GET
	switch (type) {
		case RequestType::GET:
			// Check
			do_assert(body.is_object() || body.is_null(), "Query is either null or dictionary.");

			// ***
			// Build the query

			// '?' character
			if (!body.is_null() && !body.empty()) {
				url.append("?");
			}

			// Data
			for (auto& el : body.items()) {
				std::string s;
				if (el.value().is_string()) {
					s.append(el.key()).append("=").append(el.value()).append("&");
				} else {
					std::stringstream ss;
					ss << el.key() << "=" << el.value() << "&";
					s = ss.str();
				}

				url.append(s);
			}

			if (!body.is_null()) {
				url.pop_back();
			}

			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
			break;

		case RequestType::POST:
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data_serialized.c_str());
			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data_serialized.length());
			break;
	}

	// URL
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

// Verbose CURL debug print
#if DEBUG_CURL_REQUESTS
	curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, trace_fn);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif  // DEBUG_CURL_REQUESTS

	// Insecurity
	if (allow_insecure) {
		// SHLOG_W("Doing insecure connection!");
		// Add `--insecure` option
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
	}

	// Cookies
	if (!cookie_filepath.empty()) {
		curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cookie_filepath.c_str());
		curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookie_filepath.c_str());
	}
	curl_easy_setopt(curl, CURLOPT_HEADER, 0);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, res_cb);

	std::string str_buffer;
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&str_buffer);

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, &reqheader);

	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30);
	// Add `-L` option
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	// Add `--post301 --post302 --post303` option
	curl_easy_setopt(curl, CURLOPT_POSTREDIR, CURL_REDIR_POST_ALL);

	// >>>
	auto res{ curl_easy_perform(curl) };
	// >>>

	long res_code;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);

	if (res == CURLE_OK) {
		SHLOG_D("HTTP request OK: '" << url << "'");
		std::vector<uint8_t> res_buffer(str_buffer.begin(), str_buffer.end());

		// Call the success
		cb_succ(static_cast<ReqCode>(res_code), std::move(res_buffer));
	} else {
		SHLOG_E("POST request failed with cURL error: " << curl_easy_strerror(res));

		// Call the success
		cb_err();
	}

	curl_easy_cleanup(curl);
}

Http::~Http() {
	for (auto& t : submit_threads) t.join();
}

void Http::do_POST_async(const std::string& URL, const nlohmann::json& body, const nlohmann::json& /*headers*/,
                         std::function<void(ReqCode, nlohmann::json)> cb_succ, std::function<void()> cb_err) {
	finish_flags.emplace_back(std::make_unique<bool>(false));

	submit_threads.emplace_back(request_worker, RequestType::POST, URL, body, nlohmann::json{}, cb_succ, cb_err, "",
	                            _allow_insecure);

	common_finish();
}

void Http::do_GET_async(const std::string& URL, const nlohmann::json& query, const nlohmann::json& /*headers*/,
                        std::function<void(ReqCode, nlohmann::json)> cb_succ, std::function<void()> cb_err) {
	finish_flags.emplace_back(std::make_unique<bool>(false));

	submit_threads.emplace_back(request_worker, RequestType::GET, URL, query, nlohmann::json{}, cb_succ, cb_err, "",
	                            _allow_insecure);

	common_finish();
}

std::pair<ReqCode, std::vector<uint8_t>> sh::Http::do_request_sync(const RequestType request_method,
                                                                   const std::string& URL, const nlohmann::json& body,
                                                                   const nlohmann::json& /*headers*/) {
	bool fail{ false };

	std::vector<uint8_t> res_data;
	ReqCode res_code;

	auto succ_fn = [&res_data, &res_code](ReqCode code, std::vector<uint8_t>&& res_buffer) {
		res_code = code;
		res_data = std::move(res_buffer);
	};

	auto err_fn = [&fail]() { fail = true; };

	std::thread worker{ &request_worker, request_method, URL, body,           nlohmann::json{},
		                succ_fn,         err_fn,         "",  _allow_insecure };
	worker.join();  //< Wait for the thread right away

	common_finish();
	return std::pair<ReqCode, std::vector<uint8_t>>{ res_code, res_data };
}

std::pair<ReqCode, nlohmann::json> sh::Http::do_POST_sync_json(const std::string& URL, const nlohmann::json& body,
                                                               const nlohmann::json& headers) {
	auto [res_code, res_buffer] = do_request_sync(RequestType::POST, URL, body, headers);
	nlohmann::json res_data;
	if (!res_buffer.empty()) res_data = nlohmann::json::parse(res_buffer);

	return std::pair<ReqCode, nlohmann::json>{ res_code, res_data };
}

std::pair<ReqCode, nlohmann::json> sh::Http::do_GET_sync_json(const std::string& URL, const nlohmann::json& body,
                                                              const nlohmann::json& headers) {
	auto [res_code, res_buffer] = do_request_sync(RequestType::GET, URL, body, headers);
	nlohmann::json res_data;
	if (!res_buffer.empty()) res_data = nlohmann::json::parse(res_buffer);

	return std::pair<ReqCode, nlohmann::json>{ res_code, res_data };
}

std::pair<ReqCode, std::vector<float>> sh::Http::do_GET_sync_floats(const std::string& URL, const nlohmann::json& body,
                                                                    const nlohmann::json& headers) {
	auto [res_code, res_buffer] = do_request_sync(RequestType::GET, URL, body, headers);
	do_assert(res_buffer.size() % sizeof(float) == 0, "Float array in binary should be aligned to multiple of 4!");

	std::vector<float> res_data(res_buffer.size() / sizeof(float), 0);
	std::copy(res_buffer.begin(), res_buffer.end(), (uint8_t*)res_data.data());

	return std::pair<ReqCode, std::vector<float>>{ res_code, res_data };
}
