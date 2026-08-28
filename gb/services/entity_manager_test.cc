// SPDX-License-Identifier: Apache-2.0

/// \file entity_manager_test.cc
/// \brief Unit tests for EntityManager lifecycle, caching, composite keys, and
/// CRUD operations.

import dallib;
import gblib;
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

  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 2;
  planet.popn() = 10000;
  JsonStore store(db);
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
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: EntityManager create/delete");

  shipnum_t ship_num;
  {
    auto new_ship = em.create_ship();
    ship_num = new_ship->number();
    new_ship->fuel() = 100.0;
    new_ship->mass() = 50.0;
  }
  std::println(std::cout, "  ✓ create_ship() creates and saves new ship");

  const auto* peek = em.peek_ship(ship_num);
  test::expect_ne(peek, nullptr);
  test::expect_eq(peek->fuel(), 100.0);

  em.delete_ship(ship_num);
  test::expect_throws<EntityNotFoundError>([&]() { em.peek_ship(ship_num); });
  std::println(std::cout,
               "  ✓ delete_ship() removes ship from cache and database");
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

  std::println(std::cout, "Test: EntityManager singleton (universe_struct)");

  test::expect_throws<EntityNotFoundError>([&]() { em.peek_universe(); });
  test::expect_throws<EntityNotFoundError>(
      [&]() { em.mutate_universe([](universe_struct&) {}); });
  std::println(
      std::cout,
      "  ✓ peek_universe and mutate_universe throw EntityNotFoundError "
      "when uninitialized");

  universe_struct univ{};
  univ.id = 1;
  univ.numstars = 50;
  univ.ships = 200;
  JsonStore store(db);
  UniverseRepository univ_repo(store);
  univ_repo.save(univ);

  const auto* u1 = em.peek_universe();
  const auto* u2 = em.peek_universe();
  test::expect_eq(u1, u2);
  test::expect_eq(u1->numstars, 50);

  em.mutate_universe([](universe_struct& u) { u.numstars = 75; });

  const auto* peek = em.peek_universe();
  test::expect_ne(peek, nullptr);
  test::expect_eq(peek->numstars, 75);
  std::println(std::cout, "  ✓ Singleton universe_struct works correctly");
}

void test_entity_manager_singleton_server_state() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: EntityManager singleton (ServerState)");

  ServerState state{};
  state.id = 1;
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
}

void test_entity_manager_kill_ship() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: EntityManager kill_ship()");

  Race victim{};
  victim.Playernum = 1;
  victim.name = "Victim";
  victim.morale = 1000;

  Race killer{};
  killer.Playernum = 2;
  killer.name = "Killer";
  killer.morale = 1000;

  JsonStore store(db);
  RaceRepository races(store);
  races.save(victim);
  races.save(killer);

  ship_struct ship_data{};
  ship_data.number = 100;
  ship_data.owner = 1;
  ship_data.type = ShipType::STYPE_FIGHTER;
  ship_data.alive = 1;
  ship_data.build_cost = 50;

  Ship ship(ship_data);
  ShipRepository ships(store);
  ships.save(ship);

  em.kill_ship(2, ship);
  std::println(std::cout, "  ✓ kill_ship executed without errors");

  test::expect_eq(ship.alive(), 0);
  test::expect_eq(ship.notified(), 0);
  std::println(std::cout, "  ✓ Ship marked as dead (alive=0, notified=0)");

  em.with_race(1, [](const Race& v) { test::expect_lt(v.morale, 1000); });
  em.with_race(2, [](const Race& k) { test::expect_gt(k.morale, 1000); });
  std::println(std::cout, "  ✓ Morale adjustments persisted for both races");

  ship_struct vn_data{};
  vn_data.number = 200;
  vn_data.owner = 0;
  vn_data.type = ShipType::OTYPE_VN;
  vn_data.alive = 1;
  vn_data.whatorbits = ScopeLevel::LEVEL_STAR;
  vn_data.storbits = 5;

  Ship vn_ship(vn_data);
  ships.save(vn_ship);

  universe_struct univ_data{};
  univ_data.id = 1;
  for (int i = 0; i < MAXPLAYERS; i++) {
    univ_data.VN_index1[i] = -1;
    univ_data.VN_index2[i] = -1;
  }
  univ_data.VN_hitlist[0] = 5;
  UniverseRepository univ_repo(store);
  univ_repo.save(univ_data);

  em.kill_ship(2, vn_ship);
  std::println(std::cout, "  ✓ VN ship killed without errors");

  em.with_universe([&](const universe_struct& univ) {
    test::expect_eq(univ.VN_index1[1], 5);
  });
  std::println(std::cout,
               "  ✓ VN tracking (VN_hitlist and VN_index) updated correctly");

  ship_struct pod_data{};
  pod_data.number = 300;
  pod_data.owner = 1;
  pod_data.type = ShipType::STYPE_POD;
  pod_data.alive = 1;

  Ship pod_ship(pod_data);
  ships.save(pod_ship);

  int v_morale_before = 0;
  em.with_race(1, [&](const Race& v) { v_morale_before = v.morale; });
  em.kill_ship(2, pod_ship);
  em.with_race(
      1, [&](const Race& v) { test::expect_eq(v.morale, v_morale_before); });
  std::println(std::cout, "  ✓ Pod death does not affect morale");
}

void test_entity_manager_kill_ship_gov_ship() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: EntityManager kill_ship() with Gov_ship");

  Race victim{};
  victim.Playernum = 1;
  victim.name = "Victim";
  Race killer{};
  killer.Playernum = 2;
  killer.name = "Killer";
  killer.morale = 1000;

  JsonStore store(db);
  RaceRepository races(store);
  races.save(victim);
  races.save(killer);

  ship_struct ship_data{};
  ship_data.number = 100;
  ship_data.owner = 1;
  ship_data.type = ShipType::STYPE_HABITAT;
  ship_data.alive = 1;

  Ship ship(ship_data);
  ShipRepository ships(store);
  ships.save(ship);

  em.kill_ship(2, ship);
  std::println(std::cout, "  ✓ Government ship killed");

  em.with_race(1, [](const Race& v) { test::expect_eq(v.Gov_ship, 0); });
  std::println(std::cout,
               "  ✓ Gov_ship field cleared when government ship is killed");
}

void test_peek_star_throws_on_not_found() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout,
               "Test: peek_star throws EntityNotFoundError on not found");

  test::expect_throws<EntityNotFoundError>([&]() { em.peek_star(999); });
  std::println(std::cout,
               "  ✓ peek_star throws EntityNotFoundError for invalid star_id");
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
  star_struct raw_star{};
  raw_star.star_id = 0;
  raw_star.name = "TestStar";
  Star star_data{raw_star};
  StarRepository stars(store);
  stars.save(star_data);

  {
    const auto* peek1 = em.peek_star(0);
    test::expect_ne(peek1, nullptr);
    test::expect_eq(peek1->get_name(), "TestStar");

    const auto* peek2 = em.peek_star(0);
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
    Ship ship(ship_data);
    ShipRepository ships(store);
    ships.save(ship);

    const auto* peek_ship = em.peek_ship(100);
    test::expect_ne(peek_ship, nullptr);
    test::expect_eq(peek_ship->fuel(), 1000.0);

    // Modify directly in DB
    ship.fuel() = 2500.0;
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
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: EntityManager commod management");

  JsonStore store(db);
  CommodRepository repo(store);

  Commod c{};
  c.id = 1;
  c.owner = 2;
  c.governor = 0;
  c.type = CommodType::FUEL;
  c.amount = 500;
  c.bid = 100;
  repo.save(c);

  const auto* peek = em.peek_commod(1);
  test::expect_ne(peek, nullptr);
  test::expect_eq(peek->amount, 500);
  test::expect_eq(peek->bid, 100);
  std::println(std::cout, "  ✓ peek_commod works");

  auto amount_read =
      em.with_commod(1, [](const Commod& cmd) { return cmd.amount; });
  test::expect_eq(amount_read, 500);
  std::println(std::cout, "  ✓ with_commod works");

  em.mutate_commod(1, [](Commod& cmd) {
    cmd.amount = 800;
    cmd.bid = 150;
  });

  auto updated = repo.find_by_id(1);
  test::expect_true(updated.has_value());
  test::expect_eq(updated->amount, 800);
  test::expect_eq(updated->bid, 150);
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
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: EntityManager block management");

  JsonStore store(db);
  BlockRepository repo(store);

  block b{};
  b.Playernum = 1;
  b.name = "Test Alliance";
  b.VPs = 500;
  repo.save(b);

  const auto* peek = em.peek_block(blocknum_t{1});
  test::expect_ne(peek, nullptr);
  test::expect_eq(peek->name, "Test Alliance");
  test::expect_eq(peek->VPs, 500);
  std::println(std::cout, "  ✓ peek_block works");

  auto vps_read =
      em.with_block(blocknum_t{1}, [](const block& blk) { return blk.VPs; });
  test::expect_eq(vps_read, 500);
  std::println(std::cout, "  ✓ with_block works");

  em.mutate_block(blocknum_t{1}, [](block& blk) { blk.VPs = 1000; });

  auto updated = repo.find_by_id(blocknum_t{1});
  test::expect_true(updated.has_value());
  test::expect_eq(updated->VPs, 1000);
  std::println(std::cout, "  ✓ mutate_block auto-save persisted changes");

  test::expect_throws<EntityNotFoundError>(
      [&]() { em.peek_block(blocknum_t{999}); });
  test::expect_throws<EntityNotFoundError>(
      [&]() { em.mutate_block(blocknum_t{999}, [](block&) {}); });
  std::println(std::cout,
               "  ✓ peek_block and mutate_block throw EntityNotFoundError for "
               "missing block");
}

void test_entity_manager_powers() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: EntityManager power management");

  JsonStore store(db);
  PowerRepository repo(store);

  power p{};
  p.id = 1;
  p.troops = 1000;
  p.popn = 50000;
  repo.save(p);

  const auto* peek = em.peek_power(powernum_t{1});
  test::expect_ne(peek, nullptr);
  test::expect_eq(peek->troops, 1000);
  test::expect_eq(peek->popn, 50000);
  std::println(std::cout, "  ✓ peek_power works");

  auto troops_read =
      em.with_power(powernum_t{1}, [](const power& pwr) { return pwr.troops; });
  test::expect_eq(troops_read, 1000);
  std::println(std::cout, "  ✓ with_power works");

  em.mutate_power(powernum_t{1}, [](power& pwr) { pwr.troops = 2500; });

  auto updated = repo.find_by_id(powernum_t{1});
  test::expect_true(updated.has_value());
  test::expect_eq(updated->troops, 2500);
  std::println(std::cout, "  ✓ mutate_power auto-save persisted changes");

  test::expect_throws<EntityNotFoundError>(
      [&]() { em.peek_power(powernum_t{999}); });
  test::expect_throws<EntityNotFoundError>(
      [&]() { em.mutate_power(powernum_t{999}, [](power&) {}); });
  std::println(std::cout,
               "  ✓ peek_power and mutate_power throw EntityNotFoundError for "
               "missing power");
}

void test_entity_manager_create_ship() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: EntityManager create_ship");

  ship_struct init_data{};
  init_data.owner = 1;
  init_data.governor = 0;
  init_data.name = "Discovery";
  init_data.type = ShipType::OTYPE_PROBE;
  init_data.fuel = 100.0;

  shipnum_t new_ship_num;
  {
    auto new_ship_handle = em.create_ship(init_data);
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
  star_struct raw_star{};
  raw_star.star_id = 0;
  raw_star.name = "AlphaCentauri";
  Star star(raw_star);
  StarRepository stars(store);
  stars.save(star);

  auto star_name = em.with_star(0, [](const Star& s) { return s.get_name(); });
  test::expect_eq(star_name, "AlphaCentauri");

  // 3. with_planet
  Planet p{PlanetType::EARTH, Coordinates{5, 5}};
  p.star_id() = 0;
  p.planet_order() = 1;
  p.popn() = 5000;
  PlanetRepository planets(store);
  planets.save(p);

  auto popn = em.with_planet(0, 1, [](const Planet& pl) { return pl.popn(); });
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

  auto sect_owner = em.with_sectormap(0, 1, [](const SectorMap& map) {
    return map.get(Coordinates{2, 3}).get_owner();
  });
  test::expect_eq(sect_owner, player_t{1});

  // 6. with_universe
  universe_struct u{};
  u.id = 1;
  u.numstars = 10;
  UniverseRepository u_repo(store);
  u_repo.save(u);

  auto numstars = em.with_universe(
      [](const universe_struct& univ) { return univ.numstars; });
  test::expect_eq(numstars, 10);

  // 7. with_server_state
  ServerState state{};
  state.id = 1;
  state.welcome_message = "Welcome to GB!";
  ServerStateRepository state_repo(store);
  state_repo.save(state);

  auto msg = em.with_server_state(
      [](const ServerState& st) { return st.welcome_message; });
  test::expect_eq(msg, "Welcome to GB!");

  // 8. with_ship_exam
  auto exam_name = em.with_ship_exam(
      ShipType::OTYPE_PROBE, [](const ShipExam& ex) { return ex.name; });
  test::expect_eq(exam_name, "Space Probe");

  std::println(std::cout, "  ✓ All with_* scoped peek helpers passed");
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
  test_entity_manager_kill_ship();
  test_entity_manager_kill_ship_gov_ship();
  test_peek_star_throws_on_not_found();
  test_peek_planet_throws_on_not_found();
  test_peek_sectormap_throws_on_not_found();
  test_peek_caching_and_clear_cache();
  test_entity_manager_commods();
  test_entity_manager_blocks();
  test_entity_manager_powers();
  test_entity_manager_create_ship();
  test_entity_manager_with_scoped_peeks();

  std::println(std::cout, "\n✅ All EntityManager tests passed!");
  return 0;
}
