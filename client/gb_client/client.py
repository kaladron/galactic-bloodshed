# SPDX-License-Identifier: Apache-2.0
"""
Main client for Galactic Bloodshed

This module contains:
- GBClient - main client orchestration
- setup_logging - logging configuration
- main - entry point function
"""

import argparse
import asyncio
import curses
import logging
import re
import sys
from typing import List

from .models import GameState, ScopeLevel
from .ui import TerminalUI
from .network import GameConnection
from .protocol import CSPProtocol, MapParser, OrbitMapParser
from .commands import CommandProcessor


__version__ = "0.1.0"


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
