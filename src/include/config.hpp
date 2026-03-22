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
  /** The AI provider this config applies to */
  Provider provider;

  /** API key for authenticating with this provider */
  std::string api_key;

  /** Default model identifier (e.g. "gpt-4o", "claude-sonnet-4-5-20250929") */
  std::string default_model;

  /** Maximum number of tokens in the AI response (default: 4096) */
  int default_max_tokens;

  /** Sampling temperature in [0.0, 2.0]; lower = more deterministic (default: 0.7) */
  double default_temperature;

  /** Custom API endpoint URL; empty string uses the provider's default endpoint */
  std::string api_endpoint;

  // Default constructor
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
  /** The first provider config loaded; used as fallback when no provider is specified */
  ProviderConfig default_provider;

  /** All loaded provider configs, one per [openai] / [anthropic] / [gemini] section */
  std::vector<ProviderConfig> providers;

  // === General Settings ===

  /** Log verbosity: "DEBUG", "INFO", "WARNING", or "ERROR" (default: "INFO") */
  std::string log_level;

  /** Enable or disable all log output (default: false) */
  bool enable_logging;

  /** HTTP request timeout in milliseconds (default: 30000 = 30 s) */
  int request_timeout_ms;

  /** Maximum retry attempts for failed API requests; 0 = no retries (default: 3) */
  int max_retries;

  // === Query Generation Settings ===

  /**
   * Automatically append a LIMIT clause to SELECT queries to prevent
   * accidentally returning very large result sets (default: true)
   */
  bool enforce_limit;

  /** Row limit appended when enforce_limit is true (default: 1000) */
  int default_limit;

  /** Maximum characters allowed in a natural language query (default: 4000) */
  int max_query_length;

  // === Response Format Settings ===

  /** Include an AI-generated explanation alongside the SQL query (default: true) */
  bool show_explanation;

  /** Include AI-generated warnings in the response (default: true) */
  bool show_warnings;

  /** Include a suggested visualization hint in the response (default: false) */
  bool show_suggested_visualization;

  /**
   * Return the full response as a JSON object instead of annotated SQL
   * comments. Useful for programmatic consumption (default: false)
   */
  bool use_formatted_response;

  // Default constructor with sensible defaults
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