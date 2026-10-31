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

  // Star 1 already exists in with_standard_universe(); add ships in orbit
  const auto s1 = TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE)
                      .owned_by(1)
                      .in_star_orbit(1)
                      .with_alive(true)
                      .build();

  const auto s2 = TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE)
                      .owned_by(1)
                      .in_star_orbit(1)
                      .with_alive(true)
                      .build();
  ctx.em.mutate_ship(s2, [&](Ship& s) { ctx.em.kill_ship(1, s); });

  const auto s3 = TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE)
                      .owned_by(2)
                      .in_star_orbit(1)
                      .with_alive(true)
                      .build();

  // 1. Query via ShipRepository indexed spatial queries
  auto indexed_alive = ships_repo.find_in_star(starnum_t{1});

  test::expect_eq(indexed_alive.size(), 2);
  test::expect_eq(indexed_alive, (std::vector<shipnum_t>{s1, s3}));

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

  // Planet (Star 1, Planet 1) already exists in with_standard_universe()
  const auto p1 = TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE)
                      .owned_by(1)
                      .in_planet_orbit(1, 1)
                      .with_alive(true)
                      .build();

  const auto p2 = TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE)
                      .owned_by(1)
                      .in_planet_orbit(1, 1)
                      .with_alive(true)
                      .build();

  // Destroyed ship on same planet (should be excluded)
  const auto p3 = TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE)
                      .owned_by(1)
                      .in_planet_orbit(1, 1)
                      .with_alive(true)
                      .build();
  ctx.em.mutate_ship(p3, [&](Ship& s) { ctx.em.kill_ship(1, s); });

  // 1. Query via ShipRepository indexed spatial query
  auto indexed_alive = ships_repo.find_on_planet(starnum_t{1}, planetnum_t{1});

  // 2. Verify results
  test::expect_eq(indexed_alive.size(), 2);
  test::expect_eq(indexed_alive, (std::vector<shipnum_t>{p1, p2}));

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

  // Carrier ship contains docked fighters
  const auto carrier_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
                              .owned_by(1)
                              .in_star_orbit(1)
                              .with_alive(true)
                              .build();

  const auto f1_id = TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
                         .owned_by(1)
                         .docked_to(carrier_id, 1)
                         .with_alive(true)
                         .build();

  const auto f2_id = TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
                         .owned_by(1)
                         .docked_to(carrier_id, 1)
                         .with_alive(true)
                         .build();

  // 1. Query via ShipRepository indexed hangar query
  auto indexed_hangar = ships_repo.find_in_hangar(carrier_id);

  // 2. Verify results
  test::expect_eq(indexed_hangar.size(), 2);
  test::expect_eq(indexed_hangar, (std::vector<shipnum_t>{f1_id, f2_id}));

  // 3. Verify ShipList::readonly_in_carrier
  std::vector<shipnum_t> shiplist_carrier;
  for (const Ship& s : ShipList::readonly_in_carrier(ctx.em, carrier_id)) {
    shiplist_carrier.push_back(s.number());
  }
  test::expect_eq(indexed_hangar, shiplist_carrier);

  // 4. Verify ShipList::in_carrier (mutable)
  std::vector<shipnum_t> shiplist_carrier_mutable;
  for (auto handle : ShipList::in_carrier(ctx.em, carrier_id)) {
    shiplist_carrier_mutable.push_back(handle->number());
  }
  test::expect_eq(indexed_hangar, shiplist_carrier_mutable);

  std::println(std::cout, "✓ Hangar docked query parity verified");
}

void test_empire_and_global_parity(TestContext& ctx) {
  JsonStore store(ctx.db);
  ShipRepository ships_repo(store);

  // Query player 1 ships via index
  auto p1_indexed = ships_repo.find_by_owner(player_t{1});

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
  ctx.with_standard_universe();
  test_empty_universe_parity(ctx);
  test_star_spatial_parity(ctx);
  test_planet_spatial_parity(ctx);
  test_hangar_docked_parity(ctx);
  test_empire_and_global_parity(ctx);

  std::println(std::cout, "\nAll Ship spatial parity tests passed!");
  return 0;
}
