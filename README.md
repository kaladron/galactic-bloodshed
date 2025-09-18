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

## 🛠️ Development

This project uses **C++26 with C++ Modules** and requires **CMake 4.0+** and **LLVM/Clang 19** with libc++.

### Quick Start Options

**🐳 Development Container (Recommended)**
```bash
# Open in VS Code with Dev Containers extension
# The container includes all required tools pre-installed
```

**🚀 Automated Setup Script**
```bash
# For Ubuntu/Debian systems
./setup-dev-env.sh
```

**📖 Manual Setup**
See [AGENTS.md](AGENTS.md) for detailed development instructions.

### Building
```bash
mkdir -p build && cd build
cmake ..
cmake --build .
```

### For GitHub Copilot Agent
This repository includes a custom development environment configuration in `.devcontainer/` that provides the exact tool versions needed. See [.devcontainer/README.md](.devcontainer/README.md) for details.