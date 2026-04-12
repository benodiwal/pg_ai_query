#include "metadata.hpp"

using json = nlohmann::json;

json extractSources(const std::string& text) {
  json sources = json::array();

  if (text.find("ST_") != std::string::npos) {
    sources.push_back({
        {"title", "PostGIS Documentation"},
        {"url", "https://postgis.net/docs/"}});
  }

  if (text.find("SELECT") != std::string::npos) {
    sources.push_back({
        {"title", "PostgreSQL SELECT"},
        {"url", "https://www.postgresql.org/docs/current/sql-select.html"}});
  }

  return sources;
}

double calculateConfidence(const std::string& text) {
  if (text.length() > 150) return 0.9;
  if (text.length() > 50) return 0.75;
  return 0.6;
}

json extractTags(const std::string& text) {
  json tags = json::array();

  if (text.find("ST_") != std::string::npos)
    tags.push_back("postgis");

  if (text.find("JOIN") != std::string::npos)
    tags.push_back("join");

  if (text.find("INDEX") != std::string::npos)
    tags.push_back("performance");

  return tags;
}