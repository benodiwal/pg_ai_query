#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "../test_helpers.hpp"
#include "include/config.hpp"
#include "include/constants.hpp"

using namespace pg_ai::config;
using namespace pg_ai::constants;
using namespace pg_ai::test_utils;

class ConfigManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Reset config state before each test to ensure test isolation
    ConfigManager::reset();
  }
};

// Test loading a valid complete configuration
TEST_F(ConfigManagerTest, LoadsValidCompleteConfig) {
  std::string config_path = getConfigFixture("valid_config.ini");
  ASSERT_TRUE(ConfigManager::loadConfig(config_path));

  const auto& config = ConfigManager::getConfig();

  // Check general settings
  EXPECT_EQ(config.log_level, "DEBUG");
  EXPECT_TRUE(config.enable_logging);
  EXPECT_EQ(config.request_timeout_ms, 60000);
  EXPECT_EQ(config.max_retries, 5);

  // Check query settings
  EXPECT_TRUE(config.enforce_limit);
  EXPECT_EQ(config.default_limit, 500);
  EXPECT_EQ(config.max_query_length, 2000);

  // Check response settings
  EXPECT_TRUE(config.show_explanation);
  EXPECT_TRUE(config.show_warnings);
  EXPECT_TRUE(config.show_suggested_visualization);
  EXPECT_FALSE(config.use_formatted_response);
}

// Test loading minimal configuration
TEST_F(ConfigManagerTest, LoadsMinimalConfig) {
  std::string config_path = getConfigFixture("minimal_config.ini");
  ASSERT_TRUE(ConfigManager::loadConfig(config_path));

  const auto& config = ConfigManager::getConfig();

  // Check that defaults are used for missing values
  EXPECT_EQ(config.log_level, "INFO");          // default
  EXPECT_FALSE(config.enable_logging);          // default
  EXPECT_EQ(config.request_timeout_ms, 30000);  // default
  EXPECT_EQ(config.max_retries, 3);             // default
  EXPECT_EQ(config.max_query_length, 4000);     // default

  // OpenAI key should be set
  const auto* openai = ConfigManager::getProviderConfig(Provider::OPENAI);
  ASSERT_NE(openai, nullptr);
  EXPECT_EQ(openai->api_key, "sk-minimal-test-key");
}

// Test loading configuration with only Anthropic
TEST_F(ConfigManagerTest, LoadsAnthropicOnlyConfig) {
  std::string config_path = getConfigFixture("anthropic_only.ini");
  ASSERT_TRUE(ConfigManager::loadConfig(config_path));

  const auto* anthropic = ConfigManager::getProviderConfig(Provider::ANTHROPIC);
  ASSERT_NE(anthropic, nullptr);
  EXPECT_EQ(anthropic->api_key, "sk-ant-only-key");
  EXPECT_EQ(anthropic->default_model, "claude-sonnet-4-5-20250929");
}

// Test loading empty configuration (no API keys)
TEST_F(ConfigManagerTest, LoadsEmptyConfig) {
  std::string config_path = getConfigFixture("empty_config.ini");
  ASSERT_TRUE(ConfigManager::loadConfig(config_path));

  const auto& config = ConfigManager::getConfig();

  EXPECT_EQ(config.log_level, "INFO");
  EXPECT_FALSE(config.enable_logging);
  EXPECT_FALSE(config.enforce_limit);
  EXPECT_FALSE(config.show_explanation);
}

// Test loading configuration with custom endpoints
TEST_F(ConfigManagerTest, LoadsCustomEndpoints) {
  std::string config_path = getConfigFixture("custom_endpoints.ini");
  ASSERT_TRUE(ConfigManager::loadConfig(config_path));

  const auto* openai = ConfigManager::getProviderConfig(Provider::OPENAI);
  ASSERT_NE(openai, nullptr);
  EXPECT_EQ(openai->api_endpoint, "https://custom-openai.example.com/v1");

  const auto* anthropic = ConfigManager::getProviderConfig(Provider::ANTHROPIC);
  ASSERT_NE(anthropic, nullptr);
  EXPECT_EQ(anthropic->api_endpoint, "https://custom-anthropic.example.com");
}

// Test loading non-existent file fails
TEST_F(ConfigManagerTest, ThrowsForNonexistentFile) {
  EXPECT_THROW(ConfigManager::loadConfig("/nonexistent/path/config.ini"),
               std::runtime_error);
}

// Test provider enum to string conversion
TEST_F(ConfigManagerTest, ProviderToString) {
  EXPECT_EQ(ConfigManager::providerToString(Provider::OPENAI), "openai");
  EXPECT_EQ(ConfigManager::providerToString(Provider::ANTHROPIC), "anthropic");
  EXPECT_EQ(ConfigManager::providerToString(Provider::GEMINI), "gemini");
  EXPECT_EQ(ConfigManager::providerToString(Provider::UNKNOWN), "unknown");
}

// Test string to provider enum conversion
TEST_F(ConfigManagerTest, StringToProvider) {
  EXPECT_EQ(ConfigManager::stringToProvider("openai"), Provider::OPENAI);
  EXPECT_EQ(ConfigManager::stringToProvider("OPENAI"), Provider::OPENAI);
  EXPECT_EQ(ConfigManager::stringToProvider("OpenAI"), Provider::OPENAI);

  EXPECT_EQ(ConfigManager::stringToProvider("anthropic"), Provider::ANTHROPIC);
  EXPECT_EQ(ConfigManager::stringToProvider("ANTHROPIC"), Provider::ANTHROPIC);
  EXPECT_EQ(ConfigManager::stringToProvider("Anthropic"), Provider::ANTHROPIC);

  EXPECT_EQ(ConfigManager::stringToProvider("gemini"), Provider::GEMINI);
  EXPECT_EQ(ConfigManager::stringToProvider("GEMINI"), Provider::GEMINI);
  EXPECT_EQ(ConfigManager::stringToProvider("Gemini"), Provider::GEMINI);

  EXPECT_EQ(ConfigManager::stringToProvider("invalid"), Provider::UNKNOWN);
  EXPECT_EQ(ConfigManager::stringToProvider(""), Provider::UNKNOWN);
}

// Test getProviderConfig returns nullptr for unconfigured provider
TEST_F(ConfigManagerTest, GetProviderConfigReturnsNullForUnconfigured) {
  std::string config_path = getConfigFixture("minimal_config.ini");
  ASSERT_TRUE(ConfigManager::loadConfig(config_path));

  // Anthropic not configured in minimal_config
  const auto* anthropic = ConfigManager::getProviderConfig(Provider::ANTHROPIC);
  EXPECT_EQ(anthropic, nullptr);
}

// Test parsing config with inline values (whitespace trimming)
TEST_F(ConfigManagerTest, ParsesConfigWithWhitespace) {
  TempConfigFile temp_config(R"(
[general]
  log_level   =   WARNING
  enable_logging=true

[openai]
api_key =   "  sk-with-spaces  "
)");

  ASSERT_TRUE(ConfigManager::loadConfig(temp_config.path()));

  const auto& config = ConfigManager::getConfig();
  EXPECT_EQ(config.log_level, "WARNING");
  EXPECT_TRUE(config.enable_logging);

  const auto* openai = ConfigManager::getProviderConfig(Provider::OPENAI);
  ASSERT_NE(openai, nullptr);
  // Quoted value should have internal spaces preserved, outer quotes removed
  EXPECT_EQ(openai->api_key, "  sk-with-spaces  ");
}

// Test parsing config with comments
TEST_F(ConfigManagerTest, IgnoresComments) {
  TempConfigFile temp_config(R"(
# This is a comment
[general]
# Another comment
log_level = ERROR
# enable_logging = true  <- this is commented out

[openai]
api_key = sk-test  # inline comment should be stripped
)");

  ASSERT_TRUE(ConfigManager::loadConfig(temp_config.path()));

  const auto& config = ConfigManager::getConfig();
  EXPECT_EQ(config.log_level, "ERROR");
  // Default value since the commented line is ignored
  EXPECT_FALSE(config.enable_logging);

  // Inline comments should now be stripped from unquoted values
  const auto* openai = ConfigManager::getProviderConfig(Provider::OPENAI);
  ASSERT_NE(openai, nullptr);
  EXPECT_EQ(openai->api_key, "sk-test");
}

// Test parsing config with inline comments after quoted values (Ollama use
// case)
TEST_F(ConfigManagerTest, HandlesInlineCommentsAfterQuotedValues) {
  TempConfigFile temp_config(R"(
[openai]
api_key = "ollama" # Ollama doesn't require a real key
api_endpoint = "http://localhost:11434"
default_model = "gpt-oss:20b" # Or any model you have pulled
)");

  ASSERT_TRUE(ConfigManager::loadConfig(temp_config.path()));

  const auto* openai = ConfigManager::getProviderConfig(Provider::OPENAI);
  ASSERT_NE(openai, nullptr);
  // Inline comments after quoted values should be ignored
  EXPECT_EQ(openai->api_key, "ollama");
  EXPECT_EQ(openai->api_endpoint, "http://localhost:11434");
  EXPECT_EQ(openai->default_model, "gpt-oss:20b");
}

// Test default model values
TEST_F(ConfigManagerTest, DefaultModelValues) {
  TempConfigFile temp_config(R"(
[openai]
api_key = sk-test

[anthropic]
api_key = sk-ant-test
)");

  ASSERT_TRUE(ConfigManager::loadConfig(temp_config.path()));

  const auto* openai = ConfigManager::getProviderConfig(Provider::OPENAI);
  ASSERT_NE(openai, nullptr);
  EXPECT_EQ(openai->default_model, DEFAULT_OPENAI_MODEL);

  const auto* anthropic = ConfigManager::getProviderConfig(Provider::ANTHROPIC);
  ASSERT_NE(anthropic, nullptr);
  EXPECT_EQ(anthropic->default_model,
            DEFAULT_ANTHROPIC_MODEL);
}

// Test numeric value parsing
TEST_F(ConfigManagerTest, ParsesNumericValues) {
  TempConfigFile temp_config(R"(
[general]
request_timeout_ms = 120000
max_retries = 10

[query]
default_limit = 2500
max_query_length = 8000

[openai]
api_key = sk-test
max_tokens = 16000
temperature = 0.85
)");

  ASSERT_TRUE(ConfigManager::loadConfig(temp_config.path()));

  const auto& config = ConfigManager::getConfig();
  EXPECT_EQ(config.request_timeout_ms, 120000);
  EXPECT_EQ(config.max_retries, 10);
  EXPECT_EQ(config.default_limit, 2500);
  EXPECT_EQ(config.max_query_length, 8000);

  const auto* openai = ConfigManager::getProviderConfig(Provider::OPENAI);
  ASSERT_NE(openai, nullptr);
  EXPECT_EQ(openai->default_max_tokens, 16000);
  EXPECT_DOUBLE_EQ(openai->default_temperature, 0.85);
}

// Test boolean value parsing
TEST_F(ConfigManagerTest, ParsesBooleanValues) {
  TempConfigFile temp_config(R"(
[general]
enable_logging = true

[query]
enforce_limit = false

[response]
show_explanation = true
show_warnings = false
show_suggested_visualization = true
use_formatted_response = true
)");

  ASSERT_TRUE(ConfigManager::loadConfig(temp_config.path()));

  const auto& config = ConfigManager::getConfig();
  EXPECT_TRUE(config.enable_logging);
  EXPECT_FALSE(config.enforce_limit);
  EXPECT_TRUE(config.show_explanation);
  EXPECT_FALSE(config.show_warnings);
  EXPECT_TRUE(config.show_suggested_visualization);
  EXPECT_TRUE(config.use_formatted_response);
}

// Test Configuration default constructor
TEST(ConfigurationTest, DefaultConstructorSetsDefaults) {
  Configuration config;

  EXPECT_EQ(config.log_level, "INFO");
  EXPECT_FALSE(config.enable_logging);
  EXPECT_EQ(config.request_timeout_ms, 30000);
  EXPECT_EQ(config.max_retries, 3);
  EXPECT_TRUE(config.enforce_limit);
  EXPECT_EQ(config.default_limit, 1000);
  EXPECT_EQ(config.max_query_length, 4000);
  EXPECT_TRUE(config.show_explanation);
  EXPECT_TRUE(config.show_warnings);
  EXPECT_FALSE(config.show_suggested_visualization);
  EXPECT_FALSE(config.use_formatted_response);

  EXPECT_EQ(config.default_provider.provider, Provider::OPENAI);
  EXPECT_EQ(config.default_provider.default_model,
            DEFAULT_OPENAI_MODEL);
}

// Test ProviderConfig default constructor
TEST(ProviderConfigTest, DefaultConstructorSetsDefaults) {
  ProviderConfig config;

  EXPECT_EQ(config.provider, Provider::UNKNOWN);
  EXPECT_TRUE(config.api_key.empty());
  EXPECT_TRUE(config.default_model.empty());
  EXPECT_EQ(config.default_max_tokens, 4096);
  EXPECT_DOUBLE_EQ(config.default_temperature, 0.7);
  EXPECT_TRUE(config.api_endpoint.empty());
}

// Test loading configuration with only Gemini
TEST_F(ConfigManagerTest, LoadsGeminiOnlyConfig) {
  std::string config_path = getConfigFixture("gemini_only.ini");
  ASSERT_TRUE(ConfigManager::loadConfig(config_path));

  const auto* gemini = ConfigManager::getProviderConfig(Provider::GEMINI);
  ASSERT_NE(gemini, nullptr);
  EXPECT_EQ(gemini->api_key, "AIzaSyTest-gemini-key-12345");
  EXPECT_EQ(gemini->default_model, "gemini-2.5-flash");
  EXPECT_EQ(gemini->default_max_tokens, 8192);
  EXPECT_DOUBLE_EQ(gemini->default_temperature, 0.7);
}

// Test that valid config loads all three providers
TEST_F(ConfigManagerTest, LoadsAllThreeProviders) {
  std::string config_path = getConfigFixture("valid_config.ini");
  ASSERT_TRUE(ConfigManager::loadConfig(config_path));

  const auto* openai = ConfigManager::getProviderConfig(Provider::OPENAI);
  const auto* anthropic = ConfigManager::getProviderConfig(Provider::ANTHROPIC);
  const auto* gemini = ConfigManager::getProviderConfig(Provider::GEMINI);

  ASSERT_NE(openai, nullptr);
  ASSERT_NE(anthropic, nullptr);
  ASSERT_NE(gemini, nullptr);

  EXPECT_EQ(openai->api_key, "sk-test-openai-key-12345");
  EXPECT_EQ(anthropic->api_key, "sk-ant-test-key-67890");
  EXPECT_EQ(gemini->api_key, "AIzaSyTest-gemini-key-valid");
}

// ---------------------------------------------------------------------------
// GUC variable override tests
// ---------------------------------------------------------------------------

// A GUC key overrides the api_key that was loaded from the config file.
TEST_F(ConfigManagerTest, GucOverrideAppliesOpenAIKey) {
  TempConfigFile cfg("[openai]\napi_key = sk-from-file\n");
  ASSERT_TRUE(ConfigManager::loadConfig(cfg.path()));

  ConfigManager::applyGucOverrides("sk-from-guc", nullptr, nullptr);

  const auto* openai = ConfigManager::getProviderConfig(Provider::OPENAI);
  ASSERT_NE(openai, nullptr);
  EXPECT_EQ(openai->api_key, "sk-from-guc");
}

// After SET then RESET (nullptr), the config file value must be restored.
TEST_F(ConfigManagerTest, GucResetRevertsToConfigFileValue) {
  TempConfigFile cfg("[openai]\napi_key = sk-from-file\n");
  ASSERT_TRUE(ConfigManager::loadConfig(cfg.path()));

  ConfigManager::applyGucOverrides("sk-from-guc", nullptr, nullptr);
  ASSERT_EQ(ConfigManager::getProviderConfig(Provider::OPENAI)->api_key,
            "sk-from-guc");

  // Simulate RESET pg_ai.openai_api_key (PostgreSQL passes nullptr).
  ConfigManager::applyGucOverrides(nullptr, nullptr, nullptr);

  const auto* openai = ConfigManager::getProviderConfig(Provider::OPENAI);
  ASSERT_NE(openai, nullptr);
  EXPECT_EQ(openai->api_key, "sk-from-file");
}

// An explicitly empty string GUC value is treated identically to nullptr:
// the config file key wins.
TEST_F(ConfigManagerTest, GucEmptyStringTreatedAsNoOverride) {
  TempConfigFile cfg("[openai]\napi_key = sk-from-file\n");
  ASSERT_TRUE(ConfigManager::loadConfig(cfg.path()));

  ConfigManager::applyGucOverrides("", nullptr, nullptr);

  const auto* openai = ConfigManager::getProviderConfig(Provider::OPENAI);
  ASSERT_NE(openai, nullptr);
  EXPECT_EQ(openai->api_key, "sk-from-file");
}

// When a provider has no entry in the config file, a GUC key for that
// provider creates a new entry with sensible defaults.
TEST_F(ConfigManagerTest, GucCreatesProviderEntryWhenNotInFile) {
  TempConfigFile cfg("[openai]\napi_key = sk-openai-only\n");
  ASSERT_TRUE(ConfigManager::loadConfig(cfg.path()));

  // Anthropic is absent from the config file; GUC provides its key.
  ConfigManager::applyGucOverrides(nullptr, "sk-ant-from-guc", nullptr);

  const auto* anthropic = ConfigManager::getProviderConfig(Provider::ANTHROPIC);
  ASSERT_NE(anthropic, nullptr);
  EXPECT_EQ(anthropic->api_key, "sk-ant-from-guc");
  EXPECT_FALSE(anthropic->default_model.empty());

  // Existing file entry must be intact.
  const auto* openai = ConfigManager::getProviderConfig(Provider::OPENAI);
  ASSERT_NE(openai, nullptr);
  EXPECT_EQ(openai->api_key, "sk-openai-only");
}

// A GUC-created provider entry is removed when the GUC is reset, because
// applyGucOverrides() rebuilds config_ from base_config_ (which has no entry).
TEST_F(ConfigManagerTest, GucCreatesProviderThenResetRemovesIt) {
  TempConfigFile cfg("[openai]\napi_key = sk-openai-only\n");
  ASSERT_TRUE(ConfigManager::loadConfig(cfg.path()));

  ConfigManager::applyGucOverrides(nullptr, nullptr, "AIza-gemini-guc");
  ASSERT_NE(ConfigManager::getProviderConfig(Provider::GEMINI), nullptr);

  // RESET Gemini GUC — entry should disappear.
  ConfigManager::applyGucOverrides(nullptr, nullptr, nullptr);
  EXPECT_EQ(ConfigManager::getProviderConfig(Provider::GEMINI), nullptr);
}

// Calling applyGucOverrides() multiple times with different values must
// always reflect the latest call and never accumulate stale state.
TEST_F(ConfigManagerTest, RepeatedGucAppliesAreIdempotentOnBase) {
  TempConfigFile cfg("[openai]\napi_key = sk-file-key\n");
  ASSERT_TRUE(ConfigManager::loadConfig(cfg.path()));

  ConfigManager::applyGucOverrides("sk-guc-a", nullptr, nullptr);
  ConfigManager::applyGucOverrides("sk-guc-b", nullptr, nullptr);
  ConfigManager::applyGucOverrides(nullptr, nullptr, nullptr);  // RESET

  const auto* openai = ConfigManager::getProviderConfig(Provider::OPENAI);
  ASSERT_NE(openai, nullptr);
  EXPECT_EQ(openai->api_key, "sk-file-key");
}

// All three providers can be overridden in a single call.
TEST_F(ConfigManagerTest, GucOverridesAllThreeProviders) {
  std::string config_path = getConfigFixture("valid_config.ini");
  ASSERT_TRUE(ConfigManager::loadConfig(config_path));

  ConfigManager::applyGucOverrides(
      "sk-openai-guc", "sk-ant-guc", "AIza-gemini-guc");

  const auto* openai    = ConfigManager::getProviderConfig(Provider::OPENAI);
  const auto* anthropic = ConfigManager::getProviderConfig(Provider::ANTHROPIC);
  const auto* gemini    = ConfigManager::getProviderConfig(Provider::GEMINI);

  ASSERT_NE(openai, nullptr);
  ASSERT_NE(anthropic, nullptr);
  ASSERT_NE(gemini, nullptr);
  EXPECT_EQ(openai->api_key,    "sk-openai-guc");
  EXPECT_EQ(anthropic->api_key, "sk-ant-guc");
  EXPECT_EQ(gemini->api_key,    "AIza-gemini-guc");
}
