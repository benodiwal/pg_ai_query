#include "../include/provider_selector.hpp"

#include "../include/logger.hpp"

namespace pg_ai {

ProviderSelectionResult ProviderSelector::selectProvider(
    const std::string& api_key,
    const std::string& provider_preference) {
  if (provider_preference == "openai") {
    return selectExplicitProvider(api_key, config::Provider::OPENAI);
  }

  if (provider_preference == "anthropic") {
    return selectExplicitProvider(api_key, config::Provider::ANTHROPIC);
  }

  return autoSelectProvider(api_key);
}

ProviderSelectionResult ProviderSelector::selectExplicitProvider(
    const std::string& api_key,
    config::Provider provider) {
  ProviderSelectionResult result;
  result.provider = provider;
  result.config = config::ConfigManager::getProviderConfig(provider);
  result.success = true;

  std::string provider_name = config::ConfigManager::providerToString(provider);
  logger::Logger::info("Explicit " + provider_name +
                       " provider selection from parameter");

  if (!api_key.empty()) {
    result.api_key = api_key;
    result.api_key_source = "parameter";
  } else if (result.config && !result.config->api_key.empty()) {
    result.api_key = result.config->api_key;
    result.api_key_source = provider_name + "_config";
    logger::Logger::info("Using " + provider_name +
                         " API key from configuration");
  }

  if (result.api_key.empty()) {
    result.success = false;
    result.error_message = "No API key available for " + provider_name +
                           " provider. Please provide API key as parameter "
                           "or configure it in ~/.pg_ai.config.";
  }

  return result;
}

ProviderSelectionResult ProviderSelector::autoSelectProvider(
    const std::string& api_key) {
  ProviderSelectionResult result;

  if (!api_key.empty()) {
    result.provider = config::Provider::OPENAI;
    result.config =
        config::ConfigManager::getProviderConfig(config::Provider::OPENAI);
    result.api_key = api_key;
    result.api_key_source = "parameter";
    result.success = true;
    logger::Logger::info(
        "Auto-selecting OpenAI provider (API key provided, no provider "
        "specified)");
    return result;
  }

  const auto* openai_config =
      config::ConfigManager::getProviderConfig(config::Provider::OPENAI);
  if (openai_config && !openai_config->api_key.empty()) {
    logger::Logger::info(
        "Auto-selecting OpenAI provider based on configuration");
    result.provider = config::Provider::OPENAI;
    result.config = openai_config;
    result.api_key = openai_config->api_key;
    result.api_key_source = "openai_config";
    result.success = true;
    return result;
  }

  const auto* anthropic_config =
      config::ConfigManager::getProviderConfig(config::Provider::ANTHROPIC);
  if (anthropic_config && !anthropic_config->api_key.empty()) {
    logger::Logger::info(
        "Auto-selecting Anthropic provider based on configuration");
    result.provider = config::Provider::ANTHROPIC;
    result.config = anthropic_config;
    result.api_key = anthropic_config->api_key;
    result.api_key_source = "anthropic_config";
    result.success = true;
    return result;
  }

  logger::Logger::warning("No API key found in config");
  result.success = false;
  result.error_message =
      "API key required. Pass as parameter or set OpenAI "
      "or Anthropic API key in ~/.pg_ai.config.";
  return result;
}

}  // namespace pg_ai
