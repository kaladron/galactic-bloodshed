# SPDX-License-Identifier: Apache-2.0
"""
Data models for Galactic Bloodshed client

This module contains all the data structures used by the client:
- Game state (profiles, scopes, sectors)
- Map data (planet maps, orbit maps)
"""

from dataclasses import dataclass, field
from enum import Enum
from typing import Dict, List, Optional


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
class ShipInSector:
    """Ship landed in a sector"""
    shipno: int = 0
    letter: str = "?"
    owner: int = 0


@dataclass
class SurveySector:
    """Detailed sector data from survey command"""
    x: int = 0
    y: int = 0
    sect_char: str = "."  # Terrain type character
    des: str = "."  # Description character (may show troops/ships)
    wasted: bool = False
    owner: int = 0
    eff: int = 0  # Efficiency percentage
    frt: int = 0  # Fertility
    mob: int = 0  # Mobilization
    xtal: bool = False  # Has crystals
    res: int = 0  # Resources
    civ: int = 0  # Civilian population
    mil: int = 0  # Military population
    mpopn: int = 0  # Max population
    ships: List[ShipInSector] = field(default_factory=list)


@dataclass
class SurveyData:
    """Survey data for a planet"""
    maxx: int = 0  # Width
    maxy: int = 0  # Height
    star: str = ""
    planet: str = ""
    res: int = 0  # Resource stockpile
    fuel: int = 0  # Fuel stockpile
    des: int = 0  # Destruct potential
    popn: int = 0  # Population
    mpopn: int = 0  # Max population
    tox: int = 0  # Toxicity
    compat: float = 0.0  # Compatibility percentage
    enslaved: int = 0  # Enslaved to player number (0 = not enslaved)
    sectors: List[SurveySector] = field(default_factory=list)
    
    def get_sector(self, x: int, y: int) -> Optional[SurveySector]:
        """Get sector at coordinates"""
        for sector in self.sectors:
            if sector.x == x and sector.y == y:
                return sector
        return None


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
    current_survey: Optional[SurveyData] = None
