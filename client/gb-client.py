#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""
Galactic Bloodshed II Client - Entry Point

A modern Python reimplementation of the classic gbII MUD-style game client
for Galactic Bloodshed servers.

Original C client: gbII (circa 1990-1993)
Python port: 2025
"""

import sys
from gb_client import main

if __name__ == '__main__':
    sys.exit(main())
