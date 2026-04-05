#pragma once
#include <string>
#include <nlohmann/json.hpp>

nlohmann::json extractSources(const std::string& text);
double calculateConfidence(const std::string& text);
nlohmann::json extractTags(const std::string& text);