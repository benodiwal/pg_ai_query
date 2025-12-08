#pragma once

#include <filesystem>
#include <fstream>
#include <string>

namespace pg_ai::test_utils {

inline std::string getFixturesPath() {
#ifdef TEST_FIXTURES_PATH
  return TEST_FIXTURES_PATH;
#else
  return "./fixtures";
#endif
}

inline std::string getConfigFixture(const std::string& filename) {
  return getFixturesPath() + "/configs/" + filename;
}

inline std::string getResponseFixture(const std::string& filename) {
  return getFixturesPath() + "/responses/" + filename;
}

inline std::string readTestFile(const std::string& path) {
  std::ifstream file(path);
  if (!file) {
    return "";
  }
  return std::string(std::istreambuf_iterator<char>(file),
                     std::istreambuf_iterator<char>());
}

class TempConfigFile {
 public:
  TempConfigFile(const std::string& content) {
    path_ = std::filesystem::temp_directory_path() /
            ("pg_ai_test_config_" + std::to_string(rand()) + ".ini");
    std::ofstream file(path_);
    file << content;
  }

  ~TempConfigFile() { std::filesystem::remove(path_); }

  std::string path() const { return path_.string(); }

 private:
  std::filesystem::path path_;
};

}  // namespace pg_ai::test_utils
