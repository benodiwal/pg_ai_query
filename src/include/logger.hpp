#pragma once

#include <string>

#ifdef USE_POSTGRESQL_ELOG
extern "C" {
#include <postgres.h>

#include <utils/elog.h>
}

#endif

namespace pg_ai::logger {

class Logger {
 public:
  static void debug(const std::string& message);
  static void info(const std::string& message);
  static void warning(const std::string& message);
  static void error(const std::string& message);

  static void setLoggingEnabled(bool enabled);
};

}  // namespace pg_ai::logger