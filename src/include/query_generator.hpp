#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace pg_ai {

/**
 * @brief Request structure for natural language query generation
 * 
 * Contains the input parameters needed to generate a SQL query from
 * natural language using AI providers.
 */
struct QueryRequest {
  std::string natural_language;
  std::string api_key;
  std::string provider;
};

/**
 * @brief Result of SQL query generation from natural language
 * 
 * Contains the generated SQL query along with metadata including
 * explanations, warnings, and success status.
 */
struct QueryResult {
  std::string generated_query;
  std::string explanation;
  std::vector<std::string> warnings;
  bool row_limit_applied;
  std::string suggested_visualization;
  bool success;
  std::string error_message;
};

/**
 * @brief Information about a database table
 * 
 * Contains basic metadata about a table including name, schema,
 * type, and estimated row count.
 */
struct TableInfo {
  std::string table_name;
  std::string schema_name;
  std::string table_type;
  int64_t estimated_rows;
};

/**
 * @brief Information about a table column
 * 
 * Contains complete metadata about a database column including
 * data type, constraints, and foreign key relationships.
 */
struct ColumnInfo {
  std::string column_name;
  std::string data_type;
  bool is_nullable;
  std::string column_default;
  bool is_primary_key;
  bool is_foreign_key;
  std::string foreign_table;
  std::string foreign_column;
};

/**
 * @brief Detailed information about a specific table
 * 
 * Contains comprehensive schema information for a table including
 * all columns, indexes, and retrieval status.
 */
struct TableDetails {
  std::string table_name;
  std::string schema_name;
  std::vector<ColumnInfo> columns;
  std::vector<std::string> indexes;
  bool success;
  std::string error_message;
};

/**
 * @brief Complete database schema information
 * 
 * Contains information about all accessible tables in the database.
 */
struct DatabaseSchema {
  std::vector<TableInfo> tables;
  bool success;
  std::string error_message;
};

/**
 * @brief Request structure for query performance analysis
 * 
 * Contains the SQL query to analyze and optional API configuration.
 */
struct ExplainRequest {
  std::string query_text;
  std::string api_key;
  std::string provider;
};

/**
 * @brief Result of query performance analysis
 * 
 * Contains both raw PostgreSQL EXPLAIN output and AI-generated
 * performance analysis with optimization suggestions.
 */
struct ExplainResult {
  std::string query;
  std::string explain_output;
  std::string ai_explanation;
  bool success;
  std::string error_message;
};

/**
 * @brief Main class for SQL query generation and database schema operations
 * 
 * QueryGenerator provides the core functionality for converting natural language
 * to SQL queries using AI providers (OpenAI, Anthropic, Gemini). It also handles
 * database schema retrieval and query performance analysis.
 * 
 * All methods are static and thread-safe.
 */
class QueryGenerator {
 public:
  /**
   * @brief Generate SQL query from natural language description
   * 
   * Converts a natural language query description into executable SQL by sending
   * the request to an AI provider along with database schema context.
   * 
   * @param request The query request containing natural language input and options
   * @return QueryResult containing the generated SQL query and metadata
   * @throws std::runtime_error if database connection fails or AI API call fails
   * 
   * @example
   * QueryRequest req;
   * req.natural_language = "show all users with age greater than 25";
   * req.provider = "openai";
   * auto result = QueryGenerator::generateQuery(req);
   * if (result.success) {
   *   std::cout << result.generated_query << std::endl;
   * }
   */
  static QueryResult generateQuery(const QueryRequest& request);

  /**
   * @brief Retrieve list of all accessible tables in the database
   * 
   * Queries PostgreSQL's information_schema to get metadata about all
   * tables accessible to the current user.
   * 
   * @return DatabaseSchema containing all visible tables and their metadata
   * @throws std::runtime_error if database query fails
   * 
   * @example
   * auto schema = QueryGenerator::getDatabaseTables();
   * if (schema.success) {
   *   for (const auto& table : schema.tables) {
   *     std::cout << table.schema_name << "." << table.table_name << std::endl;
   *   }
   * }
   */
  static DatabaseSchema getDatabaseTables();

  /**
   * @brief Get detailed information about a specific table
   * 
   * Retrieves complete schema information for a table including all columns,
   * data types, constraints, and indexes.
   * 
   * @param table_name Name of the table to inspect
   * @param schema_name Schema containing the table (defaults to "public")
   * @return TableDetails containing complete table schema information
   * @throws std::runtime_error if database query fails
   * 
   * @example
   * auto details = QueryGenerator::getTableDetails("users", "public");
   * if (details.success) {
   *   for (const auto& col : details.columns) {
   *     std::cout << col.column_name << ": " << col.data_type << std::endl;
   *   }
   * }
   */
  static TableDetails getTableDetails(
      const std::string& table_name,
      const std::string& schema_name = "public");

  /**
   * @brief Analyze query performance and get optimization suggestions
   * 
   * Executes EXPLAIN (ANALYZE, VERBOSE, COSTS, SETTINGS, BUFFERS, FORMAT JSON) 
   * on the provided SQL query and sends the output to an AI provider for analysis 
   * and optimization recommendations.
   * 
   * @param request The explain request containing SQL query to analyze
   * @return ExplainResult with EXPLAIN output and AI-generated insights
   * @throws std::runtime_error if query execution fails or AI API call fails
   * 
   * @example
   * ExplainRequest req;
   * req.query_text = "SELECT * FROM users WHERE age > 25";
   * req.provider = "anthropic";
   * auto result = QueryGenerator::explainQuery(req);
   * if (result.success) {
   *   std::cout << "AI Analysis: " << result.ai_explanation << std::endl;
   * }
   */
  static ExplainResult explainQuery(const ExplainRequest& request);

  /**
   * @brief Format database schema as text for AI consumption
   * 
   * Converts a DatabaseSchema structure into a formatted string suitable
   * for including in prompts sent to AI models.
   * 
   * @param schema The database schema to format
   * @return Formatted string representation of the schema
   */
  static std::string formatSchemaForAI(const DatabaseSchema& schema);

  /**
   * @brief Format table details as text for AI consumption
   * 
   * Converts a TableDetails structure into a formatted string suitable
   * for including in prompts sent to AI models.
   * 
   * @param details The table details to format
   * @return Formatted string representation of the table details
   */
  static std::string formatTableDetailsForAI(const TableDetails& details);

 private:
  /**
   * @brief Build AI prompt with schema context and query request
   * 
   * @param request Query request containing natural language description
   * @return Complete prompt string ready for AI API
   */
  static std::string buildPrompt(const QueryRequest& request);

  /**
   * @brief Log model configuration settings
   * 
   * @param model_name Name of the AI model being used
   * @param max_tokens Maximum tokens for response
   * @param temperature Temperature setting for response randomness
   */
  static void logModelSettings(const std::string& model_name,
                               std::optional<int> max_tokens,
                               std::optional<double> temperature);
};

}  // namespace pg_ai