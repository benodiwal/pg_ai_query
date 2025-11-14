# Response Formatting

The `pg_ai_query` extension offers flexible response formatting options that allow you to customize how query results are presented. You can choose to receive just the SQL query, or enhance it with explanations, warnings, and visualization suggestions.

## Response Format Options

### Basic SQL Response (Default)

By default, the extension returns only the generated SQL query:

```sql
SELECT generate_query('show all customers');
-- Returns: SELECT * FROM customers LIMIT 1000;
```

### Enhanced Text Response

When explanation and warnings are enabled, the response includes additional context as SQL comments:

```sql
-- Configuration: show_explanation=true, show_warnings=true, use_formatted_response=false
SELECT generate_query('find customers with high order values');
-- Returns:
-- SELECT c.customer_id, c.name, SUM(o.total_amount) as total_spent
-- FROM customers c
-- JOIN orders o ON c.customer_id = o.customer_id
-- GROUP BY c.customer_id, c.name
-- HAVING SUM(o.total_amount) > 1000
-- ORDER BY total_spent DESC
-- LIMIT 1000;
--
-- Explanation:
-- This query finds customers who have spent more than $1000 total across all their orders,
-- showing their total spending ranked from highest to lowest.
--
-- Warnings:
-- 1. Large dataset: This query may be slow on tables with millions of rows
-- 2. Consider indexing: customer_id and total_amount columns should be indexed
```

### JSON Response

When `use_formatted_response=true`, the extension returns structured JSON:

```json
{
  "query": "SELECT c.customer_id, c.name, SUM(o.total_amount) as total_spent FROM customers c JOIN orders o ON c.customer_id = o.customer_id GROUP BY c.customer_id, c.name HAVING SUM(o.total_amount) > 1000 ORDER BY total_spent DESC LIMIT 1000;",
  "success": true,
  "explanation": "This query finds customers who have spent more than $1000 total across all their orders, showing their total spending ranked from highest to lowest.",
  "warnings": [
    "Large dataset: This query may be slow on tables with millions of rows",
    "Consider indexing: customer_id and total_amount columns should be indexed"
  ],
  "suggested_visualization": "bar",
  "row_limit_applied": true
}
```

## Configuration Options

### Response Settings

Add these options to the `[response]` section of your `~/.pg_ai.config` file:

```ini
[response]
# Show detailed explanation of what the query does
show_explanation = true

# Show warnings about performance, security, or data implications
show_warnings = true

# Show suggested visualization type for query results
show_suggested_visualization = true

# Use formatted response (JSON format) instead of plain SQL
use_formatted_response = false
```

### Individual Option Details

#### `show_explanation`
- **Type**: Boolean
- **Default**: true
- **Description**: Includes a plain English explanation of what the query does, including business context and logic

#### `show_warnings`
- **Type**: Boolean
- **Default**: true
- **Description**: Includes warnings about:
  - Performance implications for large datasets
  - Security considerations
  - Potential data quality issues
  - Index recommendations
  - Best practice suggestions

#### `show_suggested_visualization`
- **Type**: Boolean
- **Default**: false
- **Description**: Suggests the most appropriate visualization type based on the query structure and result type:
  - `bar` - For categorical comparisons
  - `line` - For time series data
  - `pie` - For part-to-whole relationships
  - `table` - For detailed data exploration
  - `none` - When visualization isn't applicable

#### `use_formatted_response`
- **Type**: Boolean
- **Default**: false
- **Description**: When enabled, returns structured JSON with all components. When disabled, returns SQL with optional comment annotations.

## Response Examples

### Sales Analysis Query

**Input:**
```sql
SELECT generate_query('monthly sales trend for this year');
```

**Text Response (use_formatted_response=false):**
```sql
SELECT
    DATE_TRUNC('month', order_date) as month,
    SUM(total_amount) as monthly_sales,
    COUNT(*) as order_count
FROM orders
WHERE EXTRACT(year FROM order_date) = EXTRACT(year FROM CURRENT_DATE)
GROUP BY DATE_TRUNC('month', order_date)
ORDER BY month
LIMIT 1000;

-- Explanation:
-- Calculates total sales and order count by month for the current year,
-- useful for identifying seasonal trends and business performance patterns.

-- Suggested Visualization: line
-- Reasoning: Time series data showing trends over months is best displayed as a line chart
```

**JSON Response (use_formatted_response=true):**
```json
{
  "query": "SELECT DATE_TRUNC('month', order_date) as month, SUM(total_amount) as monthly_sales, COUNT(*) as order_count FROM orders WHERE EXTRACT(year FROM order_date) = EXTRACT(year FROM CURRENT_DATE) GROUP BY DATE_TRUNC('month', order_date) ORDER BY month LIMIT 1000;",
  "success": true,
  "explanation": "Calculates total sales and order count by month for the current year, useful for identifying seasonal trends and business performance patterns.",
  "warnings": [],
  "suggested_visualization": "line",
  "row_limit_applied": true
}
```

### Complex Join Query

**Input:**
```sql
SELECT generate_query('customers who bought more than 5 products in the electronics category');
```

**Enhanced Response:**
```sql
SELECT
    c.customer_id,
    c.name,
    COUNT(DISTINCT oi.product_id) as electronics_products_bought
FROM customers c
JOIN orders o ON c.customer_id = o.customer_id
JOIN order_items oi ON o.order_id = oi.order_id
JOIN products p ON oi.product_id = p.product_id
WHERE p.category = 'electronics'
GROUP BY c.customer_id, c.name
HAVING COUNT(DISTINCT oi.product_id) > 5
ORDER BY electronics_products_bought DESC
LIMIT 1000;

-- Explanation:
-- Finds customers who have purchased more than 5 different electronics products,
-- showing customer details and the count of unique electronics products they bought.

-- Warnings:
-- 1. Multiple JOINs: This query involves 4 table joins which may be slow on large datasets
-- 2. Index recommendation: Ensure indexes exist on customer_id, order_id, product_id, and category columns
-- 3. Category filter: Verify 'electronics' is the exact category name used in your database

-- Suggested Visualization: table
-- Reasoning: Customer list with counts is best displayed in a table format for detailed analysis
```

## Using Responses in Applications

### Parsing JSON Responses

When using `use_formatted_response=true`, you can parse the JSON in your application:

```python
import psycopg2
import json

# Execute query
cursor.execute("SELECT generate_query('top selling products')")
response_json = cursor.fetchone()[0]

# Parse the JSON response
response = json.loads(response_json)

# Extract components
sql_query = response['query']
explanation = response.get('explanation', '')
warnings = response.get('warnings', [])
visualization = response.get('suggested_visualization', 'table')

# Use the SQL query for actual data retrieval
cursor.execute(sql_query)
results = cursor.fetchall()
```

### Working with Text Responses

For text responses with comments, you can extract just the SQL:

```python
def extract_sql_from_response(response_text):
    """Extract just the SQL query from a text response."""
    lines = response_text.split('\n')
    sql_lines = []

    for line in lines:
        # Skip comment lines
        if line.strip().startswith('--'):
            break
        if line.strip():
            sql_lines.append(line)

    return '\n'.join(sql_lines)

# Usage
response = cursor.fetchone()[0]
clean_sql = extract_sql_from_response(response)
```

## Best Practices

### For Development
- Enable all response features during development for better understanding
- Use JSON format for programmatic parsing
- Pay attention to warnings for performance optimization

### For Production
- Consider disabling detailed explanations to reduce response size
- Keep warnings enabled for monitoring potential issues
- Use text format for human-readable responses

### For Business Intelligence
- Enable visualization suggestions to guide chart creation
- Use explanations to document generated queries
- Monitor warnings for data quality issues

## Performance Considerations

### Response Size
- JSON responses are larger than text responses
- Explanations and warnings add to response size
- Consider response formatting impact on network transfer

### AI Token Usage
- Enhanced responses require more AI processing
- Detailed explanations use additional API tokens
- Balance information value vs. cost

### Caching
- Consider caching formatted responses for repeated queries
- JSON responses are easier to cache and manipulate
- Text responses are more human-readable but harder to parse

## Troubleshooting Response Formatting

### Configuration Not Applied
If response formatting isn't working:

1. Check configuration file location: `~/.pg_ai.config`
2. Verify configuration syntax (no quotes around true/false values)
3. Restart PostgreSQL connection to reload configuration
4. Check PostgreSQL logs for configuration errors

### Incomplete Responses
If responses are missing expected components:

1. Verify AI provider supports detailed responses
2. Check API rate limits and quotas
3. Review request timeout settings
4. Enable logging to see full AI responses

### JSON Parsing Errors
When using `use_formatted_response=true`:

1. Ensure AI provider returns valid JSON
2. Check for truncated responses due to token limits
3. Verify your JSON parsing library handles the response format
4. Use text format as fallback if JSON parsing fails