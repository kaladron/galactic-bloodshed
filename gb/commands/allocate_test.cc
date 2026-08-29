// SPDX-License-Identifier: Apache-2.0

/// \file allocate_test.cc
/// \brief Unit tests for the allocate action points command.

import dallib;
import gb.entities;
import gb.services;
import test;
import commands;
import std;

int main() {
  TestContext ctx;
  JsonStore store(ctx.db);

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "Spenders";
  race.Guest = false;
  race.governor[0].active = true;

  RaceRepository races(store);
  races.save(race);

  // Create test star 0
  star_struct star{};
  star.star_id = 0;
  star.name = "Sol";
  star.AP[player_t{1}] = 20;

  StarRepository stars(store);
  stars.save(star);

  // Setup Universe APs
  ctx.em.mutate_universe([](universe_struct& u) { u.AP[player_t{1}] = 50; });

  // Create GameObj
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{0});

  // 1. Scope rejection at universe level
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"allocate", "10"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");
  std::println(std::cout, "    ✓ Scope rejection at universe level verified");

  // Switch to star scope
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 2. Syntax / argument rejection
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"allocate"});
  test::expect_contains(g.out.str(), "Syntax: allocate <action points>");
  std::println(std::cout, "    ✓ Missing argument syntax error verified");

  // 3. Non-positive allocation rejection
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"allocate", "0"});
  test::expect_contains(
      g.out.str(), "You must specify a positive amount of APs to allocate.");
  std::println(std::cout, "    ✓ Non-positive allocation rejected");

  // 4. Over-allocation rejection (more than universe has)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"allocate", "100"});
  test::expect_contains(g.out.str(), "Illegal value (100) - maximum = 50");
  std::println(std::cout, "    ✓ Over-allocation rejected");

  // 5. Successful allocation
  g.out.str("");
  ctx.assert_dispatch_success(g, {"allocate", "15"});
  test::expect_contains(g.out.str(), "Allocated");
  {
    ctx.em.clear_cache();
    const auto* u = ctx.em.peek_universe();
    const auto* s = ctx.em.peek_star(0);
    test::expect_eq(u->AP[player_t{1}], 35);
    test::expect_eq(s->AP(1), 35);
  }
  std::println(std::cout, "    ✓ Successful allocation verified");

  // 6. Guest race rejection
  ctx.em.mutate_race(1, [](Race& r) { r.Guest = true; });
  ctx.setup_game_obj(g, player_t{1}, governor_t{0});
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"allocate", "5"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");
  std::println(std::cout, "    ✓ Guest race rejection verified");

  std::println(std::cout, "allocate_test passed!");
  return 0;
}
