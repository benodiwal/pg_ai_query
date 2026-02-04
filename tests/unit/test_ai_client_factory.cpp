#include <gtest/gtest.h>

#include "include/ai_client_factory.hpp"
#include "include/config.hpp"

using namespace pg_ai;
using namespace pg_ai::config;

class AIClientFactoryTest : public ::testing::Test {
 protected:
  ProviderConfig openai;
  ProviderConfig anthropic;
  ProviderConfig gemini;

  void SetUp() override {
    openai.provider = Provider::OPENAI;
    openai.api_key = "sk-test";
    openai.default_model = "gpt-4o";

    anthropic.provider = Provider::ANTHROPIC;
    anthropic.api_key = "anthropic-test";
    anthropic.default_model = "claude-3-5-sonnet-20241022";

    gemini.provider = Provider::GEMINI;
    gemini.api_key = "gemini-test";
    gemini.default_model = "gemini-2.5-flash";
  }
};

static const std::vector<Provider> kAllProviders = {
    Provider::OPENAI,
    Provider::ANTHROPIC,
    Provider::GEMINI
};

TEST_F(AIClientFactoryTest, CreatesOpenAIClient) {
  auto result = AIClientFactory::createClient(
      Provider::OPENAI, openai.api_key, &openai);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.model_name, openai.default_model);
  EXPECT_FALSE(result.model_name.empty());
}

TEST_F(AIClientFactoryTest, CreatesAnthropicClient) {
  auto result = AIClientFactory::createClient(
      Provider::ANTHROPIC, anthropic.api_key, &anthropic);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.model_name, anthropic.default_model);
}

TEST_F(AIClientFactoryTest, CreatesGeminiClient) {
  auto result = AIClientFactory::createClient(
      Provider::GEMINI, gemini.api_key, &gemini);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.model_name, gemini.default_model);
}

TEST_F(AIClientFactoryTest, AutoSelectsGeminiWhenOnlyGeminiKeyExists) {
  auto result = AIClientFactory::createClient(
      Provider::UNKNOWN, gemini.api_key, &gemini);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.model_name, gemini.default_model);
}

TEST_F(AIClientFactoryTest, ExplicitProviderSelectionOverridesAuto) {
  auto result = AIClientFactory::createClient(
      Provider::OPENAI, openai.api_key, &openai);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.model_name, openai.default_model);
}

TEST_F(AIClientFactoryTest, AutoSelectionRespectsProviderPriority) {
  auto result = AIClientFactory::createClient(
      Provider::UNKNOWN, openai.api_key, &openai);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.model_name, openai.default_model);
}

TEST_F(AIClientFactoryTest, ExplicitProviderSelectionOverridesAuto) {
  auto result = AIClientFactory::createClient(
      Provider::OPENAI, openai.api_key, &openai);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.model_name, openai.default_model);
}

TEST_F(AIClientFactoryTest, AutoSelectionRespectsProviderPriority) {
  auto result = AIClientFactory::createClient(
      Provider::UNKNOWN, openai.api_key, &openai);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.model_name, openai.default_model);
}

TEST_F(AIClientFactoryTest, UsesCustomOpenAIEndpoint) {
  openai.api_endpoint = "http://localhost:11434";

  auto result = AIClientFactory::createClient(
      Provider::OPENAI, openai.api_key, &openai);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.model_name, openai.default_model);
}

TEST_F(AIClientFactoryTest, UsesCustomAnthropicEndpoint) {
  anthropic.api_endpoint = "http://localhost:4000";

  auto result = AIClientFactory::createClient(
      Provider::ANTHROPIC, anthropic.api_key, &anthropic);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.model_name, anthropic.default_model);
}

TEST_F(AIClientFactoryTest, FailsWhenApiKeyMissingForAllProviders) {
  for (auto provider : kAllProviders) {
    ProviderConfig* cfg =
        provider == Provider::OPENAI   ? &openai :
        provider == Provider::ANTHROPIC ? &anthropic :
                                          &gemini;

    cfg->api_key.clear();

    auto result = AIClientFactory::createClient(
        provider, "", cfg);

    EXPECT_FALSE(result.success) << "Provider: " << static_cast<int>(provider);
    EXPECT_FALSE(result.error_message.empty());
  }
}

TEST_F(AIClientFactoryTest, FailsOnInvalidProvider) {
  auto invalid_provider = static_cast<Provider>(999);

  auto result = AIClientFactory::createClient(
      invalid_provider, "some-key", &openai);

  EXPECT_FALSE(result.success);
}

TEST_F(AIClientFactoryTest, FailsWithEmptyConfigurationForAllProviders) {
  ProviderConfig empty;

  for (auto provider : kAllProviders) {
    auto result = AIClientFactory::createClient(
        provider, "key", &empty);

    EXPECT_FALSE(result.success) << "Provider: " << static_cast<int>(provider);
  }
}