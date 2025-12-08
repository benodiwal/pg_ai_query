# PostgreSQL AI Query Extension

The **PostgreSQL AI Query Extension** (`pg_ai_query`) is a powerful PostgreSQL extension that allows you to generate SQL queries from natural language descriptions using state-of-the-art AI models from OpenAI, Anthropic, and Google Gemini.

## What is pg_ai_query?

`pg_ai_query` bridges the gap between natural language and SQL by leveraging large language models to understand your intent and automatically generate optimized PostgreSQL queries. The extension integrates directly into your PostgreSQL database, providing a seamless way to query your data using plain English.

## Key Features

- **Natural Language to SQL**: Convert plain English descriptions into valid PostgreSQL queries
- **AI-Powered Query Analysis**: Analyze query performance with EXPLAIN ANALYZE and get optimization insights
- **Automatic Schema Discovery**: The extension automatically analyzes your database schema to understand table structures, relationships, and constraints
- **Multiple AI Providers**: Support for OpenAI (GPT-4, GPT-3.5), Anthropic (Claude), and Google (Gemini) models
- **Intelligent Query Generation**: Generates optimized queries with appropriate JOINs, WHERE clauses, and LIMIT constraints
- **Performance Optimization**: Get AI-powered recommendations for query improvements and index suggestions
- **Safety First**: Built-in protections against dangerous operations and unauthorized access to system tables
- **Configurable**: Flexible configuration system with support for API keys, model selection, and logging
- **PostgreSQL Native**: Runs directly within PostgreSQL as a native extension

## How It Works

1. **Schema Analysis**: The extension automatically discovers and analyzes your database schema
2. **Natural Language Processing**: Your natural language query is processed and understood by AI models
3. **SQL Generation**: The AI generates an appropriate SQL query based on your schema and request
4. **Query Validation**: The generated query is validated for safety and correctness
5. **Execution Ready**: You receive a ready-to-execute SQL query

## Example Usage

### Query Generation
```sql
-- Simple query
SELECT generate_query('show me all users created in the last 7 days');

-- With custom API key
SELECT generate_query('count orders by status', 'your-api-key-here');

-- With specific provider
SELECT generate_query('find top 10 customers by revenue', 'your-api-key', 'openai');
```

### Query Performance Analysis
```sql
-- Analyze query performance
SELECT explain_query('SELECT * FROM users WHERE active = true');

-- Get optimization suggestions for complex queries
SELECT explain_query('
    SELECT u.username, COUNT(o.id) as orders
    FROM users u
    LEFT JOIN orders o ON u.id = o.user_id
    GROUP BY u.username
    ORDER BY orders DESC
');

-- Combine generation and analysis
WITH generated AS (
    SELECT generate_query('find high-value recent orders') as query
)
SELECT explain_query((SELECT query FROM generated));
```

## Supported AI Models

### OpenAI Models
- **GPT-4o**: Latest and most capable model
- **GPT-4**: High-quality reasoning and code generation
- **GPT-3.5 Turbo**: Fast and efficient for simpler queries

### Anthropic Models
- **Claude 3.5 Sonnet**: Advanced reasoning and natural language understanding

### Google Gemini Models
- **Gemini 2.5 Pro**: Flagship model with excellent reasoning
- **Gemini 2.5 Flash**: Fast and cost-effective for most queries
- **Gemini 2.0 Flash**: Previous generation, still highly capable

## Use Cases

- **Data Exploration**: Quickly explore your data without writing complex SQL
- **Query Optimization**: Analyze and optimize slow-performing queries with AI insights
- **Performance Monitoring**: Regular analysis of critical queries for performance regression
- **Business Intelligence**: Generate reports and analytics queries from natural descriptions
- **Learning SQL**: Understand how natural language translates to SQL syntax and learn optimization techniques
- **Index Planning**: Get AI-powered recommendations for database index strategies
- **Rapid Prototyping**: Quickly generate queries for testing and development
- **Documentation**: Generate example queries for database documentation

## Architecture

The extension consists of several key components:

- **Query Generator**: Core engine that processes natural language and generates SQL
- **Query Analyzer**: Performance analysis engine using EXPLAIN ANALYZE and AI insights
- **Schema Discovery**: Automatically analyzes database structure and relationships
- **AI Provider Integration**: Handles communication with OpenAI and Anthropic APIs
- **Configuration Manager**: Manages settings, API keys, and model configurations
- **Safety Validator**: Ensures generated queries are safe and authorized

## Security and Safety

`pg_ai_query` is designed with security in mind:

- **No System Access**: Cannot access system catalogs or sensitive PostgreSQL internals
- **User Table Focus**: Only operates on user-created tables and data
- **Query Validation**: All generated queries are validated before return
- **Configurable Limits**: Built-in LIMIT enforcement to prevent large result sets
- **API Key Protection**: Secure handling of API credentials

## Getting Started

Ready to start generating SQL queries from natural language? Head over to the [Installation](./installation.md) guide to get started, or check out our [Quick Start](./quick-start.md) tutorial for a hands-on introduction.

## Repository

The source code is available on [GitHub](https://github.com/benodiwal/pg_ai_query). Feel free to contribute, report issues, or explore the implementation.