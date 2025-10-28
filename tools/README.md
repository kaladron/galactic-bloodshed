# Galactic Bloodshed Tools

This directory contains utility tools for working with Galactic Bloodshed game data.

## parse_map.py

Parse and display raw map data from the game in a human-readable format.

### Usage

```bash
./tools/parse_map.py '$PlanetName;width;height;show;sector_data...'
```

### Example

```bash
./tools/parse_map.py '$Radha;15;5;1;0?*0?.0?.0?.0?.0?.0?.0?.0?.0?.0?.0?*0?*0?#0?*0?*0?.0?.0?.0?.0?.0?.0?.0?.0?.0?.0?*0?*0?*0?*0?.0?.0?.0?.0?.0?.0?.0?.0?.0?.0?.0?.0?*0?*1@@0?.0?.0?.0?.0?.0?.0?.0?.0?.0?.0?.0?.0?.0?.0?.0?.0?.0?.0?#0?.0?.0?.0?.0?.0?.0?.0?.0?#0?#0?.'
```

This will output:
```
Planet: Radha
Size: 15 x 5

   012345678901234
00 *..........**#*
01 *..........****
02 ............**@
03 ...............
04 ...#........##.
```

### Terrain Legend

- `.` = sea/ocean
- `*` = land
- `^` = mountain
- `#` = ice
- `~` = gas
- `)` = forest
- `-` = desert
- `@` = ship (at home location)
- `x` = crystals
- `X/A/E/N` = troops (yours/allied/at war/neutral)
