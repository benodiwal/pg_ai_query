#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace pg_ai::config {

enum class Provider { OPENAI, ANTHROPIC, UNKNOWN };

struct ModelConfig {
  std::string name;
  std::string description;
  int max_tokens;
  double temperature;

  ModelConfig() : max_tokens(4096), temperature(0.7) {}
};

struct ProviderConfig {
  Provider provider;
  std::string api_key;
  std::vector<ModelConfig> available_models;
  ModelConfig default_model;

  // Default constructor
  ProviderConfig() : provider(Provider::UNKNOWN) {}
};

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
   * @brief Get model config by name from default provider
   * @param model_name Name of the model
   * @return Pointer to model config, or nullptr if not found
   */
  static const ModelConfig* getModelConfig(const std::string& model_name);

  /**
   * @brief Convert provider enum to string
   */
  static std::string providerToString(Provider provider);

  /**
   * @brief Convert string to provider enum
   */
  static Provider stringToProvider(const std::string& provider_str);

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
};

// Convenience macros for accessing config
#define PG_AI_CONFIG() pg_ai::config::ConfigManager::getConfig()
#define PG_AI_PROVIDER_CONFIG(provider) \
  pg_ai::config::ConfigManager::getProviderConfig(provider)
#define PG_AI_MODEL_CONFIG(model) \
  pg_ai::config::ConfigManager::getModelConfig(model)

}  // namespace pg_ai::config