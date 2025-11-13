#include "../include/logger.hpp"

namespace pg_ai::logger {

void Logger::debug(const std::string& message) {
    ereport(DEBUG1, (errmsg("[pg_ai_query] %s", message.c_str())));
}

void Logger::info(const std::string& message) {
    ereport(INFO, (errmsg("[pg_ai_query] %s", message.c_str())));
}

void Logger::warning(const std::string& message) {
    ereport(WARNING, (errmsg("[pg_ai_query] %s", message.c_str())));
}

void Logger::error(const std::string& message) {
    ereport(LOG, (errmsg("[pg_ai_query] ERROR: %s", message.c_str())));
}

}  // namespace pg_ai::logger