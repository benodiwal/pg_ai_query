# Nix Development Environment

This project supports Nix flakes for reproducible development environments.

## Prerequisites

1. Install Nix with flakes enabled:

```bash
# Install Nix (if not already installed)
curl -L https://nixos.org/nix/install | sh

# Enable flakes (add to ~/.config/nix/nix.conf or /etc/nix/nix.conf)
echo "experimental-features = nix-command flakes" >> ~/.config/nix/nix.conf
```

2. (Optional) Install direnv for automatic environment loading:

```bash
# On macOS
brew install direnv

# On Linux
# Use your package manager, e.g., apt, dnf, pacman

# Add to your shell rc file (~/.bashrc, ~/.zshrc, etc.)
eval "$(direnv hook bash)"  # or zsh, fish, etc.
```

## Getting Started

### Option 1: Using direnv (Recommended)

If you have direnv installed, the environment will automatically load when you enter the project directory:

```bash
cd pg_ai_query
direnv allow  # First time only
```

The environment will automatically activate and deactivate as you enter/leave the directory.

### Option 2: Manual Nix Shell

Enter the development environment manually:

```bash
# Full development environment
nix develop

# Minimal environment (fewer tools)
nix develop .#minimal
```

## What's Included

The Nix development environment provides:

### Build Tools
- CMake 3.x
- GNU Make
- Clang/LLVM toolchain with C++20 support
- pkg-config
- Git (for submodules)

### PostgreSQL
- PostgreSQL 16 with development headers
- `pg_config` utility
- `psql` client
- Database management tools (initdb, pg_ctl)

### Dependencies
- OpenSSL
- zlib
- gettext (libintl)

### Rust Toolchain
- Cargo (package manager)
- rustc (Rust compiler)
- rust-analyzer (LSP server)
- rustfmt (code formatter)
- clippy (linter)

### Documentation Tools
- mdbook (main documentation generator)
- mdbook-mermaid (diagrams)
- mdbook-linkcheck (link validation)
- mdbook-toc (table of contents)
- mdbook-admonish (callout blocks)
- graphviz, plantuml (diagram generation)

### Development Tools
- GDB debugger
- Valgrind (Linux only)
- clang-format for code formatting
- yamllint for YAML linting
- markdownlint-cli for Markdown linting

### Utilities
- ripgrep (rg) - Fast code search
- fd - Fast file finder
- jq - JSON processing

## Building the Project

Once in the Nix environment:

### Build Extension

```bash
# Initialize git submodules (if not already done)
git submodule update --init --recursive

# Create build directory
mkdir -p build && cd build

# Configure with CMake
cmake ..

# Build
make

# Install (optional - installs to PostgreSQL directory)
sudo make install
```

### Build Documentation

```bash
# Serve documentation with live reload
cd docs
mdbook serve
# Visit http://localhost:3000

# Build static documentation
mdbook build

# Check for broken links
mdbook test
```

## Building the Nix Package

You can also build the extension as a Nix package:

```bash
# Build the package
nix build

# The result will be in ./result/
ls -la result/

# Install the package to your system (NixOS/nix-darwin)
nix profile install .
```

## Local PostgreSQL Instance

The development environment sets up variables for a local PostgreSQL instance:

```bash
# Initialize a local PostgreSQL database (inside nix develop)
initdb -D .pgdata

# Start PostgreSQL
pg_ctl -D .pgdata -l logfile start

# Connect to the database
psql -d postgres

# Stop PostgreSQL when done
pg_ctl -D .pgdata stop
```

The local data directory (`.pgdata`) is gitignored.

## Environment Variables

The Nix shell automatically sets:

### PostgreSQL
- `PGDATA`: Points to `.pgdata` in project root
- `PGHOST`: Points to `.pgdata` (Unix socket)
- `PGPORT`: Set to 5432
- `PG_CONFIG`: Path to pg_config utility

### Build Environment
- `CMAKE_EXPORT_COMPILE_COMMANDS`: Generates compile_commands.json for LSP

### Rust/Cargo
- `CARGO_HOME`: Cargo installation directory (`.cargo`)
- `RUSTUP_HOME`: Rustup installation directory (`.rustup`)
- `RUST_BACKTRACE`: Enabled for better error messages

## IDE Integration

### VSCode

The environment is compatible with VSCode's C++ extension. The `compile_commands.json` file is automatically generated for IntelliSense.

### Neovim/Vim

For LSP support with clangd, the `compile_commands.json` is symlinked to the project root automatically.

## Troubleshooting

### Direnv not working

Make sure direnv is properly set up:

```bash
# Check if direnv is installed
which direnv

# Check if hook is in your shell rc
grep direnv ~/.zshrc  # or ~/.bashrc
```

### Submodule issues

If you get errors about missing submodules:

```bash
git submodule update --init --recursive
```

### PostgreSQL version mismatch

The flake uses PostgreSQL 16 by default. If you need a different version, edit `flake.nix`:

```nix
# Change this line
postgresql = pkgs.postgresql_16;

# To this (for PostgreSQL 15)
postgresql = pkgs.postgresql_15;
```

Then reload the environment:

```bash
direnv reload  # if using direnv
# or
exit && nix develop  # if using nix develop manually
```

### Building on macOS

On macOS, the extension builds as `.dylib` (for PostgreSQL 16+) or `.so` (older versions). The CMake configuration handles this automatically.

## Updating Dependencies

To update the Nix dependencies:

```bash
# Update flake inputs
nix flake update

# Or update specific input
nix flake lock --update-input nixpkgs
```

## CI/CD Integration

The flake can be used in CI/CD pipelines:

```yaml
# GitHub Actions example
- name: Install Nix
  uses: cachix/install-nix-action@v22
  with:
    extra_nix_config: |
      experimental-features = nix-command flakes

- name: Build
  run: nix build
```

## Additional Resources

- [Nix Manual](https://nixos.org/manual/nix/stable/)
- [Nix Flakes](https://nixos.wiki/wiki/Flakes)
- [direnv Documentation](https://direnv.net/)
