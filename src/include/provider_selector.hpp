#pragma once

#include <optional>
#include <string>

#include "config.hpp"

namespace pg_ai {

struct ProviderSelectionResult {
  config::Provider provider;
  const config::ProviderConfig* config;
  std::string api_key;
  std::string api_key_source;
  bool success;
  std::string error_message;
};

class ProviderSelector {
 public:
  /**
   * @brief Select the appropriate provider and resolve API key
   *
   * Selection logic:
   * 1. If provider_preference is explicitly "openai" or "anthropic", use that
   * 2. If api_key is provided without provider preference, default to OpenAI
   * 3. If no api_key is provided, auto-detect based on available config keys
   *
   * @param api_key API key passed as parameter (may be empty)
   * @param provider_preference Provider preference ("openai", "anthropic", or
   * empty for auto)
   * @return ProviderSelectionResult with selected provider, config, and API key
   */
  static ProviderSelectionResult selectProvider(
      const std::string& api_key,
      const std::string& provider_preference);

 private:
  static ProviderSelectionResult selectExplicitProvider(
      const std::string& api_key,
      config::Provider provider);

  static ProviderSelectionResult autoSelectProvider(const std::string& api_key);
};

}  // namespace pg_ai
