-- AI Query Generator Extension for PostgreSQL
-- Generates SQL queries from natural language using OpenAI

-- Version 1.0

-- Main function: Generate SQL from natural language with automatic schema discovery
CREATE OR REPLACE FUNCTION generate_query(
    natural_language_query text,
    api_key text DEFAULT NULL,
    provider text DEFAULT 'auto'
)
RETURNS text
AS 'MODULE_PATHNAME', 'generate_query'
LANGUAGE C;

-- Example usage:
-- SELECT generate_query('Show me all users created in the last 7 days');
-- SELECT generate_query('Count orders by status');
-- SELECT generate_query('Show me all users', 'your-api-key-here');
-- SELECT generate_query('Show me all users', 'your-api-key-here', 'openai');
-- SELECT generate_query('Show me all users', 'your-api-key-here', 'anthropic');

COMMENT ON FUNCTION generate_query(text, text, text) IS
'Generate a PostgreSQL SELECT query from natural language description with automatic database schema discovery. Provider options: openai, anthropic, auto (default). Pass API key as parameter or configure ~/.pg_ai.config.';

-- Get all tables in the database with metadata
CREATE OR REPLACE FUNCTION get_database_tables()
RETURNS text
AS 'MODULE_PATHNAME', 'get_database_tables'
LANGUAGE C;

-- Get detailed information about a specific table
CREATE OR REPLACE FUNCTION get_table_details(
    table_name text,
    schema_name text DEFAULT 'public'
)
RETURNS text
AS 'MODULE_PATHNAME', 'get_table_details'
LANGUAGE C;

-- Example usage:
-- SELECT pg_get_database_tables();
-- SELECT pg_get_table_details('users');
-- SELECT pg_get_table_details('orders', 'public');

COMMENT ON FUNCTION get_database_tables() IS
'Returns JSON array of all user tables in the database with metadata including table name, schema, type, and estimated row count.';

COMMENT ON FUNCTION get_table_details(text, text) IS
'Returns detailed JSON information about a specific table including columns with their data types, constraints, foreign keys, and indexes.';

