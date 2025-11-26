# Setting Up direnv for Automatic Environment Loading

## What is direnv?

direnv is a shell extension that automatically loads and unloads environment variables based on the current directory. For this project, it means the Nix development environment activates automatically when you enter the project directory.

## Installation

### macOS
```bash
brew install direnv
```

### Linux
```bash
# Ubuntu/Debian
sudo apt install direnv

# Fedora
sudo dnf install direnv

# Arch
sudo pacman -S direnv
```

## Shell Integration

Add the direnv hook to your shell configuration file:

### For Zsh (~/.zshrc)
```bash
eval "$(direnv hook zsh)"
```

### For Bash (~/.bashrc or ~/.bash_profile)
```bash
eval "$(direnv hook bash)"
```

### For Fish (~/.config/fish/config.fish)
```fish
direnv hook fish | source
```

After adding the hook, reload your shell:
```bash
source ~/.zshrc  # or ~/.bashrc
```

## First Time Setup

When you first enter the project directory, direnv will ask for permission:

```bash
cd pg_ai_query
# Output: direnv: error /Users/sachin/personal/pg_ai_query/.envrc is blocked. Run `direnv allow` to approve its content
```

Allow it to run:
```bash
direnv allow
```

## How It Works

Once set up, direnv will:

1. **Automatically activate** when you enter the directory:
   ```bash
   cd pg_ai_query
   # direnv: loading ~/pg_ai_query/.envrc
   # direnv: using flake
   # PostgreSQL AI Query Extension Development Environment
   # ===================================================
   # [environment info displayed]
   ```

2. **Automatically deactivate** when you leave:
   ```bash
   cd ..
   # direnv: unloading
   ```

3. **Reload on changes**:
   - When flake.nix is modified, direnv automatically reloads
   - You can manually reload with `direnv reload`

## Without direnv

If you don't want to use direnv, you can still use Nix manually:

```bash
cd pg_ai_query
nix develop
# or if flakes aren't enabled:
nix --extra-experimental-features "nix-command flakes" develop
```

## Verification

To verify direnv is working:

```bash
# Outside the project
which cmake
# Output: /usr/bin/cmake (system cmake)

cd pg_ai_query
# direnv loads...

which cmake
# Output: /nix/store/.../bin/cmake (Nix cmake)
```

## Troubleshooting

### direnv not activating

1. Check if hook is installed:
   ```bash
   type direnv
   # Should show: direnv is a shell function
   ```

2. Check if .envrc is allowed:
   ```bash
   direnv status
   ```

3. Manually reload:
   ```bash
   direnv allow
   direnv reload
   ```

### Slow activation

First load is slow as Nix builds the environment. Subsequent loads are fast:
- First time: ~10-30 seconds (building)
- After: <1 second (cached)

### Disabling temporarily

```bash
# Disable for current directory
direnv deny

# Re-enable
direnv allow
```

## .envrc File Content

The `.envrc` file in this project contains:
```bash
use flake
```

This tells direnv to load the Nix flake's development shell. You can customize it with additional environment variables if needed:

```bash
use flake

# Custom environment variables
export DEBUG=1
export DATABASE_URL=postgresql://localhost/mydb
```

## Security Note

direnv requires explicit approval (.envrc files could contain malicious code). Always review `.envrc` files before running `direnv allow`, especially in projects from untrusted sources.

For this project, the `.envrc` is safe - it only loads the Nix flake.
