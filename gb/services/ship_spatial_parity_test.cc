// SPDX-License-Identifier: Apache-2.0

///// \file ship_spatial_parity_test.cc
/// \brief Test suite verifying indexed spatial queries and ShipList spatial
/// helpers.

import dallib;
import gb.entities;
import gb.services;
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

  // Setup Star 0 with 3 ships in orbit
  star_struct star_data{};
  Star star(star_data);
  stars_repo.save(star);

  ship_struct s1_data{};
  s1_data.number = 1;
  s1_data.owner = 1;
  s1_data.storbits = 0;
  s1_data.whatorbits = ScopeLevel::LEVEL_STAR;
  s1_data.alive = true;
  ships_repo.save(Ship(s1_data));

  ship_struct s2_data{};
  s2_data.number = 2;
  s2_data.owner = 1;
  s2_data.storbits = 0;
  s2_data.whatorbits = ScopeLevel::LEVEL_STAR;
  s2_data.alive = false;  // Dead ship in star list
  ships_repo.save(Ship(s2_data));

  ship_struct s3_data{};
  s3_data.number = 3;
  s3_data.owner = 2;
  s3_data.storbits = 0;
  s3_data.whatorbits = ScopeLevel::LEVEL_STAR;
  s3_data.alive = true;
  ships_repo.save(Ship(s3_data));

  // 1. Query via ShipRepository indexed spatial queries
  auto indexed_alive = ships_repo.find_in_star(starnum_t{0}, true);
  auto indexed_all = ships_repo.find_in_star(starnum_t{0}, false);

  test::expect_eq(indexed_alive.size(), 2);
  test::expect_eq(indexed_alive, (std::vector<shipnum_t>{1, 3}));
  test::expect_eq(indexed_all.size(), 3);
  test::expect_eq(indexed_all, (std::vector<shipnum_t>{1, 2, 3}));

  // 2. Query via ShipList::readonly_in_star
  std::vector<shipnum_t> shiplist_in_star_alive;
  for (const Ship& s : ShipList::readonly_in_star(ctx.em, starnum_t{0})) {
    shiplist_in_star_alive.push_back(s.number());
  }
  test::expect_eq(indexed_alive, shiplist_in_star_alive);

  // 3. Query via ShipList::in_star (mutable)
  std::vector<shipnum_t> shiplist_in_star_mutable;
  for (auto handle : ShipList::in_star(ctx.em, starnum_t{0})) {
    shiplist_in_star_mutable.push_back(handle->number());
  }
  test::expect_eq(indexed_alive, shiplist_in_star_mutable);

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

  std::println(std::cout, "✓ Star spatial query parity verified");
}

void test_planet_spatial_parity(TestContext& ctx) {
  JsonStore store(ctx.db);
  ShipRepository ships_repo(store);
  PlanetRepository planets_repo(store);

  // Setup Planet (Star 1, Planet 0) with 2 ships in orbit
  Planet planet{};
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
  ships_repo.save(Ship(s10_data));

  ship_struct s11_data{};
  s11_data.number = 11;
  s11_data.owner = 1;
  s11_data.storbits = 1;
  s11_data.pnumorbits = 0;
  s11_data.whatorbits = ScopeLevel::LEVEL_PLAN;
  s11_data.alive = true;
  ships_repo.save(Ship(s11_data));

  // Dead ship on same planet (should be excluded by default)
  ship_struct s12_data{};
  s12_data.number = 12;
  s12_data.owner = 1;
  s12_data.storbits = 1;
  s12_data.pnumorbits = 0;
  s12_data.whatorbits = ScopeLevel::LEVEL_PLAN;
  s12_data.alive = false;
  ships_repo.save(Ship(s12_data));

  // 1. Query via ShipRepository indexed spatial query
  auto indexed_alive = ships_repo.find_on_planet(starnum_t{1}, planetnum_t{0},
                                                 /*alive_only=*/true);
  auto indexed_all = ships_repo.find_on_planet(starnum_t{1}, planetnum_t{0},
                                               /*alive_only=*/false);

  // 2. Verify results
  test::expect_eq(indexed_alive.size(), 2);
  test::expect_eq(indexed_alive, (std::vector<shipnum_t>{10, 11}));
  test::expect_eq(indexed_all.size(), 3);
  test::expect_eq(indexed_all, (std::vector<shipnum_t>{10, 11, 12}));

  // 3. Query via ShipList::readonly_on_planet
  std::vector<shipnum_t> shiplist_on_planet;
  for (const Ship& s :
       ShipList::readonly_on_planet(ctx.em, starnum_t{1}, planetnum_t{0})) {
    shiplist_on_planet.push_back(s.number());
  }
  test::expect_eq(indexed_alive, shiplist_on_planet);

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

  std::println(std::cout, "✓ Planet spatial query parity verified");
}

void test_hangar_docked_parity(TestContext& ctx) {
  JsonStore store(ctx.db);
  ShipRepository ships_repo(store);

  // Carrier ship 20 contains docked fighters s21 and s22
  ship_struct carrier_data{};
  carrier_data.number = 20;
  carrier_data.owner = 1;
  carrier_data.storbits = 0;
  carrier_data.whatorbits = ScopeLevel::LEVEL_STAR;
  carrier_data.alive = true;
  ships_repo.save(Ship(carrier_data));

  ship_struct s21_data{};
  s21_data.number = 21;
  s21_data.owner = 1;
  s21_data.destshipno = 20;
  s21_data.whatorbits = ScopeLevel::LEVEL_SHIP;
  s21_data.alive = true;
  ships_repo.save(Ship(s21_data));

  ship_struct s22_data{};
  s22_data.number = 22;
  s22_data.owner = 1;
  s22_data.destshipno = 20;
  s22_data.whatorbits = ScopeLevel::LEVEL_SHIP;
  s22_data.alive = true;
  ships_repo.save(Ship(s22_data));

  // 1. Query via ShipRepository indexed hangar query
  auto indexed_hangar = ships_repo.find_in_hangar(shipnum_t{20}, true);

  // 2. Verify results
  test::expect_eq(indexed_hangar.size(), 2);
  test::expect_eq(indexed_hangar, (std::vector<shipnum_t>{21, 22}));

  // 3. Verify ShipList::readonly_in_carrier
  std::vector<shipnum_t> shiplist_carrier;
  for (const Ship& s : ShipList::readonly_in_carrier(ctx.em, shipnum_t{20})) {
    shiplist_carrier.push_back(s.number());
  }
  test::expect_eq(indexed_hangar, shiplist_carrier);

  // 4. Verify ShipList::in_carrier (mutable)
  std::vector<shipnum_t> shiplist_carrier_mutable;
  for (auto handle : ShipList::in_carrier(ctx.em, shipnum_t{20})) {
    shiplist_carrier_mutable.push_back(handle->number());
  }
  test::expect_eq(indexed_hangar, shiplist_carrier_mutable);

  std::println(std::cout, "✓ Hangar docked query parity verified");
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
