-- Teardown script for pg_ai_query extension tests
-- This file cleans up the test environment

-- Drop test tables
DROP TABLE IF EXISTS pg_ai_test.order_items CASCADE;
DROP TABLE IF EXISTS pg_ai_test.orders CASCADE;
DROP TABLE IF EXISTS pg_ai_test.products CASCADE;
DROP TABLE IF EXISTS pg_ai_test.users CASCADE;

-- Drop test schema
DROP SCHEMA IF EXISTS pg_ai_test CASCADE;

-- Note: We don't drop the extension here as it might be used by other tests
-- Use: DROP EXTENSION IF EXISTS pg_ai_query CASCADE; to remove the extension
