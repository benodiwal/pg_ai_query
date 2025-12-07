# AI Providers

The `pg_ai_query` extension supports multiple AI providers, each with different models, capabilities, and pricing structures. This guide helps you choose the right provider and model for your use case.

## Supported Providers

### OpenAI

**Overview**: OpenAI provides the GPT family of models, known for strong natural language understanding and code generation capabilities.

**API Endpoint**: `https://api.openai.com/v1/`

**Available Models**:
The extension supports any valid OpenAI model name. The following are common examples:

| Model | Description | Context Length | Best For |
|-------|-------------|----------------|----------|
| `gpt-4o` | Latest GPT-4 Omni model | 128,000 tokens | Complex queries, best accuracy |
| `gpt-4` | Standard GPT-4 model | 8,192 tokens | High-quality SQL generation |
| `gpt-3.5-turbo` | Fast and efficient model | 4,096 tokens | Simple queries, fast responses |

### Anthropic

**Overview**: Anthropic provides Claude models, known for thoughtful reasoning and safety-conscious outputs.

**API Endpoint**: `https://api.anthropic.com/v1/`

**Available Models**:
The extension supports any valid Anthropic model name. The following are common examples:

| Model | Description | Context Length | Best For |
|-------|-------------|----------------|----------|
| `claude-sonnet-4-5-20250929` | Latest Claude 3.5 Sonnet | 200,000 tokens | Complex analysis, detailed reasoning |

## Provider Selection

### Automatic Provider Selection

When you use `provider = 'auto'` (the default), the extension selects a provider based on:

1. **Configuration Priority**: First configured provider with a valid API key
2. **Fallback Logic**: If primary provider fails, tries others
3. **Default Order**: OpenAI → Anthropic

```sql
-- Uses automatic provider selection
SELECT generate_query('show recent orders');
```

### Explicit Provider Selection

You can specify which provider to use:

```sql
-- Force OpenAI
SELECT generate_query('show users', null, 'openai');

-- Force Anthropic
SELECT generate_query('show users', null, 'anthropic');
```

### Configuration-Based Selection

Set your preferred provider in the configuration file:

```ini
[general]
# Provider priority is determined by order in config file

[openai]
api_key = "your-openai-key"
default_model = "gpt-4o"

[anthropic]
api_key = "your-anthropic-key"
default_model = "claude-sonnet-4-5-20250929"
```

## Provider Comparison

### Performance Characteristics

| Aspect | OpenAI GPT-4o | OpenAI GPT-3.5 | Anthropic Claude |
|--------|---------------|-----------------|------------------|
| **Response Time** | 2-5 seconds | 1-2 seconds | 3-6 seconds |
| **Accuracy** | Excellent | Good | Excellent |
| **Complex Queries** | Excellent | Fair | Excellent |
| **SQL Knowledge** | Excellent | Good | Very Good |
| **Cost** | High | Low | Medium |

### Use Case Recommendations

#### For Simple Queries
**Recommended**: OpenAI GPT-3.5-turbo
```sql
-- Simple data retrieval
SELECT generate_query('show all users', null, 'openai');
-- Configuration: default_model = "gpt-3.5-turbo"
```

**Why**: Fast, cost-effective, sufficient accuracy for basic queries.

#### For Complex Analytics
**Recommended**: OpenAI GPT-4o or Anthropic Claude
```sql
-- Complex business intelligence
SELECT generate_query(
    'calculate monthly customer retention rates with cohort analysis',
    null,
    'openai'
);
-- Configuration: default_model = "gpt-4o"
```

**Why**: Better reasoning for complex multi-table joins and advanced analytics.

#### For Data Exploration
**Recommended**: Anthropic Claude
```sql
-- Exploratory analysis
SELECT generate_query(
    'analyze sales trends and identify anomalies in the last quarter',
    null,
    'anthropic'
);
```

**Why**: Excellent at understanding context and generating insightful queries.

#### For Production Systems
**Recommended**: OpenAI GPT-4 (balanced performance/cost)
```sql
-- Production queries
SELECT generate_query('generate daily sales report', null, 'openai');
-- Configuration: default_model = "gpt-4"
```

**Why**: Good balance of accuracy, speed, and cost for production workloads.

## Model-Specific Configurations

### OpenAI Configuration

```ini
[openai]
api_key = "sk-proj-your-api-key-here"
default_model = "gpt-4o"

# Optional: Model-specific settings (future feature)
# gpt_4o_temperature = 0.7
# gpt_4o_max_tokens = 16384
```

**API Key Format**: Must start with `sk-proj-` or `sk-`

**Getting Started**:
1. Visit [platform.openai.com](https://platform.openai.com)
2. Create an account and add billing information
3. Generate an API key
4. Add to configuration file

### Anthropic Configuration

```ini
[anthropic]
api_key = "sk-ant-your-api-key-here"
default_model = "claude-sonnet-4-5-20250929"

# Optional: Model-specific settings (future feature)
# claude_temperature = 0.7
# claude_max_tokens = 8192
```

**API Key Format**: Must start with `sk-ant-`

**Getting Started**:
1. Visit [console.anthropic.com](https://console.anthropic.com)
2. Create an account and add credits
3. Generate an API key
4. Add to configuration file

## Cost Considerations

### OpenAI Pricing (Approximate)

| Model | Input Cost | Output Cost | Typical Query Cost |
|-------|------------|-------------|-------------------|
| GPT-4o | $2.50/1M tokens | $10.00/1M tokens | $0.02-0.05 |
| GPT-4 | $30.00/1M tokens | $60.00/1M tokens | $0.10-0.20 |
| GPT-3.5-turbo | $0.50/1M tokens | $1.50/1M tokens | $0.001-0.005 |

### Anthropic Pricing (Approximate)

| Model | Input Cost | Output Cost | Typical Query Cost |
|-------|------------|-------------|-------------------|
| Claude 3.5 Sonnet | $3.00/1M tokens | $15.00/1M tokens | $0.03-0.06 |

*Prices are approximate and change frequently. Check provider websites for current pricing.*

### Cost Optimization Strategies

1. **Use Appropriate Models**
   ```sql
   -- For simple queries, use cheaper models
   SELECT generate_query('count users', null, 'openai');
   -- Set: default_model = "gpt-3.5-turbo"
   ```

2. **Efficient Query Descriptions**
   ```sql
   -- Be specific to reduce back-and-forth
   SELECT generate_query('show users with orders in last 30 days including total order value');
   ```

3. **Batch Similar Queries**
   ```sql
   -- Instead of multiple calls:
   -- SELECT generate_query('show users');
   -- SELECT generate_query('show user count');

   -- Use one comprehensive query:
   SELECT generate_query('show user list with total count');
   ```

## Provider Reliability and Error Handling

### Automatic Fallback

The extension includes automatic fallback logic:

```sql
-- If OpenAI fails, automatically tries Anthropic
SELECT generate_query('show data', null, 'auto');
```

### Manual Retry with Different Provider

```sql
-- If one provider fails, try another
SELECT generate_query('complex query', null, 'openai');
-- If that fails:
SELECT generate_query('complex query', null, 'anthropic');
```

### Error Handling Configuration

```ini
[general]
max_retries = 3           # Retry failed requests
request_timeout_ms = 30000 # Timeout for each request
```

## Advanced Provider Features

### Rate Limiting

Each provider has different rate limits:

**OpenAI**:
- GPT-3.5: 3,500 requests/minute
- GPT-4: 500 requests/minute (varies by tier)

**Anthropic**:
- Claude: 1,000 requests/minute (varies by tier)

### Context Window Optimization

For large schemas, consider context window sizes:

```sql
-- Large schema? Use models with larger context windows
-- GPT-4o: 128k tokens
-- Claude 3.5: 200k tokens
SELECT generate_query('analyze relationships across all tables', null, 'anthropic');
```

### Regional Considerations

**OpenAI**: Global availability with regional data centers
**Anthropic**: Primary US-based with expanding regional support

## Provider-Specific Best Practices

### OpenAI Best Practices

1. **Model Selection**:
   - Use GPT-3.5-turbo for development and testing
   - Use GPT-4 for production workloads
   - Use GPT-4o for complex analytical queries

2. **Prompt Engineering**:
   - Be specific about desired output format
   - Include relevant business context
   - Specify PostgreSQL version if using advanced features

### Anthropic Best Practices

1. **Leveraging Reasoning**:
   - Claude excels at understanding business logic
   - Good for queries requiring domain expertise
   - Effective for complex multi-step analysis

2. **Safety Features**:
   - Claude has built-in safety considerations
   - Good for production environments with compliance needs
   - Transparent about limitations and assumptions

## Custom API Endpoints

The extension supports custom API endpoints, allowing you to use OpenAI-compatible or Anthropic-compatible APIs. This is useful for:

- **Local LLMs**: Run models locally using Ollama, vLLM, or LocalAI
- **Privacy**: Keep all data on-premises without sending to external APIs
- **Cost optimization**: Use self-hosted models or alternative providers
- **Enterprise proxies**: Route requests through custom gateways

### Supported OpenAI-Compatible APIs

Any API that implements the OpenAI Chat Completions endpoint (`/v1/chat/completions`) can be used.

**Important:** The SDK automatically appends `/v1/chat/completions` to your base URL, so do **not** include `/v1` in your endpoint configuration.

| Provider | Correct `api_endpoint` | Notes |
|----------|------------------------|-------|
| **Ollama** | `http://localhost:11434` | Local model inference |
| **vLLM** | `http://localhost:8000` | High-performance inference |
| **LiteLLM** | `http://localhost:8000` | Unified API proxy |
| **OpenRouter** | `https://openrouter.ai/api` | Multiple model providers |
| **Together AI** | `https://api.together.xyz` | Hosted open-source models |
| **Azure OpenAI** | `https://YOUR_RESOURCE.openai.azure.com` | Microsoft Azure hosting |
| **LocalAI** | `http://localhost:8080` | Local model inference |

### Configuration Examples

#### Using Ollama (Local)

```ini
[openai]
api_key = "ollama"  # Ollama doesn't require a real key
api_endpoint = "http://localhost:11434"
default_model = "llama3.2"  # Or any model you have pulled
```

#### Using LiteLLM Proxy

```ini
[openai]
api_key = "your-litellm-key"
api_endpoint = "http://localhost:8000"
default_model = "gpt-4o"  # LiteLLM handles routing
```

#### Using OpenRouter

```ini
[openai]
api_key = "sk-or-v1-..."  # Your OpenRouter API key
api_endpoint = "https://openrouter.ai/api"
default_model = "openai/gpt-4o"  # Use provider/model format
```

### Important Considerations

1. **API Compatibility**: The custom endpoint must implement the same API format as the original provider
2. **Model Names**: Different providers may use different model naming conventions
3. **Authentication**: Some local servers don't require API keys; use a placeholder value
4. **Network Security**: Local endpoints typically use HTTP; ensure your network is secure
5. **Performance**: Local models may have different latency and quality characteristics

## Future Provider Support

The extension is designed to easily support additional providers:

- **Google (Gemini)**: Planned support
- **Azure OpenAI**: Possible future integration
- **Local Models**: Full support via custom endpoints (see above)

## Monitoring Provider Performance

### Enable Logging

```ini
[general]
enable_logging = true
log_level = "INFO"
```

### Track Performance

```sql
-- Monitor which provider is being used
-- Check logs for provider selection and response times
```

### Performance Metrics

Key metrics to monitor:
- **Response Time**: How long queries take to generate
- **Success Rate**: Percentage of successful generations
- **Cost**: Monthly API usage costs
- **Quality**: Accuracy of generated queries

Choose your AI provider based on your specific needs: cost sensitivity, query complexity, response time requirements, and accuracy needs. The extension's flexible provider system allows you to optimize for your particular use case.