#include "./include/utils.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "./include/logger.hpp"

namespace pg_ai::utils {

std::pair<bool, std::string> read_file(const std::string& filepath) {
  std::ifstream file(filepath, std::ios::binary | std::ios::ate);
  if (!file) {
    logger::Logger::error("Failed to open file: " + filepath);
    return {false, {}};
  }

  const auto size = file.tellg();
  if (size == -1) {
    logger::Logger::error("Invalid file size: " + filepath);
    return {false, {}};
  }

  file.seekg(0, std::ios::beg);

  std::string content(static_cast<std::size_t>(size), '\0');
  if (size > 0) {
    if (!file.read(&content[0], static_cast<std::streamsize>(size))) {
      logger::Logger::error("Failed to read file: " + filepath);
      return {false, {}};
    }
  }

  return {true, std::move(content)};
}

std::string read_file_or_throw(const std::string& filepath) {
  auto [success, content] = read_file(filepath);
  if (!success) {
    throw std::runtime_error("Failed to read file: " + filepath);
  }
  return std::move(content);
}

// CR-someday @benodiwal: This is the basic version of API Error formatting,
// there is a lot of place for improvement. Currently it focuses on wrong model
// names in conf relate errors.

// API error formatting,
//  checks for rate limit, authentication, quota, timeout, service
//  unavailability, model not found errors.
std::string formatAPIError(const std::string& provider,
                           int status_code,
                           const std::string& raw_error) {
  std::string error_to_parse = raw_error;

  size_t json_start = raw_error.find('{');
  if (json_start != std::string::npos) {
    error_to_parse = raw_error.substr(json_start);
  }

  try {
    auto error_json = nlohmann::json::parse(error_to_parse);

    if (error_json.contains("error")) {
      auto error_obj = error_json["error"];

      std::string error_type;
      std::string error_message;

      if (error_obj.contains("type")) {
        error_type = error_obj["type"];
      }

      if (error_obj.contains("message")) {
        error_message = error_obj["message"];
      }

      // converting to lower case for easier checks
      std::string error_lower = error_message;
      std::transform(error_lower.begin(), error_lower.end(),
                     error_lower.begin(),
                     [](unsigned char c) { return std::tolower(c); });

      // rate limit errors(429)
      if (status_code == 429 || error_type == "rate_limit_error" ||
          error_lower.find("rate") != std::string::npos) {
        return "Rate limit exceeded. Please wait before making more requests.";
      }

      // authentication errors(401)
      if (status_code == 401 || error_type == "authentication_error" ||
          error_lower.find("invalid_api_key") != std::string::npos ||
          error_lower.find("unauthorized") != std::string::npos) {
        return "Invalid API key for " + provider +
               ". "
               "Please check your ~/.pg_ai.config file.";
      }

      // quota errors(402)
      if (status_code == 402 || error_type == "payment_required" ||
          error_lower.find("quota") != std::string::npos ||
          error_lower.find("insufficient_quota") != std::string::npos) {
        return "API quota exceeded. Check your " + provider + " account usage.";
      }

      // timeout errors(408)
      if (status_code == 408 || error_type == "timeout_error" ||
          error_lower.find("timeout") != std::string::npos ||
          error_lower.find("timed out") != std::string::npos) {
        return "Request timed out. Try increasing request_timeout_ms in "
               "config.";
      }

      if (status_code == 503 || status_code == 502 || status_code == 504) {
        return provider +
               " service is temporarily unavailable. Try again later.";
      }

      if (error_type == "not_found_error") {
        if (error_obj.contains("message")) {
          std::string msg = error_obj["message"];

          size_t model_pos = msg.find("model:");
          if (model_pos != std::string::npos) {
            std::string model_name = msg.substr(model_pos + 7);
            model_name.erase(0, model_name.find_first_not_of(" \t"));
            model_name.erase(model_name.find_last_not_of(" \t") + 1);

            return "Invalid model '" + model_name +
                   "'. Please check your configuration and use a valid model "
                   "name. "
                   "Common models: 'claude-sonnet-4-5-20250929' (Anthropic), "
                   "'gpt-4o' (OpenAI).";
          }
        }
        return "Model not found. Please check your model configuration and "
               "ensure you're using a valid model name.";
      }

      if (status_code >= 400 && status_code < 500) {
        if (!error_message.empty()) {
          return "The request was invalid (" + std::to_string(status_code) +
                 "): " + error_message;
        }
        return "The request was invalid.";
      }

      if (!error_message.empty()) {
        return error_message;
      }
    }
  } catch (const nlohmann::json::exception&) {
    // Ignore JSON parsing errors and return raw error
  }

  return raw_error;
}

}  // namespace pg_ai::utils