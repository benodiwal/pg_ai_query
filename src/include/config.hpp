#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace pg_ai::config {

/**
 * @brief Enumeration of supported AI providers
 *
 * Represents the available AI providers for query generation and analysis.
 */
enum class Provider { OPENAI, ANTHROPIC, GEMINI, UNKNOWN };

/**
 * @brief Configuration for a specific AI provider
 *
 * Contains all settings needed to interact with a particular AI provider
 * including API keys, model selection, and endpoint configuration.
 */
struct ProviderConfig {
  /** AI provider type: OPENAI, ANTHROPIC, GEMINI, or UNKNOWN */
  Provider provider;

  /** API key for authenticating with the provider (required) */
  std::string api_key;

  /**
   * Model identifier to use for requests.
   * Examples: "gpt-4o" (OpenAI), "claude-sonnet-4-5-20250929" (Anthropic),
   *           "gemini-2.0-flash" (Gemini)
   */
  std::string default_model;

  /** Maximum tokens in the AI response (default: 4096) */
  int default_max_tokens;

  /**
   * Sampling temperature for response randomness (default: 0.7).
   * Range: 0.0 (deterministic) to 2.0 (highly creative).
   * Lower values produce more consistent SQL output.
   */
  double default_temperature;

  /** Custom API endpoint URL. Leave empty to use the provider's default. */
  std::string api_endpoint;

  ProviderConfig()
      : provider(Provider::UNKNOWN),
        default_max_tokens(4096),
        default_temperature(0.7),
        api_endpoint() {}
};

/**
 * @brief Global configuration for the pg_ai_query extension
 *
 * Contains all configurable settings for the extension including provider
 * configurations, logging options, query generation behavior, and response
 * formatting preferences. Typically loaded from ~/.pg_ai.config.
 */
struct Configuration {
  /** The provider to use when none is specified in a request */
  ProviderConfig default_provider;

  /** All configured providers (populated from config file sections) */
  std::vector<ProviderConfig> providers;

  // === General Settings ===

  /** Log verbosity level: "DEBUG", "INFO", "WARNING", "ERROR" (default: "INFO") */
  std::string log_level;

  /** Enable or disable all logging output (default: true) */
  bool enable_logging;

  /** API request timeout in milliseconds (default: 30000 = 30 seconds) */
  int request_timeout_ms;

  /** Maximum retry attempts for failed API requests; 0 disables retries (default: 3) */
  int max_retries;

  // === Query Generation Settings ===

  /**
   * Automatically append a LIMIT clause to generated SELECT queries
   * to prevent accidental large result sets (default: true)
   */
  bool enforce_limit;

  /** Row limit to apply when enforce_limit is true (default: 100) */
  int default_limit;

  /** Maximum characters allowed in the natural language input (default: 4000) */
  int max_query_length;

  // === Response Format Settings ===

  /** Include a natural-language explanation of the generated SQL (default: true) */
  bool show_explanation;

  /** Include warnings about potential issues with the generated query (default: true) */
  bool show_warnings;

  /** Include a suggested visualization type for the query results (default: false) */
  bool show_suggested_visualization;

  /** Return a structured JSON response instead of raw SQL text (default: true) */
  bool use_formatted_response;

  Configuration();
};

/**
 * @brief Manages loading and accessing configuration settings
 *
 * Singleton-style class that handles loading configuration from files
 * or environment variables and provides access to configuration settings
 * throughout the application. All methods are static and thread-safe.
 *
 * The configuration file uses INI format with sections: [general], [query],
 * [response], [openai], [anthropic], and [gemini].
 *
 * @example
 * // Load configuration from default location
 * ConfigManager::loadConfig();
 *
 * // Access configuration
 * const auto& config = ConfigManager::getConfig();
 * if (config.show_explanation) {
 *   // Include explanation in output
 * }
 *
 * // Get provider-specific config
 * const auto* openai_config =
 * ConfigManager::getProviderConfig(Provider::OPENAI); if (openai_config &&
 * !openai_config->api_key.empty()) {
 *   // Use OpenAI provider
 * }
 */
class ConfigManager {
 public:
  /**
   * @brief Load configuration from ~/.pg_ai.config
   * @return true if config loaded successfully, false otherwise
   */
  static bool loadConfig();

  /**
   * @brief Load configuration from specific file path
   * @param config_path Path to configuration file
   * @return true if config loaded successfully, false otherwise
   */
  static bool loadConfig(const std::string& config_path);

  /**
   * @brief Get current configuration
   * @return Reference to current configuration
   */
  static const Configuration& getConfig();

  /**
   * @brief Get provider config by provider type
   * @param provider Provider type to find
   * @return Pointer to provider config, or nullptr if not found
   */
  static const ProviderConfig* getProviderConfig(Provider provider);

  /**
   * @brief Convert provider enum to string
   *
   * @param provider Provider enum value to convert
   * @return String representation ("openai", "anthropic", "gemini", or
   * "unknown")
   */
  static std::string providerToString(Provider provider);

  /**
   * @brief Convert string to provider enum
   *
   * @param provider_str Provider name as string
   * @return Provider enum value (UNKNOWN if string doesn't match any provider)
   */
  static Provider stringToProvider(const std::string& provider_str);

  /**
   * @brief Reset configuration to defaults (for testing only)
   */
  static void reset();

 private:
  static Configuration config_;
  static bool config_loaded_;

  /**
   * @brief Parse configuration file content
   *
   * @param content INI file content to parse
   * @return true if parsing succeeded, false otherwise
   */
  static bool parseConfig(const std::string& content);

  /**
   * @brief Get home directory path
   *
   * @return String containing the user's home directory path
   */
  static std::string getHomeDirectory();

  /**
   * @brief Get mutable provider config (for internal use)
   *
   * @param provider Provider type to retrieve
   * @return Pointer to mutable provider config, or nullptr if not found
   */
  static ProviderConfig* getProviderConfigMutable(Provider provider);

  /**
   * @brief Load configuration from environment variables
   *
   * Reads environment variables like PG_AI_OPENAI_API_KEY to override
   * configuration file settings.
   */
  static void loadEnvConfig();
};

// Convenience macros for accessing config
#define PG_AI_CONFIG() pg_ai::config::ConfigManager::getConfig()
#define PG_AI_PROVIDER_CONFIG(provider) \
  pg_ai::config::ConfigManager::getProviderConfig(provider)

}  // namespace pg_ai::config