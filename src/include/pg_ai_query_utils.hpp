#pragma once

#include <string>

extern "C" {
#include <postgres.h>
}

namespace pg_ai::utils {

/**
 * @brief Convert PostgreSQL text* to std::string, freeing the palloc'd buffer.
 *
 * text_to_cstring() returns memory that the caller must free with pfree().
 * This helper copies to std::string and frees the buffer to avoid leaks.
 *
 * @param t PostgreSQL text* (may be NULL)
 * @return std::string contents, or "" if t is NULL
 */
std::string pg_text_to_string(const text* t);

}  // namespace pg_ai::utils
