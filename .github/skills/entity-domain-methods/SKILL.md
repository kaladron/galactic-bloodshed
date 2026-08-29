---
name: entity-domain-methods
description: 'Query and mutate entity state using first-class member methods, computed predicates, and structured domain types. Use when querying ship capabilities, docking/landing states, hyperdrive readiness, planetary adjacency, diplomatic relations, or player exploration/habitation sets.'
user-invocable: false
---

# Entity Domain Methods & Computed Predicates

Game entities (`Ship`, `Planet`, `Star`, `Race`, `block`) encapsulate their internal data and expose rich domain member methods, computed predicates, and structured manifests.

## 1. Computed Predicates Over Stored State

Derived conditions are computed dynamically on the entity from ground-truth state:

| Entity | Method | Description |
| :--- | :--- | :--- |
| `Ship` | `ship.is_docked()` | Whether ship is docked in a carrier (`docked && whatdest == LEVEL_SHIP`) |
| `Ship` | `ship.is_landed()` | Whether ship is landed on a planet (`docked && whatdest == LEVEL_PLAN`) |
| `Ship` | `ship.is_laser_on()` | Whether combat laser is equipped and dialed to fire (`laser && fire_laser > 0`) |
| `Ship` | `ship.is_overloaded()` | Whether carried cargo, fuel, or crew exceeds capacity limits |
| `Ship` | `ship.mass()` | Dynamic total mass calculation (chassis + cargo + crew + fuel + crystals) |
| `Ship` | `ship.can_bombard()` | Whether ship type has planetary bombardment capability |
| `Ship` | `ship.can_navigate()` | Whether ship type is capable of course navigation |
| `Ship` | `ship.can_aim()` | Whether ship type (e.g. Space Mirror) can aim at celestial targets |
| `Ship` | `ship.has_switch()` | Whether ship type possesses an on/off operational switch |
| `HyperDriveData` | `hyper_drive.is_ready()` | Whether accumulator has reached readiness (`charge >= HYPER_DRIVE_READY_CHARGE`) |
| `Planet` | `planet.is_adjacent(from, to)` | Whether two sector coordinates are geometrically adjacent on the grid |

```cpp
// ✅ Query computed states directly on the entity
if (ship.is_docked()) {
  g.out << "Ship is currently docked.\n";
}
if (ship.hyper_drive().is_ready()) {
  g.out << "Hyperdrive ready for jump.\n";
}
```

## 2. Encapsulated Multi-Player Sets & Diplomatic States

Multi-player sets (exploration, inhabitation, diplomatic relations, and alliance bloc memberships) are queried and mutated strictly through domain methods on the entity:

### `Star` Exploration & Inhabitation

```cpp
// Read-only queries
if (star.is_explored_by(player)) { ... }
if (star.is_inhabited_by(player)) { ... }
if (star.is_inhabited()) { ... }  // True if any player inhabits the system

// Mutations (inside mutate_star)
star.mark_explored_by(player);
star.mark_inhabited_by(player);
star.clear_inhabited_by(player);
star.clear_all_inhabitants();
```

### `Race` Diplomatic Relations

```cpp
// Read-only queries
if (race.is_allied_with(target_player)) { ... }
if (race.is_at_war_with(target_player)) { ... }

// Mutations (inside mutate_race)
race.declare_alliance_with(target_player);
race.rescind_alliance_with(target_player);
race.declare_war_on(target_player);
race.make_peace_with(target_player);
```

### Alliance `block` Methods

```cpp
// Read-only queries
if (bloc.is_invited(player)) { ... }
if (bloc.is_pledged(player)) { ... }
if (bloc.is_allied_with(target_player)) { ... }
if (bloc.is_at_war_with(target_player)) { ... }

// Mutations (inside mutate_block)
bloc.invite_player(player);
bloc.cancel_invite(player);
bloc.pledge_player(player);
bloc.unpledge_player(player);
bloc.declare_alliance_with(target_player);
bloc.declare_war_on(target_player);
```

## 3. Strongly-Typed Manifests & Domain Structs

Domain concepts with multiple boolean options are modeled as named structs rather than raw integer bitmasks:

### `CommodityManifest` (Shipping Routes)

```cpp
export struct CommodityManifest {
  bool fuel{false};       ///< Fuel commodity
  bool destruct{false};   ///< Destruct potential commodity
  bool resources{false};  ///< Minerals / raw resources commodity
  bool crystals{false};   ///< Power crystals commodity

  [[nodiscard]] constexpr bool any() const noexcept {
    return fuel || destruct || resources || crystals;
  }
};
```

### `TechDiscoveries` (`Race`)

```cpp
export struct TechDiscoveries {
  bool hyper_drive{false};    ///< Hyperdrive propulsion technology discovered
  bool laser{false};          ///< Combat laser weapon technology discovered
  bool cew{false};            ///< Concentrated Energy Weapon technology discovered
  bool cloak{false};          ///< Cloaking device technology discovered
  bool terraform{false};      ///< Planetary terraforming technology discovered
};
```

## Anti-Patterns

- ❌ Calling raw `isset()` / `setbit()` / `clrbit()` on entity member fields directly outside the entity's own member implementation.
- ❌ Reintroducing stored boolean flags for conditions that can be computed from primary entity state.
- ❌ Using integer masks with bit-shifts (`1 << X`) when a structured boolean manifest (`CommodityManifest`) exists.
