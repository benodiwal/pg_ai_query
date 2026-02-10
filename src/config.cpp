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
  enable_logging = false;      // Default: disable logging
  request_timeout_ms = 30000;  // 30 seconds
  max_retries = 3;

  // Query generation defaults
  enforce_limit = true;
  default_limit = 1000;
  max_query_length = constants::DEFAULT_MAX_QUERY_LENGTH;

  // Response format defaults
  show_explanation = true;
  show_warnings = true;
  show_suggested_visualization = false;
  use_formatted_response = false;

  // Default OpenAI provider
  default_provider.provider = Provider::OPENAI;
  default_provider.api_key = "";
  default_provider.default_model = constants::DEFAULT_OPENAI_MODEL;
  default_provider.default_max_tokens = constants::DEFAULT_MAX_TOKENS;
  default_provider.default_temperature = constants::DEFAULT_TEMPERATURE;

  providers.push_back(default_provider);
}

bool ConfigManager::loadConfig() {
  std::string home_dir = getHomeDirectory();
  if (home_dir.empty()) {
    logger::Logger::warning("Could not determine home directory");
    return false;
  }

  std::string config_path = home_dir + "/" + constants::CONFIG_FILE_NAME;
  return loadConfig(config_path);
}

bool ConfigManager::loadConfig(const std::string& config_path) {
  logger::Logger::info("Loading configuration from: " + config_path);

  auto result = utils::read_file(config_path);
  if (!result.first) {
    logger::Logger::warning("Configuration file not found at: " + config_path);
    logger::Logger::info("To create it, run:");
    logger::Logger::info("  cat > " + config_path + " << 'EOF'");
    logger::Logger::info("  [openai]");
    logger::Logger::info("  api_key = \"your-api-key-here\"");
    logger::Logger::info("  EOF");
    logger::Logger::info("");

    logger::Logger::info("Documentation:");
    logger::Logger::info("  https://benodiwal.github.io/pg_ai_query/");
    logger::Logger::info(
        "  https://benodiwal.github.io/pg_ai_query/configuration.html");

    // Prefer failing early with a clear error so users don't get confusing
    // failures later.
    throw std::runtime_error(
        "pg_ai_query configuration file not found at: " + config_path +
        "\nCreate it with your API key. See: "
        "https://benodiwal.github.io/pg_ai_query/configuration.html");
  }

  if (parseConfig(result.second)) {
    config_loaded_ = true;
    // Enable/disable logging based on config
    logger::Logger::setLoggingEnabled(config_.enable_logging);
    pg_ai::logger::Logger::set_level(config_.log_level);
    logger::Logger::info("Configuration loaded successfully");
    // Override with environment variables
    loadEnvConfig();
    return true;
  } else {
    logger::Logger::error("Failed to parse configuration file");
    return false;
  }
}

void ConfigManager::loadEnvConfig() {
  // NOTE for developers: Environment variable loading is disabled for now - all
  // config via ~/.pg_ai.config
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

std::string ConfigManager::providerToString(Provider provider) {
  switch (provider) {
    case Provider::OPENAI:
      return constants::PROVIDER_OPENAI;
    case Provider::ANTHROPIC:
      return constants::PROVIDER_ANTHROPIC;
    case Provider::GEMINI:
      return constants::PROVIDER_GEMINI;
    default:
      return constants::PROVIDER_UNKNOWN;
  }
}

Provider ConfigManager::stringToProvider(const std::string& provider_str) {
  std::string lower = provider_str;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

  if (lower == constants::PROVIDER_OPENAI)
    return Provider::OPENAI;
  if (lower == constants::PROVIDER_ANTHROPIC)
    return Provider::ANTHROPIC;
  if (lower == constants::PROVIDER_GEMINI)
    return Provider::GEMINI;
  return Provider::UNKNOWN;
}

void ConfigManager::reset() {
  config_ = Configuration();
  config_loaded_ = false;
}

bool ConfigManager::parseConfig(const std::string& content) {
  std::istringstream stream(content);
  std::string line;
  std::string current_section;

  config_ = Configuration();

  while (std::getline(stream, line)) {
    // Remove leading/trailing whitespace
    line.erase(0, line.find_first_not_of(" \t"));
    line.erase(line.find_last_not_of(" \t") + 1);

    if (line.empty() || line[0] == '#') {
      continue;
    }

    if (line[0] == '[' && line.back() == ']') {
      current_section = line.substr(1, line.length() - 2);
      continue;
    }

    size_t eq_pos = line.find('=');
    if (eq_pos == std::string::npos) {
      continue;
    }

    std::string key = line.substr(0, eq_pos);
    std::string value = line.substr(eq_pos + 1);

    key.erase(0, key.find_first_not_of(" \t"));
    key.erase(key.find_last_not_of(" \t") + 1);
    value.erase(0, value.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t") + 1);

    // Handle quoted values and inline comments
    if (value.length() >= 2 && value[0] == '"') {
      size_t closing_quote = value.find('"', 1);
      if (closing_quote != std::string::npos) {
        value = value.substr(1, closing_quote - 1);
      }
    } else {
      size_t comment_pos = value.find('#');
      if (comment_pos != std::string::npos) {
        value = value.substr(0, comment_pos);
        value.erase(value.find_last_not_of(" \t") + 1);
      }
    }

    if (current_section == constants::SECTION_GENERAL) {
      if (key == "log_level")
        config_.log_level = value;
      else if (key == "enable_logging")
        config_.enable_logging = (value == "true");
      else if (key == "request_timeout_ms")
        config_.request_timeout_ms = std::stoi(value);
      else if (key == "max_retries")
        config_.max_retries = std::stoi(value);
    } else if (current_section == constants::SECTION_QUERY) {
      if (key == "enforce_limit")
        config_.enforce_limit = (value == "true");
      else if (key == "default_limit")
        config_.default_limit = std::stoi(value);
      else if (key == "max_query_length") {
        int val = std::stoi(value);
        if (val > 0)
          config_.max_query_length = val;
      }
    } else if (current_section == constants::SECTION_RESPONSE) {
      if (key == "show_explanation")
        config_.show_explanation = (value == "true");
      else if (key == "show_warnings")
        config_.show_warnings = (value == "true");
      else if (key == "show_suggested_visualization")
        config_.show_suggested_visualization = (value == "true");
      else if (key == "use_formatted_response") {
        config_.use_formatted_response = (value == "true");
      }
    } else if (current_section == constants::SECTION_OPENAI) {
      auto provider_config = getProviderConfigMutable(Provider::OPENAI);
      if (!provider_config) {
        ProviderConfig config;
        config.provider = Provider::OPENAI;
        config.default_model = constants::DEFAULT_OPENAI_MODEL;
        config_.providers.push_back(config);
        provider_config = &config_.providers.back();
      }

      if (key == "api_key")
        provider_config->api_key = value;
      else if (key == "default_model")
        provider_config->default_model = value;
      else if (key == "max_tokens")
        provider_config->default_max_tokens = std::stoi(value);
      else if (key == "temperature")
        provider_config->default_temperature = std::stod(value);
      else if (key == "api_endpoint")
        provider_config->api_endpoint = value;

    } else if (current_section == constants::SECTION_ANTHROPIC) {
      auto provider_config = getProviderConfigMutable(Provider::ANTHROPIC);
      if (!provider_config) {
        ProviderConfig config;
        config.provider = Provider::ANTHROPIC;
        config.default_model = constants::DEFAULT_ANTHROPIC_MODEL;
        config.default_max_tokens = constants::DEFAULT_ANTHROPIC_MAX_TOKENS;

        config_.providers.push_back(config);
        provider_config = &config_.providers.back();
      }

      if (key == "api_key")
        provider_config->api_key = value;
      else if (key == "default_model")
        provider_config->default_model = value;
      else if (key == "max_tokens")
        provider_config->default_max_tokens = std::stoi(value);
      else if (key == "temperature")
        provider_config->default_temperature = std::stod(value);
      else if (key == "api_endpoint")
        provider_config->api_endpoint = value;

    } else if (current_section == constants::SECTION_GEMINI) {
      auto provider_config = getProviderConfigMutable(Provider::GEMINI);
      if (!provider_config) {
        ProviderConfig config;
        config.provider = Provider::GEMINI;
        config.default_model = "gemini-2.5-flash";
        config.default_max_tokens = constants::DEFAULT_MAX_TOKENS;
        config.default_temperature = constants::DEFAULT_TEMPERATURE;

        config_.providers.push_back(config);
        provider_config = &config_.providers.back();
      }

      if (key == "api_key")
        provider_config->api_key = value;
      else if (key == "default_model")
        provider_config->default_model = value;
      else if (key == "max_tokens")
        provider_config->default_max_tokens = std::stoi(value);
      else if (key == "temperature")
        provider_config->default_temperature = std::stod(value);
    }
  }

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

  const char* user = std::getenv("USER");
  if (user) {
    return std::string("/home/") + user;
  }

  return "";
}

}  // namespace pg_ai::config