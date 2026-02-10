#include "../../include/gemini_client.h"

#include <sstream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace gemini {

namespace {

// Callback for libcurl to write response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
  ((std::string*)userp)->append((char*)contents, size * nmemb);
  return size * nmemb;
}

}  // namespace

GeminiClient::GeminiClient(const std::string& api_key) : api_key_(api_key) {
  curl_global_init(CURL_GLOBAL_DEFAULT);
}

std::string GeminiClient::build_request_body(const GeminiRequest& request) {
  nlohmann::json body;

  // Build contents array
  nlohmann::json contents = nlohmann::json::array();
  nlohmann::json user_message;
  user_message["parts"] = nlohmann::json::array();
  user_message["parts"].push_back({{"text", request.user_prompt}});
  contents.push_back(user_message);
  body["contents"] = contents;

  // Add system instruction if provided
  if (!request.system_prompt.empty()) {
    nlohmann::json system_instruction;
    system_instruction["parts"] = nlohmann::json::array();
    system_instruction["parts"].push_back({{"text", request.system_prompt}});
    body["systemInstruction"] = system_instruction;
  }

  // Add generation config
  nlohmann::json generation_config;
  if (request.temperature.has_value()) {
    generation_config["temperature"] = request.temperature.value();
  }
  if (request.max_tokens.has_value()) {
    generation_config["maxOutputTokens"] = request.max_tokens.value();
  }

  if (!generation_config.empty()) {
    body["generationConfig"] = generation_config;
  }

  return body.dump();
}

GeminiResponse GeminiClient::parse_response(const std::string& body,
                                            int status_code) {
  GeminiResponse response;
  response.status_code = status_code;

  if (status_code != 200) {
    response.success = false;
    try {
      auto json = nlohmann::json::parse(body);
      if (json.contains("error")) {
        auto error = json["error"];
        response.error_message = error.value("message", "Unknown error");
        if (error.contains("code")) {
          response.error_message = "Error " +
                                   std::to_string(error["code"].get<int>()) +
                                   ": " + response.error_message;
        }
      } else {
        response.error_message = "HTTP " + std::to_string(status_code);
      }
    } catch (const std::exception& e) {
      response.error_message =
          "HTTP " + std::to_string(status_code) + ": " + body;
    }
    return response;
  }

  try {
    auto json = nlohmann::json::parse(body);

    // Extract text from: candidates[0].content.parts[0].text
    if (json.contains("candidates") && json["candidates"].is_array() &&
        !json["candidates"].empty()) {
      auto& candidate = json["candidates"][0];

      if (candidate.contains("content")) {
        auto& content = candidate["content"];

        if (content.contains("parts") && content["parts"].is_array() &&
            !content["parts"].empty()) {
          auto& part = content["parts"][0];

          if (part.contains("text")) {
            response.text = part["text"].get<std::string>();
            response.success = true;
            return response;
          }
        }
      }
    }

    response.success = false;
    response.error_message = "Invalid response format: missing text content";
  } catch (const std::exception& e) {
    response.success = false;
    response.error_message = std::string("JSON parse error: ") + e.what();
  }

  return response;
}

namespace {
class CurlHandle {
 public:
  CurlHandle() : handle_(curl_easy_init()) {
    if (!handle_) {
      throw std::runtime_error("Failed to initialize CURL");
    }
  }

  ~CurlHandle() {
    if (handle_) {
      curl_easy_cleanup(handle_);
    }
  }

  CurlHandle(const CurlHandle&) = delete;
  CurlHandle& operator=(const CurlHandle&) = delete;

  CurlHandle(CurlHandle&& other) noexcept : handle_(other.handle_) {
    other.handle_ = nullptr;
  }

  CURL* get() const { return handle_; }
  operator CURL*() const { return handle_; }

 private:
  CURL* handle_;
};

class CurlSlist {
 public:
  CurlSlist() : list_(nullptr) {}

  ~CurlSlist() {
    if (list_) {
      curl_slist_free_all(list_);
    }
  }

  // Throws runtime error on allocation failure
  void append(const std::string& value) {
    curl_slist* new_list = curl_slist_append(list_, value.c_str());
    if (!new_list) {
      throw std::runtime_error("Failed to append CURL header");
    }
    list_ = new_list;
  }

  curl_slist* get() const { return list_; }

  CurlSlist(const CurlSlist&) = delete;
  CurlSlist& operator=(const CurlSlist&) = delete;

 private:
  curl_slist* list_;
};
}  // namespace

GeminiResponse GeminiClient::make_http_request(const std::string& url,
                                               const std::string& body) {
  GeminiResponse response;

  try {
    CurlHandle curl;
    CurlSlist headers;

    std::string response_body;
    long http_code = 0;

    // Set URL
    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());

    // Set headers
    headers.append("Content-Type: application/json");
    headers.append("x-goog-api-key: " + api_key_);
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());

    // Set POST data
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, body.c_str());

    // Set write callback
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response_body);

    // Perform request
    CURLcode res = curl_easy_perform(curl.get());

    if (res != CURLE_OK) {
      response.success = false;
      response.error_message =
          std::string("CURL error: ") + curl_easy_strerror(res);
    } else {
      curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);
      response = parse_response(response_body, static_cast<int>(http_code));
    }
    return response;
  } catch (const std::exception& e) {
    response.success = false;
    response.error_message = e.what();
    return response;
  }
}

GeminiResponse GeminiClient::generate_text(const GeminiRequest& request) {
  // Build URL
  std::string url = std::string(BASE_URL) + "/" + API_VERSION + "/models/" +
                    request.model + ":generateContent";

  // Build request body
  std::string body = build_request_body(request);

  // Make HTTP request
  return make_http_request(url, body);
}

}  // namespace gemini
