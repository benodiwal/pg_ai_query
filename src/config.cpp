#include "include/config.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unistd.h>

#include "include/logger.hpp"
#include "include/utils.hpp"

namespace pg_ai::config {

Configuration ConfigManager::config_;
bool ConfigManager::config_loaded_ = false;

Configuration::Configuration() {
  // General settings defaults
  log_level = "INFO";
  enable_logging = false;  // Default: disable logging
  request_timeout_ms = 30000;  // 30 seconds
  max_retries = 3;

  // Query generation defaults
  enforce_limit = true;
  default_limit = 1000;

  // Response format defaults
  show_explanation = true;
  show_warnings = true;
  show_suggested_visualization = false;
  use_formatted_response = false;


  // Set up default OpenAI provider
  default_provider.provider = Provider::OPENAI;
  default_provider.api_key = "";

  // Default OpenAI models
  ModelConfig gpt4o;
  gpt4o.name = "gpt-4o";
  gpt4o.description = "GPT-4 Omni - Latest model";
  gpt4o.max_tokens = 16384;
  gpt4o.temperature = 0.7;

  ModelConfig gpt4;
  gpt4.name = "gpt-4";
  gpt4.description = "GPT-4 - High quality model";
  gpt4.max_tokens = 8192;
  gpt4.temperature = 0.7;

  ModelConfig gpt35;
  gpt35.name = "gpt-3.5-turbo";
  gpt35.description = "GPT-3.5 Turbo - Fast and efficient";
  gpt35.max_tokens = 4096;
  gpt35.temperature = 0.7;

  default_provider.available_models = {gpt4o, gpt4, gpt35};
  default_provider.default_model = gpt4o;

  providers.push_back(default_provider);
}

bool ConfigManager::loadConfig() {
  std::string home_dir = getHomeDirectory();
  if (home_dir.empty()) {
    logger::Logger::warning("Could not determine home directory");
    return false;
  }

  std::string config_path = home_dir + "/.pg_ai.config";
  return loadConfig(config_path);
}

bool ConfigManager::loadConfig(const std::string& config_path) {
  logger::Logger::info("Loading configuration from: " + config_path);

  auto result = utils::read_file(config_path);
  if (!result.first) {
    logger::Logger::warning("Could not read config file: " + config_path +
                            ". Using defaults.");
    config_loaded_ = true;  // Use defaults
    return true;
  }

  if (parseConfig(result.second)) {
    config_loaded_ = true;
    // Enable/disable logging based on config
    logger::Logger::setLoggingEnabled(config_.enable_logging);
    logger::Logger::info("Configuration loaded successfully");
    return true;
  } else {
    logger::Logger::error("Failed to parse configuration file");
    return false;
  }
}

const Configuration& ConfigManager::getConfig() {
  if (!config_loaded_) {
    loadConfig();
  }
  return config_;
}

const ProviderConfig* ConfigManager::getProviderConfig(Provider provider) {
  if (!config_loaded_) {
    loadConfig();
  }

  for (const auto& p : config_.providers) {
    if (p.provider == provider) {
      return &p;
    }
  }
  return nullptr;
}

const ModelConfig* ConfigManager::getModelConfig(
    const std::string& model_name) {
  if (!config_loaded_) {
    loadConfig();
  }

  // Search in default provider first
  for (const auto& model : config_.default_provider.available_models) {
    if (model.name == model_name) {
      return &model;
    }
  }

  // Then search in all providers
  for (const auto& provider : config_.providers) {
    for (const auto& model : provider.available_models) {
      if (model.name == model_name) {
        return &model;
      }
    }
  }

  return nullptr;
}

std::string ConfigManager::providerToString(Provider provider) {
  switch (provider) {
    case Provider::OPENAI:
      return "openai";
    case Provider::ANTHROPIC:
      return "anthropic";
    default:
      return "unknown";
  }
}

Provider ConfigManager::stringToProvider(const std::string& provider_str) {
  std::string lower = provider_str;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

  if (lower == "openai")
    return Provider::OPENAI;
  if (lower == "anthropic")
    return Provider::ANTHROPIC;
  return Provider::UNKNOWN;
}

bool ConfigManager::parseConfig(const std::string& content) {
  std::istringstream stream(content);
  std::string line;
  std::string current_section;

  config_ = Configuration();  // Reset to defaults

  while (std::getline(stream, line)) {
    // Remove leading/trailing whitespace
    line.erase(0, line.find_first_not_of(" \t"));
    line.erase(line.find_last_not_of(" \t") + 1);

    // Skip empty lines and comments
    if (line.empty() || line[0] == '#') {
      continue;
    }

    // Handle sections
    if (line[0] == '[' && line.back() == ']') {
      current_section = line.substr(1, line.length() - 2);
      continue;
    }

    // Parse key-value pairs
    size_t eq_pos = line.find('=');
    if (eq_pos == std::string::npos) {
      continue;
    }

    std::string key = line.substr(0, eq_pos);
    std::string value = line.substr(eq_pos + 1);

    // Remove whitespace around key and value
    key.erase(0, key.find_first_not_of(" \t"));
    key.erase(key.find_last_not_of(" \t") + 1);
    value.erase(0, value.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t") + 1);

    // Remove quotes from value if present
    if (value.length() >= 2 && value[0] == '"' && value.back() == '"') {
      value = value.substr(1, value.length() - 2);
    }

    // Parse based on section
    if (current_section == "general") {
      if (key == "log_level")
        config_.log_level = value;
      else if (key == "enable_logging")
        config_.enable_logging = (value == "true");
      else if (key == "request_timeout_ms")
        config_.request_timeout_ms = std::stoi(value);
      else if (key == "max_retries")
        config_.max_retries = std::stoi(value);
    } else if (current_section == "query") {
      if (key == "enforce_limit")
        config_.enforce_limit = (value == "true");
      else if (key == "default_limit")
        config_.default_limit = std::stoi(value);
    } else if (current_section == "response") {
      if (key == "show_explanation")
        config_.show_explanation = (value == "true");
      else if (key == "show_warnings")
        config_.show_warnings = (value == "true");
      else if (key == "show_suggested_visualization")
        config_.show_suggested_visualization = (value == "true");
      else if (key == "use_formatted_response") {
        config_.use_formatted_response = (value == "true");
      }
    } else if (current_section == "openai") {
      auto provider_config = getProviderConfigMutable(Provider::OPENAI);
      if (!provider_config) {
        // Create new provider config
        ProviderConfig new_config;
        new_config.provider = Provider::OPENAI;
        config_.providers.push_back(new_config);
        provider_config = &config_.providers.back();
      }

      if (key == "api_key")
        provider_config->api_key = value;
      else if (key == "default_model") {
        // Find model in available models
        for (const auto& model : provider_config->available_models) {
          if (model.name == value) {
            provider_config->default_model = model;
            break;
          }
        }
      }
    } else if (current_section == "anthropic") {
      auto provider_config = getProviderConfigMutable(Provider::ANTHROPIC);
      if (!provider_config) {
        ProviderConfig new_config;
        new_config.provider = Provider::ANTHROPIC;

        // Add default Claude models
        ModelConfig claude3_5;
        claude3_5.name = "claude-3-5-sonnet-20241022";
        claude3_5.description = "Claude 3.5 Sonnet - Latest model";
        claude3_5.max_tokens = 8192;
        claude3_5.temperature = 0.7;

        new_config.available_models.push_back(claude3_5);
        new_config.default_model = claude3_5;

        config_.providers.push_back(new_config);
        provider_config = &config_.providers.back();
      }

      if (key == "api_key")
        provider_config->api_key = value;
    }
  }

  // Set default provider if specified
  if (!config_.providers.empty()) {
    config_.default_provider = config_.providers[0];
  }

  return true;
}

ProviderConfig* ConfigManager::getProviderConfigMutable(Provider provider) {
  for (auto& p : config_.providers) {
    if (p.provider == provider) {
      return &p;
    }
  }
  return nullptr;
}

std::string ConfigManager::getHomeDirectory() {
  const char* home = std::getenv("HOME");
  if (home) {
    return std::string(home);
  }

  // Fallback: try to get from getpwuid
  const char* user = std::getenv("USER");
  if (user) {
    return std::string("/home/") + user;
  }

  return "";
}

}  // namespace pg_ai::config