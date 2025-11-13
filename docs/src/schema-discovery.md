# Schema Discovery

The `pg_ai_query` extension includes an intelligent schema discovery system that automatically analyzes your PostgreSQL database structure to provide context for AI-powered query generation. This page explains how schema discovery works and how you can optimize it.

## Overview

Schema discovery is the process by which the extension examines your database to understand:

- **Tables and Views**: What data structures exist
- **Columns and Data Types**: What fields are available and their types
- **Relationships**: How tables connect through foreign keys
- **Constraints**: Primary keys, unique constraints, and check constraints
- **Indexes**: Available indexes for query optimization
- **Statistics**: Row counts and table sizes for context

This information is then used to generate accurate, well-structured SQL queries from your natural language requests.

## How Schema Discovery Works

### 1. Automatic Triggering

Schema discovery happens automatically when:

- The extension is first used in a PostgreSQL session
- You explicitly call schema discovery functions
- The AI needs context to generate a query

```sql
-- These actions trigger schema discovery:
SELECT generate_query('show users');  -- Automatic discovery
SELECT get_database_tables();         -- Explicit discovery
SELECT get_table_details('users');    -- Detailed table analysis
```

### 2. Discovery Process

The extension follows this process:

#### Step 1: Table Enumeration
```sql
-- Extension queries information_schema to find user tables
-- Equivalent to:
SELECT table_name, table_schema, table_type
FROM information_schema.tables
WHERE table_schema NOT IN ('information_schema', 'pg_catalog', 'pg_toast');
```

#### Step 2: Column Analysis
```sql
-- For each table, analyze columns
-- Equivalent to:
SELECT column_name, data_type, is_nullable, column_default
FROM information_schema.columns
WHERE table_name = 'your_table';
```

#### Step 3: Constraint Discovery
```sql
-- Find primary keys, foreign keys, and unique constraints
-- Equivalent to:
SELECT constraint_name, constraint_type, column_name
FROM information_schema.table_constraints tc
JOIN information_schema.constraint_column_usage ccu
  ON tc.constraint_name = ccu.constraint_name;
```

#### Step 4: Relationship Mapping
```sql
-- Discover foreign key relationships
-- Equivalent to:
SELECT
  tc.table_name,
  kcu.column_name,
  ccu.table_name AS foreign_table_name,
  ccu.column_name AS foreign_column_name
FROM information_schema.table_constraints tc
JOIN information_schema.key_column_usage kcu
  ON tc.constraint_name = kcu.constraint_name
JOIN information_schema.constraint_column_usage ccu
  ON ccu.constraint_name = tc.constraint_name
WHERE constraint_type = 'FOREIGN KEY';
```

#### Step 5: Statistics Collection
```sql
-- Gather table statistics for optimization
-- Equivalent to:
SELECT
  schemaname,
  tablename,
  n_tup_ins,
  n_tup_upd,
  n_tup_del,
  n_live_tup
FROM pg_stat_user_tables;
```

### 3. Information Processing

The collected information is then:

1. **Structured** into JSON format for AI consumption
2. **Cached** in the PostgreSQL session for performance
3. **Filtered** to exclude system tables and sensitive information
4. **Contextualized** with business-friendly descriptions

## Schema Discovery Functions

### get_database_tables()

Returns high-level information about all tables in your database.

**Usage:**
```sql
SELECT get_database_tables();
```

**Example Output:**
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

**Information Provided:**
- Table names and schemas
- Table type (BASE TABLE, VIEW, etc.)
- Estimated row counts
- Physical table sizes

### get_table_details()

Returns comprehensive information about a specific table.

**Usage:**
```sql
SELECT get_table_details('table_name', 'schema_name');
```

**Example Output:**
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
      "is_primary_key": true,
      "is_unique": true
    },
    {
      "column_name": "email",
      "data_type": "character varying",
      "character_maximum_length": 150,
      "is_nullable": false,
      "is_unique": true
    },
    {
      "column_name": "created_at",
      "data_type": "timestamp without time zone",
      "is_nullable": true,
      "column_default": "CURRENT_TIMESTAMP"
    }
  ],
  "constraints": [
    {
      "constraint_name": "users_pkey",
      "constraint_type": "PRIMARY KEY",
      "column_names": ["id"]
    }
  ],
  "foreign_keys": [],
  "indexes": [
    {
      "index_name": "users_email_idx",
      "columns": ["email"],
      "is_unique": true
    }
  ]
}
```

## Optimizing Schema Discovery

### 1. Table and Column Naming

Use clear, descriptive names that help the AI understand your data:

**Good Examples:**
```sql
-- Clear table names
CREATE TABLE customer_orders (...);
CREATE TABLE product_categories (...);
CREATE TABLE user_preferences (...);

-- Descriptive column names
CREATE TABLE users (
    user_id SERIAL PRIMARY KEY,
    full_name VARCHAR(100),
    email_address VARCHAR(150),
    registration_date TIMESTAMP,
    account_status VARCHAR(20)
);
```

**Less Optimal:**
```sql
-- Unclear abbreviations
CREATE TABLE usr (...);
CREATE TABLE prod_cat (...);

-- Ambiguous column names
CREATE TABLE users (
    id INT,
    nm VARCHAR(100),
    em VARCHAR(150),
    dt TIMESTAMP,
    st VARCHAR(20)
);
```

### 2. Adding Table Comments

Enhance schema discovery with descriptive comments:

```sql
-- Table-level comments
COMMENT ON TABLE users IS 'Customer information and account details';
COMMENT ON TABLE orders IS 'Customer purchase orders and transaction history';
COMMENT ON TABLE products IS 'Product catalog with pricing and inventory';

-- Column-level comments
COMMENT ON COLUMN users.account_status IS 'User account status: active, inactive, suspended';
COMMENT ON COLUMN orders.fulfillment_status IS 'Order processing status: pending, shipped, delivered, cancelled';
```

### 3. Proper Constraint Definition

Well-defined constraints help the AI understand data relationships:

```sql
-- Primary keys
ALTER TABLE users ADD CONSTRAINT users_pkey PRIMARY KEY (user_id);

-- Foreign keys with descriptive names
ALTER TABLE orders ADD CONSTRAINT fk_orders_customer
    FOREIGN KEY (customer_id) REFERENCES users(user_id);

-- Meaningful unique constraints
ALTER TABLE users ADD CONSTRAINT unique_user_email
    UNIQUE (email_address);

-- Check constraints with business logic
ALTER TABLE orders ADD CONSTRAINT check_order_amount_positive
    CHECK (total_amount > 0);
```

### 4. Strategic Indexing

Indexes help the AI understand query optimization opportunities:

```sql
-- Indexes on frequently queried columns
CREATE INDEX idx_users_registration_date ON users(registration_date);
CREATE INDEX idx_orders_order_date ON orders(order_date);

-- Composite indexes for common query patterns
CREATE INDEX idx_orders_customer_date ON orders(customer_id, order_date);
```

## Schema Discovery Best Practices

### 1. Database Design

- **Normalized Structure**: Use proper normalization to make relationships clear
- **Consistent Naming**: Follow consistent naming conventions across tables
- **Foreign Key Relationships**: Always define foreign key constraints
- **Meaningful Defaults**: Use sensible default values where appropriate

### 2. Documentation

```sql
-- Document complex business rules
COMMENT ON TABLE order_items IS 'Individual line items within customer orders, linked to product catalog';
COMMENT ON COLUMN products.discontinued_date IS 'Date when product was discontinued; NULL means still active';
```

### 3. Data Types

Choose appropriate data types that convey meaning:

```sql
-- Good: Specific types
CREATE TABLE events (
    event_date DATE,              -- Clearly a date
    event_time TIME,              -- Clearly a time
    duration_minutes INTEGER,     -- Clearly numeric
    event_type VARCHAR(50),       -- Clearly textual with reasonable limit
    is_public BOOLEAN            -- Clearly boolean
);

-- Less clear: Generic types
CREATE TABLE events (
    event_date TEXT,              -- Could be anything
    event_time TEXT,              -- Ambiguous format
    duration_minutes TEXT,        -- Should be numeric
    event_type TEXT,              -- No length constraint
    is_public TEXT               -- Should be boolean
);
```

### 4. Schema Organization

Organize related tables logically:

```sql
-- Group related tables with consistent prefixes
CREATE TABLE user_accounts (...);
CREATE TABLE user_preferences (...);
CREATE TABLE user_sessions (...);

CREATE TABLE product_catalog (...);
CREATE TABLE product_categories (...);
CREATE TABLE product_reviews (...);

CREATE TABLE order_headers (...);
CREATE TABLE order_items (...);
CREATE TABLE order_shipments (...);
```

## Performance Considerations

### Schema Discovery Caching

- **Session-level Caching**: Schema information is cached per PostgreSQL session
- **Cache Invalidation**: Schema cache is cleared when the session ends
- **Refresh Triggers**: Schema is re-analyzed if tables are modified

### Optimization Tips

1. **Minimize Table Count**: Large numbers of tables slow down discovery
2. **Use Views**: Create views to present simplified interfaces to complex schemas
3. **Partition Strategy**: Consider table partitioning for very large tables
4. **Statistics Updates**: Keep table statistics current with `ANALYZE`

```sql
-- Update statistics for better schema discovery
ANALYZE users;
ANALYZE orders;

-- Or update all tables
ANALYZE;
```

## Security Considerations

### What Schema Discovery Accesses

✅ **Accessed Information:**
- User table structures
- Column names and types
- Constraint definitions
- Index information
- Table statistics

❌ **Not Accessed:**
- Actual data content
- System catalog details
- User passwords or sensitive data
- Database configuration

### Privacy Protection

The schema discovery system:
- Only reads metadata, never actual data
- Respects PostgreSQL's permission system
- Excludes system tables and schemas
- Never transmits sensitive information to external APIs

## Troubleshooting Schema Discovery

### Common Issues

**Problem**: "No tables found"
```sql
-- Check if tables exist
SELECT table_name FROM information_schema.tables
WHERE table_schema = 'public' AND table_type = 'BASE TABLE';

-- Verify permissions
SELECT has_table_privilege('users', 'SELECT');
```

**Problem**: Missing relationships
```sql
-- Check foreign key constraints
SELECT
    tc.table_name,
    kcu.column_name,
    ccu.table_name AS foreign_table_name
FROM information_schema.table_constraints tc
JOIN information_schema.key_column_usage kcu
  ON tc.constraint_name = kcu.constraint_name
WHERE tc.constraint_type = 'FOREIGN KEY';
```

**Problem**: Outdated statistics
```sql
-- Update table statistics
ANALYZE your_table_name;

-- Check last analyze time
SELECT schemaname, tablename, last_analyze
FROM pg_stat_user_tables;
```

Schema discovery is a powerful feature that makes your database "AI-ready" by providing the context needed for intelligent query generation. Following these best practices will help you get the most accurate and useful results from the `pg_ai_query` extension.