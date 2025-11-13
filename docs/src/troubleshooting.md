# Troubleshooting Guide

This guide helps you diagnose and fix common issues with the `pg_ai_query` extension.

## Installation Issues

### Extension Not Found

**Error**: `extension "pg_ai_query" is not available`

**Causes & Solutions**:

1. **Extension not installed**
   ```bash
   # Check if extension files are present
   ls /usr/share/postgresql/*/extension/pg_ai_query*

   # If missing, reinstall
   cd pg_ai_query
   sudo make install
   ```

2. **Wrong PostgreSQL version**
   ```bash
   # Check PostgreSQL version
   psql -c "SELECT version();"

   # Ensure extension was built for this version
   pg_config --version
   ```

3. **Permission issues**
   ```bash
   # Check file permissions
   ls -la /usr/share/postgresql/*/extension/pg_ai_query*

   # Fix if needed
   sudo chmod 644 /usr/share/postgresql/*/extension/pg_ai_query*
   ```

### Compilation Errors

**Error**: Various C++ compilation errors

**Common Causes**:

1. **Missing dependencies**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install postgresql-server-dev-all cmake gcc g++ libssl-dev

   # CentOS/RHEL
   sudo yum install postgresql-devel cmake gcc-c++ openssl-devel
   ```

2. **Wrong PostgreSQL headers**
   ```bash
   # Check pg_config path
   which pg_config
   pg_config --includedir

   # Use specific version if needed
   PG_CONFIG=/usr/pgsql-14/bin/pg_config make
   ```

3. **C++ standard issues**
   ```bash
   # Ensure C++20 support
   gcc --version  # Should be 8.0+
   ```

## Configuration Issues

### API Key Problems

**Error**: `"API key not configured"` or `"Invalid API key"`

**Diagnosis**:
```sql
-- Enable logging to see configuration loading
-- In ~/.pg_ai.config:
[general]
enable_logging = true
log_level = "DEBUG"
```

**Solutions**:

1. **Check configuration file exists**
   ```bash
   ls -la ~/.pg_ai.config
   cat ~/.pg_ai.config
   ```

2. **Verify API key format**
   ```ini
   [openai]
   api_key = "sk-proj-abc123..."  # Must start with sk- for OpenAI

   [anthropic]
   api_key = "sk-ant-abc123..."   # Must start with sk-ant- for Anthropic
   ```

3. **Test API key directly**
   ```bash
   # Test OpenAI key
   curl -H "Authorization: Bearer YOUR_API_KEY" \
        -H "Content-Type: application/json" \
        -d '{"model":"gpt-3.5-turbo","messages":[{"role":"user","content":"test"}]}' \
        https://api.openai.com/v1/chat/completions
   ```

4. **Check file permissions**
   ```bash
   # Config file should be readable by PostgreSQL user
   chmod 644 ~/.pg_ai.config
   ```

### Configuration Not Loading

**Error**: Configuration seems ignored

**Solutions**:

1. **Restart PostgreSQL session**
   ```sql
   -- Disconnect and reconnect to reload configuration
   \q
   psql -d your_database
   ```

2. **Check file syntax**
   ```bash
   # Verify INI format is correct
   # No spaces around = signs
   # Quotes around string values
   api_key = "your-key-here"  # Correct
   api_key=your-key-here      # Wrong
   ```

3. **Verify home directory**
   ```bash
   echo $HOME
   # Config should be at $HOME/.pg_ai.config
   ```

## Runtime Issues

### No Tables Found

**Error**: `"No tables found"` or empty results

**Diagnosis**:
```sql
-- Check what tables exist
SELECT get_database_tables();

-- Check table permissions
SELECT schemaname, tablename, tableowner
FROM pg_tables
WHERE schemaname NOT IN ('information_schema', 'pg_catalog');
```

**Solutions**:

1. **Create some tables**
   ```sql
   -- Extension only works with user tables
   CREATE TABLE test_table (id SERIAL, name TEXT);
   ```

2. **Check schema focus**
   ```sql
   -- Extension looks at 'public' schema by default
   SET search_path TO public, your_schema;
   ```

### Query Generation Failures

**Error**: `"Query generation failed"` or timeouts

**Common Causes & Solutions**:

1. **Network connectivity**
   ```bash
   # Test connection to AI providers
   curl -I https://api.openai.com/v1/models
   curl -I https://api.anthropic.com/v1/messages
   ```

2. **API rate limiting**
   ```ini
   # Increase timeout in config
   [general]
   request_timeout_ms = 60000  # 60 seconds
   max_retries = 5
   ```

3. **Complex database schema**
   ```sql
   -- For large schemas, try with simpler requests first
   SELECT generate_query('SELECT 1');  -- Test basic functionality
   ```

4. **Model-specific issues**
   ```sql
   -- Try different models
   SELECT generate_query('show tables', null, 'openai');
   SELECT generate_query('show tables', null, 'anthropic');
   ```

### Poor Query Quality

**Issue**: Generated queries are incorrect or suboptimal

**Solutions**:

1. **Be more specific in requests**
   ```sql
   -- Instead of: "show data"
   -- Use: "show user names and email addresses from users table"
   ```

2. **Check schema information**
   ```sql
   -- Ensure tables have good names and structure
   SELECT get_table_details('your_table');
   ```

3. **Try different models**
   ```sql
   -- GPT-4 is generally more accurate than GPT-3.5
   SELECT generate_query('complex query here', null, 'openai');
   ```

4. **Break complex requests into parts**
   ```sql
   -- Instead of one complex request, try multiple simpler ones
   SELECT generate_query('show users');
   SELECT generate_query('show orders for user id 5');
   ```

## Performance Issues

### Slow Query Generation

**Issue**: Extension takes a long time to respond

**Diagnosis**:
```sql
-- Enable detailed logging
-- In ~/.pg_ai.config:
[general]
enable_logging = true
log_level = "DEBUG"
```

**Solutions**:

1. **Optimize database schema**
   ```sql
   -- Reduce number of tables if possible
   -- Use clear table and column names
   -- Add table comments for context
   COMMENT ON TABLE users IS 'Customer information and contact details';
   ```

2. **Adjust timeout settings**
   ```ini
   [general]
   request_timeout_ms = 45000  # Increase from default 30000
   ```

3. **Use faster models**
   ```ini
   [openai]
   default_model = "gpt-3.5-turbo"  # Faster than gpt-4
   ```

### High Memory Usage

**Issue**: PostgreSQL memory usage increases significantly

**Solutions**:

1. **Restart PostgreSQL sessions periodically**
   ```bash
   # Schema information is cached per session
   # Restart to clear cache
   ```

2. **Reduce concurrent users of extension**

3. **Monitor extension usage**
   ```sql
   -- Track function calls
   SELECT * FROM pg_stat_user_functions
   WHERE funcname LIKE '%generate%';
   ```

## Debugging Tools

### Enable Debug Logging

```ini
# ~/.pg_ai.config
[general]
enable_logging = true
log_level = "DEBUG"
enable_postgresql_elog = true
```

### Check Extension Status

```sql
-- Verify extension is loaded
SELECT extname, extversion FROM pg_extension WHERE extname = 'pg_ai_query';

-- Check function availability
\df *generate*
\df *database_tables*
\df *table_details*

-- Test basic functionality
SELECT generate_query('SELECT 1 as test');
```

### Monitor System Resources

```bash
# Check PostgreSQL logs
tail -f /var/log/postgresql/postgresql-*.log

# Monitor network connections
netstat -an | grep :5432

# Check memory usage
ps aux | grep postgres
```

### Test Configuration

```sql
-- Test API connectivity
SELECT generate_query('test query', 'your-test-key', 'openai');

-- Test schema discovery
SELECT get_database_tables();
SELECT get_table_details('your_table');
```

## Common Error Messages

| Error Message | Likely Cause | Solution |
|---------------|--------------|----------|
| `extension "pg_ai_query" is not available` | Extension not installed | Run `make install` |
| `function generate_query does not exist` | Extension not created in DB | Run `CREATE EXTENSION pg_ai_query` |
| `API key not configured` | Missing or invalid API key | Check `~/.pg_ai.config` file |
| `No tables found` | No user tables in database | Create some tables or check permissions |
| `Query generation failed: timeout` | Network or API issues | Check connectivity and increase timeout |
| `Invalid provider: xyz` | Wrong provider name | Use 'openai', 'anthropic', or 'auto' |
| `Table does not exist: xyz` | Table name incorrect | Check table name and schema |

## Getting Additional Help

### Log Analysis

When reporting issues, include:

1. **Extension version**
   ```sql
   SELECT extversion FROM pg_extension WHERE extname = 'pg_ai_query';
   ```

2. **PostgreSQL version**
   ```sql
   SELECT version();
   ```

3. **Error logs with debug enabled**

4. **Configuration file** (with API keys redacted)

### Testing Isolation

To isolate problems:

1. **Test with minimal configuration**
2. **Use simple test tables**
3. **Try different AI providers**
4. **Test basic functions first**

### Community Resources

- GitHub Issues: Report bugs and feature requests
- Documentation: Check for updated troubleshooting info
- PostgreSQL Community: For database-specific issues

## Prevention Best Practices

1. **Regular Updates**: Keep extension and dependencies updated
2. **Monitoring**: Set up logging and monitoring for production use
3. **Testing**: Test configuration changes in development first
4. **Backups**: Always backup before major updates
5. **Documentation**: Document your specific configuration and customizations