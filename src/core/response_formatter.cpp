#include "../include/response_formatter.hpp"

#include <sstream>
#include <nlohmann/json.hpp>

namespace pg_ai {

std::string ResponseFormatter::formatResponse(const QueryResult& result,
                                            const config::Configuration& config) {
  if (config.use_formatted_response) {
    return createJSONResponse(result, config);
  } else {
    return createPlainTextResponse(result, config);
  }
}

std::string ResponseFormatter::createJSONResponse(const QueryResult& result,
                                                const config::Configuration& config) {
  nlohmann::json response;

  // Always include the query
  response["query"] = result.generated_query;
  response["success"] = result.success;

  // Add optional fields based on configuration
  if (config.show_explanation && !result.explanation.empty()) {
    response["explanation"] = result.explanation;
  }

  if (config.show_warnings && !result.warnings.empty()) {
    response["warnings"] = result.warnings;
  }

  if (config.show_suggested_visualization && !result.suggested_visualization.empty()) {
    response["suggested_visualization"] = result.suggested_visualization;
  }

  // Add metadata
  if (result.row_limit_applied) {
    response["row_limit_applied"] = true;
  }

  return response.dump(2);  // Pretty print with 2-space indentation
}

std::string ResponseFormatter::createPlainTextResponse(const QueryResult& result,
                                                     const config::Configuration& config) {
  std::ostringstream output;

  // Main query result
  output << result.generated_query;

  // Add explanation if enabled
  if (config.show_explanation && !result.explanation.empty()) {
    output << "\n\n-- Explanation:\n-- " << result.explanation;
  }

  // Add warnings if enabled
  if (config.show_warnings && !result.warnings.empty()) {
    output << "\n\n" << formatWarnings(result.warnings);
  }

  // Add suggested visualization if enabled
  if (config.show_suggested_visualization && !result.suggested_visualization.empty()) {
    output << "\n\n" << formatVisualization(result.suggested_visualization);
  }

  // Add metadata
  if (result.row_limit_applied) {
    output << "\n\n-- Note: Row limit was automatically applied to this query for safety";
  }

  return output.str();
}

std::string ResponseFormatter::formatWarnings(const std::vector<std::string>& warnings) {
  std::ostringstream output;

  if (warnings.size() == 1) {
    output << "-- Warning: " << warnings[0];
  } else {
    output << "-- Warnings:";
    for (size_t i = 0; i < warnings.size(); ++i) {
      output << "\n--   " << (i + 1) << ". " << warnings[i];
    }
  }

  return output.str();
}

std::string ResponseFormatter::formatVisualization(const std::string& visualization) {
  std::ostringstream output;

  output << "-- Suggested Visualization:\n-- " << visualization;

  return output.str();
}

}  // namespace pg_ai