# Frequently Asked Questions (FAQ)

This page answers common questions about the `pg_ai_query` extension.

## General Questions

### What is pg_ai_query?

`pg_ai_query` is a PostgreSQL extension that converts natural language descriptions into SQL queries using AI models from OpenAI and Anthropic. It integrates directly with your PostgreSQL database to provide intelligent query generation.

### How does it work?

1. You provide a natural language description of what you want to query
2. The extension analyzes your database schema automatically
3. It sends the schema context and your request to an AI provider
4. The AI generates a PostgreSQL-compatible SQL query
5. The extension returns the SQL query for you to execute

### Is my data sent to AI providers?

**No.** Only your database schema metadata (table names, column names, relationships) is sent to AI providers, never your actual data. The extension is designed with privacy in mind.

## Installation and Setup

### What PostgreSQL versions are supported?

PostgreSQL 12 and later are supported. We recommend PostgreSQL 14 or later for the best experience.

### Do I need to install anything besides PostgreSQL?

You need:
- PostgreSQL with development headers
- C++ compiler with C++20 support
- CMake for building
- An API key from OpenAI or Anthropic

### Can I use this with cloud PostgreSQL services?

**It depends.** The extension needs to be compiled and installed on the PostgreSQL server. This works with:
- Self-hosted PostgreSQL
- VPS or cloud instances where you have admin access
- Some managed services that support custom extensions

It won't work with fully managed services like AWS RDS, Google Cloud SQL, or Azure Database for PostgreSQL that don't allow custom extensions.

### How do I get an API key?

**For OpenAI:**
1. Visit [platform.openai.com](https://platform.openai.com)
2. Create an account and add billing information
3. Go to API Keys section and create a new key

**For Anthropic:**
1. Visit [console.anthropic.com](https://console.anthropic.com)
2. Create an account and add credits
3. Go to API Keys section and create a new key

## Usage Questions

### Why does it say "No tables found"?

This happens when:
- Your database has no user-created tables (only system tables)
- The extension user doesn't have permission to access tables
- You're in the wrong schema

**Solution:** Create some tables or check permissions:
```sql
-- Check if you have tables
SELECT table_name FROM information_schema.tables
WHERE table_schema = 'public' AND table_type = 'BASE TABLE';

-- Check permissions
SELECT has_table_privilege('your_table', 'SELECT');
```

### Can I use this for INSERT, UPDATE, or DELETE queries?

**Yes,** but with caution. The extension can generate all types of SQL queries, but:
- Always review generated queries before executing
- Consider using a read-only database user for safety
- Be especially careful with data modification queries

### How accurate are the generated queries?

Accuracy depends on several factors:
- **Schema quality**: Well-named tables and columns improve results
- **AI model**: GPT-4 and Claude generally produce better results than GPT-3.5
- **Query complexity**: Simple queries are more accurate than complex ones
- **Request clarity**: Specific requests generate better queries

### Can I customize the AI model used?

**Yes.** You can specify the model in your configuration:

```ini
[openai]
default_model = "gpt-4o"  # or "gpt-4", "gpt-3.5-turbo"

[anthropic]
default_model = "claude-3-5-sonnet-20241022"
```

You can also specify the provider per query:
```sql
SELECT generate_query('show users', null, 'openai');
SELECT generate_query('show users', null, 'anthropic');
```

## Performance Questions

### How fast is query generation?

Typical response times:
- **GPT-3.5-turbo**: 1-2 seconds
- **GPT-4/GPT-4o**: 2-5 seconds
- **Claude 3.5 Sonnet**: 3-6 seconds

Performance depends on:
- AI provider response time
- Database schema complexity
- Network latency

### Does it cache results?

**Schema information** is cached per PostgreSQL session, but **AI responses are not cached**. Each query generation makes a fresh API request.

### How much does it cost to use?

Costs depend on your AI provider and usage:
- **GPT-3.5-turbo**: ~$0.001-0.005 per query
- **GPT-4**: ~$0.10-0.20 per query
- **GPT-4o**: ~$0.02-0.05 per query
- **Claude 3.5 Sonnet**: ~$0.03-0.06 per query

Actual costs vary based on schema complexity and query length.

## Configuration Questions

### Where should I put the configuration file?

Place it at `~/.pg_ai.config` in the home directory of the user running PostgreSQL (often the `postgres` user).

### Can I use environment variables instead of a config file?

Currently, only configuration files are supported. Place your configuration at `~/.pg_ai.config`.

### How do I enable logging for debugging?

```ini
[general]
enable_logging = true
log_level = "DEBUG"
```

Then restart your PostgreSQL session and check the logs.

### Can I configure multiple providers?

**Yes.** Configure both OpenAI and Anthropic:

```ini
[openai]
api_key = "sk-proj-openai-key"
default_model = "gpt-4o"

[anthropic]
api_key = "sk-ant-anthropic-key"
default_model = "claude-3-5-sonnet-20241022"
```

The extension will use the first configured provider as default.

## Troubleshooting

### "Extension not found" error

Make sure the extension is properly installed:
```bash
# Check if extension files exist
ls /usr/share/postgresql/*/extension/pg_ai_query*

# Reinstall if needed
cd pg_ai_query
sudo make install
```

### "API key not configured" error

Check your configuration:
```bash
# Verify config file exists and has correct content
cat ~/.pg_ai.config

# Check file permissions
ls -la ~/.pg_ai.config  # Should be readable by PostgreSQL user
```

### "Query generation failed" errors

This usually indicates:
- Network connectivity issues
- Invalid API key
- AI provider service problems
- Rate limiting

**Solutions:**
- Check your internet connection
- Verify API key is correct and active
- Try a different provider
- Wait a few minutes and retry

### Generated queries are wrong

**Tips for better results:**
- Be more specific in your requests
- Use proper table and column names
- Add table comments to provide context
- Try different AI models
- Break complex requests into simpler parts

### Extension crashes PostgreSQL

This shouldn't happen in normal operation. If it does:
1. Check PostgreSQL logs for error messages
2. Ensure you have the latest version of the extension
3. Report the issue with reproduction steps

## Security Questions

### Is it safe to use in production?

**Yes,** with proper precautions:
- Use a read-only database user for query generation
- Always review generated queries before execution
- Monitor API usage and costs
- Keep API keys secure
- Enable logging for audit trails

### What data is sent to AI providers?

Only database metadata:
- Table names and schemas
- Column names and data types
- Constraint definitions
- Relationship information

**Never sent:**
- Actual data content
- Database credentials
- User information

### Can it generate harmful queries?

The extension includes safety features:
- Automatic LIMIT clauses on SELECT statements
- Blocks access to system tables
- Validates query structure

However, always review generated queries, especially for data modification operations.

### How do I secure API keys?

- Set file permissions: `chmod 600 ~/.pg_ai.config`
- Never commit keys to version control
- Use secure configuration files in production
- Rotate keys regularly
- Monitor usage through provider dashboards

## Business Questions

### Can I use this commercially?

**Yes.** The extension itself is open source, but you need to comply with:
- Your AI provider's terms of service
- PostgreSQL's license
- Your organization's data policies

### What about compliance (GDPR, HIPAA, etc.)?

Consider these factors:
- Schema metadata may contain sensitive information
- AI providers have different compliance certifications
- You may need data processing agreements with AI providers
- Consider on-premises AI models for sensitive environments

### Is there support available?

- **Community Support**: GitHub issues and discussions
- **Documentation**: Comprehensive docs with examples
- **Enterprise Support**: May be available for commercial deployments

## Advanced Usage

### Can I extend the extension?

The extension is open source and designed to be extensible. You can:
- Add support for new AI providers
- Customize query generation logic
- Add new schema analysis features
- Contribute improvements back to the project

### Can I use local AI models?

Currently, only cloud-based OpenAI and Anthropic models are supported. Local model support is being considered for future versions.

### How do I optimize for my specific use case?

- Choose appropriate AI models for your needs
- Design your schema with clear, descriptive names
- Add comments to tables and columns
- Create views for complex business logic
- Monitor usage and costs
- Train your users on effective natural language queries

### Can I integrate this with business intelligence tools?

**Yes.** Many BI tools can call PostgreSQL functions, so you can integrate query generation into dashboards and reports. Consider creating wrapper functions for specific business use cases.

---

**Don't see your question here?** Check the [Troubleshooting Guide](./troubleshooting.md) or open an issue on the [GitHub repository](https://github.com/benodiwal/pg_ai_query/issues).