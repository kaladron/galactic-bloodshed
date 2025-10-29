#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""
Galactic Bloodshed II Client - Python Implementation

A modern Python reimplementation of the classic gbII MUD-style game client
for Galactic Bloodshed servers.

Original C client: gbII (circa 1990-1993)
Python port: 2025
"""

__version__ = "0.1.0"

# Export main classes for convenient imports
from .models import (
    ScopeLevel,
    Scope,
    Profile,
    Sector,
    PlanetMap,
    OrbitObject,
    OrbitMap,
    GameState,
)
from .ui import TerminalUI
from .network import NetworkBuffer, GameConnection
from .protocol import CSPProtocol, MapParser, OrbitMapParser
from .commands import CommandType, Command, CommandProcessor
from .client import GBClient, setup_logging, main

__all__ = [
    '__version__',
    # Models
    'ScopeLevel',
    'Scope',
    'Profile',
    'Sector',
    'PlanetMap',
    'OrbitObject',
    'OrbitMap',
    'GameState',
    # UI
    'TerminalUI',
    # Network
    'NetworkBuffer',
    'GameConnection',
    # Protocol
    'CSPProtocol',
    'MapParser',
    'OrbitMapParser',
    # Commands
    'CommandType',
    'Command',
    'CommandProcessor',
    # Client
    'GBClient',
    'setup_logging',
    'main',
]
