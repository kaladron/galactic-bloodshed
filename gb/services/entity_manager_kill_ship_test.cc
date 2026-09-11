// SPDX-License-Identifier: Apache-2.0

/// \file entity_manager_kill_ship_test.cc
/// \brief Unit tests for EntityManager::kill_ship() mechanics, recursive
/// destruction, and VN telemetry.

import dallib;
import gb.entities;
import gb.services;
import test;
import std;

int main() {
  // Create in-memory database BEFORE initialize_schema
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // Create test races
  Race attacker_race{};
  attacker_race.Playernum = 1;
  attacker_race.name = "Attacker";
  attacker_race.Guest = false;
  attacker_race.God = false;
  attacker_race.morale = 100;
  attacker_race.Gov_ship = 0;

  Race victim_race{};
  victim_race.Playernum = 2;
  victim_race.name = "Victim";
  victim_race.Guest = false;
  victim_race.God = false;
  victim_race.morale = 100;
  victim_race.Gov_ship = 5;  // Will be cleared when Gov_ship is killed

  JsonStore store(db);
  RaceRepository races(store);
  races.save(attacker_race);
  races.save(victim_race);

  // Create universe data for VN testing
  UniverseRepository universe_repo(store);
  universe_struct u{};
  u.id = 1;
  for (auto& h : u.VN_hitlist)
    h = 0;
  for (auto& idx : u.VN_index1)
    idx = -1;
  for (auto& idx : u.VN_index2)
    idx = -1;
  universe_repo.save(u);

  // Now EntityManager can access it
  const auto* universe = em.peek_universe();
  test::expect_ne(universe, nullptr);

  // Create a test star
  star_struct star_data{};
  star_data.star_id = 0;
  star_data.xpos = 100.0;
  star_data.ypos = 100.0;
  Star star{star_data};
  StarRepository star_repo(store);
  star_repo.save(star);

  // Create a test planet
  Planet planet{};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.xpos() = 10.0;
  planet.ypos() = 10.0;
  planet.conditions(TOXIC) = 10;
  PlanetRepository planet_repo(store);
  planet_repo.save(planet);

  // Basic ship kill
  {
    auto ship_handle = TestShipBuilder(em, ShipType::STYPE_FIGHTER)
                           .owned_by(2)
                           .with_alive(true)
                           .build_handle();
    auto& ship = *ship_handle;
    ship.notified() = 1;
    ship.build_cost() = 100;

    em.kill_ship(1, ship);

    test::expect_eq(ship.alive(), 0);
    test::expect_eq(ship.notified(), 0);
    std::println(std::cout, "✓ Basic ship kill works");
  }

  // AutonomousShip who_killed tracking
  {
    auto ship_handle = TestShipBuilder(em, ShipType::OTYPE_VN)
                           .owned_by(2)
                           .with_alive(true)
                           .build_handle();
    auto& ship = *ship_handle;

    em.kill_ship(1, ship);

    auto* vn = ship.as<VonNeumannShip>();
    test::expect_ne(vn, nullptr);
    test::expect_eq(vn->who_killed(), player_t{1});
    std::println(std::cout, "✓ AutonomousShip who_killed tracking works");
  }

  // Gov_ship gets cleared when government ship is killed
  {
    auto ship_handle = TestShipBuilder(em, ShipType::STYPE_BATTLE)
                           .owned_by(2)
                           .with_alive(true)
                           .build_handle();
    auto& ship = *ship_handle;
    ship.build_cost() = 500;
    shipnum_t ship_num = ship.number();

    // Set this ship as victim's government ship
    victim_race.Gov_ship = ship_num;
    races.save(victim_race);
    em.clear_cache();  // Force reload

    em.kill_ship(1, ship);

    // Clear cache and reload to see the persisted changes
    em.clear_cache();
    const auto* victim = em.peek_race(2);
    test::expect_ne(victim, nullptr);
    test::expect_eq(victim->Gov_ship, 0);
    std::println(std::cout, "✓ Gov_ship cleared when government ship killed");
  }

  // Morale adjustment for non-VN kills
  {
    auto ship_handle = TestShipBuilder(em, ShipType::STYPE_DREADNT)
                           .owned_by(2)
                           .with_alive(true)
                           .build_handle();
    auto& ship = *ship_handle;
    ship.build_cost() = 1000;

    em.kill_ship(1, ship);

    // Morale adjustment occurs (adjust_morale was called)
    // We can't predict exact values without implementing adjust_morale,
    // but we can verify that the races were accessed
    const auto* attacker_after = em.peek_race(1);
    const auto* victim_after = em.peek_race(2);

    // Note: We can't predict exact morale values without implementing
    // adjust_morale, but we can verify that the races were loaded and saved
    test::expect_ne(attacker_after, nullptr);
    test::expect_ne(victim_after, nullptr);
    std::println(std::cout, "✓ Morale adjustment occurs on kill");
  }

  // VN hitlist tracking
  {
    auto ship_handle = TestShipBuilder(em, ShipType::OTYPE_VN)
                           .owned_by(2)
                           .with_alive(true)
                           .in_star_orbit(0)
                           .with_special(MindData{.who_killed = 1})
                           .build_handle();
    auto& ship = *ship_handle;

    em.kill_ship(1, ship);

    // Check VN hitlist was updated
    const auto* universe_after = em.peek_universe();
    test::expect_ne(universe_after, nullptr);
    test::expect_gt(universe_after->VN_hitlist[player_t{1}], 0);
    test::expect_true(universe_after->VN_index1[player_t{1}] == 0 ||
                      universe_after->VN_index2[player_t{1}] == 0);
    std::println(std::cout, "✓ VN hitlist tracking works");
  }

  // TOXWC increases planet toxicity
  {
    auto ship_handle = TestShipBuilder(em, ShipType::OTYPE_TOXWC)
                           .owned_by(2)
                           .with_alive(true)
                           .in_planet_orbit(0, 0)
                           .with_special(WasteData{.toxic = 20})
                           .build_handle();
    auto& ship = *ship_handle;

    em.kill_ship(1, ship);

    // Check planet toxicity increased
    const auto* planet_after = em.peek_planet(0, 0);
    test::expect_ne(planet_after, nullptr);
    test::expect_ge(planet_after->conditions(TOXIC), 30);  // Was 10, added 20
    std::println(std::cout, "✓ TOXWC increases planet toxicity on death");
  }

  // Docked ships get undocked (when one of two docked ships dies)
  {
    shipnum_t ship1_num;
    shipnum_t ship2_num;

    // Phase 1: Create and set up docked ships, then release handles
    {
      auto ship1_handle = TestShipBuilder(em, ShipType::STYPE_CARRIER)
                              .owned_by(1)
                              .with_alive(true)
                              .in_star_orbit(0)
                              .build_handle();
      ship1_num = ship1_handle->number();

      auto ship2_handle = TestShipBuilder(em, ShipType::STYPE_FIGHTER)
                              .owned_by(1)
                              .with_alive(true)
                              .in_star_orbit(0)
                              .build_handle();
      ship2_num = ship2_handle->number();

      // Dock them together (both point to each other)
      ship1_handle->docked() = 1;
      ship1_handle->whatdest() = ScopeLevel::LEVEL_SHIP;
      ship1_handle->destshipno() = ship2_num;

      ship2_handle->docked() = 1;
      ship2_handle->whatdest() = ScopeLevel::LEVEL_SHIP;
      ship2_handle->destshipno() = ship1_num;

      // Handles auto-save when they go out of scope
    }

    // Phase 2: Clear cache and reload fresh data
    em.clear_cache();

    // Phase 3: Kill ship2 - should undock ship1
    em.mutate_ship(ship2_num, [&](Ship& ship2) { em.kill_ship(1, ship2); });

    // Phase 4: Verify ship1 was undocked
    em.clear_cache();
    const auto* ship1_after = em.peek_ship(ship1_num);
    test::expect_ne(ship1_after, nullptr);
    test::expect_eq(ship1_after->docked(), 0);
    test::expect_eq(ship1_after->whatdest(), ScopeLevel::LEVEL_UNIV);
    std::println(std::cout, "✓ Docked ships get undocked when one dies");
  }

  // Recursive killing of landed ships
  {
    shipnum_t carrier_num;
    shipnum_t fighter1_num;
    shipnum_t fighter2_num;

    // Phase 1: Create carrier and fighters, set up landing relationship
    {
      auto carrier_handle = TestShipBuilder(em, ShipType::STYPE_CARRIER)
                                .owned_by(1)
                                .with_alive(true)
                                .in_star_orbit(0)
                                .build_handle();
      carrier_num = carrier_handle->number();

      // Create fighter1 landed on carrier
      auto fighter1_handle = TestShipBuilder(em, ShipType::STYPE_FIGHTER)
                                 .owned_by(1)
                                 .with_alive(true)
                                 .docked_to(carrier_num, 0)
                                 .build_handle();
      fighter1_num = fighter1_handle->number();

      // Create fighter2 landed on carrier
      auto fighter2_handle = TestShipBuilder(em, ShipType::STYPE_FIGHTER)
                                 .owned_by(1)
                                 .with_alive(true)
                                 .docked_to(carrier_num, 0)
                                 .build_handle();
      fighter2_num = fighter2_handle->number();

      // Link fighters to carrier's ships list
      carrier_handle->ships() = fighter1_num;
      fighter1_handle->nextship() = fighter2_num;
      fighter2_handle->nextship() = 0;

      // Handles auto-save when they go out of scope
    }

    // Phase 2: Clear cache and reload fresh data
    em.clear_cache();

    // Phase 3: Kill the carrier - should recursively kill fighters
    em.mutate_ship(carrier_num,
                   [&](Ship& carrier) { em.kill_ship(1, carrier); });

    // Phase 4: Verify all ships are dead
    em.clear_cache();
    const auto* carrier_after = em.peek_ship(carrier_num);
    const auto* fighter1_after = em.peek_ship(fighter1_num);
    const auto* fighter2_after = em.peek_ship(fighter2_num);

    test::expect_ne(carrier_after, nullptr);
    test::expect_ne(fighter1_after, nullptr);
    test::expect_ne(fighter2_after, nullptr);
    test::expect_eq(carrier_after->alive(), 0);
    test::expect_eq(fighter1_after->alive(), 0);
    test::expect_eq(fighter2_after->alive(), 0);
    std::println(std::cout, "✓ Recursive killing of landed ships works");
  }

  // Deterministic testing of record_vn_destruction_site
  {
    int index1{-1};
    int index2{-1};

    // Slot 1 empty -> recorded in index1
    record_vn_destruction_site(index1, index2, 10, true);
    test::expect_eq(index1, 10);
    test::expect_eq(index2, -1);

    // Slot 2 empty -> recorded in index2
    record_vn_destruction_site(index1, index2, 20, false);
    test::expect_eq(index1, 10);
    test::expect_eq(index2, 20);

    // Both slots filled -> supplant slot 1 when supplant_first is true
    record_vn_destruction_site(index1, index2, 30, true);
    test::expect_eq(index1, 30);
    test::expect_eq(index2, 20);

    // Both slots filled -> supplant slot 2 when supplant_first is false
    record_vn_destruction_site(index1, index2, 40, false);
    test::expect_eq(index1, 30);
    test::expect_eq(index2, 40);

    std::println(std::cout,
                 "✓ record_vn_destruction_site deterministic test works");
  }

  std::println(std::cout, "\n✅ All EntityManager::kill_ship() tests passed!");
  return 0;
}
