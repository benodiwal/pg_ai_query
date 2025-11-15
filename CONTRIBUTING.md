# Contributing to PostgreSQL AI Query Extension

We appreciate your interest in contributing to the PostgreSQL AI Query Extension! This document provides guidelines for contributing to the project.

## Table of Contents

- [Getting Started](#getting-started)
- [Development Environment](#development-environment)
- [Code Style and Standards](#code-style-and-standards)
- [Testing](#testing)
- [Submitting Changes](#submitting-changes)
- [Reporting Issues](#reporting-issues)
- [Feature Requests](#feature-requests)
- [Documentation](#documentation)
- [Community](#community)

## Getting Started

### Prerequisites

Before you start contributing, make sure you have:

- PostgreSQL 12+ with development headers
- CMake 3.16 or later
- C++20 compatible compiler (GCC 8+, Clang 10+, or MSVC 2019+)
- Git for version control
- API key from OpenAI or Anthropic for testing

### Setting Up Development Environment

1. **Fork the repository** on GitHub

2. **Clone your fork**:
   ```bash
   git clone https://github.com/your-username/pg_ai_query.git
   cd pg_ai_query
   ```

3. **Initialize submodules**:
   ```bash
   git submodule update --init --recursive
   ```

4. **Create a development branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```

5. **Set up build environment**:
   ```bash
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Debug ..
   make
   ```

## Development Environment

### Required Tools

- **C++ Compiler**: GCC 8+, Clang 10+, or MSVC 2019+
- **CMake**: Version 3.16 or later
- **PostgreSQL**: Development headers and libraries
- **Git**: For version control

### Development Dependencies

The project uses these key dependencies:

- **ai-sdk-cpp**: For AI provider integration (included as submodule)
- **nlohmann/json**: For JSON processing (included with ai-sdk-cpp)
- **OpenSSL**: For secure HTTP communications
- **PostgreSQL**: Extension development headers

### Build Configuration

For development, use Debug build:

```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_LOGGER_TEST=ON ..
```

For release builds:

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

## Code Style and Standards

### C++ Standards

- **Language Standard**: C++20
- **Header Style**: Use `#pragma once` for header guards
- **Naming Conventions**:
  - Classes: `PascalCase` (e.g., `QueryGenerator`)
  - Functions: `camelCase` (e.g., `generateQuery`)
  - Variables: `snake_case` (e.g., `api_key`)
  - Constants: `SCREAMING_SNAKE_CASE` (e.g., `MAX_RETRIES`)

### Code Formatting

The project uses clang-format with Chromium style:

```bash
# Format all source files
make format 
# OR
find src -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Check formatting
find src -name "*.cpp" -o -name "*.hpp" | xargs clang-format --dry-run -Werror
```

### PostgreSQL Integration

- **Include Order**: Always include `postgres.h` first in PostgreSQL extensions
- **Error Handling**: Use PostgreSQL's `ereport()` for error reporting
- **Memory Management**: Use PostgreSQL's memory contexts
- **Function Signatures**: Follow PostgreSQL's function signature conventions

### Documentation Standards

- **Header Comments**: Include brief descriptions for all public functions
- **Inline Comments**: Explain complex logic and business rules
- **API Documentation**: Document all public interfaces
- **Configuration**: Document all configuration options

## Testing

### Unit Tests

Run the test suite:

```bash
# Build tests
cmake -DBUILD_LOGGER_TEST=ON ..
make

# Run tests
./test_logger
```

### Integration Tests

Test with a real PostgreSQL instance:

```bash
# Install extension
sudo make install

# Connect to PostgreSQL and test
psql -d your_database -c "CREATE EXTENSION pg_ai_query;"
psql -d your_database -c "SELECT generate_query('show all tables');"
```

### Testing Guidelines

1. **Test Coverage**: Aim for good test coverage of new functionality
2. **Error Cases**: Test both success and failure scenarios
3. **Edge Cases**: Test boundary conditions and edge cases
4. **Performance**: Test performance with large datasets
5. **Security**: Test for SQL injection and other security issues

### Test Configuration

Create a test configuration file:

```ini
[general]
log_level = "DEBUG"
enable_logging = true

[response]
show_explanation = true
show_warnings = true
use_formatted_response = true

[openai]
api_key = "test-key-here"
default_model = "gpt-3.5-turbo"
```

## Submitting Changes

### Pull Request Process

1. **Ensure tests pass**:
   ```bash
   make && make test
   ```

2. **Update documentation** if needed

3. **Create descriptive commit messages**:
   ```
   feat: add response formatting configuration

   - Add support for JSON response format
   - Implement configurable explanation and warnings
   - Update documentation for new features
   ```

4. **Submit pull request**:
   - Use a clear, descriptive title
   - Include a detailed description of changes
   - Reference any related issues
   - Include screenshots for UI changes

### Commit Message Format

Use conventional commits format:

```
type(scope): description

[optional body]

[optional footer]
```

**Types**:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes
- `refactor`: Code refactoring
- `test`: Test additions or changes
- `chore`: Build process or auxiliary tool changes

### Code Review Process

All submissions require review:

1. **Automated Checks**: CI/CD pipeline runs tests and code quality checks
2. **Peer Review**: At least one maintainer reviews the code
3. **Testing**: Changes are tested in development environment
4. **Documentation**: Documentation is updated if necessary

## Reporting Issues

### Bug Reports

When reporting bugs, include:

1. **PostgreSQL version and OS**
2. **Extension version**
3. **Configuration file** (remove API keys)
4. **Steps to reproduce**
5. **Expected vs actual behavior**
6. **Error messages and logs**

### Issue Template

```markdown
## Bug Report

**Environment:**
- PostgreSQL version:
- OS:
- Extension version:

**Configuration:**
```ini
[response]
show_explanation = true
# ... (remove API keys)
```

**Steps to reproduce:**
1.
2.
3.

**Expected behavior:**

**Actual behavior:**

**Error messages:**
```

## Feature Requests

When requesting features:

1. **Describe the use case**
2. **Explain the expected behavior**
3. **Provide examples**
4. **Consider implementation complexity**
5. **Discuss potential alternatives**

### Feature Request Template

```markdown
## Feature Request

**Use case:**
Describe the scenario where this feature would be useful.

**Proposed solution:**
Describe how you envision the feature working.

**Example:**
```sql
SELECT generate_query('example usage');
```

**Alternatives:**
Other ways to solve the same problem.
```

## Documentation

### Types of Documentation

1. **Code Documentation**: Inline comments and header documentation
2. **User Documentation**: Installation, configuration, and usage guides
3. **Developer Documentation**: Architecture and contribution guidelines
4. **API Documentation**: Function and configuration reference

### Documentation Location

- **User Docs**: `docs/` directory (mdBook format)
- **Code Docs**: Inline comments in source files
- **README**: Project overview and quick start
- **Contributing**: This file

### Building Documentation

```bash
# Install mdBook
cargo install mdbook

# Build documentation
cd docs && mdbook build

# Serve locally
mdbook serve
```

## Community

### Communication Channels

- **GitHub Issues**: Bug reports and feature requests
- **GitHub Discussions**: Questions and community discussion
- **Pull Requests**: Code review and collaboration

### Code of Conduct

We are committed to providing a welcoming and inclusive environment. Please:

- Be respectful and constructive in discussions
- Focus on what is best for the community
- Show empathy towards other community members
- Be open to feedback and different perspectives

### Getting Help

If you need help:

1. **Documentation**: Check the [official documentation](https://benodiwal.github.io/pg_ai_query/)
2. **Issues**: Search existing issues for similar problems
3. **Discussions**: Start a discussion for questions
4. **Community**: Engage with other contributors

## Release Process

### Version Numbers

We use semantic versioning (SemVer):

- **MAJOR**: Incompatible API changes
- **MINOR**: Backward-compatible functionality additions
- **PATCH**: Backward-compatible bug fixes

### Release Checklist

- [ ] Update version numbers
- [ ] Update CHANGELOG.md
- [ ] Test with multiple PostgreSQL versions
- [ ] Update documentation
- [ ] Create release notes
- [ ] Tag release in Git
- [ ] Update package distributions

Thank you for contributing to the PostgreSQL AI Query Extension! Your help makes this project better for everyone.