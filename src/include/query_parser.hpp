#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace pg_ai {

// Forward declaration to avoid circular dependency
struct QueryResult;

/**
 * @brief Pure parsing functions for query generation responses
 *
 * These functions are extracted from QueryGenerator to allow unit testing
 * without PostgreSQL dependencies.
 */
class QueryParser {
 public:
  /**
   * @brief Extract SQL JSON from an LLM response
   *
   * Handles various response formats:
   * - Direct JSON object
   * - JSON embedded in markdown code blocks
   * - Raw SQL text (fallback)
   *
   * @param response The raw response text from the LLM
   * @return Parsed JSON object with sql, explanation, warnings, etc.
   */
  static nlohmann::json extractSQLFromResponse(const std::string& response);

  /**
   * @brief Parse a JSON response into a QueryResult struct
   *
   * @param response_text The raw response text from the LLM
   * @return QueryResult with parsed fields and success/error status
   */
  static QueryResult parseQueryResponse(const std::string& response_text);

  /**
   * @brief Check if a SQL query accesses system tables
   *
   * @param sql The SQL query to check
   * @return true if the query accesses information_schema or pg_catalog
   */
  static bool accessesSystemTables(const std::string& sql);

  /**
   * @brief Check if an explanation indicates an error condition
   *
   * @param explanation The explanation text to check
   * @param warnings Vector of warning messages
   * @return true if error indicators are found
   */
  static bool hasErrorIndicators(const std::string& explanation,
                                 const std::vector<std::string>& warnings);
};

}  // namespace pg_ai
