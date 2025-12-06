# AI Agent Guide for Galactic Bloodshed Python Client

## 🎯 Overview

This directory contains the Python client (`gb-client.py`) for connecting to the Galactic Bloodshed game server. The client provides a terminal-based interface using curses for split-screen output/input display.

## 🔧 Key Technologies

- **Python 3.12+** with asyncio for concurrent I/O
- **curses library** for terminal UI (split screen with output/input)
- **Client-Server Protocol (CSP)** - messages prefixed with `|`

## 📋 Phase 1 Complete (December 2025)

Phase 1 implementation achieved:
- ✅ Curses-based terminal UI with split screen
- ✅ Character-by-character input with editing (backspace, arrows, etc.)
- ✅ Async I/O with proper timeout handling
- ✅ Graceful quit/exit functionality
- ✅ ~780 lines of working Python code

## 🚀 Running the Client

### Start the Server (Terminal 1)
```bash
cd /workspaces/galactic-bloodshed/build
ninja run-gb-debug
```

### Run the Client (Terminal 2)
```bash
cd /workspaces/galactic-bloodshed/client
./gb-client.py localhost 2010
```

### Optional: Run without curses for debugging
```bash
./gb-client.py localhost 2010 --no-curses
```

## 🔑 Critical Asyncio Patterns

### 1. Use `asyncio.wait()` not `gather()` for concurrent loops

**❌ Wrong:**
```python
# gather() waits for ALL tasks to complete
await asyncio.gather(input_loop(), receive_loop())
```

**✅ Correct:**
```python
# wait(FIRST_COMPLETED) exits when ANY task completes
done, pending = await asyncio.wait(
    [input_task, receive_task],
    return_when=asyncio.FIRST_COMPLETED
)
```

**Why:** Input and receive loops run indefinitely. Using `gather()` means the program never exits because it waits for ALL tasks. Using `wait(FIRST_COMPLETED)` allows proper exit handling.

### 2. Always add timeouts to receive loops

**❌ Wrong:**
```python
# Hangs indefinitely, can't check exit flags
data = await reader.read(4096)
```

**✅ Correct:**
```python
try:
    data = await asyncio.wait_for(reader.read(4096), timeout=0.5)
except asyncio.TimeoutError:
    # Check exit flag every 0.5s
    if should_exit:
        break
```

**Why:** Without timeouts, the receive loop blocks forever waiting for data. With timeouts, we can periodically check exit flags and respond to quit/exit commands.

### 3. Let `curses.wrapper()` handle cleanup

**❌ Wrong:**
```python
stdscr = curses.initscr()
try:
    # ... curses code ...
finally:
    curses.endwin()  # Can cause "endwin() returned ERR"
```

**✅ Correct:**
```python
def main(stdscr):
    # ... curses code ...
    # No need to call endwin()

curses.wrapper(main)  # Handles all cleanup automatically
```

**Why:** `curses.wrapper()` sets up the terminal correctly and ensures proper cleanup even if exceptions occur. Calling `endwin()` manually can cause terminal state issues.

## 📝 Code Structure

### Main Components

- **`gb-client.py`** - Main executable script
- **`gb_client/`** - Python package directory (if modularized)

### Key Functions

#### Connection Management
```python
async def connect_to_server(host: str, port: int):
    """Establish TCP connection to game server"""
    reader, writer = await asyncio.open_connection(host, port)
    return reader, writer
```

#### Input Loop
```python
async def input_loop(stdscr, writer, exit_flag):
    """Handle character-by-character input with editing support"""
    # - Capture keystrokes
    # - Handle backspace, arrows, enter
    # - Send completed lines to server
    # - Check exit_flag periodically
```

#### Receive Loop
```python
async def receive_loop(stdscr, reader, exit_flag):
    """Receive and display server messages"""
    # - Read from server with timeout
    # - Parse CSP messages (| prefix)
    # - Update output window
    # - Check exit_flag periodically
```

## 🎨 UI Layout

```
┌─────────────────────────────────────┐
│                                     │
│         OUTPUT WINDOW               │
│      (server messages)              │
│                                     │
│                                     │
├─────────────────────────────────────┤
│ > input text here_                  │
└─────────────────────────────────────┘
```

## 🐛 Debugging Tips

### Run without curses
For debugging, use the `--no-curses` flag to see raw output:
```bash
./gb-client.py localhost 2010 --no-curses
```

### Check server connectivity
```bash
# Test if server is running
telnet localhost 2010
```

### View client logs
```bash
# If logging is implemented
tail -f gb-client.log
```

## 🔮 Future Enhancements (Phase 2+)

Potential features for future phases:
- [ ] Command history (up/down arrows)
- [ ] Tab completion for game commands
- [ ] Color-coded output (ANSI colors)
- [ ] Multiple window layouts
- [ ] CSP message parsing and special handling
- [ ] Client-side command aliases
- [ ] Configuration file support
- [ ] TLS/SSL connection support
- [ ] Reconnection handling
- [ ] Session persistence

## 📚 Related Documentation

- [Game Server AGENTS.md](../AGENTS.md) - Server-side development guide
- [Client-Server Protocol](../docs/) - CSP message format documentation
- [Python asyncio docs](https://docs.python.org/3/library/asyncio.html)
- [Python curses docs](https://docs.python.org/3/library/curses.html)

---

**Remember**: The client is a thin terminal interface. Complex game logic stays on the server. The client just sends commands and displays results.
