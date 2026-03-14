#include "../include/response_formatter.hpp"

#include <sstream>
#include <string>
#include <nlohmann/json.hpp>

namespace pg_ai {
namespace {

std::string format_multiline_comment(const std::string& text,
                                     const std::string& prefix = "--   ",
                                     std::size_t max_width = 70) {
  std::string result;
  std::istringstream stream(text);
  std::string word;
  std::string current_line = prefix;

  while (stream >> word) {
    const std::size_t space_needed = current_line.length() + word.length() +
                                     (current_line != prefix ? 1 : 0);
    if (space_needed > max_width) {
      result += current_line + "\n";
      current_line = prefix + word;
    } else {
      if (current_line != prefix)
        current_line += " ";
      current_line += word;
    }
  }
  if (current_line != prefix) {
    result += current_line;
  }
  return result;
}
}  //  anonymous namespace

std::string ResponseFormatter::formatResponse(
    const QueryResult& result,
    const config::Configuration& config) {
  if (config.use_formatted_response) {
    return createJSONResponse(result, config);
  } else {
    return createPlainTextResponse(result, config);
  }
}

std::string ResponseFormatter::createJSONResponse(
    const QueryResult& result,
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

  if (config.show_suggested_visualization &&
      !result.suggested_visualization.empty()) {
    response["suggested_visualization"] = result.suggested_visualization;
  }

  // Add metadata
  if (result.row_limit_applied) {
    response["row_limit_applied"] = true;
  }

  return response.dump(2);  // Pretty print with 2-space indentation
}

std::string ResponseFormatter::createPlainTextResponse(
    const QueryResult& result,
    const config::Configuration& config) {
  std::ostringstream output;

  // Main query result
  output << "-- Query:\n";
  output << result.generated_query;

  // Add explanation if enabled
  if (config.show_explanation && !result.explanation.empty()) {
    output << "\n\n-- Explanation:\n";
    output << format_multiline_comment(result.explanation);
  }

  // Add warnings if enabled
  if (config.show_warnings && !result.warnings.empty()) {
    output << "\n\n" << formatWarnings(result.warnings);
  }

  // Add suggested visualization if enabled
  if (config.show_suggested_visualization &&
      !result.suggested_visualization.empty()) {
    output << "\n\n" << formatVisualization(result.suggested_visualization);
  }

  // Add metadata
  if (result.row_limit_applied) {
    output << "\n\n-- Note: Row limit was automatically applied to this query "
              "for safety";
  }

  return output.str();
}

std::string ResponseFormatter::formatWarnings(
    const std::vector<std::string>& warnings) {
  std::ostringstream output;

  if (warnings.size() == 1) {
    output << "-- Warning:\n";
    output << format_multiline_comment(warnings[0]);
  } else {
    output << "-- Warnings:\n";
    for (size_t i = 0; i < warnings.size(); ++i) {
      std::string numbered = std::to_string(i + 1) + ". " + warnings[i];
      output << format_multiline_comment(numbered, "--   ", 70);
      if (i + 1 < warnings.size()) {
        output << "\n";
      }
    }
  }
  return output.str();
}

std::string ResponseFormatter::formatVisualization(
    const std::string& visualization) {
  std::ostringstream output;

  output << "-- Suggested Visualization:\n";
  output << format_multiline_comment(visualization);

  return output.str();
}

}  // namespace pg_ai