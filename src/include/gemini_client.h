#pragma once

#include <optional>
#include <string>

namespace gemini {

struct GeminiRequest {
  std::string model;
  std::string system_prompt;
  std::string user_prompt;
  std::optional<double> temperature;
  std::optional<int> max_tokens;
};

struct GeminiResponse {
  std::string text;
  bool success;
  std::string error_message;
  int status_code;
};

class GeminiClient {
 public:
  explicit GeminiClient(const std::string& api_key);
  ~GeminiClient() = default;

  GeminiResponse generate_text(const GeminiRequest& request);

 private:
  std::string api_key_;
  static constexpr const char* BASE_URL =
      "https://generativelanguage.googleapis.com";
  static constexpr const char* API_VERSION = "v1beta";

  std::string build_request_body(const GeminiRequest& request);
  GeminiResponse parse_response(const std::string& body, int status_code);
  GeminiResponse make_http_request(const std::string& url,
                                   const std::string& body);
};

}  // namespace gemini
