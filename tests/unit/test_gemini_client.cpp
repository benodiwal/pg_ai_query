// Unit tests for the Gemini AI provider client (src/providers/gemini/client.cpp).
// Covers request building (build_request_body), response parsing (parse_response),
// and error handling for the Gemini API without making live HTTP calls.

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "include/gemini_client.h"

using namespace gemini;

namespace gemini {

// Exposes private methods for unit testing via friend declaration in GeminiClient.
class TestableGeminiClient : public GeminiClient {
 public:
  explicit TestableGeminiClient(const std::string& api_key)
      : GeminiClient(api_key) {}

  std::string build_request_body(const GeminiRequest& request) {
    return GeminiClient::build_request_body(request);
  }

  GeminiResponse parse_response(const std::string& body, int status_code) {
    return GeminiClient::parse_response(body, status_code);
  }
};

}  // namespace gemini

class GeminiClientTest : public ::testing::Test {
 protected:
  void SetUp() override {
    client_ = std::make_unique<TestableGeminiClient>("test-api-key");
  }

  std::unique_ptr<TestableGeminiClient> client_;
};

// =============================================================================
// build_request_body tests
// =============================================================================

TEST_F(GeminiClientTest, BuildRequestBodyIncludesUserPrompt) {
  GeminiRequest request;
  // gemini-2.0-flash is a valid Gemini API model name.
  request.model = "gemini-2.0-flash";
  request.user_prompt = "Generate a query";
  request.system_prompt = "";

  std::string body = client_->build_request_body(request);
  auto json = nlohmann::json::parse(body);

  ASSERT_TRUE(json.contains("contents")) << "Response missing 'contents' field";
  ASSERT_TRUE(json["contents"].is_array()) << "'contents' is not an array";
  ASSERT_FALSE(json["contents"].empty()) << "'contents' array is empty";
  EXPECT_EQ(json["contents"][0]["parts"][0]["text"], "Generate a query");
}

TEST_F(GeminiClientTest, BuildRequestBodyIncludesSystemPrompt) {
  GeminiRequest request;
  request.model = "gemini-2.0-flash";
  request.user_prompt = "Generate a query";
  request.system_prompt = "You are a SQL expert";

  std::string body = client_->build_request_body(request);
  auto json = nlohmann::json::parse(body);

  ASSERT_TRUE(json.contains("systemInstruction"))
      << "Response missing 'systemInstruction' field";
  ASSERT_TRUE(json["systemInstruction"].contains("parts"))
      << "'systemInstruction' missing 'parts'";
  ASSERT_TRUE(json["systemInstruction"]["parts"].is_array())
      << "'parts' is not an array";
  ASSERT_FALSE(json["systemInstruction"]["parts"].empty())
      << "'parts' array is empty";
  EXPECT_EQ(json["systemInstruction"]["parts"][0]["text"],
            "You are a SQL expert");
}

TEST_F(GeminiClientTest, BuildRequestBodyOmitsSystemInstructionWhenEmpty) {
  GeminiRequest request;
  request.model = "gemini-2.0-flash";
  request.user_prompt = "Test";
  request.system_prompt = "";

  std::string body = client_->build_request_body(request);
  auto json = nlohmann::json::parse(body);

  EXPECT_FALSE(json.contains("systemInstruction"));
}

TEST_F(GeminiClientTest, BuildRequestBodyIncludesGenerationConfig) {
  GeminiRequest request;
  request.model = "gemini-2.0-flash";
  request.user_prompt = "Test";
  request.temperature = 0.7;
  request.max_tokens = 1000;

  std::string body = client_->build_request_body(request);
  auto json = nlohmann::json::parse(body);

  ASSERT_TRUE(json.contains("generationConfig"))
      << "Response missing 'generationConfig' field";
  EXPECT_DOUBLE_EQ(json["generationConfig"]["temperature"].get<double>(), 0.7);
  EXPECT_EQ(json["generationConfig"]["maxOutputTokens"].get<int>(), 1000);
}

TEST_F(GeminiClientTest, BuildRequestBodyOmitsGenerationConfigWhenOptionalEmpty) {
  GeminiRequest request;
  request.model = "gemini-2.0-flash";
  request.user_prompt = "Test";
  request.temperature = std::nullopt;
  request.max_tokens = std::nullopt;

  std::string body = client_->build_request_body(request);
  auto json = nlohmann::json::parse(body);

  EXPECT_FALSE(json.contains("generationConfig"));
}

TEST_F(GeminiClientTest, BuildRequestBodyPartialGenerationConfig) {
  GeminiRequest request;
  request.model = "gemini-2.0-flash";
  request.user_prompt = "Test";
  request.temperature = 0.5;
  request.max_tokens = std::nullopt;

  std::string body = client_->build_request_body(request);
  auto json = nlohmann::json::parse(body);

  ASSERT_TRUE(json.contains("generationConfig"))
      << "Response missing 'generationConfig' field";
  EXPECT_TRUE(json["generationConfig"].contains("temperature"));
  EXPECT_FALSE(json["generationConfig"].contains("maxOutputTokens"));
}

TEST_F(GeminiClientTest, BuildRequestBodyGenerationConfigTemperatureBoundary) {
  GeminiRequest request;
  request.model = "gemini-2.0-flash";
  request.user_prompt = "Test";
  request.temperature = 2.0;
  request.max_tokens = std::nullopt;

  std::string body = client_->build_request_body(request);
  auto json = nlohmann::json::parse(body);

  ASSERT_TRUE(json.contains("generationConfig"));
  EXPECT_DOUBLE_EQ(json["generationConfig"]["temperature"].get<double>(), 2.0);
}

TEST_F(GeminiClientTest, BuildRequestBodyGenerationConfigNegativeTemperature) {
  GeminiRequest request;
  request.model = "gemini-2.0-flash";
  request.user_prompt = "Test";
  request.temperature = -0.5;
  request.max_tokens = std::nullopt;

  std::string body = client_->build_request_body(request);
  auto json = nlohmann::json::parse(body);

  ASSERT_TRUE(json.contains("generationConfig"));
  EXPECT_DOUBLE_EQ(json["generationConfig"]["temperature"].get<double>(), -0.5);
}

TEST_F(GeminiClientTest, BuildRequestBodyGenerationConfigZeroMaxTokens) {
  GeminiRequest request;
  request.model = "gemini-2.0-flash";
  request.user_prompt = "Test";
  request.temperature = std::nullopt;
  request.max_tokens = 0;

  std::string body = client_->build_request_body(request);
  auto json = nlohmann::json::parse(body);

  ASSERT_TRUE(json.contains("generationConfig"));
  EXPECT_EQ(json["generationConfig"]["maxOutputTokens"].get<int>(), 0);
}

TEST_F(GeminiClientTest, BuildRequestBodyEscapesSpecialCharacters) {
  GeminiRequest request;
  request.model = "gemini-2.0-flash";
  request.user_prompt = "Show \"users\" with\nnewlines and 'quotes'";
  request.system_prompt = "";

  std::string body = client_->build_request_body(request);
  EXPECT_NO_THROW(nlohmann::json::parse(body));
}

TEST_F(GeminiClientTest, BuildRequestBodyHandlesEmptyUserPrompt) {
  GeminiRequest request;
  request.model = "gemini-2.0-flash";
  request.user_prompt = "";
  request.system_prompt = "";

  std::string body = client_->build_request_body(request);
  auto json = nlohmann::json::parse(body);

  ASSERT_TRUE(json.contains("contents")) << "Response missing 'contents' field";
  ASSERT_TRUE(json["contents"].is_array()) << "'contents' is not an array";
  ASSERT_FALSE(json["contents"].empty()) << "'contents' array is empty";
  EXPECT_EQ(json["contents"][0]["parts"][0]["text"].get<std::string>(), "");
}

// =============================================================================
// parse_response tests - success
// =============================================================================

TEST_F(GeminiClientTest, ParseResponseExtractsContent) {
  std::string response_body = R"({
        "candidates": [{
            "content": {
                "parts": [{"text": "SELECT * FROM users;"}]
            }
        }]
    })";

  auto result = client_->parse_response(response_body, 200);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.status_code, 200);
  EXPECT_EQ(result.text, "SELECT * FROM users;");
}

TEST_F(GeminiClientTest, ParseResponseUsesFirstCandidateOnly) {
  std::string response_body = R"({
        "candidates": [
            {
                "content": {
                    "parts": [{"text": "First candidate text"}]
                }
            },
            {
                "content": {
                    "parts": [{"text": "Second candidate text"}]
                }
            }
        ]
    })";

  auto result = client_->parse_response(response_body, 200);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.status_code, 200);
  EXPECT_EQ(result.text, "First candidate text");
}

TEST_F(GeminiClientTest, ParseResponseUsesFirstPartOnly) {
  std::string response_body = R"({
        "candidates": [{
            "content": {
                "parts": [
                    {"text": "First part text"},
                    {"text": "Second part text"}
                ]
            }
        }]
    })";

  auto result = client_->parse_response(response_body, 200);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.status_code, 200);
  EXPECT_EQ(result.text, "First part text");
}

TEST_F(GeminiClientTest, ParseResponseHandlesEmptyText) {
  std::string response_body = R"({
        "candidates": [{
            "content": {
                "parts": [{"text": ""}]
            }
        }]
    })";

  auto result = client_->parse_response(response_body, 200);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.status_code, 200);
  EXPECT_EQ(result.text, "");
}

// =============================================================================
// parse_response tests - HTTP error (non-200)
// =============================================================================

TEST_F(GeminiClientTest, ParseResponseHandlesHttpError401) {
  std::string error_body = R"({
        "error": {
            "code": 401,
            "message": "Invalid API key"
        }
    })";

  auto result = client_->parse_response(error_body, 401);

  EXPECT_FALSE(result.success);
  EXPECT_FALSE(result.error_message.empty()) << "Error response should have a user-friendly message";
  EXPECT_TRUE(result.error_message.find("Invalid API key") !=
              std::string::npos);
  EXPECT_EQ(result.status_code, 401);
}

TEST_F(GeminiClientTest, ParseResponseHandlesHttpError429) {
  std::string error_body = R"({
        "error": {
            "code": 429,
            "message": "Resource has been exhausted"
        }
    })";

  auto result = client_->parse_response(error_body, 429);

  EXPECT_FALSE(result.success);
  EXPECT_FALSE(result.error_message.empty()) << "Error response should have a user-friendly message";
  EXPECT_TRUE(result.error_message.find("Resource has been exhausted") !=
              std::string::npos);
  EXPECT_EQ(result.status_code, 429);
}

TEST_F(GeminiClientTest, ParseResponseHandlesNon200WithoutErrorJson) {
  std::string error_body = "Internal Server Error";

  auto result = client_->parse_response(error_body, 500);

  EXPECT_FALSE(result.success);
  EXPECT_TRUE(result.error_message.find("500") != std::string::npos);
}

// =============================================================================
// parse_response tests - missing or invalid structure (200)
// =============================================================================

TEST_F(GeminiClientTest, ParseResponseHandlesMissingCandidates) {
  std::string response_body = R"({"usageMetadata": {}})";

  auto result = client_->parse_response(response_body, 200);

  EXPECT_FALSE(result.success);
  EXPECT_TRUE(result.error_message.find("Invalid response format") !=
              std::string::npos);
}

TEST_F(GeminiClientTest, ParseResponseHandlesEmptyCandidates) {
  std::string response_body = R"({"candidates": []})";

  auto result = client_->parse_response(response_body, 200);

  EXPECT_FALSE(result.success);
  EXPECT_TRUE(result.error_message.find("Invalid response format") !=
              std::string::npos);
}

TEST_F(GeminiClientTest, ParseResponseHandlesMissingContent) {
  std::string response_body = R"({
        "candidates": [{}]
    })";

  auto result = client_->parse_response(response_body, 200);

  EXPECT_FALSE(result.success);
  EXPECT_TRUE(result.error_message.find("Invalid response format") !=
              std::string::npos);
}

TEST_F(GeminiClientTest, ParseResponseHandlesEmptyParts) {
  std::string response_body = R"({
        "candidates": [{
            "content": {
                "parts": []
            }
        }]
    })";

  auto result = client_->parse_response(response_body, 200);

  EXPECT_FALSE(result.success);
  EXPECT_TRUE(result.error_message.find("Invalid response format") !=
              std::string::npos);
}

TEST_F(GeminiClientTest, ParseResponseHandlesMissingText) {
  std::string response_body = R"({
        "candidates": [{
            "content": {
                "parts": [{}]
            }
        }]
    })";

  auto result = client_->parse_response(response_body, 200);

  EXPECT_FALSE(result.success);
  EXPECT_TRUE(result.error_message.find("Invalid response format") !=
              std::string::npos);
}

// =============================================================================
// parse_response tests - malformed JSON
// =============================================================================

TEST_F(GeminiClientTest, ParseResponseHandlesMalformedJson) {
  std::string response_body = "not valid json {{{";

  auto result = client_->parse_response(response_body, 200);

  EXPECT_FALSE(result.success);
  EXPECT_TRUE(result.error_message.find("JSON parse error") !=
              std::string::npos);
}
