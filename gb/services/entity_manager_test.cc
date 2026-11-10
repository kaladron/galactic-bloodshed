// SPDX-License-Identifier: Apache-2.0

/// \file entity_manager_test.cc
/// \brief Unit tests for EntityManager lifecycle, caching, composite keys, and
/// CRUD operations.

import dallib;
import gb.entities;
import gb.services;
import test;
import std;

void test_entity_manager_basic() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: EntityManager basic functionality");

  Race race{};
  race.Playernum = 1;
  race.name = "Test Race";
  race.tech = 50.0;
  JsonStore store(db);
  RaceRepository races(store);
  races.save(race);

  em.mutate_race(1, [](Race& r) {
    test::expect_eq(r.name, "Test Race");
    test::expect_eq(r.tech, 50.0);
    r.tech = 75.0;
    r.name = "Updated Race";
  });

  const auto* updated_race = em.peek_race(1);
  test::expect_ne(updated_race, nullptr);
  test::expect_eq(updated_race->tech, 75.0);
  test::expect_eq(updated_race->name, "Updated Race");

  std::println(std::cout, "  ✓ Basic mutate/auto-save works");
}

void test_entity_manager_caching() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: EntityManager caching");

  Race race{};
  race.Playernum = 1;
  race.name = "Cache Race";
  JsonStore store(db);
  RaceRepository races(store);
  races.save(race);

  const Race* first_ptr = em.peek_race(1);
  const Race* second_ptr = em.peek_race(1);
  test::expect_eq(first_ptr, second_ptr);
  std::println(std::cout,
               "  ✓ Multiple peek calls return same cached instance");

  em.mutate_race(1, [](Race& r) { r.name = "Modified in Mutate"; });
  test::expect_eq(first_ptr->name, "Modified in Mutate");
  std::println(std::cout,
               "  ✓ Modifications visible across all peeks (same instance)");

  em.clear_cache();
  const auto* third_ptr = em.peek_race(1);
  test::expect_eq(third_ptr->name, "Modified in Mutate");
  std::println(std::cout, "  ✓ Entity persists after cache clear");
}

void test_entity_manager_composite_keys() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: EntityManager composite keys (Planet)");

  Planet planet{1, 2};
  planet.popn() = 10000;
  JsonStore store(db);
  StarRepository(store).save(Star{1, "Sol"});
  PlanetRepository planets(store);
  planets.save(planet);

  const auto* p1 = em.peek_planet(1, 2);
  const auto* p2 = em.peek_planet(1, 2);
  test::expect_eq(p1, p2);
  std::println(std::cout,
               "  ✓ Multiple peeks to same planet return identical instance");

  em.mutate_planet(1, 2, [](Planet& p) { p.popn() = 20000; });

  const auto* peek = em.peek_planet(1, 2);
  test::expect_ne(peek, nullptr);
  test::expect_eq(peek->popn(), 20000);
  std::println(std::cout, "  ✓ Composite keys work for Planet entities");

  test::expect_throws<EntityNotFoundError>([&]() { em.peek_planet(999, 999); });
  test::expect_throws<EntityNotFoundError>(
      [&]() { em.mutate_planet(999, 999, [](Planet&) {}); });
  std::println(
      std::cout,
      "  ✓ peek_planet and mutate_planet throw EntityNotFoundError for "
      "missing planet");
}

void test_entity_manager_create_delete() {
  TestContext ctx;
  EntityManager& em = ctx.em;

  std::println(std::cout, "Test: EntityManager create/delete");

  shipnum_t ship_num =
      TestShipBuilder(em, ShipType::STYPE_SHUTTLE).with_fuel(100.0).build();
  std::println(std::cout, "  ✓ create_ship() creates and saves new ship");

  const auto* peek = em.peek_ship(ship_num);
  test::expect_ne(peek, nullptr);
  test::expect_eq(peek->fuel(), 100.0);

  // Calling delete_ship without barrier throws DeletionBarrierRequiredError
  test::expect_throws<DeletionBarrierRequiredError>(
      [&]() { em.delete_ship(ship_num); });
  std::println(
      std::cout,
      "  ✓ delete_ship() throws DeletionBarrierRequiredError without barrier");

  {
    auto barrier = em.create_deletion_barrier();
    em.delete_ship(ship_num);
    // Ship is still cached and peekable during barrier scope
    test::expect_ne(em.peek_ship(ship_num), nullptr);
  }
  // After barrier exit, ship is deleted
  test::expect_throws<EntityNotFoundError>([&]() { em.peek_ship(ship_num); });
  std::println(std::cout, "  ✓ delete_ship() with DeletionBarrier removes ship "
                          "from cache and database");
}

void test_entity_manager_read_only_access() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: EntityManager read-only access");

  Race race{};
  race.Playernum = 1;
  race.name = "ReadOnly Race";
  JsonStore store(db);
  RaceRepository races(store);
  races.save(race);

  const auto* peek_race = em.peek_race(1);
  test::expect_ne(peek_race, nullptr);
  test::expect_eq(peek_race->name, "ReadOnly Race");
  std::println(std::cout,
               "  ✓ peek_race() provides read-only access to cached entity");

  test::expect_throws<EntityNotFoundError>([&]() { em.peek_race(999); });
  std::println(
      std::cout,
      "  ✓ peek_*() throws EntityNotFoundError for non-existent entities");

  test::expect_throws<EntityNotFoundError>(
      [&]() { em.mutate_race(999, [](Race&) {}); });
  std::println(
      std::cout,
      "  ✓ mutate_race() throws EntityNotFoundError for non-existent entities");
}

void test_entity_manager_get_ship_throws() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(
      std::cout,
      "Test: EntityManager mutate_ship() throwing on non-existent ship");

  test::expect_throws<EntityNotFoundError>(
      [&]() { em.mutate_ship(999, [](Ship&) {}); });
  std::println(
      std::cout,
      "  ✓ mutate_ship() throws EntityNotFoundError for non-existent ships");

  test::expect_throws<EntityNotFoundError>([&]() { em.peek_ship(999); });
  std::println(
      std::cout,
      "  ✓ peek_ship() throws EntityNotFoundError for non-existent ships");
}

void test_entity_manager_flush_all() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: EntityManager flush_all");

  Race race{};
  race.Playernum = 1;
  race.name = "Flush Race";
  JsonStore store(db);
  RaceRepository races(store);
  races.save(race);

  em.mutate_race(1, [](Race& r) { r.tech = 100.0; });

  em.flush_all();

  auto direct_read = races.find_by_player(1);
  test::expect_true(direct_read.has_value());
  test::expect_eq(direct_read->tech, 100.0);
  std::println(std::cout,
               "  ✓ flush_all() saves all cached entities immediately");
}

void test_entity_manager_clear_cache() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: EntityManager clear_cache");

  Race race{};
  race.Playernum = 1;
  race.name = "ClearCache Race";
  JsonStore store(db);
  RaceRepository races(store);
  races.save(race);

  em.mutate_race(1, [](Race& r) { r.tech = 100.0; });

  em.clear_cache();

  const auto* peek = em.peek_race(1);
  test::expect_ne(peek, nullptr);
  test::expect_eq(peek->tech, 100.0);
  std::println(std::cout, "  ✓ peek_* reloads from database after cache clear");

  em.mutate_race(1, [](Race& r) { r.name = "Updated After Clear"; });

  const auto* peek_after = em.peek_race(1);
  test::expect_ne(peek_after, nullptr);
  test::expect_eq(peek_after->name, "Updated After Clear");
  std::println(std::cout, "  ✓ Entities can be reloaded after cache clear");
}

void test_entity_manager_singleton_universe() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: EntityManager singleton (Universe)");

  test::expect_throws<EntityNotFoundError>([&]() { em.peek_universe(); });
  test::expect_throws<EntityNotFoundError>(
      [&]() { em.mutate_universe([](Universe&) {}); });
  std::println(
      std::cout,
      "  ✓ peek_universe and mutate_universe throw EntityNotFoundError "
      "when uninitialized");

  Universe univ{};
  univ.set_AP(1, 50);
  JsonStore store(db);
  UniverseRepository univ_repo(store);
  univ_repo.save(univ);

  const auto* u1 = em.peek_universe();
  const auto* u2 = em.peek_universe();
  test::expect_eq(u1, u2);
  test::expect_eq(u1->get_AP(1), 50);

  em.mutate_universe([](Universe& u) { u.set_AP(1, 75); });

  const auto* peek = em.peek_universe();
  test::expect_ne(peek, nullptr);
  test::expect_eq(peek->get_AP(1), 75);
  std::println(std::cout, "  ✓ Singleton Universe works correctly");
}

void test_entity_manager_singleton_server_state() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: EntityManager singleton (ServerState)");

  ServerState state{};
  state.segments = 10;
  state.nsegments_done = 3;
  JsonStore store(db);
  ServerStateRepository server_repo(store);
  server_repo.save(state);

  const auto* s1 = em.peek_server_state();
  const auto* s2 = em.peek_server_state();
  test::expect_eq(s1, s2);
  test::expect_eq(s1->segments, 10);

  em.mutate_server_state([](ServerState& s) { s.segments = 15; });

  const auto* peek = em.peek_server_state();
  test::expect_ne(peek, nullptr);
  test::expect_eq(peek->segments, 15);
  std::println(std::cout, "  ✓ Singleton ServerState works correctly");
}

void test_entity_manager_get_player() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: EntityManager find_player_by_name()");

  Race race1{};
  race1.Playernum = 1;
  race1.name = "Federation";

  Race race2{};
  race2.Playernum = 2;
  race2.name = "Empire";

  JsonStore store(db);
  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  auto p1 = em.find_player_by_name("Federation");
  test::expect_true(p1.has_value() && *p1 == 1);
  auto p2 = em.find_player_by_name("Empire");
  test::expect_true(p2.has_value() && *p2 == 2);
  std::println(std::cout, "  ✓ find_player_by_name finds race by name");

  auto p1_by_num = em.find_player_by_name("1");
  test::expect_true(p1_by_num.has_value() && *p1_by_num == 1);
  auto p2_by_num = em.find_player_by_name("2");
  test::expect_true(p2_by_num.has_value() && *p2_by_num == 2);
  std::println(std::cout,
               "  ✓ find_player_by_name finds race by number string");

  test::expect_false(em.find_player_by_name("").has_value());
  std::println(std::cout,
               "  ✓ find_player_by_name returns nullopt for empty string");

  test::expect_false(em.find_player_by_name("Unknown").has_value());
  std::println(std::cout,
               "  ✓ find_player_by_name returns nullopt for non-existent race");

  test::expect_false(em.find_player_by_name("999").has_value());
  std::println(
      std::cout,
      "  ✓ find_player_by_name returns nullopt for out-of-range number");

  test::expect_false(em.find_player_by_name("0").has_value());
  test::expect_false(em.find_player_by_name("-1").has_value());
  std::println(
      std::cout,
      "  ✓ find_player_by_name returns nullopt for invalid player number");

  // Sparse player IDs (Race 4 exists, Race 3 does not; total race count == 3)
  Race race4{};
  race4.Playernum = 4;
  race4.name = "Klingon";
  races.save(race4);

  auto p4_by_name = em.find_player_by_name("Klingon");
  test::expect_true(p4_by_name.has_value() && *p4_by_name == 4);
  auto p4_by_num = em.find_player_by_name("4");
  test::expect_true(p4_by_num.has_value() && *p4_by_num == 4);
  test::expect_eq(get_player(em, "4"), player_t{4});
  test::expect_false(em.find_player_by_name("3").has_value());
  test::expect_eq(get_player(em, "3"), player_t{0});
  std::println(std::cout,
               "  ✓ find_player_by_name and get_player handle sparse player "
               "IDs and reject missing gap IDs");
}

void test_entity_manager_destroy_ship() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: EntityManager destroy_ship()");

  Race owner{};
  owner.Playernum = 1;
  owner.name = "Owner";
  owner.morale = 1000;

  JsonStore store(db);
  RaceRepository(store).save(owner);
  ShipRepository ships(store);

  ship_struct fighter_data{
      .number = 100,
      .owner = 1,
      .build_cost = 50,
      .type = ShipType::STYPE_FIGHTER,
      .alive = 1,
  };
  Ship fighter(fighter_data);
  ships.save(fighter);

  em.begin_mutation_tracking();
  em.destroy_ship(fighter);
  DirtyEntitySet dirty = em.take_dirty_entities();
  test::expect_eq(fighter.alive(), 0);
  test::expect_eq(fighter.notified(), 0);
  test::expect_true(dirty.races.empty());
  test::expect_eq(em.peek_race(1)->morale, 1000);

  // Gov_ship FK cleared across habitat, pod, and factory government ships
  for (auto [ship_id, ship_type] :
       {std::pair{shipnum_t{101}, ShipType::STYPE_POD},
        std::pair{shipnum_t{102}, ShipType::OTYPE_FACTORY},
        std::pair{shipnum_t{103}, ShipType::STYPE_HABITAT}}) {
    Ship gov_ship(ship_struct{
        .number = ship_id, .owner = 1, .type = ship_type, .alive = 1});
    ships.save(gov_ship);
    em.mutate_race(1, [&](Race& r) { r.Gov_ship = ship_id; });
    em.destroy_ship(gov_ship);
    em.clear_cache();
    test::expect_eq(em.peek_race(1)->Gov_ship, std::nullopt);
  }
  std::println(std::cout,
               "  ✓ destroy_ship retires ships and clears Gov_ship across all "
               "ship types without altering morale");
}

void test_peek_star_throws_on_not_found() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(
      std::cout,
      "Test: peek_star and mutate_star throw EntityNotFoundError on not found");

  test::expect_throws<EntityNotFoundError>([&]() { em.peek_star(999); });
  test::expect_throws<EntityNotFoundError>(
      [&]() { em.mutate_star(999, [](Star&) {}); });
  std::println(
      std::cout,
      "  ✓ peek_star and mutate_star throw EntityNotFoundError for invalid "
      "star_id");
}

void test_peek_planet_throws_on_not_found() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout,
               "Test: peek_planet throws EntityNotFoundError on not found");

  test::expect_throws<EntityNotFoundError>([&]() { em.peek_planet(5, 3); });
  std::println(std::cout,
               "  ✓ peek_planet throws EntityNotFoundError for invalid planet");
}

void test_peek_sectormap_throws_on_not_found() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout,
               "Test: peek_sectormap throws EntityNotFoundError on not found");

  test::expect_throws<EntityNotFoundError>([&]() { em.peek_sectormap(10, 5); });
  std::println(
      std::cout,
      "  ✓ peek_sectormap throws EntityNotFoundError for invalid planet");
}

void test_peek_caching_and_clear_cache() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: peek caching and clear_cache eviction");

  JsonStore store(db);
  StarRepository stars(store);
  stars.save(Star{1, "TestStar"});

  {
    const auto* peek1 = em.peek_star(1);
    test::expect_ne(peek1, nullptr);
    test::expect_eq(peek1->get_name(), "TestStar");

    const auto* peek2 = em.peek_star(1);
    test::expect_eq(peek1, peek2);
    std::println(std::cout, "  ✓ Multiple peeks return same cached instance");
  }

  {
    Race race{};
    race.Playernum = 1;
    race.name = "TestRace";
    race.tech = 50.0;
    RaceRepository races(store);
    races.save(race);

    const auto* peek_race = em.peek_race(1);
    test::expect_ne(peek_race, nullptr);
    test::expect_eq(peek_race->name, "TestRace");

    // Modify in DB directly while peeked
    race.name = "ModifiedInDB";
    races.save(race);

    // Still returns cached value before clear_cache
    const auto* peek_cached = em.peek_race(1);
    test::expect_eq(peek_cached->name, "TestRace");

    // clear_cache must evict peeked entities (no refcount leak)
    em.clear_cache();

    // After clear_cache, peek_race reloads fresh data from DB
    const auto* peek_reloaded = em.peek_race(1);
    test::expect_ne(peek_reloaded, nullptr);
    test::expect_eq(peek_reloaded->name, "ModifiedInDB");
    std::println(
        std::cout,
        "  ✓ peek does not leak refcount; clear_cache evicts peeked entities");
  }

  {
    ship_struct ship_data{};
    ship_data.number = 100;
    ship_data.owner = 1;
    ship_data.fuel = 1000.0;
    ship_data.max_fuel = 5000.0;
    Ship ship(ship_data);
    ShipRepository ships(store);
    ships.save(ship);

    const auto* peek_ship = em.peek_ship(100);
    test::expect_ne(peek_ship, nullptr);
    test::expect_eq(peek_ship->fuel(), 1000.0);

    // Modify directly in DB
    ship.add_fuel(1500.0);
    ships.save(ship);

    em.clear_cache();

    const auto* peek_reloaded_ship = em.peek_ship(100);
    test::expect_ne(peek_reloaded_ship, nullptr);
    test::expect_eq(peek_reloaded_ship->fuel(), 2500.0);
    std::println(std::cout,
                 "  ✓ clear_cache successfully evicts peeked ship entities");
  }

  std::println(std::cout, "  ✅ All peek caching and eviction tests passed");
}

void test_entity_manager_commods() {
  TestContext ctx;
  ctx.with_standard_universe();
  Database& db = ctx.db;
  EntityManager& em = ctx.em;

  std::println(std::cout, "Test: EntityManager commod management");

  JsonStore store(db);
  CommodRepository repo(store);

  Commod c{{
      .id = 1,
      .owner = 2,
      .governor = 1,
      .type = CommodType::FUEL,
      .amount = 500,
      .bid = 100,
      .bidder = 1,
      .star_from = 1,
      .planet_from = 1,
      .star_to = 1,
      .planet_to = 1,
  }};
  repo.save(c);

  const auto* peek = em.peek_commod(1);
  test::expect_ne(peek, nullptr);
  test::expect_eq(peek->amount(), 500);
  test::expect_eq(peek->bid(), 100);
  std::println(std::cout, "  ✓ peek_commod works");

  auto amount_read =
      em.with_commod(1, [](const Commod& cmd) { return cmd.amount(); });
  test::expect_eq(amount_read, 500);
  std::println(std::cout, "  ✓ with_commod works");

  em.mutate_commod(1, [](Commod& cmd) {
    cmd.set_amount(800);
    cmd.set_bid(150);
  });

  auto updated = repo.find_by_id(1);
  test::expect_true(updated.has_value());
  test::expect_eq(updated->amount(), 800);
  test::expect_eq(updated->bid(), 150);
  std::println(std::cout, "  ✓ mutate_commod auto-save persisted changes");

  test::expect_throws<EntityNotFoundError>([&]() { em.peek_commod(999); });
  test::expect_throws<EntityNotFoundError>(
      [&]() { em.mutate_commod(999, [](Commod&) {}); });
  std::println(
      std::cout,
      "  ✓ peek_commod and mutate_commod throw EntityNotFoundError for "
      "missing commod");
}

void test_entity_manager_blocks() {
  TestContext ctx;
  Database& db = ctx.db;
  EntityManager& em = ctx.em;
  em.clear_cache();

  std::println(std::cout, "Test: EntityManager block management");

  JsonStore store(db);
  BlockRepository repo(store);

  Block b{{
      .Playernum = 1,
      .name = "Test Alliance",
      .VPs = 500,
  }};
  repo.save(b);

  const auto* peek = em.peek_block(1);
  test::expect_ne(peek, nullptr);
  test::expect_eq(peek->name(), "Test Alliance");
  test::expect_eq(peek->VPs(), 500);
  std::println(std::cout, "  ✓ peek_block works");

  auto vps_read = em.with_block(1, [](const Block& blk) { return blk.VPs(); });
  test::expect_eq(vps_read, 500);
  std::println(std::cout, "  ✓ with_block works");

  em.mutate_block(1, [](Block& blk) { blk.VPs() = 1000; });

  auto updated = repo.find_by_id(1);
  test::expect_true(updated.has_value());
  test::expect_eq(updated->VPs(), 1000);
  std::println(std::cout, "  ✓ mutate_block auto-save persisted changes");

  test::expect_throws<EntityNotFoundError>([&]() { em.peek_block(999); });
  test::expect_throws<EntityNotFoundError>(
      [&]() { em.mutate_block(999, [](Block&) {}); });
  std::println(std::cout,
               "  ✓ peek_block and mutate_block throw EntityNotFoundError for "
               "missing block");
}

void test_entity_manager_powers() {
  TestContext ctx;
  Database& db = ctx.db;
  EntityManager& em = ctx.em;
  em.clear_cache();

  std::println(std::cout, "Test: EntityManager power management");

  JsonStore store(db);
  PowerRepository repo(store);

  Power p{{
      .id = 1,
      .troops = 1000,
      .popn = 50000,
  }};
  repo.save(p);

  const auto* peek = em.peek_power(1);
  test::expect_ne(peek, nullptr);
  test::expect_eq(peek->troops(), 1000);
  test::expect_eq(peek->popn(), 50000);
  std::println(std::cout, "  ✓ peek_power works");

  auto troops_read =
      em.with_power(1, [](const Power& pwr) { return pwr.troops(); });
  test::expect_eq(troops_read, 1000);
  std::println(std::cout, "  ✓ with_power works");

  em.mutate_power(1, [](Power& pwr) { pwr.troops() = 2500; });

  auto updated = repo.find_by_id(1);
  test::expect_true(updated.has_value());
  test::expect_eq(updated->troops(), 2500);
  std::println(std::cout, "  ✓ mutate_power auto-save persisted changes");

  test::expect_throws<EntityNotFoundError>([&]() { em.peek_power(999); });
  test::expect_throws<EntityNotFoundError>(
      [&]() { em.mutate_power(999, [](Power&) {}); });
  std::println(std::cout,
               "  ✓ peek_power and mutate_power throw EntityNotFoundError for "
               "missing power");
}

void test_entity_manager_create_ship() {
  TestContext ctx;
  ctx.with_standard_universe();
  EntityManager& em = ctx.em;

  std::println(std::cout, "Test: EntityManager create_ship");

  shipnum_t new_ship_num;
  {
    auto new_ship_handle = TestShipBuilder(em, ShipType::OTYPE_PROBE)
                               .owned_by(1)
                               .named("Discovery")
                               .with_fuel(100.0)
                               .build_handle();
    test::expect_ne(new_ship_handle.get(), nullptr);
    new_ship_num = new_ship_handle->number();
    test::expect_gt(new_ship_num, 0);
    test::expect_eq(new_ship_handle->name(), "Discovery");
    test::expect_eq(new_ship_handle->fuel(), 100.0);
  }

  const auto* peek = em.peek_ship(new_ship_num);
  test::expect_ne(peek, nullptr);
  test::expect_eq(peek->name(), "Discovery");
  test::expect_eq(peek->owner(), 1);
  test::expect_eq(peek->fuel(), 100.0);

  // Test create_ship from template
  shipnum_t cargo_num;
  {
    auto cargo_handle = em.create_ship(ShipType::STYPE_CARGO, player_t{2});
    test::expect_ne(cargo_handle.get(), nullptr);
    cargo_num = cargo_handle->number();
    test::expect_gt(cargo_num, new_ship_num);
    test::expect_eq(cargo_handle->owner(), 2);
    test::expect_eq(cargo_handle->name(), "Cargo Ship");
    test::expect_eq(cargo_handle->max_resource(), 1000);
    test::expect_true(cargo_handle->alive());
  }

  const auto* peek_cargo = em.peek_ship(cargo_num);
  test::expect_ne(peek_cargo, nullptr);
  test::expect_eq(peek_cargo->owner(), 2);
  test::expect_eq(peek_cargo->type(), ShipType::STYPE_CARGO);
  test::expect_eq(peek_cargo->max_resource(), 1000);

  // Test create_ship from std::unique_ptr<Ship>
  shipnum_t battle_num;
  {
    auto ship_ptr =
        ShipFactory::create_from_template(ShipType::STYPE_BATTLE, player_t{3});
    auto battle_handle = em.create_ship(std::move(ship_ptr));
    test::expect_ne(battle_handle.get(), nullptr);
    battle_num = battle_handle->number();
    test::expect_gt(battle_num, cargo_num);
    test::expect_eq(battle_handle->owner(), 3);
    test::expect_eq(battle_handle->type(), ShipType::STYPE_BATTLE);
  }

  const auto* peek_battle = em.peek_ship(battle_num);
  test::expect_ne(peek_battle, nullptr);
  test::expect_eq(peek_battle->owner(), 3);
  test::expect_eq(peek_battle->type(), ShipType::STYPE_BATTLE);

  // Test create_ship with nullptr throws
  test::expect_throws<std::invalid_argument>(
      [&]() { em.create_ship(std::unique_ptr<Ship>{nullptr}); });

  std::println(std::cout,
               "  ✓ create_ship allocated ID and persisted successfully");
}

void test_entity_manager_with_scoped_peeks() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  std::println(std::cout, "Test: EntityManager with_* scoped peek helpers");

  // 1. with_race
  Race race{};
  race.Playernum = 1;
  race.name = "MonadicEmpire";
  race.morale = 42;
  RaceRepository races(store);
  races.save(race);

  auto race_name = em.with_race(1, [](const Race& r) { return r.name; });
  test::expect_eq(race_name, "MonadicEmpire");

  int morale = em.with_race(1, [](const Race& r) { return r.morale; });
  test::expect_eq(morale, 42);

  // 2. with_star
  StarRepository stars(store);
  stars.save(Star{1, "AlphaCentauri"});

  auto star_name = em.with_star(1, [](const Star& s) { return s.get_name(); });
  test::expect_eq(star_name, "AlphaCentauri");

  // 3. with_planet
  Planet p{1, 1, PlanetType::EARTH, Coordinates{5, 5}};
  p.popn() = 5000;
  PlanetRepository planets(store);
  planets.save(p);

  auto popn = em.with_planet(1, 1, [](const Planet& pl) { return pl.popn(); });
  test::expect_eq(popn, 5000);

  // 4. with_ship
  ship_struct raw_ship{};
  raw_ship.number = 77;
  raw_ship.owner = 1;
  raw_ship.name = "Voyager";
  raw_ship.fuel = 300.0;
  Ship ship(raw_ship);
  ShipRepository ship_repo(store);
  ship_repo.save(ship);

  auto ship_fuel = em.with_ship(77, [](const Ship& s) { return s.fuel(); });
  test::expect_eq(ship_fuel, 300.0);

  // 5. with_sectormap
  SectorMap smap(p);
  smap.get(Coordinates{2, 3}).set_owner(player_t{1});
  SectorRepository sectors(store);
  sectors.save_map(smap);

  auto sect_owner = em.with_sectormap(1, 1, [](const SectorMap& map) {
    return map.get(Coordinates{2, 3}).get_owner();
  });
  test::expect_eq(sect_owner, player_t{1});

  // 6. with_universe
  Universe u{};
  u.set_AP(1, 10);
  UniverseRepository u_repo(store);
  u_repo.save(u);

  auto univ_ap =
      em.with_universe([](const Universe& univ) { return univ.get_AP(1); });
  test::expect_eq(univ_ap, 10);

  // 7. with_server_state
  ServerState state{};
  state.welcome_message = "Welcome to GB!";
  ServerStateRepository state_repo(store);
  state_repo.save(state);

  auto msg = em.with_server_state(
      [](const ServerState& st) { return st.welcome_message; });
  test::expect_eq(msg, "Welcome to GB!");

  // 8. with_ship_exam (pure read-only default synthesis, zero DB writes)
  const auto before_exam = db.total_changes();
  auto exam_name = em.with_ship_exam(
      ShipType::OTYPE_PROBE, [](const ShipExam& ex) { return ex.name; });
  test::expect_eq(exam_name, "Space Probe");
  test::expect_eq(db.total_changes(), before_exam);

  // 9. with_planet_and_sectors
  bool paired_check = em.with_planet_and_sectors(
      1, 1, [](const Planet& pl, const SectorMap& map) {
        return pl.popn() == 5000 &&
               map.get(Coordinates{2, 3}).get_owner() == player_t{1};
      });
  test::expect_true(paired_check);

  std::println(std::cout, "  ✓ All with_* scoped peek helpers passed");
}

void test_entity_manager_mutate_planet_and_sectors() {
  TestContext ctx;
  ctx.with_standard_universe();

  std::println(std::cout, "Test: EntityManager mutate_planet_and_sectors");

  ctx.em.mutate_planet_and_sectors(1, 1, [](Planet& pl, SectorMap& map) {
    pl.popn() = 2500;
    map.get(Coordinates{1, 1}).set_owner(player_t{2});
  });

  // Verify persistence beyond cache
  ctx.em.clear_cache();

  test::expect_eq(ctx.em.peek_planet(1, 1)->popn(), 2500);
  test::expect_eq(
      ctx.em.peek_sectormap(1, 1)->get(Coordinates{1, 1}).get_owner(),
      player_t{2});

  // Verify throws on non-existent planet or sectors
  test::expect_throws<EntityNotFoundError>([&]() {
    ctx.em.mutate_planet_and_sectors(999, 999, [](Planet&, SectorMap&) {});
  });
  test::expect_throws<EntityNotFoundError>([&]() {
    ctx.em.with_planet_and_sectors(
        999, 999, [](const Planet&, const SectorMap&) { return 0; });
  });

  std::println(
      std::cout,
      "  ✓ mutate_planet_and_sectors atomic coordination and auto-save passed");
}

void test_deletion_barrier() {
  TestContext ctx;
  ctx.with_standard_universe();
  EntityManager& em = ctx.em;

  std::println(std::cout, "Test: EntityManager DeletionBarrier");

  // 1. Mandatory barrier checks
  test::expect_false(em.is_deletion_barrier_active());
  test::expect_throws<DeletionBarrierRequiredError>(
      [&]() { em.delete_ship(1); });
  test::expect_throws<DeletionBarrierRequiredError>(
      [&]() { em.delete_commod(1); });
  std::println(std::cout, "  ✓ delete_ship() and delete_commod() throw "
                          "DeletionBarrierRequiredError "
                          "when inactive");

  // 2. Anti-recursion / non-nesting
  {
    auto barrier = em.create_deletion_barrier();
    test::expect_true(em.is_deletion_barrier_active());
    test::expect_throws<std::logic_error>(
        [&]() { auto nested = em.create_deletion_barrier(); });
    std::println(std::cout,
                 "  ✓ creating nested DeletionBarrier throws std::logic_error");
  }
  test::expect_false(em.is_deletion_barrier_active());

  // 3. Deferred ship deletion and pointer stability
  shipnum_t ship_id =
      TestShipBuilder(em, ShipType::STYPE_SHUTTLE).with_fuel(50.0).build();
  {
    auto barrier = em.create_deletion_barrier();
    const auto* peek1 = em.peek_ship(ship_id);
    test::expect_ne(peek1, nullptr);
    test::expect_eq(peek1->fuel(), 50.0);

    em.delete_ship(ship_id);

    // Still accessible and pointer is stable during barrier
    const auto* peek2 = em.peek_ship(ship_id);
    test::expect_eq(peek1, peek2);
  }
  // Evicted from cache and database after barrier exits
  test::expect_throws<EntityNotFoundError>([&]() { em.peek_ship(ship_id); });
  std::println(
      std::cout,
      "  ✓ deferred ship deletion preserves cache and pointer stability until "
      "barrier exit");

  // 4. Deferred commodity deletion
  int commod_id;
  {
    auto new_commod = em.create_commod({
        .owner = 1,
        .governor = 1,
        .amount = 123,
        .star_from = 1,
        .planet_from = 1,
    });
    commod_id = new_commod->id();
  }
  {
    auto barrier = em.create_deletion_barrier();
    const auto* peek1 = em.peek_commod(commod_id);
    test::expect_ne(peek1, nullptr);
    test::expect_eq(peek1->amount(), 123);

    em.delete_commod(commod_id);

    const auto* peek2 = em.peek_commod(commod_id);
    test::expect_eq(peek1, peek2);
  }
  test::expect_throws<EntityNotFoundError>(
      [&]() { em.peek_commod(commod_id); });
  std::println(
      std::cout,
      "  ✓ deferred commod deletion preserves cache until barrier exit");

  // 5. Active handle at drain time throws EntityInUseError
  shipnum_t handle_ship_id;
  {
    auto new_ship = em.create_ship(ShipType::STYPE_SHUTTLE, player_t{1});
    handle_ship_id = new_ship->number();
  }
  em.mutate_ship(handle_ship_id, [&](Ship&) {
    // Inside mutate_ship callback, an EntityHandle is active on the stack
    test::expect_throws<EntityInUseError>([&]() {
      auto barrier = em.create_deletion_barrier();
      em.delete_ship(handle_ship_id);
      // Scope exit triggers ~DeletionBarrier -> refcount > 0 throws
      // EntityInUseError
    });
  });
  std::println(std::cout, "  ✓ DeletionBarrier drain throws EntityInUseError "
                          "if active handles exist");

  // 6. Safe inline deletion during ShipList iteration
  shipnum_t s1, s2, s3;
  {
    auto ship1 = em.create_ship(ShipType::STYPE_SHUTTLE, player_t{1});
    s1 = ship1->number();

    auto ship2 = em.create_ship(ShipType::STYPE_SHUTTLE, player_t{1});
    s2 = ship2->number();

    auto ship3 = em.create_ship(ShipType::STYPE_SHUTTLE, player_t{1});
    s3 = ship3->number();
  }
  {
    auto barrier = em.create_deletion_barrier();
    for (const Ship& s : ShipList::readonly(em, ShipList::IterationType::All)) {
      if (s.number() == s2) {
        em.delete_ship(s.number());
      }
    }
  }
  test::expect_ne(em.peek_ship(s1), nullptr);
  test::expect_throws<EntityNotFoundError>([&]() { em.peek_ship(s2); });
  test::expect_ne(em.peek_ship(s3), nullptr);
  std::println(std::cout, "  ✓ inline deletion during ShipList iteration "
                          "successfully cleans up ships");
}

void test_entity_manager_count_non_asteroid_planets() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);
  StarRepository stars(store);
  stars.save(Star{1, "Star1"});
  stars.save(Star{2, "Star2"});
  PlanetRepository planets(store);

  std::println(std::cout, "Test: EntityManager count_non_asteroid_planets");

  test::expect_eq(em.count_non_asteroid_planets(), 0);

  Planet earth{1, 1, PlanetType::EARTH};
  planets.save(earth);

  test::expect_eq(em.count_non_asteroid_planets(), 1);

  Planet asteroid{1, 2, PlanetType::ASTEROID};
  planets.save(asteroid);

  // Asteroid must not increment the non-asteroid count
  test::expect_eq(em.count_non_asteroid_planets(), 1);

  Planet gas_giant{2, 1, PlanetType::GASGIANT};
  planets.save(gas_giant);

  test::expect_eq(em.count_non_asteroid_planets(), 2);
  std::println(std::cout,
               "  ✓ count_non_asteroid_planets correctly excludes asteroids");
}

void test_entity_manager_create_race() {
  TestContext ctx;
  ctx.with_standard_universe();

  std::println(std::cout, "Test: EntityManager create_race auto-seeding");

  // Verify standard universe races (players 1..4) auto-seeded blocks & powers
  for (player_t pid : {player_t{1}, player_t{2}, player_t{3}, player_t{4}}) {
    const auto* r = ctx.em.peek_race(pid);
    test::expect_ne(r, nullptr);
    const auto* b = ctx.em.peek_block(pid);
    test::expect_ne(b, nullptr);
    test::expect_eq(b->Playernum(), pid);
    test::expect_eq(b->name(), r->name);
    const auto* p = ctx.em.peek_power(pid);
    test::expect_ne(p, nullptr);
    test::expect_eq(p->id(), pid);
  }

  // Create an additional race (Player 5)
  Race race{};
  race.name = "Martians";

  auto race_handle = ctx.em.create_race(race);
  player_t p5 = race_handle->Playernum;
  test::expect_eq(p5, 5);

  const auto* created_race = ctx.em.peek_race(p5);
  test::expect_ne(created_race, nullptr);
  test::expect_eq(created_race->name, "Martians");
  test::expect_eq(created_race->Playernum, p5);

  // Auto-seeded block and power checks in cache
  const auto* created_block = ctx.em.peek_block(p5);
  test::expect_ne(created_block, nullptr);
  test::expect_eq(created_block->Playernum(), p5);
  test::expect_eq(created_block->name(), "Martians");

  const auto* created_power = ctx.em.peek_power(p5);
  test::expect_ne(created_power, nullptr);
  test::expect_eq(created_power->id(), p5);

  // Persistence check across cache flush/clear
  ctx.em.clear_cache();

  const auto* persisted_race = ctx.em.peek_race(p5);
  test::expect_ne(persisted_race, nullptr);
  test::expect_eq(persisted_race->name, "Martians");

  const auto* persisted_block = ctx.em.peek_block(p5);
  test::expect_ne(persisted_block, nullptr);
  test::expect_eq(persisted_block->Playernum(), p5);
  test::expect_eq(persisted_block->name(), "Martians");

  const auto* persisted_power = ctx.em.peek_power(p5);
  test::expect_ne(persisted_power, nullptr);
  test::expect_eq(persisted_power->id(), p5);

  std::println(
      std::cout,
      "  ✓ create_race correctly creates and auto-seeds block and power");
}

void test_entity_manager_mutation_tracking() {
  std::println(std::cout,
               "Test: EntityManager mutation tracking and DirtyEntitySet");

  TestContext ctx;
  ctx.with_standard_universe();
  EntityManager& em = ctx.em;

  // 1. Initial state
  test::expect_false(em.is_tracking_mutations());
  em.begin_mutation_tracking();
  test::expect_true(em.is_tracking_mutations());

  // 2. Track mutations on various entities
  // Star
  em.mutate_star(1, [](Star& s) { s.set_name("Sol Prime"); });
  // Planet
  em.mutate_planet(1, 1, [](Planet& p) { p.popn() = 12345; });
  // SectorMap with dirty changes
  em.mutate_sectormap(1, 1, [](SectorMap& sm) {
    sm.get(Coordinates{0, 0}).set_popn_exact(999);
    sm.get(Coordinates{1, 0}).set_popn_exact(888);
  });
  // SectorMap with NO dirty changes (should not add to dirty sectors)
  em.mutate_sectormap(1, 1, [](SectorMap& /*sm*/) {
    // No modifications
  });
  // Race
  em.mutate_race(1, [](Race& r) { r.morale += 10; });
  // Block
  em.mutate_block(1, [](Block& b) { b.name() = "Modified Block"; });
  // Universe
  em.mutate_universe([](Universe& u) { u.set_AP(1, 75); });
  // ServerState
  em.mutate_server_state([](ServerState& ss) { ss.segments = 15; });

  // Create & mutate ship
  shipnum_t ship_id = 0;
  {
    auto sh = em.create_ship(ShipType::STYPE_SHUTTLE, 1);
    ship_id = sh->number();
  }
  em.mutate_ship(ship_id, [](Ship& s) { s.name() = "MutatedShip1"; });

  // Create & mutate commod
  commodnum_t commod_id{0};
  {
    auto ch = em.create_commod(commod_struct{
        .owner = 1, .amount = 100, .star_from = 1, .planet_from = 1});
    commod_id = commodnum_t{ch->id()};
  }
  em.mutate_commod(commod_id.value, [](Commod& c) { c.set_amount(200); });

  // Create race
  player_t new_player{0};
  {
    Race new_r{};
    new_r.name = "MutTrackers";
    auto rh = em.create_race(new_r);
    new_player = rh->Playernum;
  }

  // Inspect dirty entity set
  DirtyEntitySet dirty = em.take_dirty_entities();
  test::expect_true(dirty.stars.contains(1));
  test::expect_true(dirty.planets.contains({1, 1}));
  test::expect_true(dirty.sectors.contains({1, 1}));
  test::expect_eq(dirty.sectors[{1, 1}].size(), 2);
  test::expect_true(dirty.sectors[{1, 1}].contains(Coordinates{0, 0}));
  test::expect_true(dirty.sectors[{1, 1}].contains(Coordinates{1, 0}));
  test::expect_true(dirty.races.contains(1));
  test::expect_true(dirty.races.contains(new_player));
  test::expect_true(dirty.blocks.contains(1));
  test::expect_true(dirty.ships.contains(ship_id));
  test::expect_true(dirty.commods.contains(commod_id));
  test::expect_true(dirty.universe_dirty);
  test::expect_true(dirty.server_state_dirty);

  // After take_dirty_entities(), EntityManager's dirty set is empty
  DirtyEntitySet empty_set = em.take_dirty_entities();
  test::expect_true(empty_set.stars.empty());
  test::expect_false(empty_set.universe_dirty);
  test::expect_false(empty_set.server_state_dirty);

  // 3. Test ship and commod deletion superseding mutation
  em.mutate_ship(ship_id, [](Ship& s) { s.name() = "MutatedShip2"; });
  em.mutate_commod(commod_id.value, [](Commod& c) { c.set_amount(300); });
  // Verify they are in dirty set
  DirtyEntitySet pre_del = em.take_dirty_entities();
  test::expect_true(pre_del.ships.contains(ship_id));
  test::expect_true(pre_del.commods.contains(commod_id));

  // Now delete them under DeletionBarrier
  em.mutate_ship(ship_id, [](Ship& s) { s.name() = "MutatedShip3"; });
  em.mutate_commod(commod_id.value, [](Commod& c) { c.set_amount(400); });
  {
    auto barrier = em.create_deletion_barrier();
    em.delete_ship(ship_id);
    em.delete_commod(commod_id.value);
  }
  // After barrier drain, deleted_ships / deleted_commods are populated and
  // ships / commods are cleared
  DirtyEntitySet post_del = em.take_dirty_entities();
  test::expect_false(post_del.ships.contains(ship_id));
  test::expect_true(post_del.deleted_ships.contains(ship_id));
  test::expect_false(post_del.commods.contains(commod_id));
  test::expect_true(post_del.deleted_commods.contains(commod_id));

  // Test release_ship deletion when not in barrier (e.g. destroy_ship on an
  // alive ship)
  shipnum_t ship2_id = 0;
  {
    auto sh2 = em.create_ship(ShipType::STYPE_SHUTTLE, 1);
    ship2_id = sh2->number();
  }
  // Mutate ship2
  em.mutate_ship(ship2_id, [](Ship& s) { s.name() = "Ship2"; });
  // destroy_ship without DeletionBarrier triggers immediate deletion via
  // release_ship
  em.mutate_ship(ship2_id, [&](Ship& s) { em.destroy_ship(s); });
  DirtyEntitySet post_kill = em.take_dirty_entities();
  test::expect_false(post_kill.ships.contains(ship2_id));
  test::expect_true(post_kill.deleted_ships.contains(ship2_id));

  // 4. Test clear_dirty_entities()
  em.mutate_star(1, [](Star& s) { s.set_name("Sol Clear"); });
  em.clear_dirty_entities();
  test::expect_true(em.take_dirty_entities().stars.empty());

  // 5. Test stop_mutation_tracking()
  em.stop_mutation_tracking();
  test::expect_false(em.is_tracking_mutations());
  em.mutate_star(1, [](Star& s) { s.set_name("Sol Stopped"); });
  test::expect_true(em.take_dirty_entities().stars.empty());

  // 6. Test DeferredWriteScope skips mutation tracking
  em.begin_mutation_tracking();
  {
    auto scope = em.create_deferred_write_scope();
    em.mutate_star(1, [](Star& s) { s.set_name("Sol Deferred"); });
    em.mutate_planet(1, 1, [](Planet& p) { p.popn() = 55555; });
    scope.commit();
  }
  // While deferred write scope was active, save_fn was not called, so dirty
  // entities remained empty
  DirtyEntitySet scope_dirty = em.take_dirty_entities();
  test::expect_true(scope_dirty.stars.empty());
  test::expect_true(scope_dirty.planets.empty());

  std::println(
      std::cout,
      "  ✓ EntityManager mutation tracking and DirtyEntitySet verified");
}

void test_entity_manager_read_only_scope() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore raw_store(db);
  Race r{};
  r.Playernum = 1;
  r.name = "GuestRace";
  em.create_race(r);
  StarRepository(raw_store).save(Star{1, "Sol"});
  const shipnum_t sid = em.create_ship(ShipType::OTYPE_PROBE, 1)->number();
  Ship probe_copy(em.peek_ship(sid)->to_struct());
  const auto before = db.total_changes();
  {
    auto guard = em.create_read_only_scope();
    test::expect_true(em.is_read_only());
    test::expect_eq(em.peek_star(1)->get_name(), "Sol");
    bool lambda_ran = false;
    test::expect_throws<ReadOnlyError>(
        [&] { em.mutate_star(1, [&](Star&) { lambda_ran = true; }); });
    test::expect_false(lambda_ran);
    test::expect_throws<ReadOnlyError>(
        [&] { em.create_ship(ShipType::OTYPE_PROBE, 1); });
    test::expect_throws<ReadOnlyError>([&] { em.delete_ship(sid); });
    test::expect_throws<ReadOnlyError>([&] { em.destroy_ship(probe_copy); });
    test::expect_true(probe_copy.alive());
    test::expect_throws<ReadOnlyError>(
        [&] { em.post_news(NewsType::ANNOUNCE, "blocked"); });
    test::expect_throws<ReadOnlyError>(
        [&] { em.post_telegram(1, 1, "blocked"); });
    test::expect_throws<ReadOnlyError>(
        [&] { raw_store.store("tbl_star", 1, R"({"name":"Bypass"})"); });
    test::expect_eq(db.total_changes(), before);
  }
  // DAL-initiated ReadOnlyScope synchronizes with EntityManager::is_read_only()
  {
    auto dal_guard = db.create_read_only_scope();
    test::expect_true(em.is_read_only());
    test::expect_throws<ReadOnlyError>(
        [&] { em.mutate_star(1, [](Star&) {}); });
  }
  test::expect_false(em.is_read_only());
  em.mutate_star(1, [](Star& s) { s.set_name("WritableAgain"); });
  test::expect_eq(em.peek_star(1)->get_name(), "WritableAgain");
}

int main() {
  test_entity_manager_basic();
  test_entity_manager_caching();
  test_entity_manager_composite_keys();
  test_entity_manager_create_delete();
  test_entity_manager_read_only_access();
  test_entity_manager_get_ship_throws();
  test_entity_manager_flush_all();
  test_entity_manager_clear_cache();
  test_entity_manager_singleton_universe();
  test_entity_manager_singleton_server_state();
  test_entity_manager_get_player();
  test_entity_manager_destroy_ship();
  test_peek_star_throws_on_not_found();
  test_peek_planet_throws_on_not_found();
  test_peek_sectormap_throws_on_not_found();
  test_peek_caching_and_clear_cache();
  test_entity_manager_commods();
  test_entity_manager_blocks();
  test_entity_manager_powers();
  test_entity_manager_create_ship();
  test_entity_manager_with_scoped_peeks();
  test_entity_manager_mutate_planet_and_sectors();
  test_deletion_barrier();
  test_entity_manager_count_non_asteroid_planets();
  test_entity_manager_create_race();
  test_entity_manager_mutation_tracking();
  test_entity_manager_read_only_scope();

  std::println(std::cout, "\n✅ All EntityManager tests passed!");
  return 0;
}
