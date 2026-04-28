# Client-Server Protocol (CSP) Commands Reference

## Overview

This document lists all CSP commands supported by the original gbII client and indicates which ones are implemented in the Python client.

## CSP Commands Implemented by Original gbII Client

### ✅ Currently Implemented in Python Client

#### SURVEY (CSP 101-103) ✅ COMPLETE
- **Command:** `client_survey -` or `client_survey x:x,y:y`
- **CSP Messages:**
  - `101` - Survey intro (planet overview)
  - `102` - Survey sector (detailed sector data)
  - `103` - Survey end
- **Status:** Fully implemented with parsing and display

#### SCOPE_PROMPT (CSP 40) ✅ PARTIAL
- **CSP Message:** `40` - Scope prompt
- **Status:** Not sent by current server (uses text prompts instead)
- **Python Client:** Parses text prompts like `( [APs] /scope/path )`

#### CLIENT_ON/OFF (CSP 30-31) ✅ PARTIAL
- **CSP Messages:**
  - `30` - Client mode on (login notification)
  - `31` - Client mode off
- **Status:** Python client handles login detection differently

### 🚧 Not Yet Implemented (But Supported by gbII)

#### 1. EXPLORE (CSP 501-506)
- **Command:** Client sends CSP explore request
- **CSP Messages:**
  - `501` - Explore intro
  - `502` - Explore star
  - `503` - Star aliens info
  - `504` - Star data (# Name Ex Inhab Auto Slaved Toxic Compat Type)
  - `505` - Star end
  - `506` - Explore end
- **Purpose:** Explore universe/star systems with detailed info
- **Priority:** Medium - useful for navigation

#### 2. MAP (CSP 601-606)
- **Command:** Client sends CSP map request
- **CSP Messages:**
  - `601` - Map intro
  - `602` - Map dynamic 1 (Type Sects Guns MobPoints Res Des Fuel Xtals)
  - `603` - Map dynamic 2 (Mob AMob Compat Pop ^Pop ^TPop Mil Tax ATax Deposits Est Prod)
  - `604` - Map aliens
  - `605` - Map data
  - `606` - Map end
- **Purpose:** Enhanced planet map with economic/military stats
- **Priority:** Medium - richer than basic map display

#### 3. PROFILE (CSP 301-310)
- **Command:** Client sends CSP profile request
- **CSP Messages:**
  - `301` - Profile intro
  - `302` - Profile personal
  - `303` - Profile dynamic (Active Knowledge Capital Morale Gun GTele STele)
  - `304` - Profile dynamic other (%Know Morale Gun GTele OTele SecPref)
  - `305` - Profile race stats
  - `306` - Profile planet
  - `307` - Profile sector
  - `308` - Profile discovery
  - `309/310` - Profile end
- **Purpose:** Detailed race/player profile information
- **Priority:** Medium - useful for tracking player stats

#### 4. RELATION (CSP 201-203)
- **Command:** Client sends CSP relation request
- **CSP Messages:**
  - `201` - Relation intro (race # & name)
  - `202` - Relation data
  - `203` - Relation end
- **Purpose:** View diplomatic relations between races
- **Priority:** Low-Medium - nice to have

#### 5. WHO (CSP 401-403)
- **Command:** Client sends CSP who request
- **CSP Messages:**
  - `401` - Who intro
  - `402` - Who data (player info)
  - `403` - Who end / cowards count
- **Purpose:** See who's logged in
- **Priority:** Low - informational

#### 6. ORBIT (CSP 2020-2025)
- **Command:** Client sends CSP orbit request (1501)
- **CSP Messages:**
  - `2020` - Orbit output intro
  - `2021` - Orbit star data
  - `2022` - Unexplored planet data
  - `2023` - Explored planet data
  - `2024` - Ship data
  - `2025` - Orbit output end
- **Purpose:** Enhanced orbit display with detailed object info
- **Priority:** Medium - richer than basic orbit display
- **Note:** Python client already has basic orbit parsing from `#` format

#### 7. SHIPDUMP (CSP 2030-2055)
- **Command:** Client sends shipdump request (1504)
- **CSP Messages:**
  - `2030` - General information
  - `2031` - Stock information
  - `2032` - Status information
  - `2033` - Weapons information
  - `2034` - Factory information
  - `2035` - Destination information
  - `2036-2040` - Tactical information
  - `2041` - Ship orders
  - `2042` - Threshloading
  - `2043` - Special abilities
  - `2044` - Hyperdrive usage
  - `2055` - End
- **Purpose:** Detailed ship information dump
- **Priority:** Low - very detailed, probably overkill

#### 8. PLANDUMP (CSP 2000-2009)
- **Command:** Client sends plandump request (1503)
- **CSP Messages:**
  - `2000` - Planet intro
  - `2001` - Conditions
  - `2002` - Stockpiles
  - `2003` - Production
  - `2004` - Miscellaneous
  - `2005` - Not explored
  - `2009` - End
- **Purpose:** Detailed planet information dump
- **Priority:** Low-Medium - detailed planet stats

#### 9. UNIVDUMP (CSP 4010-4011)
- **Command:** Client sends univdump request (1114)
- **CSP Messages:**
  - `4010` - Universe dump intro
  - `4011` - Star data
- **Purpose:** Dump entire universe data
- **Priority:** Low - mostly for mapping tools

#### 10. STARDUMP (CSP 4000-4004)
- **Command:** Client sends stardump request (1112)
- **CSP Messages:**
  - `4000` - Star dump intro
  - `4001` - Star conditions
  - `4002` - Planet data
  - `4003` - End
  - `4004` - Wormhole data
- **Purpose:** Detailed star system dump
- **Priority:** Low-Medium

### 🔔 Event Notifications (Passive - Server Sends)

#### UPDATE/SEGMENT/BACKUP Events (CSP 50-59) ✅ SHOULD IMPLEMENT
- **CSP Messages:**
  - `50` - Update started
  - `51` - Update finished
  - `52` - Segment started
  - `53` - Segment finished
  - `54` - Reset started
  - `55` - Reset finished
  - `56` - Backup started
  - `57` - Backup finished
  - `58` - Updates suspended
  - `59` - Updates resumed
- **Purpose:** Notify client of game state changes
- **Priority:** HIGH - important for game state awareness
- **Implementation:** Simple - just display notifications

#### BROADCAST/ANNOUNCE/THINK/SHOUT (CSP 802-805)
- **CSP Messages:**
  - `802` - Broadcast
  - `803` - Announce
  - `804` - Think
  - `805` - Shout/Emote
- **Purpose:** Different types of communications
- **Priority:** Medium - nice for distinguishing message types

#### PING (CSP 60)
- **CSP Message:** `60` - Ping/pong
- **Purpose:** Keep-alive mechanism
- **Priority:** Low - connection usually stays alive anyway

#### PAUSE (CSP 61)
- **CSP Message:** `61` - Pause display
- **Purpose:** Pause output during long operations
- **Priority:** Low

### ❌ Error Messages (CSP 9900-9905)
- **CSP Messages:**
  - `9900` - General error
  - `9901` - Too many arguments
  - `9902` - Too few arguments
  - `9903` - Unknown command
  - `9904` - No such player
  - `9905` - No such place (scope error)
- **Purpose:** Error handling
- **Priority:** Medium - good for user feedback

## Recommendations for Python Client

### High Priority (Implement Next)
1. **Update/Segment Events (50-59)** - Easy to implement, important for game awareness
2. **Error Messages (9900-9905)** - Better error handling
3. **EXPLORE (501-506)** - Useful for navigation and universe exploration

### Medium Priority
4. **MAP (601-606)** - Enhanced map with economic/military data
5. **PROFILE (301-310)** - Player statistics tracking
6. **ORBIT (2020-2025)** - Enhanced orbit display
7. **Broadcast types (802-805)** - Better message type handling

### Low Priority (Nice to Have)
8. **RELATION (201-203)** - Diplomatic relations
9. **WHO (401-403)** - Player list
10. **Planet/Star dumps** - Very detailed data, mostly for power users

### Very Low Priority (Probably Skip)
11. **SHIPDUMP (2030-2055)** - Too detailed, better done with regular commands
12. **UNIVDUMP** - Mapping tool feature, not essential for gameplay

## Server Implementation Status

**Important Note:** Most CSP commands are **defined but not implemented** in the current server. Only these actually work:

✅ **Working in Current Server:**
- SURVEY (101-103) via `client_survey` command
- Basic login/scope handling

❌ **Not Implemented in Server:**
- EXPLORE, MAP, PROFILE, RELATION, WHO, ORBIT, dumps, etc.
- Server would need to implement these before client can use them

## Next Steps

1. **Implement Event Notifications** - Easy win, server already sends some of these
2. **Check which CSP commands the server actually implements** - Test with telnet
3. **Implement error handling** - Parse CSP error messages
4. **Add EXPLORE support** - If server implements it, very useful feature
