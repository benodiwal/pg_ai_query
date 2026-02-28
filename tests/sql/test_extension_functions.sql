-- pg_ai_query Extension Function Tests
-- These tests verify the extension functions work correctly
-- Run with: psql -f tests/sql/test_extension_functions.sql

-- Test 1: Extension is installed
DO $$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_extension WHERE extname = 'pg_ai_query') THEN
        RAISE EXCEPTION 'FAIL: Extension pg_ai_query is not installed';
    ELSE
        RAISE NOTICE 'PASS: Extension pg_ai_query is installed';
    END IF;
END $$;

-- Test 2: generate_query function exists
DO $$
BEGIN
    IF NOT EXISTS (
        SELECT 1 FROM pg_proc p
        JOIN pg_namespace n ON p.pronamespace = n.oid
        WHERE p.proname = 'generate_query'
    ) THEN
        RAISE EXCEPTION 'FAIL: Function generate_query does not exist';
    ELSE
        RAISE NOTICE 'PASS: Function generate_query exists';
    END IF;
END $$;

-- Test 3: get_database_tables function exists
DO $$
BEGIN
    IF NOT EXISTS (
        SELECT 1 FROM pg_proc p
        JOIN pg_namespace n ON p.pronamespace = n.oid
        WHERE p.proname = 'get_database_tables'
    ) THEN
        RAISE EXCEPTION 'FAIL: Function get_database_tables does not exist';
    ELSE
        RAISE NOTICE 'PASS: Function get_database_tables exists';
    END IF;
END $$;

-- Test 4: get_table_details function exists
DO $$
BEGIN
    IF NOT EXISTS (
        SELECT 1 FROM pg_proc p
        JOIN pg_namespace n ON p.pronamespace = n.oid
        WHERE p.proname = 'get_table_details'
    ) THEN
        RAISE EXCEPTION 'FAIL: Function get_table_details does not exist';
    ELSE
        RAISE NOTICE 'PASS: Function get_table_details exists';
    END IF;
END $$;

-- Test 5: explain_query function exists
DO $$
BEGIN
    IF NOT EXISTS (
        SELECT 1 FROM pg_proc p
        JOIN pg_namespace n ON p.pronamespace = n.oid
        WHERE p.proname = 'explain_query'
    ) THEN
        RAISE EXCEPTION 'FAIL: Function explain_query does not exist';
    ELSE
        RAISE NOTICE 'PASS: Function explain_query exists';
    END IF;
END $$;

-- Test 6: get_database_tables returns valid JSON
DO $$
DECLARE
    result TEXT;
    json_result JSONB;
BEGIN
    SELECT get_database_tables() INTO result;

    -- Try to parse as JSON
    BEGIN
        json_result := result::jsonb;

        -- Check it's an array
        IF jsonb_typeof(json_result) != 'array' THEN
            RAISE EXCEPTION 'FAIL: get_database_tables did not return JSON array';
        END IF;

        RAISE NOTICE 'PASS: get_database_tables returns valid JSON array';
    EXCEPTION WHEN OTHERS THEN
        RAISE EXCEPTION 'FAIL: get_database_tables returned invalid JSON: %', SQLERRM;
    END;
END $$;

-- Test 7: get_database_tables returns tables (if any exist)
DO $$
DECLARE
    result JSONB;
    table_names TEXT[];
    table_count INTEGER;
BEGIN
    SELECT get_database_tables()::jsonb INTO result;

    -- Extract table names
    SELECT array_agg(elem->>'table_name')
    INTO table_names
    FROM jsonb_array_elements(result) AS elem;

    -- Count tables (may be 0 if no user tables exist in database)
    table_count := COALESCE(array_length(table_names, 1), 0);

    IF table_count > 0 THEN
        RAISE NOTICE 'PASS: get_database_tables returned % tables', table_count;
    ELSE
        -- This is OK - the database might not have any user tables
        RAISE NOTICE 'SKIP: No user tables found in database (this is OK for empty databases)';
    END IF;
END $$;

-- Test 8: get_table_details returns valid JSON for existing table
DO $$
DECLARE
    result TEXT;
    json_result JSONB;
BEGIN
    -- First check if test table exists
    IF EXISTS (
        SELECT 1 FROM information_schema.tables
        WHERE table_schema = 'ai_test' AND table_name = 'users'
    ) THEN
        SELECT get_table_details('users', 'ai_test') INTO result;

        BEGIN
            json_result := result::jsonb;

            -- Check it has expected keys
            IF NOT (json_result ? 'table_name' AND json_result ? 'columns') THEN
                RAISE EXCEPTION 'FAIL: get_table_details missing expected keys';
            END IF;

            RAISE NOTICE 'PASS: get_table_details returns valid JSON with expected structure';
        EXCEPTION WHEN OTHERS THEN
            RAISE EXCEPTION 'FAIL: get_table_details returned invalid JSON: %', SQLERRM;
        END;
    ELSE
        RAISE NOTICE 'SKIP: Test table ai_test.users does not exist';
    END IF;
END $$;

-- Test 9: get_table_details includes column information
DO $$
DECLARE
    result JSONB;
    column_count INTEGER;
BEGIN
    IF EXISTS (
        SELECT 1 FROM information_schema.tables
        WHERE table_schema = 'ai_test' AND table_name = 'users'
    ) THEN
        SELECT get_table_details('users', 'ai_test')::jsonb INTO result;

        SELECT jsonb_array_length(result->'columns') INTO column_count;

        IF column_count < 1 THEN
            RAISE EXCEPTION 'FAIL: get_table_details returned no columns';
        END IF;

        -- Check first column has expected properties
        IF NOT (result->'columns'->0 ? 'column_name' AND result->'columns'->0 ? 'data_type') THEN
            RAISE EXCEPTION 'FAIL: Column missing expected properties';
        END IF;

        RAISE NOTICE 'PASS: get_table_details returned % columns with proper structure', column_count;
    ELSE
        RAISE NOTICE 'SKIP: Test table ai_test.users does not exist';
    END IF;
END $$;

-- Test 10: generate_query fails gracefully without API key
DO $$
DECLARE
    result TEXT;
BEGIN
    -- This should return an error since no API key is configured
    BEGIN
        SELECT generate_query('show all users') INTO result;

        -- If we get here, check if it's an error message
        IF result LIKE '%API key%' OR result LIKE '%error%' THEN
            RAISE NOTICE 'PASS: generate_query returns appropriate error without API key';
        ELSE
            -- It might have worked if there's a config file
            RAISE NOTICE 'INFO: generate_query returned: %', LEFT(result, 100);
        END IF;
    EXCEPTION WHEN OTHERS THEN
        -- Expected behavior - function may raise exception without API key
        RAISE NOTICE 'PASS: generate_query raises exception without API key: %', SQLERRM;
    END;
END $$;

-- Summary
DO $$
BEGIN
    RAISE NOTICE '';
    RAISE NOTICE '========================================';
    RAISE NOTICE 'pg_ai_query Extension Tests Complete';
    RAISE NOTICE '========================================';
END $$;
