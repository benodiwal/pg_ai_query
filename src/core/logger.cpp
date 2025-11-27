#include "../include/logger.hpp"
#include <iostream>

namespace pg_ai::logger {

// Static flag to control logging, set by config manager
static bool logging_enabled = false;

void Logger::setLoggingEnabled(bool enabled) {
  logging_enabled = enabled;
}

void Logger::debug(const std::string& message) {
  if (!logging_enabled)
    return;
#ifdef USE_POSTGRESQL_ELOG
  ereport(DEBUG1, (errmsg("[pg_ai_query] %s", message.c_str())));
#else
  std::cerr << "[DEBUG] [pg_ai_query] " << message << std::endl;
#endif
}

void Logger::info(const std::string& message) {
  if (!logging_enabled)
    return;
#ifdef USE_POSTGRESQL_ELOG
  ereport(INFO, (errmsg("[pg_ai_query] %s", message.c_str())));
#else
  std::cerr << "[INFO] [pg_ai_query] " << message << std::endl;
#endif
}

void Logger::warning(const std::string& message) {
  if (!logging_enabled)
    return;
#ifdef USE_POSTGRESQL_ELOG
  ereport(WARNING, (errmsg("[pg_ai_query] %s", message.c_str())));
#else
  std::cerr << "[WARNING] [pg_ai_query] " << message << std::endl;
#endif
}

void Logger::error(const std::string& message) {
  if (!logging_enabled)
    return;
#ifdef USE_POSTGRESQL_ELOG
  ereport(LOG, (errmsg("[pg_ai_query] ERROR: %s", message.c_str())));
#else
  std::cerr << "[ERROR] [pg_ai_query] " << message << std::endl;
#endif
}

}  // namespace pg_ai::logger