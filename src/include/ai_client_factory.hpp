#pragma once

#include <string>

#include <ai/types/client.h>

#include "config.hpp"

namespace pg_ai {

namespace constants {
constexpr const char* DEFAULT_OPENAI_MODEL = "gpt-4o";
constexpr const char* DEFAULT_ANTHROPIC_MODEL = "claude-sonnet-4-5-20250929";
}  // namespace constants

struct AIClientResult {
  ai::Client client;
  std::string model_name;
  bool success;
  std::string error_message;
};

class AIClientFactory {
 public:
  /**
   * @brief Create an AI client for the specified provider
   *
   * @param provider The provider to create client for
   * @param api_key API key to use
   * @param provider_config Optional provider config for model/settings
   * @return AIClientResult with client and model name
   */
  static AIClientResult createClient(
      config::Provider provider,
      const std::string& api_key,
      const config::ProviderConfig* provider_config = nullptr);

  /**
   * @brief Get the default model name for a provider
   */
  static std::string getDefaultModel(config::Provider provider);
};

}  // namespace pg_ai
