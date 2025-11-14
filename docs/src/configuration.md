# Configuration

The `pg_ai_query` extension supports flexible configuration through a configuration file that allows you to customize AI providers, API keys, logging, and query behavior.

## Configuration File Location

The configuration file should be placed at:

```
~/.pg_ai.config
```

This file will be automatically detected and loaded when the extension is first used.

## Configuration File Format

The configuration uses an INI-style format with sections and key-value pairs:

```ini
# PG AI Query Configuration File
[section_name]
key = value
```

## Complete Configuration Example

Here's a complete example configuration file with all available options:

```ini
# PG AI Query Configuration File
# Place this file at ~/.pg_ai.config

[general]
# Logging level: DEBUG, INFO, WARNING, ERROR
log_level = "INFO"

# Enable or disable all logging output (true/false)
enable_logging = false

# Request timeout in milliseconds
request_timeout_ms = 30000

# Maximum number of retries for failed requests
max_retries = 3

[query]
# Always enforce LIMIT clause on SELECT queries
enforce_limit = true

# Default LIMIT value when not specified by user
default_limit = 1000

[response]
# Show detailed explanation of what the query does
show_explanation = true

# Show warnings about performance, security, or data implications
show_warnings = true

# Show suggested visualization type for query results
show_suggested_visualization = true

# Use formatted response (JSON format) instead of plain SQL
# When enabled, returns structured JSON with query, explanation, warnings, etc.
# When disabled, returns plain SQL with optional comments
use_formatted_response = false

[openai]
# Your OpenAI API key
api_key = "sk-your-openai-api-key-here"

# Default model to use (options: gpt-4o, gpt-4, gpt-3.5-turbo)
default_model = "gpt-4o"

[anthropic]
# Your Anthropic API key (if using Claude)
api_key = "sk-ant-your-anthropic-api-key-here"

# Default model to use (options: claude-3-5-sonnet-20241022)
default_model = "claude-3-5-sonnet-20241022"
```

## Configuration Sections

### [general] Section

Controls general behavior of the extension.

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `log_level` | string | "INFO" | Minimum level for log messages: DEBUG, INFO, WARNING, ERROR |
| `enable_logging` | boolean | false | Enable/disable all logging output |
| `request_timeout_ms` | integer | 30000 | Timeout for AI API requests in milliseconds |
| `max_retries` | integer | 3 | Maximum retry attempts for failed API requests |

### [query] Section

Controls query generation behavior.

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `enforce_limit` | boolean | true | Always add LIMIT clause to SELECT queries |
| `default_limit` | integer | 1000 | Default row limit when none specified |

### [response] Section

Controls how query results are formatted and what additional information is included.

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `show_explanation` | boolean | true | Include detailed explanation of what the query does |
| `show_warnings` | boolean | true | Include warnings about performance, security, or data implications |
| `show_suggested_visualization` | boolean | false | Include suggested visualization type for the query results |
| `use_formatted_response` | boolean | false | Return structured JSON instead of plain SQL |

### [openai] Section

OpenAI provider configuration.

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `api_key` | string | "" | Your OpenAI API key from platform.openai.com |
| `default_model` | string | "gpt-4o" | Default OpenAI model to use |

**Available OpenAI Models:**
- `gpt-4o` - Latest GPT-4 Omni model (recommended)
- `gpt-4` - High-quality GPT-4 model
- `gpt-3.5-turbo` - Fast and efficient model

### [anthropic] Section

Anthropic (Claude) provider configuration.

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `api_key` | string | "" | Your Anthropic API key from console.anthropic.com |
| `default_model` | string | "claude-3-5-sonnet-20241022" | Default Claude model to use |

**Available Anthropic Models:**
- `claude-3-5-sonnet-20241022` - Latest Claude 3.5 Sonnet model

## Setting Up API Keys

### Getting an OpenAI API Key

1. Visit [platform.openai.com](https://platform.openai.com)
2. Create an account or sign in
3. Navigate to API Keys section
4. Create a new API key
5. Copy the key and add it to your config file

### Getting an Anthropic API Key

1. Visit [console.anthropic.com](https://console.anthropic.com)
2. Create an account or sign in
3. Navigate to API Keys section
4. Create a new API key
5. Copy the key and add it to your config file

## Provider Selection Priority

The extension automatically selects an AI provider based on the following priority:

1. **Explicit provider parameter** in function call
2. **First configured provider** with a valid API key in config file
3. **Error** if no providers are configured

## Configuration Validation

The extension validates configuration on startup:

- **API Key Format**: Checks that API keys follow expected format
- **Model Availability**: Validates that specified models are available
- **Numeric Values**: Ensures timeouts and limits are positive integers
- **Boolean Values**: Validates true/false values

## Environment Variables

You can also set configuration via environment variables (config file takes precedence):

```bash
export PG_AI_OPENAI_API_KEY="your-key-here"
export PG_AI_ANTHROPIC_API_KEY="your-key-here"
export PG_AI_ENABLE_LOGGING="true"
```

## Security Considerations

### API Key Security

- **Never commit** API keys to version control
- **Use appropriate file permissions**: `chmod 600 ~/.pg_ai.config`
- **Rotate keys regularly** as per your organization's security policy
- **Monitor usage** through your AI provider's dashboard

### Network Security

- API requests are made over HTTPS
- Consider firewall rules for outbound connections to AI providers
- Monitor network traffic for unexpected API usage

### Database Security

- The extension only accesses user tables (not system catalogs)
- Generated queries respect PostgreSQL's permission system
- Consider using dedicated database users with limited privileges

## Debugging Configuration

### Enable Logging

To troubleshoot configuration issues, enable logging:

```ini
[general]
enable_logging = true
log_level = "DEBUG"
```

### Check Configuration Loading

You can verify configuration loading by observing log messages when first calling the extension:

```sql
-- This will log configuration loading details
SELECT generate_query('test query');
```

### Validate API Keys

Test your API configuration:

```sql
-- Test with explicit API key
SELECT generate_query('show tables', 'your-api-key-here', 'openai');
```

## Updating Configuration

Configuration is loaded on first use. To reload after making changes:

1. **Disconnect** from PostgreSQL
2. **Reconnect** to your database
3. **Call any extension function** to trigger reload

Or restart your PostgreSQL session.

## Next Steps

Once configured:

1. Follow the [Quick Start Guide](./quick-start.md) to test your setup
2. Explore [Usage Examples](./examples.md)
3. Learn about [AI Providers](./providers.md) to optimize your model selection