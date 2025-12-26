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

After cloning the repository, run this command to install git hooks:

```bash
./tools/install-hooks.sh
```

This installs a pre-commit hook that checks code formatting before each commit, preventing CI failures.

### Building

```bash
cmake -S . -B build
cmake --build build
```

### Code Formatting

The project uses clang-format for consistent code style. 

**Check formatting:**
```bash
ninja -C build format
```

**Fix formatting issues:**
```bash
ninja -C build format-fix
```

The pre-commit hook will automatically check formatting before commits. To bypass (not recommended):
```bash
git commit --no-verify
```

### Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for detailed contribution guidelines.