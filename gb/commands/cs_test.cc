// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import commands;
import std;

#include <cassert>

int main() {
  // Create in-memory database and initialize schema
  Database db(":memory:");
  initialize_schema(db);

  // Create EntityManager and JsonStore
  EntityManager em(db);
  JsonStore store(db);

  // Create universe with 2 stars
  universe_struct us{};
  us.id = 1;  // Universe is a singleton with ID 1
  us.numstars = 2;
  us.ships = 0;  // No ships at universe level

  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  // Verify universe was saved and can be loaded
  {
    const auto* loaded = em.peek_universe();
    if (!loaded) {
      std::println("ERROR: Universe not found immediately after save!");
      return 1;
    }
    std::println("Universe loaded successfully: {} stars", loaded->numstars);
  }

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.governor[0].active = true;

  RaceRepository races(store);
  races.save(race);

  // Create two test stars
  star_struct ss0{};
  ss0.star_id = 0;
  ss0.name = "Alpha";
  ss0.xpos = 100.0;
  ss0.ypos = 200.0;
  ss0.pnames.emplace_back("AlphaPrime");  // Has 1 planet
  ss0.explored = (1ULL << 1);             // Player 1 has explored
  Star star0(ss0);

  star_struct ss1{};
  ss1.star_id = 1;
  ss1.name = "Beta";
  ss1.xpos = 300.0;
  ss1.ypos = 400.0;
  // No planets (pnames empty)
  ss1.explored = (1ULL << 1);  // Player 1 has explored
  Star star1(ss1);

  StarRepository stars_repo(store);
  stars_repo.save(star0);
  stars_repo.save(star1);

  // Create a test planet at star 0
  Planet planet{PlanetType::EARTH};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.Maxx() = 5;
  planet.Maxy() = 5;
  planet.explored() = true;
  planet.info(0).explored = true;  // Player 1 has explored this planet

  PlanetRepository planets_repo(store);
  planets_repo.save(planet);

  // Create GameObj for command execution
  GameObj g(em);
  g.player = 1;
  g.governor = 0;
  g.race = em.peek_race(1);  // Set race pointer like production

  std::println("Test 1: cs command switches to universe scope");
  {
    g.level = ScopeLevel::LEVEL_STAR;
    g.snum = 0;

    command_t argv = {"cs", "/"};
    GB::commands::cs(argv, g);

    assert(g.level == ScopeLevel::LEVEL_UNIV);
    std::println("    ✓ Switched to universe scope");
  }

  std::println("Test 2: cs command switches to star scope by name");
  {
    g.level = ScopeLevel::LEVEL_UNIV;

    command_t argv = {"cs", "Beta"};
    GB::commands::cs(argv, g);

    assert(g.level == ScopeLevel::LEVEL_STAR);
    assert(g.snum == 1);
    std::println("    ✓ Switched to star Beta (1) scope");
  }

  std::println("Test 3: cs command switches to star scope by name");
  {
    g.level = ScopeLevel::LEVEL_UNIV;

    command_t argv = {"cs", "Alpha"};
    GB::commands::cs(argv, g);

    assert(g.level == ScopeLevel::LEVEL_STAR);
    assert(g.snum == 0);
    std::println("    ✓ Switched to star Alpha (0) scope");
  }

  std::println("Test 4: cs command rejects invalid star name");
  {
    g.level = ScopeLevel::LEVEL_UNIV;

    command_t argv = {"cs", "NonExistent"};
    GB::commands::cs(argv, g);

    // Should still be at universe level
    assert(g.level == ScopeLevel::LEVEL_UNIV);
    std::println("    ✓ Rejected invalid star name");
  }

  std::println("Test 5: cs command switches to planet scope");
  {
    g.level = ScopeLevel::LEVEL_STAR;
    g.snum = 0;

    command_t argv = {"cs", "AlphaPrime"};
    GB::commands::cs(argv, g);

    assert(g.level == ScopeLevel::LEVEL_PLAN);
    assert(g.snum == 0);
    assert(g.pnum == 0);
    std::println("    ✓ Switched to planet AlphaPrime scope");
  }

  std::println("\n✅ All cs tests passed!");
  return 0;
}
