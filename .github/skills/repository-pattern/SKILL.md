---
name: repository-pattern
description: 'Implement or extend a repository in dallib/gblib for an entity persisted as JSON in SQLite. Use when adding a new entity type to the database, adding fields that need to round-trip through JSON, or wiring a new repository into EntityManager. Covers Glaze meta specialization, JsonStore CRUD, composite-key repositories, and gap-finding ID allocation.'
user-invocable: false
---

# Repository Pattern

Each entity type has a repository that owns its serialization, table layout, and CRUD. Repositories live below the service layer: they know how a `Race`/`Ship`/`Planet` maps to storage but do not know game rules.

All entity tables follow the same shape: `id INTEGER PRIMARY KEY` plus a `data TEXT` column holding the Glaze JSON encoding of the struct.

## Anatomy of a Repository

```cpp
export class FooRepository {
public:
  explicit FooRepository(JsonStore& store) : store_(store) {}

  void save(const Foo& foo) {
    store_.upsert("tbl_foo", foo.id, glz::write_json(foo).value());
  }

  std::optional<Foo> find(int id) {
    auto row = store_.find("tbl_foo", id);
    if (!row) return std::nullopt;
    Foo out{};
    if (glz::read_json(out, *row)) return std::nullopt;  // parse failure
    return out;
  }

  std::vector<Foo> all() { /* iterate via store_.scan(...) */ }

private:
  JsonStore& store_;
};
```

The repository never opens connections, manages transactions, or executes raw SQL. `JsonStore` (in `dallib`) owns those.

## Glaze Meta Specialization

Every persisted struct needs a `glz::meta` specialization that lists each serialized field by name. Strings drive the JSON shape:

```cpp
namespace glz {
template <>
struct meta<Foo> {
  using T = Foo;
  static constexpr auto value = object(
    "id",    &T::id,
    "name",  &T::name,
    "value", &T::value
  );
};
}  // namespace glz
```

Adding a field requires updating both the struct and this meta. See the add-field-to-serialized-entity skill for the full playbook.

Strong ID types (`player_t`, `governor_t`) already serialize as plain integers through Glaze — no extra meta needed.

## Composite Keys

Planets, sectors, and similar entities are keyed by two columns. Use the existing `JsonStore` composite-key helpers:

```cpp
// PlanetRepository keys on (star_id, planet_order)
void save(const Planet& p) {
  store_.upsert_composite("tbl_planet", p.star_id, p.planet_order,
                          glz::write_json(p).value());
}

std::optional<Planet> find(starnum_t star, planetnum_t order) {
  auto row = store_.find_composite("tbl_planet", star, order);
  /* deserialize ... */
}
```

Mirror an existing composite repository (`PlanetRepository`, `SectorRepository`) — don't invent a new layout.

## Gap-Finding ID Allocation

`ShipRepository` (and similar) allocate dense IDs by scanning for the first unused row. New repositories that need a similar guarantee should call into the existing `JsonStore` helper rather than reimplementing the scan.

## Schema Registration

New tables must be created by `initialize_schema()` in `gb/dal/Schema.cppm`. Add a `CREATE TABLE IF NOT EXISTS ...` for every new table, plus any indexes:

```cpp
db.exec(R"sql(
  CREATE TABLE IF NOT EXISTS tbl_foo (
    id   INTEGER PRIMARY KEY,
    data TEXT NOT NULL
  );
)sql");
db.exec("CREATE INDEX IF NOT EXISTS idx_foo_xxx ON tbl_foo(json_extract(data, '$.xxx'));");
```

Tests using `Database(":memory:") + initialize_schema(...)` will then pick up the new table automatically.

## Wiring Into EntityManager

If the entity is game-facing (loaded/cached/iterated), the repository becomes a private member of `EntityManager` and exposes `peek_*` / `get_*` methods. If it is a write-once log (telegrams, news), it can be a private member with thin service methods (`post_*`, `delete_*`) and no caching. Match the closest existing entity for guidance.

The application/command layer must always go through `EntityManager`, never the repository directly.

## Anti-Patterns

- ❌ Game logic (validation, cost calculations, side effects) inside a repository.
- ❌ Raw `sqlite3_*` calls inside a repository — that belongs in `dallib`.
- ❌ Forgetting `glz::meta` for a new struct (silent zero-fields-roundtripped JSON).
- ❌ Using strings/`int`s for IDs in a meta when the struct uses strong IDs (just point at the strong-ID field).
- ❌ Adding a table without registering it in `initialize_schema`.
- ❌ Bypassing `EntityManager` and calling repositories from commands.

## Checklist

- [ ] Struct definition in the appropriate `gblib-*.cppm`
- [ ] `glz::meta<Foo>` specialization listing every persisted field
- [ ] Repository in `gb/repositories/` exposing `save` / `find` / `all` (and any composite-key methods)
- [ ] `CREATE TABLE` registered in `gb/dal/Schema.cppm::initialize_schema`
- [ ] `EntityManager` exposes `peek_foo` / `get_foo` (or service methods for non-cached entities)
- [ ] Round-trip test: save, `clear_cache()`, peek — fields match
- [ ] No raw SQL or game logic leaked into the repository
