#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../test_helpers.hpp"
#include "include/config.hpp"
#include "include/prompts.hpp"

using namespace pg_ai::config;
using namespace pg_ai::prompts;
using namespace pg_ai::test_utils;

class PromptsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Reset config state before each test to ensure test isolation
    ConfigManager::reset();
  }
};

// Test that getSystemPrompt returns default when no custom prompt is configured
TEST_F(PromptsTest, GetSystemPromptReturnsDefaultWhenNotConfigured) {
  std::string config_path = getConfigFixture("default_prompts.ini");
  ASSERT_TRUE(ConfigManager::loadConfig(config_path));

  std::string prompt = getSystemPrompt();

  // Should return the built-in default prompt
  EXPECT_FALSE(prompt.empty());
  EXPECT_THAT(prompt, testing::HasSubstr("You are a senior PostgreSQL database analyst"));
  EXPECT_THAT(prompt, testing::HasSubstr("JSON only, no extra text"));
}

// Test that getExplainSystemPrompt returns default when not configured
TEST_F(PromptsTest, GetExplainSystemPromptReturnsDefaultWhenNotConfigured) {
  std::string config_path = getConfigFixture("default_prompts.ini");
  ASSERT_TRUE(ConfigManager::loadConfig(config_path));

  std::string prompt = getExplainSystemPrompt();

  // Should return the built-in default explain prompt
  EXPECT_FALSE(prompt.empty());
  EXPECT_THAT(prompt, testing::HasSubstr("PostgreSQL query performance expert"));
  EXPECT_THAT(prompt, testing::HasSubstr("EXPLAIN ANALYZE"));
}

// Test that getSystemPrompt returns custom prompt when configured
TEST_F(PromptsTest, GetSystemPromptReturnsCustomWhenConfigured) {
  std::string config_path = getConfigFixture("custom_prompts.ini");
  ASSERT_TRUE(ConfigManager::loadConfig(config_path));

  std::string prompt = getSystemPrompt();

  // Should return the custom prompt from config
  EXPECT_FALSE(prompt.empty());
  EXPECT_THAT(prompt, testing::HasSubstr("specialized PostgreSQL assistant"));
  EXPECT_THAT(prompt, testing::HasSubstr("data analytics"));
  EXPECT_THAT(prompt, testing::Not(testing::HasSubstr("senior PostgreSQL database analyst")));
}

// Test that getExplainSystemPrompt returns custom prompt when configured
TEST_F(PromptsTest, GetExplainSystemPromptReturnsCustomWhenConfigured) {
  std::string config_path = getConfigFixture("custom_prompts.ini");
  ASSERT_TRUE(ConfigManager::loadConfig(config_path));

  std::string prompt = getExplainSystemPrompt();

  // Should return the custom explain prompt from config
  EXPECT_FALSE(prompt.empty());
  EXPECT_THAT(prompt, testing::HasSubstr("optimization suggestions"));
  EXPECT_THAT(prompt, testing::HasSubstr("performance improvement"));
  EXPECT_THAT(prompt, testing::Not(testing::HasSubstr("query performance expert")));
}

// Test that multi-line custom prompts are parsed correctly
TEST_F(PromptsTest, GetSystemPromptHandlesMultilinePrompts) {
  std::string config_path = getConfigFixture("custom_prompts_multiline.ini");
  ASSERT_TRUE(ConfigManager::loadConfig(config_path));

  std::string prompt = getSystemPrompt();

  // Should contain all sections of the multi-line prompt
  EXPECT_FALSE(prompt.empty());
  EXPECT_THAT(prompt, testing::HasSubstr("expert SQL query generator"));
  EXPECT_THAT(prompt, testing::HasSubstr("table aliases"));
  EXPECT_THAT(prompt, testing::HasSubstr("JOIN syntax"));
  EXPECT_THAT(prompt, testing::HasSubstr("JSON format"));
}

// Test that multi-line custom explain prompts are parsed correctly
TEST_F(PromptsTest, GetExplainSystemPromptHandlesMultilinePrompts) {
  std::string config_path = getConfigFixture("custom_prompts_multiline.ini");
  ASSERT_TRUE(ConfigManager::loadConfig(config_path));

  std::string prompt = getExplainSystemPrompt();

  // Should contain all sections of the multi-line explain prompt
  EXPECT_FALSE(prompt.empty());
  EXPECT_THAT(prompt, testing::HasSubstr("PostgreSQL performance expert"));
  EXPECT_THAT(prompt, testing::HasSubstr("Scan efficiency"));
  EXPECT_THAT(prompt, testing::HasSubstr("Join strategies"));
  EXPECT_THAT(prompt, testing::HasSubstr("Optimization recommendations"));
}

// Test that prompts loaded from files work correctly
TEST_F(PromptsTest, GetSystemPromptLoadsFromFile) {
  // Create temporary prompt files
  std::string fixtures_dir = getFixturesPath();
  std::string prompts_dir = fixtures_dir + "/prompts";

  // Get absolute paths to prompt files
  std::string system_prompt_path = prompts_dir + "/custom_system_prompt.txt";

  // Create a temporary config with absolute paths
  TempConfigFile temp_config(R"(
[general]
log_level = INFO

[openai]
api_key = sk-test-files
)");
  // Append the prompts section with absolute path
  std::string config_content = std::string(temp_config.path()) + "\n[prompts]\n";
  config_content += "system_prompt = " + system_prompt_path + "\n";

  // Write the updated config
  std::ofstream out(std::filesystem::path(temp_config.path()), std::ios::app);
  out << "[prompts]\n";
  out << "system_prompt = " << system_prompt_path << "\n";
  out.close();

  ASSERT_TRUE(ConfigManager::loadConfig(temp_config.path()));

  std::string prompt = getSystemPrompt();

  // Should return the prompt loaded from file
  EXPECT_FALSE(prompt.empty());
  EXPECT_THAT(prompt, testing::HasSubstr("loaded from file"));
  EXPECT_THAT(prompt, testing::HasSubstr("data warehousing"));
  EXPECT_THAT(prompt, testing::HasSubstr("window functions"));
  EXPECT_THAT(prompt, testing::HasSubstr("materialized views"));
}

// Test that explain prompts loaded from files work correctly
TEST_F(PromptsTest, GetExplainSystemPromptLoadsFromFile) {
  // Create temporary prompt files
  std::string fixtures_dir = getFixturesPath();
  std::string prompts_dir = fixtures_dir + "/prompts";

  // Get absolute paths to prompt files
  std::string explain_prompt_path = prompts_dir + "/custom_explain_prompt.txt";

  // Create a temporary config with absolute paths
  TempConfigFile temp_config(R"(
[general]
log_level = INFO

[openai]
api_key = sk-test-files
)");

  // Append the prompts section with absolute path
  std::ofstream out(std::filesystem::path(temp_config.path()), std::ios::app);
  out << "[prompts]\n";
  out << "explain_system_prompt = " << explain_prompt_path << "\n";
  out.close();

  ASSERT_TRUE(ConfigManager::loadConfig(temp_config.path()));

  std::string prompt = getExplainSystemPrompt();

  // Should return the explain prompt loaded from file
  EXPECT_FALSE(prompt.empty());
  EXPECT_THAT(prompt, testing::HasSubstr("loaded from file"));
  EXPECT_THAT(prompt, testing::HasSubstr("Sequential scan"));
  EXPECT_THAT(prompt, testing::HasSubstr("Join algorithm"));
  EXPECT_THAT(prompt, testing::HasSubstr("Buffer cache hit rates"));
}

// Test that default SYSTEM_PROMPT constant is accessible
TEST(PromptsConstantsTest, SystemPromptConstantIsAccessible) {
  EXPECT_FALSE(SYSTEM_PROMPT.empty());
  EXPECT_THAT(SYSTEM_PROMPT, testing::HasSubstr("PostgreSQL database analyst"));
  EXPECT_THAT(SYSTEM_PROMPT, testing::HasSubstr("JSON only"));
}

// Test that default EXPLAIN_SYSTEM_PROMPT constant is accessible
TEST(PromptsConstantsTest, ExplainSystemPromptConstantIsAccessible) {
  EXPECT_FALSE(EXPLAIN_SYSTEM_PROMPT.empty());
  EXPECT_THAT(EXPLAIN_SYSTEM_PROMPT, testing::HasSubstr("performance expert"));
  EXPECT_THAT(EXPLAIN_SYSTEM_PROMPT, testing::HasSubstr("EXPLAIN ANALYZE"));
}

// Test Configuration default constructor initializes prompt fields as empty
TEST(ConfigurationTest, DefaultConstructorInitializesPromptsAsEmpty) {
  Configuration config;

  EXPECT_TRUE(config.system_prompt.empty());
  EXPECT_TRUE(config.explain_system_prompt.empty());
}

// Test that custom prompts are correctly stored in Configuration
TEST_F(PromptsTest, CustomPromptsAreStoredInConfiguration) {
  std::string config_path = getConfigFixture("custom_prompts.ini");
  ASSERT_TRUE(ConfigManager::loadConfig(config_path));

  const auto& config = ConfigManager::getConfig();

  // Check that custom prompts are stored
  EXPECT_FALSE(config.system_prompt.empty());
  EXPECT_FALSE(config.explain_system_prompt.empty());
  EXPECT_THAT(config.system_prompt, testing::HasSubstr("specialized PostgreSQL assistant"));
  EXPECT_THAT(config.explain_system_prompt, testing::HasSubstr("optimization suggestions"));
}

// Test that empty prompt configuration falls back to defaults
TEST_F(PromptsTest, EmptyPromptConfigurationFallsBackToDefault) {
  TempConfigFile temp_config(R"(
[general]
log_level = INFO

[openai]
api_key = sk-test

[prompts]
# Explicitly empty - should use defaults
system_prompt = ""
explain_system_prompt = ""
)");

  ASSERT_TRUE(ConfigManager::loadConfig(temp_config.path()));

  const auto& config = ConfigManager::getConfig();

  // Empty strings should be stored
  EXPECT_TRUE(config.system_prompt.empty());
  EXPECT_TRUE(config.explain_system_prompt.empty());

  // But getters should return defaults
  std::string system_prompt = getSystemPrompt();
  std::string explain_prompt = getExplainSystemPrompt();

  EXPECT_FALSE(system_prompt.empty());
  EXPECT_FALSE(explain_prompt.empty());
  EXPECT_THAT(system_prompt, testing::HasSubstr("PostgreSQL database analyst"));
  EXPECT_THAT(explain_prompt, testing::HasSubstr("performance expert"));
}

// Test that only system_prompt can be customized independently
TEST_F(PromptsTest, CanCustomizeOnlySystemPrompt) {
  TempConfigFile temp_config(R"(
[general]
log_level = INFO

[openai]
api_key = sk-test

[prompts]
system_prompt = "Custom system prompt only"
# explain_system_prompt not set - should use default
)");

  ASSERT_TRUE(ConfigManager::loadConfig(temp_config.path()));

  const auto& config = ConfigManager::getConfig();

  EXPECT_EQ(config.system_prompt, "Custom system prompt only");
  EXPECT_TRUE(config.explain_system_prompt.empty());

  // getSystemPrompt should return custom
  std::string system_prompt = getSystemPrompt();
  EXPECT_EQ(system_prompt, "Custom system prompt only");

  // getExplainSystemPrompt should return default
  std::string explain_prompt = getExplainSystemPrompt();
  EXPECT_FALSE(explain_prompt.empty());
  EXPECT_THAT(explain_prompt, testing::HasSubstr("performance expert"));
}

// Test that only explain_system_prompt can be customized independently
TEST_F(PromptsTest, CanCustomizeOnlyExplainSystemPrompt) {
  TempConfigFile temp_config(R"(
[general]
log_level = INFO

[openai]
api_key = sk-test

[prompts]
# system_prompt not set - should use default
explain_system_prompt = "Custom explain prompt only"
)");

  ASSERT_TRUE(ConfigManager::loadConfig(temp_config.path()));

  const auto& config = ConfigManager::getConfig();

  EXPECT_TRUE(config.system_prompt.empty());
  EXPECT_EQ(config.explain_system_prompt, "Custom explain prompt only");

  // getSystemPrompt should return default
  std::string system_prompt = getSystemPrompt();
  EXPECT_FALSE(system_prompt.empty());
  EXPECT_THAT(system_prompt, testing::HasSubstr("PostgreSQL database analyst"));

  // getExplainSystemPrompt should return custom
  std::string explain_prompt = getExplainSystemPrompt();
  EXPECT_EQ(explain_prompt, "Custom explain prompt only");
}
