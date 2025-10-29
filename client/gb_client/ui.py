# SPDX-License-Identifier: Apache-2.0
"""
Terminal UI for Galactic Bloodshed client

This module provides the curses-based terminal interface with:
- Split screen layout (output area + status bar + input line)
- Scrolling output window
- Interactive input with editing
- Map display with inverse video support
"""

import curses
import logging
from collections import deque
from typing import Optional, TYPE_CHECKING

if TYPE_CHECKING:
    from .models import PlanetMap, OrbitMap, Profile
    from .protocol import OrbitMapParser


class TerminalUI:
    """Basic curses-based terminal UI
    
    Provides a split screen with:
    - Main output area (scrolling)
    - Status bar (reverse video)
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
            # Import here to avoid circular dependency
            from .protocol import OrbitMapParser
            
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
        # Import here to avoid circular dependency
        from .protocol import MapParser
        
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
        # Import here to avoid circular dependency
        from .models import ScopeLevel
        
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
