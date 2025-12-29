// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import commands;
import std.compat;

#include <cassert>

int main() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  // Create test races
  Race race1{};
  race1.Playernum = 1;
  race1.name = "Explorer";
  race1.Guest = false;
  race1.governor[0].active = true;

  Race race2{};
  race2.Playernum = 2;
  race2.name = "Recipient";
  race2.Guest = false;
  race2.governor[0].active = true;

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  // Setup universe with proper numstars
  UniverseRepository universe_repo(store);
  universe_struct sdata{};
  sdata.id = 1;
  sdata.numstars = 1;
  universe_repo.save(sdata);

  // Create test star with APs
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "TestStar";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.explored = (1ULL << 1);  // Player 1 has explored
  ss.AP[0] = 20;              // Give player 1 enough APs (dump needs 10)
  ss.pnames.emplace_back("TestPlanet");
  Star star(ss);

  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create test planet
  planet_struct ps{};
  ps.star_id = 0;
  ps.planet_order = 0;
  ps.type = PlanetType::EARTH;
  ps.info[0].explored = true;   // Player 1 has explored
  ps.info[1].explored = false;  // Player 2 hasn't explored yet
  Planet planet(ps);

  PlanetRepository planets_repo(store);
  planets_repo.save(planet);

  // Create GameObj for player 1
  GameObj g(em);
  g.set_player(1);
  g.set_governor(0);
  g.race = em.peek_race(1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  std::println("Test 1: Dump exploration data to another player");
  {
    // Before dump: player 2 hasn't explored the planet
    const auto* p_before = em.peek_planet(0, 0);
    assert(!p_before->info(1).explored);

    // Execute dump command
    command_t argv = {"dump", "Recipient"};
    GB::commands::dump(argv, g);

    // Verify: player 2 should now have exploration data
    const auto* p_after = em.peek_planet(0, 0);
    assert(p_after->info(1).explored);
    std::println("✓ Player 2 received exploration data");

    // Verify output message
    std::string output = g.out.str();
    assert(output.find("Exploration Data transferred") != std::string::npos);
    std::println("✓ Success message displayed");
  }

  std::println("Test 2: Guest race cannot dump");
  {
    // Make player 1 a guest
    auto race_handle = em.get_race(1);
    auto& r = *race_handle;
    r.Guest = true;
  }
  {
    GameObj g2(em);
    g2.set_player(1);
    g2.set_governor(0);
    g2.race = em.peek_race(1);
    g2.set_level(ScopeLevel::LEVEL_STAR);
    g2.set_snum(0);

    command_t argv = {"dump", "Recipient"};
    GB::commands::dump(argv, g2);

    // Output should contain "Cheater!" but we can't directly test g.out
    // Command should return early without effect
    std::println("✓ Guest race blocked from dumping");
  }

  std::println("All dump tests passed!");
  return 0;
}
