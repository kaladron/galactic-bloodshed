---
name: add-field-to-serialized-entity
description: 'Add a new field to a persisted entity (Race, Ship, Planet, Sector, ServerState, etc.). Use when extending a struct that is stored as JSON in SQLite, adding a flag/counter/name to an existing entity, or migrating a feature from filesystem state to database state. Covers struct edit, Glaze meta update, default values, optional schema/index changes, and updating call sites and tests.'
user-invocable: false
---

# Add a Field to a Serialized Entity

Persisted entities round-trip through Glaze JSON in `tbl_<entity>`. Adding a field is mechanical, but every step matters — miss the meta and the field silently does not persist; miss the default and existing rows deserialize to whatever uninitialized memory had.

## Steps

### 1. Add the field to the struct

In the appropriate `gblib-*.cppm` (or `types.cppm`) — e.g. `gblib-race.cppm`, `gblib-ships.cppm`, `gblib-types.cppm` for `ServerState`:

```cpp
export struct ServerState {
  int  id{1};
  // ... existing fields ...
  bool nogo{false};   // NEW
};
```

Always use a default initializer (`{false}`, `{0}`, `{}`). Glaze deserializes existing rows that lack the new key as the default; without one the field is undefined.

### 2. Update the Glaze meta

Find `glz::meta<EntityType>` (typically in `gb/repositories/gblib-repositories.cppm` or beside the struct). Add the new field to the `object(...)` list:

```cpp
namespace glz {
template <>
struct meta<ServerState> {
  using T = ServerState;
  static constexpr auto value = object(
    "id",     &T::id,
    // ... existing entries ...
    "nogo",   &T::nogo
  );
};
}  // namespace glz
```

If a `glz::meta` does not exist for the struct, create one — without it the JSON serialization is empty.

For strong-ID fields (`player_t`, `governor_t`), no special handling is needed; they already serialize as integers.

### 3. (Optional) Add or change a SQL index

If the new field is queried by the application, add an `IF NOT EXISTS` index in `gb/dal/Schema.cppm::initialize_schema`:

```cpp
db.exec("CREATE INDEX IF NOT EXISTS idx_xxx_nogo "
        "ON tbl_xxx(json_extract(data, '$.nogo'));");
```

Most flag-style fields do not need an index.

### 4. Update writers / readers

Replace any old source-of-truth (file flag, hardcoded constant, removed field) with the new field. For migrations from filesystem state to a DB field, this is where you swap `stat(...)` checks for `state.nogo` reads.

Use the standard EntityManager pattern:

```cpp
auto state_handle = em.get_server_state();
auto& state = *state_handle;
state.nogo = true;
```

Read-only checks should use `peek_*`.

### 5. Initialize on universe creation

`makeuniv` / `enrol` populate fresh databases. If the field needs a non-default value at world birth, set it there. Otherwise rely on the struct default — that is also what existing JSON rows decode to.

### 6. Update every call site

Use the clang-query-refactoring skill or grep to find every site that read or wrote the **old** mechanism (filesystem flag, removed function, etc.). Replace them all in one change set so no path is left on the old plumbing.

### 7. Add tests

In a `*_test.cc` exercising round-trip:

```cpp
{
  auto h = em.get_server_state();
  h->nogo = true;
}
em.clear_cache();                              // drop the cache
const auto* s = em.peek_server_state();
assert(s->nogo == true);                       // came back from JSON
```

See the database-test-pattern skill.

### 8. Build and run the suite

```bash
ninja -C build
(cd build && ctest)
```

A passing suite plus a manual round-trip assertion confirms the field persists.

## Anti-Patterns

- ❌ Adding the field to the struct but forgetting `glz::meta` — silent data loss on save.
- ❌ Omitting a default initializer — undefined value on existing rows.
- ❌ Renaming a meta key (`"old_name"` → `"new_name"`) without a migration — every existing row decodes the field to the default.
- ❌ Reading the new field from a stale cached entity in tests — call `em.clear_cache()` first.
- ❌ Leaving the old source of truth (file flag, deprecated field) in place — both will drift.

## Checklist

- [ ] Field added to the struct with a sensible default
- [ ] `glz::meta` updated with the new key
- [ ] Schema unchanged, or new index registered in `initialize_schema`
- [ ] All readers/writers migrated to the new field
- [ ] Universe creation sets a non-default initial value if needed
- [ ] Round-trip test (save → `clear_cache` → peek) added or extended
- [ ] `ninja -C build` and `ctest` both green
