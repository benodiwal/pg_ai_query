#pragma once

/**
 * @brief Configuration constants and default values
 *
 * Contains all default settings including provider names, API endpoints,
 * config file paths, and default model parameters.
 */
namespace pg_ai::constants {
// Provider name strings
constexpr const char* PROVIDER_OPENAI = "openai";
constexpr const char* PROVIDER_ANTHROPIC = "anthropic";
constexpr const char* PROVIDER_GEMINI = "gemini";
constexpr const char* PROVIDER_AUTO = "auto";
constexpr const char* PROVIDER_UNKNOWN = "unknown";

// Default API endpoints
constexpr const char* DEFAULT_OPENAI_ENDPOINT = "https://api.openai.com";
constexpr const char* DEFAULT_ANTHROPIC_ENDPOINT = "https://api.anthropic.com";

// Config file path
constexpr const char* CONFIG_FILE_NAME = ".pg_ai.config";

// Config section names
constexpr const char* SECTION_GENERAL = "general";
constexpr const char* SECTION_QUERY = "query";
constexpr const char* SECTION_RESPONSE = "response";
constexpr const char* SECTION_OPENAI = "openai";
constexpr const char* SECTION_ANTHROPIC = "anthropic";
constexpr const char* SECTION_GEMINI = "gemini";

// Default model names
constexpr const char* DEFAULT_OPENAI_MODEL = "gpt-4o";
constexpr const char* DEFAULT_ANTHROPIC_MODEL = "claude-sonnet-4-5-20250929";

// Default token limits
constexpr int DEFAULT_OPENAI_MAX_TOKENS = 16384;
constexpr int DEFAULT_ANTHROPIC_MAX_TOKENS = 8192;
constexpr int DEFAULT_MAX_TOKENS = 4096;
constexpr double DEFAULT_TEMPERATURE = 0.7;
constexpr int DEFAULT_MAX_QUERY_LENGTH = 4000;

// Env Variables Names
constexpr const char* OPENAI_API_KEY_VARIABLE_NAME = "OPENAI_API_KEY";
constexpr const char* ANTHROPIC_API_KEY_VARIABLE_NAME = "ANTHROPIC_API_KEY";
constexpr const char* GEMINI_API_KEY_VARIABLE_NAME = "GEMINI_API_KEY";
}  // namespace pg_ai::constants
