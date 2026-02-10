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
  if (query.length() > static_cast<size_t>(max_query_length)) {
    return "Query too long. Maximum " + std::to_string(max_query_length) +
           " characters allowed. Your query: " +
           std::to_string(query.length()) + " characters.";
  }
  if (query.empty() ||
      std::all_of(query.begin(), query.end(),
                  [](unsigned char c) { return std::isspace(c); })) {
    return "Query cannot be empty.";
  }
  return std::nullopt;
}

// CR-someday @benodiwal: This is the basic version of API Error formatting,
// there is a lot of place for improvement. Currently it focuses on wrong model
// names in conf relate errors.
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

      if (error_obj.contains("type") &&
          error_obj["type"] == "not_found_error") {
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

      if (error_obj.contains("message")) {
        return error_obj["message"];
      }
    }
  } catch (const nlohmann::json::exception&) {
  }

  return raw_error;
}

}  // namespace pg_ai::utils