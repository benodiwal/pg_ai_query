#pragma once

#include <optional>
#include <string>
#include <utility>

namespace pg_ai::utils {

/**
 * @brief Validate natural language query input before sending to AI
 *
 * Checks length and empty/whitespace-only. Returns an error message if
 * invalid, or std::nullopt if valid.
 *
 * @param query The user's natural language query string
 * @param max_query_length Maximum allowed character count
 * @return std::nullopt if valid, or error message string if invalid
 */
std::optional<std::string> validate_natural_language_query(
    const std::string& query,
    int max_query_length);

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

/**
 * @brief Format AI API error response into user-friendly message
 *
 * Parses error responses from various AI provider APIs (OpenAI, Anthropic,
 * Gemini) and extracts the most relevant error information for display to
 * users. Handles both JSON error objects and plain text error messages.
 *
 * @param raw_error Raw error response string from the AI API
 * @return Formatted, user-friendly error message
 *
 * @example
 * // Input: {"error": {"message": "Invalid API key", "type":
 * "invalid_request_error"}}
 * // Output: "Invalid API key"
 * std::string formatted = formatAPIError(api_response);
 * std::cerr << "API Error: " << formatted << std::endl;
 */
std::string formatAPIError(const std::string& raw_error);

/**
 * @brief Check if SQL is a read-only SELECT query for safe EXPLAIN
 *
 * Returns true only when the first keyword (after whitespace and comments)
 * is SELECT. Rejects INSERT, UPDATE, DELETE, DROP, and other mutating or
 * DDL statements to prevent execution via EXPLAIN ANALYZE.
 *
 * @param sql The SQL string to validate
 * @return true if the query appears to be a SELECT-only statement
 */
bool is_select_only_query(const std::string& sql);

}  // namespace pg_ai::utils