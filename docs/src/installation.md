# Installation

This guide will walk you through installing the PostgreSQL AI Query extension on your system.

## Prerequisites

Before installing `pg_ai_query`, ensure you have the following:

### System Requirements
- **PostgreSQL 14+**: The extension requires PostgreSQL version 14 or later
- **Operating System**: Linux, macOS, or Windows (with proper development tools)
- **C++ Compiler**: GCC 8+ or Clang 10+ with C++20 support
- **CMake**: Version 3.15 or later for building

### Development Dependencies
- PostgreSQL development headers (`postgresql-devel` on RHEL/CentOS, `postgresql-server-dev-all` on Ubuntu/Debian)
- OpenSSL development libraries
- Git (for cloning the repository)

### AI Provider Requirements
You'll need API access to at least one of the following:
- **OpenAI API**: Create an account at [platform.openai.com](https://platform.openai.com)
- **Anthropic API**: Create an account at [console.anthropic.com](https://console.anthropic.com)

## Installation Methods

### Method 1: Build from Source (Recommended)

#### 1. Clone the Repository

```bash
git clone https://github.com/benodiwal/pg_ai_query.git
cd pg_ai_query
```

#### 2. Initialize Submodules

```bash
git submodule update --init --recursive
```

#### 3. Build the Extension

```bash
make clean
make
```

#### 4. Install the Extension

```bash
# Install to default PostgreSQL location
sudo make install

# Or specify a custom PostgreSQL installation
PG_CONFIG=/path/to/pg_config make install
```

### Method 2: Package Installation (Future)

*Package installations will be available in future releases for major Linux distributions and package managers.*

## Platform-Specific Instructions

### Ubuntu/Debian

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install postgresql-server-dev-all cmake gcc g++ libssl-dev git

# Clone and build
git clone https://github.com/benodiwal/pg_ai_query.git
cd pg_ai_query
git submodule update --init --recursive
make && sudo make install
```

### CentOS/RHEL/Fedora

```bash
# Install dependencies
sudo yum install postgresql-devel cmake gcc-c++ openssl-devel git
# OR on newer systems:
sudo dnf install postgresql-devel cmake gcc-c++ openssl-devel git

# Clone and build
git clone https://github.com/benodiwal/pg_ai_query.git
cd pg_ai_query
git submodule update --init --recursive
make && sudo make install
```

### macOS

```bash
# Install dependencies (with Homebrew)
brew install postgresql cmake openssl git

# Clone and build
git clone https://github.com/benodiwal/pg_ai_query.git
cd pg_ai_query
git submodule update --init --recursive
make && sudo make install
```

### Windows

For Windows installation, you'll need:
- Visual Studio 2019 or later with C++ tools
- PostgreSQL installed from EDB or compiled from source
- CMake for Windows

*Detailed Windows installation instructions will be added in a future update.*

## Verification

After installation, verify that the extension is properly installed:

### 1. Connect to PostgreSQL

Connect to your default database (usually `postgres`) or a specific database where you want to install the extension:

```bash
psql -d postgres
```

### 2. Create the Extension

```sql
CREATE EXTENSION IF NOT EXISTS pg_ai_query;
```

### 3. Test the Installation

```sql
-- Check if functions are available
\\df generate_query

-- Test with a simple query (will fail without API key, which is expected)
SELECT generate_query('show me all tables');
```

You should see the function listed and get an error about missing API configuration (which is normal - we'll configure that next).

## Troubleshooting Installation

### Common Issues

#### PostgreSQL Development Headers Missing

**Error**: `postgres.h: No such file or directory`

**Solution**: Install PostgreSQL development packages:
```bash
# Ubuntu/Debian
sudo apt-get install postgresql-server-dev-all

# CentOS/RHEL
sudo yum install postgresql-devel
```

#### CMake Not Found

**Error**: `cmake: command not found`

**Solution**: Install CMake:
```bash
# Ubuntu/Debian
sudo apt-get install cmake

# macOS
brew install cmake

# CentOS/RHEL
sudo yum install cmake
```

#### Compilation Errors

**Error**: Various C++ compilation errors

**Solutions**:
- Ensure you have a C++20-compatible compiler
- Check that all submodules are initialized: `git submodule update --init --recursive`
- Clean and rebuild: `make clean && make`

#### Permission Denied During Installation

**Error**: Permission denied when running `make install`

**Solution**: Use `sudo` for installation:
```bash
sudo make install
```

#### Wrong PostgreSQL Installation

**Error**: Extension installs to wrong PostgreSQL version

**Solution**: Specify the correct `pg_config`:
```bash
PG_CONFIG=/usr/pgsql-14/bin/pg_config make install
```

### Getting Help

If you encounter issues not covered here:

1. Check the [Troubleshooting](./troubleshooting.md) guide
2. Review the [FAQ](./faq.md) for common questions
3. Open an issue on the [GitHub repository](https://github.com/benodiwal/pg_ai_query/issues)

## Next Steps

Once installation is complete:

1. [Configure the extension](./configuration.md) with your API keys
2. Follow the [Quick Start Guide](./quick-start.md) to generate your first queries
3. Explore [Usage Examples](./examples.md) for inspiration