# generate_query Function

The `generate_query` function converts natural language descriptions into valid PostgreSQL queries using AI models from OpenAI and Anthropic.

## Function Signature

```sql
generate_query(
    natural_language_query text,
    api_key text DEFAULT NULL,
    provider text DEFAULT 'auto'
) RETURNS text
```

## Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `natural_language_query` | `text` | *required* | The natural language description of the desired query |
| `api_key` | `text` | `NULL` | OpenAI or Anthropic API key (uses config if not provided) |
| `provider` | `text` | `'auto'` | AI provider: `'openai'`, `'anthropic'`, or `'auto'` |

## Basic Usage

### Simple Queries

```sql
-- Basic data retrieval
SELECT generate_query('show all users');

-- With filtering
SELECT generate_query('find users created in the last week');

-- Counting records
SELECT generate_query('count total orders');
```

### Complex Queries

```sql
-- Joins and aggregation
SELECT generate_query('show top 10 customers by total order value with their email addresses');

-- Date-based analysis
SELECT generate_query('monthly revenue trend for the last year');
```

## Configuration

### API Key Setup

Create `~/.pg_ai.config`:

```ini
[openai]
api_key = "sk-your-openai-api-key"
default_model = "gpt-4o"

[anthropic]
api_key = "sk-ant-your-anthropic-key"
default_model = "claude-sonnet-4-5-20250929"
```

## See Also

- [explain_query Function](./explain-query.md) - Analyze query performance
- [Examples](./examples.md) - More usage examples
- [Error Codes](./error-codes.md) - Troubleshooting guide