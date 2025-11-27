#include "../include/query_generator.hpp"

extern "C" {
#include <postgres.h>

#include <utils/builtins.h>

#include <executor/spi.h>
}

#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>
#include <regex>
#include <sstream>
#include <vector>

#include <ai/anthropic.h>
#include <ai/openai.h>
#include <nlohmann/json.hpp>

#include "../include/config.hpp"
#include "../include/logger.hpp"
#include "../include/prompts.hpp"
#include "../include/utils.hpp"

using namespace pg_ai::logger;

namespace pg_ai {

QueryResult QueryGenerator::generateQuery(const QueryRequest& request) {
  try {
    if (request.natural_language.empty()) {
      return {.success = false,
              .error_message = "Natural language query cannot be empty"};
    }

    const auto& cfg = config::ConfigManager::getConfig();

    std::string api_key = request.api_key;
    std::string api_key_source = "parameter";
    std::string provider_preference = request.provider;

    config::Provider selected_provider;
    const config::ProviderConfig* provider_config = nullptr;

    if (provider_preference == "openai") {
      selected_provider = config::Provider::OPENAI;
      provider_config =
          config::ConfigManager::getProviderConfig(config::Provider::OPENAI);
      logger::Logger::info("Explicit OpenAI provider selection from parameter");

      if (api_key.empty() && provider_config &&
          !provider_config->api_key.empty()) {
        api_key = provider_config->api_key;
        api_key_source = "openai_config";
        logger::Logger::info("Using OpenAI API key from configuration");
      }
    } else if (provider_preference == "anthropic") {
      selected_provider = config::Provider::ANTHROPIC;
      provider_config =
          config::ConfigManager::getProviderConfig(config::Provider::ANTHROPIC);
      logger::Logger::info(
          "Explicit Anthropic provider selection from parameter");

      if (api_key.empty() && provider_config &&
          !provider_config->api_key.empty()) {
        api_key = provider_config->api_key;
        api_key_source = "anthropic_config";
        logger::Logger::info("Using Anthropic API key from configuration");
      }
    } else {
      if (api_key.empty()) {
        const auto* openai_config =
            config::ConfigManager::getProviderConfig(config::Provider::OPENAI);
        if (openai_config && !openai_config->api_key.empty()) {
          logger::Logger::info(
              "Auto-selecting OpenAI provider based on configuration");
          selected_provider = config::Provider::OPENAI;
          provider_config = openai_config;
          api_key = openai_config->api_key;
          api_key_source = "openai_config";
        } else {
          const auto* anthropic_config =
              config::ConfigManager::getProviderConfig(
                  config::Provider::ANTHROPIC);
          if (anthropic_config && !anthropic_config->api_key.empty()) {
            logger::Logger::info(
                "Auto-selecting Anthropic provider based on configuration");
            selected_provider = config::Provider::ANTHROPIC;
            provider_config = anthropic_config;
            api_key = anthropic_config->api_key;
            api_key_source = "anthropic_config";
          } else {
            logger::Logger::warning("No API key found in config");
            return {.success = false,
                    .error_message =
                        "API key required. Pass as 4th parameter or set OpenAI "
                        "or Anthropic API key in ~/.pg_ai.config."};
          }
        }
      } else {
        selected_provider = config::Provider::OPENAI;
        provider_config =
            config::ConfigManager::getProviderConfig(config::Provider::OPENAI);
        logger::Logger::info(
            "Auto-selecting OpenAI provider (API key provided, no provider "
            "specified)");
      }
    }

    if (api_key.empty()) {
      std::string provider_name =
          config::ConfigManager::providerToString(selected_provider);
      return {.success = false,
              .error_message = "No API key available for " + provider_name +
                               " provider. Please provide API key as parameter "
                               "or configure it in ~/.pg_ai.config."};
    }

    std::string system_prompt = prompts::SYSTEM_PROMPT;

    std::string prompt = buildPrompt(request);

    config::Provider provider = selected_provider;

    ai::Client client;
    std::string model_name;

    try {
      if (provider == config::Provider::OPENAI) {
        logger::Logger::info("Creating OpenAI client");
        client = ai::openai::create_client(api_key);
        model_name =
            (provider_config && !provider_config->default_model.empty())
                ? provider_config->default_model
                : "gpt-4o";
      } else if (provider == config::Provider::ANTHROPIC) {
        logger::Logger::info("Creating Anthropic client");
        client = ai::anthropic::create_client(api_key);
        model_name =
            (provider_config && !provider_config->default_model.empty())
                ? provider_config->default_model
                : "claude-sonnet-4-5-20250929";
      } else {
        logger::Logger::warning("Unknown provider, defaulting to OpenAI");
        client = ai::openai::create_client(api_key);
        model_name = "gpt-4o";
      }
      logger::Logger::info("Using Provider: " +
                           config::ConfigManager::providerToString(provider));
    } catch (const std::exception& e) {
      logger::Logger::error("Failed to create " +
                            config::ConfigManager::providerToString(provider) +
                            " client: " + std::string(e.what()));
      throw std::runtime_error("Failed to create AI client: " +
                               std::string(e.what()));
    }

    ai::GenerateOptions options(model_name, system_prompt, prompt);

    if (provider_config) {
      options.max_tokens = provider_config->default_max_tokens;
      options.temperature = provider_config->default_temperature;
      std::string log_msg = "Using model: " + model_name;
      if (options.max_tokens.has_value()) {
        log_msg +=
            " with max_tokens=" + std::to_string(options.max_tokens.value());
      }
      if (options.temperature.has_value()) {
        log_msg +=
            ", temperature=" + std::to_string(options.temperature.value());
      }
      logger::Logger::info(log_msg);
    } else {
      logger::Logger::info("Using model: " + model_name +
                           " with default settings");
    }

    auto result = client.generate_text(options);

    if (!result) {
      return {.success = false,
              .error_message = "AI API error: " + result.error_message()};
    }

    if (result.text.empty()) {
      return {.success = false,
              .error_message = "Empty response from AI service"};
    }

    nlohmann::json j = extractSQLFromResponse(result.text);
    std::string sql = j.value("sql", "");
    std::string explanation = j.value("explanation", "");

    if (sql.empty()) {
      return {
          .success = true, .explanation = explanation, .generated_query = ""};
    }

    std::string upper_sql = sql;
    std::transform(upper_sql.begin(), upper_sql.end(), upper_sql.begin(),
                   ::toupper);
    if (upper_sql.find("INFORMATION_SCHEMA") != std::string::npos ||
        upper_sql.find("PG_CATALOG") != std::string::npos) {
      return {.success = false,
              .error_message =
                  "Generated query accesses system tables. Please query user "
                  "tables only."};
    }

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
    }

    return {
        .generated_query = sql,
        .explanation = explanation,
        .warnings = warnings_vec,
        .row_limit_applied = j.value("row_limit_applied", false),
        .suggested_visualization = j.value("suggested_visualization", "table"),
        .success = true,
        .error_message = ""};
  } catch (const std::exception& e) {
    return {.success = false,
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
  } catch (...) {
  }

  if (!schema_context.empty()) {
    prompt << "Schema info:\n" << schema_context << "\n";
  }

  return prompt.str();
}

nlohmann::json QueryGenerator::extractSQLFromResponse(const std::string& text) {
  std::regex json_block(R"(```(?:json)?\s*(\{[\s\S]*?\})\s*```)",
                        std::regex::icase);
  std::smatch match;

  if (std::regex_search(text, match, json_block)) {
    try {
      return nlohmann::json::parse(match[1].str());
    } catch (...) {
    }
  }

  try {
    return nlohmann::json::parse(text);
  } catch (...) {
  }

  // Fallback
  return {{"sql", text}, {"explanation", "Raw LLM output (no JSON detected)"}};
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

    result.query = request.query_text;

    if (SPI_connect() != SPI_OK_CONNECT) {
      result.error_message = "Failed to connect to SPI";
      return result;
    }

    std::string explain_query =
        "EXPLAIN (ANALYZE, VERBOSE, COSTS, SETTINGS, BUFFERS, FORMAT JSON) " +
        request.query_text;

    int ret = SPI_execute(explain_query.c_str(), false, 0);

    if (ret < 0) {
      result.error_message = "Failed to execute EXPLAIN query: " +
                             std::string(SPI_result_code_string(ret));
      SPI_finish();
      return result;
    }

    if (ret != SPI_OK_SELECT && ret != SPI_OK_UTILITY) {
      result.error_message =
          "Failed to execute EXPLAIN query. SPI result code: " +
          std::to_string(ret) + " (" +
          std::string(SPI_result_code_string(ret)) + "). " +
          "This may indicate the query failed or EXPLAIN ANALYZE is not "
          "supported in this context.";
      SPI_finish();
      return result;
    }

    if (SPI_processed == 0) {
      result.error_message = "No output from EXPLAIN query";
      SPI_finish();
      return result;
    }

    SPITupleTable* tuptable = SPI_tuptable;
    TupleDesc tupdesc = tuptable->tupdesc;
    HeapTuple tuple = tuptable->vals[0];

    char* explain_output = SPI_getvalue(tuple, tupdesc, 1);
    if (!explain_output) {
      result.error_message = "Failed to get EXPLAIN output";
      SPI_finish();
      return result;
    }

    result.explain_output = std::string(explain_output);
    SPI_finish();

    const auto& cfg = config::ConfigManager::getConfig();
    std::string api_key = request.api_key;
    std::string provider_preference = request.provider;

    config::Provider selected_provider;
    const config::ProviderConfig* provider_config = nullptr;

    if (provider_preference == "openai") {
      selected_provider = config::Provider::OPENAI;
      provider_config =
          config::ConfigManager::getProviderConfig(config::Provider::OPENAI);
      if (api_key.empty() && provider_config &&
          !provider_config->api_key.empty()) {
        api_key = provider_config->api_key;
      }
    } else if (provider_preference == "anthropic") {
      selected_provider = config::Provider::ANTHROPIC;
      provider_config =
          config::ConfigManager::getProviderConfig(config::Provider::ANTHROPIC);
      if (api_key.empty() && provider_config &&
          !provider_config->api_key.empty()) {
        api_key = provider_config->api_key;
      }
    } else {
      if (api_key.empty()) {
        const auto* openai_config =
            config::ConfigManager::getProviderConfig(config::Provider::OPENAI);
        if (openai_config && !openai_config->api_key.empty()) {
          selected_provider = config::Provider::OPENAI;
          provider_config = openai_config;
          api_key = openai_config->api_key;
        } else {
          const auto* anthropic_config =
              config::ConfigManager::getProviderConfig(
                  config::Provider::ANTHROPIC);
          if (anthropic_config && !anthropic_config->api_key.empty()) {
            selected_provider = config::Provider::ANTHROPIC;
            provider_config = anthropic_config;
            api_key = anthropic_config->api_key;
          } else {
            result.error_message =
                "API key required. Pass as parameter or configure "
                "~/.pg_ai.config";
            return result;
          }
        }
      } else {
        selected_provider = config::Provider::OPENAI;
        provider_config =
            config::ConfigManager::getProviderConfig(config::Provider::OPENAI);
      }
    }

    if (api_key.empty()) {
      result.error_message = "No API key available for provider";
      return result;
    }

    std::string system_prompt = prompts::EXPLAIN_SYSTEM_PROMPT;

    std::string prompt =
        "Please analyze this PostgreSQL EXPLAIN ANALYZE output:\n\nQuery:\n" +
        request.query_text + "\n\nEXPLAIN Output:\n" + result.explain_output;

    ai::Client client;
    std::string model_name;

    try {
      if (selected_provider == config::Provider::OPENAI) {
        client = ai::openai::create_client(api_key);
        model_name =
            (provider_config && !provider_config->default_model.empty())
                ? provider_config->default_model
                : "gpt-4o";
      } else if (selected_provider == config::Provider::ANTHROPIC) {
        client = ai::anthropic::create_client(api_key);
        model_name =
            (provider_config && !provider_config->default_model.empty())
                ? provider_config->default_model
                : "claude-sonnet-4-5-20250929";
      } else {
        client = ai::openai::create_client(api_key);
        model_name = "gpt-4o";
      }
    } catch (const std::exception& e) {
      result.error_message =
          "Failed to create AI client: " + std::string(e.what());
      return result;
    }

    ai::GenerateOptions options(model_name, system_prompt, prompt);

    if (provider_config) {
      options.max_tokens = provider_config->default_max_tokens;
      options.temperature = provider_config->default_temperature;
    }

    auto ai_result = client.generate_text(options);

    if (!ai_result) {
      result.error_message = "AI API error: " + ai_result.error_message();
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