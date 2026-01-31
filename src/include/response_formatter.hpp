#pragma once

#include <string>
#include "config.hpp"
#include "query_generator.hpp"

namespace pg_ai {

/**
 * @brief Formats query results for display to users
 * 
 * Converts QueryResult structures into user-friendly output formats.
 * Supports both JSON and plain text (SQL comment) output styles based
 * on configuration settings.
 * 
 * @example
 * QueryResult result; // From query generation
 * Configuration config = ConfigManager::getConfig();
 * std::string output = ResponseFormatter::formatResponse(result, config);
 * std::cout << output << std::endl;
 */
class ResponseFormatter {
 public:
  /**
   * @brief Format query result based on configuration settings
   * 
   * Main entry point for formatting query results. Delegates to either
   * createJSONResponse or createPlainTextResponse based on the
   * use_formatted_response configuration flag.
   * 
   * @param result The query result to format
   * @param config Configuration settings controlling output format and content
   * @return Formatted response string (JSON or plain text)
   * 
   * @example
   * QueryResult result;
   * result.generated_query = "SELECT * FROM users LIMIT 100";
   * result.explanation = "Retrieves all user records";
   * result.success = true;
   * 
   * Configuration config;
   * config.use_formatted_response = false;
   * config.show_explanation = true;
   * 
   * std::string output = ResponseFormatter::formatResponse(result, config);
   * // Output:
   * // SELECT * FROM users LIMIT 100
   * //
   * // -- Explanation:
   * // -- Retrieves all user records
   */
  static std::string formatResponse(const QueryResult& result,
                                    const config::Configuration& config);

 private:
  /**
   * @brief Create JSON formatted response
   * 
   * Converts the query result into a structured JSON object with fields
   * for the query, explanation, warnings, and metadata. Only includes
   * fields that are enabled in the configuration.
   * 
   * @param result The query result to format
   * @param config Configuration controlling which fields to include
   * @return Pretty-printed JSON string
   */
  static std::string createJSONResponse(const QueryResult& result,
                                        const config::Configuration& config);

  /**
   * @brief Create plain text formatted response
   * 
   * Formats the query result as plain text with SQL-style comments for
   * explanations and warnings. Suitable for direct display in psql console.
   * 
   * @param result The query result to format
   * @param config Configuration controlling which sections to include
   * @return Plain text string with SQL comments
   */
  static std::string createPlainTextResponse(
      const QueryResult& result,
      const config::Configuration& config);

  /**
   * @brief Format warnings for display
   * 
   * Converts a vector of warning messages into formatted text with
   * SQL comment markers. Handles both single and multiple warnings.
   * 
   * @param warnings Vector of warning messages to format
   * @return Formatted warning text with "-- Warning:" or "-- Warnings:" prefix
   */
  static std::string formatWarnings(const std::vector<std::string>& warnings);

  /**
   * @brief Format suggested visualization for display
   * 
   * Converts a visualization suggestion into formatted text with SQL comments.
   * 
   * @param visualization Suggested visualization type (e.g., "bar_chart", "table")
   * @return Formatted text with "-- Suggested Visualization:" prefix
   */
  static std::string formatVisualization(const std::string& visualization);
};

}  // namespace pg_ai