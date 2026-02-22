#include "../include/query_generator.hpp"

extern "C" {
#include <postgres.h>

#include <utils/builtins.h>

#include <executor/spi.h>
}

#include <optional>
#include <sstream>
#include <vector>

#include "../include/gemini_client.h"
#include <ai/anthropic.h>
#include <ai/openai.h>
#include <nlohmann/json.hpp>

#include "../include/ai_client_factory.hpp"
#include "../include/config.hpp"
#include "../include/logger.hpp"
#include "../include/prompts.hpp"
#include "../include/provider_selector.hpp"
#include "../include/query_parser.hpp"
#include "../include/spi_connection.hpp"
#include "../include/utils.hpp"

using namespace pg_ai::logger;

namespace pg_ai {

QueryResult QueryGenerator::generateQuery(const QueryRequest& request) {
  try {
    const auto& cfg = config::ConfigManager::getConfig();

    auto validation_error = utils::validate_natural_language_query(
        request.natural_language, cfg.max_query_length);
    if (validation_error) {
      return QueryResult{.generated_query = "",
                         .explanation = "",
                         .warnings = {},
                         .row_limit_applied = false,
                         .suggested_visualization = "",
                         .success = false,
                         .error_message = *validation_error};
    }

    // Use ProviderSelector to determine the provider
    auto selection =
        ProviderSelector::selectProvider(request.api_key, request.provider);

    if (!selection.success) {
      return QueryResult{.generated_query = "",
                         .explanation = "",
                         .warnings = {},
                         .row_limit_applied = false,
                         .suggested_visualization = "",
                         .success = false,
                         .error_message = selection.error_message};
    }

    // Handle Gemini separately as it uses a different client
    if (selection.provider == config::Provider::GEMINI) {
      std::string model_name =
          (selection.config && !selection.config->default_model.empty())
              ? selection.config->default_model
              : "gemini-2.5-flash";
      logger::Logger::info("Using Gemini model: " + model_name);

      std::string system_prompt = prompts::SYSTEM_PROMPT;
      std::string prompt = buildPrompt(request);

      gemini::GeminiClient gemini_client(selection.api_key);
      gemini::GeminiRequest gemini_request{
          .model = model_name,
          .system_prompt = system_prompt,
          .user_prompt = prompt,
          .temperature =
              selection.config
                  ? std::optional<double>(selection.config->default_temperature)
                  : std::nullopt,
          .max_tokens =
              selection.config
                  ? std::optional<int>(selection.config->default_max_tokens)
                  : std::nullopt};

      auto gemini_result = gemini_client.generate_text(gemini_request);

      if (!gemini_result.success) {
        return QueryResult{.success = false,
                           .error_message = "Gemini API error: " +
                                            gemini_result.error_message};
      }

      return QueryParser::parseQueryResponse(gemini_result.text);
    }

    // Use AIClientFactory for OpenAI and Anthropic
    auto client_result = AIClientFactory::createClient(
        selection.provider, selection.api_key, selection.config);

    if (!client_result.success) {
      return QueryResult{.generated_query = "",
                         .explanation = "",
                         .warnings = {},
                         .row_limit_applied = false,
                         .suggested_visualization = "",
                         .success = false,
                         .error_message = client_result.error_message};
    }

    std::string prompt = buildPrompt(request);
    ai::GenerateOptions options(client_result.model_name,
                                prompts::SYSTEM_PROMPT, prompt);

    if (selection.config) {
      options.max_tokens = selection.config->default_max_tokens;
      options.temperature = selection.config->default_temperature;
      logModelSettings(client_result.model_name, options.max_tokens,
                       options.temperature);
    } else {
      logger::Logger::info("Using model: " + client_result.model_name +
                           " with default settings");
    }

    auto result = client_result.client.generate_text(options);

    if (!result) {
      return QueryResult{
          .generated_query = "",
          .explanation = "",
          .warnings = {},
          .row_limit_applied = false,
          .suggested_visualization = "",
          .success = false,
          .error_message =
              "AI API error: " + utils::formatAPIError(result.error_message())};
    }

    if (result.text.empty()) {
      return QueryResult{.generated_query = "",
                         .explanation = "",
                         .warnings = {},
                         .row_limit_applied = false,
                         .suggested_visualization = "",
                         .success = false,
                         .error_message = "Empty response from AI service"};
    }

    return QueryParser::parseQueryResponse(result.text);
  } catch (const std::exception& e) {
    return QueryResult{.generated_query = "",
                       .explanation = "",
                       .warnings = {},
                       .row_limit_applied = false,
                       .suggested_visualization = "",
                       .success = false,
                       .error_message = std::string("Exception: ") + e.what()};
  }
}

std::string QueryGenerator::buildPrompt(const QueryRequest& request) {
  std::ostringstream prompt;
  const auto& cfg = config::ConfigManager::getConfig();

  prompt << "Generate a PostgreSQL query for this request:\n\n";
  prompt << "Request: " << request.natural_language << "\n";

  std::string schema_context;
  try {
    auto schema = getDatabaseTables();
    if (schema.success) {
      schema_context = formatSchemaForAI(schema);

      std::vector<std::string> mentioned_tables;
      for (const auto& table : schema.tables) {
        if (request.natural_language.find(table.table_name) !=
            std::string::npos) {
          mentioned_tables.push_back(table.table_name);
        }
      }

      for (size_t i = 0; i < mentioned_tables.size() && i < 3; ++i) {
        auto table_details = getTableDetails(mentioned_tables[i]);
        if (table_details.success) {
          schema_context += "\n" + formatTableDetailsForAI(table_details);
        }
      }
    }
  } catch (const std::exception& e) {
    logger::Logger::warning("Error building schema context for prompt: " +
                            std::string(e.what()));
  }

  if (!schema_context.empty()) {
    prompt << "Schema info:\n" << schema_context << "\n";
  }

  return prompt.str();
}

// Parsing logic has been moved to QueryParser class for testability

void QueryGenerator::logModelSettings(const std::string& model_name,
                                      std::optional<int> max_tokens,
                                      std::optional<double> temperature) {
  std::string log_msg = "Using model: " + model_name;
  if (max_tokens.has_value()) {
    log_msg += " with max_tokens=" + std::to_string(max_tokens.value());
  }
  if (temperature.has_value()) {
    log_msg += ", temperature=" + std::to_string(temperature.value());
  }
  logger::Logger::info(log_msg);
}

DatabaseSchema QueryGenerator::getDatabaseTables() {
  DatabaseSchema result;
  result.success = false;

  try {
    if (SPI_connect() != SPI_OK_CONNECT) {
      result.error_message = "Failed to connect to SPI";
      return result;
    }

    const char* query = R"(
            SELECT
                t.table_name,
                t.table_schema,
                t.table_type,
                COALESCE(pg_stat.n_tup_ins + pg_stat.n_tup_upd + pg_stat.n_tup_del, 0) as estimated_rows
            FROM information_schema.tables t
            LEFT JOIN pg_stat_user_tables pg_stat ON t.table_name = pg_stat.relname
                AND t.table_schema = pg_stat.schemaname
            WHERE t.table_schema NOT IN ('information_schema', 'pg_catalog')
                AND t.table_type = 'BASE TABLE'
            ORDER BY t.table_schema, t.table_name
        )";

    int ret = SPI_execute(query, true, 0);

    if (ret != SPI_OK_SELECT) {
      result.error_message = "Failed to execute query";
      SPI_finish();
      return result;
    }

    SPITupleTable* tuptable = SPI_tuptable;
    TupleDesc tupdesc = tuptable->tupdesc;

    for (uint64 i = 0; i < SPI_processed; i++) {
      HeapTuple tuple = tuptable->vals[i];
      TableInfo table_info;

      char* table_name = SPI_getvalue(tuple, tupdesc, 1);
      char* schema_name = SPI_getvalue(tuple, tupdesc, 2);
      char* table_type = SPI_getvalue(tuple, tupdesc, 3);
      char* estimated_rows_str = SPI_getvalue(tuple, tupdesc, 4);

      if (table_name)
        table_info.table_name = std::string(table_name);
      if (schema_name)
        table_info.schema_name = std::string(schema_name);
      if (table_type)
        table_info.table_type = std::string(table_type);
      if (estimated_rows_str) {
        table_info.estimated_rows = atoll(estimated_rows_str);
      } else {
        table_info.estimated_rows = 0;
      }

      result.tables.push_back(table_info);

      if (table_name)
        pfree(table_name);
      if (schema_name)
        pfree(schema_name);
      if (table_type)
        pfree(table_type);
      if (estimated_rows_str)
        pfree(estimated_rows_str);
    }

    result.success = true;
    SPI_finish();

  } catch (const std::exception& e) {
    result.error_message = std::string("Exception: ") + e.what();
    SPI_finish();
  }

  return result;
}

TableDetails QueryGenerator::getTableDetails(const std::string& table_name,
                                             const std::string& schema_name) {
  TableDetails result;
  result.success = false;
  result.table_name = table_name;
  result.schema_name = schema_name;

  try {
    if (SPI_connect() != SPI_OK_CONNECT) {
      result.error_message = "Failed to connect to SPI";
      return result;
    }

    std::string column_query = R"(
            SELECT
                c.column_name,
                c.data_type,
                c.is_nullable,
                c.column_default,
                CASE WHEN pk.column_name IS NOT NULL THEN true ELSE false END as is_primary_key,
                CASE WHEN fk.column_name IS NOT NULL THEN true ELSE false END as is_foreign_key,
                fk.foreign_table_name,
                fk.foreign_column_name
            FROM information_schema.columns c
            LEFT JOIN (
                SELECT kcu.column_name, kcu.table_name, kcu.table_schema
                FROM information_schema.table_constraints tc
                JOIN information_schema.key_column_usage kcu
                    ON tc.constraint_name = kcu.constraint_name
                    AND tc.table_schema = kcu.table_schema
                WHERE tc.constraint_type = 'PRIMARY KEY'
            ) pk ON c.column_name = pk.column_name
                AND c.table_name = pk.table_name
                AND c.table_schema = pk.table_schema
            LEFT JOIN (
                SELECT
                    kcu.column_name,
                    kcu.table_name,
                    kcu.table_schema,
                    ccu.table_name AS foreign_table_name,
                    ccu.column_name AS foreign_column_name
                FROM information_schema.table_constraints tc
                JOIN information_schema.key_column_usage kcu
                    ON tc.constraint_name = kcu.constraint_name
                    AND tc.table_schema = kcu.table_schema
                JOIN information_schema.constraint_column_usage ccu
                    ON ccu.constraint_name = tc.constraint_name
                    AND ccu.table_schema = tc.table_schema
                WHERE tc.constraint_type = 'FOREIGN KEY'
            ) fk ON c.column_name = fk.column_name
                AND c.table_name = fk.table_name
                AND c.table_schema = fk.table_schema
            WHERE c.table_name = ')" +
                               table_name + R"('
                AND c.table_schema = ')" +
                               schema_name + R"('
            ORDER BY c.ordinal_position
        )";

    int ret = SPI_execute(column_query.c_str(), true, 0);

    if (ret != SPI_OK_SELECT) {
      result.error_message = "Failed to execute column query";
      SPI_finish();
      return result;
    }

    SPITupleTable* tuptable = SPI_tuptable;
    TupleDesc tupdesc = tuptable->tupdesc;

    for (uint64 i = 0; i < SPI_processed; i++) {
      HeapTuple tuple = tuptable->vals[i];
      ColumnInfo column_info;

      char* column_name = SPI_getvalue(tuple, tupdesc, 1);
      char* data_type = SPI_getvalue(tuple, tupdesc, 2);
      char* is_nullable = SPI_getvalue(tuple, tupdesc, 3);
      char* column_default = SPI_getvalue(tuple, tupdesc, 4);
      char* is_primary_key = SPI_getvalue(tuple, tupdesc, 5);
      char* is_foreign_key = SPI_getvalue(tuple, tupdesc, 6);
      char* foreign_table = SPI_getvalue(tuple, tupdesc, 7);
      char* foreign_column = SPI_getvalue(tuple, tupdesc, 8);

      if (column_name)
        column_info.column_name = std::string(column_name);
      if (data_type)
        column_info.data_type = std::string(data_type);
      if (is_nullable)
        column_info.is_nullable = (std::string(is_nullable) == "YES");
      if (column_default)
        column_info.column_default = std::string(column_default);
      if (is_primary_key)
        column_info.is_primary_key = (std::string(is_primary_key) == "t");
      if (is_foreign_key)
        column_info.is_foreign_key = (std::string(is_foreign_key) == "t");
      if (foreign_table)
        column_info.foreign_table = std::string(foreign_table);
      if (foreign_column)
        column_info.foreign_column = std::string(foreign_column);

      result.columns.push_back(column_info);

      if (column_name)
        pfree(column_name);
      if (data_type)
        pfree(data_type);
      if (is_nullable)
        pfree(is_nullable);
      if (column_default)
        pfree(column_default);
      if (is_primary_key)
        pfree(is_primary_key);
      if (is_foreign_key)
        pfree(is_foreign_key);
      if (foreign_table)
        pfree(foreign_table);
      if (foreign_column)
        pfree(foreign_column);
    }

    std::string index_query = R"(
            SELECT indexname, indexdef
            FROM pg_indexes
            WHERE tablename = ')" +
                              table_name + R"('
                AND schemaname = ')" +
                              schema_name + R"('
            ORDER BY indexname
        )";

    ret = SPI_execute(index_query.c_str(), true, 0);

    if (ret == SPI_OK_SELECT) {
      tuptable = SPI_tuptable;
      tupdesc = tuptable->tupdesc;

      for (uint64 i = 0; i < SPI_processed; i++) {
        HeapTuple tuple = tuptable->vals[i];
        char* indexname = SPI_getvalue(tuple, tupdesc, 1);
        char* indexdef = SPI_getvalue(tuple, tupdesc, 2);

        if (indexdef) {
          result.indexes.push_back(std::string(indexdef));
        }

        if (indexname)
          pfree(indexname);
        if (indexdef)
          pfree(indexdef);
      }
    }

    result.success = true;
    SPI_finish();

  } catch (const std::exception& e) {
    result.error_message = std::string("Exception: ") + e.what();
    SPI_finish();
  }

  return result;
}

std::string QueryGenerator::formatSchemaForAI(const DatabaseSchema& schema) {
  std::ostringstream result;
  result << "=== DATABASE SCHEMA ===\n";
  result
      << "IMPORTANT: These are the ONLY tables available in this database:\n\n";

  for (const auto& table : schema.tables) {
    result << "- " << table.schema_name << "." << table.table_name << " ("
           << table.table_type << ", ~" << table.estimated_rows << " rows)\n";
  }

  if (schema.tables.empty()) {
    result << "- No user tables found in database\n";
  }

  result << "\nCRITICAL: If user asks for tables not listed above, return an "
            "error with available table names.\n";
  result << "Do NOT query information_schema or pg_catalog tables.\n";
  return result.str();
}

std::string QueryGenerator::formatTableDetailsForAI(
    const TableDetails& details) {
  std::ostringstream result;
  result << "=== TABLE: " << details.schema_name << "." << details.table_name
         << " ===\n\n";

  result << "COLUMNS:\n";
  for (const auto& col : details.columns) {
    result << "- " << col.column_name << " (" << col.data_type << ")";

    if (col.is_primary_key)
      result << " [PRIMARY KEY]";
    if (col.is_foreign_key) {
      result << " [FK -> " << col.foreign_table << "." << col.foreign_column
             << "]";
    }
    if (!col.is_nullable)
      result << " [NOT NULL]";
    if (!col.column_default.empty()) {
      result << " [DEFAULT: " << col.column_default << "]";
    }
    result << "\n";
  }

  if (!details.indexes.empty()) {
    result << "\nINDEXES:\n";
    for (const auto& idx : details.indexes) {
      result << "- " << idx << "\n";
    }
  }

  return result.str();
}

ExplainResult QueryGenerator::explainQuery(const ExplainRequest& request) {
  ExplainResult result{.success = false};

  try {
    if (request.query_text.empty()) {
      result.error_message = "Query text cannot be empty";
      return result;
    }

    if (!utils::is_select_only_query(request.query_text)) {
      result.error_message =
          "Only SELECT queries can be explained. Mutating or DDL statements "
          "(e.g. INSERT, UPDATE, DELETE, DROP) are not allowed for safety.";
      return result;
    }

    result.query = request.query_text;

    SPIConnection spi_conn;
    if (!spi_conn) {
      result.error_message = spi_conn.getErrorMessage();
      return result;
    }

    std::string explain_query =
        "EXPLAIN (ANALYZE, VERBOSE, COSTS, SETTINGS, BUFFERS, FORMAT JSON) " +
        request.query_text;

    int ret = SPI_execute(explain_query.c_str(), false, 0);

    if (ret < 0) {
      result.error_message = "Failed to execute EXPLAIN query: " +
                             std::string(SPI_result_code_string(ret));
      return result;
    }

    if (ret != SPI_OK_SELECT && ret != SPI_OK_UTILITY) {
      result.error_message =
          "Failed to execute EXPLAIN query. SPI result code: " +
          std::to_string(ret) + " (" +
          std::string(SPI_result_code_string(ret)) + "). " +
          "This may indicate the query failed or EXPLAIN ANALYZE is not "
          "supported in this context.";
      return result;
    }

    if (SPI_processed == 0) {
      result.error_message = "No output from EXPLAIN query";
      return result;
    }

    SPITupleTable* tuptable = SPI_tuptable;
    TupleDesc tupdesc = tuptable->tupdesc;
    HeapTuple tuple = tuptable->vals[0];

    SPIValue explain_output(SPI_getvalue(tuple, tupdesc, 1));
    if (!explain_output) {
      result.error_message = "Failed to get EXPLAIN output";
      return result;
    }

    result.explain_output = explain_output.toString();

    auto selection =
        ProviderSelector::selectProvider(request.api_key, request.provider);

    if (!selection.success) {
      result.error_message = selection.error_message;
      return result;
    }

    std::string prompt =
        "Please analyze this PostgreSQL EXPLAIN ANALYZE output:\n\nQuery:\n" +
        request.query_text + "\n\nEXPLAIN Output:\n" + result.explain_output;

    // Handle Gemini separately as it uses a different client
    if (selection.provider == config::Provider::GEMINI) {
      std::string model_name =
          (selection.config && !selection.config->default_model.empty())
              ? selection.config->default_model
              : "gemini-2.5-flash";
      logger::Logger::info("Using Gemini model for explain: " + model_name);

      gemini::GeminiClient gemini_client(selection.api_key);
      gemini::GeminiRequest gemini_request{
          .model = model_name,
          .system_prompt = prompts::EXPLAIN_SYSTEM_PROMPT,
          .user_prompt = prompt,
          .temperature =
              selection.config
                  ? std::optional<double>(selection.config->default_temperature)
                  : std::nullopt,
          .max_tokens =
              selection.config
                  ? std::optional<int>(selection.config->default_max_tokens)
                  : std::nullopt};

      auto gemini_result = gemini_client.generate_text(gemini_request);

      if (!gemini_result.success) {
        result.error_message =
            "Gemini API error: " + gemini_result.error_message;
        return result;
      }

      if (gemini_result.text.empty()) {
        result.error_message = "Empty response from Gemini service";
        return result;
      }

      result.ai_explanation = gemini_result.text;
      result.success = true;
      return result;
    }

    // Use AIClientFactory for OpenAI and Anthropic
    auto client_result = AIClientFactory::createClient(
        selection.provider, selection.api_key, selection.config);

    if (!client_result.success) {
      result.error_message = client_result.error_message;
      return result;
    }

    ai::GenerateOptions options(client_result.model_name,
                                prompts::EXPLAIN_SYSTEM_PROMPT, prompt);

    if (selection.config) {
      options.max_tokens = selection.config->default_max_tokens;
      options.temperature = selection.config->default_temperature;
    }

    auto ai_result = client_result.client.generate_text(options);

    if (!ai_result) {
      result.error_message =
          "AI API error: " + utils::formatAPIError(ai_result.error_message());
      return result;
    }

    if (ai_result.text.empty()) {
      result.error_message = "Empty response from AI service";
      return result;
    }

    result.ai_explanation = ai_result.text;
    result.success = true;
    return result;

  } catch (const std::exception& e) {
    result.error_message = "Internal error: " + std::string(e.what());
    return result;
  }
}

}  // namespace pg_ai