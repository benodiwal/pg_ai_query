# Quick Start Guide

This guide will get you up and running with `pg_ai_query` in just a few minutes. We'll walk through setting up the extension, configuring it, and generating your first AI-powered SQL queries.

## Step 1: Install and Enable the Extension

First, make sure you have `pg_ai_query` installed (see [Installation Guide](./installation.md) if needed), then enable it in your database:

```sql
-- Connect to your database
psql -d your_database

-- Enable the extension
CREATE EXTENSION IF NOT EXISTS pg_ai_query;

-- Verify installation
\df generate_query
```

You should see the `generate_query` function listed.

## Step 2: Configure API Access

Create a configuration file at `~/.pg_ai.config`:

```ini
# Quick start configuration
[general]
enable_logging = true

[openai]
api_key = "your-openai-api-key-here"
default_model = "gpt-4o"
```

**Get your OpenAI API key**:
1. Visit [platform.openai.com](https://platform.openai.com)
2. Sign up or log in
3. Create an API key in the API keys section
4. Replace `your-openai-api-key-here` with your actual key

## Step 3: Create Sample Data

Let's create some sample tables to work with:

```sql
-- Create a users table
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(150) UNIQUE NOT NULL,
    age INTEGER,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    city VARCHAR(50)
);

-- Create an orders table
CREATE TABLE orders (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id),
    product_name VARCHAR(200),
    amount DECIMAL(10,2),
    status VARCHAR(20) DEFAULT 'pending',
    order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Insert sample data
INSERT INTO users (name, email, age, city) VALUES
    ('Alice Johnson', 'alice@example.com', 28, 'New York'),
    ('Bob Smith', 'bob@example.com', 35, 'San Francisco'),
    ('Carol Davis', 'carol@example.com', 31, 'Chicago'),
    ('David Wilson', 'david@example.com', 27, 'Seattle'),
    ('Eva Brown', 'eva@example.com', 33, 'Boston');

INSERT INTO orders (user_id, product_name, amount, status) VALUES
    (1, 'Laptop Pro', 1299.99, 'completed'),
    (1, 'Wireless Mouse', 79.99, 'completed'),
    (2, 'Monitor 4K', 399.99, 'pending'),
    (3, 'Keyboard Mechanical', 159.99, 'completed'),
    (4, 'Tablet', 599.99, 'shipped'),
    (5, 'Phone Case', 29.99, 'completed');
```

## Step 4: Generate Your First Queries

Now let's use natural language to query our data:

### Basic Query
```sql
-- Simple user lookup
SELECT generate_query('show me all users');
```

**Result:**
```sql
SELECT id, name, email, age, created_at, city FROM public.users LIMIT 1000;
```

### Query with Conditions
```sql
-- Users from specific city
SELECT generate_query('find all users from New York');
```

**Result:**
```sql
SELECT id, name, email, age, created_at, city
FROM public.users
WHERE city = 'New York'
LIMIT 1000;
```

### Query with Joins
```sql
-- Orders with user information
SELECT generate_query('show me all orders with customer names');
```

**Result:**
```sql
SELECT o.id, o.product_name, o.amount, o.status, o.order_date, u.name as customer_name
FROM public.orders o
JOIN public.users u ON o.user_id = u.id
LIMIT 1000;
```

### Aggregation Query
```sql
-- Revenue analysis
SELECT generate_query('calculate total revenue by order status');
```

**Result:**
```sql
SELECT status, SUM(amount) as total_revenue
FROM public.orders
GROUP BY status
LIMIT 1000;
```

## Step 5: Execute Generated Queries

The extension generates SQL queries that you can execute immediately:

```sql
-- Generate and execute in one step
WITH generated AS (
    SELECT generate_query('show users older than 30') as query
)
SELECT query FROM generated;

-- Copy the generated query and run it:
SELECT id, name, email, age, created_at, city
FROM public.users
WHERE age > 30
LIMIT 1000;
```

## Step 6: Explore Advanced Features

### Schema Discovery
Check what tables the extension can see:

```sql
-- View all tables in your database
SELECT get_database_tables();

-- Get detailed information about a specific table
SELECT get_table_details('users');
```

### Different AI Providers
If you have multiple providers configured:

```sql
-- Use specific provider
SELECT generate_query('show recent orders', null, 'openai');
SELECT generate_query('show recent orders', null, 'anthropic');
SELECT generate_query('show recent orders', null, 'gemini');
```

### Complex Queries
Try more complex natural language requests:

```sql
-- Complex aggregation
SELECT generate_query('show top 3 customers by total order amount with their contact info');

-- Date-based filtering
SELECT generate_query('find orders placed in the last 7 days');

-- Multiple conditions
SELECT generate_query('show users from California or New York who are older than 25');
```

## Common Patterns

### 1. Exploratory Data Analysis
```sql
-- Understand your data structure
SELECT generate_query('describe the structure of my database');
SELECT generate_query('show me sample data from each table');
SELECT generate_query('count records in all tables');
```

### 2. Business Intelligence
```sql
-- Sales analysis
SELECT generate_query('monthly revenue trends');
SELECT generate_query('top selling products');
SELECT generate_query('customer acquisition by month');
```

### 3. Data Quality Checks
```sql
-- Find data issues
SELECT generate_query('find users with missing email addresses');
SELECT generate_query('show duplicate orders');
SELECT generate_query('find orders without valid user references');
```

## Troubleshooting Quick Start

### Error: "Extension not found"
```sql
-- Solution: Install the extension first
CREATE EXTENSION pg_ai_query;
```

### Error: "API key not configured"
- Check your `~/.pg_ai.config` file exists
- Verify the API key is valid
- Restart your PostgreSQL session

### Error: "No tables found"
- Make sure you have user tables (not just system tables)
- Check table permissions
- Try creating the sample tables from Step 3

### Unexpected Results
- Enable logging to see what's happening:
  ```ini
  [general]
  enable_logging = true
  log_level = "DEBUG"
  ```
- Check the generated query makes sense
- Try simpler natural language descriptions

## Next Steps

Now that you're up and running:

1. **Learn more about [Usage Patterns](./usage.md)** for advanced query generation
2. **Explore [Examples](./examples.md)** for inspiration
3. **Read about [AI Providers](./providers.md)** to optimize your model choice
4. **Check [Best Practices](./best-practices.md)** for production usage

## Getting Help

If you run into issues:
- Check the [Troubleshooting Guide](./troubleshooting.md)
- Review the [FAQ](./faq.md)
- Look at more [Examples](./examples.md) for inspiration