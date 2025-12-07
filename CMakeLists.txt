cmake_minimum_required(VERSION 3.16)
project(pg_ai_query)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find PostgreSQL
find_program(PG_CONFIG pg_config REQUIRED)
execute_process(COMMAND ${PG_CONFIG} --includedir-server OUTPUT_VARIABLE PG_INCLUDE_SERVER OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${PG_CONFIG} --includedir OUTPUT_VARIABLE PG_INCLUDE OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${PG_CONFIG} --pkglibdir OUTPUT_VARIABLE PG_PKGLIB OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${PG_CONFIG} --libdir OUTPUT_VARIABLE PG_LIBDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${PG_CONFIG} --bindir OUTPUT_VARIABLE PG_BINDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${PG_CONFIG} --sharedir OUTPUT_VARIABLE PG_SHAREDIR OUTPUT_STRIP_TRAILING_WHITESPACE)

# Include PostgreSQL headers
include_directories(${PG_INCLUDE_SERVER})
include_directories(${PG_INCLUDE})

# Configure ai-sdk-cpp options to use static libraries only
set(AI_SDK_BUILD_EXAMPLES OFF CACHE BOOL "")
set(AI_SDK_BUILD_TESTS OFF CACHE BOOL "")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
set(AI_SDK_STATIC_ONLY ON CACHE BOOL "")

# Add the ai-sdk-cpp submodule
add_subdirectory(third_party/ai-sdk-cpp)

# Add Gemini client library
add_subdirectory(third_party/gemini)

# Include our source directories
include_directories(src)
include_directories(third_party/gemini/include)

# Source files for the clean implementation
set(SOURCES
    src/pg_ai_query.cpp
    src/core/query_generator.cpp
    src/core/response_formatter.cpp
    src/core/logger.cpp
    src/utils.cpp
    src/prompts.cpp
    src/config.cpp
)

# Create module library for PostgreSQL extension
# Get PostgreSQL version for extension naming
execute_process(COMMAND ${PG_CONFIG} --version OUTPUT_VARIABLE PG_VERSION_OUTPUT OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REGEX MATCH "PostgreSQL ([0-9]+)" PG_VERSION_MATCH ${PG_VERSION_OUTPUT})
set(PG_MAJOR_VERSION ${CMAKE_MATCH_1})

if(APPLE)
    add_library(pg_ai_query MODULE ${SOURCES})
    # PostgreSQL 16+ on macOS expects .dylib, earlier versions expect .so
    if(PG_MAJOR_VERSION GREATER_EQUAL 16)
        set(PG_EXTENSION_SUFFIX ".dylib")
    else()
        set(PG_EXTENSION_SUFFIX ".so")
    endif()
    set_target_properties(pg_ai_query PROPERTIES
        PREFIX ""
        SUFFIX ${PG_EXTENSION_SUFFIX}
        LINK_FLAGS "-bundle_loader ${PG_BINDIR}/postgres"
    )
    message(STATUS "Building PostgreSQL ${PG_MAJOR_VERSION} extension with suffix: ${PG_EXTENSION_SUFFIX}")
else()
    add_library(pg_ai_query SHARED ${SOURCES})
    set_target_properties(pg_ai_query PROPERTIES
        PREFIX ""
        SUFFIX ".so"
        POSITION_INDEPENDENT_CODE ON
    )
endif()

# Find required system libraries
find_package(OpenSSL REQUIRED)

# Find gettext for NLS support (required by PostgreSQL)
find_package(Intl)
if(Intl_FOUND)
    message(STATUS "Found Intl library: ${Intl_LIBRARIES}")
else()
    # Try to find gettext manually on macOS
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

# Link with ai-sdk-cpp static libraries to avoid RPATH issues
target_link_libraries(pg_ai_query PRIVATE
    ai-sdk-cpp-core
    ai-sdk-cpp-openai
    ai-sdk-cpp-anthropic
    gemini-client
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Add system zlib separately (will be added by macOS section if needed)

# Link Intl library if found
if(Intl_FOUND)
    target_link_libraries(pg_ai_query PRIVATE ${Intl_LIBRARIES})
    target_include_directories(pg_ai_query PRIVATE ${Intl_INCLUDE_DIRS})
endif()

# Set RPATH for macOS to find system libraries
if(APPLE)
    # Get the system library path where libz.1.dylib actually exists
    execute_process(COMMAND find /usr/lib /System/Library/Frameworks -name "libz.1.dylib" -type f 2>/dev/null | head -1
        OUTPUT_VARIABLE LIBZ_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
    get_filename_component(LIBZ_DIR "${LIBZ_PATH}" DIRECTORY)

    set_target_properties(pg_ai_query PROPERTIES
        INSTALL_RPATH "@loader_path;/usr/lib;/System/Library/Frameworks;${LIBZ_DIR};/opt/homebrew/lib;/usr/local/lib"
        BUILD_RPATH "@loader_path;/usr/lib;/System/Library/Frameworks;${LIBZ_DIR};/opt/homebrew/lib;/usr/local/lib"
        MACOSX_RPATH ON
        # Force static linking to avoid RPATH issues
        LINK_SEARCH_START_STATIC ON
        LINK_SEARCH_END_STATIC ON
    )

    # Find and link system zlib to avoid duplicates
    find_library(SYSTEM_Z z REQUIRED)
    if(SYSTEM_Z)
        target_link_libraries(pg_ai_query PRIVATE ${SYSTEM_Z})
        message(STATUS "Using system zlib at: ${SYSTEM_Z}")
    else()
        # Fallback to standard zlib
        find_package(ZLIB REQUIRED)
        target_link_libraries(pg_ai_query PRIVATE ZLIB::ZLIB)
        message(STATUS "Using standard zlib package")
    endif()

    message(STATUS "Found libz.1.dylib at: ${LIBZ_PATH}")
endif()

# Include directories
target_include_directories(pg_ai_query PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ai-sdk-cpp/include
)

# Add compile definitions
target_compile_definitions(pg_ai_query PRIVATE
    USE_POSTGRESQL_ELOG
)

# Get PostgreSQL version to handle path differences
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

# Install prompts directory
install(DIRECTORY prompts/
    DESTINATION ${PG_SHAREDIR}/extension/prompts/
    FILES_MATCHING PATTERN "*.txt"
)

# Optional: Build logger test (not built by default)
# Uncomment to build: cmake .. -DBUILD_LOGGER_TEST=ON
option(BUILD_LOGGER_TEST "Build logger test executable" OFF)
if(BUILD_LOGGER_TEST)
    add_executable(test_logger
        src/test_logger.cpp
        src/core/logger.cpp
    )
    target_include_directories(test_logger PRIVATE src)
endif()