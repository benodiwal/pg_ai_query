#include "../include/logger.hpp"
#include <iostream>
#include <algorithm>
#include <cctype>

namespace pg_ai::logger {

// Static flag for log level filtering
LogLevel Logger::current_level = LogLevel::LOG_INFO;
bool Logger::logging_enabled = true;

void Logger::set_level(LogLevel level) {
    current_level = level;
}

LogLevel Logger::get_level() {
    return current_level;
}

void Logger::set_level(const std::string& level_str) {
    std::string level = level_str;
    std::transform(level.begin(), level.end(), level.begin(), ::toupper);

    if (level == "DEBUG") {
        current_level = LogLevel::LOG_DEBUG;
    } else if (level == "INFO") {
        current_level = LogLevel::LOG_INFO;
    } else if (level == "WARNING") {
        current_level = LogLevel::LOG_WARNING;
    } else if (level == "ERROR") {
        current_level = LogLevel::LOG_ERROR;
    }
    // Invalid string → silently keep existing level
}

void Logger::log(LogLevel level, const std::string& prefix, const std::string& message) {
  if (!logging_enabled) { return; }
  if (level < current_level) { return; }

  #ifdef USE_POSTGRESQL_ELOG
    int pg_level = INFO;
    switch (level) {
      case LogLevel::LOG_DEBUG:
        pg_level = DEBUG1;
        break;
      case LogLevel::LOG_INFO:
        pg_level = INFO;
        break;
      case LogLevel::LOG_WARNING:
        pg_level = WARNING;
        break;
      case LogLevel::LOG_ERROR:
        pg_level = ERROR;
        break;
    }

    elog(pg_level, "%s %s", prefix.c_str(), message.c_str());
#else
    std::cerr << prefix << " " << message << std::endl;
#endif
}

// ---- Public logging APIs ----
void Logger::debug(const std::string& message) {
  log(LogLevel::LOG_DEBUG, "[DEBUG]", message);
}

void Logger::info(const std::string& message) {
  log(LogLevel::LOG_INFO, "[INFO]", message);
}

void Logger::warning(const std::string& message) {
  log(LogLevel::LOG_WARNING, "[WARNING]", message);
}

void Logger::error(const std::string& message) {
  log(LogLevel::LOG_ERROR, "[ERROR]", message);
}

void Logger::setLoggingEnabled(bool enabled) {
  logging_enabled = enabled;
}

}  // namespace pg_ai::logger