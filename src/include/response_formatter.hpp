#pragma once

#include <string>
#include "query_generator.hpp"
#include "config.hpp"

namespace pg_ai {

class ResponseFormatter {
 public:
  /**
   * @brief Format query result based on configuration settings
   * @param result The query result to format
   * @param config Configuration settings for formatting
   * @return Formatted response string
   */
  static std::string formatResponse(const QueryResult& result,
                                   const config::Configuration& config);

 private:
  /**
   * @brief Create JSON formatted response
   */
  static std::string createJSONResponse(const QueryResult& result,
                                       const config::Configuration& config);

  /**
   * @brief Create plain text formatted response
   */
  static std::string createPlainTextResponse(const QueryResult& result,
                                           const config::Configuration& config);

  /**
   * @brief Format warnings for display
   */
  static std::string formatWarnings(const std::vector<std::string>& warnings);

  /**
   * @brief Format suggested visualization for display
   */
  static std::string formatVisualization(const std::string& visualization);
};

}  // namespace pg_ai