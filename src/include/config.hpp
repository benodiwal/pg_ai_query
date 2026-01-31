#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace pg_ai::config {

/**
 * @brief Configuration constants and default values
 * 
 * Contains all default settings including provider names, API endpoints,
 * config file paths, and default model parameters.
 */
namespace constants {
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
}  // namespace constants

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
 * const auto* openai_config = ConfigManager::getProviderConfig(Provider::OPENAI);
 * if (openai_config && !openai_config->api_key.empty()) {
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
   * @return String representation ("openai", "anthropic", "gemini", or "unknown")
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