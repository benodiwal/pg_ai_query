#pragma once

#include <string>

#ifdef USE_POSTGRESQL_ELOG
extern "C" {
#include <postgres.h>

#include <utils/elog.h>
}

#endif

namespace pg_ai::logger {

  enum class LogLevel {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARNING = 2,
    LOG_ERROR = 3
  };

  class Logger {
  public:
    static void set_level(LogLevel level);
    static void set_level(const std::string& level_str);
    static LogLevel get_level();

    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);

    static void setLoggingEnabled(bool enabled);

  private:
    static LogLevel current_level;
    static bool logging_enabled;
    static void log(LogLevel level, const std::string& prefix, const std::string& message);
  };

}  // namespace pg_ai::logger