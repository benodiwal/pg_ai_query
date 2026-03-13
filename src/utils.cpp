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

// CR-someday @benodiwal: This is the basic version of API Error formatting,
// there is a lot of place for improvement. Currently it focuses on wrong model
// names in conf relate errors.

std::string formatAPIError(const std::string& provider,
                           int status_code,
                           const std::string& raw_error) {
  // 1. High-Priority HTTP Status Mapping
  try {
    size_t start = raw_error.find('{');
    if (start != std::string::npos) {
      auto j = nlohmann::json::parse(raw_error.substr(start));
      if (j.contains("error") && j["error"].contains("message")) {
        std::string msg = j["error"]["message"];

        if (raw_error.find("not_found_error") != std::string::npos) {
          // If the message specifically mentions "model:", use "Invalid model"
          if (msg.find("model:") != std::string::npos) {
            return "Invalid model: " + msg;
          }
          // Fallback for other not_found_errors
          return "Model not found: " + msg;
        }
        return msg;
      }
    }
  } catch (...) {
    // Ignore parsing errors and move to keyword mapping
  }
  if (status_code == 429 || raw_error.find("rate_limit") != std::string::npos) {
    return "Rate limit exceeded. Please wait before making more requests.";
  } else if (status_code == 401 ||
             raw_error.find("invalid_api_key") != std::string::npos) {
    return "Invalid API key for " + provider +
           ". "
           "Please check your ~/.pg_ai.config file.";
  } else if (status_code == 402 ||
             raw_error.find("quota") != std::string::npos) {
    return "API quota exceeded. Check your " + provider + " account usage.";
  } else if (status_code == 408 || status_code == 504 ||
             raw_error.find("timeout") != std::string::npos) {
    return "Request timed out. Try increasing request_timeout_ms in config.";
  } else if (status_code == 503 || status_code == 502) {
    return provider + " service is temporarily unavailable. Try again later.";
  } else if (status_code >= 400 && status_code < 500) {
    return "Request error (" + std::to_string(status_code) + "): " + raw_error;
  } else if (status_code >= 500) {
    return provider +
           " service is temporarily unavailable. Please try again later.";
  } else if (provider == "Unknown" || provider.empty()) {
    return raw_error;
  } else {
    return provider + raw_error;
  }
}
}  // namespace pg_ai::utils