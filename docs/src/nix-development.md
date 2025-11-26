# Nix Development Environment

This project provides a comprehensive Nix flake for reproducible development environments with all required tools pre-installed.

## What's Included

The Nix development environment includes:

### C++ Build Tools
- CMake 3.16+
- GNU Make
- Clang/LLVM toolchain with C++20 support
- clang-format for code formatting
- GDB and LLDB debuggers
- pkg-config

### PostgreSQL
- PostgreSQL 16 with development headers
- `pg_config` utility
- `psql` client
- `initdb` and `pg_ctl` for local database management

### Rust Toolchain
- Cargo (package manager)
- rustc (compiler)
- rust-analyzer (LSP server)
- rustfmt (code formatter)
- clippy (linter)

### Documentation Tools
- **mdbook** - Main documentation generator
- **mdbook-mermaid** - For creating diagrams
- **mdbook-linkcheck** - Validates links in documentation
- **mdbook-toc** - Generates table of contents
- **mdbook-admonish** - Adds callout blocks (notes, warnings, tips)
- **graphviz** - For generating diagrams
- **plantuml** - For UML diagrams

### Additional Utilities
- **ripgrep (rg)** - Fast code search
- **fd** - Fast file finder
- **jq** - JSON processing
- **yamllint** - YAML validation
- **markdownlint-cli** - Markdown linting
- **valgrind** (Linux only) - Memory debugging

## Getting Started

See the [NIX.md](../../NIX.md) file in the project root for installation instructions.

## Quick Commands

Once in the Nix environment (`nix develop`):

### Building Documentation

```bash
# Serve documentation locally (with live reload)
cd docs
mdbook serve

# Access at http://localhost:3000

# Build documentation to static files
mdbook build

# Check for broken links
mdbook test
```

### Building the Extension

```bash
# Configure and build
mkdir -p build && cd build
cmake ..
make

# Install to PostgreSQL
sudo make install
```

### Using Rust Tools

```bash
# Check Rust version
rustc --version
cargo --version

# If you add Rust components to the project:
cargo build
cargo test
cargo fmt
cargo clippy
```

### Code Quality

```bash
# Format C++ code
clang-format -i src/**/*.cpp src/**/*.h

# Lint YAML files
yamllint .github/workflows/

# Lint Markdown
markdownlint docs/src/*.md

# Search code
rg "query_generator" src/

# Find files
fd "\.cpp$" src/
```

## Environment Variables

The Nix shell sets up several environment variables:

- `PGDATA` - Points to `.pgdata` directory for local PostgreSQL
- `PGHOST` - Unix socket location
- `PGPORT` - PostgreSQL port (default: 5432)
- `PG_CONFIG` - Path to pg_config utility
- `CMAKE_EXPORT_COMPILE_COMMANDS` - Generates compile_commands.json for LSP
- `CARGO_HOME` - Cargo installation directory
- `RUSTUP_HOME` - Rustup installation directory
- `RUST_BACKTRACE` - Enabled for better error messages

## Local PostgreSQL Instance

The environment supports running a local PostgreSQL database:

```bash
# Initialize database
initdb -D .pgdata

# Start PostgreSQL
pg_ctl -D .pgdata -l logfile start

# Connect
psql -d postgres

# Create test database
createdb testdb

# Load extension
psql -d testdb -c "CREATE EXTENSION pg_ai_query;"

# Stop PostgreSQL
pg_ctl -D .pgdata stop
```

The `.pgdata` directory is gitignored and isolated to your project.

## IDE Integration

### VSCode

The environment works seamlessly with VSCode:

1. Install the C/C++ extension
2. The `compile_commands.json` is automatically generated
3. Rust-analyzer works out of the box

### Neovim/Vim

For clangd (C++) and rust-analyzer (Rust) LSP support:

1. The compile_commands.json is symlinked automatically
2. Both LSP servers are available in PATH

### Other IDEs

Any IDE that supports:
- clangd for C++
- rust-analyzer for Rust
- Will work with the generated compile_commands.json

## Documentation Workflow

### Writing Documentation

All documentation is in `docs/src/` as Markdown files:

```bash
cd docs

# Start live preview
mdbook serve

# Edit files in docs/src/
# Browser auto-refreshes on save
```

### Adding Diagrams

#### Mermaid Diagrams

```markdown
\`\`\`mermaid
graph TD
    A[User Query] --> B[Query Generator]
    B --> C[AI Provider]
    C --> D[SQL Query]
\`\`\`
```

#### Graphviz

```bash
# Create diagram
echo 'digraph G { A -> B; }' | dot -Tpng > diagram.png
```

### Using Admonitions

```markdown
\`\`\`admonish note
This is a helpful note for readers.
\`\`\`

\`\`\`admonish warning
Be careful with this operation!
\`\`\`

\`\`\`admonish tip
Here's a pro tip!
\`\`\`
```

### Checking Documentation

```bash
# Check for broken links
mdbook-linkcheck docs/

# Lint markdown
markdownlint docs/src/

# Build to verify
mdbook build docs/
```

## Development Shells

The flake provides two shells:

### Default Shell (Full)
```bash
nix develop
```
Includes all tools: C++, Rust, PostgreSQL, documentation, utilities.

### Minimal Shell
```bash
nix develop .#minimal
```
Only includes: CMake, Make, Clang, PostgreSQL, core libraries.

Use minimal for CI/CD or when you only need build essentials.

## Updating Dependencies

### Update All Dependencies
```bash
nix flake update
```

### Update Specific Input
```bash
nix flake lock --update-input nixpkgs
```

### Pin Different PostgreSQL Version

Edit `flake.nix`:
```nix
# Change from:
postgresql = pkgs.postgresql_16;

# To:
postgresql = pkgs.postgresql_15;
```

Then reload:
```bash
direnv reload  # if using direnv
# or
exit && nix develop
```

## CI/CD Usage

The Nix environment can be used in CI/CD:

```yaml
# GitHub Actions example
name: Build and Test

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive

      - uses: cachix/install-nix-action@v22
        with:
          extra_nix_config: |
            experimental-features = nix-command flakes

      - name: Build
        run: |
          nix develop --command bash -c "
            mkdir -p build && cd build
            cmake ..
            make
          "

      - name: Build Docs
        run: |
          nix develop --command bash -c "
            cd docs
            mdbook build
          "
```

## Troubleshooting

### Rust tools not found

Ensure the environment is properly loaded:
```bash
which cargo
rustc --version
```

If not found, reload the environment:
```bash
direnv reload  # or exit and re-enter nix develop
```

### mdbook plugins not working

Some mdbook plugins may need additional setup. The flake includes the most common ones, but you can add more:

Edit `flake.nix` and add to buildInputs:
```nix
mdbook-katex        # Math equations
mdbook-graphviz     # More diagram support
```

### Documentation build fails

Check for syntax errors:
```bash
cd docs
mdbook build 2>&1 | tee build.log
```

Common issues:
- Broken Markdown links
- Invalid YAML in front matter
- Malformed mermaid diagrams

### Slow first load

The first time entering the environment, Nix needs to download and build packages. Subsequent loads are instant (cached).

To pre-build:
```bash
nix develop --command echo "Ready"
```

## Additional Resources

- [mdbook Documentation](https://rust-lang.github.io/mdBook/)
- [Nix Flakes](https://nixos.wiki/wiki/Flakes)
