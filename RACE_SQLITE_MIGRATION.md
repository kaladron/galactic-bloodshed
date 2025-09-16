# Race SQLite JSON Storage Migration

This document describes the implementation of Race data storage using SQLite with JSON serialization, building on the JSON serialization functionality added in PR #13.

## Overview

Race data in Galactic Bloodshed is now stored in a SQLite database table using JSON format instead of the previous binary file approach. This provides better data portability, easier debugging, and future extensibility.

## Implementation Details

### Database Schema

A new table `tbl_race` has been added to the SQLite database:

```sql
CREATE TABLE tbl_race(
  player_id INT PRIMARY KEY NOT NULL,
  race_data TEXT NOT NULL
);
```

- `player_id`: The unique player number (Race.Playernum)
- `race_data`: The complete Race structure serialized as JSON

### Modified Functions

#### `putrace(const Race& r)`
- Serializes the Race structure to JSON using the existing `race_to_json()` function
- Stores the JSON data in the `tbl_race` table
- Maintains backward compatibility by also writing to the original binary file

#### `getrace(player_t rnum)`
- First attempts to read from the SQLite database
- Deserializes JSON data using the existing `race_from_json()` function
- Falls back to the original binary file if SQLite data is not available
- Provides smooth migration path

### Backward Compatibility

The implementation maintains full backward compatibility:

1. **Read Path**: `getrace()` tries SQLite first, falls back to file if needed
2. **Write Path**: `putrace()` writes to both SQLite and file during transition
3. **Migration**: Existing installations continue to work without data loss

## JSON Serialization

The implementation uses the Glaze library for JSON serialization, with comprehensive reflection metadata defined for:

- `Race` structure and all its nested components
- `Race::gov` (governor) structure
- `toggletype` structure
- All arrays and primitive fields

## Testing

Three test programs verify the implementation:

1. **`race_serialization_test`**: Tests JSON serialization/deserialization (existing)
2. **`race_sqlite_test`**: Tests SQLite storage and retrieval
3. **`race_integration_demo`**: Comprehensive demonstration of the complete flow

## Benefits

1. **Data Portability**: JSON format is human-readable and cross-platform
2. **Debugging**: Race data can be inspected and modified using standard tools
3. **Future Extensibility**: Easy to add new fields or modify structure
4. **Database Benefits**: Leverages SQLite's ACID properties and query capabilities
5. **No Data Loss**: Smooth migration preserves all existing data

## Migration Strategy

The implementation uses a gradual migration approach:

1. **Phase 1** (Current): Dual write (both SQLite and file), SQLite-first read
2. **Phase 2** (Future): Remove file fallback after confirming SQLite stability
3. **Phase 3** (Future): Remove legacy file operations entirely

## Usage

The changes are transparent to existing code. All existing `putrace()` and `getrace()` calls continue to work without modification.

```cpp
// Existing code continues to work unchanged
Race player_race = getrace(player_num);
player_race.tech += 10.0;
putrace(player_race);
```

The Race data will now be automatically stored as JSON in SQLite while maintaining compatibility with existing file-based storage.