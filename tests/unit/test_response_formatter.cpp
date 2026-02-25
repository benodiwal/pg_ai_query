#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "../test_helpers.hpp"
#include "include/config.hpp"
#include "include/query_generator.hpp"
#include "include/response_formatter.hpp"

using namespace pg_ai;
using namespace pg_ai::config;
using namespace pg_ai::test_utils;
using json = nlohmann::json;

class ResponseFormatterTest : public ::testing::Test {
 protected:
  QueryResult createBasicResult() {
    return QueryResult{.generated_query = "SELECT * FROM users",
                       .explanation = "Retrieves all users",
                       .warnings = {},
                       .row_limit_applied = false,
                       .suggested_visualization = "table",
                       .success = true,
                       .error_message = ""};
  }

  QueryResult createResultWithWarnings() {
    return QueryResult{.generated_query = "SELECT * FROM large_table",
                       .explanation = "Query may be slow",
                       .warnings = {"Consider adding LIMIT", "Full table scan"},
                       .row_limit_applied = true,
                       .suggested_visualization = "table",
                       .success = true,
                       .error_message = ""};
  }

  Configuration createConfig(bool formatted,
                             bool showExplanation,
                             bool showWarnings,
                             bool showVisualization) {
    Configuration config;
    config.use_formatted_response = formatted;
    config.show_explanation = showExplanation;
    config.show_warnings = showWarnings;
    config.show_suggested_visualization = showVisualization;
    return config;
  }
};

// Test plain text output - basic query only
TEST_F(ResponseFormatterTest, PlainTextBasicQuery) {
  auto result = createBasicResult();
  auto config = createConfig(false, false, false, false);

  std::string output = ResponseFormatter::formatResponse(result, config);

  EXPECT_EQ(output, "-- Query:\nSELECT * FROM users");
}

// Test plain text output with explanation
TEST_F(ResponseFormatterTest, PlainTextWithExplanation) {
  auto result = createBasicResult();
  auto config = createConfig(false, true, false, false);

  std::string output = ResponseFormatter::formatResponse(result, config);

  EXPECT_THAT(output, testing::HasSubstr("-- Query:\nSELECT * FROM users"));
  EXPECT_THAT(output, testing::HasSubstr("-- Explanation:"));
  EXPECT_THAT(output, testing::HasSubstr("--   Retrieves all users"));
}

// Test plain text output with single warning
TEST_F(ResponseFormatterTest, PlainTextWithSingleWarning) {
  auto result = createBasicResult();
  result.warnings = {"Performance may be slow"};
  auto config = createConfig(false, false, true, false);

  std::string output = ResponseFormatter::formatResponse(result, config);

  EXPECT_THAT(output, testing::HasSubstr("-- Warnings:"));
  EXPECT_THAT(output, testing::HasSubstr("--   1. Performance may be slow"));
}

// Test plain text output with multiple warnings
TEST_F(ResponseFormatterTest, PlainTextWithMultipleWarnings) {
  auto result = createResultWithWarnings();
  auto config = createConfig(false, false, true, false);

  std::string output = ResponseFormatter::formatResponse(result, config);

  EXPECT_THAT(output, testing::HasSubstr("-- Warnings:"));
  EXPECT_THAT(output, testing::HasSubstr("--   1. Consider adding LIMIT"));
  EXPECT_THAT(output, testing::HasSubstr("--   2. Full table scan"));
}

// Test plain text output with visualization
TEST_F(ResponseFormatterTest, PlainTextWithVisualization) {
  auto result = createBasicResult();
  result.suggested_visualization = "bar_chart";
  auto config = createConfig(false, false, false, true);

  std::string output = ResponseFormatter::formatResponse(result, config);

  EXPECT_THAT(output, testing::HasSubstr("-- Suggested Visualization:"));
  EXPECT_THAT(output, testing::HasSubstr("--   bar_chart"));
}

// Test plain text output with row limit note
TEST_F(ResponseFormatterTest, PlainTextWithRowLimitNote) {
  auto result = createBasicResult();
  result.row_limit_applied = true;
  auto config = createConfig(false, false, false, false);

  std::string output = ResponseFormatter::formatResponse(result, config);

  EXPECT_THAT(output,
              testing::HasSubstr("Row limit was automatically applied"));
}

// Test plain text output with all options enabled
TEST_F(ResponseFormatterTest, PlainTextAllOptions) {
  auto result = createResultWithWarnings();
  result.suggested_visualization = "line_chart";
  auto config = createConfig(false, true, true, true);

  std::string output = ResponseFormatter::formatResponse(result, config);

  EXPECT_THAT(output, testing::HasSubstr("-- Query:\nSELECT * FROM large_table"));
  EXPECT_THAT(output, testing::HasSubstr("-- Explanation:"));
  EXPECT_THAT(output, testing::HasSubstr("-- Warnings:"));
  EXPECT_THAT(output, testing::HasSubstr("-- Suggested Visualization:"));
  EXPECT_THAT(output,
              testing::HasSubstr("Row limit was automatically applied"));
}

// Test plain text explanation wraps long text
TEST_F(ResponseFormatterTest, PlainTextExplanationWrapsLongText) {
  auto result = createBasicResult();
  result.explanation =
      "This explanation is intentionally long to verify that wrapping keeps "
      "lines readable in a standard terminal window without overflowing the "
      "expected width.";
  auto config = createConfig(false, true, false, false);

  std::string output = ResponseFormatter::formatResponse(result, config);

  EXPECT_THAT(output, testing::HasSubstr("-- Explanation:"));
  EXPECT_THAT(output,
              testing::HasSubstr("--   This explanation is intentionally "
                                 "long to verify that wrapping keeps"));
  EXPECT_THAT(output,
              testing::HasSubstr("--   "));  // Verify continuation lines
}

// Test plain text warnings wrap with proper continuation indentation
TEST_F(ResponseFormatterTest, PlainTextWarningsWrapWithContinuationIndent) {
  auto result = createBasicResult();
  result.warnings = {
      "This warning message is long enough to force wrapping "
      "across multiple lines for easier readability in psql terminal windows."};
  auto config = createConfig(false, false, true, false);

  std::string output = ResponseFormatter::formatResponse(result, config);

  EXPECT_THAT(output, testing::HasSubstr("-- Warnings:"));
  EXPECT_THAT(output,
              testing::HasSubstr("--   1. This warning message is long enough "
                                 "to force wrapping"));
  EXPECT_THAT(output,
              testing::HasSubstr("--      "));  // Verify continuation indentation
}

// Test plain text output is terminal-friendly (80 char limit)
TEST_F(ResponseFormatterTest, PlainTextRespects80CharLimit) {
  auto result = createBasicResult();
  result.explanation =
      "A very long explanation text that contains many words and should be "
      "wrapped to ensure that no single line exceeds the standard terminal "
      "width of 80 characters which is important for readability.";
  auto config = createConfig(false, true, false, false);

  std::string output = ResponseFormatter::formatResponse(result, config);

  // Check each line is within limit
  std::istringstream stream(output);
  std::string line;
  while (std::getline(stream, line)) {
    EXPECT_LE(line.length(), 80)
        << "Line exceeds 80 characters: " << line;
  }
}

// Test JSON output - basic query
TEST_F(ResponseFormatterTest, JSONBasicQuery) {
  auto result = createBasicResult();
  auto config = createConfig(true, false, false, false);

  std::string output = ResponseFormatter::formatResponse(result, config);
  json j = json::parse(output);

  EXPECT_EQ(j["query"], "SELECT * FROM users");
  EXPECT_TRUE(j["success"]);
  EXPECT_FALSE(j.contains("explanation"));
  EXPECT_FALSE(j.contains("warnings"));
  EXPECT_FALSE(j.contains("suggested_visualization"));
}

// Test JSON output with explanation
TEST_F(ResponseFormatterTest, JSONWithExplanation) {
  auto result = createBasicResult();
  auto config = createConfig(true, true, false, false);

  std::string output = ResponseFormatter::formatResponse(result, config);
  json j = json::parse(output);

  EXPECT_EQ(j["query"], "SELECT * FROM users");
  EXPECT_EQ(j["explanation"], "Retrieves all users");
}

// Test JSON output with warnings
TEST_F(ResponseFormatterTest, JSONWithWarnings) {
  auto result = createResultWithWarnings();
  auto config = createConfig(true, false, true, false);

  std::string output = ResponseFormatter::formatResponse(result, config);
  json j = json::parse(output);

  EXPECT_TRUE(j.contains("warnings"));
  EXPECT_TRUE(j["warnings"].is_array());
  EXPECT_EQ(j["warnings"].size(), 2);
  EXPECT_EQ(j["warnings"][0], "Consider adding LIMIT");
  EXPECT_EQ(j["warnings"][1], "Full table scan");
}

// Test JSON output with visualization
TEST_F(ResponseFormatterTest, JSONWithVisualization) {
  auto result = createBasicResult();
  result.suggested_visualization = "pie_chart";
  auto config = createConfig(true, false, false, true);

  std::string output = ResponseFormatter::formatResponse(result, config);
  json j = json::parse(output);

  EXPECT_EQ(j["suggested_visualization"], "pie_chart");
}

// Test JSON output with row_limit_applied
TEST_F(ResponseFormatterTest, JSONWithRowLimit) {
  auto result = createBasicResult();
  result.row_limit_applied = true;
  auto config = createConfig(true, false, false, false);

  std::string output = ResponseFormatter::formatResponse(result, config);
  json j = json::parse(output);

  EXPECT_TRUE(j["row_limit_applied"]);
}

// Test JSON output does NOT include row_limit_applied when false
TEST_F(ResponseFormatterTest, JSONNoRowLimitWhenFalse) {
  auto result = createBasicResult();
  result.row_limit_applied = false;
  auto config = createConfig(true, false, false, false);

  std::string output = ResponseFormatter::formatResponse(result, config);
  json j = json::parse(output);

  EXPECT_FALSE(j.contains("row_limit_applied"));
}

// Test JSON output with all options
TEST_F(ResponseFormatterTest, JSONAllOptions) {
  auto result = createResultWithWarnings();
  result.suggested_visualization = "scatter";
  auto config = createConfig(true, true, true, true);

  std::string output = ResponseFormatter::formatResponse(result, config);
  json j = json::parse(output);

  EXPECT_EQ(j["query"], "SELECT * FROM large_table");
  EXPECT_TRUE(j["success"]);
  EXPECT_EQ(j["explanation"], "Query may be slow");
  EXPECT_EQ(j["warnings"].size(), 2);
  EXPECT_EQ(j["suggested_visualization"], "scatter");
  EXPECT_TRUE(j["row_limit_applied"]);
}

// Test empty explanation is not included
TEST_F(ResponseFormatterTest, EmptyExplanationNotIncluded) {
  auto result = createBasicResult();
  result.explanation = "";
  auto config =
      createConfig(true, true, false, false);  // show_explanation = true

  std::string output = ResponseFormatter::formatResponse(result, config);
  json j = json::parse(output);

  // Empty explanation should not be included even with show_explanation = true
  EXPECT_FALSE(j.contains("explanation"));
}

// Test empty warnings array is not included
TEST_F(ResponseFormatterTest, EmptyWarningsNotIncluded) {
  auto result = createBasicResult();
  result.warnings.clear();
  auto config = createConfig(true, false, true, false);  // show_warnings = true

  std::string output = ResponseFormatter::formatResponse(result, config);
  json j = json::parse(output);

  EXPECT_FALSE(j.contains("warnings"));
}

// Test empty visualization is not included
TEST_F(ResponseFormatterTest, EmptyVisualizationNotIncluded) {
  auto result = createBasicResult();
  result.suggested_visualization = "";
  auto config = createConfig(true, false, false,
                             true);  // show_suggested_visualization = true

  std::string output = ResponseFormatter::formatResponse(result, config);
  json j = json::parse(output);

  EXPECT_FALSE(j.contains("suggested_visualization"));
}

// Test JSON output is pretty-printed (contains newlines)
TEST_F(ResponseFormatterTest, JSONIsPrettyPrinted) {
  auto result = createBasicResult();
  auto config = createConfig(true, true, false, false);

  std::string output = ResponseFormatter::formatResponse(result, config);

  // Pretty-printed JSON should contain newlines
  EXPECT_THAT(output, testing::HasSubstr("\n"));
}
