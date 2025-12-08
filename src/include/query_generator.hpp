#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace pg_ai {

struct QueryRequest {
  std::string natural_language;
  std::string api_key;
  std::string provider;
};

struct QueryResult {
  std::string generated_query;
  std::string explanation;
  std::vector<std::string> warnings;
  bool row_limit_applied;
  std::string suggested_visualization;
  bool success;
  std::string error_message;
};

struct TableInfo {
  std::string table_name;
  std::string schema_name;
  std::string table_type;
  int64_t estimated_rows;
};

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

struct TableDetails {
  std::string table_name;
  std::string schema_name;
  std::vector<ColumnInfo> columns;
  std::vector<std::string> indexes;
  bool success;
  std::string error_message;
};

struct DatabaseSchema {
  std::vector<TableInfo> tables;
  bool success;
  std::string error_message;
};

struct ExplainRequest {
  std::string query_text;
  std::string api_key;
  std::string provider;
};

struct ExplainResult {
  std::string query;
  std::string explain_output;
  std::string ai_explanation;
  bool success;
  std::string error_message;
};

class QueryGenerator {
 public:
  static QueryResult generateQuery(const QueryRequest& request);
  static DatabaseSchema getDatabaseTables();
  static TableDetails getTableDetails(
      const std::string& table_name,
      const std::string& schema_name = "public");
  static ExplainResult explainQuery(const ExplainRequest& request);

  static std::string formatSchemaForAI(const DatabaseSchema& schema);
  static std::string formatTableDetailsForAI(const TableDetails& details);

 private:
  static std::string buildPrompt(const QueryRequest& request);
  static void logModelSettings(const std::string& model_name,
                               std::optional<int> max_tokens,
                               std::optional<double> temperature);
};

}  // namespace pg_ai