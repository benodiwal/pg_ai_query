# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [v0.1.0-beta] - 2024-12-08

### Added

#### Core Functions
- `generate_query(text, text, text)` - Generate SQL queries from natural language descriptions
  - Automatic database schema discovery
  - Support for provider selection (`openai`, `anthropic`, `auto`)
  - Optional inline API key parameter
- `explain_query(text, text, text)` - AI-powered query plan analysis
  - Runs `EXPLAIN ANALYZE` and provides human-readable explanations
  - Performance insights and optimization suggestions
- `get_database_tables()` - Returns JSON array of all user tables with metadata
- `get_table_details(text, text)` - Returns detailed table information including columns, constraints, and indexes

#### AI Provider Support
- **OpenAI** - All current supported models
- **Anthropic** - All current supported models
- **OpenAI-compatible APIs** - Support for OpenRouter and other compatible endpoints
- Automatic provider selection with fallback logic
- Configurable retry policies with exponential backoff

#### Configuration
- File-based configuration via `~/.pg_ai.config`
- Support for multiple AI providers in single config
- Configurable options:
  - Log level and logging toggle
  - Request timeout and retry settings
  - Query limit enforcement
  - Response formatting (explanations, warnings, visualizations)
  - Custom API endpoints for OpenAI-compatible services

#### Documentation
- Comprehensive mdBook documentation
- Installation and configuration guides
- Provider comparison and model selection guide
- Usage examples and troubleshooting

### Technical Details

- Built with C++20
- Uses [ai-sdk-cpp](https://github.com/clickhouse/ai-sdk-cpp) for AI provider integration
- SSL/TLS support via OpenSSL
- PostgreSQL extension API compliance

---

[v0.1.0-beta]: https://github.com/benodiwal/pg_ai_query/releases/tag/v0.1.0-beta
