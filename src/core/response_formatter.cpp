#include "../include/response_formatter.hpp"

#include <sstream>
#include <nlohmann/json.hpp>

namespace pg_ai {

namespace {

// Wraps text at word boundaries to fit within max_width (default 78 chars)
std::string wrapText(const std::string& text,
                     const std::string& first_line_prefix,
                     const std::string& continuation_prefix,
                     std::size_t max_width = 78) {
  std::istringstream stream(text);
  std::string word;
  std::string result;
  std::string current_line = first_line_prefix;
  std::string current_prefix = first_line_prefix;

  while (stream >> word) {
    const std::size_t separator_width =
        (current_line.length() == current_prefix.length()) ? 0 : 1;

    if (current_line.length() + separator_width + word.length() > max_width) {
      if (!result.empty()) {
        result += "\n";
      }
      result += current_line;
      current_line = continuation_prefix + word;
      current_prefix = continuation_prefix;
    } else {
      if (current_line.length() > current_prefix.length()) {
        current_line += " ";
      }
      current_line += word;
    }
  }

  if (current_line.length() > current_prefix.length()) {
    if (!result.empty()) {
      result += "\n";
    }
    result += current_line;
  }

  return result;
}

}  // namespace

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

  response["query"] = result.generated_query;
  response["success"] = result.success;

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

  if (result.row_limit_applied) {
    response["row_limit_applied"] = true;
  }

  return response.dump(2);
}

std::string ResponseFormatter::createPlainTextResponse(
    const QueryResult& result,
    const config::Configuration& config) {
  std::ostringstream output;

  output << "-- Query:\n" << result.generated_query;

  if (config.show_explanation && !result.explanation.empty()) {
    output << "\n\n-- Explanation:\n"
           << wrapText(result.explanation, "--   ", "--   ");
  }

  if (config.show_warnings && !result.warnings.empty()) {
    output << "\n\n" << formatWarnings(result.warnings);
  }

  if (config.show_suggested_visualization &&
      !result.suggested_visualization.empty()) {
    output << "\n\n" << formatVisualization(result.suggested_visualization);
  }

  if (result.row_limit_applied) {
    output << "\n\n-- Note: Row limit was automatically applied to this query "
              "for safety";
  }

  return output.str();
}

std::string ResponseFormatter::formatWarnings(
    const std::vector<std::string>& warnings) {
  std::ostringstream output;

  output << "-- Warnings:";
  for (size_t i = 0; i < warnings.size(); ++i) {
    const std::string number_prefix =
        "--   " + std::to_string(i + 1) + ". ";
    const std::string continuation_prefix = "--      ";
    
    output << "\n"
           << wrapText(warnings[i], number_prefix, continuation_prefix);
  }

  return output.str();
}

std::string ResponseFormatter::formatVisualization(
    const std::string& visualization) {
  std::ostringstream output;

  output << "-- Suggested Visualization:\n"
         << wrapText(visualization, "--   ", "--   ");

  return output.str();
}

}  // namespace pg_ai