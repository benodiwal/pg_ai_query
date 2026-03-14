-- Setup script for pg_ai_query extension tests
-- This file sets up the test environment

-- Create test schema
CREATE SCHEMA IF NOT EXISTS ai_test;

-- Create test tables for schema introspection tests
CREATE TABLE IF NOT EXISTS ai_test.users (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    active BOOLEAN DEFAULT true
);

CREATE TABLE IF NOT EXISTS ai_test.orders (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES ai_test.users(id),
    total_amount DECIMAL(10, 2) NOT NULL,
    status VARCHAR(50) DEFAULT 'pending',
    order_date DATE DEFAULT CURRENT_DATE
);

CREATE TABLE IF NOT EXISTS ai_test.products (
    id SERIAL PRIMARY KEY,
    name VARCHAR(200) NOT NULL,
    price DECIMAL(10, 2) NOT NULL,
    category VARCHAR(100),
    in_stock BOOLEAN DEFAULT true
);

CREATE TABLE IF NOT EXISTS ai_test.order_items (
    id SERIAL PRIMARY KEY,
    order_id INTEGER REFERENCES ai_test.orders(id),
    product_id INTEGER REFERENCES ai_test.products(id),
    quantity INTEGER NOT NULL,
    unit_price DECIMAL(10, 2) NOT NULL
);

-- Create index for testing
CREATE INDEX IF NOT EXISTS idx_users_email ON ai_test.users(email);
CREATE INDEX IF NOT EXISTS idx_orders_user_id ON ai_test.orders(user_id);

-- Insert some test data
INSERT INTO ai_test.users (name, email, active) VALUES
    ('Alice', 'alice@test.com', true),
    ('Bob', 'bob@test.com', true),
    ('Charlie', 'charlie@test.com', false)
ON CONFLICT (email) DO NOTHING;

INSERT INTO ai_test.products (name, price, category, in_stock) VALUES
    ('Widget', 9.99, 'gadgets', true),
    ('Gadget', 19.99, 'gadgets', true),
    ('Doohickey', 29.99, 'tools', false)
ON CONFLICT DO NOTHING;

-- Ensure the extension is loaded
CREATE EXTENSION IF NOT EXISTS pg_ai_query;
