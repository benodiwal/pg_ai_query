# Basic Usage

This guide covers the fundamental ways to use `pg_ai_query` for generating SQL queries from natural language. Whether you're exploring data, creating reports, or learning SQL, this guide will help you get the most out of the extension.

## Core Function: generate_query()

The main function you'll use is `generate_query()`, which converts natural language into SQL:

```sql
generate_query(natural_language_query, [api_key], [provider])
```

### Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `natural_language_query` | text | Yes | - | Your question in plain English |
| `api_key` | text | No | null | API key (uses config if not provided) |
| `provider` | text | No | 'auto' | AI provider: 'openai', 'anthropic', or 'auto' |

### Basic Examples

```sql
-- Simplest form - uses configured API key and provider
SELECT generate_query('show me all users');

-- With custom API key
SELECT generate_query('count total orders', 'your-api-key-here');

-- With specific provider
SELECT generate_query('find recent customers', null, 'openai');
```

## Query Types

### 1. Simple Data Retrieval

**Goal**: Get basic data from tables

```sql
-- Show all records
SELECT generate_query('show me all products');
SELECT generate_query('list all customers');
SELECT generate_query('display user information');

-- Show specific columns
SELECT generate_query('show user names and emails');
SELECT generate_query('get product names and prices');
```

### 2. Filtered Queries

**Goal**: Find specific records based on conditions

```sql
-- Simple filters
SELECT generate_query('find users older than 25');
SELECT generate_query('show completed orders');
SELECT generate_query('get products under $100');

-- Multiple conditions
SELECT generate_query('find users from New York who are older than 30');
SELECT generate_query('show pending orders from the last month');

-- Date-based filters
SELECT generate_query('show orders from yesterday');
SELECT generate_query('find users created in 2024');
SELECT generate_query('get sales from last quarter');
```

### 3. Aggregation and Analysis

**Goal**: Calculate totals, averages, counts, and other statistics

```sql
-- Simple counts
SELECT generate_query('count total users');
SELECT generate_query('how many orders do we have');

-- Sums and totals
SELECT generate_query('calculate total revenue');
SELECT generate_query('sum all order amounts');

-- Averages
SELECT generate_query('average order value');
SELECT generate_query('mean user age');

-- Group by analysis
SELECT generate_query('count orders by status');
SELECT generate_query('total sales by month');
SELECT generate_query('average order value by customer');
```

### 4. Joined Data

**Goal**: Combine information from multiple tables

```sql
-- Simple joins
SELECT generate_query('show orders with customer names');
SELECT generate_query('list products with category information');

-- Complex relationships
SELECT generate_query('show customers with their total order amounts');
SELECT generate_query('find users who have never placed an order');
SELECT generate_query('list top customers by number of orders');
```

### 5. Ranking and Sorting

**Goal**: Order results and find top/bottom records

```sql
-- Top N queries
SELECT generate_query('top 10 customers by revenue');
SELECT generate_query('5 most expensive products');
SELECT generate_query('latest 20 orders');

-- Sorting
SELECT generate_query('show users ordered by age');
SELECT generate_query('list products by price descending');
```

## Best Practices for Natural Language

### Be Specific About What You Want

**Good:**
```sql
SELECT generate_query('show user names and email addresses for active users');
```

**Less Good:**
```sql
SELECT generate_query('show users');
```

### Use Clear Column References

**Good:**
```sql
SELECT generate_query('find orders with amount greater than 1000');
```

**Better:**
```sql
SELECT generate_query('find orders where order_amount > 1000');
```

### Specify Time Periods Clearly

**Good:**
```sql
SELECT generate_query('show orders from the last 30 days');
SELECT generate_query('find users created between January 1, 2024 and March 31, 2024');
```

### Use Business Terms

The AI understands common business terminology:

```sql
-- These work well
SELECT generate_query('show quarterly revenue');
SELECT generate_query('find churned customers');
SELECT generate_query('calculate customer lifetime value');
SELECT generate_query('show conversion rates by month');
```

## Advanced Usage Patterns

### 1. Incremental Query Building

Start simple and add complexity:

```sql
-- Step 1: Basic query
SELECT generate_query('show orders');

-- Step 2: Add filters
SELECT generate_query('show completed orders');

-- Step 3: Add aggregation
SELECT generate_query('count completed orders by customer');

-- Step 4: Add sorting
SELECT generate_query('count completed orders by customer, ordered by count desc');
```

### 2. Data Exploration Workflow

```sql
-- 1. Understand data structure
SELECT get_database_tables();

-- 2. Explore individual tables
SELECT generate_query('show sample data from users table');
SELECT generate_query('describe the structure of orders table');

-- 3. Understand relationships
SELECT generate_query('show how users and orders are connected');

-- 4. Analyze patterns
SELECT generate_query('show distribution of users by city');
SELECT generate_query('analyze order patterns by day of week');
```

### 3. Report Generation

```sql
-- Daily report
SELECT generate_query('daily sales summary for today');

-- Customer analysis
SELECT generate_query('customer segmentation by order frequency');

-- Product performance
SELECT generate_query('product performance metrics for last month');
```

## Working with Results

### 1. Copying and Executing Queries

```sql
-- Method 1: Copy-paste the result
SELECT generate_query('show top customers');
-- Copy the returned SQL and execute it separately

-- Method 2: Store in a variable (if your client supports it)
\set query `SELECT generate_query('show top customers')`
:query
```

### 2. Query Refinement

If the generated query isn't quite right:

```sql
-- Try rephrasing
SELECT generate_query('show customers with highest total orders');
-- vs
SELECT generate_query('rank customers by total order value');

-- Be more specific
SELECT generate_query('show customers with more than 5 orders and total value over $1000');
```

### 3. Validation

Always review generated queries before using in production:

```sql
-- Check the generated query
SELECT generate_query('delete old records');
-- Review before executing - the extension won't generate dangerous queries,
-- but always verify the logic matches your intent
```

## Error Handling

### Common Issues and Solutions

**"No tables found"**
```sql
-- Check your database has user tables
SELECT get_database_tables();
```

**"Query too complex"**
```sql
-- Break down into simpler parts
-- Instead of: "show customers with orders and products and categories"
-- Try: "show customers with their order details"
```

**"Ambiguous column reference"**
```sql
-- Be more specific about table/column names
-- Instead of: "show orders with customer info"
-- Try: "show orders with customer names from users table"
```

## Performance Considerations

### Query Limits

All queries automatically include LIMIT clauses for safety:

```sql
-- This automatically gets LIMIT 1000 (configurable)
SELECT generate_query('show all orders');
```

### Large Datasets

For large tables, be specific about what you need:

```sql
-- Instead of: "show all user data"
-- Use: "show user summary with names and creation dates"
```

### Indexes

Generated queries work best with proper indexes:

```sql
-- Create indexes on commonly filtered columns
CREATE INDEX idx_users_city ON users(city);
CREATE INDEX idx_orders_date ON orders(order_date);
CREATE INDEX idx_orders_status ON orders(status);
```

## Integration Patterns

### 1. Application Development

```python
# Python example
cursor.execute("SELECT generate_query(%s)", ['show recent user activity'])
query = cursor.fetchone()[0]
cursor.execute(query)
results = cursor.fetchall()
```

### 2. Business Intelligence Tools

Many BI tools can use the extension to generate queries dynamically based on user input.

### 3. Data Documentation

Use the extension to generate example queries for documentation:

```sql
-- Generate examples for each table
SELECT generate_query('show sample data from ' || table_name)
FROM information_schema.tables
WHERE table_schema = 'public';
```

## Next Steps

- Explore the [Function Reference](./function-reference.md) for complete API details
- Check out [Examples](./examples.md) for more complex use cases
- Learn about [Schema Discovery](./schema-discovery.md) to understand how the extension analyzes your database