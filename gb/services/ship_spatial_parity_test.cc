// SPDX-License-Identifier: Apache-2.0

/// \file ship_spatial_parity_test.cc
/// \brief Parity test suite comparing indexed spatial queries against legacy
/// nextship/ships linked lists.

import dallib;
import gblib;
import test;
import std;

namespace {

void test_empty_universe_parity(TestContext& ctx) {
  JsonStore store(ctx.db);
  ShipRepository ships_repo(store);

  // When no ships exist, all spatial queries return empty vectors matching
  // empty lists
  test::expect_true(ships_repo.find_in_star(starnum_t{0}).empty());
  test::expect_true(
      ships_repo.find_on_planet(starnum_t{0}, planetnum_t{0}).empty());
  test::expect_true(ships_repo.find_in_hangar(shipnum_t{1}).empty());
  test::expect_true(ships_repo.find_by_owner(player_t{1}).empty());
  test::expect_true(ships_repo.find_alive().empty());

  std::println(std::cout, "✓ Empty universe parity verified");
}

void test_star_spatial_parity(TestContext& ctx) {
  JsonStore store(ctx.db);
  ShipRepository ships_repo(store);
  StarRepository stars_repo(store);

  // Setup Star 0 with 3 ships in orbit (linked list: Star.ships -> s1 -> s2 ->
  // s3)
  star_struct star_data{};
  star_data.ships = 1;
  Star star(star_data);
  stars_repo.save(star);

  ship_struct s1_data{};
  s1_data.number = 1;
  s1_data.owner = 1;
  s1_data.storbits = 0;
  s1_data.whatorbits = ScopeLevel::LEVEL_STAR;
  s1_data.alive = true;
  s1_data.nextship = 2;
  ships_repo.save(Ship(s1_data));

  ship_struct s2_data{};
  s2_data.number = 2;
  s2_data.owner = 1;
  s2_data.storbits = 0;
  s2_data.whatorbits = ScopeLevel::LEVEL_STAR;
  s2_data.alive = false;  // Dead ship in star list
  s2_data.nextship = 3;
  ships_repo.save(Ship(s2_data));

  ship_struct s3_data{};
  s3_data.number = 3;
  s3_data.owner = 2;
  s3_data.storbits = 0;
  s3_data.whatorbits = ScopeLevel::LEVEL_STAR;
  s3_data.alive = true;
  s3_data.nextship = 0;
  ships_repo.save(Ship(s3_data));

  // 1. Traverse legacy star.ships() linked list
  std::vector<shipnum_t> linked_all;
  std::vector<shipnum_t> linked_alive;
  for (shipnum_t curr = star.ships(); curr != 0;) {
    auto ship_opt = ships_repo.find_by_number(curr);
    test::expect_true(ship_opt.has_value());
    linked_all.push_back(curr);
    if (ship_opt->alive()) {
      linked_alive.push_back(curr);
    }
    curr = ship_opt->nextship();
  }

  // 2. Query via ShipRepository indexed spatial queries
  auto indexed_alive = ships_repo.find_in_star(starnum_t{0}, true);
  auto indexed_all = ships_repo.find_in_star(starnum_t{0}, false);

  // 3. Verify 100% parity between legacy linked list and indexed query
  test::expect_eq(indexed_alive.size(), linked_alive.size());
  test::expect_eq(indexed_alive, linked_alive);

  test::expect_eq(indexed_all.size(), linked_all.size());
  test::expect_eq(indexed_all, linked_all);

  // 4. Verify GameObj Star ScopeLevel matches
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  std::vector<shipnum_t> shiplist_scope_alive;
  for (const Ship& s :
       ShipList::readonly(ctx.em, g, ShipList::IterationType::Scope)) {
    if (s.alive()) {
      shiplist_scope_alive.push_back(s.number());
    }
  }
  test::expect_eq(indexed_alive, shiplist_scope_alive);

  std::println(std::cout,
               "✓ Star spatial query vs linked list parity verified");
}

void test_planet_spatial_parity(TestContext& ctx) {
  JsonStore store(ctx.db);
  ShipRepository ships_repo(store);
  PlanetRepository planets_repo(store);

  // Setup Planet (Star 1, Planet 0) with 2 ships in orbit (linked list:
  // planet.ships -> s10 -> s11)
  Planet planet{};
  planet.ships() = 10;
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planets_repo.save(planet);

  ship_struct s10_data{};
  s10_data.number = 10;
  s10_data.owner = 1;
  s10_data.storbits = 1;
  s10_data.pnumorbits = 0;
  s10_data.whatorbits = ScopeLevel::LEVEL_PLAN;
  s10_data.alive = true;
  s10_data.nextship = 11;
  ships_repo.save(Ship(s10_data));

  ship_struct s11_data{};
  s11_data.number = 11;
  s11_data.owner = 1;
  s11_data.storbits = 1;
  s11_data.pnumorbits = 0;
  s11_data.whatorbits = ScopeLevel::LEVEL_PLAN;
  s11_data.alive = true;
  s11_data.nextship = 0;
  ships_repo.save(Ship(s11_data));

  // Dead ship on same planet (should be excluded by default)
  ship_struct s12_data{};
  s12_data.number = 12;
  s12_data.owner = 1;
  s12_data.storbits = 1;
  s12_data.pnumorbits = 0;
  s12_data.whatorbits = ScopeLevel::LEVEL_PLAN;
  s12_data.alive = false;
  s12_data.nextship = 0;
  ships_repo.save(Ship(s12_data));

  // 1. Traverse legacy planet.ships() linked list
  std::vector<shipnum_t> linked_alive;
  for (shipnum_t curr = planet.ships(); curr != 0;) {
    auto ship_opt = ships_repo.find_by_number(curr);
    test::expect_true(ship_opt.has_value());
    if (ship_opt->alive()) {
      linked_alive.push_back(curr);
    }
    curr = ship_opt->nextship();
  }

  // 2. Query via ShipRepository indexed spatial query
  auto indexed_alive = ships_repo.find_on_planet(starnum_t{1}, planetnum_t{0},
                                                 /*alive_only=*/true);

  // 3. Verify parity
  test::expect_eq(indexed_alive.size(), 2);
  test::expect_eq(indexed_alive, linked_alive);

  // 4. Verify ShipList Scope iteration at planet scope
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);

  std::vector<shipnum_t> shiplist_scope_alive;
  for (const Ship& s :
       ShipList::readonly(ctx.em, g, ShipList::IterationType::Scope)) {
    if (s.alive()) {
      shiplist_scope_alive.push_back(s.number());
    }
  }
  test::expect_eq(indexed_alive, shiplist_scope_alive);

  std::println(std::cout,
               "✓ Planet spatial query vs linked list parity verified");
}

void test_hangar_docked_parity(TestContext& ctx) {
  JsonStore store(ctx.db);
  ShipRepository ships_repo(store);

  // Carrier ship 20 contains docked fighters s21 and s22 (carrier.ships() ->
  // s21 -> s22)
  ship_struct carrier_data{};
  carrier_data.number = 20;
  carrier_data.owner = 1;
  carrier_data.storbits = 0;
  carrier_data.whatorbits = ScopeLevel::LEVEL_STAR;
  carrier_data.alive = true;
  carrier_data.ships = 21;  // Head of docked ships list
  carrier_data.nextship = 0;
  ships_repo.save(Ship(carrier_data));

  ship_struct s21_data{};
  s21_data.number = 21;
  s21_data.owner = 1;
  s21_data.destshipno = 20;
  s21_data.whatorbits = ScopeLevel::LEVEL_SHIP;
  s21_data.alive = true;
  s21_data.nextship = 22;
  ships_repo.save(Ship(s21_data));

  ship_struct s22_data{};
  s22_data.number = 22;
  s22_data.owner = 1;
  s22_data.destshipno = 20;
  s22_data.whatorbits = ScopeLevel::LEVEL_SHIP;
  s22_data.alive = true;
  s22_data.nextship = 0;
  ships_repo.save(Ship(s22_data));

  // 1. Traverse legacy carrier.ships() linked list
  std::vector<shipnum_t> linked_docked;
  for (shipnum_t curr = carrier_data.ships; curr != 0;) {
    auto ship_opt = ships_repo.find_by_number(curr);
    test::expect_true(ship_opt.has_value());
    linked_docked.push_back(curr);
    curr = ship_opt->nextship();
  }

  // 2. Query via ShipRepository indexed hangar query
  auto indexed_hangar = ships_repo.find_in_hangar(shipnum_t{20}, true);

  // 3. Verify parity
  test::expect_eq(indexed_hangar.size(), 2);
  test::expect_eq(indexed_hangar, linked_docked);

  // 4. Verify ShipList Nested iteration from carrier
  std::vector<shipnum_t> shiplist_nested;
  for (const Ship& s : ShipList::readonly(ctx.em, carrier_data.ships,
                                          ShipList::IterationType::Nested)) {
    shiplist_nested.push_back(s.number());
  }
  test::expect_eq(indexed_hangar, shiplist_nested);

  std::println(std::cout,
               "✓ Hangar docked query vs linked list parity verified");
}

void test_empire_and_global_parity(TestContext& ctx) {
  JsonStore store(ctx.db);
  ShipRepository ships_repo(store);

  // Query player 1 ships via index
  auto p1_indexed = ships_repo.find_by_owner(player_t{1}, true);

  // Collect player 1 ships via ShipList AllAlive
  std::vector<shipnum_t> p1_shiplist;
  for (const Ship& s :
       ShipList::readonly(ctx.em, ShipList::IterationType::AllAlive)) {
    if (s.owner() == 1) {
      p1_shiplist.push_back(s.number());
    }
  }
  test::expect_eq(p1_indexed, p1_shiplist);

  // Query all alive ships via index
  auto all_alive_indexed = ships_repo.find_alive();

  // Collect all alive ships via ShipList AllAlive
  std::vector<shipnum_t> all_alive_shiplist;
  for (const Ship& s :
       ShipList::readonly(ctx.em, ShipList::IterationType::AllAlive)) {
    all_alive_shiplist.push_back(s.number());
  }
  test::expect_eq(all_alive_indexed, all_alive_shiplist);

  std::println(std::cout, "✓ Empire and global query parity verified");
}

}  // namespace

int main() {
  std::println(std::cout, "Running Ship spatial parity tests...");

  TestContext ctx;
  test_empty_universe_parity(ctx);
  test_star_spatial_parity(ctx);
  test_planet_spatial_parity(ctx);
  test_hangar_docked_parity(ctx);
  test_empire_and_global_parity(ctx);

  std::println(std::cout, "\nAll Ship spatial parity tests passed!");
  return 0;
}
