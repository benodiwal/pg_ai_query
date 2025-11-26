{
  description = "PostgreSQL AI Query Extension - Natural language to SQL with AI";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        # PostgreSQL version to use
        postgresql = pkgs.postgresql_16;

        # Build the extension
        pg-ai-query = pkgs.stdenv.mkDerivation {
          pname = "pg_ai_query";
          version = "1.0.0";

          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            pkg-config
            postgresql
          ];

          buildInputs = with pkgs; [
            openssl
            zlib
            gettext
          ];

          # Initialize git submodules
          preConfigure = ''
            if [ -d .git ]; then
              git submodule update --init --recursive || true
            fi
          '';

          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Release"
            "-DPG_CONFIG=${postgresql}/bin/pg_config"
          ];

          installPhase = ''
            mkdir -p $out/lib
            mkdir -p $out/share/extension

            # Install the shared library
            if [ -f pg_ai_query.dylib ]; then
              cp pg_ai_query.dylib $out/lib/
            elif [ -f pg_ai_query.so ]; then
              cp pg_ai_query.so $out/lib/
            fi

            # Install SQL and control files
            cp ${./sql/pg_ai_query--1.0.sql} $out/share/extension/
            cp ${./pg_ai_query.control} $out/share/extension/
          '';

          meta = with pkgs.lib; {
            description = "PostgreSQL extension for generating SQL queries from natural language using AI";
            homepage = "https://github.com/benodiwal/pg_ai_query";
            license = licenses.mit;
            platforms = platforms.unix;
          };
        };

      in
      {
        packages = {
          default = pg-ai-query;
          pg-ai-query = pg-ai-query;
        };

        # Development shell
        devShells.default = pkgs.mkShell {
          name = "pg-ai-query-dev";

          buildInputs = with pkgs; [
            # Build tools
            cmake
            gnumake
            pkg-config
            git

            # Compiler toolchain
            clang
            clang-tools
            lldb

            # PostgreSQL
            postgresql

            # Libraries
            openssl
            zlib
            gettext

            # Development tools
            gdb

            # Rust toolchain (for mdbook and other tools)
            cargo
            rustc
            rust-analyzer
            rustfmt
            clippy

            # Documentation tools
            mdbook
            mdbook-mermaid       # Diagrams in mdbook
            mdbook-linkcheck     # Check for broken links
            mdbook-toc           # Table of contents
            mdbook-admonish      # Callout blocks (notes, warnings)

            # Additional documentation tools
            graphviz             # For generating diagrams
            plantuml             # UML diagrams

            # Formatting and linting
            clang-tools          # includes clang-format
            yamllint
            markdownlint-cli     # Markdown linting

            # Utility tools
            ripgrep              # Fast grep alternative
            fd                   # Fast find alternative
            jq                   # JSON processing
          ] ++ pkgs.lib.optionals pkgs.stdenv.isLinux [
            # Linux-only tools (valgrind doesn't work on macOS ARM)
            pkgs.valgrind
          ];

          shellHook = ''
            echo "PostgreSQL AI Query Extension Development Environment"
            echo "====================================================="
            echo ""
            echo "Tool Versions:"
            echo "  PostgreSQL: ${postgresql.version}"
            echo "  C++ Compiler: $(clang++ --version | head -n1)"
            echo "  CMake: $(cmake --version | head -n1)"
            echo "  Rust: $(rustc --version)"
            echo "  Cargo: $(cargo --version)"
            echo "  mdbook: $(mdbook --version)"
            echo ""
            echo "Available commands:"
            echo "  C++ Build:"
            echo "    - cmake, make, clang-format"
            echo "  PostgreSQL:"
            echo "    - pg_config, psql, initdb, pg_ctl"
            echo "  Rust/Cargo:"
            echo "    - cargo, rustc, rust-analyzer, rustfmt, clippy"
            echo "  Documentation:"
            echo "    - mdbook, mdbook-mermaid, mdbook-linkcheck"
            echo "  Utilities:"
            echo "    - rg (ripgrep), fd, jq, yamllint, markdownlint"
            echo ""
            echo "Quick start:"
            echo "  Extension: mkdir -p build && cd build && cmake .. && make"
            echo "  Docs: cd docs && mdbook serve"
            echo ""

            # Set up PostgreSQL environment
            export PGDATA="$PWD/.pgdata"
            export PGHOST="$PWD/.pgdata"
            export PGPORT=5432
            export PG_CONFIG="${postgresql}/bin/pg_config"

            # Add pg_config to PATH
            export PATH="${postgresql}/bin:$PATH"

            # Set up build environment
            export CMAKE_EXPORT_COMPILE_COMMANDS=1

            # For clangd LSP
            if [ ! -f compile_commands.json ] && [ -f build/compile_commands.json ]; then
              ln -sf build/compile_commands.json compile_commands.json
            fi

            # Rust environment
            export RUST_BACKTRACE=1
            export CARGO_HOME="$PWD/.cargo"
            export RUSTUP_HOME="$PWD/.rustup"

            echo "Environment variables set:"
            echo "  PGDATA=$PGDATA"
            echo "  PG_CONFIG=$PG_CONFIG"
            echo "  CARGO_HOME=$CARGO_HOME"
            echo ""
            echo "PostgreSQL Setup:"
            echo "  initdb -D .pgdata"
            echo "  pg_ctl -D .pgdata -l logfile start"
            echo ""
            echo "Documentation:"
            echo "  cd docs && mdbook serve"
            echo "  View at: http://localhost:3000"
            echo ""
          '';

          # Set C++ standard and other flags
          hardeningDisable = [ "fortify" ];

          # Additional environment variables
          CMAKE_BUILD_TYPE = "Debug";
          CMAKE_EXPORT_COMPILE_COMMANDS = "1";
        };

        # Alternative development shells
        devShells.minimal = pkgs.mkShell {
          name = "pg-ai-query-minimal";

          buildInputs = with pkgs; [
            cmake
            gnumake
            clang
            postgresql
            openssl
            zlib
            gettext
          ];

          shellHook = ''
            echo "Minimal PostgreSQL AI Query development environment"
            export PG_CONFIG="${postgresql}/bin/pg_config"
            export PATH="${postgresql}/bin:$PATH"
          '';
        };
      }
    );
}
