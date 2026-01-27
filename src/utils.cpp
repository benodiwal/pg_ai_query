#include "./include/utils.hpp"

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

// CR-someday @benodiwal: This is the basic version of API Error formatting,
// there is a lot of place for improvement. Currently it focuses on wrong model
// names in conf relate errors.


std::string formatAPIError(const std::string& provider, int status_code, const std::string& error_body){
    if (status_code < 100 || status_code > 599) {
        return provider + " returned an invalid status code (" + std::to_string(status_code) + "): \n" + error_body;
    }

    const auto it = error_reasons.find(status_code);

    // Use references to avoid copying strings unnecessarily
    const std::string& err_reason = (it != error_reasons.end()) ? it->second.first : "Unknown";
    const std::string& err_message = (it != error_reasons.end()) ? it->second.second : "Unknown Reason";

    std::string formatted_error = provider + " returned " + std::to_string(status_code) +":  "+err_reason+ "  " + err_message +"\n";

    return formatted_error;
}



}