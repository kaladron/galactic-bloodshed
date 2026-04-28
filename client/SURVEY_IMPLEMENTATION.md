# Survey Command Implementation - Summary

## What Was Implemented

The Python client now has full support for the `survey` command's CSP protocol messages, matching the functionality of the original gbII C client.

## Changes Made

### 1. Data Structures (models.py)

Added three new dataclasses:

- **`ShipInSector`** - Represents a ship landed in a sector
  - `shipno`: Ship number
  - `letter`: Ship type letter
  - `owner`: Owner player number

- **`SurveySector`** - Detailed sector information from survey
  - Position (x, y)
  - Terrain and description characters
  - Owner, efficiency, fertility, mobilization
  - Resources, crystals, populations
  - List of landed ships

- **`SurveyData`** - Complete planet survey
  - Planet dimensions and name
  - Resource/fuel stockpiles
  - Population and compatibility
  - Toxicity and enslaved status
  - List of sectors

### 2. Protocol Parser (protocol.py)

Added `SurveyParser` class with:

- **`parse_survey_intro()`** - Parses CSP 101 (planet overview)
- **`parse_survey_sector()`** - Parses CSP 102 (sector details)
  - Handles optional ship data (semicolon-separated)
  - Supports multiple ships per sector
- **`format_survey_display()`** - Formats survey data for terminal display
  - Shows planet info header
  - Displays sector table with all stats
  - Lists landed ships per sector

### 3. CSP Handler (client.py)

Added CSP message handlers in `handle_csp_command()`:

- **CSP 101** (SURVEY_INTRO) - Starts new survey, stores in game state
- **CSP 102** (SURVEY_SECTOR) - Adds sector to current survey
- **CSP 103** (SURVEY_END) - Displays completed survey

### 4. Client Command (commands.py)

Added `showsurvey` command:

- Redisplays the last parsed survey data
- Alias: `ss`
- Usage: `showsurvey` or `ss`

## Usage

### In-Game

1. **Scope to a planet:**
   ```
   cs /Star/Planet
   ```

2. **Run client_survey command (NOT regular survey):**
   ```
   client_survey -              # All sectors with CSP protocol
   client_survey 0:10,0:10     # Specific range with CSP protocol
   ```
   
   **Important:** You must use `client_survey` (not `survey`) to get CSP protocol messages that the client can parse. The regular `survey` command sends plain text output.

3. **View results:**
   - Survey automatically displays when complete
   - Or use `showsurvey` to redisplay last survey

### Example Output

```
=== Survey: Sol/Earth ===
Size: 10x10
Compatibility: 85.50%
Population: 5000 / 10000 (max)
Toxicity: 10%
Resources: 1000  Fuel: 500  Destruct: 0

 x,y cond/type  owner eff frt mob res  civ  mil ^popn xtals ships
---------------------------------------------------------------------------
 5,3   -   .       1 100%  50 127   25  100   50   150       
 2,1   ^   .       1 100%  30 100   50    0   25   100       D#123,c#456
```

## Protocol Details

### CSP_SURVEY_INTRO (101)
Format: `| 101 <maxx> <maxy> <star> <planet> <res> <fuel> <des> <popn> <mpopn> <tox> <compat> <enslaved>`

### CSP_SURVEY_SECTOR (102)
Format: `| 102 <x> <y> <sect_char> <des> <wasted> <owner> <eff> <frt> <mob> <xtal> <res> <civ> <mil> <mpopn>[;<shipno> <ltr> <owner>;...]`

Ship data is optional and appears after sector data, separated by semicolons.

### CSP_SURVEY_END (103)
Format: `| 103`

## Testing

All parser functions were tested with `test_survey.py`:
- ✓ Survey intro parsing
- ✓ Survey sector parsing
- ✓ Survey sector with multiple ships
- ✓ Display formatting

## Compatibility

This implementation matches the original gbII C client's survey handling from:
- `gbII/source/imap.c` - `process_client_survey()`
- CSP codes defined in `gbII/source/csp.h`
- Protocol documented in `gbII/docs/CLIENT_PROTOCOL`

## What's Next

The survey feature is now complete. Next priorities from the plan:
1. Interactive map mode (IMAP) - keyboard navigation of sectors
2. More pager for long output
3. Configuration file support (.gbrc)
