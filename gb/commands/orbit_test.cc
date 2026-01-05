// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import test;
import commands;
import std.compat;

#include <cassert>

int main() {
  // Create test context
  TestContext ctx;
  JsonStore store(ctx.db);

  // Create universe with 1 star and 2 ships at universe level
  universe_struct us{};
  us.id = 1;  // Universe is a singleton with ID 1
  us.numstars = 1;
  us.ships = 2;  // Ship #2 will be at universe level (in transit)

  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.governor[0].active = true;

  // Save race via repository
  RaceRepository races(store);
  races.save(race);

  // Create a test star using star_struct, then wrap in Star
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "TestStar";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.explored = (1ULL << 1);  // Player 1 has explored this star (bit 1)
  Star star(ss);

  // Save star via repository
  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create a test ship orbiting the star
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.alive() = true;
  ship.active() = true;
  ship.type() = ShipType::STYPE_FIGHTER;
  ship.name() = "TestFighter";
  ship.whatorbits() = ScopeLevel::LEVEL_STAR;
  ship.storbits() = 0;  // Orbiting star 0
  ship.xpos() = 100.0;
  ship.ypos() = 200.0;

  // Save ship via repository
  ShipRepository ships_repo(store);
  ships_repo.save(ship);

  // Create a second ship at universe level (in transit between stars)
  Ship ship2{};
  ship2.number() = 2;
  ship2.owner() = 1;
  ship2.governor() = 0;
  ship2.alive() = true;
  ship2.active() = true;
  ship2.type() = ShipType::STYPE_CRUISER;
  ship2.name() = "Voyager";
  ship2.whatorbits() = ScopeLevel::LEVEL_UNIV;
  ship2.xpos() = 150.0;
  ship2.ypos() = 250.0;
  ship2.nextship() = 0;  // End of universe ship list

  ships_repo.save(ship2);

  // Create GameObj for command execution
  auto* registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);  // Set race pointer like production
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);  // At star 0

  std::println("Test 1: Orbit command displays ship at star");
  {
    // orbit command should display ships in current scope
    // This is a read-only display command
    command_t argv = {"orbit"};
    GB::commands::orbit(argv, g);

    // Verify ship is still the same (not modified)
    const auto* saved_ship = ctx.em.peek_ship(1);
    assert(saved_ship != nullptr);
    assert(saved_ship->owner() == 1);
    assert(saved_ship->whatorbits() == ScopeLevel::LEVEL_STAR);
    assert(saved_ship->storbits() == 0);
    std::println("    ✓ Orbit display works correctly");
  }

  std::println("Test 2: Orbit at universe level");
  {
    // Change scope to universe level
    g.set_level(ScopeLevel::LEVEL_UNIV);

    command_t argv = {"orbit"};
    GB::commands::orbit(argv, g);

    // Verify ships are still unchanged
    const auto* saved_ship = ctx.em.peek_ship(1);
    assert(saved_ship != nullptr);
    assert(saved_ship->owner() == 1);

    const auto* saved_ship2 = ctx.em.peek_ship(2);
    assert(saved_ship2 != nullptr);
    assert(saved_ship2->owner() == 1);
    assert(saved_ship2->whatorbits() == ScopeLevel::LEVEL_UNIV);
    std::println(
        "    ✓ Universe-level orbit displays stars and ships in transit");
  }

  std::println("Test 3: Orbit at universe level with no universe (edge case)");
  {
    // Create a new EntityManager with no universe
    Database db2(":memory:");
    initialize_schema(db2);
    EntityManager em2(db2);

    // Create race but no universe
    JsonStore store2(db2);
    RaceRepository races2(store2);
    races2.save(race);

    auto* registry = get_test_session_registry();
    GameObj g2(em2, registry);
    g2.set_player(1);
    g2.set_governor(0);
    g2.race = em2.peek_race(1);
    g2.set_level(ScopeLevel::LEVEL_UNIV);

    command_t argv = {"orbit"};
    GB::commands::orbit(argv, g2);

    // Should have printed error and not crashed
    std::println("    ✓ Handled missing universe gracefully");
  }

  std::println("\n✅ All orbit tests passed!");
  return 0;
}
