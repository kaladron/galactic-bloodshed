#!/usr/bin/env python3
"""
Parse and display Galactic Bloodshed map data.

This tool takes the raw map string output from the game and displays it as a
readable terrain grid.

Usage:
    ./parse_map.py '$Radha;15;5;1;0?*0?.0?.0?...'
"""

import sys


def parse_map(data):
    """Parse map data string and return structured information."""
    # Parse header
    parts = data.split(';', 4)
    if len(parts) < 5:
        print("Error: Invalid map data format", file=sys.stderr)
        sys.exit(1)
    
    name = parts[0][1:]  # Remove leading $
    width = int(parts[1])
    height = int(parts[2])
    show = int(parts[3])
    sectors_data = parts[4]
    
    # Parse sectors - each sector is 3 characters (owner_char, flag, terrain_char)
    sectors = []
    i = 0
    while i < len(sectors_data):
        if i + 2 < len(sectors_data):
            owner = sectors_data[i]
            flag = sectors_data[i+1]
            terrain = sectors_data[i+2]
            sectors.append({
                'owner': owner,
                'flag': flag,
                'terrain': terrain
            })
            i += 3
        else:
            break
    
    return {
        'name': name,
        'width': width,
        'height': height,
        'show': show,
        'sectors': sectors
    }


def display_map(map_data):
    """Display the map as a readable grid."""
    name = map_data['name']
    width = map_data['width']
    height = map_data['height']
    sectors = map_data['sectors']
    
    print(f"Planet: {name}")
    print(f"Size: {width} x {height}")
    print()
    
    # Display column headers
    print("   " + "".join(str(x % 10) for x in range(width)))
    
    # Display each row
    for y in range(height):
        row_start = y * width
        row_end = row_start + width
        if row_end <= len(sectors):
            row = [s['terrain'] for s in sectors[row_start:row_end]]
        else:
            row = [s['terrain'] for s in sectors[row_start:]]
        print(f"{y:02d} {''.join(row)}")
    
    print()
    print("Legend:")
    print("  . = sea/ocean")
    print("  * = land")
    print("  ^ = mountain")
    print("  # = ice")
    print("  ~ = gas")
    print("  ) = forest")
    print("  - = desert")
    print("  @ = ship (at home location)")
    print("  x = crystals")
    print("  X/A/E/N = troops (yours/allied/at war/neutral)")


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        print("Error: No map data provided", file=sys.stderr)
        sys.exit(1)
    
    map_string = sys.argv[1]
    
    # Handle if the string doesn't start with $
    if not map_string.startswith('$'):
        print("Error: Map data should start with '$'", file=sys.stderr)
        sys.exit(1)
    
    map_data = parse_map(map_string)
    display_map(map_data)


if __name__ == '__main__':
    main()
