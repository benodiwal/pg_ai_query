Developer Guide

This guide is designed to help new contributors get started with common development tasks in `pg_ai_query`, such as adding new AI providers, updating configuration options, and debugging the extension.

## Table of Contents
1. [Adding a New AI Provider](#1-adding-a-new-ai-provider)
2. [Writing Tests](#2-writing-tests)
3. [Debugging](#3-debugging)
4. [Build and Test Workflow](#4-build-and-test-workflow)
5. [Additional Resources](#5-additional-resources)

---

## 1. Adding a New AI Provider

To add support for a new AI provider (e.g., "Cohere"), follow these steps:

### 1.1. Create Provider Files

Create a new directory and files within `src/providers/`:

```text
src/providers/cohere/
├── client.hpp
└── client.cpp
```

### 1.2. Implement the Client Interface

The client interface defines how the extension communicates with the AI provider's API. You'll need to implement request building, HTTP communication, and response parsing.

**Header File: `src/providers/cohere/client.hpp`**

```cpp
#pragma once

#include <string>
#include <optional>


namespace pg_ai::providers::cohere {

// Request structure for Cohere API calls
struct CohereRequest {
  std::string model;
  std::string system_prompt;
  std::string user_prompt;
  std::optional<double> temperature;
  std::optional<int> max_tokens;
};

// Normalized response structure
struct CohereResponse {
  std::string text;
  bool success;
  std::string error_message;
  int status_code;
};

class CohereClient {
 public:
  explicit CohereClient(const std::string& api_key,
                        const std::string& endpoint);
  ~CohereClient() = default;

  // Main entry point: generates text based on prompts
  std::string generate_text(const std::string& prompt,
                       const std::string& system_prompt);

 private:
  std::string api_key_;
  std::string endpoint_;

  // Helper methods - implement according to provider's API specification
  std::string build_request_body(const CohereRequest& request);
  CohereResponse make_http_request(const std::string& url,
                                   const std::string& body);
  CohereResponse parse_response(const std::string& body, int status_code);
};

}  // namespace pg_ai::providers::cohere
```

**Implementation File: `src/providers/cohere/client.cpp`**

```cpp
#include "client.hpp"
#include "include/logger.hpp"

namespace pg_ai::providers::cohere {

CohereClient::CohereClient(const std::string& api_key,
                           const std::string& endpoint)
    : api_key_(api_key), endpoint_(endpoint) {
  // Validate API key is not empty
  // Initialize HTTP client or SDK if needed
}

std::string CohereClient::generate_text(const std::string& prompt,
                                   const std::string& system_prompt) {
  // Build CohereRequest object with prompts and configuration
  // Serialize request to JSON using build_request_body()
  // Construct full API URL (endpoint + path)
  // Call make_http_request() with URL and JSON body
  // Check response.success and throw on error
  // Return response.text
  
  return "";
}

std::string CohereClient::build_request_body(const CohereRequest& request) {
  // Create JSON object with required fields per provider API docs
  // Add optional fields (temperature, max_tokens) if present
  // Serialize to string and return
  
  return "";
}

CohereResponse CohereClient::make_http_request(const std::string& url,
                                                const std::string& body) {
  // Set HTTP headers (Authorization, Content-Type)
  // Execute POST request to URL with body
  // Capture HTTP status code and response body
  // Call parse_response() and return result
  
  CohereResponse response;
  return response;
}

CohereResponse CohereClient::parse_response(const std::string& body,
                                            int status_code) {
  // Create CohereResponse object
  // If status_code indicates error, extract error message
  // If success, parse JSON and extract generated text field
  // Handle malformed JSON gracefully
  // Return populated response
  
  CohereResponse response;
  response.status_code = status_code;
  response.success = (status_code == 200);
  return response;
}

}  // namespace pg_ai::providers::cohere
```

**Implementation Guidelines:**
- Consult the provider's official API documentation for request/response schemas
- Use `nlohmann/json` or similar library for JSON serialization
- Use `libcurl` wrapper or HTTP client library for network requests
- Always validate inputs and handle edge cases (empty responses, timeouts, etc.)

### 1.3. Register in Factory

The factory pattern allows the extension to dynamically create the appropriate client based on configuration. Add a new case to the provider switch in `src/core/ai_client_factory.cpp`.

```cpp
// src/core/ai_client_factory.cpp

case config::Provider::COHERE: {
  logger::Logger::info("Creating Cohere client");

  // Determine the API endpoint to use:
  // 1. Use custom endpoint from config if provided
  // 2. Otherwise fall back to the default Cohere endpoint
  std::string base_url =
      (provider_config && !provider_config->api_endpoint.empty())
          ? provider_config->api_endpoint
          : config::constants::DEFAULT_COHERE_ENDPOINT;

  if (provider_config && !provider_config->api_endpoint.empty()) {
    logger::Logger::info("Using custom Cohere endpoint: " + base_url);
  }

  // Create Cohere client with API key and endpoint
  result.client = ai::cohere::create_client(api_key, base_url);

  // Select model:
  // 1. Use model from config if provided
  // 2. Otherwise use the default Cohere model
  result.model_name =
      (provider_config && !provider_config->default_model.empty())
          ? provider_config->default_model
          : constants::DEFAULT_COHERE_MODEL;

  break;
}

```

Provider defaults should be defined in `src/include/config.hpp`
under `pg_ai::config::constants`.


```cpp
namespace constants {

constexpr const char* DEFAULT_COHERE_MODEL = "command-r-plus";
constexpr const char* DEFAULT_COHERE_ENDPOINT = "https://api.cohere.ai";

}  // namespace constants
```

### 1.4. Add Configuration Support

The configuration system needs to recognize the new provider name and parse its settings from the INI file.

**Update `src/include/config.hpp`:**

First, extend the `Provider` enum and add configuration constants:

```cpp
namespace pg_ai::config {

// Add to Provider enum
enum class Provider {
  OPENAI,
  ANTHROPIC,
  GEMINI,
  COHERE,     // Add your provider here
  UNKNOWN
};

namespace constants {

// Provider identifier (matches INI section name)
constexpr const char* PROVIDER_COHERE = "cohere";
constexpr const char* SECTION_COHERE = "cohere";

// Defaults (used if not specified in config)
constexpr const char* DEFAULT_COHERE_MODEL = "command-r-plus";
constexpr const char* DEFAULT_COHERE_ENDPOINT = "https://api.cohere.ai";

}  // namespace constants
}  // namespace pg_ai::config
```

**Update `src/config.cpp`:**

Add provider string conversion and config parsing logic:

```cpp
std::string ConfigManager::providerToString(Provider provider) {
  switch (provider) {
    case Provider::COHERE:
      return constants::PROVIDER_COHERE;
    // ... other providers
    default:
      return "unknown";
  }
}

Provider ConfigManager::stringToProvider(const std::string& provider_str) {
  std::string lower = provider_str;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

  if (lower == constants::PROVIDER_COHERE)
    return Provider::COHERE;
  // ... other providers
  
  return Provider::UNKNOWN;
}

// In parseConfigFile() method, add section handler:
else if (current_section == constants::SECTION_COHERE) {
  auto provider_config = getProviderConfigMutable(Provider::COHERE);
  
  // Initialize config if not present
  if (!provider_config) {
    ProviderConfig config;
    config.provider = Provider::COHERE;
    config.default_model = constants::DEFAULT_COHERE_MODEL;
    config.default_max_tokens = constants::DEFAULT_MAX_TOKENS;
    config.default_temperature = constants::DEFAULT_TEMPERATURE;
    
    config_.providers.push_back(config);
    provider_config = &config_.providers.back();
  }

  // Parse key-value pairs from INI
  if (key == "api_key")
    provider_config->api_key = value;
  else if (key == "default_model")
    provider_config->default_model = value;
  else if (key == "max_tokens")
    provider_config->default_max_tokens = std::stoi(value);
  else if (key == "temperature")
    provider_config->default_temperature = std::stod(value);
  else if (key == "api_endpoint")
    provider_config->api_endpoint = value;
}
```


### 2. Add Tests

Create tests to validate your implementation. Focus on testing the public interface and critical error paths.

**Create `tests/unit/test_cohere_client.cpp`:**

```cpp
#include <gtest/gtest.h>
#include "providers/cohere/client.hpp"
#include "config/config.hpp"

using namespace pg_ai::providers::cohere;
using namespace pg_ai::config;

class CohereClientTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Load test configuration
    // Get Cohere provider config
  }
};

TEST_F(CohereClientTest, RejectsEmptyAPIKey) {
  EXPECT_THROW(
      CohereClient("", "https://api.cohere.ai"),
      // Throw std::runtime_error if api_key is empty
      std::runtime_error
  );
}
```

**Recommended Test Categories:**

Your test suite should cover:

1. **Initialization**
   - Valid API key and endpoint
   - Empty/invalid API keys
   - Malformed endpoint URLs

2. **Request Building**
   - Required fields are present
   - Optional parameters handled correctly
   - System prompts included properly

3. **Response Parsing**
   - Success responses (HTTP 200)
   - Error responses (HTTP 4xx, 5xx)
   - Malformed JSON
   - Empty or missing fields

4. **Error Handling**
   - Network failures
   - Authentication errors
   - Rate limiting

5. **Configuration Integration**
   - Default model selection
   - Custom endpoint override
   - Parameter defaults from config

## Update CMakeLists.txt

The `tests/CMakeLists.txt` needs to be updated to include the new Cohere client source files and the new unit test file so they are part of the test build.

#### tests/CMakeLists.txt
#### ... existing code ...

```cmake
# Source files for core library (without PostgreSQL dependencies)
set(CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/config.cpp
    ${CMAKE_SOURCE_DIR}/src/core/provider_selector.cpp
    ${CMAKE_SOURCE_DIR}/src/core/response_formatter.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ai_client_factory.cpp
    ${CMAKE_SOURCE_DIR}/src/core/logger.cpp
    ${CMAKE_SOURCE_DIR}/src/core/query_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/utils.cpp
    ${CMAKE_SOURCE_DIR}/src/prompts.cpp
    ${CMAKE_SOURCE_DIR}/src/providers/gemini/client.cpp # Include Gemini client if not already
    ${CMAKE_SOURCE_DIR}/src/providers/cohere/client.cpp # Add Cohere client implementation
)

# ... existing code ...

# Unit tests executable
add_executable(pg_ai_query_tests
    unit/test_config.cpp
    unit/test_provider_selector.cpp
    unit/test_response_formatter.cpp
    unit/test_utils.cpp
    unit/test_query_parser.cpp
    unit/test_cohere_client.cpp # Add the new Cohere client test file
)

# ... existing code ...

```

## 3. Debugging

Effective debugging is crucial for developing and maintaining the `pg_ai_query` extension. This section covers tools and techniques to identify and resolve issues.

### Using `elog`

The extension uses PostgreSQL's `elog` mechanism for logging, wrapped by the `pg_ai::logger::Logger` class for convenient C++ usage.

#### Logging Levels

- **`Logger::info("message")`**: General status updates (maps to PostgreSQL's `INFO`)
- **`Logger::warning("message")`**: Non-critical issues (maps to `WARNING`)
- **`Logger::error("message")`**: Critical failures that abort the transaction (maps to `ERROR`)
- **`Logger::debug("message")`**: Detailed internal state for debugging (maps to `DEBUG1`)

To see debug logs, adjust the log level in your PostgreSQL session:

```sql
SET log_min_messages TO DEBUG1;
```

#### Example Usage

```cpp
#include "include/logger.hpp"

void generate_query(const std::string& provider) {
  pg_ai::logger::Logger::info("Starting query generation for provider: " + provider);
  
  // ... processing ...
  
  pg_ai::logger::Logger::debug("Request payload size: " + std::to_string(payload.size()));
}
```

### Reading Extension Logs

Extension logs appear in PostgreSQL's standard log output:

- **Linux (Debian/Ubuntu)**: `/var/log/postgresql/postgresql-<version>-main.log`
- **macOS (Homebrew)**: `/usr/local/var/log/postgres.log`
- **Docker**: View with `docker logs <container_name>`

For interactive debugging in `psql`:

```sql
-- See debug logs in your client session
SET client_min_messages TO DEBUG1;
SELECT pg_ai.generate_query('Show me all active users');
```

### Common Issues and Solutions

| Issue | Possible Cause | Solution |
| :--- | :--- | :--- |
| `ERROR: could not load library ... undefined symbol` | Missing implementation or linker error | Ensure all functions are defined and declared `extern "C"` if called by PostgreSQL. Rebuild and restart PostgreSQL. |
| `ERROR: function ... does not exist` | SQL definition missing or not loaded | Check extension SQL file (e.g., `pg_ai_query--0.0.1.sql`) and run `DROP EXTENSION pg_ai_query; CREATE EXTENSION pg_ai_query;`. |
| Logs not appearing | Log level too restrictive | Check `log_min_messages` and `client_min_messages`. Set to `INFO` or `DEBUG1`. |
| Network/curl errors | Connection issues or missing API keys | Verify API key is configured and database server has internet access. Check firewall rules. |

---

## 4. Build and Test Workflow

Before diving into code changes, ensure you can build and test the project locally.

### Local Development Cycle

1. **Configure:** Set up the build directory with CMake (Debug mode recommended for development):

    ```bash
    mkdir -p build && cd build
    cmake -DCMAKE_BUILD_TYPE=Debug ..
    ```

2. **Build:** Compile the extension:

    ```bash
    make
    ```

3. **Install:** Install to your local PostgreSQL instance (requires `sudo`):

    ```bash
    sudo make install
    ```

4. **Reload in PostgreSQL:** After installation, reload the extension:

    ```sql
    DROP EXTENSION IF EXISTS pg_ai_query;
    CREATE EXTENSION pg_ai_query;
    ```

### Running Tests

The project uses Make targets for different test types:

- **Unit Tests:** Run C++ unit tests (GoogleTest) without PostgreSQL:

    ```bash
    make test-unit
    ```

- **Integration Tests:** Run tests within a PostgreSQL instance:

    ```bash
    make test-pg
    ```

- **Specific Test Suite:** Run a particular test suite:

    ```bash
    make test-suite SUITE=CohereClientTest
    ```

- **All Tests:** Run both unit and integration tests:

    ```bash
    make test
    ```

### Checking Code Style

Maintain code consistency with `clang-format`:

- **Format Code:** Automatically format all source files:

    ```bash
    make format
    ```

- **Check Formatting:** Verify formatting without changes (useful for CI):

    ```bash
    make format-check
    ```

### Development Tips

- **Incremental builds:** After the first build, `make` will only recompile changed files
- **Clean build:** If you encounter linking issues, try `make clean && make`
- **Verbose output:** For detailed build information, use `make VERBOSE=1`
- **Parallel builds:** Speed up compilation with `make -j4` (adjust number based on CPU cores)

---

## 5. Additional Resources

- **PostgreSQL Extension Documentation**: [https://www.postgresql.org/docs/current/extend.html](https://www.postgresql.org/docs/current/extend.html)
- **GoogleTest Documentation**: [https://google.github.io/googletest/](https://google.github.io/googletest/)
- **nlohmann/json Library**: [https://github.com/nlohmann/json](https://github.com/nlohmann/json)