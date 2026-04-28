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

## Where IDs Are Defined

`gb/types.cppm`:

```cpp
export using player_t    = ID<"player">;
export using governor_t  = ID<"governor">;
export using starnum_t   = std::uint32_t;
export using planetnum_t = std::uint32_t;
export using shipnum_t   = std::uint64_t;
export using commodnum_t = std::int64_t;
```

`player_t` and `governor_t` are strong; the others are still type aliases (consistent within the codebase but not enforced by the compiler — still prefer the alias name for clarity).

## Always Use the Typed Name

```cpp
// ❌ Hides intent and breaks if int width changes
int player = g.player;
for (int i = 0; i <= MAXGOVERNORS; ++i) { ... }

// ✅ Tells the compiler — and the reader — what the value means
player_t player = g.player;
for (governor_t gov{0}; gov <= MAXGOVERNORS; ++gov) { ... }
```

Function signatures, struct fields, loop variables, and locals that hold an identifier must use the typed name.

## Comparisons Are Natural

`ID<...>` defines `operator<=>`, so comparisons against integer literals and the typed constants work directly:

```cpp
governor_t gov = g.governor;
if (gov <= MAXGOVERNORS) { ... }       // ✅ ok
if (gov > 0) { ... }                   // ✅ ok
if (new_gov < 0 || new_gov > MAXGOVERNORS) { ... }  // ✅ ok
```

Do not reach for `gov.value` to "make it compile" — if a comparison fails, you have crossed type boundaries.

## Construction and Conversion

```cpp
player_t p{1};                 // explicit construction from int
governor_t gov = governor_t{0};

player_t bad = 1;              // ❌ implicit conversion is rejected
governor_t mixed = player_id;  // ❌ cannot cross ID brands
```

When parsing user input, parse to `int`/`unsigned`, validate, then construct the strong ID.

## JSON Serialization

`ID<...>` serializes transparently as a plain integer through Glaze, so existing JSON in the database stays compatible. No special meta is needed.

## Iteration

Prefer the iterator helpers (e.g. `Race::active_governors()`) when available — they yield typed entries:

```cpp
for (auto [gov_id, gov_data] : race.active_governors()) {
  // gov_id is governor_t
}
```

Manual numeric loops are still acceptable when the code genuinely needs the index (e.g. bookkeeping side arrays).

## Refusing Type Confusion at Boundaries

If two functions accept both a player and a governor, type them separately so the compiler enforces order:

```cpp
// ✅ caller cannot swap these silently
void notify(player_t player, governor_t gov, std::string_view msg);
```

If you find a function still typed `int player, int gov`, fix the signature; that is exactly the bug class strong IDs prevent.

## Anti-Patterns

- ❌ Using `int` / `unsigned` to hold IDs.
- ❌ `static_cast<int>(gov)` or `gov.value` just to compile a comparison.
- ❌ Implicit construction (`player_t p = 1;`).
- ❌ Mixing brands (`player_t = governor_t{0};`).
- ❌ Custom JSON meta for ID fields — Glaze already handles it.
- ❌ Reintroducing `int` parameters in new APIs that take an identifier.

## Checklist

- [ ] Every variable, parameter, field, and return type that holds an identifier uses the typed alias
- [ ] No `.value` access in comparisons
- [ ] No implicit ID construction from raw integers
- [ ] User input is parsed into a primitive, validated, then wrapped in the strong ID
- [ ] New function signatures use distinct types when both player and governor are accepted
- [ ] No casts between different ID brands
