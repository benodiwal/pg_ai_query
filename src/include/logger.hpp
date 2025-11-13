#pragma once

#include <string>

extern "C" {
#include <postgres.h>
#include <utils/elog.h>
}

namespace pg_ai::logger {

class Logger {
   public:
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);
};

}  // namespace pg_ai::logger