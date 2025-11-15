# Performance & Best Practices

This guide provides recommendations for using `pg_ai_query` effectively in production environments, optimizing performance, and following security best practices.

## Performance Optimization

### Database Design for AI Query Generation

#### 1. Schema Design

**Use Descriptive Naming**
```sql
-- Good: Clear, business-friendly names
CREATE TABLE customer_orders (
    order_id SERIAL PRIMARY KEY,
    customer_id INTEGER,
    order_date TIMESTAMP,
    total_amount DECIMAL(10,2),
    order_status VARCHAR(20)
);

-- Avoid: Cryptic abbreviations
CREATE TABLE cust_ord (
    id INT,
    cid INT,
    dt TIMESTAMP,
    amt DECIMAL,
    st VARCHAR
);
```

**Proper Relationships**
```sql
-- Always define foreign key relationships
ALTER TABLE customer_orders
ADD CONSTRAINT fk_customer_orders_customer
FOREIGN KEY (customer_id) REFERENCES customers(customer_id);

-- Add meaningful constraints
ALTER TABLE customer_orders
ADD CONSTRAINT check_positive_amount CHECK (total_amount > 0);
```

**Strategic Indexing**
```sql
-- Index frequently queried columns
CREATE INDEX idx_orders_date ON customer_orders(order_date);
CREATE INDEX idx_orders_customer ON customer_orders(customer_id);
CREATE INDEX idx_orders_status ON customer_orders(order_status);

-- Composite indexes for common query patterns
CREATE INDEX idx_orders_customer_date
ON customer_orders(customer_id, order_date);
```

#### 2. Data Types and Constraints

**Choose Appropriate Data Types**
```sql
-- Use specific types that convey meaning
CREATE TABLE products (
    product_id SERIAL PRIMARY KEY,
    product_name VARCHAR(200) NOT NULL,      -- Limited length
    category_id INTEGER,                     -- Clear numeric ID
    price DECIMAL(10,2),                     -- Exact decimal for money
    is_active BOOLEAN DEFAULT true,          -- Clear boolean
    created_date DATE,                       -- Date only, not timestamp
    last_updated TIMESTAMP DEFAULT NOW()     -- Full timestamp for updates
);
```

**Add Table and Column Comments**
```sql
COMMENT ON TABLE products IS 'Product catalog with pricing and inventory information';
COMMENT ON COLUMN products.category_id IS 'References product_categories.category_id';
COMMENT ON COLUMN products.is_active IS 'false = discontinued product, true = available for sale';
```

### Query Generation Performance

#### 1. Optimize Natural Language Requests

**Be Specific and Clear**
```sql
-- Good: Specific request
SELECT generate_query('show customers who placed orders in the last 30 days with their total order value');

-- Less optimal: Vague request
SELECT generate_query('show customer stuff');
```

**Include Context When Needed**
```sql
-- For complex schemas, provide business context
SELECT generate_query('show quarterly revenue trends by product category for fiscal year 2024');

-- Specify table relationships when ambiguous
SELECT generate_query('show orders joined with customer information including customer names and emails');
```

#### 2. Schema Size Optimization

**Limit Table Discovery Scope**
```sql
-- For very large schemas, use specific schemas
-- Configure to focus on business tables only
SELECT generate_query('show sales data from last month');
```

**Use Views for Complex Schemas**
```sql
-- Create business-friendly views
CREATE VIEW sales_summary AS
SELECT
    o.order_id,
    c.customer_name,
    o.order_date,
    o.total_amount,
    p.product_name
FROM orders o
JOIN customers c ON o.customer_id = c.customer_id
JOIN order_items oi ON o.order_id = oi.order_id
JOIN products p ON oi.product_id = p.product_id;

-- AI can then reference the simplified view
SELECT generate_query('show recent sales from sales_summary view');
```

#### 3. Caching and Session Management

**Understand Caching Behavior**
- Schema information is cached per PostgreSQL session
- Reconnecting clears the cache and triggers re-analysis
- Keep sessions alive for better performance

**Session Management**
```python
# Python example: Reuse connections
import psycopg2
from psycopg2 import pool

# Create connection pool
connection_pool = psycopg2.pool.SimpleConnectionPool(1, 20, database="mydb")

def generate_ai_query(natural_query):
    conn = connection_pool.getconn()
    try:
        cursor = conn.cursor()
        cursor.execute("SELECT generate_query(%s)", (natural_query,))
        return cursor.fetchone()[0]
    finally:
        connection_pool.putconn(conn)
```

### Configuration Optimization

#### 1. Provider and Model Selection

**Choose Appropriate Models**
```ini
# For production: Balance speed and accuracy
[openai]
api_key = "your-key"
default_model = "gpt-4"  # Good balance

# For development: Use faster models
[openai]
default_model = "gpt-3.5-turbo"  # Faster, cheaper
```

**Configure Timeouts**
```ini
[general]
request_timeout_ms = 45000  # Increase for complex schemas
max_retries = 5            # Increase for production reliability
```

#### 2. Logging Configuration

**Production Logging**
```ini
[general]
enable_logging = false      # Disable for performance
enable_postgresql_elog = true  # Use PostgreSQL logging only
```

**Development Logging**
```ini
[general]
enable_logging = true
log_level = "INFO"         # or "DEBUG" for troubleshooting
```

## Security Best Practices

### 1. API Key Security

**Secure Storage**
```bash
# Set proper file permissions
chmod 600 ~/.pg_ai.config

# Ensure correct ownership
chown postgres:postgres ~/.pg_ai.config  # For PostgreSQL user
```

**Configuration Files Only**
```ini
# Use secure configuration files
[openai]
api_key = "your-key-here"
```

**Key Rotation**
```ini
# Regularly rotate API keys
[openai]
api_key = "new-rotated-key"
```

### 2. Database Security

**Principle of Least Privilege**
```sql
-- Create dedicated user for AI queries
CREATE USER ai_query_user WITH PASSWORD 'secure_password';

-- Grant only necessary permissions
GRANT SELECT ON ALL TABLES IN SCHEMA public TO ai_query_user;
GRANT USAGE ON SCHEMA public TO ai_query_user;

-- Revoke dangerous permissions
REVOKE INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA public FROM ai_query_user;
```

**Schema Isolation**
```sql
-- Create separate schema for AI-accessible views
CREATE SCHEMA ai_views;

-- Create simplified, safe views
CREATE VIEW ai_views.customer_summary AS
SELECT
    customer_id,
    customer_name,
    city,
    registration_date
FROM customers
WHERE is_active = true;  -- Only show active customers

-- Grant access only to AI schema
GRANT USAGE ON SCHEMA ai_views TO ai_query_user;
GRANT SELECT ON ALL TABLES IN SCHEMA ai_views TO ai_query_user;
```

### 3. Query Validation

**Always Review Generated Queries**
```python
def safe_ai_query_execution(natural_query):
    # Generate the query
    generated_sql = generate_query(natural_query)

    # Log for audit
    logger.info(f"Generated SQL: {generated_sql}")

    # Basic safety checks
    if any(keyword in generated_sql.upper() for keyword in ['DROP', 'DELETE', 'TRUNCATE']):
        raise SecurityError("Potentially dangerous SQL generated")

    # Execute with read-only user
    return execute_with_readonly_user(generated_sql)
```

**Implement Query Whitelisting**
```sql
-- Create function to validate query patterns
CREATE OR REPLACE FUNCTION validate_ai_query(query_text TEXT)
RETURNS BOOLEAN AS $$
BEGIN
    -- Only allow SELECT statements
    IF NOT query_text ILIKE 'SELECT%' THEN
        RETURN FALSE;
    END IF;

    -- Block system table access
    IF query_text ILIKE '%pg_%' OR query_text ILIKE '%information_schema%' THEN
        RETURN FALSE;
    END IF;

    RETURN TRUE;
END;
$$ LANGUAGE plpgsql;
```

### 4. Audit and Monitoring

**Query Logging**
```python
import logging

# Set up audit logging
audit_logger = logging.getLogger('ai_query_audit')
audit_handler = logging.FileHandler('/var/log/ai_queries.log')
audit_logger.addHandler(audit_handler)

def audited_generate_query(natural_query, user_id):
    audit_logger.info(f"User {user_id} requested: {natural_query}")

    result = generate_query(natural_query)

    audit_logger.info(f"Generated for user {user_id}: {result}")
    return result
```

**Monitor API Usage**
```ini
# Enable logging to monitor API costs
[general]
enable_logging = true
log_level = "INFO"
```

## Production Deployment

### 1. Architecture Patterns

**Application-Level Integration**
```python
class AIQueryService:
    def __init__(self):
        self.connection_pool = create_pool()
        self.cache = {}

    def generate_query(self, natural_query, user_context=None):
        # Add user context to improve query generation
        contextual_query = f"{natural_query}"
        if user_context:
            contextual_query += f" for user role {user_context['role']}"

        return self._execute_ai_query(contextual_query)

    def _execute_ai_query(self, query):
        # Implement caching, security checks, etc.
        pass
```

**Microservice Pattern**
```yaml
# Docker Compose example
version: '3.8'
services:
  ai-query-service:
    image: your-app:latest
    environment:
      - DATABASE_URL=postgresql://ai_user:pass@db:5432/mydb
    volumes:
      - ./pg_ai_config:/etc/pg_ai
    depends_on:
      - database

  database:
    image: postgres:14
    environment:
      - POSTGRES_DB=mydb
    volumes:
      - postgres_data:/var/lib/postgresql/data
```

### 2. Error Handling

**Graceful Degradation**
```python
def robust_query_generation(natural_query, fallback_provider=None):
    try:
        return generate_query(natural_query)
    except APIError as e:
        if fallback_provider:
            logger.warning(f"Primary provider failed: {e}, trying {fallback_provider}")
            return generate_query(natural_query, provider=fallback_provider)
        else:
            logger.error(f"Query generation failed: {e}")
            return None
    except Exception as e:
        logger.error(f"Unexpected error: {e}")
        return None
```

**Retry Logic**
```python
import time
from functools import wraps

def retry_with_backoff(max_retries=3, base_delay=1):
    def decorator(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            for attempt in range(max_retries):
                try:
                    return func(*args, **kwargs)
                except Exception as e:
                    if attempt == max_retries - 1:
                        raise
                    delay = base_delay * (2 ** attempt)
                    time.sleep(delay)
            return None
        return wrapper
    return decorator

@retry_with_backoff(max_retries=3)
def generate_query_with_retry(natural_query):
    return generate_query(natural_query)
```

### 3. Monitoring and Alerting

**Key Metrics to Monitor**
- Query generation success rate
- Average response time
- API cost per query
- Error rates by provider
- Schema discovery performance

**Alerting Setup**
```python
import prometheus_client

# Prometheus metrics
query_generation_duration = prometheus_client.Histogram(
    'ai_query_generation_duration_seconds',
    'Time spent generating AI queries'
)

query_generation_errors = prometheus_client.Counter(
    'ai_query_generation_errors_total',
    'Total number of query generation errors',
    ['provider', 'error_type']
)

@query_generation_duration.time()
def monitored_generate_query(natural_query):
    try:
        return generate_query(natural_query)
    except Exception as e:
        query_generation_errors.labels(
            provider='openai',
            error_type=type(e).__name__
        ).inc()
        raise
```

## Cost Management

### 1. Usage Optimization

**Request Optimization**
```python
def optimize_query_request(natural_query):
    # Cache common queries
    cache_key = hashlib.md5(natural_query.encode()).hexdigest()
    if cache_key in query_cache:
        return query_cache[cache_key]

    # Use cheaper model for simple queries
    if is_simple_query(natural_query):
        result = generate_query(natural_query, provider='openai', model='gpt-3.5-turbo')
    else:
        result = generate_query(natural_query, provider='openai', model='gpt-4')

    query_cache[cache_key] = result
    return result

def is_simple_query(query):
    simple_patterns = ['show', 'list', 'count', 'find']
    return any(pattern in query.lower() for pattern in simple_patterns)
```

### 2. Budget Controls

**API Cost Tracking**
```python
class CostTracker:
    def __init__(self, monthly_budget=100):
        self.monthly_budget = monthly_budget
        self.current_spend = 0

    def track_request(self, provider, model, tokens_used):
        cost = calculate_cost(provider, model, tokens_used)
        self.current_spend += cost

        if self.current_spend > self.monthly_budget * 0.9:
            logger.warning("Approaching monthly budget limit")

        if self.current_spend > self.monthly_budget:
            raise BudgetExceededException("Monthly budget exceeded")
```

### 3. Usage Analytics

**Query Pattern Analysis**
```sql
-- Analyze query patterns for optimization
CREATE TABLE ai_query_log (
    id SERIAL PRIMARY KEY,
    user_id INTEGER,
    natural_query TEXT,
    generated_sql TEXT,
    provider VARCHAR(20),
    model VARCHAR(50),
    execution_time_ms INTEGER,
    created_at TIMESTAMP DEFAULT NOW()
);

-- Find most common query types
SELECT
    LEFT(natural_query, 50) as query_pattern,
    COUNT(*) as frequency,
    AVG(execution_time_ms) as avg_time
FROM ai_query_log
GROUP BY LEFT(natural_query, 50)
ORDER BY frequency DESC;
```

Following these best practices will help you deploy `pg_ai_query` successfully in production environments while maintaining security, performance, and cost efficiency.