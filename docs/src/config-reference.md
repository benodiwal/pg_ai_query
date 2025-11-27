# Configuration File Reference

This page provides a complete reference for all configuration options available in the `pg_ai_query` extension configuration file.

## Configuration File Location

The configuration file must be placed at:
```
~/.pg_ai.config
```

## File Format

The configuration uses INI format with sections and key-value pairs:

```ini
# Comments start with # and are ignored
[section_name]
key = value
key_with_quotes = "value with spaces"
```

## Complete Configuration Template

```ini
# PG AI Query Configuration File
# Place this file at ~/.pg_ai.config

[general]
# Logging configuration
log_level = "INFO"
enable_logging = false
enable_postgresql_elog = true

# Request configuration
request_timeout_ms = 30000
max_retries = 3

[query]
# Query generation behavior
enforce_limit = true
default_limit = 1000

[openai]
# OpenAI provider configuration
api_key = ""
default_model = "gpt-4o"

[anthropic]
# Anthropic provider configuration
api_key = ""
default_model = "claude-sonnet-4-5-20250929"
```

## Configuration Sections

### [general] Section

Controls general extension behavior and logging.

| Option | Type | Default | Range/Values | Description |
|--------|------|---------|--------------|-------------|
| `log_level` | string | "INFO" | "DEBUG", "INFO", "WARNING", "ERROR" | Minimum log level for messages |
| `enable_logging` | boolean | false | true, false | Enable/disable all logging output |
| `enable_postgresql_elog` | boolean | true | true, false | Use PostgreSQL's elog system for logging |
| `request_timeout_ms` | integer | 30000 | 1000-300000 | Timeout for AI API requests in milliseconds |
| `max_retries` | integer | 3 | 0-10 | Maximum retry attempts for failed requests |

#### log_level

Controls the verbosity of log messages.

**Values:**
- `"DEBUG"`: Most verbose, includes all internal operations
- `"INFO"`: General information about operations
- `"WARNING"`: Warnings about potential issues
- `"ERROR"`: Only error messages

**Example:**
```ini
[general]
log_level = "DEBUG"  # Show all log messages
```

#### enable_logging

Master switch for all logging output.

**Values:**
- `true`: Enable logging (respects log_level)
- `false`: Disable all logging output

**Example:**
```ini
[general]
enable_logging = true  # Turn on logging
log_level = "INFO"     # Show INFO level and above
```

#### enable_postgresql_elog

Controls whether to use PostgreSQL's internal logging system.

**Values:**
- `true`: Use PostgreSQL's elog system (recommended)
- `false`: Use alternative logging method

**Example:**
```ini
[general]
enable_postgresql_elog = true  # Use PostgreSQL logging
```

#### request_timeout_ms

Timeout for AI API requests in milliseconds.

**Range:** 1000-300000 (1 second to 5 minutes)
**Recommended:** 30000-60000 for most use cases

**Example:**
```ini
[general]
request_timeout_ms = 45000  # 45 second timeout
```

#### max_retries

Maximum number of retry attempts for failed API requests.

**Range:** 0-10
**Recommended:** 3-5 for production use

**Example:**
```ini
[general]
max_retries = 5  # Retry up to 5 times
```

### [query] Section

Controls query generation behavior and safety features.

| Option | Type | Default | Range/Values | Description |
|--------|------|---------|--------------|-------------|
| `enforce_limit` | boolean | true | true, false | Always add LIMIT clause to SELECT queries |
| `default_limit` | integer | 1000 | 1-1000000 | Default row limit when none specified |

#### enforce_limit

Controls whether SELECT queries automatically include LIMIT clauses.

**Values:**
- `true`: Always add LIMIT to SELECT queries (recommended for safety)
- `false`: Allow unlimited SELECT queries

**Example:**
```ini
[query]
enforce_limit = true    # Always limit query results
default_limit = 500     # Default to 500 rows
```

#### default_limit

Default number of rows to limit when no explicit limit is requested.

**Range:** 1-1000000
**Recommended:** 100-5000 depending on use case

**Example:**
```ini
[query]
default_limit = 2000  # Default to 2000 rows
```

### [openai] Section

Configuration for OpenAI provider.

| Option | Type | Default | Values | Description |
|--------|------|---------|--------|-------------|
| `api_key` | string | "" | API key format | Your OpenAI API key |
| `default_model` | string | "gpt-4o" | Any valid model name | Default OpenAI model to use |

#### api_key

Your OpenAI API key from platform.openai.com.

**Format:** Must start with `sk-` or `sk-proj-`
**Security:** Keep this key secure and never commit to version control

**Example:**
```ini
[openai]
api_key = "sk-proj-abc123..."  # Your actual API key
```

#### default_model

Default OpenAI model to use for query generation.

**Available Models:**
- `"gpt-4o"`: Latest GPT-4 Omni model (recommended)
- `"gpt-4"`: Standard GPT-4 model
- `"gpt-3.5-turbo"`: Fast and economical model

**Example:**
```ini
[openai]
default_model = "gpt-4o"  # Use latest model
```

### [anthropic] Section

Configuration for Anthropic (Claude) provider.

| Option | Type | Default | Values | Description |
|--------|------|---------|--------|-------------|
| `api_key` | string | "" | API key format | Your Anthropic API key |
| `default_model` | string | "claude-sonnet-4-5-20250929" | Any valid model name | Default Claude model to use |

#### api_key

Your Anthropic API key from console.anthropic.com.

**Format:** Must start with `sk-ant-`
**Security:** Keep this key secure and never commit to version control

**Example:**
```ini
[anthropic]
api_key = "sk-ant-abc123..."  # Your actual API key
```

#### default_model

Default Anthropic model to use for query generation.

**Available Models:**
- `"claude-sonnet-4-5-20250929"`: Latest Claude 3.5 Sonnet model

**Example:**
```ini
[anthropic]
default_model = "claude-sonnet-4-5-20250929"  # Use Claude 3.5 Sonnet
```

## Configuration Examples

### Development Configuration

```ini
# Development setup with detailed logging
[general]
log_level = "DEBUG"
enable_logging = true
enable_postgresql_elog = true
request_timeout_ms = 60000  # Longer timeout for debugging
max_retries = 2             # Fewer retries for faster feedback

[query]
enforce_limit = true
default_limit = 100         # Small limit for testing

[openai]
api_key = "sk-proj-dev-key-here"
default_model = "gpt-3.5-turbo"  # Cheaper for development
```

### Production Configuration

```ini
# Production setup optimized for performance and reliability
[general]
log_level = "WARNING"
enable_logging = false      # Disable for performance
enable_postgresql_elog = true
request_timeout_ms = 30000
max_retries = 5             # More retries for reliability

[query]
enforce_limit = true
default_limit = 1000

[openai]
api_key = "sk-proj-prod-key-here"
default_model = "gpt-4"     # Good balance of quality and cost
```

### Multi-Provider Configuration

```ini
# Setup with both providers for redundancy
[general]
log_level = "INFO"
enable_logging = true
request_timeout_ms = 45000
max_retries = 3

[query]
enforce_limit = true
default_limit = 500

[openai]
api_key = "sk-proj-openai-key-here"
default_model = "gpt-4o"

[anthropic]
api_key = "sk-ant-anthropic-key-here"
default_model = "claude-sonnet-4-5-20250929"

# Extension will use OpenAI first (first configured)
# Falls back to Anthropic if OpenAI fails
```

### High-Performance Configuration

```ini
# Optimized for speed and low cost
[general]
log_level = "ERROR"         # Minimal logging
enable_logging = false
request_timeout_ms = 15000  # Shorter timeout
max_retries = 2             # Fewer retries

[query]
enforce_limit = true
default_limit = 200         # Smaller default limit

[openai]
api_key = "sk-proj-key-here"
default_model = "gpt-3.5-turbo"  # Fastest model
```

## Configuration Validation

### File Format Validation

The extension validates:
- **INI Format**: Proper section headers and key-value pairs
- **Required Quotes**: String values with spaces must be quoted
- **Section Names**: Only valid section names are accepted
- **Key Names**: Only valid configuration keys are recognized

### Value Validation

Each configuration value is validated:
- **Type Checking**: Strings, integers, and booleans are validated
- **Range Checking**: Numeric values must be within acceptable ranges
- **Format Checking**: API keys must match expected formats
- **Model Validation**: Accepts any valid model name string

### Error Handling

Configuration errors are reported with specific messages:

```
Configuration Error: Invalid log_level 'VERBOSE' in section [general]. Valid values are: DEBUG, INFO, WARNING, ERROR
Configuration Error: request_timeout_ms must be between 1000 and 300000, got 500000
Configuration Error: OpenAI API key must start with 'sk-' or 'sk-proj-'
```

## Environment Variables

In addition to the configuration file, API keys can be provided via environment variables.

| Environment Variable | Description |
|----------------------|-------------|
| `OPENAI_API_KEY` | API key for OpenAI |
| `ANTHROPIC_API_KEY` | API key for Anthropic |

**Note:** Environment variables take precedence over values specified in the configuration file.

### Example Usage

```bash
export OPENAI_API_KEY="sk-..."
export ANTHROPIC_API_KEY="sk-ant-..."
```

## Security Considerations

### File Permissions

Set secure permissions on the configuration file:

```bash
# Make file readable only by owner
chmod 600 ~/.pg_ai.config

# Verify permissions
ls -la ~/.pg_ai.config
# Should show: -rw------- 1 user user ...
```

### API Key Security

- **Never commit** configuration files with API keys to version control
- **Use secure configuration files** in containerized or CI/CD environments
- **Rotate keys regularly** as per your organization's security policy
- **Monitor usage** through your AI provider's dashboard

### Configuration Templates

Use template files for version control:

```ini
# .pg_ai.config.template
[general]
log_level = "INFO"
enable_logging = false

[openai]
api_key = "${OPENAI_API_KEY}"  # Replace with actual key
default_model = "gpt-4o"

[anthropic]
api_key = "${ANTHROPIC_API_KEY}"  # Replace with actual key
```

### Configuration Loading Order

The extension loads configuration in this order (last one wins):

1. **Default values** (hardcoded)
2. **Configuration file** (`~/.pg_ai.config`)
3. **Environment variables** (`OPENAI_API_KEY`, `ANTHROPIC_API_KEY`)

This allows you to override file-based configuration with environment variables for specific runs or in containerized environments.