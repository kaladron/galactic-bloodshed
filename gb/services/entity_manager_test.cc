// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std;

#include <cassert>

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

  {
    auto race_handle = em.get_race(1);
    assert(race_handle.get() != nullptr);
    assert(race_handle->name == "Test Race");
    assert(race_handle->tech == 50.0);

    race_handle->tech = 75.0;
    race_handle->name = "Updated Race";
  }

  const auto* updated_race = em.peek_race(1);
  assert(updated_race != nullptr);
  assert(updated_race->tech == 75.0);
  assert(updated_race->name == "Updated Race");

  std::println(std::cout, "  ✓ Basic get/modify/auto-save works");
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

  const Race* first_ptr = nullptr;
  {
    auto handle1 = em.get_race(1);
    first_ptr = handle1.get();

    auto handle2 = em.get_race(1);
    assert(handle1.get() == handle2.get());
    assert(first_ptr == handle2.get());
    std::println(std::cout, "  ✓ Multiple get calls return same cached instance");

    handle2->name = "Modified in Handle 2";
    assert(handle1->name == "Modified in Handle 2");
    std::println(std::cout, "  ✓ Modifications visible across all handles (same instance)");
  }

  em.clear_cache();
  auto handle3 = em.get_race(1);
  assert(handle3->name == "Modified in Handle 2");
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

  {
    auto p1 = em.get_planet(1, 2);
    auto p2 = em.get_planet(1, 2);
    assert(p1.get() == p2.get());
    std::println(std::cout, "  ✓ Multiple handles to same planet return identical instance");

    p1->popn() = 20000;
  }

  const auto* peek = em.peek_planet(1, 2);
  assert(peek != nullptr);
  assert(peek->popn() == 20000);
  std::println(std::cout, "  ✓ Composite keys work for Planet entities");
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
  assert(peek != nullptr);
  assert(peek->fuel() == 100.0);

  em.delete_ship(ship_num);
  bool threw = false;
  try {
    em.peek_ship(ship_num);
  } catch (const EntityNotFoundError&) {
    threw = true;
  }
  assert(threw);
  std::println(std::cout, "  ✓ delete_ship() removes ship from cache and database");
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
  assert(peek_race != nullptr);
  assert(peek_race->name == "ReadOnly Race");
  std::println(std::cout, "  ✓ peek_race() provides read-only access to cached entity");

  bool peek_threw = false;
  try {
    em.peek_race(999);
  } catch (const EntityNotFoundError&) {
    peek_threw = true;
  }
  assert(peek_threw);
  std::println(std::cout, "  ✓ peek_*() throws EntityNotFoundError for non-existent entities");

  bool get_threw = false;
  try {
    em.get_race(999);
  } catch (const EntityNotFoundError&) {
    get_threw = true;
  }
  assert(get_threw);
  std::println(std::cout, "  ✓ get_race() throws EntityNotFoundError for non-existent entities");
}

void test_entity_manager_get_ship_throws() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: EntityManager get_ship() throwing on non-existent ship");

  bool get_threw = false;
  try {
    em.get_ship(999);
  } catch (const EntityNotFoundError&) {
    get_threw = true;
  }
  assert(get_threw);
  std::println(std::cout, "  ✓ get_ship() throws EntityNotFoundError for non-existent ships");

  bool peek_threw = false;
  try {
    em.peek_ship(999);
  } catch (const EntityNotFoundError&) {
    peek_threw = true;
  }
  assert(peek_threw);
  std::println(std::cout, "  ✓ peek_ship() throws EntityNotFoundError for non-existent ships");
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

  auto handle = em.get_race(1);
  handle->tech = 100.0;

  em.flush_all();

  auto direct_read = races.find_by_player(1);
  assert(direct_read.has_value());
  assert(direct_read->tech == 100.0);
  std::println(std::cout, "  ✓ flush_all() saves all cached entities immediately");
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

  auto handle = em.get_race(1);
  handle->tech = 100.0;

  em.clear_cache();
  assert(handle->tech == 100.0);
  std::println(std::cout, "  ✓ clear_cache() preserves entities with active handles");

  const auto* peek = em.peek_race(1);
  assert(peek != nullptr);
  assert(peek->tech == 100.0);
  std::println(std::cout, "  ✓ peek_* reloads from database after cache clear");

  handle = em.get_race(1);
  handle->name = "Updated After Clear";

  const auto* peek_after = em.peek_race(1);
  assert(peek_after != nullptr);
  assert(peek_after->name == "Updated After Clear");
  std::println(std::cout, "  ✓ Entities can be reloaded after cache clear");
}

void test_entity_manager_singleton_universe() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: EntityManager singleton (universe_struct)");

  universe_struct univ{};
  univ.id = 1;
  univ.numstars = 50;
  univ.ships = 200;
  JsonStore store(db);
  UniverseRepository univ_repo(store);
  univ_repo.save(univ);

  {
    auto u1 = em.get_universe();
    auto u2 = em.get_universe();
    assert(u1.get() == u2.get());
    assert(u1->numstars == 50);

    u1->numstars = 75;
  }

  const auto* peek = em.peek_universe();
  assert(peek != nullptr);
  assert(peek->numstars == 75);
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

  {
    auto s1 = em.get_server_state();
    auto s2 = em.get_server_state();
    assert(s1.get() == s2.get());
    assert(s1->segments == 10);

    s1->segments = 15;
  }

  const auto* peek = em.peek_server_state();
  assert(peek != nullptr);
  assert(peek->segments == 15);
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
  assert(p1.has_value() && *p1 == 1);
  auto p2 = em.find_player_by_name("Empire");
  assert(p2.has_value() && *p2 == 2);
  std::println(std::cout, "  ✓ find_player_by_name finds race by name");

  auto p1_by_num = em.find_player_by_name("1");
  assert(p1_by_num.has_value() && *p1_by_num == 1);
  auto p2_by_num = em.find_player_by_name("2");
  assert(p2_by_num.has_value() && *p2_by_num == 2);
  std::println(std::cout, "  ✓ find_player_by_name finds race by number string");

  assert(!em.find_player_by_name("").has_value());
  std::println(std::cout, "  ✓ find_player_by_name returns nullopt for empty string");

  assert(!em.find_player_by_name("Unknown").has_value());
  std::println(std::cout, "  ✓ find_player_by_name returns nullopt for non-existent race");

  assert(!em.find_player_by_name("999").has_value());
  std::println(std::cout, "  ✓ find_player_by_name returns nullopt for out-of-range number");

  assert(!em.find_player_by_name("0").has_value());
  assert(!em.find_player_by_name("-1").has_value());
  std::println(std::cout, "  ✓ find_player_by_name returns nullopt for invalid player number");
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

  assert(ship.alive() == 0);
  assert(ship.notified() == 0);
  std::println(std::cout, "  ✓ Ship marked as dead (alive=0, notified=0)");

  auto v_handle = em.get_race(1);
  auto k_handle = em.get_race(2);
  assert(v_handle->morale < 1000);
  assert(k_handle->morale > 1000);
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

  auto univ_handle = em.get_universe();
  assert(univ_handle->VN_index1[1] == 5);
  std::println(std::cout, "  ✓ VN tracking (VN_hitlist and VN_index) updated correctly");

  ship_struct pod_data{};
  pod_data.number = 300;
  pod_data.owner = 1;
  pod_data.type = ShipType::STYPE_POD;
  pod_data.alive = 1;

  Ship pod_ship(pod_data);
  ships.save(pod_ship);

  int v_morale_before = v_handle->morale;
  em.kill_ship(2, pod_ship);
  assert(v_handle->morale == v_morale_before);
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

  auto v_handle = em.get_race(1);
  assert(v_handle->Gov_ship == 0);
  std::println(std::cout, "  ✓ Gov_ship field cleared when government ship is killed");
}

void test_peek_star_throws_on_not_found() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: peek_star throws EntityNotFoundError on not found");

  bool threw = false;
  try {
    em.peek_star(999);
  } catch (const EntityNotFoundError& e) {
    threw = true;
    std::println(std::cout, "  ✓ Exception caught for invalid star_id");
  }
  assert(threw);
  std::println(std::cout, "  ✓ peek_star throws EntityNotFoundError for invalid star_id");
}

void test_peek_planet_throws_on_not_found() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: peek_planet throws EntityNotFoundError on not found");

  bool threw = false;
  try {
    em.peek_planet(5, 3);
  } catch (const EntityNotFoundError& e) {
    threw = true;
    std::println(std::cout, "  ✓ Exception caught for invalid planet");
  }
  assert(threw);
  std::println(std::cout, "  ✓ peek_planet throws EntityNotFoundError for invalid planet");
}

void test_peek_sectormap_throws_on_not_found() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: peek_sectormap throws EntityNotFoundError on not found");

  bool threw = false;
  try {
    em.peek_sectormap(10, 5);
  } catch (const EntityNotFoundError& e) {
    threw = true;
    std::println(std::cout, "  ✓ Exception caught for invalid planet");
  }
  assert(threw);
  std::println(std::cout, "  ✓ peek_sectormap throws EntityNotFoundError for invalid planet");
}

void test_peek_increments_refcount() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println(std::cout, "Test: peek increments refcount (pointers remain valid)");

  JsonStore store(db);
  star_struct raw_star{};
  raw_star.star_id = 0;
  raw_star.name = "TestStar";
  Star star_data{raw_star};
  StarRepository stars(store);
  stars.save(star_data);

  {
    const auto* peek1 = em.peek_star(0);
    assert(peek1 != nullptr);
    assert(peek1->get_name() == "TestStar");

    {
      auto star_handle = em.get_star(0);
      star_handle->set_name("ModifiedStar");
    }

    std::string peek_name_after = peek1->get_name();
    assert(peek_name_after == "ModifiedStar");
    std::println(std::cout, "  ✓ peek pointer remains valid after get/release cycle");
  }

  {
    const auto* peek1 = em.peek_star(0);
    const auto* peek2 = em.peek_star(0);
    assert(peek1 == peek2);
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
    assert(peek_race != nullptr);
    assert(peek_race->name == "TestRace");

    {
      auto race_handle = em.get_race(1);
      race_handle->tech = 75.0;
    }

    assert(peek_race->name == "TestRace");
    std::println(std::cout, "  ✓ peek on race also increments refcount correctly");
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
    assert(peek_ship != nullptr);

    {
      auto ship_handle = em.get_ship(100);
      ship_handle->fuel() = 500.0;
    }

    assert(peek_ship != nullptr);
    std::println(std::cout, "  ✓ peek on ship also increments refcount correctly");
  }

  std::println(std::cout, "  ✅ All peek refcount tests passed");
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
  assert(peek != nullptr);
  assert(peek->amount == 500);
  assert(peek->bid == 100);
  std::println(std::cout, "  ✓ peek_commod works");

  {
    auto handle = em.get_commod(1);
    handle->amount = 800;
    handle->bid = 150;
  }

  auto updated = repo.find_by_id(1);
  assert(updated.has_value());
  assert(updated->amount == 800);
  assert(updated->bid == 150);
  std::println(std::cout, "  ✓ get_commod auto-save persisted changes");

  assert(em.peek_commod(999) == nullptr);
  std::println(std::cout, "  ✓ peek_commod returns nullptr for missing commod");
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

  const auto* peek = em.peek_block(1);
  assert(peek != nullptr);
  assert(peek->name == "Test Alliance");
  assert(peek->VPs == 500);
  std::println(std::cout, "  ✓ peek_block works");

  {
    auto handle = em.get_block(1);
    handle->VPs = 1000;
  }

  auto updated = repo.find_by_id(1);
  assert(updated.has_value());
  assert(updated->VPs == 1000);
  std::println(std::cout, "  ✓ get_block auto-save persisted changes");

  assert(em.peek_block(999) == nullptr);
  std::println(std::cout, "  ✓ peek_block returns nullptr for missing block");
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

  const auto* peek = em.peek_power(1);
  assert(peek != nullptr);
  assert(peek->troops == 1000);
  assert(peek->popn == 50000);
  std::println(std::cout, "  ✓ peek_power works");

  {
    auto handle = em.get_power(1);
    handle->troops = 2500;
  }

  auto updated = repo.find_by_id(1);
  assert(updated.has_value());
  assert(updated->troops == 2500);
  std::println(std::cout, "  ✓ get_power auto-save persisted changes");

  assert(em.peek_power(999) == nullptr);
  std::println(std::cout, "  ✓ peek_power returns nullptr for missing power");
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
    assert(new_ship_handle.get() != nullptr);
    new_ship_num = new_ship_handle->number();
    assert(new_ship_num > 0);
    assert(new_ship_handle->name() == "Discovery");
    assert(new_ship_handle->fuel() == 100.0);
  }

  const auto* peek = em.peek_ship(new_ship_num);
  assert(peek != nullptr);
  assert(peek->name() == "Discovery");
  assert(peek->owner() == 1);
  assert(peek->fuel() == 100.0);
  std::println(std::cout, "  ✓ create_ship allocated ID and persisted successfully");
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
  test_peek_increments_refcount();
  test_entity_manager_commods();
  test_entity_manager_blocks();
  test_entity_manager_powers();
  test_entity_manager_create_ship();

  std::println(std::cout, "\n✅ All EntityManager tests passed!");
  return 0;
}
