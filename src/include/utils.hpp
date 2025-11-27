#pragma once
#include <string>
#include <utility>

namespace pg_ai::utils {

std::pair<bool, std::string> read_file(const std::string& filepath);

std::string read_file_or_throw(const std::string& filepath);

std::string formatAPIError(const std::string& raw_error);

}  // namespace pg_ai::utils