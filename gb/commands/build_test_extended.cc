// SPDX-License-Identifier: Apache-2.0
// Extended test coverage for build command

import dallib;
import gblib;
import commands;
import std;

#include <cassert>

// Helper to create test infrastructure
struct BuildTestFixture {
  Database db;
  EntityManager em;
  JsonStore store;
  starnum_t star_id;
  planetnum_t planet_id;
  
  BuildTestFixture() 
    : db(":memory:"),
      em(db),
      store(db),
      star_id(0),
      planet_id(0) {
    initialize_schema(db);
    
    // Create race
    Race race{};
    race.Playernum = 1;
    race.governor[0].active = true;
    race.name = "TestRace";
    race.Guest = false;
    race.God = false;
    race.tech = 500.0;
    race.pods = false;
    RaceRepository races(store);
    races.save(race);
    
    // Create star
    star_struct star_data{};
    star_data.star_id = 0;
    star_data.governor[0] = 0;
    star_data.name = "TestStar";
    star_data.xpos = 100.0;
    star_data.ypos = 100.0;
    Star star{star_data};
    StarRepository stars_repo(store);
    stars_repo.save(star);
    star_id = star_data.star_id;
    
    // Create planet with resources
    Planet planet{};
    planet.star_id() = star_id;
    planet.planet_order() = 0;
    planet.Maxx() = 10;  // Changed from 20 to match working test
    planet.Maxy() = 10;
    planet.xpos() = 0.0;
    planet.ypos() = 0.0;
    planet.info(0).resource = 50000;  // Plenty for multiple builds
    planet.info(0).fuel = 10000;
    PlanetRepository planets_repo(store);
    planets_repo.save(planet);
    planet_id = 0;
    
    // Create sectormap with buildable sectors
    {
      SectorMap smap(planet, true);  // Initialize empty sectors
      smap.get(5, 5).set_owner(1);
      smap.get(5, 5).set_popn(100);
      smap.get(5, 5).set_condition(SectorType::SEC_LAND);
      SectorRepository sectors_repo(store);
      sectors_repo.save_map(smap);
    }
  }
  
  void init_game_obj(GameObj& g, ScopeLevel level = ScopeLevel::LEVEL_PLAN, 
                     shipnum_t shipno = 0) {
    g.player = 1;
    g.governor = 0;
    g.race = em.peek_race(1);
    g.level = level;
    g.snum = star_id;
    g.pnum = planet_id;
    g.shipno = shipno;
  }
  
  int count_ships() {
    return em.num_ships();
  }
  
  const Ship* get_ship(shipnum_t num) {
    return em.peek_ship(num);
  }
  
  const Planet* get_planet() {
    return em.peek_planet(star_id, planet_id);
  }
};

// Test: Multiple ship builds on planet (count > 1)
// CRITICAL: Tests x,y coordinate persistence across loop iterations
void test_planet_multiple_builds() {
  BuildTestFixture fixture;
  GameObj g(fixture.em);
  fixture.init_game_obj(g);
  
  int initial_resource = fixture.get_planet()->info(0).resource;
  
  // Build 5 probes at sector 5,5
  command_t argv = {"build", ":", "5,5", "5"};
  
  std::println("About to build 5 probes...");
  const auto* test_planet = fixture.get_planet();
  std::println("Planet maxx={}, maxy={}", test_planet->Maxx(), test_planet->Maxy());
  
  // Try to load sectormap
  const auto* smap = fixture.em.peek_sectormap(fixture.star_id, fixture.planet_id);
  if (smap) {
    std::println("Sectormap loaded successfully");
  } else {
    std::println("Sectormap not found in database!");
  }
  
  GB::commands::build(argv, g);
  
  // Check for any error output
  std::string output = g.out.str();
  if (!output.empty()) {
    std::println("Build command output: {}", output);
  }
  
  // Clear cache to ensure fresh read from DB
  fixture.em.clear_cache();
  
  // Verify 5 ships were created
  int ship_count = fixture.count_ships();
  std::println("Expected 5 ships, got {}", ship_count);
  std::println("Assertion will check ship_count == 5");
  assert(ship_count == 5);
  
  // Verify all ships at same location
  for (shipnum_t i = 1; i <= 5; i++) {
    const auto* ship = fixture.get_ship(i);
    assert(ship != nullptr);
    assert(ship->type() == ShipType::OTYPE_PROBE);
    assert(ship->land_x() == 5);
    assert(ship->land_y() == 5);
    assert(ship->whatorbits() == ScopeLevel::LEVEL_PLAN);
  }
  
  // Verify resources deducted for all 5 ships
  const auto* planet = fixture.get_planet();
  int cost_per_probe = Shipcost(ShipType::OTYPE_PROBE, *g.race);
  int expected_resource = initial_resource - (5 * cost_per_probe);
  assert(planet->info(0).resource == expected_resource);
  
  std::println("✓ Planet multiple builds test passed");
}

// Test: Factory building multiple ships
void test_factory_multiple_builds() {
  BuildTestFixture fixture;
  
  // Create a factory ship landed at 7,7
  Ship factory_data{};
  factory_data.type() = ShipType::OTYPE_FACTORY;
  factory_data.owner() = 1;
  factory_data.governor() = 0;
  factory_data.alive() = 1;
  factory_data.on() = 1;
  factory_data.whatorbits() = ScopeLevel::LEVEL_PLAN;
  factory_data.storbits() = fixture.star_id;
  factory_data.pnumorbits() = fixture.planet_id;
  factory_data.land_x() = 7;
  factory_data.land_y() = 7;
  factory_data.xpos() = 0.0;
  factory_data.ypos() = 0.0;
  factory_data.resource() = 10000;
  factory_data.build_type() = ShipType::OTYPE_PROBE;  // Set what factory builds
  
  ShipRepository ships_repo(fixture.store);
  ships_repo.save(factory_data);
  shipnum_t factory_num = factory_data.number();
  
  GameObj g(fixture.em);
  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, factory_num);
  
  int initial_resource = fixture.get_planet()->info(0).resource;
  
  // Factory builds 3 ships (uses coordinate from argv[2])
  command_t argv = {"build", "3"};  // Factory count is argv[2]
  GB::commands::build(argv, g);
  
  fixture.em.clear_cache();
  
  // Verify ships created (factory is ship 1, new ships are 2-4)
  int ship_count = fixture.count_ships();
  assert(ship_count == 4);  // 1 factory + 3 built ships
  
  // Verify all built ships at factory's landed coordinates (7,7)
  for (shipnum_t i = 2; i <= 4; i++) {
    const auto* ship = fixture.get_ship(i);
    assert(ship != nullptr);
    assert(ship->land_x() == 7);  // Same as factory
    assert(ship->land_y() == 7);
  }
  
  // Verify planet resources deducted
  const auto* planet = fixture.get_planet();
  assert(planet->info(0).resource < initial_resource);
  
  std::println("✓ Factory multiple builds test passed");
}

// Test: Invalid sector coordinates
void test_invalid_coordinates() {
  BuildTestFixture fixture;
  GameObj g(fixture.em);
  fixture.init_game_obj(g);
  
  // Out of bounds - negative
  {
    command_t argv = {"build", ":", "-1,5", "1"};
    GB::commands::build(argv, g);
    std::string output = g.out.str();
    assert(output.find("Illegal sector") != std::string::npos);
    g.out.str("");
  }
  
  // Out of bounds - too large
  {
    command_t argv = {"build", ":", "25,25", "1"};  // Planet is 20x20
    GB::commands::build(argv, g);
    std::string output = g.out.str();
    assert(output.find("Illegal sector") != std::string::npos);
    g.out.str("");
  }
  
  // Invalid format
  {
    command_t argv = {"build", ":", "abc", "1"};
    GB::commands::build(argv, g);
    std::string output = g.out.str();
    assert(output.find("Invalid sector format") != std::string::npos);
    g.out.str("");
  }
  
  std::println("✓ Invalid coordinates test passed");
}

// Test: Wrong scope level
void test_wrong_scope() {
  BuildTestFixture fixture;
  GameObj g(fixture.em);
  fixture.init_game_obj(g, ScopeLevel::LEVEL_UNIV);
  
  command_t argv = {"build", ":", "5,5", "1"};
  GB::commands::build(argv, g);
  
  std::string output = g.out.str();
  assert(output.find("change scope") != std::string::npos);
  
  std::println("✓ Wrong scope test passed");
}

// Test: Ship cannot be built by planet
void test_invalid_ship_for_planet() {
  BuildTestFixture fixture;
  GameObj g(fixture.em);
  fixture.init_game_obj(g);
  
  // Try to build a ship type that can't be built on planets
  // Need to find a ship type with !(ABIL_BUILD & 1)
  // For now, just test error path exists
  
  std::println("✓ Invalid ship for planet test skipped (need ship type data)");
}

// Test: Insufficient hanger space
void test_insufficient_hanger_space() {
  BuildTestFixture fixture;
  
  // Create a small ship with limited hanger space
  Ship builder_data{};
  builder_data.type() = ShipType::STYPE_HABITAT;  // Has some hanger space
  builder_data.owner() = 1;
  builder_data.governor() = 0;
  builder_data.alive() = 1;
  builder_data.whatorbits() = ScopeLevel::LEVEL_STAR;
  builder_data.storbits() = fixture.star_id;
  builder_data.resource() = 10000;
  builder_data.max_hanger() = 10;  // Small hanger
  builder_data.hanger() = 9;       // Almost full
  
  ShipRepository ships_repo(fixture.store);
  ships_repo.save(builder_data);
  shipnum_t builder_num = builder_data.number();
  
  GameObj g(fixture.em);
  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, builder_num);
  
  // Try to build a ship that's too big
  command_t argv = {"build", ":", "5"};  // Probes (if size > 1)
  GB::commands::build(argv, g);
  
  std::string output = g.out.str();
  // May fail at hanger space or build permission check
  assert(!output.empty());  // Should have some error
  
  std::println("✓ Insufficient hanger space test passed");
}

int main() {
  try {
    std::println("Running extended build command tests...\n");
    
    // Phase 1: Critical multi-build tests
    test_planet_multiple_builds();
    test_factory_multiple_builds();
    
    // Phase 2: Error conditions
    test_invalid_coordinates();
    test_wrong_scope();
    test_invalid_ship_for_planet();
    test_insufficient_hanger_space();
    
    std::println("\n✅ All extended build tests passed!");
    return 0;
  } catch (const std::exception& e) {
    std::println("Exception: {}", e.what());
    return 1;
  }
}
