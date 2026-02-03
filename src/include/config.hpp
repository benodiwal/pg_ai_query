#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace pg_ai::config {

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

enum class Provider { OPENAI, ANTHROPIC, GEMINI, UNKNOWN };

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

struct Configuration {
  ProviderConfig default_provider;
  std::vector<ProviderConfig> providers;

  // General settings

  /**
  * Minimum level for log messages: DEBUG, INFO, WARNING, ERROR
  * Default: INFO
  * Optional
  */
  std::string log_level;

  /**
   * Enable/disable all logging output
   * Default: false
   */
  bool enable_logging;

  /**
   * Timeout for AI API requests in milliseconds
   * Default: 30000 (30 seconds)
   */
  int request_timeout_ms;

  /**
   * Maximum retry attempts for failed API requests
   * Default: 3
   */
  int max_retries;

  // Query generation settings

  /**
   * Always add LIMIT clause to SELECT queries
   * Default: true
   */
  bool enforce_limit;

  /**
   * Default row limit when none specified
   * Default: 1000
   */
  int default_limit;

  // Response format settings

  /**
   * Include detailed explanation of what the query does
   * Default: true
   */
  bool show_explanation;

  /**
   * Include warnings about performance, security, or data implications
   * Default: true
   */
  bool show_warnings;

/**
 * Include suggested visualization type for the query results
 * Default: false
 */
  bool show_suggested_visualization;

  /**
   * Return structured JSON instead of plain SQL
   * Default: false
   */
  bool use_formatted_response;

  // Default constructor with sensible defaults
  Configuration();
};

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
   */
  static std::string providerToString(Provider provider);

  /**
   * @brief Convert string to provider enum
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
   */
  static bool parseConfig(const std::string& content);

  /**
   * @brief Get home directory path
   */
  static std::string getHomeDirectory();

  /**
   * @brief Get mutable provider config (for internal use)
   */
  static ProviderConfig* getProviderConfigMutable(Provider provider);

  /**
   * @brief Load configuration from environment variables
   */
  static void loadEnvConfig();
};

// Convenience macros for accessing config
#define PG_AI_CONFIG() pg_ai::config::ConfigManager::getConfig()
#define PG_AI_PROVIDER_CONFIG(provider) \
  pg_ai::config::ConfigManager::getProviderConfig(provider)

}  // namespace pg_ai::config