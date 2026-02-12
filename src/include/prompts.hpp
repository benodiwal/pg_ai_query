#pragma once

#include <string>

namespace pg_ai::prompts {

// Default system prompts (used when no custom prompt is configured)
extern const std::string SYSTEM_PROMPT;
extern const std::string EXPLAIN_SYSTEM_PROMPT;

/**
 * @brief Get the system prompt for query generation
 *
 * Returns the custom system prompt from configuration if available,
 * otherwise returns the default built-in SYSTEM_PROMPT.
 *
 * @return The system prompt to use for query generation
 */
std::string getSystemPrompt();

/**
 * @brief Get the system prompt for query explanation
 *
 * Returns the custom explain system prompt from configuration if available,
 * otherwise returns the default built-in EXPLAIN_SYSTEM_PROMPT.
 *
 * @return The system prompt to use for query explanation
 */
std::string getExplainSystemPrompt();

}  // namespace pg_ai::prompts