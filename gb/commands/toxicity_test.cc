// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import test;
import commands;
import std.compat;

#include <cassert>

int main() {
  TestContext ctx;
  JsonStore store(ctx.db);

  // Create test race via repository
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  RaceRepository races(store);
  races.save(race);

  // Create test star with AP via repository
  StarRepository stars(store);
  star_struct star_data{};
  star_data.star_id = 1;
  star_data.name = "TestStar";
  star_data.pnames.push_back("TestPlanet");
  star_data.AP[0] = 100;
  Star star(star_data);
  stars.save(star);

  // Create test planet via repository
  PlanetRepository planets(store);
  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.info(0).tox_thresh = 50;  // Default threshold
  planets.save(planet);

  // Test: Set toxicity threshold to 0
  {
    auto planet_handle = ctx.em.get_planet(1, 0);
    auto& p = *planet_handle;
    p.info(0).tox_thresh = 0;
  }

  // Verify: Threshold saved
  {
    const auto* saved = ctx.em.peek_planet(1, 0);
    assert(saved);
    assert(saved->info(0).tox_thresh == 0);
    std::println("✓ Toxicity threshold 0 saved correctly");
  }

  // Test: Set threshold to 100
  {
    auto planet_handle = ctx.em.get_planet(1, 0);
    auto& p = *planet_handle;
    p.info(0).tox_thresh = 100;
  }

  // Verify: Threshold updated
  {
    const auto* saved = ctx.em.peek_planet(1, 0);
    assert(saved);
    assert(saved->info(0).tox_thresh == 100);
    std::println("✓ Toxicity threshold 100 saved correctly");
  }

  // Test: Set threshold to mid-range value
  {
    auto planet_handle = ctx.em.get_planet(1, 0);
    auto& p = *planet_handle;
    p.info(0).tox_thresh = 75;
  }

  // Verify: Mid-range threshold saved
  {
    const auto* saved = ctx.em.peek_planet(1, 0);
    assert(saved);
    assert(saved->info(0).tox_thresh == 75);
    std::println("✓ Toxicity threshold 75 saved correctly");
  }

  // Test: Multiple planets with different thresholds
  Planet planet2{};
  planet2.star_id() = 1;
  planet2.planet_order() = 1;
  planet2.info(0).tox_thresh = 25;
  planets.save(planet2);

  // Verify: Both planets have correct thresholds
  {
    const auto* p1 = ctx.em.peek_planet(1, 0);
    const auto* p2 = ctx.em.peek_planet(1, 1);
    assert(p1);
    assert(p2);
    assert(p1->info(0).tox_thresh == 75);
    assert(p2->info(0).tox_thresh == 25);
    std::println(
        "✓ Multiple planets with different thresholds saved correctly");
  }

  std::println("All toxicity tests passed!");
  return 0;
}
