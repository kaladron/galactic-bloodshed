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
  test::expect_true(ships_repo.find_in_star(starnum_t{1}).empty());
  test::expect_true(
      ships_repo.find_on_planet(starnum_t{1}, planetnum_t{1}).empty());
  test::expect_true(ships_repo.find_in_hangar(shipnum_t{1}).empty());
  test::expect_true(ships_repo.find_by_owner(player_t{1}).empty());
  test::expect_true(ships_repo.find_alive().empty());

  std::println(std::cout, "✓ Empty universe parity verified");
}

void test_star_spatial_parity(TestContext& ctx) {
  JsonStore store(ctx.db);
  ShipRepository ships_repo(store);

  // Setup Star 1 with 3 ships in orbit
  TestWorldBuilder(ctx).add_star("Star1", 100, starnum_t{1});

  TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE, 1)
      .owned_by(1)
      .in_star_orbit(1)
      .with_alive(true)
      .build();

  TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE, 2)
      .owned_by(1)
      .in_star_orbit(1)
      .with_alive(false)  // Dead ship in star list
      .build();

  TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE, 3)
      .owned_by(2)
      .in_star_orbit(1)
      .with_alive(true)
      .build();

  // 1. Query via ShipRepository indexed spatial queries
  auto indexed_alive = ships_repo.find_in_star(starnum_t{1}, true);
  auto indexed_all = ships_repo.find_in_star(starnum_t{1}, false);

  test::expect_eq(indexed_alive.size(), 2);
  test::expect_eq(indexed_alive, (std::vector<shipnum_t>{1, 3}));
  test::expect_eq(indexed_all.size(), 3);
  test::expect_eq(indexed_all, (std::vector<shipnum_t>{1, 2, 3}));

  // 2. Query via ShipList::readonly_in_star
  std::vector<shipnum_t> shiplist_in_star_alive;
  for (const Ship& s : ShipList::readonly_in_star(ctx.em, starnum_t{1})) {
    shiplist_in_star_alive.push_back(s.number());
  }
  test::expect_eq(indexed_alive, shiplist_in_star_alive);

  // 3. Query via ShipList::in_star (mutable)
  std::vector<shipnum_t> shiplist_in_star_mutable;
  for (auto handle : ShipList::in_star(ctx.em, starnum_t{1})) {
    shiplist_in_star_mutable.push_back(handle->number());
  }
  test::expect_eq(indexed_alive, shiplist_in_star_mutable);

  // 4. Verify GameObj Star ScopeLevel matches
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

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

  // Setup Planet (Star 1, Planet 1) with 2 ships in orbit
  TestPlanetBuilder(ctx, 1, PlanetType::EARTH, {10, 10}, planetnum_t{1})
      .build();

  TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE, 10)
      .owned_by(1)
      .in_planet_orbit(1, 1)
      .with_alive(true)
      .build();

  TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE, 11)
      .owned_by(1)
      .in_planet_orbit(1, 1)
      .with_alive(true)
      .build();

  // Dead ship on same planet (should be excluded by default)
  TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE, 12)
      .owned_by(1)
      .in_planet_orbit(1, 1)
      .with_alive(false)
      .build();

  // 1. Query via ShipRepository indexed spatial query
  auto indexed_alive = ships_repo.find_on_planet(starnum_t{1}, planetnum_t{1},
                                                 /*alive_only=*/true);
  auto indexed_all = ships_repo.find_on_planet(starnum_t{1}, planetnum_t{1},
                                               /*alive_only=*/false);

  // 2. Verify results
  test::expect_eq(indexed_alive.size(), 2);
  test::expect_eq(indexed_alive, (std::vector<shipnum_t>{10, 11}));
  test::expect_eq(indexed_all.size(), 3);
  test::expect_eq(indexed_all, (std::vector<shipnum_t>{10, 11, 12}));

  // 3. Query via ShipList::readonly_on_planet
  std::vector<shipnum_t> shiplist_on_planet;
  for (const Ship& s :
       ShipList::readonly_on_planet(ctx.em, starnum_t{1}, planetnum_t{1})) {
    shiplist_on_planet.push_back(s.number());
  }
  test::expect_eq(indexed_alive, shiplist_on_planet);

  // 4. Verify ShipList Scope iteration at planet scope
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

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
  TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER, 20)
      .owned_by(1)
      .in_star_orbit(1)
      .with_alive(true)
      .build();

  TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER, 21)
      .owned_by(1)
      .docked_to(20, 1)
      .with_alive(true)
      .build();

  TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER, 22)
      .owned_by(1)
      .docked_to(20, 1)
      .with_alive(true)
      .build();

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
