# ==============================================================================
# pg_ai_query - PostgreSQL Extension Build Configuration
# ==============================================================================

cmake_minimum_required(VERSION 3.16)

# Project definition
# We use C++20 for features like designated initializers, std::format
# and better template support which is required by modern AI SDKs.
project(pg_ai_query 
    VERSION 0.1.1
    LANGUAGES CXX
    DESCRIPTION "PostgreSQL extension for AI-powered query generation"
)

# Set C++ standard to 20
# Required for modern C++ features and compatibility with ai-sdk-cpp
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------------------
# PostgreSQL Configuration
# ------------------------------------------------------------------------------

# Find pg_config to locate PostgreSQL installation paths
# pg_config is the standard tool for discovering PostgreSQL build environment
find_program(PG_CONFIG pg_config
    HINTS /usr/bin /usr/local/bin /opt/homebrew/bin
    DOC "Path to pg_config executable"
)

# If pg_config is not found, provide helpful installation instructions
if(NOT PG_CONFIG)
    message(FATAL_ERROR 
        "pg_config not found. Install postgresql-server-dev package.\n"
        "On Ubuntu: apt install postgresql-server-dev-14\n"
        "On Debian: apt install postgresql-server-dev-all\n"
        "On macOS: brew install postgresql@14\n"
        "On Fedora/RHEL: dnf install postgresql-devel\n"
        "On Arch Linux: pacman -S postgresql-libs"
    )
endif()

# Extract required paths from pg_config
# --includedir-server: Where PostgreSQL server headers are (postgres.h, spi.h, etc.)
# --includedir: Where general PostgreSQL client headers are
# --pkglibdir: Where extension shared libraries (.so/.dylib) must be installed
# --sharedir: Where extension SQL scripts and control files are located
execute_process(COMMAND ${PG_CONFIG} --includedir-server OUTPUT_VARIABLE PG_INCLUDE_SERVER OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${PG_CONFIG} --includedir OUTPUT_VARIABLE PG_INCLUDE OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${PG_CONFIG} --pkglibdir OUTPUT_VARIABLE PG_PKGLIB OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${PG_CONFIG} --libdir OUTPUT_VARIABLE PG_LIBDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${PG_CONFIG} --bindir OUTPUT_VARIABLE PG_BINDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${PG_CONFIG} --sharedir OUTPUT_VARIABLE PG_SHAREDIR OUTPUT_STRIP_TRAILING_WHITESPACE)

# Include PostgreSQL headers
# These are essential for any PostgreSQL extension to access internal APIs
include_directories(${PG_INCLUDE_SERVER})
include_directories(${PG_INCLUDE})

# ------------------------------------------------------------------------------
# Third-Party Dependencies (Submodules)
# ------------------------------------------------------------------------------

# Configure ai-sdk-cpp options
# We build as static libraries to simplify extension packaging and avoid 
# runtime shared library loading issues (RPATH) within the PostgreSQL process.
# Trade-off: Larger binary size, but eliminates dependency management headaches.
set(AI_SDK_BUILD_EXAMPLES OFF CACHE BOOL "Disable SDK examples")
set(AI_SDK_BUILD_TESTS OFF CACHE BOOL "Disable SDK tests")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Force static libraries for dependencies")
set(AI_SDK_STATIC_ONLY ON CACHE BOOL "Only use static SDK components")

# Enable Position Independent Code for all targets (required for Linux shared libraries)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Add the ai-sdk-cpp submodule
# This provides the core AI client logic for OpenAI, Anthropic, and Gemini.
add_subdirectory(third_party/ai-sdk-cpp)

# Include our source directories
include_directories(src)
include_directories(third_party/ai-sdk-cpp/third_party/nlohmann_json_patched/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/ai-sdk-cpp/include)

# ------------------------------------------------------------------------------
# Extension Target Definition
# ------------------------------------------------------------------------------

# Source files for the clean implementation
set(SOURCES
    src/pg_ai_query.cpp
    src/core/query_generator.cpp
    src/core/query_parser.cpp
    src/core/response_formatter.cpp
    src/core/logger.cpp
    src/providers/gemini/client.cpp
    src/core/provider_selector.cpp
    src/core/ai_client_factory.cpp
    src/core/spi_connection.cpp
    src/utils.cpp
    src/prompts.cpp
    src/config.cpp
)

# Get PostgreSQL version to handle platform-specific naming conventions
execute_process(COMMAND ${PG_CONFIG} --version OUTPUT_VARIABLE PG_VERSION_OUTPUT OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REGEX MATCH "PostgreSQL ([0-9]+)" PG_VERSION_MATCH ${PG_VERSION_OUTPUT})
set(PG_MAJOR_VERSION ${CMAKE_MATCH_1})

# Platform-specific build logic
if(APPLE)
    # macOS: PostgreSQL extensions should be built as 'MODULE'
    add_library(pg_ai_query MODULE ${SOURCES})
    # PostgreSQL 16+ on macOS expects .dylib, earlier versions expect .so
    if(PG_MAJOR_VERSION GREATER_EQUAL 16)
        set(PG_EXTENSION_SUFFIX ".dylib")
    else()
        set(PG_EXTENSION_SUFFIX ".so")
    endif()
    
    # Prefix must be empty (postgres expects 'pg_ai_query.so', not 'libpg_ai_query.so')
    # bundle_loader ensures symbols are resolved against the postgres binary at load time
    set_target_properties(pg_ai_query PROPERTIES
        PREFIX ""
        SUFFIX ${PG_EXTENSION_SUFFIX}
        LINK_FLAGS "-bundle_loader ${PG_BINDIR}/postgres"
    )
    message(STATUS "Building PostgreSQL ${PG_MAJOR_VERSION} extension for macOS with suffix: ${PG_EXTENSION_SUFFIX}")
elseif(UNIX)
    # Linux: Extensions are standard shared objects
    add_library(pg_ai_query SHARED ${SOURCES})
    set_target_properties(pg_ai_query PROPERTIES
        PREFIX ""
        SUFFIX ".so"
        POSITION_INDEPENDENT_CODE ON
        # Set RPATH for finding system libraries at runtime
        # $ORIGIN helps locate libraries relative to the extension path
        INSTALL_RPATH "$ORIGIN;$ORIGIN/../lib;/usr/lib;/usr/local/lib"
        BUILD_RPATH "$ORIGIN;$ORIGIN/../lib;/usr/lib;/usr/local/lib"
        BUILD_WITH_INSTALL_RPATH ON
    )
    message(STATUS "Building PostgreSQL ${PG_MAJOR_VERSION} extension for Linux with suffix: .so")
else()
    # Windows or other platforms (basic support)
    add_library(pg_ai_query SHARED ${SOURCES})
    set_target_properties(pg_ai_query PROPERTIES
        PREFIX ""
        SUFFIX ".dll"
    )
    message(STATUS "Building PostgreSQL extension for Windows/Other")
endif()

# ------------------------------------------------------------------------------
# System Dependencies
# ------------------------------------------------------------------------------

# OpenSSL: Required for secure HTTPS communication with AI provider APIs
find_package(OpenSSL QUIET)
if(NOT OpenSSL_FOUND)
    message(FATAL_ERROR "OpenSSL not found. Required for AI provider API calls.\n"
        "Install via:\n"
        "  Ubuntu/Debian: sudo apt install libssl-dev\n"
        "  Fedora/RHEL: sudo dnf install openssl-devel\n"
        "  Arch Linux: sudo pacman -S openssl\n"
        "  macOS: brew install openssl"
    )
endif()

if(OPENSSL_VERSION VERSION_LESS "3.0.0")
    message(WARNING "OpenSSL 3.0.0+ recommended for best security and performance")
endif()

# CURL: Used by libcurl in Google Gemini implementation to perform HTTP requests
find_package(CURL QUIET)
if(NOT CURL_FOUND)
    message(FATAL_ERROR "CURL library not found. Required for Gemini provider.\n"
        "Install via:\n"
        "  Ubuntu/Debian: sudo apt install libcurl4-openssl-dev\n"
        "  Fedora/RHEL: sudo dnf install libcurl-devel\n"
        "  Arch Linux: sudo pacman -S curl\n"
        "  macOS: brew install curl"
    )
endif()

# Intl (gettext): Required by PostgreSQL for Native Language Support (NLS)
find_package(Intl)
if(Intl_FOUND)
    message(STATUS "Found Intl library: ${Intl_LIBRARIES}")
else()
    # Manual fallback for Homebrew or other non-standard paths on macOS
    if(APPLE)
        find_path(LIBINTL_INCLUDE_DIR libintl.h PATHS /opt/homebrew/include /usr/local/include)
        find_library(LIBINTL_LIBRARY intl PATHS /opt/homebrew/lib /usr/local/lib)
        if(LIBINTL_INCLUDE_DIR AND LIBINTL_LIBRARY)
            set(Intl_FOUND TRUE)
            set(Intl_LIBRARIES ${LIBINTL_LIBRARY})
            set(Intl_INCLUDE_DIRS ${LIBINTL_INCLUDE_DIR})
            message(STATUS "Found libintl manually: ${LIBINTL_LIBRARY}")
        endif()
    endif()
endif()

# ------------------------------------------------------------------------------
# Linking and Installation
# ------------------------------------------------------------------------------

# Link libraries to our extension
# PRIVATE linking ensures these dependencies don't leak to the PostgreSQL process global namespace
target_link_libraries(pg_ai_query PRIVATE
    ai-sdk-cpp-core
    ai-sdk-cpp-openai
    ai-sdk-cpp-anthropic
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Link Intl if found (required for NLS support)
if(Intl_FOUND)
    target_link_libraries(pg_ai_query PRIVATE ${Intl_LIBRARIES})
    target_include_directories(pg_ai_query PRIVATE ${Intl_INCLUDE_DIRS})
endif()

# Add system zlib separately
# Platform-specific library configuration
if(APPLE)
    # macOS: Set RPATH for Dynamic library discovery
    execute_process(COMMAND find /usr/lib /System/Library/Frameworks -name "libz.1.dylib" -type f 2>/dev/null | head -1
        OUTPUT_VARIABLE LIBZ_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
    get_filename_component(LIBZ_DIR "${LIBZ_PATH}" DIRECTORY)

    # MACOSX_RPATH ensures the library can find its dependencies relative to itself
    set_target_properties(pg_ai_query PROPERTIES
        INSTALL_RPATH "@loader_path;/usr/lib;/System/Library/Frameworks;${LIBZ_DIR};/opt/homebrew/lib;/usr/local/lib"
        BUILD_RPATH "@loader_path;/usr/lib;/System/Library/Frameworks;${LIBZ_DIR};/opt/homebrew/lib;/usr/local/lib"
        MACOSX_RPATH ON
        LINK_SEARCH_START_STATIC ON
        LINK_SEARCH_END_STATIC ON
    )

    # Find and link system zlib (required by many HTTP/SDK components)
    find_library(SYSTEM_Z z REQUIRED)
    if(SYSTEM_Z)
        target_link_libraries(pg_ai_query PRIVATE ${SYSTEM_Z})
        message(STATUS "Using system zlib at: ${SYSTEM_Z}")
    else()
        find_package(ZLIB REQUIRED)
        target_link_libraries(pg_ai_query PRIVATE ZLIB::ZLIB)
        message(STATUS "Using standard zlib package")
    endif()

    message(STATUS "Found libz.1.dylib at: ${LIBZ_PATH}")

elseif(UNIX)
    # Linux: Link required system libraries for threading and compression
    find_package(ZLIB REQUIRED)
    find_package(Threads REQUIRED)

    target_link_libraries(pg_ai_query PRIVATE
        ZLIB::ZLIB
        Threads::Threads
    )

    # Check for libdl (dynamic loading library, needed on some Linux systems)
    find_library(DL_LIBRARY dl)
    if(DL_LIBRARY)
        target_link_libraries(pg_ai_query PRIVATE ${DL_LIBRARY})
    endif()

    message(STATUS "Linux build: Linked zlib, pthread")
endif()

# Global include directories for the target
target_include_directories(pg_ai_query PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ai-sdk-cpp/include
)

# Compile definitions
# USE_POSTGRESQL_ELOG: Tells the code to use PostgreSQL's internal logging
target_compile_definitions(pg_ai_query PRIVATE
    USE_POSTGRESQL_ELOG
)

# Installation logic
# Install to both locations for maximum compatibility:
# - PKGLIBDIR: Standard PostgreSQL extension location (primary)
# - LIBDIR: Some distributions expect libraries here (fallback)
execute_process(COMMAND ${PG_CONFIG} --version OUTPUT_VARIABLE PG_VERSION_STRING OUTPUT_STRIP_TRAILING_WHITESPACE)
set(PG_MAJOR_VERSION ${CMAKE_MATCH_1})

install(TARGETS pg_ai_query LIBRARY DESTINATION ${PG_LIBDIR})
install(TARGETS pg_ai_query LIBRARY DESTINATION ${PG_PKGLIB})

# Debug: Print installation paths
message(STATUS "PostgreSQL version: ${PG_VERSION_STRING}")
message(STATUS "PostgreSQL major version: ${PG_MAJOR_VERSION}")
message(STATUS "PostgreSQL pkglibdir: ${PG_PKGLIB}")
message(STATUS "PostgreSQL sharedir: ${PG_SHAREDIR}")
message(STATUS "Extension will be installed to: ${PG_PKGLIB}")
message(STATUS "SQL files will be installed to: ${PG_SHAREDIR}/extension/")

# Install SQL files to the correct PostgreSQL extension directory
install(FILES sql/pg_ai_query--1.0.sql
    DESTINATION ${PG_SHAREDIR}/extension/
)

install(FILES pg_ai_query.control
    DESTINATION ${PG_SHAREDIR}/extension/
)

# Install prompt templates used by the query generator
install(DIRECTORY prompts/
    DESTINATION ${PG_SHAREDIR}/extension/prompts/
    FILES_MATCHING PATTERN "*.txt"
)

# Optional: Build logger test (not built by default)
# Enable with: cmake .. -DBUILD_LOGGER_TEST=ON
# This creates a standalone test_logger executable for debugging the logging system.
option(BUILD_LOGGER_TEST "Build logger test executable" OFF)
if(BUILD_LOGGER_TEST)
    add_executable(test_logger
        src/test_logger.cpp
        src/core/logger.cpp
    )
    target_include_directories(test_logger PRIVATE src)
endif()

# Build tests (cmake .. -DBUILD_TESTS=ON)
# This enables the unit test suite located in the tests/ directory.
# Tests can be run with: make test
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()