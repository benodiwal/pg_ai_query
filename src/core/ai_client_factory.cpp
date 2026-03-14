#include "../include/ai_client_factory.hpp"

#include <ai/anthropic.h>
#include <ai/openai.h>

#include "../include/logger.hpp"
#include "include/constants.hpp"

namespace pg_ai {

AIClientResult AIClientFactory::createClient(
    config::Provider provider,
    const std::string& api_key,
    const config::ProviderConfig* provider_config) {
  AIClientResult result;
  result.success = true;

  try {
    switch (provider) {
      case config::Provider::OPENAI: {
        logger::Logger::info("Creating OpenAI client");

        // Determine the API endpoint to use
        std::string base_url =
            (provider_config && !provider_config->api_endpoint.empty())
                ? provider_config->api_endpoint
                : constants::DEFAULT_OPENAI_ENDPOINT;

        if (provider_config && !provider_config->api_endpoint.empty()) {
          logger::Logger::info("Using custom OpenAI endpoint: " + base_url);
        }

        result.client = ai::openai::create_client(api_key, base_url);
        result.model_name =
            (provider_config && !provider_config->default_model.empty())
                ? provider_config->default_model
                : constants::DEFAULT_OPENAI_MODEL;
        break;
      }

      case config::Provider::ANTHROPIC: {
        logger::Logger::info("Creating Anthropic client");

        // Determine the API endpoint to use
        std::string base_url =
            (provider_config && !provider_config->api_endpoint.empty())
                ? provider_config->api_endpoint
                : constants::DEFAULT_ANTHROPIC_ENDPOINT;

        if (provider_config && !provider_config->api_endpoint.empty()) {
          logger::Logger::info("Using custom Anthropic endpoint: " + base_url);
        }

        result.client = ai::anthropic::create_client(api_key, base_url);
        result.model_name =
            (provider_config && !provider_config->default_model.empty())
                ? provider_config->default_model
                : constants::DEFAULT_ANTHROPIC_MODEL;
        break;
      }

      default: {
        logger::Logger::warning("Unknown provider, defaulting to OpenAI");
        result.client = ai::openai::create_client(
            api_key, constants::DEFAULT_OPENAI_ENDPOINT);
        result.model_name = constants::DEFAULT_OPENAI_MODEL;
        break;
      }
    }

    logger::Logger::info("Using Provider: " +
                         config::ConfigManager::providerToString(provider));

  } catch (const std::exception& e) {
    logger::Logger::error("Failed to create " +
                          config::ConfigManager::providerToString(provider) +
                          " client: " + std::string(e.what()));
    result.success = false;
    result.error_message =
        "Failed to create AI client: " + std::string(e.what());
  }

  return result;
}

std::string AIClientFactory::getDefaultModel(config::Provider provider) {
  switch (provider) {
    case config::Provider::OPENAI:
      return constants::DEFAULT_OPENAI_MODEL;
    case config::Provider::ANTHROPIC:
      return constants::DEFAULT_ANTHROPIC_MODEL;
    default:
      return constants::DEFAULT_OPENAI_MODEL;
  }
}

}  // namespace pg_ai
