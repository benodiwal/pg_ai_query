#include "./include/utils.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
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

std::optional<std::string> validate_natural_language_query(
    const std::string& query,
    int max_query_length) {
  // Validate content first: ensure query exists before checking properties.
  if (query.empty() ||
      std::all_of(query.begin(), query.end(),
                  [](unsigned char c) { return std::isspace(c); })) {
    return "Query cannot be empty.";
  }
  if (max_query_length < 0) {
    return "Invalid maximum query length.";
  }
  if (query.length() > static_cast<size_t>(max_query_length)) {
    return "Query too long. Maximum " + std::to_string(max_query_length) +
           " characters allowed. Your query: " +
           std::to_string(query.length()) + " characters.";
  }
  return std::nullopt;
}

std::string formatAPIError(const std::string& raw_error) {
  std::string error_to_parse = raw_error;

  size_t json_start = raw_error.find('{');
  if (json_start != std::string::npos) {
    error_to_parse = raw_error.substr(json_start);
  }

  try {
    auto error_json = nlohmann::json::parse(error_to_parse);

    if (error_json.contains("error")) {
      auto error_obj = error_json["error"];
      std::string error_type =
          error_obj.contains("type") ? error_obj["type"].get<std::string>() : "";
      std::string error_msg =
          error_obj.contains("message") ? error_obj["message"].get<std::string>() : "";

      if (error_type == "not_found_error") {
        size_t model_pos = error_msg.find("model:");
        if (model_pos != std::string::npos) {
          std::string model_name = error_msg.substr(model_pos + 7);
          model_name.erase(0, model_name.find_first_not_of(" \t"));
          model_name.erase(model_name.find_last_not_of(" \t") + 1);
          return "Invalid model '" + model_name +
                 "'. Please check your configuration and use a valid model "
                 "name. "
                 "Common models: 'claude-sonnet-4-5-20250929' (Anthropic), "
                 "'gpt-4o' (OpenAI).";
        }
        return "Model not found. Please check your model configuration and "
               "ensure you're using a valid model name.";
      }

      if (error_type == "rate_limit_error" ||
          error_msg.find("rate limit") != std::string::npos ||
          error_msg.find("Rate limit") != std::string::npos) {
        return "Rate limit exceeded. Please wait a moment and try again, "
               "or check your API plan's rate limits.";
      }

      if (error_type == "authentication_error" ||
          error_type == "invalid_api_key" ||
          error_msg.find("API key") != std::string::npos ||
          error_msg.find("api_key") != std::string::npos ||
          error_msg.find("Incorrect API key") != std::string::npos) {
        return "Authentication failed. Please verify your API key in "
               "~/.pg_ai.config is correct and has not expired.";
      }

      if (error_type == "insufficient_quota" ||
          error_msg.find("quota") != std::string::npos ||
          error_msg.find("billing") != std::string::npos) {
        return "API quota exceeded or billing issue. Please check your "
               "account balance and billing settings with your AI provider.";
      }

      if (error_type == "invalid_request_error") {
        if (error_msg.find("context length") != std::string::npos ||
            error_msg.find("maximum") != std::string::npos ||
            error_msg.find("too long") != std::string::npos) {
          return "Request too large: the database schema and query exceed the "
                 "model's context window. Try using a model with a larger "
                 "context, or reduce the number of tables in your database.";
        }
        if (!error_msg.empty()) {
          return "Invalid request: " + error_msg;
        }
      }

      if (error_type == "server_error" || error_type == "overloaded_error" ||
          error_type == "service_unavailable") {
        return "The AI provider is temporarily unavailable. Please try again "
               "in a few moments.";
      }

      if (!error_msg.empty()) {
        return error_msg;
      }
    }
  } catch (const nlohmann::json::exception&) {
  }

  if (raw_error.find("Connection refused") != std::string::npos ||
      raw_error.find("connect") != std::string::npos) {
    return "Could not connect to the AI provider. Please check your network "
           "connection and any custom api_endpoint in your configuration.";
  }

  if (raw_error.find("timed out") != std::string::npos ||
      raw_error.find("Timeout") != std::string::npos) {
    return "Request timed out. Try increasing request_timeout_ms in your "
           "configuration, or simplify your query.";
  }

  return raw_error;
}

}  // namespace pg_ai::utils