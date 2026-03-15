extern "C" {
#include <postgres.h>

#include <access/htup_details.h>
#include <catalog/pg_type.h>
#include <fmgr.h>
#include <funcapi.h>
#include <miscadmin.h>
#include <utils/builtins.h>
#include <utils/elog.h>
#include <utils/memutils.h>
}

#include <nlohmann/json.hpp>

#include "include/config.hpp"
#include "include/pg_ai_query_utils.hpp"
#include "include/query_generator.hpp"
#include "include/response_formatter.hpp"

extern "C" {
PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(generate_query);
PG_FUNCTION_INFO_V1(get_database_tables);
PG_FUNCTION_INFO_V1(get_table_details);
PG_FUNCTION_INFO_V1(explain_query);

/**
 * generate_query(natural_language_query text, api_key text DEFAULT NULL,
 * provider text DEFAULT 'auto')
 *
 * Generates a SQL query from natural language input with automatic schema
 * discovery Provider options: 'openai', 'anthropic', 'auto' (auto-select based
 * on config)
 */
Datum generate_query(PG_FUNCTION_ARGS) {
  try {
    text* nl_query_arg = PG_GETARG_TEXT_PP(0);
    text* api_key_arg = PG_ARGISNULL(1) ? nullptr : PG_GETARG_TEXT_PP(1);
    text* provider_arg = PG_ARGISNULL(2) ? nullptr : PG_GETARG_TEXT_PP(2);

    std::string nl_query = pg_ai::utils::pg_text_to_string(nl_query_arg);
    std::string api_key = pg_ai::utils::pg_text_to_string(api_key_arg);
    std::string provider = pg_ai::utils::pg_text_to_string(provider_arg);
    if (provider.empty())
      provider = "auto";

    pg_ai::QueryRequest request{
        .natural_language = nl_query, .api_key = api_key, .provider = provider};

    auto result = pg_ai::QueryGenerator::generateQuery(request);

    if (!result.success) {
      ereport(ERROR, (errcode(ERRCODE_EXTERNAL_ROUTINE_EXCEPTION),
                      errmsg("Query generation failed: %s",
                             result.error_message.c_str())));
    }

    const auto& config = pg_ai::config::ConfigManager::getConfig();

    std::string formatted_response =
        pg_ai::ResponseFormatter::formatResponse(result, config);

    if (result.generated_query.empty()) {
      ereport(INFO, (errmsg("%s", result.explanation.c_str())));
      PG_RETURN_TEXT_P(cstring_to_text(""));
    }

    PG_RETURN_TEXT_P(cstring_to_text(formatted_response.c_str()));
  } catch (const std::exception& e) {
    ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
                    errmsg("Internal error: %s", e.what())));
    PG_RETURN_NULL();
  }
}

/**
 * get_database_tables()
 *
 * Returns JSON array of all tables in the database with their schema info
 */
Datum get_database_tables(PG_FUNCTION_ARGS) {
  try {
    auto result = pg_ai::QueryGenerator::getDatabaseTables();

    if (!result.success) {
      ereport(ERROR, (errcode(ERRCODE_EXTERNAL_ROUTINE_EXCEPTION),
                      errmsg("Failed to get database tables: %s",
                             result.error_message.c_str())));
    }

    nlohmann::json json_result = nlohmann::json::array();

    for (const auto& table : result.tables) {
      nlohmann::json table_json;
      table_json["table_name"] = table.table_name;
      table_json["schema_name"] = table.schema_name;
      table_json["table_type"] = table.table_type;
      table_json["estimated_rows"] = table.estimated_rows;
      json_result.push_back(table_json);
    }

    std::string json_string = json_result.dump(2);
    PG_RETURN_TEXT_P(cstring_to_text(json_string.c_str()));

  } catch (const std::exception& e) {
    ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
                    errmsg("Internal error: %s", e.what())));
    PG_RETURN_NULL();
  }
}

/**
 * get_table_details(table_name text, schema_name text DEFAULT 'public')
 *
 * Returns detailed JSON information about a specific table including columns,
 * constraints, and indexes
 */
Datum get_table_details(PG_FUNCTION_ARGS) {
  try {
    text* table_name_arg = PG_GETARG_TEXT_PP(0);
    text* schema_name_arg = PG_ARGISNULL(1) ? nullptr : PG_GETARG_TEXT_PP(1);

    std::string table_name = pg_ai::utils::pg_text_to_string(table_name_arg);
    std::string schema_name = pg_ai::utils::pg_text_to_string(schema_name_arg);
    if (schema_name.empty())
      schema_name = "public";

    auto result =
        pg_ai::QueryGenerator::getTableDetails(table_name, schema_name);

    if (!result.success) {
      ereport(ERROR, (errcode(ERRCODE_EXTERNAL_ROUTINE_EXCEPTION),
                      errmsg("Failed to get table details: %s",
                             result.error_message.c_str())));
    }

    nlohmann::json json_result;
    json_result["table_name"] = result.table_name;
    json_result["schema_name"] = result.schema_name;

    nlohmann::json columns = nlohmann::json::array();
    for (const auto& column : result.columns) {
      nlohmann::json column_json;
      column_json["column_name"] = column.column_name;
      column_json["data_type"] = column.data_type;
      column_json["is_nullable"] = column.is_nullable;
      column_json["column_default"] = column.column_default;
      column_json["is_primary_key"] = column.is_primary_key;
      column_json["is_foreign_key"] = column.is_foreign_key;
      if (!column.foreign_table.empty()) {
        column_json["foreign_table"] = column.foreign_table;
        column_json["foreign_column"] = column.foreign_column;
      }
      columns.push_back(column_json);
    }
    json_result["columns"] = columns;

    json_result["indexes"] = result.indexes;

    std::string json_string = json_result.dump(2);
    PG_RETURN_TEXT_P(cstring_to_text(json_string.c_str()));

  } catch (const std::exception& e) {
    ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
                    errmsg("Internal error: %s", e.what())));
    PG_RETURN_NULL();
  }
}

/**
 * explain_query(query_text text, api_key text DEFAULT NULL,
 * provider text DEFAULT 'auto')
 *
 * Runs EXPLAIN ANALYZE on a query and returns an AI-generated explanation
 * of the execution plan, performance insights, and optimization suggestions.
 */
Datum explain_query(PG_FUNCTION_ARGS) {
  try {
    text* query_text_arg = PG_GETARG_TEXT_PP(0);
    text* api_key_arg = PG_ARGISNULL(1) ? nullptr : PG_GETARG_TEXT_PP(1);
    text* provider_arg = PG_ARGISNULL(2) ? nullptr : PG_GETARG_TEXT_PP(2);

    std::string query_text = pg_ai::utils::pg_text_to_string(query_text_arg);
    std::string api_key = pg_ai::utils::pg_text_to_string(api_key_arg);
    std::string provider = pg_ai::utils::pg_text_to_string(provider_arg);
    if (provider.empty())
      provider = "auto";

    pg_ai::ExplainRequest request{
        .query_text = query_text, .api_key = api_key, .provider = provider};

    auto result = pg_ai::QueryGenerator::explainQuery(request);

    if (!result.success) {
      ereport(ERROR, (errcode(ERRCODE_EXTERNAL_ROUTINE_EXCEPTION),
                      errmsg("Query explanation failed: %s",
                             result.error_message.c_str())));
    }

    PG_RETURN_TEXT_P(cstring_to_text(result.ai_explanation.c_str()));
  } catch (const std::exception& e) {
    ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
                    errmsg("Internal error: %s", e.what())));
    PG_RETURN_NULL();
  }
}
}