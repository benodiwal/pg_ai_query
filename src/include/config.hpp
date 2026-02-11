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
  Provider provider;
  std::string api_key;
  std::string default_model;
  int default_max_tokens;
  double default_temperature;
  std::string api_endpoint;  // Custom API endpoint URL (optional)

  // Default constructor
  ProviderConfig()
      : provider(Provider::UNKNOWN),
        default_max_tokens(4096),
        default_temperature(0.7),
        api_endpoint() {}

  ProviderConfig(const Provider provider,
                 std::string default_model,
                 const int default_max_tokens,
                 const double default_temperature = 0.7)
      : provider(provider),
        default_model(std::move(default_model)),
        default_max_tokens(default_max_tokens),
        default_temperature(default_temperature),
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
  ProviderConfig default_provider;
  std::vector<ProviderConfig> providers;

  // General settings
  std::string log_level;
  bool enable_logging;
  int request_timeout_ms;
  int max_retries;

  // Query generation settings
  bool enforce_limit;
  int default_limit;
  /** Maximum characters allowed in natural language query (default: 4000) */
  int max_query_length;

  // Response format settings
  bool show_explanation;
  bool show_warnings;
  bool show_suggested_visualization;
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

  static ProviderConfig getProviderDefaultConfigValues(Provider provider);
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

  /**
   * @brief Validates configuration section
   *
   * @param section section that will be validated to current available sections
   * @return true if valid section, false otherwise
   * @note Internal use only
   */
  static bool isValidSection(const std::string& section);

  /**
   * @brief Validates key-value line to INI style format using a regex pattern
   *
   * @param line key-value line that will be validated
   * @return true if valid INI style format, false otherwise
   * @note Internal use only
   */
  static bool isValidLine(const std::string& line);

  /**
   * @brief parses boolean value with multiple possible variants (for internal
   * use)
   *
   * @param value boolean value that can be (true/false,yes/no,1/0), will be
   * converted to lowercase.
   * @return true/false based on the provided value, will fall back to false
   * with warning if wrong boolean value provided
   * @note Internal use only
   */
  static bool parseBooleanValue(const std::string& value);

  /**
   * @brief Unescape quotes in a string
   *
   * Converts escaped quotes (`\"` or `\'`) into literal quotes. Other
   * characters preceded by `\` are treated literally.
   *
   * @param value Input string possibly containing escaped quotes
   * @return String with escaped quotes converted to literal quotes
   * @note Internal use only
   */
  static std::string unescapeQuotes(const std::string& value);

  /**
   * @brief Find the closing quote in a string
   *
   * Scans a quoted string to locate the position of the closing quote, taking
   * escaped quotes into account.
   *
   * @param value Input string starting with an opening quote
   * @param quote Quote character to match (`'` or `"`)
   * @return Index of the closing quote in the string, or std::string::npos if
   * not found
   * @note Internal use only
   */
  static size_t findClosingQuote(const std::string& value, char quote);

  /**
   * @brief Parse a provider-specific configuration key-value pair
   *
   * Updates the configuration for a given provider (OpenAI, Anthropic, Gemini)
   * with a specific key-value pair.
   *
   * @param key Configuration key (e.g., "api_key", "default_model")
   * @param value Configuration value as string
   * @param provider Provider enum specifying which provider to update
   *
   * if the provider is UNKNOWN, the function does nothing.
   * @note Internal use only
   */
  static void parseProviderSection(const std::string& key,
                                   const std::string& value,
                                   Provider provider);
};

// Convenience macros for accessing config
#define PG_AI_CONFIG() pg_ai::config::ConfigManager::getConfig()
#define PG_AI_PROVIDER_CONFIG(provider) \
  pg_ai::config::ConfigManager::getProviderConfig(provider)

}  // namespace pg_ai::config