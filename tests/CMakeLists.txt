cmake_minimum_required(VERSION 3.16)

enable_testing()

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
    ${CMAKE_SOURCE_DIR}/src/core/ai_client_factory.cpp
)

# Create a testable core library (without PostgreSQL SPI dependencies)
add_library(pg_ai_query_core STATIC ${CORE_SOURCES})

target_include_directories(pg_ai_query_core PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/ai-sdk-cpp/include
)

target_link_libraries(pg_ai_query_core PUBLIC
    ai-sdk-cpp-core
    ai-sdk-cpp-openai
    ai-sdk-cpp-anthropic
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Don't define USE_POSTGRESQL_ELOG for tests - use stdout logging
# This ensures Logger uses stdout instead of PostgreSQL's elog()

# Unit tests executable
add_executable(pg_ai_query_tests
    unit/test_config.cpp
    unit/test_provider_selector.cpp
    unit/test_response_formatter.cpp
    unit/test_utils.cpp
    unit/test_query_parser.cpp
    unit/test_ai_client_factory.cpp
)

target_include_directories(pg_ai_query_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/ai-sdk-cpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pg_ai_query_tests PRIVATE
    pg_ai_query_core
    gtest_main
    gmock
)

# Define test fixtures path
target_compile_definitions(pg_ai_query_tests PRIVATE
    TEST_FIXTURES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)

include(GoogleTest)
gtest_discover_tests(pg_ai_query_tests)