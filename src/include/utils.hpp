#pragma once
#include <string>
#include <utility>

namespace pg_ai::utils {

/**
 * @brief Read entire file contents into a string
 *
 * Attempts to read a file and returns a pair indicating success/failure.
 * On success, returns (true, file_contents). On failure, returns (false,
 * error_message).
 *
 * @param filepath Path to the file to read (absolute or relative)
 * @return std::pair<bool, std::string> where first element is success flag
 *         and second element is either file contents or error message
 *
 * @example
 * auto [success, content] = read_file("/path/to/config.ini");
 * if (success) {
 *   std::cout << "File contents: " << content << std::endl;
 * } else {
 *   std::cerr << "Error: " << content << std::endl;
 * }
 */
std::pair<bool, std::string> read_file(const std::string& filepath);

/**
 * @brief Read entire file contents or throw exception on failure
 *
 * Reads a file and returns its contents. Unlike read_file(), this function
 * throws an exception if the file cannot be read.
 *
 * @param filepath Path to the file to read
 * @return String containing the complete file contents
 * @throws std::runtime_error if file does not exist, cannot be opened, or read
 * fails
 *
 * @example
 * try {
 *   std::string config = read_file_or_throw("~/.pg_ai.config");
 *   // Process config...
 * } catch (const std::runtime_error& e) {
 *   std::cerr << "Failed to read config: " << e.what() << std::endl;
 * }
 */
std::string read_file_or_throw(const std::string& filepath);

std::string formatAPIError(const std::string& provider,
                           int status_code,
                           const std::string& raw_error);

}  // namespace pg_ai::utils