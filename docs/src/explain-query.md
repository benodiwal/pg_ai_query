# explain_query Function

The `explain_query` function runs EXPLAIN ANALYZE on PostgreSQL queries and provides AI-powered performance analysis and optimization recommendations.

## Overview

This function combines PostgreSQL's built-in EXPLAIN ANALYZE functionality with advanced AI analysis to provide:
- Detailed execution plan analysis
- Performance bottleneck identification
- Index recommendations
- Query optimization suggestions
- Easy-to-understand explanations in plain English

## Function Signature

```sql
explain_query(
    query_text text,
    api_key text DEFAULT NULL,
    provider text DEFAULT 'auto'
) RETURNS text
```

## Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `query_text` | `text` | *required* | The SQL query to analyze |
| `api_key` | `text` | `NULL` | OpenAI or Anthropic API key (uses config if not provided) |
| `provider` | `text` | `'auto'` | AI provider: `'openai'`, `'anthropic'`, or `'auto'` |

## Basic Usage

### Simple Query Analysis

```sql
SELECT explain_query('SELECT * FROM users WHERE created_at > NOW() - INTERVAL ''7 days''');
```

### Complex Query Analysis

```sql
SELECT explain_query('
    SELECT u.username, COUNT(o.id) as order_count
    FROM users u
    LEFT JOIN orders o ON u.id = o.user_id
    WHERE u.created_at > NOW() - INTERVAL ''30 days''
    GROUP BY u.id, u.username
    HAVING COUNT(o.id) > 5
    ORDER BY order_count DESC
    LIMIT 100
');
```

### Using Specific AI Provider

```sql
-- Use OpenAI specifically
SELECT explain_query(
    'SELECT * FROM products WHERE price > 100 ORDER BY price DESC LIMIT 10',
    'your-openai-api-key',
    'openai'
);

-- Use Anthropic specifically
SELECT explain_query(
    'SELECT * FROM products WHERE price > 100 ORDER BY price DESC LIMIT 10',
    'your-anthropic-api-key',
    'anthropic'
);
```

## Output Format

The function returns a structured text analysis with these sections:

### Query Overview
Brief description of what the query accomplishes.

### Performance Summary
- Overall execution time
- Total cost estimate (PostgreSQL's relative measure)
- Number of rows processed

### Execution Plan Analysis
- Key operations in the execution plan
- Join strategies and scan methods
- Focus on expensive operations

### Performance Issues
- Identified bottlenecks
- Inefficient operations
- Resource usage concerns

### Optimization Suggestions
- Specific recommendations for improvement
- Query rewriting suggestions
- Configuration recommendations

### Index Recommendations
- Missing indexes that could improve performance
- Specific `CREATE INDEX` statements
- Partial index suggestions where applicable

## Example Output

```
Query Overview:
This query retrieves users created within the last 7 days along with their order statistics,
focusing on active customers with more than 5 orders.

Performance Summary:
- Overall Execution Time: 45.2 milliseconds
- Total Cost: 1250.75 (PostgreSQL's relative cost estimate)
- Rows Processed: 156 rows returned from 50,000 rows examined

Execution Plan Analysis:
- Hash Join: Efficiently joins users and orders tables
- Index Scan: Uses existing index on users.created_at
- Sequential Scan: Full table scan on orders table (potential bottleneck)
- HashAggregate: Groups results for COUNT calculations
- Sort: Orders results by order count

Performance Issues:
- Sequential scan on orders table indicates missing index on user_id
- Hash join spills to disk due to large orders table size
- HAVING clause applied after aggregation, could be optimized

Optimization Suggestions:
1. Add index on orders.user_id to eliminate sequential scan
2. Consider partitioning orders table by date if very large
3. Move some HAVING conditions to WHERE clause if possible
4. Increase work_mem setting if hash joins frequently spill to disk

Index Recommendations:
-- Primary recommendation
CREATE INDEX idx_orders_user_id ON orders(user_id);

-- Optional: Composite index for better performance
CREATE INDEX idx_orders_user_date ON orders(user_id, created_at);

-- Consider partial index for recent orders
CREATE INDEX idx_orders_recent ON orders(user_id) WHERE created_at > NOW() - INTERVAL '1 year';
```

## Supported Query Types

The function supports analysis of:
- `SELECT` statements
- `WITH` (Common Table Expressions) queries
- `VALUES` clauses

> **Note**: Only read-only queries are supported for security reasons. DDL, DML, and other statement types will return an error.

## Configuration

The function uses the same configuration system as other pg_ai_query functions.

### API Keys

Configure API keys in `~/.pg_ai.config`:

```ini
[openai]
api_key = "your-openai-api-key"
default_model = "gpt-4o"

[anthropic]
api_key = "your-anthropic-api-key"
default_model = "claude-sonnet-4-5-20250929"
```

### Provider Selection

- `'auto'` (default): Uses the first available API key from configuration
- `'openai'`: Forces use of OpenAI models
- `'anthropic'`: Forces use of Anthropic models

## Error Handling

Common error scenarios and their solutions:

### Invalid Query
```sql
SELECT explain_query('SELECT * FROM non_existent_table');
-- Error: relation "non_existent_table" does not exist
```

### Missing API Key
```sql
-- When no API key is configured
SELECT explain_query('SELECT * FROM users');
-- Error: API key required. Pass as parameter or configure ~/.pg_ai.config
```

### Syntax Error
```sql
SELECT explain_query('SELECT * FORM users'); -- typo in FROM
-- Error: syntax error at or near "FORM"
```

### Unsupported Query Type
```sql
SELECT explain_query('DROP TABLE users');
-- Error: Only SELECT, WITH, and VALUES queries are allowed
```

## Performance Considerations

- **Query Execution**: The function actually executes your query via EXPLAIN ANALYZE
- **Execution Time**: Query execution time is included in the analysis
- **AI Processing**: AI analysis adds typically 1-3 seconds of processing time
- **Large Queries**: Very complex queries may take longer to analyze

## Security Notes

- Queries are executed with the same permissions as the calling user
- No privilege escalation occurs
- API keys are handled securely and not logged
- Only read-only query types are permitted

## Best Practices

1. **Use for Optimization**: Run on queries that are performing slowly
2. **Test Variations**: Compare different approaches to the same query
3. **Monitor Trends**: Regular analysis helps track performance changes over time
4. **Validate Recommendations**: Always test suggested indexes before implementing in production
5. **Consider Data Volume**: Remember that performance characteristics change with data size

## Integration with Other Functions

The `explain_query` function works well with other pg_ai_query functions:

```sql
-- Generate a query, then analyze its performance
WITH generated_query AS (
    SELECT generate_query('show recent high-value orders') as sql
)
SELECT explain_query((SELECT sql FROM generated_query));
```

## Common Use Cases

- **Query Optimization**: Identify and fix slow queries
- **Index Planning**: Determine what indexes to create
- **Performance Monitoring**: Regular health checks of critical queries
- **Code Reviews**: Analyze query performance before deployment
- **Learning**: Understand how PostgreSQL executes different query patterns

## See Also

- [Performance Analysis Examples](./performance-analysis.md)
- [Best Practices](./best-practices.md)
- [Integration Patterns](./integration.md)
- [Troubleshooting](./troubleshooting.md)