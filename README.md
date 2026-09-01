# Galactic Bloodshed

## What is GB?

GB stands for "Galactic Bloodshed", it is a piece of Internet history.

GB is one of the original update-based client-server multi-player 4X
Internet text games. Expand your empire and exterminate your opponents
as you explore the galaxy and exploit its many resources.

GB was first written in 1989; it presented an alternative to the very
complex game 'Empire'. Both are 4X games, however, GB appealed to sci-fi
fans. It never quite gained the popularity of Empire, however, those that
played GB were hugely fanatical about it. The game itself required less
daily time to play than Empire and dealt more with combat and less with
starving civilians. GB is one of the first true examples of Open Source
on the Internet; most admins that ran the game also contributed to the
code.

GB is a game played over the Internet by several users at various sites.
The remote site, where the game and its database reside, is generally
referred to as the "server". To connect to a game, players use a program
called a "client" that is run from their local computer.

(From the Galactic Bloodshed FAQ)

## Licensing

I received permission from the authors of GB on December 9, 2021 to relicense the game to apache2.

## 🚀 Getting Started

### ⚠️ First-time Setup (Required!)

After cloning the repository, run this command to install git hooks and developer aliases:

```bash
./tools/install-hooks.sh
```

This configures:
1. **Pre-commit hook**: Checks code formatting before each commit, preventing CI failures.
2. **`git clang-tidy` alias**: Configures `git clang-tidy` to run static analysis on modified files (`./tools/tidy-changed.sh`).

To configure the `git clang-tidy` alias globally across all repositories on your system:
```bash
git config --global alias.clang-tidy '!./tools/tidy-changed.sh'
```

### Building

```bash
cmake -S . -B build
cmake --build build
```

### Command-Line Utilities

All utilities support `-h` / `--help` to show usage information and `-d` / `--database` / `--db` (or `-D` for `makeuniv`) to specify an alternative database location (defaulting to `/var/lib/galactic-bloodshed/gb.db` in release builds or `build/gamedata/gb.db` in debug builds).

#### `GB` (Main Game Server)
Starts the Galactic Bloodshed server.
```bash
./build/gb/GB [options] [port] [update_time] [segments]
```
* `-p, --port <port>`: Port to listen on (default: `2010`).
* `-d, --database, --db <path>`: Path to SQLite database.
* `-h, --help`: Display help message and exit.

#### `makeuniv` (Universe Generator)
Initializes and populates a new game universe database.
```bash
./build/gb/makeuniv [options]
```
* `-D, --database, --db <path>`: Path to SQLite database.
* `-a`: Autoload star names from `star.list`.
* `-b`: Autoload planet names from `planet.list`.
* `-d`: Use all defaults and autoloaded names.
* `-e <E>`: Percentage of stars that have no planets.
* `-l <MIN>`: Minimum planets per system.
* `-m <MAX>`: Maximum planets per system.
* `-s <N>`: Total number of stars in the universe.
* `-v`: Print generated planet maps and info.
* `-w`: Print generated star info.
* `-h, --help`: Display help message and exit.

#### `enrol` (Race Enrollment CLI)
Interactive CLI tool to enroll new player races into an existing game database.
```bash
./build/gb/enrol [options]
```
* `-d, --database, --db <path>`: Path to SQLite database.
* `-h, --help`: Display help message and exit.

#### `racegen` (Race Generator)
Interactive race creation and point-budget design tool.
```bash
./build/gb/racegen [options]
```
* `-d, --database, --db <path>`: Path to SQLite database.
* `-h, --help`: Display help message and exit.


### Code Formatting & Static Analysis

#### Formatting with clang-format
The project uses `clang-format` for consistent code style.

* **Format changed files or diffs with Git:**
  ```bash
  git clang-format        # Formats staged changes
  git clang-format -f     # Formats unstaged working tree changes
  ```
* **Format all files via Ninja:**
  ```bash
  ninja -C build format       # Check formatting compliance
  ninja -C build format-fix   # Reformat all source files
  ```

#### Static Analysis with clang-tidy
* **Run static analysis on changed files:**
  ```bash
  git clang-tidy              # Checks changed files vs HEAD
  git clang-tidy -fix         # Automatically applies suggested fixes
  ninja -C build tidy-changed # Same check via build system
  ```
* **Exhaustive or full-repo checks:**
  ```bash
  git clang-tidy --full       # Checks changed files against .clang-tidy-full
  ninja -C build tidy         # Checks all files in repository
  ```

The pre-commit hook will automatically check formatting before commits. To bypass (not recommended):
```bash
git commit --no-verify
```

### Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for detailed contribution guidelines.