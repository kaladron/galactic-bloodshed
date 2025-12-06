# SPDX-License-Identifier: Apache-2.0
"""
Protocol handling for Galactic Bloodshed client

This module handles:
- Client-Server Protocol (CSP) parsing
- Planet map parsing and display
- Orbit/system map parsing and display
"""

import logging
from typing import List, Optional, Tuple, TYPE_CHECKING

if TYPE_CHECKING:
    from .models import PlanetMap, OrbitMap, Sector, OrbitObject, SurveyData, SurveySector, ShipInSector

from .models import PlanetMap, OrbitMap, Sector, OrbitObject, SurveyData, SurveySector, ShipInSector


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
        """Check if line is an orbit map data line
        
        Orbit lines start with #<digit> (e.g., #1 or #0 for explored flag).
        This distinguishes them from markdown headings which use # followed by space.
        """
        return len(line) >= 2 and line[0] == '#' and line[1].isdigit()
    
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


class SurveyParser:
    """Parser for survey command CSP messages
    
    The survey command sends CSP messages when used with coordinates or '-':
    
    CSP_SURVEY_INTRO (101): Planet overview
    Format: | 101 <maxx> <maxy> <star> <planet> <res> <fuel> <des> <popn> <mpopn> <tox> <compat> <enslaved>
    
    CSP_SURVEY_SECTOR (102): Individual sector data (can be multiple)
    Format: | 102 <x> <y> <sect_char> <des> <wasted> <owner> <eff> <frt> <mob> <xtal> <res> <civ> <mil> <mpopn>[;<shipno> <ltr> <owner>;...]
    
    CSP_SURVEY_END (103): End marker
    Format: | 103
    """
    
    CSP_SURVEY_INTRO = "101"
    CSP_SURVEY_SECTOR = "102"
    CSP_SURVEY_END = "103"
    
    @staticmethod
    def parse_survey_intro(args: List[str]) -> Optional[SurveyData]:
        """Parse survey intro message (CSP 101)
        
        Format: <maxx> <maxy> <star> <planet> <res> <fuel> <des> <popn> <mpopn> <tox> <compat> <enslaved>
        
        Args:
            args: List of arguments from CSP message
            
        Returns:
            SurveyData object with header info, or None if parsing fails
        """
        try:
            if len(args) < 12:
                logging.warning(f"Survey intro has too few args: {len(args)}")
                return None
            
            survey = SurveyData(
                maxx=int(args[0]),
                maxy=int(args[1]),
                star=args[2],
                planet=args[3],
                res=int(args[4]),
                fuel=int(args[5]),
                des=int(args[6]),
                popn=int(args[7]),
                mpopn=int(args[8]),
                tox=int(args[9]),
                compat=float(args[10]),
                enslaved=int(args[11])
            )
            
            logging.info(f"Parsed survey intro for {survey.star}/{survey.planet}: "
                        f"{survey.maxx}x{survey.maxy}, compat={survey.compat:.1f}%")
            
            return survey
            
        except (ValueError, IndexError) as e:
            logging.error(f"Failed to parse survey intro: {e}")
            return None
    
    @staticmethod
    def parse_survey_sector(args: List[str]) -> Optional[SurveySector]:
        """Parse survey sector message (CSP 102)
        
        Format: <x> <y> <sect_char> <des> <wasted> <owner> <eff> <frt> <mob> <xtal> <res> <civ> <mil> <mpopn>[;<shipno> <ltr> <owner>;...]
        
        The ship data (after semicolon) is optional and contains semicolon-separated ship info.
        
        Args:
            args: List of arguments from CSP message (may include ship data after semicolon)
            
        Returns:
            SurveySector object, or None if parsing fails
        """
        try:
            if len(args) < 14:
                logging.warning(f"Survey sector has too few args: {len(args)}")
                return None
            
            # Parse basic sector data
            sector = SurveySector(
                x=int(args[0]),
                y=int(args[1]),
                sect_char=args[2],
                des=args[3],
                wasted=(int(args[4]) != 0),
                owner=int(args[5]),
                eff=int(args[6]),
                frt=int(args[7]),
                mob=int(args[8]),
                xtal=(int(args[9]) != 0),
                res=int(args[10]),
                civ=int(args[11]),
                mil=int(args[12]),
                mpopn=int(args[13])
            )
            
            # Parse ship data if present (args[14] starts with semicolon, then ship data)
            # The ship format is: ;<shipno> <ltr> <owner>;<shipno> <ltr> <owner>;...
            if len(args) > 14:
                ship_data = ' '.join(args[14:])  # Rejoin in case ships got split
                # Remove leading semicolon and split by semicolon
                ship_parts = [s.strip() for s in ship_data.split(';') if s.strip()]
                
                for ship_part in ship_parts:
                    ship_fields = ship_part.split()
                    if len(ship_fields) >= 3:
                        ship = ShipInSector(
                            shipno=int(ship_fields[0]),
                            letter=ship_fields[1],
                            owner=int(ship_fields[2])
                        )
                        sector.ships.append(ship)
            
            return sector
            
        except (ValueError, IndexError) as e:
            logging.error(f"Failed to parse survey sector: {e}")
            return None
    
    @staticmethod
    def format_survey_display(survey: SurveyData) -> str:
        """Format survey data for display in terminal
        
        Args:
            survey: The parsed survey data
            
        Returns:
            Formatted string ready for display
        """
        lines = []
        
        # Header
        lines.append(f"=== Survey: {survey.star}/{survey.planet} ===")
        lines.append(f"Size: {survey.maxx}x{survey.maxy}")
        lines.append(f"Compatibility: {survey.compat:.2f}%")
        lines.append(f"Population: {survey.popn} / {survey.mpopn} (max)")
        lines.append(f"Toxicity: {survey.tox}%")
        lines.append(f"Resources: {survey.res}  Fuel: {survey.fuel}  Destruct: {survey.des}")
        if survey.enslaved:
            lines.append(f"ENSLAVED to player {survey.enslaved}!")
        lines.append("")
        
        # Sector table
        if survey.sectors:
            lines.append(" x,y cond/type  owner eff frt mob res  civ  mil ^popn xtals ships")
            lines.append("-" * 75)
            
            for sector in survey.sectors:
                # Format sector line
                xtal_str = "yes" if sector.xtal else ""
                wasted_str = "W" if sector.wasted else " "
                
                # Format ships if present
                ships_str = ""
                if sector.ships:
                    ship_letters = [f"{s.letter}#{s.shipno}" for s in sector.ships]
                    ships_str = " " + ",".join(ship_letters)
                
                line = (f"{sector.x:2d},{sector.y:<2d} {wasted_str}{sector.sect_char}   {sector.des}   "
                       f"{sector.owner:5} {sector.eff:3}% {sector.frt:3} {sector.mob:3} "
                       f"{sector.res:4} {sector.civ:4} {sector.mil:4} {sector.mpopn:5} "
                       f"{xtal_str:5}{ships_str}")
                lines.append(line)
        else:
            lines.append("(No sector data received)")
        
        return "\n".join(lines)
