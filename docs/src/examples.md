# Examples

This page provides practical examples of using `pg_ai_query` for common database tasks and use cases.

## Sample Database

For these examples, we'll use a sample e-commerce database with the following tables:

```sql
-- Users table
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(150) UNIQUE NOT NULL,
    age INTEGER,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    city VARCHAR(50),
    status VARCHAR(20) DEFAULT 'active'
);

-- Products table
CREATE TABLE products (
    id SERIAL PRIMARY KEY,
    name VARCHAR(200) NOT NULL,
    category VARCHAR(50),
    price DECIMAL(10,2),
    stock_quantity INTEGER DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Orders table
CREATE TABLE orders (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id),
    total_amount DECIMAL(10,2),
    status VARCHAR(20) DEFAULT 'pending',
    order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    shipping_city VARCHAR(50)
);

-- Order items table
CREATE TABLE order_items (
    id SERIAL PRIMARY KEY,
    order_id INTEGER REFERENCES orders(id),
    product_id INTEGER REFERENCES products(id),
    quantity INTEGER,
    unit_price DECIMAL(10,2)
);
```

## Basic Queries

### Simple Data Retrieval

```sql
-- Show all users
SELECT generate_query('show me all users');
-- Result: SELECT id, name, email, age, created_at, city, status FROM public.users LIMIT 1000;

-- List products with their prices
SELECT generate_query('show product names and prices');
-- Result: SELECT name, price FROM public.products LIMIT 1000;

-- Get recent orders
SELECT generate_query('show orders from the last 7 days');
-- Result: SELECT * FROM public.orders WHERE order_date >= CURRENT_DATE - INTERVAL '7 days' LIMIT 1000;
```

### Filtered Searches

```sql
-- Find active users
SELECT generate_query('find all active users');
-- Result: SELECT * FROM public.users WHERE status = 'active' LIMIT 1000;

-- Search by city
SELECT generate_query('show users from New York');
-- Result: SELECT * FROM public.users WHERE city = 'New York' LIMIT 1000;

-- Price range filtering
SELECT generate_query('find products under $50');
-- Result: SELECT * FROM public.products WHERE price < 50 LIMIT 1000;

-- Age-based filtering
SELECT generate_query('show users between 25 and 35 years old');
-- Result: SELECT * FROM public.users WHERE age BETWEEN 25 AND 35 LIMIT 1000;
```

## Aggregation and Analytics

### Counting and Totals

```sql
-- Count users by city
SELECT generate_query('count users by city');
-- Result: SELECT city, COUNT(*) as count FROM public.users GROUP BY city LIMIT 1000;

-- Total revenue
SELECT generate_query('calculate total revenue from all orders');
-- Result: SELECT SUM(total_amount) as total_revenue FROM public.orders;

-- Average order value
SELECT generate_query('what is the average order value');
-- Result: SELECT AVG(total_amount) as average_order_value FROM public.orders;

-- Product inventory summary
SELECT generate_query('show total stock quantity by product category');
-- Result: SELECT category, SUM(stock_quantity) as total_stock FROM public.products GROUP BY category LIMIT 1000;
```

### Time-Based Analysis

```sql
-- Monthly sales trends
SELECT generate_query('show monthly sales for this year');
-- Result: SELECT DATE_TRUNC('month', order_date) as month, SUM(total_amount) as monthly_sales
--         FROM public.orders WHERE EXTRACT(year FROM order_date) = EXTRACT(year FROM CURRENT_DATE)
--         GROUP BY DATE_TRUNC('month', order_date) ORDER BY month LIMIT 1000;

-- Daily order count
SELECT generate_query('count orders by day for the last 30 days');
-- Result: SELECT DATE(order_date) as order_day, COUNT(*) as order_count
--         FROM public.orders WHERE order_date >= CURRENT_DATE - INTERVAL '30 days'
--         GROUP BY DATE(order_date) ORDER BY order_day LIMIT 1000;

-- User registration trends
SELECT generate_query('show user registrations by month');
-- Result: SELECT DATE_TRUNC('month', created_at) as month, COUNT(*) as new_users
--         FROM public.users GROUP BY DATE_TRUNC('month', created_at) ORDER BY month LIMIT 1000;
```

## Complex Joins and Relationships

### Customer Order Analysis

```sql
-- Orders with customer information
SELECT generate_query('show orders with customer names and emails');
-- Result: SELECT o.id, o.total_amount, o.status, o.order_date, u.name, u.email
--         FROM public.orders o JOIN public.users u ON o.user_id = u.id LIMIT 1000;

-- Customer lifetime value
SELECT generate_query('calculate total spent by each customer');
-- Result: SELECT u.name, u.email, SUM(o.total_amount) as total_spent
--         FROM public.users u LEFT JOIN public.orders o ON u.id = o.user_id
--         GROUP BY u.id, u.name, u.email ORDER BY total_spent DESC LIMIT 1000;

-- Top customers by order count
SELECT generate_query('find customers with the most orders');
-- Result: SELECT u.name, u.email, COUNT(o.id) as order_count
--         FROM public.users u LEFT JOIN public.orders o ON u.id = o.user_id
--         GROUP BY u.id, u.name, u.email ORDER BY order_count DESC LIMIT 1000;
```

### Product and Sales Analysis

```sql
-- Best-selling products
SELECT generate_query('show top selling products by quantity');
-- Result: SELECT p.name, SUM(oi.quantity) as total_sold
--         FROM public.products p JOIN public.order_items oi ON p.id = oi.product_id
--         GROUP BY p.id, p.name ORDER BY total_sold DESC LIMIT 1000;

-- Revenue by product category
SELECT generate_query('calculate revenue by product category');
-- Result: SELECT p.category, SUM(oi.quantity * oi.unit_price) as category_revenue
--         FROM public.products p JOIN public.order_items oi ON p.id = oi.product_id
--         GROUP BY p.category ORDER BY category_revenue DESC LIMIT 1000;

-- Detailed order breakdown
SELECT generate_query('show order details with product names and quantities');
-- Result: SELECT o.id, o.order_date, u.name as customer_name, p.name as product_name,
--                oi.quantity, oi.unit_price, (oi.quantity * oi.unit_price) as line_total
--         FROM public.orders o
--         JOIN public.users u ON o.user_id = u.id
--         JOIN public.order_items oi ON o.id = oi.order_id
--         JOIN public.products p ON oi.product_id = p.id LIMIT 1000;
```

## Business Intelligence Queries

### Customer Segmentation

```sql
-- High-value customers
SELECT generate_query('find customers who have spent more than $1000 total');
-- Result: SELECT u.name, u.email, SUM(o.total_amount) as total_spent
--         FROM public.users u JOIN public.orders o ON u.id = o.user_id
--         GROUP BY u.id, u.name, u.email
--         HAVING SUM(o.total_amount) > 1000 ORDER BY total_spent DESC LIMIT 1000;

-- Customer activity segments
SELECT generate_query('categorize customers by number of orders');
-- Result: SELECT
--           CASE
--             WHEN order_count = 0 THEN 'No Orders'
--             WHEN order_count BETWEEN 1 AND 3 THEN 'Low Activity'
--             WHEN order_count BETWEEN 4 AND 10 THEN 'Medium Activity'
--             ELSE 'High Activity'
--           END as customer_segment,
--           COUNT(*) as customer_count
--         FROM (
--           SELECT u.id, COUNT(o.id) as order_count
--           FROM public.users u LEFT JOIN public.orders o ON u.id = o.user_id
--           GROUP BY u.id
--         ) customer_orders
--         GROUP BY customer_segment LIMIT 1000;

-- Inactive customers
SELECT generate_query('find customers who haven\'t ordered in the last 90 days');
-- Result: SELECT u.name, u.email, MAX(o.order_date) as last_order_date
--         FROM public.users u LEFT JOIN public.orders o ON u.id = o.user_id
--         GROUP BY u.id, u.name, u.email
--         HAVING MAX(o.order_date) < CURRENT_DATE - INTERVAL '90 days'
--         OR MAX(o.order_date) IS NULL LIMIT 1000;
```

### Inventory and Product Analysis

```sql
-- Low stock alerts
SELECT generate_query('show products with less than 10 items in stock');
-- Result: SELECT name, stock_quantity FROM public.products WHERE stock_quantity < 10 LIMIT 1000;

-- Product performance metrics
SELECT generate_query('show product sales performance with revenue and quantity sold');
-- Result: SELECT p.name, p.category,
--                COUNT(oi.id) as times_ordered,
--                SUM(oi.quantity) as total_quantity_sold,
--                SUM(oi.quantity * oi.unit_price) as total_revenue
--         FROM public.products p
--         LEFT JOIN public.order_items oi ON p.id = oi.product_id
--         GROUP BY p.id, p.name, p.category
--         ORDER BY total_revenue DESC LIMIT 1000;

-- Seasonal trends
SELECT generate_query('show monthly product sales trends');
-- Result: SELECT p.category,
--                DATE_TRUNC('month', o.order_date) as month,
--                SUM(oi.quantity) as quantity_sold
--         FROM public.products p
--         JOIN public.order_items oi ON p.id = oi.product_id
--         JOIN public.orders o ON oi.order_id = o.id
--         GROUP BY p.category, DATE_TRUNC('month', o.order_date)
--         ORDER BY month, category LIMIT 1000;
```

## Advanced Analytics

### Cohort Analysis

```sql
-- Customer acquisition by month
SELECT generate_query('show new customer registrations by month with their first order date');
-- Result: SELECT DATE_TRUNC('month', u.created_at) as registration_month,
--                COUNT(*) as new_customers,
--                COUNT(first_order.user_id) as customers_with_orders
--         FROM public.users u
--         LEFT JOIN (
--           SELECT user_id, MIN(order_date) as first_order_date
--           FROM public.orders
--           GROUP BY user_id
--         ) first_order ON u.id = first_order.user_id
--         GROUP BY DATE_TRUNC('month', u.created_at)
--         ORDER BY registration_month LIMIT 1000;
```

### Geographic Analysis

```sql
-- Sales by shipping location
SELECT generate_query('show total sales by shipping city');
-- Result: SELECT shipping_city,
--                COUNT(*) as order_count,
--                SUM(total_amount) as total_sales
--         FROM public.orders
--         WHERE shipping_city IS NOT NULL
--         GROUP BY shipping_city
--         ORDER BY total_sales DESC LIMIT 1000;

-- Customer distribution
SELECT generate_query('show customer count and average order value by city');
-- Result: SELECT u.city,
--                COUNT(DISTINCT u.id) as customer_count,
--                COUNT(o.id) as total_orders,
--                AVG(o.total_amount) as avg_order_value
--         FROM public.users u
--         LEFT JOIN public.orders o ON u.id = o.user_id
--         GROUP BY u.city
--         ORDER BY customer_count DESC LIMIT 1000;
```

## Data Quality and Validation

### Data Integrity Checks

```sql
-- Find orphaned records
SELECT generate_query('find orders without valid customer references');
-- Result: SELECT o.* FROM public.orders o
--         LEFT JOIN public.users u ON o.user_id = u.id
--         WHERE u.id IS NULL LIMIT 1000;

-- Missing email addresses
SELECT generate_query('find users with missing or invalid email addresses');
-- Result: SELECT * FROM public.users
--         WHERE email IS NULL OR email = '' OR email NOT LIKE '%@%' LIMIT 1000;

-- Duplicate detection
SELECT generate_query('find duplicate user email addresses');
-- Result: SELECT email, COUNT(*) as count
--         FROM public.users
--         GROUP BY email
--         HAVING COUNT(*) > 1 LIMIT 1000;
```

### Data Range Validation

```sql
-- Unusual values
SELECT generate_query('find orders with negative amounts');
-- Result: SELECT * FROM public.orders WHERE total_amount < 0 LIMIT 1000;

-- Age validation
SELECT generate_query('find users with unrealistic ages');
-- Result: SELECT * FROM public.users WHERE age < 0 OR age > 120 LIMIT 1000;

-- Future dates
SELECT generate_query('find orders with future dates');
-- Result: SELECT * FROM public.orders WHERE order_date > CURRENT_TIMESTAMP LIMIT 1000;
```

## Performance Monitoring

### Database Statistics

```sql
-- Table sizes and row counts
SELECT generate_query('show row count for each table');
-- Result: This will generate appropriate queries to count rows in each table

-- Activity monitoring
SELECT generate_query('show order activity by hour of day');
-- Result: SELECT EXTRACT(hour FROM order_date) as hour_of_day,
--                COUNT(*) as order_count
--         FROM public.orders
--         GROUP BY EXTRACT(hour FROM order_date)
--         ORDER BY hour_of_day LIMIT 1000;
```

## Tips for Better Results

### 1. Be Specific
```sql
-- Instead of: "show sales"
SELECT generate_query('show total sales amount by month for 2024');

-- Instead of: "find customers"
SELECT generate_query('find customers with more than 5 orders and total spent over $500');
```

### 2. Use Clear Time References
```sql
-- Good time references:
SELECT generate_query('show orders from last 30 days');
SELECT generate_query('find users registered between January 1, 2024 and March 31, 2024');
SELECT generate_query('show sales for Q1 2024');
```

### 3. Specify Sorting and Limits
```sql
-- Include sorting preferences:
SELECT generate_query('show top 10 customers by total revenue');
SELECT generate_query('list products ordered by price ascending');
SELECT generate_query('show recent orders sorted by date descending');
```

### 4. Reference Column Names When Needed
```sql
-- When ambiguous, specify column names:
SELECT generate_query('find users where user status is active');
SELECT generate_query('show products where stock quantity is less than reorder level');
```

These examples demonstrate the flexibility and power of `pg_ai_query` for various database tasks, from simple queries to complex business intelligence analysis.

## Enhanced Response Formatting Examples

The following examples show how different response configuration options affect the output:

### Example: Enhanced Text Response

**Configuration**:
```ini
[response]
show_explanation = true
show_warnings = true
show_suggested_visualization = true
use_formatted_response = false
```

**Query**:
```sql
SELECT generate_query('monthly sales trend for the last year with top performing categories');
```

**Enhanced Response**:
```sql
SELECT
    DATE_TRUNC('month', o.order_date) as month,
    p.category,
    SUM(o.total_amount) as monthly_sales,
    COUNT(o.id) as order_count
FROM orders o
JOIN order_items oi ON o.id = oi.order_id
JOIN products p ON oi.product_id = p.id
WHERE o.order_date >= CURRENT_DATE - INTERVAL '1 year'
GROUP BY DATE_TRUNC('month', o.order_date), p.category
ORDER BY month DESC, monthly_sales DESC
LIMIT 1000;

-- Explanation:
-- Analyzes monthly sales trends over the past year, broken down by product category.
-- Shows total sales amount and order count for each category per month,
-- helping identify seasonal patterns and top-performing categories over time.

-- Warnings:
-- 1. Multiple JOINs: Query involves 3 table joins which may impact performance on large datasets
-- 2. Date calculation: Uses CURRENT_DATE which may give different results when run at different times
-- 3. Index recommendation: Consider indexes on order_date, order_id, and product_id for optimal performance

-- Suggested Visualization: line
-- Reasoning: Time series data showing trends over months is best displayed as line charts,
-- with separate lines for each category to compare performance patterns
```

### Example: JSON Response

**Configuration**:
```ini
[response]
show_explanation = true
show_warnings = true
show_suggested_visualization = true
use_formatted_response = true
```

**Query**:
```sql
SELECT generate_query('top 10 customers by lifetime value');
```

**JSON Response**:
```json
{
  "query": "SELECT u.id, u.name, u.email, SUM(o.total_amount) as lifetime_value, COUNT(o.id) as total_orders FROM users u JOIN orders o ON u.id = o.user_id GROUP BY u.id, u.name, u.email ORDER BY lifetime_value DESC LIMIT 10;",
  "success": true,
  "explanation": "Identifies the top 10 customers based on their total lifetime purchase value. Shows customer details along with their total spending and number of orders, useful for customer relationship management and targeted marketing.",
  "warnings": [
    "Customer segmentation: Consider including date ranges for more meaningful lifetime value calculations",
    "Performance: Large customer and orders tables may require optimization with proper indexing"
  ],
  "suggested_visualization": "bar",
  "row_limit_applied": true
}
```

### Example: Minimal Response

**Configuration**:
```ini
[response]
show_explanation = false
show_warnings = false
show_suggested_visualization = false
use_formatted_response = false
```

**Query**:
```sql
SELECT generate_query('all products in electronics category');
```

**Minimal Response**:
```sql
SELECT * FROM products WHERE category = 'electronics' LIMIT 1000;
```

### Example: Warnings Only

**Configuration**:
```ini
[response]
show_explanation = false
show_warnings = true
show_suggested_visualization = false
use_formatted_response = false
```

**Query**:
```sql
SELECT generate_query('delete old order records from 2020');
```

**Response with Warnings**:
```sql
DELETE FROM orders WHERE EXTRACT(year FROM order_date) = 2020;

-- Warnings:
-- 1. DESTRUCTIVE OPERATION: This DELETE statement will permanently remove data
-- 2. No transaction control: Consider wrapping in BEGIN/COMMIT for safety
-- 3. Foreign key impact: Deleting orders may affect order_items table if cascade is not set
-- 4. Backup recommendation: Ensure you have recent backups before executing
-- 5. Row count check: Consider SELECT COUNT(*) first to verify the number of rows that will be deleted
```

### Working with JSON Responses in Applications

**Python Example**:
```python
import psycopg2
import json

# Connect to database
conn = psycopg2.connect("dbname=mydb user=myuser")
cur = conn.cursor()

# Execute query with JSON response
cur.execute("SELECT generate_query('monthly sales by product category')")
response_json = cur.fetchone()[0]

# Parse JSON response
response = json.loads(response_json)

if response['success']:
    # Extract query and execute it
    data_query = response['query']
    cur.execute(data_query)
    results = cur.fetchall()

    # Use metadata for visualization
    viz_type = response.get('suggested_visualization', 'table')
    explanation = response.get('explanation', '')
    warnings = response.get('warnings', [])

    print(f"Query explanation: {explanation}")
    print(f"Suggested visualization: {viz_type}")

    if warnings:
        print("Warnings:")
        for warning in warnings:
            print(f"  - {warning}")

    # Process results based on suggested visualization
    if viz_type == 'line':
        # Create line chart
        create_line_chart(results)
    elif viz_type == 'bar':
        # Create bar chart
        create_bar_chart(results)
    else:
        # Display as table
        display_table(results)
```

These enhanced formatting examples show how the new configuration options provide flexible control over response detail and format, making the extension suitable for both interactive use and application integration.