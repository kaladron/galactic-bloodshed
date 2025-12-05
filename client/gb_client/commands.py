# SPDX-License-Identifier: Apache-2.0
"""
Command processing for Galactic Bloodshed client

This module handles:
- Command parsing and dispatch
- Built-in client commands
- Macro expansion and execution
- Variable substitution
"""

import logging
from collections import deque
from dataclasses import dataclass, field
from enum import Enum
from typing import Callable, Dict, List, TYPE_CHECKING

if TYPE_CHECKING:
    from .client import GBClient


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
            name="clienthelp",
            handler=self._cmd_help,
            cmd_type=CommandType.CLIENT,
            aliases=["chelp", "?"],
            help_text="Show client help information"
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
        
        self.register_command(Command(
            name="showmap",
            handler=self._cmd_showmap,
            cmd_type=CommandType.CLIENT,
            aliases=["sm"],
            help_text="Show the current parsed map"
        ))
        
        self.register_command(Command(
            name="showorbit",
            handler=self._cmd_showorbit,
            cmd_type=CommandType.CLIENT,
            aliases=["so"],
            help_text="Show the current parsed orbit/system map"
        ))
        
        self.register_command(Command(
            name="history",
            handler=self._cmd_history,
            cmd_type=CommandType.CLIENT,
            aliases=["hist"],
            help_text="Show command history"
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
        
        # Expand history commands first (before variables)
        line = self._expand_history(line)
        
        # Expand variables
        line = self._expand_variables(line)
        
        # Split on semicolon for multiple commands
        commands = [cmd.strip() for cmd in line.split(';')]
        
        for cmd in commands:
            await self._execute_single_command(cmd)
    
    def _expand_history(self, line: str) -> str:
        """Expand history commands (!! and !n)"""
        import re
        
        # Get history from UI
        history = self.client.ui.get_history()
        if not history:
            logging.debug("No history available for expansion")
            return line
        
        # Handle !! (last command)
        if line.strip() == '!!':
            expanded = history[-1]
            logging.debug(f"Expanded !! to: {expanded!r}")
            return expanded
        
        # Handle !n (command number n)
        # Match !<number> at the start of the line or after whitespace
        match = re.match(r'^!(\d+)(?:\s|$)', line)
        if match:
            cmd_num = int(match.group(1))
            if 1 <= cmd_num <= len(history):
                # Replace !n with the command
                remaining = line[match.end():]
                expanded = history[cmd_num - 1] + (' ' + remaining if remaining else '')
                logging.debug(f"Expanded !{cmd_num} to: {expanded!r}")
                return expanded
            else:
                self.client.display_output(f"Error: No such command in history: !{cmd_num}")
                logging.debug(f"!{cmd_num} out of range (history has {len(history)} commands)")
                return ""  # Return empty to skip execution
        
        return line
    
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
        logging.info("Quit command received, setting running=False")
        self.client.running = False
        self.client.display_output("Exiting...")
    
    async def _cmd_help(self, args: str):
        """Show help"""
        self.client.display_output("\nAvailable client commands:")
        for name, cmd in self.commands.items():
            if name == cmd.name:  # Only show primary name, not aliases
                self.client.display_output(f"  {name}: {cmd.help_text}")
        
        self.client.display_output("\nHistory features:")
        self.client.display_output("  UP/DOWN arrows: Navigate command history")
        self.client.display_output("  !!: Repeat last command")
        self.client.display_output("  !n: Repeat command number n (use 'history' to see numbers)")
    
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
    
    async def _cmd_showmap(self, args: str):
        """Display the currently parsed map"""
        if not self.client.state.current_map:
            self.client.display_output("No map data available. Use the 'map' command first.")
            return
        
        # Use the UI's map display method for proper inverse video
        self.client.ui.display_map(self.client.state.current_map)
    
    async def _cmd_showorbit(self, args: str):
        """Display the currently parsed orbit map"""
        if not self.client.state.current_orbit_map:
            self.client.display_output("No orbit map data available. Use the 'orbit' command first.")
            return
        
        from .protocol import OrbitMapParser
        formatted = OrbitMapParser.format_orbit_display(self.client.state.current_orbit_map)
        self.client.display_output(formatted)
    
    async def _cmd_history(self, args: str):
        """Display command history"""
        history = self.client.ui.get_history()
        if not history:
            self.client.display_output("No command history yet.")
            return
        
        # Parse optional argument for how many commands to show
        try:
            count = int(args) if args.strip() else len(history)
        except ValueError:
            count = len(history)
        
        # Show last 'count' commands
        start_idx = max(0, len(history) - count)
        self.client.display_output("\nCommand History:")
        for i, cmd in enumerate(history[start_idx:], start=start_idx + 1):
            self.client.display_output(f"  {i:3d}  {cmd}")
        self.client.display_output(f"\nTotal: {len(history)} commands (use !<number> to repeat)")

