-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_ai_query" to load this file. \quit

-- Function to generate SQL queries from natural language
CREATE OR REPLACE FUNCTION pg_gen_query(query_description text)
RETURNS text
AS 'MODULE_PATHNAME', 'pg_gen_query'
LANGUAGE C STRICT;

-- Function to execute generated queries (returns result as text)
CREATE OR REPLACE FUNCTION pg_gen_and_execute(query_description text)
RETURNS text
AS 'MODULE_PATHNAME', 'pg_gen_and_execute'
LANGUAGE C STRICT;

-- Function to configure AI provider settings
CREATE OR REPLACE FUNCTION pg_ai_configure(provider text, api_key text)
RETURNS boolean
AS 'MODULE_PATHNAME', 'pg_ai_configure'
LANGUAGE C STRICT;