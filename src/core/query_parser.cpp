#include "../include/query_parser.hpp"

#include <algorithm>
#include <regex>

#include "../include/logger.hpp"
#include "../include/query_generator.hpp"

namespace pg_ai {

nlohmann::json QueryParser::extractSQLFromResponse(const std::string& text) {
  // ------------------------------------------------------------
  // Attempt to extract JSON embedded in markdown code blocks.
  //
  // AI/LLM responses often wrap structured output like JSON
  // inside markdown fences, e.g.:
  //
  // ```json
  // {
  //   "sql": "SELECT * FROM users",
  //   "explanation": "Fetch all users"
  // }
  // ```
  //
  // Regex explanation:
  // ```              -> opening markdown fence
  // (?:json)?        -> optional "json" language identifier
  // \s*              -> optional whitespace/newlines
  // (\{[\s\S]*?\})   -> CAPTURE GROUP: JSON object {...}
  // \s*
  // ```              -> closing markdown fence
  //
  // This allows us to safely extract the raw JSON for parsing.
  // ------------------------------------------------------------
  std::regex json_block(R"(```(?:json)?\s*(\{[\s\S]*?\})\s*```)",
                        std::regex::icase);
  std::smatch match;

  if (std::regex_search(text, match, json_block)) {
    try {
      return nlohmann::json::parse(match[1].str());
    } catch (const nlohmann::json::parse_error& e) {
      logger::Logger::debug("JSON parse error in markdown block: " +
                            std::string(e.what()));
    } catch (const std::exception& e) {
      logger::Logger::warning("Unexpected error parsing markdown JSON: " +
                              std::string(e.what()));
    }
  }

  // Try to parse as direct JSON
  try {
    return nlohmann::json::parse(text);
  } catch (const nlohmann::json::parse_error& e) {
    logger::Logger::debug("JSON parse error (direct): " +
                          std::string(e.what()));
  } catch (const std::exception& e) {
    logger::Logger::warning("Unexpected error parsing direct JSON: " +
                            std::string(e.what()));
  }

  // ------------------------------------------------------------
  // Fallback: Treat the entire response as raw SQL text.
  //
  // This ensures we still return a usable structure even when
  // the AI output is not valid JSON.
  // ------------------------------------------------------------
  return {{"sql", text}, {"explanation", "Raw LLM output (no JSON detected)"}};
}

bool QueryParser::accessesSystemTables(const std::string& sql) {
  // ------------------------------------------------------------
  // Detect access to system / catalog tables.
  //
  // Why this is blocked:
  // - Prevents exposure of internal database metadata
  // - Avoids security risks and privilege escalation
  // - Ensures AI-generated queries only target user data
  //
  // Tables checked:
  // - INFORMATION_SCHEMA (SQL-standard metadata)
  // - PG_CATALOG (PostgreSQL internal catalog)
  // ------------------------------------------------------------
  std::string upper_sql = sql;
  std::transform(upper_sql.begin(), upper_sql.end(), upper_sql.begin(),
                 ::toupper);
  return upper_sql.find("INFORMATION_SCHEMA") != std::string::npos ||
         upper_sql.find("PG_CATALOG") != std::string::npos;
}

bool QueryParser::hasErrorIndicators(const std::string& explanation,
                                     const std::vector<std::string>& warnings) {
  // ------------------------------------------------------------
  // Scan the AI explanation text for phrases that indicate
  // query generation failure.
  //
  // These keywords are based on common LLM failure responses
  // and database-style error messages.
  // ------------------------------------------------------------
  std::string lower_explanation = explanation;
  std::transform(lower_explanation.begin(), lower_explanation.end(),
                 lower_explanation.begin(), ::tolower);

  bool has_error =
      // Explicit AI failure statements
      lower_explanation.find("cannot generate query") != std::string::npos ||
      lower_explanation.find("cannot create query") != std::string::npos ||
      lower_explanation.find("unable to generate") != std::string::npos ||
      // Missing schema elements
      lower_explanation.find("does not exist") != std::string::npos ||
      lower_explanation.find("do not exist") != std::string::npos ||
      // Database-style error messages
      lower_explanation.find("table not found") != std::string::npos ||
      lower_explanation.find("column not found") != std::string::npos ||
      lower_explanation.find("no such table") != std::string::npos ||
      lower_explanation.find("no such column") != std::string::npos;

  if (has_error) {
    return true;
  }
  // ------------------------------------------------------------
  // Also scan warnings for error indicators.
  //
  // Some LLMs place failure signals inside warnings instead of
  // the main explanation field.
  // ------------------------------------------------------------
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
  // Parse SQL, explanation, and metadata from the AI response
  nlohmann::json j = extractSQLFromResponse(response_text);
  std::string sql = j.value("sql", "");
  std::string explanation = j.value("explanation", "");

  std::vector<std::string> warnings_vec;
  try {
    // ------------------------------------------------------------
    // Extract warnings from JSON.
    //
    // Supported formats:
    // 1. Array:   "warnings": ["msg1", "msg2"]
    // 2. String:  "warnings": "single warning"
    //
    // This flexible handling improves robustness against
    // varying AI output formats.
    // ------------------------------------------------------------
    if (j.contains("warnings")) {
      if (j["warnings"].is_array()) {
        warnings_vec = j["warnings"].get<std::vector<std::string>>();
      } else if (j["warnings"].is_string()) {
        warnings_vec.push_back(j["warnings"].get<std::string>());
      }
    }
  } catch (const std::exception& e) {
    logger::Logger::warning("Error parsing warnings from JSON: " +
                            std::string(e.what()));
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
