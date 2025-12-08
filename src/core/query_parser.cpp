#include "../include/query_parser.hpp"

#include <algorithm>
#include <regex>

#include "../include/query_generator.hpp"

namespace pg_ai {

nlohmann::json QueryParser::extractSQLFromResponse(const std::string& text) {
  // Try to find JSON in markdown code block
  std::regex json_block(R"(```(?:json)?\s*(\{[\s\S]*?\})\s*```)",
                        std::regex::icase);
  std::smatch match;

  if (std::regex_search(text, match, json_block)) {
    try {
      return nlohmann::json::parse(match[1].str());
    } catch (...) {
      // Continue to other parsing methods
    }
  }

  // Try to parse as direct JSON
  try {
    return nlohmann::json::parse(text);
  } catch (...) {
    // Continue to fallback
  }

  // Fallback: treat as raw SQL
  return {{"sql", text}, {"explanation", "Raw LLM output (no JSON detected)"}};
}

bool QueryParser::accessesSystemTables(const std::string& sql) {
  std::string upper_sql = sql;
  std::transform(upper_sql.begin(), upper_sql.end(), upper_sql.begin(),
                 ::toupper);
  return upper_sql.find("INFORMATION_SCHEMA") != std::string::npos ||
         upper_sql.find("PG_CATALOG") != std::string::npos;
}

bool QueryParser::hasErrorIndicators(const std::string& explanation,
                                     const std::vector<std::string>& warnings) {
  std::string lower_explanation = explanation;
  std::transform(lower_explanation.begin(), lower_explanation.end(),
                 lower_explanation.begin(), ::tolower);

  bool has_error =
      lower_explanation.find("cannot generate query") != std::string::npos ||
      lower_explanation.find("cannot create query") != std::string::npos ||
      lower_explanation.find("unable to generate") != std::string::npos ||
      lower_explanation.find("does not exist") != std::string::npos ||
      lower_explanation.find("do not exist") != std::string::npos ||
      lower_explanation.find("table not found") != std::string::npos ||
      lower_explanation.find("column not found") != std::string::npos ||
      lower_explanation.find("no such table") != std::string::npos ||
      lower_explanation.find("no such column") != std::string::npos;

  if (has_error) {
    return true;
  }

  for (const auto& warning : warnings) {
    std::string lower_warning = warning;
    std::transform(lower_warning.begin(), lower_warning.end(),
                   lower_warning.begin(), ::tolower);
    if (lower_warning.find("error:") != std::string::npos ||
        lower_warning.find("does not exist") != std::string::npos ||
        lower_warning.find("do not exist") != std::string::npos) {
      return true;
    }
  }

  return false;
}

QueryResult QueryParser::parseQueryResponse(const std::string& response_text) {
  nlohmann::json j = extractSQLFromResponse(response_text);
  std::string sql = j.value("sql", "");
  std::string explanation = j.value("explanation", "");

  std::vector<std::string> warnings_vec;
  try {
    if (j.contains("warnings")) {
      if (j["warnings"].is_array()) {
        warnings_vec = j["warnings"].get<std::vector<std::string>>();
      } else if (j["warnings"].is_string()) {
        warnings_vec.push_back(j["warnings"].get<std::string>());
      }
    }
  } catch (...) {
    // Ignore warnings parsing errors
  }

  // Check for error indicators in explanation/warnings
  if (hasErrorIndicators(explanation, warnings_vec)) {
    return QueryResult{.generated_query = "",
                       .explanation = explanation,
                       .warnings = warnings_vec,
                       .row_limit_applied = false,
                       .suggested_visualization = "",
                       .success = false,
                       .error_message = explanation};
  }

  // Handle empty SQL (but not an error)
  if (sql.empty()) {
    return QueryResult{.generated_query = "",
                       .explanation = explanation,
                       .warnings = warnings_vec,
                       .row_limit_applied = false,
                       .suggested_visualization = "",
                       .success = true,
                       .error_message = ""};
  }

  // Check for system table access
  if (accessesSystemTables(sql)) {
    return QueryResult{
        .generated_query = "",
        .explanation = "",
        .warnings = {},
        .row_limit_applied = false,
        .suggested_visualization = "",
        .success = false,
        .error_message =
            "Generated query accesses system tables. Please query user "
            "tables only."};
  }

  // Success case
  return QueryResult{
      .generated_query = sql,
      .explanation = explanation,
      .warnings = warnings_vec,
      .row_limit_applied = j.value("row_limit_applied", false),
      .suggested_visualization = j.value("suggested_visualization", "table"),
      .success = true,
      .error_message = ""};
}

}  // namespace pg_ai
