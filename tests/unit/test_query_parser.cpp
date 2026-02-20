#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../test_helpers.hpp"
#include "include/query_generator.hpp"
#include "include/query_parser.hpp"

using namespace pg_ai;
using namespace pg_ai::test_utils;
using json = nlohmann::json;

class QueryParserTest : public ::testing::Test {
 protected:
  std::string readResponseFixture(const std::string& filename) {
    return readTestFile(getResponseFixture(filename));
  }
};

// ============================================================================
// extractSQLFromResponse tests
// ============================================================================

// Test parsing valid JSON directly
TEST_F(QueryParserTest, ExtractSQL_DirectJSON) {
  std::string response = R"({
        "sql": "SELECT * FROM users",
        "explanation": "Retrieves all users"
    })";

  json result = QueryParser::extractSQLFromResponse(response);

  EXPECT_EQ(result["sql"], "SELECT * FROM users");
  EXPECT_EQ(result["explanation"], "Retrieves all users");
}

// Test parsing JSON in markdown code block
TEST_F(QueryParserTest, ExtractSQL_MarkdownCodeBlock) {
  std::string response = R"(Here is the query:

```json
{
    "sql": "SELECT id FROM orders",
    "explanation": "Gets order IDs"
}
```

Let me know if you need changes.)";

  json result = QueryParser::extractSQLFromResponse(response);

  EXPECT_EQ(result["sql"], "SELECT id FROM orders");
  EXPECT_EQ(result["explanation"], "Gets order IDs");
}

// Test parsing JSON in markdown code block without language specifier
TEST_F(QueryParserTest, ExtractSQL_MarkdownCodeBlockNoLang) {
  std::string response = R"(```
{
    "sql": "SELECT name FROM products",
    "explanation": "Gets product names"
}
```)";

  json result = QueryParser::extractSQLFromResponse(response);

  EXPECT_EQ(result["sql"], "SELECT name FROM products");
}

// Test fallback to raw SQL when no JSON detected
TEST_F(QueryParserTest, ExtractSQL_RawSQLFallback) {
  std::string response = "SELECT * FROM customers WHERE active = true";

  json result = QueryParser::extractSQLFromResponse(response);

  EXPECT_EQ(result["sql"], response);
  EXPECT_EQ(result["explanation"], "Raw LLM output (no JSON detected)");
}

// Test parsing JSON with warnings array
TEST_F(QueryParserTest, ExtractSQL_WithWarnings) {
  std::string response = R"({
        "sql": "SELECT * FROM big_table",
        "explanation": "Full table scan",
        "warnings": ["May be slow", "Consider adding LIMIT"]
    })";

  json result = QueryParser::extractSQLFromResponse(response);

  EXPECT_TRUE(result["warnings"].is_array());
  EXPECT_EQ(result["warnings"].size(), 2);
  EXPECT_EQ(result["warnings"][0], "May be slow");
}

// Test parsing with extra fields
TEST_F(QueryParserTest, ExtractSQL_ExtraFields) {
  std::string response = R"({
        "sql": "SELECT * FROM users",
        "explanation": "Query",
        "row_limit_applied": true,
        "suggested_visualization": "table"
    })";

  json result = QueryParser::extractSQLFromResponse(response);

  EXPECT_TRUE(result["row_limit_applied"]);
  EXPECT_EQ(result["suggested_visualization"], "table");
}

// Test parsing malformed JSON falls back
TEST_F(QueryParserTest, ExtractSQL_MalformedJSON) {
  std::string response = R"({sql: "broken")";

  json result = QueryParser::extractSQLFromResponse(response);

  // Should fall back to treating as raw SQL
  EXPECT_EQ(result["sql"], response);
}

// ============================================================================
// accessesSystemTables tests
// ============================================================================

TEST_F(QueryParserTest, SystemTables_InformationSchema) {
  EXPECT_TRUE(QueryParser::accessesSystemTables(
      "SELECT * FROM information_schema.tables"));
  EXPECT_TRUE(QueryParser::accessesSystemTables(
      "SELECT * FROM INFORMATION_SCHEMA.COLUMNS"));
  EXPECT_TRUE(QueryParser::accessesSystemTables(
      "select column_name from information_schema.columns"));
}

TEST_F(QueryParserTest, SystemTables_PgCatalog) {
  EXPECT_TRUE(
      QueryParser::accessesSystemTables("SELECT * FROM pg_catalog.pg_tables"));
  EXPECT_TRUE(
      QueryParser::accessesSystemTables("SELECT * FROM PG_CATALOG.pg_class"));
}

TEST_F(QueryParserTest, SystemTables_UserTables) {
  EXPECT_FALSE(QueryParser::accessesSystemTables("SELECT * FROM users"));
  EXPECT_FALSE(
      QueryParser::accessesSystemTables("SELECT * FROM public.orders"));
  EXPECT_FALSE(QueryParser::accessesSystemTables(
      "SELECT id, name FROM products WHERE active = true"));
}

// ============================================================================
// hasErrorIndicators tests
// ============================================================================

TEST_F(QueryParserTest, ErrorIndicators_CannotGenerate) {
  EXPECT_TRUE(QueryParser::hasErrorIndicators(
      "Cannot generate query for this request", {}));
  EXPECT_TRUE(QueryParser::hasErrorIndicators("CANNOT CREATE QUERY", {}));
  EXPECT_TRUE(
      QueryParser::hasErrorIndicators("Unable to generate the SQL query", {}));
}

TEST_F(QueryParserTest, ErrorIndicators_TableNotFound) {
  EXPECT_TRUE(QueryParser::hasErrorIndicators(
      "Table 'foo' does not exist in the database", {}));
  EXPECT_TRUE(
      QueryParser::hasErrorIndicators("The requested tables do not exist", {}));
  EXPECT_TRUE(QueryParser::hasErrorIndicators("Table not found: orders", {}));
  EXPECT_TRUE(QueryParser::hasErrorIndicators("No such table as 'users'", {}));
}

TEST_F(QueryParserTest, ErrorIndicators_ColumnNotFound) {
  EXPECT_TRUE(QueryParser::hasErrorIndicators("Column not found: email", {}));
  EXPECT_TRUE(
      QueryParser::hasErrorIndicators("No such column in the table", {}));
}

TEST_F(QueryParserTest, ErrorIndicators_InWarnings) {
  EXPECT_TRUE(QueryParser::hasErrorIndicators("Query generated",
                                              {"Error: Table does not exist"}));
  EXPECT_TRUE(QueryParser::hasErrorIndicators("Success",
                                              {"Column 'foo' does not exist"}));
}

TEST_F(QueryParserTest, ErrorIndicators_NoErrors) {
  EXPECT_FALSE(
      QueryParser::hasErrorIndicators("Query retrieves all active users", {}));
  EXPECT_FALSE(QueryParser::hasErrorIndicators(
      "This query selects data from the users table",
      {"Consider adding an index"}));
}

// ============================================================================
// parseQueryResponse tests
// ============================================================================

TEST_F(QueryParserTest, ParseResponse_ValidQuery) {
  std::string response = R"({
        "sql": "SELECT * FROM users WHERE id = 1",
        "explanation": "Retrieves user with ID 1",
        "warnings": [],
        "suggested_visualization": "table"
    })";

  QueryResult result = QueryParser::parseQueryResponse(response);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.generated_query, "SELECT * FROM users WHERE id = 1");
  EXPECT_EQ(result.explanation, "Retrieves user with ID 1");
  EXPECT_TRUE(result.warnings.empty());
  EXPECT_TRUE(result.error_message.empty());
}

TEST_F(QueryParserTest, ParseResponse_WithWarnings) {
  std::string response = R"({
        "sql": "SELECT * FROM large_table",
        "explanation": "Full table scan",
        "warnings": ["May be slow", "Add LIMIT"]
    })";

  QueryResult result = QueryParser::parseQueryResponse(response);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.warnings.size(), 2);
  EXPECT_EQ(result.warnings[0], "May be slow");
  EXPECT_EQ(result.warnings[1], "Add LIMIT");
}

TEST_F(QueryParserTest, ParseResponse_SingleWarningAsString) {
  std::string response = R"({
        "sql": "SELECT * FROM users",
        "explanation": "Query",
        "warnings": "Single warning message"
    })";

  QueryResult result = QueryParser::parseQueryResponse(response);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.warnings.size(), 1);
  EXPECT_EQ(result.warnings[0], "Single warning message");
}

TEST_F(QueryParserTest, ParseResponse_RowLimitApplied) {
  std::string response = R"({
        "sql": "SELECT * FROM users LIMIT 1000",
        "explanation": "Query with limit",
        "row_limit_applied": true
    })";

  QueryResult result = QueryParser::parseQueryResponse(response);

  EXPECT_TRUE(result.success);
  EXPECT_TRUE(result.row_limit_applied);
}

TEST_F(QueryParserTest, ParseResponse_ErrorInExplanation) {
  std::string response = R"({
        "sql": "",
        "explanation": "Cannot generate query: Table 'foo' does not exist",
        "warnings": []
    })";

  QueryResult result = QueryParser::parseQueryResponse(response);

  EXPECT_FALSE(result.success);
  EXPECT_TRUE(result.generated_query.empty());
  EXPECT_THAT(result.error_message, testing::HasSubstr("does not exist"));
}

TEST_F(QueryParserTest, ParseResponse_SystemTableAccess) {
  std::string response = R"({
        "sql": "SELECT * FROM information_schema.tables",
        "explanation": "Lists all tables"
    })";

  QueryResult result = QueryParser::parseQueryResponse(response);

  EXPECT_FALSE(result.success);
  EXPECT_TRUE(result.generated_query.empty());
  EXPECT_THAT(result.error_message, testing::HasSubstr("system tables"));
}

TEST_F(QueryParserTest, ParseResponse_EmptySQLNotError) {
  std::string response = R"({
        "sql": "",
        "explanation": "No query needed for this request"
    })";

  QueryResult result = QueryParser::parseQueryResponse(response);

  // Empty SQL with no error indicators is success
  EXPECT_TRUE(result.success);
  EXPECT_TRUE(result.generated_query.empty());
}

TEST_F(QueryParserTest, ParseResponse_RawSQLFallback) {
  std::string response = "SELECT id, name FROM customers";

  QueryResult result = QueryParser::parseQueryResponse(response);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.generated_query, response);
}

TEST_F(QueryParserTest, ParseResponse_DefaultVisualization) {
  std::string response = R"({
        "sql": "SELECT * FROM users"
    })";

  QueryResult result = QueryParser::parseQueryResponse(response);

  EXPECT_TRUE(result.success);
  // Default visualization should be "table"
  EXPECT_EQ(result.suggested_visualization, "table");
}

// Test with fixture files
TEST_F(QueryParserTest, ParseResponse_ValidQueryFixture) {
  std::string response = readResponseFixture("valid_query_response.json");
  ASSERT_FALSE(response.empty()) << "Could not read fixture file";

  QueryResult result = QueryParser::parseQueryResponse(response);

  EXPECT_TRUE(result.success);
  EXPECT_THAT(result.generated_query, testing::HasSubstr("SELECT"));
  EXPECT_THAT(result.generated_query, testing::HasSubstr("users"));
  EXPECT_FALSE(result.explanation.empty());
}

TEST_F(QueryParserTest, ParseResponse_MarkdownFixture) {
  std::string response = readResponseFixture("valid_query_markdown.txt");
  ASSERT_FALSE(response.empty()) << "Could not read fixture file";

  QueryResult result = QueryParser::parseQueryResponse(response);

  EXPECT_TRUE(result.success);
  EXPECT_THAT(result.generated_query, testing::HasSubstr("COUNT"));
  EXPECT_THAT(result.generated_query, testing::HasSubstr("orders"));
}

TEST_F(QueryParserTest, ParseResponse_ErrorFixture) {
  std::string response = readResponseFixture("error_table_not_found.json");
  ASSERT_FALSE(response.empty()) << "Could not read fixture file";

  QueryResult result = QueryParser::parseQueryResponse(response);

  EXPECT_FALSE(result.success);
  EXPECT_TRUE(result.generated_query.empty());
}

TEST_F(QueryParserTest, ParseResponse_SystemTableFixture) {
  std::string response = readResponseFixture("system_table_access.json");
  ASSERT_FALSE(response.empty()) << "Could not read fixture file";

  QueryResult result = QueryParser::parseQueryResponse(response);

  EXPECT_FALSE(result.success);
  EXPECT_THAT(result.error_message, testing::HasSubstr("system tables"));
}

TEST_F(QueryParserTest, ParseResponse_SystemTableAllowedWhenFlagTrue) {
  std::string response = R"({
        "sql": "SELECT * FROM information_schema.tables",
        "explanation": "Lists all tables"
    })";

  QueryResult result =
      QueryParser::parseQueryResponse(response, true /* allow_system_table_access */);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.generated_query, "SELECT * FROM information_schema.tables");
  EXPECT_EQ(result.explanation, "Lists all tables");
}

TEST_F(QueryParserTest, ParseResponse_MultipleWarningsFixture) {
  std::string response = readResponseFixture("multiple_warnings.json");
  ASSERT_FALSE(response.empty()) << "Could not read fixture file";

  QueryResult result = QueryParser::parseQueryResponse(response);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.warnings.size(), 3);
}
