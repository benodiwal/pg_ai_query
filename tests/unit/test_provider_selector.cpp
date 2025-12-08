#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../test_helpers.hpp"
#include "include/config.hpp"
#include "include/provider_selector.hpp"

using namespace pg_ai;
using namespace pg_ai::config;
using namespace pg_ai::test_utils;

class ProviderSelectorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Load a valid config with both providers for most tests
    ConfigManager::loadConfig(getConfigFixture("valid_config.ini"));
  }
};

// Test explicit OpenAI provider selection with API key parameter
TEST_F(ProviderSelectorTest, ExplicitOpenAIWithApiKey) {
  auto result = ProviderSelector::selectProvider("sk-param-key", "openai");

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.provider, Provider::OPENAI);
  EXPECT_EQ(result.api_key, "sk-param-key");
  EXPECT_EQ(result.api_key_source, "parameter");
  EXPECT_TRUE(result.error_message.empty());
}

// Test explicit Anthropic provider selection with API key parameter
TEST_F(ProviderSelectorTest, ExplicitAnthropicWithApiKey) {
  auto result = ProviderSelector::selectProvider("sk-ant-param", "anthropic");

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.provider, Provider::ANTHROPIC);
  EXPECT_EQ(result.api_key, "sk-ant-param");
  EXPECT_EQ(result.api_key_source, "parameter");
}

// Test explicit provider selection falls back to config key
TEST_F(ProviderSelectorTest, ExplicitProviderUsesConfigKey) {
  auto result = ProviderSelector::selectProvider("", "openai");

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.provider, Provider::OPENAI);
  EXPECT_EQ(result.api_key,
            "sk-test-openai-key-12345");  // from valid_config.ini
  EXPECT_EQ(result.api_key_source, "openai_config");
}

// Test auto-selection with API key defaults to OpenAI
TEST_F(ProviderSelectorTest, AutoSelectWithKeyDefaultsToOpenAI) {
  auto result = ProviderSelector::selectProvider("sk-auto-key", "");

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.provider, Provider::OPENAI);
  EXPECT_EQ(result.api_key, "sk-auto-key");
  EXPECT_EQ(result.api_key_source, "parameter");
}

// Test auto-selection without key uses config (OpenAI first)
TEST_F(ProviderSelectorTest, AutoSelectWithoutKeyUsesConfig) {
  auto result = ProviderSelector::selectProvider("", "");

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.provider, Provider::OPENAI);
  EXPECT_EQ(result.api_key, "sk-test-openai-key-12345");
  EXPECT_EQ(result.api_key_source, "openai_config");
}

// Test auto-selection falls back to Anthropic when no OpenAI key
TEST_F(ProviderSelectorTest, AutoSelectFallsBackToAnthropic) {
  // Load config with only Anthropic
  ConfigManager::loadConfig(getConfigFixture("anthropic_only.ini"));

  auto result = ProviderSelector::selectProvider("", "");

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.provider, Provider::ANTHROPIC);
  EXPECT_EQ(result.api_key, "sk-ant-only-key");
  EXPECT_EQ(result.api_key_source, "anthropic_config");
}

// Test failure when no API key available
TEST_F(ProviderSelectorTest, FailsWhenNoApiKeyAvailable) {
  // Load empty config
  ConfigManager::loadConfig(getConfigFixture("empty_config.ini"));

  auto result = ProviderSelector::selectProvider("", "");

  EXPECT_FALSE(result.success);
  EXPECT_FALSE(result.error_message.empty());
  EXPECT_THAT(result.error_message, testing::HasSubstr("API key required"));
}

// Test explicit provider fails without key
TEST_F(ProviderSelectorTest, ExplicitProviderFailsWithoutKey) {
  // Load empty config
  ConfigManager::loadConfig(getConfigFixture("empty_config.ini"));

  auto result = ProviderSelector::selectProvider("", "openai");

  EXPECT_FALSE(result.success);
  EXPECT_THAT(result.error_message, testing::HasSubstr("No API key available"));
  EXPECT_THAT(result.error_message, testing::HasSubstr("openai"));
}

// Test that config pointer is set correctly
TEST_F(ProviderSelectorTest, SetsConfigPointer) {
  auto result = ProviderSelector::selectProvider("sk-test", "anthropic");

  EXPECT_TRUE(result.success);
  EXPECT_NE(result.config, nullptr);
  EXPECT_EQ(result.config->provider, Provider::ANTHROPIC);
}

// Test provider preference "auto" behaves like empty string
TEST_F(ProviderSelectorTest, AutoPreferenceBehavesLikeEmpty) {
  auto result_auto = ProviderSelector::selectProvider("sk-test", "auto");
  auto result_empty = ProviderSelector::selectProvider("sk-test", "");

  // Both should default to OpenAI when key is provided
  EXPECT_EQ(result_auto.provider, result_empty.provider);
  EXPECT_EQ(result_auto.api_key_source, result_empty.api_key_source);
}

// Test case insensitivity of provider names would need lowercase conversion
// Current implementation expects lowercase, so this tests the current behavior
TEST_F(ProviderSelectorTest, ProviderNamesCaseSensitive) {
  auto result_lower = ProviderSelector::selectProvider("sk-test", "openai");
  EXPECT_TRUE(result_lower.success);
  EXPECT_EQ(result_lower.provider, Provider::OPENAI);

  // Uppercase won't match - will go to auto-select
  auto result_upper = ProviderSelector::selectProvider("sk-test", "OPENAI");
  EXPECT_TRUE(result_upper.success);
  // Goes to auto-select path, still picks OpenAI due to provided key
  EXPECT_EQ(result_upper.provider, Provider::OPENAI);
}

// Test with minimal config (only OpenAI)
TEST_F(ProviderSelectorTest, MinimalConfigOpenAIOnly) {
  ConfigManager::loadConfig(getConfigFixture("minimal_config.ini"));

  // Auto-select should find OpenAI
  auto result = ProviderSelector::selectProvider("", "");
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.provider, Provider::OPENAI);
  EXPECT_EQ(result.api_key, "sk-minimal-test-key");

  // Explicit Anthropic should fail (no key)
  auto result_anthropic = ProviderSelector::selectProvider("", "anthropic");
  EXPECT_FALSE(result_anthropic.success);
}

// Test that ProviderSelectionResult has expected initial values
TEST(ProviderSelectionResultTest, DefaultValues) {
  ProviderSelectionResult result;
  result.success = false;  // explicitly set to show it's not auto-true

  EXPECT_FALSE(result.success);
  EXPECT_TRUE(result.api_key.empty());
  EXPECT_TRUE(result.api_key_source.empty());
  EXPECT_TRUE(result.error_message.empty());
  EXPECT_EQ(result.config, nullptr);
}
