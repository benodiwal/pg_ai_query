# Function Reference

This page provides complete reference documentation for all functions provided by the `pg_ai_query` extension.

## Main Functions

### generate_query()

Generates SQL queries from natural language descriptions with automatic schema discovery.

#### Signature
```sql
generate_query(
    natural_language_query text,
    api_key text DEFAULT NULL,
    provider text DEFAULT 'auto'
) RETURNS text
```

#### Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `natural_language_query` | text | ✓ | - | The natural language description of the query you want |
| `api_key` | text | ✗ | NULL | API key for AI provider (uses config if NULL) |
| `provider` | text | ✗ | 'auto' | AI provider to use: 'openai', 'anthropic', or 'auto' |

#### Returns
- **Type**: `text`
- **Content**: A valid PostgreSQL SQL query

#### Examples

```sql
-- Basic usage
SELECT generate_query('show all users');

-- With custom API key
SELECT generate_query(
    'find users created in the last week',
    'sk-your-api-key-here'
);

-- With specific provider
SELECT generate_query(
    'calculate monthly revenue by product category',
    NULL,
    'openai'
);

-- Complex query
SELECT generate_query('show top 10 customers by total order value with their contact information');
```

#### Behavior

- **Schema Discovery**: Automatically analyzes your database schema to understand table structures and relationships
- **Safety Limits**: Always adds LIMIT clauses to SELECT queries (configurable)
- **Query Validation**: Validates generated queries for safety and correctness
- **Error Handling**: Returns descriptive error messages for invalid requests

#### Supported Query Types

| Type | Description | Examples |
|------|-------------|----------|
| SELECT | Data retrieval with filtering, joins, aggregation | `'show users'`, `'count orders by status'` |
| INSERT | Data insertion | `'insert a new user with name John'` |
| UPDATE | Data modification | `'update user email where id is 5'` |
| DELETE | Data removal | `'delete cancelled orders'` |

---

### get_database_tables()

Returns metadata about all user tables in the database.

#### Signature
```sql
get_database_tables() RETURNS text
```

#### Parameters
None.

#### Returns
- **Type**: `text` (JSON format)
- **Content**: Array of table metadata objects

#### JSON Structure
```json
[
  {
    "table_name": "users",
    "schema_name": "public",
    "table_type": "BASE TABLE",
    "estimated_rows": 1500,
    "table_size": "128 kB"
  },
  {
    "table_name": "orders",
    "schema_name": "public",
    "table_type": "BASE TABLE",
    "estimated_rows": 5000,
    "table_size": "512 kB"
  }
]
```

#### Example Usage

```sql
-- Get all tables
SELECT get_database_tables();

-- Pretty print with formatting
SELECT jsonb_pretty(get_database_tables()::jsonb);

-- Extract specific information
SELECT
    table_name,
    estimated_rows
FROM jsonb_to_recordset(get_database_tables()::jsonb)
AS x(table_name text, estimated_rows int);
```

---

### get_table_details()

Returns detailed information about a specific table including columns, constraints, and relationships.

#### Signature
```sql
get_table_details(
    table_name text,
    schema_name text DEFAULT 'public'
) RETURNS text
```

#### Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `table_name` | text | ✓ | - | Name of the table to analyze |
| `schema_name` | text | ✗ | 'public' | Schema containing the table |

#### Returns
- **Type**: `text` (JSON format)
- **Content**: Detailed table information object

#### JSON Structure
```json
{
  "table_name": "users",
  "schema_name": "public",
  "columns": [
    {
      "column_name": "id",
      "data_type": "integer",
      "is_nullable": false,
      "column_default": "nextval('users_id_seq'::regclass)",
      "is_primary_key": true
    },
    {
      "column_name": "email",
      "data_type": "character varying",
      "character_maximum_length": 150,
      "is_nullable": false,
      "column_default": null,
      "is_unique": true
    }
  ],
  "constraints": [
    {
      "constraint_name": "users_pkey",
      "constraint_type": "PRIMARY KEY",
      "column_names": ["id"]
    },
    {
      "constraint_name": "users_email_key",
      "constraint_type": "UNIQUE",
      "column_names": ["email"]
    }
  ],
  "foreign_keys": [],
  "indexes": [
    {
      "index_name": "users_pkey",
      "columns": ["id"],
      "is_unique": true,
      "is_primary": true
    }
  ]
}
```

#### Example Usage

```sql
-- Basic usage
SELECT get_table_details('users');

-- Specific schema
SELECT get_table_details('orders', 'sales');

-- Extract column information
SELECT
    column_name,
    data_type,
    is_nullable
FROM jsonb_to_recordset(
    (get_table_details('users')::jsonb)->'columns'
) AS x(column_name text, data_type text, is_nullable boolean);

-- Find all foreign keys
SELECT
    constraint_name,
    column_names
FROM jsonb_to_recordset(
    (get_table_details('orders')::jsonb)->'foreign_keys'
) AS x(constraint_name text, column_names text[]);
```

---

## Utility Functions

### Schema Discovery Process

The extension uses these internal processes when analyzing your database:

1. **Table Discovery**: Identifies all user tables (excludes system catalogs)
2. **Column Analysis**: Examines data types, constraints, and nullable fields
3. **Relationship Mapping**: Discovers foreign key relationships between tables
4. **Index Analysis**: Identifies indexes that might optimize query performance
5. **Statistics Gathering**: Collects row counts and table sizes for optimization

### Internal Validation

All functions perform these validations:

- **Permission Checks**: Ensures access to only authorized tables
- **Schema Validation**: Verifies table and schema existence
- **SQL Injection Prevention**: Sanitizes all inputs
- **Query Safety**: Prevents generation of harmful operations

## Error Codes and Messages

### Common Error Conditions

| Error | Cause | Solution |
|-------|-------|----------|
| `"API key not configured"` | No valid API key found | Configure API key in `~/.pg_ai.config` or pass as parameter |
| `"No tables found"` | Database has no user tables | Create some tables or check permissions |
| `"Table does not exist"` | Specified table not found | Check table name and schema |
| `"Query generation failed"` | AI service error | Check API key, network connectivity, and service status |
| `"Invalid provider"` | Unknown provider specified | Use 'openai', 'anthropic', or 'auto' |

### Debugging Functions

```sql
-- Check extension version and status
SELECT extversion FROM pg_extension WHERE extname = 'pg_ai_query';

-- Verify function availability
\df generate_query
\df get_database_tables
\df get_table_details

-- Test basic functionality
SELECT generate_query('SELECT 1');
```

## Performance Characteristics

### Query Generation Performance

| Factor | Impact | Recommendations |
|--------|--------|-----------------|
| Database Size | Minimal | Schema analysis is cached |
| Table Count | Low | Only user tables are analyzed |
| Query Complexity | Medium | More complex requests take longer |
| AI Model | High | GPT-4o is slower but more accurate than GPT-3.5 |

### Memory Usage

- **Schema Analysis**: ~1MB per 100 tables
- **Query Generation**: ~5-10MB per request
- **Caching**: Schema information cached in session

### Network Requirements

- **API Requests**: HTTPS to OpenAI/Anthropic APIs
- **Bandwidth**: ~1-10KB per query generation request
- **Latency**: Typically 1-5 seconds per request

## Function Security

### Access Control

- Functions execute with caller's privileges
- No privilege escalation
- Respects PostgreSQL's standard permission system

### Data Protection

- Never sends actual data to AI providers
- Only schema metadata is transmitted
- Generated queries can be reviewed before execution

### API Security

- API keys transmitted over HTTPS only
- Keys stored securely in configuration files
- No logging of API keys

## Next Steps

- See [Examples](./examples.md) for practical usage patterns
- Review [Best Practices](./best-practices.md) for production usage
- Check [API Reference](./api-reference.md) for complete technical details