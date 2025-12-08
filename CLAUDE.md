# pg_ai_query - Claude Code Instructions

## Project Overview

This is a PostgreSQL extension written in C++20 that generates SQL queries from natural language using AI models (OpenAI and Anthropic).

## Tech Stack

- **Language**: C++20
- **Build System**: CMake 3.16+
- **Database**: PostgreSQL 14+ extension
- **Dependencies**: ai-sdk-cpp (git submodule in `third_party/`)

## Project Structure

```
src/
├── include/          # Header files (.hpp)
├── core/             # Core implementation
│   ├── ai_client_factory.cpp
│   ├── provider_selector.cpp
│   ├── query_generator.cpp
│   ├── response_formatter.cpp
│   ├── spi_connection.cpp
│   └── logger.cpp
├── config.cpp        # Configuration handling
├── prompts.cpp       # AI prompt templates
├── utils.cpp         # Utility functions
└── pg_ai_query.cpp   # Main extension entry point

sql/
└── pg_ai_query--1.0.sql  # PostgreSQL extension SQL definitions

docs/                 # mdbook documentation
```

## Build Commands

```bash
# Full build
mkdir -p build && cd build && cmake .. && make

# Install to PostgreSQL
sudo make install

# Clean build
make clean
```

## Code Style

- Follow `.clang-format` configuration
- Use C++20 features where appropriate
- Header files use `.hpp` extension
- Implementation files use `.cpp` extension
- Use `#pragma once` for header guards

## Key Conventions

1. **Error Handling**: Use PostgreSQL's `ereport()` for errors
2. **Memory Management**: Use PostgreSQL's memory contexts (`palloc`, `pfree`)
3. **String Handling**: Use PostgreSQL's text handling functions
4. **Logging**: Use the custom Logger class in `src/core/logger.cpp`

## PostgreSQL Extension Guidelines

- All exported functions must be declared with `PG_FUNCTION_INFO_V1()`
- Use SPI (Server Programming Interface) for database queries
- Configuration is read from `~/.pg_ai.config`
- Environment variables `OPENAI_API_KEY` and `ANTHROPIC_API_KEY` override config

## Testing

```bash
# Connect to PostgreSQL and test
psql -d your_database -c "CREATE EXTENSION IF NOT EXISTS pg_ai_query;"
psql -d your_database -c "SELECT generate_query('show all users');"
```

## Important Files

- `CMakeLists.txt` - Build configuration
- `sql/pg_ai_query--1.0.sql` - Extension SQL definitions
- `src/pg_ai_query.cpp` - Main entry point with PG functions
- `src/core/query_generator.cpp` - Query generation logic
- `src/prompts.cpp` - AI prompt templates

## Security Considerations

- Never expose API keys in code or logs
- Block access to system tables (`information_schema`, `pg_catalog`)
- Validate and sanitize all user inputs
- Enforce query limits to prevent resource exhaustion

## When Making Changes

1. Always read the relevant header file before modifying implementation
2. Check `src/include/` for type definitions and interfaces
3. Run `cmake ..` after modifying `CMakeLists.txt`
4. Test changes with actual PostgreSQL queries
5. Ensure backwards compatibility with PostgreSQL 14+
