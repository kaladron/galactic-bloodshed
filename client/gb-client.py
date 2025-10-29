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
    inverse: bool = False  # Inverse video display flag


@dataclass
class PlanetMap:
    """Parsed planet map data"""
    planet_name: str = ""
    width: int = 0
    height: int = 0
    show: int = 1
    sectors: List[List[Sector]] = field(default_factory=list)
    
    def get_sector(self, x: int, y: int) -> Optional[Sector]:
        """Get sector at coordinates"""
        if 0 <= y < len(self.sectors) and 0 <= x < len(self.sectors[y]):
            return self.sectors[y][x]
        return None


@dataclass
class OrbitObject:
    """Object in an orbit map (star, planet, or ship)"""
    x: int = 0
    y: int = 0
    symbol: str = "?"
    name: str = ""
    explored: int = 0  # 0 or 1 (or player number in color mode)
    inhabited: int = 0  # 0 or 1 (or player number in color mode)
    obj_type: str = "unknown"  # "star", "planet", "ship"


@dataclass
class OrbitMap:
    """Parsed orbit/system map data"""
    objects: List[OrbitObject] = field(default_factory=list)
    
    def get_stars(self) -> List[OrbitObject]:
        """Get all star objects"""
        return [obj for obj in self.objects if obj.obj_type == "star"]
    
    def get_planets(self) -> List[OrbitObject]:
        """Get all planet objects"""
        return [obj for obj in self.objects if obj.obj_type == "planet"]
    
    def get_ships(self) -> List[OrbitObject]:
        """Get all ship objects"""
        return [obj for obj in self.objects if obj.obj_type == "ship"]


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
    current_map: Optional[PlanetMap] = None
    current_orbit_map: Optional[OrbitMap] = None


# ============================================================================
# Terminal UI
# ============================================================================

class TerminalUI:
    """Basic curses-based terminal UI
    
    Provides a split screen with:
    - Main output area (scrolling)
    - Input line at bottom
    """
    
    def __init__(self, use_curses: bool = True):
        self.use_curses = use_curses
        self.stdscr = None
        self.output_win = None
        self.status_win = None
        self.input_win = None
        self.output_buffer: deque = deque(maxlen=10000)
        self.input_buffer: str = ""
        self.input_pos: int = 0
        self.height: int = 0
        self.width: int = 0
        self.status_text: str = ""
        self.last_displayed_map: Optional['PlanetMap'] = None  # Track last map for redraw
        self.last_displayed_orbit: Optional['OrbitMap'] = None  # Track last orbit map for redraw
        
    def init(self, stdscr=None):
        """Initialize the terminal UI"""
        if not self.use_curses:
            return
            
        self.stdscr = stdscr
        if stdscr:
            curses.curs_set(1)  # Show cursor
            self.height, self.width = stdscr.getmaxyx()
            
            # Create output window (all but last 2 lines)
            # The status bar will replace the separator line
            self.output_win = curses.newwin(self.height - 2, self.width, 0, 0)
            self.output_win.scrollok(True)
            self.output_win.idlok(True)
            
            # Create status window (same line as the old separator, line height-2)
            self.status_win = curses.newwin(1, self.width, self.height - 2, 0)
            
            # Create input window (last line only, for prompt)
            self.input_win = curses.newwin(1, self.width, self.height - 1, 0)
            
            self.refresh_status()
            self.refresh_input()
    
    def handle_resize(self):
        """Handle terminal window resize event
        
        This is called when curses.KEY_RESIZE is detected.
        Recreates windows with new dimensions.
        """
        if not self.use_curses or not self.stdscr:
            return
        
        try:
            # Get new terminal dimensions
            self.height, self.width = self.stdscr.getmaxyx()
            
            # Clear and refresh the main screen
            self.stdscr.clear()
            self.stdscr.refresh()
            
            # Recreate windows with new dimensions
            self.output_win = curses.newwin(self.height - 2, self.width, 0, 0)
            self.output_win.scrollok(True)
            self.output_win.idlok(True)
            
            self.status_win = curses.newwin(1, self.width, self.height - 2, 0)
            self.input_win = curses.newwin(1, self.width, self.height - 1, 0)
            
            # Redraw the status bar and input line
            self.refresh_status()
            self.refresh_input()
            
            # If a planet map was displayed, redraw it instead of text buffer
            if self.last_displayed_map:
                try:
                    # Redraw the planet sector map
                    self._draw_map_internal(self.last_displayed_map, show_coords=True)
                    self.output_win.refresh()
                except Exception as e:
                    logging.error(f"Error redrawing map on resize: {e}")
                    # Fall back to text buffer if map redraw fails
                    self.last_displayed_map = None
            # If an orbit map was displayed, redraw it
            elif self.last_displayed_orbit:
                try:
                    # Redraw the orbit map
                    formatted = OrbitMapParser.format_orbit_display(self.last_displayed_orbit, 
                                                                     width=self.width, 
                                                                     height=self.height - 5)
                    for line in formatted.split('\n'):
                        self.output_win.addstr(line + "\n")
                    self.output_win.refresh()
                except Exception as e:
                    logging.error(f"Error redrawing orbit map on resize: {e}")
                    # Fall back to text buffer if orbit redraw fails
                    self.last_displayed_orbit = None
            
            # If no special map, redraw recent output (last 10 lines from buffer)
            if not self.last_displayed_map and not self.last_displayed_orbit:
                recent_lines = list(self.output_buffer)[-min(10, self.height - 3):]
                for line in recent_lines:
                    try:
                        self.output_win.addstr(line + "\n")
                    except curses.error:
                        pass
                self.output_win.refresh()
            
            logging.info(f"Terminal resized to {self.width}x{self.height}")
        except Exception as e:
            logging.error(f"Error handling resize: {e}")
    
    def cleanup(self):
        """Cleanup terminal state"""
        if self.use_curses and self.stdscr:
            try:
                # Just reset cursor visibility, don't call endwin()
                # The curses.wrapper() will handle endwin() for us
                curses.curs_set(1)
            except:
                pass  # Ignore errors during cleanup
            # Mark as cleaned up so we don't try to use curses anymore
            self.stdscr = None
            self.output_win = None
            self.status_win = None
            self.input_win = None
    
    def display(self, text: str):
        """Display text in output area"""
        # Check for null characters and log them for debugging
        if '\0' in text:
            # Log the problematic text with context
            logging.warning(f"Received text with null character(s): {text!r}")
            # Also log hex representation for clarity
            hex_repr = ' '.join(f'{ord(c):02x}' for c in text[:100])  # First 100 chars
            logging.warning(f"Hex representation (first 100 chars): {hex_repr}")
        
        # Sanitize text: remove null characters and other control chars that curses can't handle
        sanitized = text.replace('\0', '').replace('\r', '')
        
        # Strip ANSI escape codes if using curses (curses doesn't interpret them)
        if self.use_curses:
            # Remove ANSI escape sequences like \033[7m and \033[0m
            import re
            sanitized = re.sub(r'\033\[[0-9;]*m', '', sanitized)
        
        self.output_buffer.append(sanitized)
        
        # Note: We don't clear last_displayed_map here because text output
        # often follows a map display (planet stats, etc.)
        # Maps are cleared when a new map is displayed or when we detect non-map content
        
        if not self.use_curses or not self.output_win:
            # Fallback to simple print
            print(sanitized)
            return
        
        try:
            # Add to output window
            self.output_win.addstr(sanitized + "\n")
            self.output_win.refresh()
            self.refresh_input()
        except (curses.error, ValueError) as e:
            # Ignore curses errors (e.g., writing to last line)
            # Also catch ValueError for any remaining problematic characters
            logging.debug(f"Display error (ignored): {e}")
            pass
    
    def display_map(self, planet_map: 'PlanetMap', show_coords: bool = True):
        """Display a planet map with proper inverse video support
        
        This uses curses attributes directly instead of ANSI codes
        """
        # Track the last displayed map for redrawing on resize
        self.last_displayed_map = planet_map
        
        if not self.use_curses or not self.output_win:
            # Fallback to text display with ANSI codes
            formatted = MapParser.format_map_display(planet_map, show_coords, use_ansi=True)
            self.display(formatted)
            return
        
        try:
            # Header
            header = f"=== {planet_map.planet_name} ({planet_map.width}x{planet_map.height}) ==="
            self.output_win.addstr(header + "\n")
            
            # Column headers if requested
            if show_coords:
                coord_header = "   " + "".join(str(x % 10) for x in range(planet_map.width))
                self.output_win.addstr(coord_header + "\n")
            
            # Display each row
            for y, row in enumerate(planet_map.sectors):
                # Row label
                if show_coords:
                    self.output_win.addstr(f"{y:2d} ")
                
                # Display sectors with proper attributes
                for sector in row:
                    attr = curses.A_REVERSE if sector.inverse else curses.A_NORMAL
                    self.output_win.addstr(sector.type, attr)
                
                self.output_win.addstr("\n")
            
            self.output_win.refresh()
            self.refresh_input()
        except curses.error as e:
            logging.debug(f"Map display error (ignored): {e}")
            pass
    
    def _draw_map_internal(self, planet_map: 'PlanetMap', show_coords: bool = True):
        """Internal method to draw a map without tracking it
        
        Used by handle_resize to avoid overwriting last_displayed_map.
        """
        if not self.use_curses or not self.output_win:
            return
        
        try:
            # Header
            header = f"=== {planet_map.planet_name} ({planet_map.width}x{planet_map.height}) ==="
            self.output_win.addstr(header + "\n")
            
            # Column headers if requested
            if show_coords:
                coord_header = "   " + "".join(str(x % 10) for x in range(planet_map.width))
                self.output_win.addstr(coord_header + "\n")
            
            # Display each row
            for y, row in enumerate(planet_map.sectors):
                # Row label
                if show_coords:
                    self.output_win.addstr(f"{y:2d} ")
                
                # Display sectors with proper attributes
                for sector in row:
                    attr = curses.A_REVERSE if sector.inverse else curses.A_NORMAL
                    self.output_win.addstr(sector.type, attr)
                
                self.output_win.addstr("\n")
        except curses.error as e:
            logging.debug(f"Internal map draw error (ignored): {e}")
            pass
    
    def refresh_input(self):
        """Refresh the input line display"""
        if not self.use_curses or not self.input_win:
            return
        
        try:
            self.input_win.clear()
            prompt = "> "
            self.input_win.addstr(0, 0, prompt + self.input_buffer)
            # Position cursor at input position
            cursor_x = len(prompt) + self.input_pos
            if cursor_x < self.width:
                self.input_win.move(0, cursor_x)
            self.input_win.refresh()
        except curses.error:
            pass
    
    def refresh_status(self):
        """Refresh the status bar display"""
        if not self.use_curses or not self.status_win:
            return
        
        try:
            self.status_win.clear()
            # Use reverse video for status bar
            attr = curses.A_REVERSE
            # Pad or truncate status text to fit width
            status_display = self.status_text[:self.width-1].ljust(self.width-1)
            self.status_win.addstr(0, 0, status_display, attr)
            self.status_win.refresh()
        except curses.error:
            pass
    
    def update_status(self, profile: 'Profile'):
        """Update status bar with current game state"""
        # Build status string: scope path | APs: N
        scope = profile.scope
        
        # Check if logged in (have race_id or race_name)
        logged_in = profile.race_id > 0 or profile.race_name
        
        if not logged_in:
            # Not logged in yet
            self.status_text = "Not logged in"
            self.refresh_status()
            return
        
        # Format scope part - just show the path like the prompt
        if scope.level == ScopeLevel.UNIVERSE:
            scope_str = "/"
        elif scope.level == ScopeLevel.STAR:
            scope_str = f"/{scope.star}" if scope.star else "/"
        elif scope.level == ScopeLevel.PLANET:
            scope_str = f"/{scope.star}/{scope.planet}" if scope.star and scope.planet else "/"
        elif scope.level == ScopeLevel.SHIP:
            # Show full path with ship
            if scope.planet:
                scope_str = f"/{scope.star}/{scope.planet}/{scope.ship}"
            elif scope.star:
                scope_str = f"/{scope.star}/{scope.ship}"
            else:
                scope_str = f"/{scope.ship}"
        else:
            scope_str = "/"
        
        # Format APs
        aps_str = f"APs: {scope.aps}"
        
        # Combine into status string (no race/gov info needed)
        self.status_text = f"{scope_str} | {aps_str}"
        self.refresh_status()
    
    def get_input_char(self) -> Optional[int]:
        """Get a single character from input (non-blocking)"""
        if not self.use_curses or not self.input_win:
            return None
        
        try:
            self.input_win.nodelay(True)
            ch = self.input_win.getch()
            self.input_win.nodelay(False)
            return ch if ch != -1 else None
        except curses.error:
            return None
    
    def handle_input_char(self, ch: int) -> Optional[str]:
        """Handle input character, return complete line if Enter pressed"""
        if ch == curses.KEY_RESIZE:  # Terminal resize
            self.handle_resize()
            return None
        elif ch == ord('\n'):  # Enter
            line = self.input_buffer
            self.input_buffer = ""
            self.input_pos = 0
            self.refresh_input()
            return line
        elif ch == curses.KEY_BACKSPACE or ch == 127 or ch == 8:  # Backspace
            if self.input_pos > 0:
                self.input_buffer = (
                    self.input_buffer[:self.input_pos-1] + 
                    self.input_buffer[self.input_pos:]
                )
                self.input_pos -= 1
                self.refresh_input()
        elif ch == curses.KEY_DC:  # Delete
            if self.input_pos < len(self.input_buffer):
                self.input_buffer = (
                    self.input_buffer[:self.input_pos] + 
                    self.input_buffer[self.input_pos+1:]
                )
                self.refresh_input()
        elif ch == curses.KEY_LEFT:
            if self.input_pos > 0:
                self.input_pos -= 1
                self.refresh_input()
        elif ch == curses.KEY_RIGHT:
            if self.input_pos < len(self.input_buffer):
                self.input_pos += 1
                self.refresh_input()
        elif ch == curses.KEY_HOME or ch == 1:  # Home or Ctrl-A
            self.input_pos = 0
            self.refresh_input()
        elif ch == curses.KEY_END or ch == 5:  # End or Ctrl-E
            self.input_pos = len(self.input_buffer)
            self.refresh_input()
        elif 32 <= ch <= 126:  # Printable character
            self.input_buffer = (
                self.input_buffer[:self.input_pos] + 
                chr(ch) + 
                self.input_buffer[self.input_pos:]
            )
            self.input_pos += 1
            self.refresh_input()
        
        return None


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


class MapParser:
    """Parser for planet map data from server
    
    Map format from server:
    $PlanetName;width;height;show;sector_data
    
    Each sector is 3 chars:
    - First char: display flag ('0'=normal, '1'=inverse) OR color code (owner + '?')
    - Second char: owner code (owner + '?')
    - Third char: sector type or ship letter
    
    Example:
    $Radha;15;5;1;0?*0?.0?.0?...
    where 0?* = normal display, owner='?'-'?'=0, terrain='*'
    """
    
    # Sector type characters (from gb/map.cc desshow function)
    CHAR_WASTED = '%'
    CHAR_SEA = '.'
    CHAR_LAND = '-'
    CHAR_MOUNT = '^'
    CHAR_GAS = '~'
    CHAR_PLATED = '#'
    CHAR_ICE = '*'
    CHAR_DESERT = '"'
    CHAR_FOREST = ')'
    CHAR_CRYSTAL = 'x'
    CHAR_MY_TROOPS = 'X'
    CHAR_ALLIED_TROOPS = 'A'
    CHAR_ATWAR_TROOPS = 'E'
    CHAR_NEUTRAL_TROOPS = 'N'
    
    @staticmethod
    def is_map_line(line: str) -> bool:
        """Check if line is a map data line"""
        return line.startswith('$')
    
    @staticmethod
    def parse_map(line: str) -> Optional[PlanetMap]:
        """Parse a map line into PlanetMap structure
        
        Args:
            line: Map data line starting with '$'
            
        Returns:
            PlanetMap object or None if parsing fails
        """
        if not MapParser.is_map_line(line):
            return None
        
        try:
            # Strip leading $ and split by semicolons
            parts = line[1:].split(';')
            if len(parts) < 5:
                logging.warning(f"Map line has too few parts: {len(parts)}")
                return None
            
            planet_name = parts[0]
            width = int(parts[1])
            height = int(parts[2])
            show = int(parts[3])
            sector_data = parts[4] if len(parts) > 4 else ""
            
            logging.info(f"Parsing map for {planet_name}: {width}x{height}, show={show}")
            
            # Create the map structure
            planet_map = PlanetMap(
                planet_name=planet_name,
                width=width,
                height=height,
                show=show,
                sectors=[]
            )
            
            # Parse sector data - each sector is 3 characters
            # Data is row-major: y=0 all x, then y=1 all x, etc.
            idx = 0
            for y in range(height):
                row = []
                for x in range(width):
                    if idx + 2 >= len(sector_data):
                        # Not enough data, fill with empty sector
                        row.append(Sector(x=x, y=y, owner=0, type='?'))
                        continue
                    
                    # Get the three-character sector code
                    flag_char = sector_data[idx]
                    owner_char = sector_data[idx + 1]
                    terrain_char = sector_data[idx + 2]
                    idx += 3
                    
                    # Parse first character - display flag
                    # '0' = normal, '1' = inverse, or color code
                    inverse = False
                    if flag_char == '1':
                        inverse = True
                    # Note: if flag_char > '?', it's a color code, but we don't use it yet
                    
                    # Parse second character - owner encoded as char - '?'
                    owner = ord(owner_char) - ord('?')
                    
                    # Third character is the sector type or ship letter
                    sector_type = terrain_char
                    
                    # Create sector
                    sector = Sector(
                        x=x,
                        y=y,
                        owner=owner,
                        type=sector_type,
                        inverse=inverse
                    )
                    row.append(sector)
                
                planet_map.sectors.append(row)
            
            logging.info(f"Successfully parsed map: {width}x{height} = {len(planet_map.sectors[0]) if planet_map.sectors else 0}x{len(planet_map.sectors)}")
            return planet_map
            
        except Exception as e:
            logging.error(f"Error parsing map line: {e}", exc_info=True)
            logging.error(f"Map line was: {line[:100]}...")
            return None
    
    @staticmethod
    def format_map_display(planet_map: PlanetMap, show_coords: bool = True, use_ansi: bool = True) -> str:
        """Format map for display in terminal
        
        Args:
            planet_map: The parsed map data
            show_coords: Whether to show coordinate labels
            use_ansi: Whether to use ANSI codes for inverse video
            
        Returns:
            Formatted string ready for display
        """
        lines = []
        
        # Header with planet name
        lines.append(f"=== {planet_map.planet_name} ({planet_map.width}x{planet_map.height}) ===")
        
        if show_coords:
            # X coordinate header
            header = "   "
            for x in range(planet_map.width):
                header += str(x % 10)
            lines.append(header)
        
        # ANSI codes for inverse video
        INVERSE_START = "\033[7m" if use_ansi else ""
        INVERSE_END = "\033[0m" if use_ansi else ""
        
        # Map rows
        for y, row in enumerate(planet_map.sectors):
            if show_coords:
                line = f"{y:2d} "
            else:
                line = ""
            
            # Track whether we're currently in inverse mode to minimize escapes
            in_inverse = False
            
            for sector in row:
                if use_ansi:
                    # Use ANSI escape codes for inverse video
                    if sector.inverse and not in_inverse:
                        line += INVERSE_START
                        in_inverse = True
                    elif not sector.inverse and in_inverse:
                        line += INVERSE_END
                        in_inverse = False
                    line += sector.type
                else:
                    # Fallback: show inverse sectors in brackets
                    if sector.inverse:
                        line += f"[{sector.type}]"
                    else:
                        line += sector.type
            
            # Close inverse if we ended in that mode
            if in_inverse and use_ansi:
                line += INVERSE_END
            
            lines.append(line)
        
        return "\n".join(lines)


class OrbitMapParser:
    """Parser for orbit/system map data from server
    
    Map format from server (starts with '#'):
    #explored x y 0 symbol inhabited Name;explored x y 0 symbol inhabited Name;...
    
    Each object (star/planet/ship) is semicolon-separated:
    - explored: 0/1 (or player+'?' for color mode) - whether player has explored it
    - x, y: coordinates (0-200 scale, 100=center)
    - 0: unused field
    - symbol: '*' for star, planet type char for planets, ship letter for ships
    - inhabited: 0/1 (or player+'?') - whether player inhabits/owns it
    - Name: object name
    
    Example:
    #1 129 84 0 * 1 Hadar;0 141 102 0 ? 0 Yang;1 100 100 0 @ 1 Radha;
    """
    
    @staticmethod
    def is_orbit_line(line: str) -> bool:
        """Check if line is an orbit map data line"""
        return line.startswith('#') and ' ' in line
    
    @staticmethod
    def parse_orbit_map(line: str) -> Optional[OrbitMap]:
        """Parse an orbit map line into OrbitMap structure
        
        Args:
            line: Orbit data line starting with '#'
            
        Returns:
            OrbitMap object or None if parsing fails
        """
        if not OrbitMapParser.is_orbit_line(line):
            return None
        
        try:
            # Strip leading # and split by semicolons
            line = line[1:].strip()
            if not line:
                return None
            
            orbit_map = OrbitMap(objects=[])
            
            # Split into object entries
            entries = [e.strip() for e in line.split(';') if e.strip()]
            
            logging.info(f"Parsing orbit map with {len(entries)} objects")
            
            for entry in entries:
                # Parse each entry: explored x y 0 symbol inhabited Name
                parts = entry.split(None, 6)  # Split on whitespace, max 7 parts
                if len(parts) < 7:
                    logging.warning(f"Orbit entry has too few parts: {len(parts)} - {entry}")
                    continue
                
                explored_str = parts[0]
                x = int(parts[1])
                y = int(parts[2])
                # parts[3] is always 0, skip it
                symbol = parts[4]
                inhabited_str = parts[5]
                name = parts[6] if len(parts) > 6 else ""
                
                # Parse explored status (0/1 or player+'?')
                if explored_str.isdigit():
                    explored = int(explored_str)
                else:
                    # Color mode: char - '?'
                    explored = ord(explored_str) - ord('?')
                
                # Parse inhabited status
                if inhabited_str.isdigit():
                    inhabited = int(inhabited_str)
                else:
                    # Color mode: char - '?'
                    inhabited = ord(inhabited_str) - ord('?')
                
                # Determine object type from symbol
                if symbol == '*':
                    obj_type = "star"
                elif symbol in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ':
                    # Ship letters (assuming uppercase = ships)
                    obj_type = "ship"
                else:
                    # Planet symbols: @, ?, etc.
                    obj_type = "planet"
                
                orbit_obj = OrbitObject(
                    x=x,
                    y=y,
                    symbol=symbol,
                    name=name,
                    explored=explored,
                    inhabited=inhabited,
                    obj_type=obj_type
                )
                orbit_map.objects.append(orbit_obj)
            
            logging.info(f"Successfully parsed orbit map with {len(orbit_map.objects)} objects")
            return orbit_map
            
        except Exception as e:
            logging.error(f"Error parsing orbit map line: {e}", exc_info=True)
            logging.error(f"Orbit line was: {line[:100]}...")
            return None
    
    @staticmethod
    def format_orbit_display(orbit_map: OrbitMap, width: int = 60, height: int = 20) -> str:
        """Format orbit map for display in terminal
        
        Args:
            orbit_map: The parsed orbit map data
            width: Display width in characters
            height: Display height in characters
            
        Returns:
            Formatted string ready for display
        """
        lines = []
        
        # Create a 2D grid
        grid = [[' ' for _ in range(width)] for _ in range(height)]
        labels = []
        
        # Scale from 0-200 coordinate space to display grid
        scale_x = (width - 1) / 200.0
        scale_y = (height - 1) / 200.0
        
        # Place objects on grid
        for obj in orbit_map.objects:
            display_x = int(obj.x * scale_x)
            display_y = int(obj.y * scale_y)
            
            # Clamp to grid bounds
            display_x = max(0, min(width - 1, display_x))
            display_y = max(0, min(height - 1, display_y))
            
            # Place symbol on grid
            grid[display_y][display_x] = obj.symbol
            
            # Add label for named objects
            if obj.name:
                labels.append(f"  {obj.symbol} = {obj.name}" +
                            (f" (inhabited)" if obj.inhabited else ""))
        
        # Header
        lines.append("=== Orbit Map ===")
        
        # Display grid
        for row in grid:
            lines.append(''.join(row))
        
        # Legend
        if labels:
            lines.append("")
            lines.append("Objects:")
            lines.extend(labels)
        
        return "\n".join(lines)


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
        logging.info("Quit command received, setting running=False")
        self.client.running = False
        self.client.display_output("Exiting...")
    
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
        
        formatted = OrbitMapParser.format_orbit_display(self.client.state.current_orbit_map)
        self.client.display_output(formatted)


# ============================================================================
# Main Client
# ============================================================================

class GBClient:
    """Main Galactic Bloodshed client"""
    
    def __init__(self, host: str, port: int, use_curses: bool = True):
        self.connection = GameConnection(host, port)
        self.state = GameState()
        self.state.server_host = host
        self.state.server_port = port
        self.command_processor = CommandProcessor(self)
        self.running = False
        self.ui = TerminalUI(use_curses=use_curses)
    
    def display_output(self, text: str):
        """Display output to user"""
        self.ui.display(text)
    
    async def process_server_message(self, line: str):
        """Process a line received from server"""
        # Check if it's a planet sector map line (starts with $)
        if MapParser.is_map_line(line):
            planet_map = MapParser.parse_map(line)
            if planet_map:
                self.state.current_map = planet_map
                logging.info(f"Parsed map for {planet_map.planet_name}: {planet_map.width}x{planet_map.height}")
                # Display map using UI method for proper inverse video
                self.ui.display_map(planet_map)
            return  # Don't display the raw map line
        
        # Check if it's an orbit/system map line (starts with #)
        if OrbitMapParser.is_orbit_line(line):
            orbit_map = OrbitMapParser.parse_orbit_map(line)
            if orbit_map:
                self.state.current_orbit_map = orbit_map
                logging.info(f"Parsed orbit map with {len(orbit_map.objects)} objects")
                # Display orbit map and track it for redraw
                formatted = OrbitMapParser.format_orbit_display(orbit_map)
                self.ui.last_displayed_orbit = orbit_map
                self.display_output(formatted)
            return  # Don't display the raw orbit line
        
        # Check if it's a CSP message
        if CSPProtocol.is_csp_message(line):
            command, args = CSPProtocol.parse_csp(line)
            await self.handle_csp_command(command, args)
        else:
            # Log lines that might contain race/gov info for debugging
            if 'logged on' in line.lower() or ('"' in line and '[' in line):
                logging.info(f"LOGIN INFO: {line!r}")
            
            # Check for scope prompt line to extract APs and scope
            # Format: ( [APs] /scope/path )
            # Examples:
            #   ( [0] / )                  - Universe, 0 APs
            #   ( [5] /Hadar )             - Star Hadar, 5 APs
            #   ( [3] /Hadar/Radha )       - Planet Radha in Hadar, 3 APs
            #   ( [2] /Hadar/Radha/#123 )  - Ship #123 at planet, 2 APs
            prompt_match = re.match(r'\s*\(\s*\[(\d+)\]\s*/([^)]*)\s*\)', line)
            if prompt_match:
                # Clear any displayed maps when we see a new prompt - indicates end of output
                self.ui.last_displayed_map = None
                self.ui.last_displayed_orbit = None
                
                # Extract APs
                aps = int(prompt_match.group(1))
                self.state.profile.scope.aps = aps
                
                # Extract scope from the path
                path = prompt_match.group(2).strip()
                if not path:
                    # Just "/" means universe level
                    self.state.profile.scope.level = ScopeLevel.UNIVERSE
                    self.state.profile.scope.star = ""
                    self.state.profile.scope.planet = ""
                    self.state.profile.scope.ship = ""
                else:
                    # Parse path: StarName or StarName/PlanetName or with #ShipNum
                    parts = path.split('/')
                    if len(parts) == 1:
                        if parts[0].startswith('#'):
                            # Ship at universe level: /#123
                            self.state.profile.scope.level = ScopeLevel.SHIP
                            self.state.profile.scope.star = ""
                            self.state.profile.scope.planet = ""
                            self.state.profile.scope.ship = parts[0]
                        else:
                            # Star level: /Hadar
                            self.state.profile.scope.level = ScopeLevel.STAR
                            self.state.profile.scope.star = parts[0]
                            self.state.profile.scope.planet = ""
                            self.state.profile.scope.ship = ""
                    elif len(parts) == 2:
                        if parts[1].startswith('#'):
                            # Star with ship: /Star/#123
                            self.state.profile.scope.level = ScopeLevel.SHIP
                            self.state.profile.scope.star = parts[0]
                            self.state.profile.scope.planet = ""
                            self.state.profile.scope.ship = parts[1]
                        else:
                            # Planet level: /Hadar/Radha
                            self.state.profile.scope.level = ScopeLevel.PLANET
                            self.state.profile.scope.star = parts[0]
                            self.state.profile.scope.planet = parts[1]
                            self.state.profile.scope.ship = ""
                    elif len(parts) >= 3:
                        # Planet with ship: /Hadar/Radha/#123
                        self.state.profile.scope.level = ScopeLevel.SHIP
                        self.state.profile.scope.star = parts[0]
                        self.state.profile.scope.planet = parts[1] if not parts[1].startswith('#') else ""
                        self.state.profile.scope.ship = parts[-1]  # Last part is the ship
                
                # Update status bar
                self.ui.update_status(self.state.profile)
            
            # Check for login message to extract race/gov names
            # Format: RaceName "GovName" [race_id,gov_id] logged on.
            # May have leading newline or whitespace
            # GovName may be empty: ""
            login_match = re.search(r'(\S+)\s+"([^"]*)"\s+\[(\d+),(\d+)\]\s+logged on', line)
            if login_match:
                self.state.profile.race_name = login_match.group(1)
                gov_name = login_match.group(2)
                self.state.profile.gov_name = gov_name if gov_name else f"Gov{login_match.group(4)}"
                self.state.profile.race_id = int(login_match.group(3))
                self.state.profile.gov_id = int(login_match.group(4))
                logging.info(f"Detected login: {self.state.profile.race_name}/{self.state.profile.gov_name}")
                # Update status bar with new names
                self.ui.update_status(self.state.profile)
            
            # Regular game output
            self.display_output(line)
    
    async def handle_csp_command(self, command: str, args: List[str]):
        """Handle CSP protocol command"""
        logging.debug(f"CSP: {command} {args}")
        
        # Handle common CSP commands
        if command == "PROMPT":
            # Server is ready for input
            pass
        elif command == "1011":
            # CSP_CLIENT_ON - login notification with race/gov IDs
            if len(args) >= 2:
                self.state.profile.race_id = int(args[0])
                self.state.profile.gov_id = int(args[1])
                # Update status bar
                self.ui.update_status(self.state.profile)
        elif command == "SCOPE":
            # Scope change notification
            if args:
                self.update_scope(args)
        elif command == "APS":
            # Action points update
            if args:
                self.state.profile.scope.aps = int(args[0])
                # Update status bar
                self.ui.update_status(self.state.profile)
    
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
        
        # Update status bar
        self.ui.update_status(self.state.profile)
    
    async def input_loop(self):
        """Handle user input"""
        loop = asyncio.get_event_loop()
        
        while self.running:
            try:
                if self.ui.use_curses:
                    # Curses-based input with character-by-character handling
                    await asyncio.sleep(0.05)  # Small delay to prevent busy loop
                    ch = self.ui.get_input_char()
                    if ch is not None:
                        line = self.ui.handle_input_char(ch)
                        if line is not None:
                            await self.command_processor.process_command(line)
                else:
                    # Fallback to line-based input
                    line = await loop.run_in_executor(None, input, "> ")
                    await self.command_processor.process_command(line)
            except EOFError:
                break
            except Exception as e:
                logging.error(f"Input error: {e}")
    
    async def receive_loop(self):
        """Handle receiving data from server"""
        while self.running:
            try:
                # Add timeout so we can check running flag periodically
                data = await asyncio.wait_for(
                    self.connection.receive(), 
                    timeout=0.5
                )
                if data is None:
                    self.display_output("Connection closed by server")
                    self.running = False
                    break
                
                # Add to buffer and process complete lines
                lines = self.connection.buffer.add_data(data)
                for line in lines:
                    await self.process_server_message(line)
            except asyncio.TimeoutError:
                # Timeout is normal, just continue to check running flag
                continue
            except Exception as e:
                logging.error(f"Receive error: {e}")
                self.running = False
                break
    
    async def run(self):
        """Main client loop"""
        self.display_output(f"Galactic Bloodshed II Client v{__version__}")
        self.display_output(f"Connecting to {self.connection.host}:{self.connection.port}...")
        
        # Initialize status bar
        self.ui.update_status(self.state.profile)
        
        if not await self.connection.connect():
            self.display_output("Connection failed")
            return
        
        self.state.connected = True
        self.running = True
        
        # Create tasks for input and receive loops
        input_task = asyncio.create_task(self.input_loop())
        receive_task = asyncio.create_task(self.receive_loop())
        
        # Run loops concurrently, but stop both when one finishes
        try:
            # Wait for either task to complete (e.g., when running becomes False)
            done, pending = await asyncio.wait(
                [input_task, receive_task],
                return_when=asyncio.FIRST_COMPLETED
            )
            
            # Cancel any pending tasks
            for task in pending:
                task.cancel()
                try:
                    await task
                except asyncio.CancelledError:
                    pass
                    
        except KeyboardInterrupt:
            self.display_output("\nInterrupted")
            input_task.cancel()
            receive_task.cancel()
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
        '--no-curses',
        action='store_true',
        help='Disable curses UI (use simple line mode)'
    )
    parser.add_argument(
        '--version',
        action='version',
        version=f'%(prog)s {__version__}'
    )
    
    args = parser.parse_args()
    
    setup_logging(args.verbose)
    
    use_curses = not args.no_curses
    
    if use_curses:
        # Run with curses wrapper
        def curses_main(stdscr):
            client = GBClient(args.host, args.port, use_curses=True)
            client.ui.init(stdscr)
            try:
                asyncio.run(client.run())
            except Exception as e:
                logging.exception("Error in client")
                # Try to display error before cleanup
                try:
                    client.ui.display(f"\nError: {e}")
                except:
                    pass
                raise
            finally:
                client.ui.cleanup()
        
        try:
            curses.wrapper(curses_main)
        except KeyboardInterrupt:
            # Normal exit on Ctrl-C
            pass
        except curses.error as e:
            # Ignore curses cleanup errors (endwin() issues)
            if "endwin" not in str(e).lower():
                logging.exception("Curses error")
                print(f"Error: {e}", file=sys.stderr)
                return 1
        except Exception as e:
            logging.exception("Fatal error")
            print(f"Error: {e}", file=sys.stderr)
            return 1
    else:
        # Run without curses
        client = GBClient(args.host, args.port, use_curses=False)
        try:
            asyncio.run(client.run())
        except Exception as e:
            logging.exception("Fatal error")
            print(f"Error: {e}", file=sys.stderr)
            return 1
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
