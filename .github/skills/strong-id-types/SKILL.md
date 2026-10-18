---
name: strong-id-types
description: 'Work with strong ID types like player_t and governor_t (defined as ID<"name", int>). Use when declaring identifier-typed variables, comparing IDs, passing IDs across function boundaries, serializing IDs to JSON, or fixing compile errors caused by ID type confusion. Covers when raw int is wrong, comparison rules, and migration patterns.'
user-invocable: false
---

# Strong ID Types

Identifier types like `player_t`, `governor_t`, `shipnum_t`, `starnum_t` are not interchangeable integers. `player_t` and `governor_t` are `ID<"player">` / `ID<"governor">` — distinct types that the compiler refuses to mix. Use them deliberately to keep the type system catching the bugs that look like:

```cpp
notify_player(governor, player, msg);  // swapped args — should not compile
```

## Where IDs & Metric Aliases Are Defined

`gb/entities/types.cppm`:

```cpp
// Strong IDs (ID<"brand", T>):
export using player_t    = ID<"player">;                // 1-based (>= 1)
export using governor_t  = ID<"governor">;              // 1-based (>= 1, 1 = Leader)
export using starnum_t   = ID<"star", std::uint32_t>;   // 1-based (1..numstars)
export using planetnum_t = ID<"planet", std::uint32_t>; // 1-based (1..numplanets)
export using shipnum_t   = ID<"ship", std::uint64_t>;   // 1-based (>= 1)
export using commodnum_t = ID<"commod", std::int64_t>;  // 1-based (>= 1)

// Semantic Metric Aliases (signed int32_t / int64_t):
export using segments_t     = std::int32_t;
export using turn_t         = std::int32_t;  ///< Full update turn counter
export using ap_t           = std::int32_t;
export using planet_count_t = std::int32_t;  ///< Cardinality / tally of planets or star systems
export using resource_t     = std::int64_t;
export using money_t        = std::int64_t;
export using population_t   = std::int64_t;
export using victory_score_t = std::int64_t;
export using armor_t        = std::int32_t;  ///< Armor rating absorbing combat damage
export using damage_t       = std::int32_t;  ///< Hull damage percentage (0..100)
export using speed_t        = Bounded<"speed", std::int32_t, 0, 9>;
export using radiation_t    = std::int32_t;  ///< Accumulated radiation dose level
export using gun_count_t    = std::int32_t;  ///< Number of gun mounts or battery power rating
export using gun_caliber_t  = std::int32_t;  ///< Gun battery caliber multiplier (0..3)
export using charge_t       = std::int32_t;  ///< Faster-than-light hyperdrive charge accumulator
export using bearing_t      = std::int32_t;  ///< Course navigation heading angle (0..359)
export using hangar_t       = std::int32_t;  ///< Internal carried ship hangar capacity/space
export using ship_size_t    = std::int32_t;  ///< Ship physical size / volume
export using weapon_power_t = std::int32_t;  ///< Concentrated energy weapon / laser power setting
export using weapon_range_t = std::int32_t;  ///< Tactical weapon or mine proximity trigger range
export using hit_count_t    = std::int32_t;  ///< Number of successful weapon hits or penetrations
export using hit_odds_t     = std::int32_t;  ///< Weapon hit probability percentage (0..100)
export using sector_count_t = std::int32_t;  ///< Cardinality / tally of planetary sectors
export using temp_delta_t   = std::int32_t;  ///< Thermal adjustment applied to planet temperature
export using iq_t           = std::int32_t;  ///< Race intelligence rating
export using fighters_t     = std::int32_t;  ///< Race military combat capability rating
export using sexes_t        = std::int32_t;  ///< Number of reproductive sexes
```

All entity and governor identifiers (`player_t`, `governor_t`, `starnum_t`, `planetnum_t`, `shipnum_t`, `commodnum_t`) are uniformly **1-based (`>= 1`)**; `0` is never a valid entity or governor ID. Any nullable foreign key reference MUST be wrapped in `std::optional<ID>` so that absent references serialize as JSON `null` and SQL `NULL`. Sector `(x, y)` grid coordinates are 0-based.

## Always Use the Typed Name

```cpp
// ❌ Hides intent and breaks if int width changes
int player = g.player();

// ✅ Tells the compiler — and the reader — what the value means
player_t player = g.player();
for (auto [gov_id, gov_data] : race.active_governors()) { ... }
```

Function signatures, struct fields, loop variables, and locals that hold an identifier must use the typed name.

## Comparisons Are Natural

`ID<...>` defines `operator<=>`, so comparisons against integer literals and the typed constants work directly:

```cpp
governor_t gov = g.governor();
if (gov >= 1) { ... }                  // ✅ ok
if (new_gov < 1 || !race.has_governor(new_gov)) { ... }  // ✅ ok
```

Do not reach for `gov.value` to "make it compile" — if a comparison fails, you have crossed type boundaries.

## Construction and Conversion

```cpp
player_t p{1};                 // explicit construction from int
governor_t gov = governor_t::leader();

player_t bad = 1;              // ❌ implicit conversion is rejected
governor_t mixed = player_id;  // ❌ cannot cross ID brands
```

When parsing user input, parse to `int`/`unsigned`, validate, then construct the strong ID.

## JSON Serialization

`ID<...>` serializes transparently as a plain integer through Glaze, so existing JSON in the database stays compatible. No special meta is needed.

## Iteration & Sparse Player Containers

There is no fixed upper player cap (`MAXPLAYERS`). Per-player sets and maps use sparse `std::flat_set<player_t>` and `std::flat_map<player_t, V>` containers (or `std::flat_map<governor_t, V>` for governors):

1. **Iterating over enrolled races**:
   ```cpp
   // ✅ Iterate over enrolled races via EntityManager
   for (const Race& race : RaceList::readonly(em)) {
     scores[race.Playernum] = 100;
   }
   ```

2. **Iterating over sparse per-player sets or maps on entities**:
   ```cpp
   // ✅ Iterate over players who have explored a star or planet
   for (player_t p : star.explored()) { ... }
   for (const auto& [player_id, pinfo] : planet.info_map()) { ... }
   ```

3. **Iterating over active governors**:
   ```cpp
   // ✅ Yields typed entries with governor_t
   for (auto [gov_id, gov_data] : race.active_governors()) {
     // gov_id is governor_t
   }
   ```

## Refusing Type Confusion at Boundaries

If two functions accept both a player and a governor, type them separately so the compiler enforces order:

```cpp
// ✅ caller cannot swap these silently
void notify(player_t player, governor_t gov, std::string_view msg);
```

If you find a function still typed `int player, int gov`, fix the signature; that is exactly the bug class strong IDs prevent.

## Anti-Patterns

- ❌ Using fixed-size player arrays instead of `std::flat_map<player_t, V>` / `std::flat_set<player_t>` or `RaceList::readonly(em)`.
- ❌ Using `int` / `unsigned` to hold IDs.
- ❌ `static_cast<int>(gov)` or `gov.value` just to compile a comparison.
- ❌ Implicit construction (`player_t p = 1;`).
- ❌ Mixing brands (`player_t = governor_t{1};`).
- ❌ Custom JSON meta for ID fields — Glaze already handles it.
- ❌ Reintroducing `int` parameters in new APIs that take an identifier.


## Checklist

- [ ] Every variable, parameter, field, and return type that holds an identifier uses the typed alias
- [ ] No `.value` access in comparisons
- [ ] No implicit ID construction from raw integers
- [ ] User input is parsed into a primitive, validated, then wrapped in the strong ID
- [ ] New function signatures use distinct types when both player and governor are accepted
- [ ] No casts between different ID brands
