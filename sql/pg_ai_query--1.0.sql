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

COMMENT ON FUNCTION generate_query(text) IS
'Generate SQL query from natural language using configured AI provider.
Parameters:
- natural_language_query: Natural language description of desired query
Returns: Generated SQL query string
Example: SELECT generate_query(''show all users with email ending in gmail.com'');';

COMMENT ON FUNCTION generate_query(text, text) IS
'Generate SQL query with custom API key.
Parameters:
- natural_language_query: Natural language description of desired query
- api_key: API key for the AI provider (overrides config file)
Returns: Generated SQL query string
Example: SELECT generate_query(''show all active users'', ''sk-...'');';

COMMENT ON FUNCTION generate_query(text, text, text) IS
'Generate a PostgreSQL SELECT query from natural language description with automatic database schema discovery.
Parameters:
  - natural_language_query: Natural language description of desired query
  - api_key: API key for the AI provider (NULL to use config file)
  - provider: AI provider name (openai, anthropic, gemini, or auto)
Returns: Generated SQL query string
Example: SELECT generate_query(''show top 10 products by sales'', ''sk-...'', ''openai'');';

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
'Returns JSON array of all user tables in the database with metadata.
Returns: JSON array containing table name, schema, type, and estimated row count for each table
Example: SELECT get_database_tables();';

COMMENT ON FUNCTION get_table_details(text) IS
'Returns detailed information about a specific table in the public schema.
Parameters:
- table_name: Name of the table to inspect
Returns: JSON object with columns (data types, constraints, foreign keys) and indexes
Example: SELECT get_table_details(''users'');';

COMMENT ON FUNCTION get_table_details(text, text) IS
'Returns detailed JSON information about a specific table including columns with their data types, constraints, foreign keys, and indexes.
Parameters:
- table_name: Name of the table to inspect
- schema_name: Schema containing the table (default: public)
Returns: JSON object with complete table schema information
Example: SELECT get_table_details(''orders'', ''public'');';

-- Explain query function: Runs explain analyze and provides AI-generated explanation
CREATE OR REPLACE FUNCTION explain_query(
    query_text text,
    api_key text DEFAULT NULL,
    provider text DEFAULT 'auto'
)
RETURNS text
AS 'MODULE_PATHNAME', 'explain_query'
LANGUAGE C
VOLATILE
SECURITY DEFINER;

-- Example usage:
-- SELECT explain_query('SELECT * FROM users WHERE created_at > NOW() - INTERVAL ''7 days''');
-- SELECT explain_query('SELECT u.name, COUNT(o.id) FROM users u LEFT JOIN orders o ON u.id = o.user_id GROUP BY u.id', 'your-api-key-here');
-- SELECT explain_query('SELECT * FROM products ORDER BY price DESC LIMIT 10', 'your-api-key-here', 'openai');

COMMENT ON FUNCTION explain_query(text) IS
'Analyze query performance and get AI-generated optimization suggestions.
Parameters:
- query_text: SQL query to analyze
Returns: JSON with raw explain output and AI-generated performance insights
Example: SELECT explain_query(''SELECT * FROM users WHERE created_at > NOW() - INTERVAL ''''7 days'''''');';

COMMENT ON FUNCTION explain_query(text, text) IS
'Analyze query performance with custom API key.
Parameters:
- query_text: SQL query to analyze
- api_key: API key for the AI provider (overrides config file)
Returns: JSON with raw explain output and AI-generated performance insights
Example: SELECT explain_query(''SELECT COUNT(*) FROM orders GROUP BY status'', ''sk-...'');';

COMMENT ON FUNCTION explain_query(text, text, text) IS
'Runs explain analyze on a query and returns an AI-generated explanation of the execution plan, performance insights, and optimization suggestions.
Parameters:
- query_text: SQL query to analyze
- api_key: API key for the AI provider (NULL to use config file)
- provider: AI provider name (openai, anthropic, gemini, or auto)
Returns: JSON with raw explain output and AI-generated performance insights
Example: SELECT explain_query(''SELECT * FROM products ORDER BY price DESC LIMIT 10'', ''sk-...'', ''anthropic'');';

