#pragma once

#include <string>

namespace pg_ai::prompts {
extern const std::string EXPLAIN_SYSTEM_PROMPT;
std::string getSystemPrompt(bool enforce_limit, int default_limit);
}  // namespace pg_ai::prompts