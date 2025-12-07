cmake_minimum_required(VERSION 3.16)

project(gemini-client)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(CURL REQUIRED)

# Create library
add_library(gemini-client STATIC
    src/client.cpp
)

target_include_directories(gemini-client PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ai-sdk-cpp/third_party/nlohmann_json_patched/include
)

target_link_libraries(gemini-client PUBLIC
    CURL::libcurl
)
