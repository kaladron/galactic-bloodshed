// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import commands;
import std;

#include <cassert>

int main() {
  // Initialize database
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // Create two test races - one giving, one receiving
  Race race1{};
  race1.Playernum = 1;
  race1.governor[0].active = true;
  race1.name = "Giver";
  race1.Guest = false;
  race1.God = false;
  setbit<std::uint64_t>(race1.allied, 2U);  // Mutually allied with race 2

  Race race2{};
  race2.Playernum = 2;
  race2.governor[0].active = true;
  race2.name = "Receiver";
  race2.Guest = false;
  race2.God = false;
  setbit<std::uint64_t>(race2.allied, 1U);  // Mutually allied with race 1

  JsonStore store(db);
  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  // Clear cache so EntityManager picks up races from database
  em.clear_cache();

  // Create a test star
  star_struct star_data{};
  star_data.star_id = 0;
  star_data.governor[0] = 0;
  star_data.name = "TestStar";
  star_data.xpos = 100.0;
  star_data.ypos = 100.0;
  star_data.pnames = {"TestPlanet"};  // Star has 1 planet
  Star star{star_data};
  star.AP(0) = 100;  // Give race 1 (index 0) enough action points
  setbit<std::uint64_t>(star.explored(), 1U);  // Mark explored by race 1
  StarRepository stars_repo(store);
  stars_repo.save(star);
  const starnum_t star_id = star_data.star_id;

  // Create a test planet
  Planet planet{};
  planet.star_id() = star_id;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  planet.xpos() = 0.0;
  planet.ypos() = 0.0;
  PlanetRepository planets_repo(store);
  planets_repo.save(planet);

  // Create a test ship owned by race 1
  Ship ship{};
  ship.owner() = 1;
  ship.governor() = 0;
  ship.type() = ShipType::OTYPE_PROBE;
  ship.alive() = 1;
  ship.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship.storbits() = star_id;
  ship.pnumorbits() = 0;
  ship.popn() = 0;     // No crew (required for transfer)
  ship.troops() = 0;   // No troops (required for transfer)
  ship.ships() = 0;    // No loaded ships (required for transfer)
  ShipRepository ships_repo(store);
  ships_repo.save(ship);
  const shipnum_t ship_id = ship.number();

  // Flush entities to ensure they're in the database
  em.flush_all();

  // Create GameObj for testing
  GameObj g(em);
  g.player = 1;
  g.governor = 0;
  g.race = em.peek_race(1);
  g.level = ScopeLevel::LEVEL_PLAN;
  g.snum = star_id;
  g.pnum = 0;

  // Test: Give ship to another player
  {
    // Debug: Check what races exist
    std::println("Available races:");
    for (auto race_handle : RaceList(em)) {
      const auto& race = race_handle.read();
      std::println("  Player {}: {}", race.Playernum, race.name);
    }
    
    command_t argv = {"give", "Receiver", std::to_string(ship_id)};
    GB::commands::give(argv, g);

    // Debug: Check for error messages
    std::string result = g.out.str();
    if (!result.empty()) {
      std::println("Give command output: {}", result);
    }
    
    // The give command uses notify() which sends to connected clients,
    // not g.out. In tests, we verify success by checking the database.

    // Verify ship ownership changed
    em.clear_cache();
    const auto* ship_verify = em.peek_ship(ship_id);
    assert(ship_verify);
    std::println("Ship owner: {} (expected 2)", ship_verify->owner());
    std::println("Ship governor: {} (expected 0)", ship_verify->governor());
    assert(ship_verify->owner() == 2);
    assert(ship_verify->governor() == 0);  // Given to leader

    // Verify planet exploration bit set for receiver
    const auto* planet_verify = em.peek_planet(star_id, 0);
    assert(planet_verify);
    assert(planet_verify->info(1).explored == 1);  // Race 2 (index 1)

    // Verify star exploration bit set for receiver
    const auto* star_verify = em.peek_star(star_id);
    assert(star_verify);
    assert(isset<std::uint64_t>(star_verify->explored(), 2U));

    std::println("Give command test passed: Ship ownership transferred");
    
    // Clear output for next test
    g.out.str("");
  }

  // Test: Try to give ship from non-governor (should fail)
  {
    // Create another ship owned by race 1
    auto ship2_handle = em.create_ship();
    auto& ship2 = *ship2_handle;
    ship2.owner() = 1;
    ship2.governor() = 0;
    ship2.type() = ShipType::OTYPE_PROBE;
    ship2.alive() = 1;
    ship2.whatorbits() = ScopeLevel::LEVEL_PLAN;
    ship2.storbits() = star_id;
    ship2.pnumorbits() = 0;
    ship2.popn() = 0;
    ship2.troops() = 0;
    ship2.ships() = 0;
    const shipnum_t ship2_id = ship2.number();
    std::println("Ship2 ID: {}", ship2_id);
    
    // Ship is auto-saved when handle goes out of scope
    em.clear_cache();

    // Try as non-leader governor
    g.governor = 1;

    command_t argv = {"give", "Receiver", std::to_string(ship2_id)};
    GB::commands::give(argv, g);

    std::string result = g.out.str();
    std::println("Non-leader output: {}", result);
    assert(result.find("not authorized") != std::string::npos);
    g.out.str("");

    // Verify ship ownership unchanged
    em.clear_cache();
    const auto* ship2_verify = em.peek_ship(ship2_id);
    assert(ship2_verify);
    std::println("Ship2 owner: {} (expected 1)", ship2_verify->owner());
    assert(ship2_verify->owner() == 1);  // Still owned by race 1

    std::println("Give command test passed: Non-governor cannot give");
  }

  // Test: Try to give ship with crew (should fail unless God)
  {
    g.governor = 0;  // Reset to leader

    // Create ship with crew
    auto ship3_handle = em.create_ship();
    auto& ship3 = *ship3_handle;
    ship3.owner() = 1;
    ship3.governor() = 0;
    ship3.type() = ShipType::OTYPE_PROBE;
    ship3.alive() = 1;
    ship3.whatorbits() = ScopeLevel::LEVEL_PLAN;
    ship3.storbits() = star_id;
    ship3.pnumorbits() = 0;
    ship3.popn() = 10;  // Has crew
    ship3.troops() = 0;
    ship3.ships() = 0;
    const shipnum_t ship3_id = ship3.number();
    
    // Ship is auto-saved when handle goes out of scope
    em.clear_cache();

    command_t argv = {"give", "Receiver", std::to_string(ship3_id)};
    GB::commands::give(argv, g);

    std::string result = g.out.str();
    assert(result.find("crew/mil on board") != std::string::npos);
    g.out.str("");

    // Verify ship ownership unchanged
    const auto* ship3_verify = em.peek_ship(ship3_id);
    assert(ship3_verify);
    assert(ship3_verify->owner() == 1);  // Still owned by race 1

    std::println("Give command test passed: Cannot give ship with crew");
  }

  std::println("\nAll give command tests passed!");
  return 0;
}
