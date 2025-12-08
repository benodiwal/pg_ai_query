#include <filesystem>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../test_helpers.hpp"
#include "include/utils.hpp"

using namespace pg_ai::utils;
using namespace pg_ai::test_utils;

class UtilsTest : public ::testing::Test {
 protected:
  std::filesystem::path temp_dir_;

  void SetUp() override {
    temp_dir_ = std::filesystem::temp_directory_path() / "pg_ai_utils_test";
    std::filesystem::create_directories(temp_dir_);
  }

  void TearDown() override { std::filesystem::remove_all(temp_dir_); }

  std::string createTempFile(const std::string& filename,
                             const std::string& content) {
    auto path = temp_dir_ / filename;
    std::ofstream file(path);
    file << content;
    file.close();
    return path.string();
  }
};

// Test read_file with valid file
TEST_F(UtilsTest, ReadFileValid) {
  std::string content = "Hello, World!\nLine 2\n";
  std::string path = createTempFile("test.txt", content);

  auto [success, read_content] = read_file(path);

  EXPECT_TRUE(success);
  EXPECT_EQ(read_content, content);
}

// Test read_file with empty file
TEST_F(UtilsTest, ReadFileEmpty) {
  std::string path = createTempFile("empty.txt", "");

  auto [success, content] = read_file(path);

  EXPECT_TRUE(success);
  EXPECT_TRUE(content.empty());
}

// Test read_file with non-existent file
TEST_F(UtilsTest, ReadFileNonExistent) {
  auto [success, content] = read_file("/nonexistent/path/file.txt");

  EXPECT_FALSE(success);
  EXPECT_TRUE(content.empty());
}

// Test read_file with binary content
TEST_F(UtilsTest, ReadFileBinary) {
  std::string binary_content = "test\x00with\x00nulls";
  auto path = temp_dir_ / "binary.bin";
  std::ofstream file(path, std::ios::binary);
  file.write(binary_content.c_str(), binary_content.size());
  file.close();

  auto [success, content] = read_file(path.string());

  EXPECT_TRUE(success);
  // Note: string comparison may stop at null bytes, so check size
  EXPECT_EQ(content.size(), binary_content.size());
}

// Test read_file_or_throw with valid file
TEST_F(UtilsTest, ReadFileOrThrowValid) {
  std::string expected = "Test content";
  std::string path = createTempFile("valid.txt", expected);

  std::string content = read_file_or_throw(path);

  EXPECT_EQ(content, expected);
}

// Test read_file_or_throw with non-existent file throws
TEST_F(UtilsTest, ReadFileOrThrowThrows) {
  EXPECT_THROW(read_file_or_throw("/nonexistent/file.txt"), std::runtime_error);
}

// Test formatAPIError with valid JSON error
TEST_F(UtilsTest, FormatAPIErrorValidJSON) {
  std::string raw_error = R"({
        "error": {
            "type": "not_found_error",
            "message": "Model not found: model: invalid-model-name"
        }
    })";

  std::string formatted = formatAPIError(raw_error);

  EXPECT_THAT(formatted, testing::HasSubstr("Invalid model"));
  EXPECT_THAT(formatted, testing::HasSubstr("invalid-model-name"));
}

// Test formatAPIError with generic error message
TEST_F(UtilsTest, FormatAPIErrorGenericMessage) {
  std::string raw_error = R"({
        "error": {
            "type": "rate_limit_error",
            "message": "Rate limit exceeded. Please try again later."
        }
    })";

  std::string formatted = formatAPIError(raw_error);

  EXPECT_EQ(formatted, "Rate limit exceeded. Please try again later.");
}

// Test formatAPIError with not_found_error but no model info
TEST_F(UtilsTest, FormatAPIErrorNotFoundNoModel) {
  std::string raw_error = R"({
        "error": {
            "type": "not_found_error",
            "message": "Resource not found"
        }
    })";

  std::string formatted = formatAPIError(raw_error);

  EXPECT_THAT(formatted, testing::HasSubstr("Model not found"));
}

// Test formatAPIError with invalid JSON returns raw
TEST_F(UtilsTest, FormatAPIErrorInvalidJSON) {
  std::string raw_error = "This is not JSON";

  std::string formatted = formatAPIError(raw_error);

  EXPECT_EQ(formatted, raw_error);
}

// Test formatAPIError with JSON embedded in text
TEST_F(UtilsTest, FormatAPIErrorJSONInText) {
  std::string raw_error =
      R"(API Error: {"error": {"message": "Authentication failed"}})";

  std::string formatted = formatAPIError(raw_error);

  EXPECT_EQ(formatted, "Authentication failed");
}

// Test formatAPIError with empty error object
TEST_F(UtilsTest, FormatAPIErrorEmptyError) {
  std::string raw_error = R"({"error": {}})";

  std::string formatted = formatAPIError(raw_error);

  // Falls through to return raw error
  EXPECT_EQ(formatted, raw_error);
}

// Test formatAPIError with missing error key
TEST_F(UtilsTest, FormatAPIErrorMissingErrorKey) {
  std::string raw_error = R"({"status": "error", "code": 500})";

  std::string formatted = formatAPIError(raw_error);

  EXPECT_EQ(formatted, raw_error);
}

// Test reading actual fixture files
TEST_F(UtilsTest, ReadFixtureFiles) {
  std::string config_path = getConfigFixture("valid_config.ini");
  auto [success, content] = read_file(config_path);

  EXPECT_TRUE(success);
  EXPECT_THAT(content, testing::HasSubstr("[general]"));
  EXPECT_THAT(content, testing::HasSubstr("[openai]"));
}

// Test TempConfigFile helper
TEST(TempConfigFileTest, CreatesAndCleansUp) {
  std::string path;
  {
    TempConfigFile temp("test content");
    path = temp.path();

    // File should exist
    EXPECT_TRUE(std::filesystem::exists(path));

    // Content should be readable
    std::ifstream file(path);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    EXPECT_EQ(content, "test content");
  }

  // File should be deleted after scope
  EXPECT_FALSE(std::filesystem::exists(path));
}
