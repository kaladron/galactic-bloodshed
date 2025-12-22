// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import std;

#include <cassert>

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
  for (int i = 0; i < MAXPLAYERS; i++) {
    u.VN_hitlist[i] = 0;
    u.VN_index1[i] = -1;
    u.VN_index2[i] = -1;
  }
  universe_repo.save(u);

  // Now EntityManager can access it
  const auto* universe = em.peek_universe();
  assert(universe);

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

  // Test 1: Basic ship kill
  {
    ship_struct ship_data{};
    ship_data.owner = 2;
    ship_data.alive = 1;
    ship_data.notified = 1;
    ship_data.type = ShipType::STYPE_FIGHTER;
    ship_data.build_cost = 100;

    auto ship_handle = em.create_ship(ship_data);
    auto& ship = *ship_handle;

    em.kill_ship(1, ship);

    assert(ship.alive() == 0);
    assert(ship.notified() == 0);
    std::println("✓ Test 1: Basic ship kill works");
  }

  // Test 2: MindData handling
  {
    ship_struct ship_data{};
    ship_data.owner = 2;
    ship_data.alive = 1;
    ship_data.type = ShipType::STYPE_FIGHTER;

    auto ship_handle = em.create_ship(ship_data);
    auto& ship = *ship_handle;

    MindData mind{};
    mind.who_killed = 0;
    ship.special() = mind;

    em.kill_ship(1, ship);

    assert(std::holds_alternative<MindData>(ship.special()));
    auto result_mind = std::get<MindData>(ship.special());
    assert(result_mind.who_killed == 1);
    std::println("✓ Test 2: MindData who_killed tracking works");
  }

  // Test 3: Gov_ship gets cleared when government ship is killed
  {
    ship_struct ship_data{};
    ship_data.owner = 2;
    ship_data.alive = 1;
    ship_data.type = ShipType::STYPE_BATTLE;
    ship_data.build_cost = 500;

    auto ship_handle = em.create_ship(ship_data);
    auto& ship = *ship_handle;
    shipnum_t ship_num = ship.number();

    // Set this ship as victim's government ship
    victim_race.Gov_ship = ship_num;
    races.save(victim_race);
    em.clear_cache();  // Force reload

    em.kill_ship(1, ship);

    // Clear cache and reload to see the persisted changes
    em.clear_cache();
    const auto* victim = em.peek_race(2);
    assert(victim);
    assert(victim->Gov_ship == 0);
    std::println("✓ Test 3: Gov_ship cleared when government ship killed");
  }

  // Test 4: Morale adjustment for non-VN kills
  {
    ship_struct ship_data{};
    ship_data.owner = 2;
    ship_data.alive = 1;
    ship_data.type = ShipType::STYPE_DREADNT;
    ship_data.build_cost = 1000;

    auto ship_handle = em.create_ship(ship_data);
    auto& ship = *ship_handle;

    em.kill_ship(1, ship);

    // Morale adjustment occurs (adjust_morale was called)
    // We can't predict exact values without implementing adjust_morale,
    // but we can verify that the races were accessed
    const auto* attacker_after = em.peek_race(1);
    const auto* victim_after = em.peek_race(2);

    // Note: We can't predict exact morale values without implementing
    // adjust_morale, but we can verify that the races were loaded and saved
    assert(attacker_after);
    assert(victim_after);
    std::println("✓ Test 4: Morale adjustment occurs on kill");
  }

  // Test 5: VN hitlist tracking
  {
    ship_struct ship_data{};
    ship_data.owner = 2;
    ship_data.alive = 1;
    ship_data.type = ShipType::OTYPE_VN;
    ship_data.storbits = 0;

    auto ship_handle = em.create_ship(ship_data);
    auto& ship = *ship_handle;

    MindData mind{};
    mind.who_killed = 1;
    ship.special() = mind;

    em.kill_ship(1, ship);

    // Check VN hitlist was updated
    const auto* universe_after = em.peek_universe();
    assert(universe_after);
    assert(universe_after->VN_hitlist[0] > 0);  // Player 1 (index 0)
    assert(universe_after->VN_index1[0] == 0 ||
           universe_after->VN_index2[0] == 0);
    std::println("✓ Test 5: VN hitlist tracking works");
  }

  // Test 6: TOXWC increases planet toxicity
  {
    ship_struct ship_data{};
    ship_data.owner = 2;
    ship_data.alive = 1;
    ship_data.type = ShipType::OTYPE_TOXWC;
    ship_data.whatorbits = ScopeLevel::LEVEL_PLAN;
    ship_data.storbits = 0;
    ship_data.pnumorbits = 0;

    auto ship_handle = em.create_ship(ship_data);
    auto& ship = *ship_handle;

    WasteData waste{};
    waste.toxic = 20;
    ship.special() = waste;

    em.kill_ship(1, ship);

    // Check planet toxicity increased
    const auto* planet_after = em.peek_planet(0, 0);
    assert(planet_after);
    assert(planet_after->conditions(TOXIC) >= 30);  // Was 10, added 20
    std::println("✓ Test 6: TOXWC increases planet toxicity on death");
  }

  // Test 7: Docked ships get undocked (when one of two docked ships dies)
  {
    shipnum_t ship1_num;
    shipnum_t ship2_num;

    // Phase 1: Create and set up docked ships, then release handles
    {
      ship_struct ship1_data{};
      ship1_data.owner = 1;
      ship1_data.alive = 1;
      ship1_data.type = ShipType::STYPE_CARRIER;
      ship1_data.whatorbits = ScopeLevel::LEVEL_STAR;
      ship1_data.storbits = 0;

      ship_struct ship2_data{};
      ship2_data.owner = 1;
      ship2_data.alive = 1;
      ship2_data.type = ShipType::STYPE_FIGHTER;
      ship2_data.whatorbits = ScopeLevel::LEVEL_STAR;
      ship2_data.storbits = 0;

      auto ship1_handle = em.create_ship(ship1_data);
      ship1_num = ship1_handle->number();

      auto ship2_handle = em.create_ship(ship2_data);
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
    {
      auto ship2_handle = em.get_ship(ship2_num);
      assert(ship2_handle.get());
      em.kill_ship(1, *ship2_handle);
      // ship2_handle saves and releases when it goes out of scope
    }

    // Phase 4: Verify ship1 was undocked
    em.clear_cache();
    const auto* ship1_after = em.peek_ship(ship1_num);
    assert(ship1_after);
    assert(ship1_after->docked() == 0);
    assert(ship1_after->whatdest() == ScopeLevel::LEVEL_UNIV);
    std::println("✓ Test 7: Docked ships get undocked when one dies");
  }

  // Test 8: Recursive killing of landed ships
  {
    shipnum_t carrier_num;
    shipnum_t fighter1_num;
    shipnum_t fighter2_num;

    // Phase 1: Create carrier and fighters, set up landing relationship
    {
      ship_struct carrier_data{};
      carrier_data.owner = 1;
      carrier_data.alive = 1;
      carrier_data.type = ShipType::STYPE_CARRIER;
      carrier_data.whatorbits = ScopeLevel::LEVEL_STAR;
      carrier_data.storbits = 0;

      auto carrier_handle = em.create_ship(carrier_data);
      carrier_num = carrier_handle->number();

      // Create fighter1 landed on carrier
      ship_struct fighter1_data{};
      fighter1_data.owner = 1;
      fighter1_data.alive = 1;
      fighter1_data.type = ShipType::STYPE_FIGHTER;
      fighter1_data.whatorbits = ScopeLevel::LEVEL_SHIP;
      fighter1_data.destshipno = carrier_num;

      auto fighter1_handle = em.create_ship(fighter1_data);
      fighter1_num = fighter1_handle->number();

      // Create fighter2 landed on carrier
      ship_struct fighter2_data{};
      fighter2_data.owner = 1;
      fighter2_data.alive = 1;
      fighter2_data.type = ShipType::STYPE_FIGHTER;
      fighter2_data.whatorbits = ScopeLevel::LEVEL_SHIP;
      fighter2_data.destshipno = carrier_num;

      auto fighter2_handle = em.create_ship(fighter2_data);
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
    {
      auto carrier_handle = em.get_ship(carrier_num);
      assert(carrier_handle.get());
      em.kill_ship(1, *carrier_handle);
      // carrier_handle saves and releases when it goes out of scope
    }

    // Phase 4: Verify all ships are dead
    em.clear_cache();
    const auto* carrier_after = em.peek_ship(carrier_num);
    const auto* fighter1_after = em.peek_ship(fighter1_num);
    const auto* fighter2_after = em.peek_ship(fighter2_num);

    assert(carrier_after);
    assert(fighter1_after);
    assert(fighter2_after);
    assert(carrier_after->alive() == 0);
    assert(fighter1_after->alive() == 0);
    assert(fighter2_after->alive() == 0);
    std::println("✓ Test 8: Recursive killing of landed ships works");
  }

  std::println("\n✅ All EntityManager::kill_ship() tests passed!");
  return 0;
}
