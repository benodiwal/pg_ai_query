# API Reference

This page provides complete technical reference for the `pg_ai_query` extension API.

## Extension Information

| Property | Value |
|----------|-------|
| **Name** | `pg_ai_query` |
| **Version** | 1.0 |
| **Schema** | `public` |
| **Dependencies** | PostgreSQL 12+ |

## SQL API

### CREATE EXTENSION

```sql
CREATE EXTENSION [IF NOT EXISTS] pg_ai_query [WITH] [SCHEMA schema_name];
```

Creates the extension and installs all functions.

**Example:**
```sql
CREATE EXTENSION IF NOT EXISTS pg_ai_query;
```

### DROP EXTENSION

```sql
DROP EXTENSION [IF EXISTS] pg_ai_query [CASCADE | RESTRICT];
```

Removes the extension and all its functions.

**Example:**
```sql
DROP EXTENSION IF EXISTS pg_ai_query CASCADE;
```

## Core Functions API

### generate_query()

**Signature:**
```sql
generate_query(
    natural_language_query text,
    api_key text DEFAULT NULL,
    provider text DEFAULT 'auto'
) RETURNS text
```

**Parameters:**

| Parameter | Type | Constraints | Description |
|-----------|------|-------------|-------------|
| `natural_language_query` | `text` | NOT NULL, max 4096 chars | Natural language description of desired query |
| `api_key` | `text` | NULL allowed | API key for AI provider (optional if configured) |
| `provider` | `text` | 'openai', 'anthropic', 'auto' | AI provider selection |

**Returns:**
- **Type:** `text`
- **Format:** Valid PostgreSQL SQL query
- **Constraints:** Always includes LIMIT clause for SELECT statements

**Exceptions:**
- `EXTERNAL_ROUTINE_EXCEPTION`: AI API communication failures
- `INVALID_PARAMETER_VALUE`: Invalid provider or malformed input
- `CONFIGURATION_FILE_ERROR`: Configuration issues

**Example:**
```sql
SELECT generate_query('show top 10 customers by revenue');
```

---

### get_database_tables()

**Signature:**
```sql
get_database_tables() RETURNS text
```

**Parameters:** None

**Returns:**
- **Type:** `text`
- **Format:** JSON array
- **Content:** Table metadata objects

**JSON Schema:**
```json
{
  "type": "array",
  "items": {
    "type": "object",
    "properties": {
      "table_name": {"type": "string"},
      "schema_name": {"type": "string"},
      "table_type": {"type": "string"},
      "estimated_rows": {"type": "integer"},
      "table_size": {"type": "string"}
    },
    "required": ["table_name", "schema_name", "table_type"]
  }
}
```

**Exceptions:**
- `EXTERNAL_ROUTINE_EXCEPTION`: Database introspection failures

**Example:**
```sql
SELECT get_database_tables();
```

---

### get_table_details()

**Signature:**
```sql
get_table_details(
    table_name text,
    schema_name text DEFAULT 'public'
) RETURNS text
```

**Parameters:**

| Parameter | Type | Constraints | Description |
|-----------|------|-------------|-------------|
| `table_name` | `text` | NOT NULL, must exist | Name of table to analyze |
| `schema_name` | `text` | Valid schema name | Schema containing the table |

**Returns:**
- **Type:** `text`
- **Format:** JSON object
- **Content:** Detailed table structure information

**JSON Schema:**
```json
{
  "type": "object",
  "properties": {
    "table_name": {"type": "string"},
    "schema_name": {"type": "string"},
    "columns": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "column_name": {"type": "string"},
          "data_type": {"type": "string"},
          "is_nullable": {"type": "boolean"},
          "column_default": {"type": ["string", "null"]},
          "character_maximum_length": {"type": ["integer", "null"]},
          "numeric_precision": {"type": ["integer", "null"]},
          "is_primary_key": {"type": "boolean"},
          "is_unique": {"type": "boolean"}
        }
      }
    },
    "constraints": {"type": "array"},
    "foreign_keys": {"type": "array"},
    "indexes": {"type": "array"}
  }
}
```

**Exceptions:**
- `INVALID_PARAMETER_VALUE`: Table does not exist
- `EXTERNAL_ROUTINE_EXCEPTION`: Database introspection failures

**Example:**
```sql
SELECT get_table_details('users', 'public');
```

## Configuration API

### Configuration File Location

The extension reads configuration from:
```
~/.pg_ai.config
```

### Configuration Structure

```ini
[general]
# General settings
log_level = "INFO" | "DEBUG" | "WARNING" | "ERROR"
enable_logging = true | false
enable_postgresql_elog = true | false
request_timeout_ms = <integer>
max_retries = <integer>

[query]
# Query generation settings
enforce_limit = true | false
default_limit = <integer>

[openai]
# OpenAI provider settings
api_key = "<api_key_string>"
default_model = "gpt-4o" | "gpt-4" | "gpt-3.5-turbo"

[anthropic]
# Anthropic provider settings
api_key = "<api_key_string>"
default_model = "claude-3-5-sonnet-20241022"
```

### Configuration Validation Rules

| Setting | Type | Range/Values | Default |
|---------|------|--------------|---------|
| `log_level` | string | DEBUG, INFO, WARNING, ERROR | INFO |
| `enable_logging` | boolean | true, false | false |
| `enable_postgresql_elog` | boolean | true, false | true |
| `request_timeout_ms` | integer | 1000-300000 | 30000 |
| `max_retries` | integer | 0-10 | 3 |
| `enforce_limit` | boolean | true, false | true |
| `default_limit` | integer | 1-1000000 | 1000 |
| `api_key` | string | Provider-specific format | "" |
| `default_model` | string | Provider-specific values | Provider default |

## Error Codes

### Extension Error Codes

| Code | Name | Description |
|------|------|-------------|
| `22023` | `INVALID_PARAMETER_VALUE` | Invalid function parameter |
| `38001` | `CONTAINING_SQL_NOT_PERMITTED` | Unsafe SQL operation attempted |
| `38003` | `PROHIBITED_SQL_STATEMENT_ATTEMPTED` | Forbidden query type |
| `39001` | `EXTERNAL_ROUTINE_EXCEPTION` | AI API communication error |
| `58030` | `IO_ERROR` | File system or network I/O error |
| `XX000` | `INTERNAL_ERROR` | Unexpected internal error |

### AI Provider Error Codes

#### OpenAI Errors
| HTTP Code | Error Type | Description |
|-----------|------------|-------------|
| 400 | `invalid_request_error` | Invalid request parameters |
| 401 | `authentication_error` | Invalid API key |
| 403 | `permission_error` | Insufficient permissions |
| 429 | `rate_limit_error` | Rate limit exceeded |
| 500 | `api_error` | OpenAI server error |

#### Anthropic Errors
| HTTP Code | Error Type | Description |
|-----------|------------|-------------|
| 400 | `invalid_request_error` | Invalid request format |
| 401 | `authentication_error` | Invalid API key |
| 403 | `permission_error` | Access denied |
| 429 | `rate_limit_error` | Rate limit exceeded |
| 500 | `api_error` | Anthropic server error |

## Internal APIs

### Schema Discovery Engine

The extension uses these internal functions for schema analysis:

```sql
-- Internal function signatures (not callable by users)
_pg_ai_analyze_schema() RETURNS schema_info;
_pg_ai_get_table_relationships() RETURNS relationship_map;
_pg_ai_validate_query(query text) RETURNS boolean;
```

### Query Generation Pipeline

1. **Input Validation**: Validates natural language input
2. **Schema Discovery**: Analyzes database structure
3. **Context Building**: Creates AI prompt with schema context
4. **AI Request**: Sends request to configured provider
5. **Response Parsing**: Extracts SQL from AI response
6. **Query Validation**: Validates generated SQL for safety
7. **Result Return**: Returns validated SQL query

### Security Validation

The extension performs these security checks:

- **SQL Injection Prevention**: Sanitizes all inputs
- **System Table Protection**: Blocks access to pg_* and information_schema
- **DDL Restriction**: Can generate DDL but with warnings
- **Privilege Respect**: Honors PostgreSQL permission system

## Performance Characteristics

### Memory Usage

| Operation | Memory Usage | Duration |
|-----------|--------------|----------|
| Schema Analysis | ~1MB per 100 tables | 100-500ms |
| Query Generation | ~5-10MB per request | 1-5 seconds |
| Configuration Load | ~1KB | <10ms |

### Network Requirements

| Provider | Endpoint | Protocol | Bandwidth |
|----------|----------|----------|-----------|
| OpenAI | `api.openai.com` | HTTPS | 1-10KB per request |
| Anthropic | `api.anthropic.com` | HTTPS | 1-10KB per request |

### Caching Behavior

- **Schema Information**: Cached per PostgreSQL session
- **Configuration**: Cached until session restart
- **AI Responses**: Not cached (each request is fresh)

## Extension Metadata

### System Catalog Integration

The extension registers these entries:

```sql
-- Extension registration
SELECT * FROM pg_extension WHERE extname = 'pg_ai_query';

-- Function registration
SELECT * FROM pg_proc WHERE proname LIKE '%generate%';

-- Dependencies
SELECT * FROM pg_depend WHERE refobjid IN (
    SELECT oid FROM pg_extension WHERE extname = 'pg_ai_query'
);
```

### Version Information

```sql
-- Check extension version
SELECT extversion FROM pg_extension WHERE extname = 'pg_ai_query';

-- Function definitions
\df+ generate_query
\df+ get_database_tables
\df+ get_table_details
```

## Compatibility

### PostgreSQL Versions

| PostgreSQL Version | Support Status | Notes |
|-------------------|----------------|-------|
| 12.x | ✅ Supported | Minimum required version |
| 13.x | ✅ Supported | Full compatibility |
| 14.x | ✅ Supported | Recommended |
| 15.x | ✅ Supported | Full compatibility |
| 16.x | ✅ Supported | Latest tested |

### Operating Systems

| OS | Support Status | Notes |
|----|----------------|-------|
| Linux (RHEL/CentOS) | ✅ Supported | Primary development platform |
| Linux (Ubuntu/Debian) | ✅ Supported | Well tested |
| macOS | ✅ Supported | Development and testing |
| Windows | ⚠️ Experimental | Limited testing |

### AI Provider Compatibility

| Provider | API Version | Models Supported |
|----------|-------------|------------------|
| OpenAI | v1 | GPT-4o, GPT-4, GPT-3.5-turbo |
| Anthropic | v1 | Claude 3.5 Sonnet |

This completes the technical API reference for the pg_ai_query extension.