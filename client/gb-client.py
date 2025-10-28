#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""
Galactic Bloodshed II Client - Python Implementation

A modern Python reimplementation of the classic gbII MUD-style game client
for Galactic Bloodshed servers.

Original C client: gbII (circa 1990-1993)
Python port: 2025
"""

import argparse
import asyncio
import curses
import hashlib
import logging
import re
import socket
import sys
from collections import defaultdict, deque
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
from typing import Any, Callable, Dict, List, Optional, Tuple


__version__ = "0.1.0"


# ============================================================================
# Data Structures
# ============================================================================

class ScopeLevel(Enum):
    """Scope levels in the game hierarchy"""
    UNIVERSE = 0
    STAR = 1
    PLANET = 2
    SHIP = 3


@dataclass
class Scope:
    """Current scope in the game"""
    level: ScopeLevel = ScopeLevel.UNIVERSE
    star: str = ""
    planet: str = ""
    ship: str = ""
    aps: int = 0  # Action points


@dataclass
class Profile:
    """Player profile information"""
    race_id: int = 0
    gov_id: int = 0
    race_name: str = ""
    gov_name: str = ""
    password: str = ""
    scope: Scope = field(default_factory=Scope)


@dataclass
class Sector:
    """Planet sector data"""
    x: int = 0
    y: int = 0
    owner: int = 0
    type: str = "."
    population: int = 0
    troops: int = 0
    resources: int = 0
    efficiency: int = 0


@dataclass
class GameState:
    """Overall game state"""
    profile: Profile = field(default_factory=Profile)
    connected: bool = False
    server_host: str = ""
    server_port: int = 0
    variables: Dict[str, str] = field(default_factory=dict)
    last_ship: str = ""
    last_lot: str = ""


# ============================================================================
# Network Communication
# ============================================================================

class CSPProtocol:
    """Client-Server Protocol handler
    
    Handles messages starting with '|' which are protocol messages
    versus regular game output.
    """
    
    CSP_PREFIX = "|"
    
    @staticmethod
    def is_csp_message(line: str) -> bool:
        """Check if line is a CSP protocol message"""
        return line.startswith(CSPProtocol.CSP_PREFIX)
    
    @staticmethod
    def parse_csp(line: str) -> Tuple[str, List[str]]:
        """Parse CSP message into command and arguments
        
        Format: |command arg1 arg2 ...
        Returns: (command, [args])
        """
        if not CSPProtocol.is_csp_message(line):
            return "", []
        
        parts = line[1:].strip().split()
        command = parts[0] if parts else ""
        args = parts[1:] if len(parts) > 1 else []
        return command, args


class NetworkBuffer:
    """Buffer for handling partial lines from network"""
    
    def __init__(self):
        self.buffer = ""
        self.lines: deque[str] = deque()
    
    def add_data(self, data: str) -> List[str]:
        """Add data to buffer and return complete lines"""
        self.buffer += data
        
        # Split on newlines but keep incomplete last line
        parts = self.buffer.split('\n')
        self.buffer = parts[-1]  # Keep incomplete line
        
        # Add complete lines to queue
        complete_lines = parts[:-1]
        self.lines.extend(complete_lines)
        
        return complete_lines
    
    def get_line(self) -> Optional[str]:
        """Get next complete line from buffer"""
        return self.lines.popleft() if self.lines else None
    
    def has_lines(self) -> bool:
        """Check if buffer has complete lines"""
        return len(self.lines) > 0


class GameConnection:
    """Manages connection to game server"""
    
    def __init__(self, host: str, port: int):
        self.host = host
        self.port = port
        self.socket: Optional[socket.socket] = None
        self.reader: Optional[asyncio.StreamReader] = None
        self.writer: Optional[asyncio.StreamWriter] = None
        self.buffer = NetworkBuffer()
    
    async def connect(self) -> bool:
        """Establish connection to game server"""
        try:
            self.reader, self.writer = await asyncio.open_connection(
                self.host, self.port
            )
            logging.info(f"Connected to {self.host}:{self.port}")
            return True
        except Exception as e:
            logging.error(f"Connection failed: {e}")
            return False
    
    async def send(self, message: str):
        """Send message to server"""
        if not self.writer:
            return
        
        if not message.endswith('\n'):
            message += '\n'
        
        self.writer.write(message.encode('utf-8'))
        await self.writer.drain()
    
    async def receive(self) -> Optional[str]:
        """Receive data from server"""
        if not self.reader:
            return None
        
        try:
            data = await self.reader.read(4096)
            if not data:
                return None
            
            text = data.decode('utf-8', errors='replace')
            return text
        except Exception as e:
            logging.error(f"Receive error: {e}")
            return None
    
    def close(self):
        """Close connection"""
        if self.writer:
            self.writer.close()


# ============================================================================
# Command Processing
# ============================================================================

class CommandType(Enum):
    """Types of commands"""
    CLIENT = "client"      # Processed locally
    SERVER = "server"      # Sent to game server
    MACRO = "macro"        # User-defined macro


@dataclass
class Command:
    """Command definition"""
    name: str
    handler: Callable
    cmd_type: CommandType
    aliases: List[str] = field(default_factory=list)
    help_text: str = ""


class CommandProcessor:
    """Handles command parsing and execution"""
    
    def __init__(self, client: 'GBClient'):
        self.client = client
        self.commands: Dict[str, Command] = {}
        self.macros: Dict[str, str] = {}
        self.command_queue: deque[str] = deque()
        
        self._register_builtin_commands()
    
    def _register_builtin_commands(self):
        """Register built-in client commands"""
        self.register_command(Command(
            name="quit",
            handler=self._cmd_quit,
            cmd_type=CommandType.CLIENT,
            aliases=["exit", "q"],
            help_text="Exit the client"
        ))
        
        self.register_command(Command(
            name="help",
            handler=self._cmd_help,
            cmd_type=CommandType.CLIENT,
            aliases=["?"],
            help_text="Show help information"
        ))
        
        self.register_command(Command(
            name="set",
            handler=self._cmd_set,
            cmd_type=CommandType.CLIENT,
            help_text="Set a variable: set <name> <value>"
        ))
        
        self.register_command(Command(
            name="macro",
            handler=self._cmd_macro,
            cmd_type=CommandType.CLIENT,
            help_text="Define a macro: macro <name> <commands>"
        ))
    
    def register_command(self, command: Command):
        """Register a command and its aliases"""
        self.commands[command.name] = command
        for alias in command.aliases:
            self.commands[alias] = command
    
    async def process_command(self, line: str):
        """Process a command line"""
        if not line.strip():
            return
        
        # Expand variables
        line = self._expand_variables(line)
        
        # Split on semicolon for multiple commands
        commands = [cmd.strip() for cmd in line.split(';')]
        
        for cmd in commands:
            await self._execute_single_command(cmd)
    
    def _expand_variables(self, line: str) -> str:
        """Expand $variable references"""
        # Special variables
        replacements = {
            '$b': self.client.state.last_ship,
            '$l': self.client.state.last_lot,
        }
        
        # User variables
        for var_name, var_value in self.client.state.variables.items():
            replacements[f'${var_name}'] = var_value
        
        for pattern, value in replacements.items():
            line = line.replace(pattern, value)
        
        return line
    
    async def _execute_single_command(self, cmd: str):
        """Execute a single command"""
        if not cmd:
            return
        
        parts = cmd.split(None, 1)
        cmd_name = parts[0].lower()
        args = parts[1] if len(parts) > 1 else ""
        
        # Check if it's a client command
        if cmd_name in self.commands:
            command = self.commands[cmd_name]
            await command.handler(args)
            return
        
        # Check if it's a macro
        if cmd_name in self.macros:
            macro_text = self.macros[cmd_name]
            await self.process_command(macro_text)
            return
        
        # Otherwise, send to server
        await self.client.connection.send(cmd)
    
    async def _cmd_quit(self, args: str):
        """Quit the client"""
        self.client.running = False
    
    async def _cmd_help(self, args: str):
        """Show help"""
        self.client.display_output("\nAvailable client commands:")
        for name, cmd in self.commands.items():
            if name == cmd.name:  # Only show primary name, not aliases
                self.client.display_output(f"  {name}: {cmd.help_text}")
    
    async def _cmd_set(self, args: str):
        """Set a variable"""
        parts = args.split(None, 1)
        if len(parts) < 2:
            self.client.display_output("Usage: set <name> <value>")
            return
        
        var_name, var_value = parts
        self.client.state.variables[var_name] = var_value
        self.client.display_output(f"Set ${var_name} = {var_value}")
    
    async def _cmd_macro(self, args: str):
        """Define a macro"""
        parts = args.split(None, 1)
        if len(parts) < 2:
            self.client.display_output("Usage: macro <name> <commands>")
            return
        
        macro_name, macro_text = parts
        self.macros[macro_name] = macro_text
        self.client.display_output(f"Macro '{macro_name}' defined")


# ============================================================================
# Main Client
# ============================================================================

class GBClient:
    """Main Galactic Bloodshed client"""
    
    def __init__(self, host: str, port: int):
        self.connection = GameConnection(host, port)
        self.state = GameState()
        self.state.server_host = host
        self.state.server_port = port
        self.command_processor = CommandProcessor(self)
        self.running = False
        self.output_buffer: deque[str] = deque(maxlen=10000)
    
    def display_output(self, text: str):
        """Display output to user"""
        print(text)
        self.output_buffer.append(text)
    
    async def process_server_message(self, line: str):
        """Process a line received from server"""
        # Check if it's a CSP message
        if CSPProtocol.is_csp_message(line):
            command, args = CSPProtocol.parse_csp(line)
            await self.handle_csp_command(command, args)
        else:
            # Regular game output
            self.display_output(line)
    
    async def handle_csp_command(self, command: str, args: List[str]):
        """Handle CSP protocol command"""
        logging.debug(f"CSP: {command} {args}")
        
        # Handle common CSP commands
        if command == "PROMPT":
            # Server is ready for input
            pass
        elif command == "SCOPE":
            # Scope change notification
            if args:
                self.update_scope(args)
        elif command == "APS":
            # Action points update
            if args:
                self.state.profile.scope.aps = int(args[0])
    
    def update_scope(self, scope_parts: List[str]):
        """Update current scope from server"""
        # Parse scope string, e.g., ["STAR", "Sol"]
        if not scope_parts:
            return
        
        level_str = scope_parts[0].upper()
        if level_str == "UNIVERSE":
            self.state.profile.scope.level = ScopeLevel.UNIVERSE
        elif level_str == "STAR":
            self.state.profile.scope.level = ScopeLevel.STAR
            self.state.profile.scope.star = scope_parts[1] if len(scope_parts) > 1 else ""
        elif level_str == "PLANET":
            self.state.profile.scope.level = ScopeLevel.PLANET
            self.state.profile.scope.planet = scope_parts[1] if len(scope_parts) > 1 else ""
    
    async def input_loop(self):
        """Handle user input"""
        loop = asyncio.get_event_loop()
        
        while self.running:
            try:
                # Read input asynchronously
                line = await loop.run_in_executor(None, input, "> ")
                await self.command_processor.process_command(line)
            except EOFError:
                break
            except Exception as e:
                logging.error(f"Input error: {e}")
    
    async def receive_loop(self):
        """Handle receiving data from server"""
        while self.running:
            data = await self.connection.receive()
            if data is None:
                self.display_output("Connection closed by server")
                self.running = False
                break
            
            # Add to buffer and process complete lines
            lines = self.connection.buffer.add_data(data)
            for line in lines:
                await self.process_server_message(line)
    
    async def run(self):
        """Main client loop"""
        self.display_output(f"Galactic Bloodshed II Client v{__version__}")
        self.display_output(f"Connecting to {self.connection.host}:{self.connection.port}...")
        
        if not await self.connection.connect():
            self.display_output("Connection failed")
            return
        
        self.state.connected = True
        self.running = True
        
        # Run input and receive loops concurrently
        try:
            await asyncio.gather(
                self.input_loop(),
                self.receive_loop()
            )
        except KeyboardInterrupt:
            self.display_output("\nInterrupted")
        finally:
            self.connection.close()
            self.display_output("Disconnected")


# ============================================================================
# Main Entry Point
# ============================================================================

def setup_logging(verbose: bool = False):
    """Setup logging configuration"""
    level = logging.DEBUG if verbose else logging.INFO
    logging.basicConfig(
        level=level,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        filename='gb-client.log'
    )


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description="Galactic Bloodshed II Client"
    )
    parser.add_argument(
        'host',
        help='Game server hostname'
    )
    parser.add_argument(
        'port',
        type=int,
        help='Game server port'
    )
    parser.add_argument(
        '-v', '--verbose',
        action='store_true',
        help='Enable verbose logging'
    )
    parser.add_argument(
        '--version',
        action='version',
        version=f'%(prog)s {__version__}'
    )
    
    args = parser.parse_args()
    
    setup_logging(args.verbose)
    
    client = GBClient(args.host, args.port)
    
    try:
        asyncio.run(client.run())
    except Exception as e:
        logging.exception("Fatal error")
        print(f"Error: {e}", file=sys.stderr)
        return 1
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
