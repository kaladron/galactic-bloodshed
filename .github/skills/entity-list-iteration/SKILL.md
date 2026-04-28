---
name: entity-list-iteration
description: 'Iterate over collections of game entities (races, stars, planets, ships, commodities) using XxxList helpers. Use when scanning every race/star/planet/ship in a command, turn-processing loop, or test, when deciding between read-only and mutable iteration, or when migrating a numeric for-loop to the entity list pattern. Covers ::readonly() factory, mutable handle iteration, composite-key lists (PlanetList), ShipList iteration modes, and when raw numeric loops are still appropriate.'
user-invocable: false
---

# Entity List Iteration

`RaceList`, `StarList`, `PlanetList`, `CommodList`, and `ShipList` provide range-for compatible iteration over entities managed by `EntityManager`. They replace ad-hoc `for (int i = 0; i < num_xxx(); ++i)` loops with cleaner, type-safe alternatives.

There are **two modes** that match the two `EntityManager` access modes:

| Mode | Construction | Yields | Use for |
| --- | --- | --- | --- |
| Read-only | `XxxList::readonly(em, ...)` | `const T*` | Inspect / report |
| Mutable | `XxxList(em, ...)` | `EntityHandle<T>` | Modify, with auto-save |

## Read-Only Iteration

```cpp
for (const Race* race : RaceList::readonly(em)) {
  g.out << std::format("{}: tech {}\n", race->name, race->tech);
}
```

Behavior:

- `::readonly(...)` returns `const Derived` — that constness is what makes range-for pick the const `begin()` and yield `const T*`. **Always store the factory result inline** (or use it directly in the for-range expression). Assigning to a non-const variable defeats the constness.
- No handles, no refcount churn, no auto-save.
- Use this everywhere a loop only reads.

## Mutable Iteration

```cpp
for (auto race_handle : RaceList(em)) {
  race_handle->tech += 1.0;   // marks dirty; saved on next iteration
}
```

Each iteration yields a fresh `EntityHandle<T>`. Modifications go through `handle->...` or `*handle`, and the handle’s destructor at the end of the loop body fires the auto-save.

## Composite-Key Lists (Planets)

`PlanetList` iterates over the planets of a specific star:

```cpp
const auto* star = em.peek_star(snum);
for (auto planet_handle : PlanetList(em, snum, *star)) {
  planet_handle->popn += migrants;
}
// Read-only equivalent:
for (const Planet* p : PlanetList::readonly(em, snum, *star)) {
  observe(*p);
}
```

## ShipList Iteration Modes

`ShipList` accepts an `IterationType` because ships are scattered across linked lists by location:

```cpp
// All living ships
for (auto sh : ShipList(em, ShipList::IterationType::AllAlive)) { ... }

// Ships in a star system / planet / fleet — see ShipList declaration
```

Read-only equivalent uses `ShipList::readonly(em, IterationType::AllAlive)`.

## When To Keep a Numeric Loop

Use `for (starnum_t s{0}; s < em.num_stars(); ++s)` only when the **index itself** is needed — for example to look up parallel arrays, to print "system %d", or to seed RNGs deterministically. If the loop body just touches the entity, switch to a list.

## Migration Recipe

To migrate a loop:

1. Identify whether the body mutates (write to fields, call `put*`, mark dirty) or only reads.
2. Pick the matching list and mode.
3. Drop the index variable unless it is referenced inside the body.
4. Remove manual `peek_xxx`/`get_xxx` calls — the list does the lookup.
5. Build and run tests.

## Common Pitfalls

- **Lost constness on readonly factory.** Capturing the factory in `auto list = RaceList::readonly(em);` may bind to a non-const variable in some scopes; the safe form is using it directly in `for (... : RaceList::readonly(em))`. The codebase assumes inline use.
- **Holding a handle past the loop body.** The handle’s save fires on destruction. Don’t store handles in containers — use IDs.
- **Mixing modes.** Don’t pass a mutable list to code that should not mutate; that obscures intent and causes spurious dirty marks.

## Anti-Patterns

- ❌ `for (int i = 0; i < em.num_races(); ++i) { auto* r = em.peek_race(i); ... }` for read-only loops — use `RaceList::readonly`.
- ❌ Assigning `RaceList::readonly(em)` to a `RaceList` (non-const) variable.
- ❌ Iterating mutable when only reading — extra refcount/save cost and misleading.
- ❌ Calling `em.get_xxx(i)` inside a numeric loop just to mutate — use the mutable list and let the handle auto-save.

## Checklist

- [ ] Read-only loops use `XxxList::readonly(...)` inline
- [ ] Mutable loops use `XxxList(...)` and operate through the handle
- [ ] Numeric loops kept only when the index is genuinely needed
- [ ] No handles stored beyond the loop iteration
- [ ] PlanetList uses the composite (star number, star) signature
- [ ] ShipList passes the right `IterationType`
