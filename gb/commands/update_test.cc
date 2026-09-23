// SPDX-License-Identifier: Apache-2.0

/// \file update_test.cc
/// \brief Unit tests for @@update command.

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_update_matrix() {
  TestContext ctx;
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  Race deity_race{};
  deity_race.Playernum = 1;
  deity_race.name = "DeityRace";
  deity_race.God = true;
  deity_race.leader().active = true;

  Race mortal_race{};
  mortal_race.Playernum = 2;
  mortal_race.name = "MortalRace";
  mortal_race.God = false;
  mortal_race.leader().active = true;

  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(deity_race);
    races.save(mortal_race);

    ServerStateRepository state_repo(store);
    ServerState state{};
    state.segments = 1;
    state.update_time_minutes = 60;
    state_repo.save(state);
  }

  // --- Case 1: Happy Path (God user runs @@update) ---
  ctx.setup_game_obj(g, 1, 0);
  g.set_god(true);
  g.out.str("");

  test::expect_true(GB::commands::dispatch_command(g, GB::commands::update_cmd,
                                                   {"@@update"}));
  std::string out = g.out.str();
  test::expect_contains(out, "Starting update...");
  test::expect_contains(out, "Update completed.");

  // --- Case 2: Role Rejection (Mortal player cannot run @@update) ---
  ctx.setup_game_obj(g, 2, 0);
  g.set_god(false);
  g.out.str("");

  test::expect_false(GB::commands::dispatch_command(g, GB::commands::update_cmd,
                                                    {"@@update"}));
  test::expect_contains(g.out.str(), "Only deity can use this command.");

  // --- Case 3: Scope Testing (Valid in all scopes) ---
  ctx.setup_game_obj(g, 1, 0);
  g.set_god(true);
  for (auto scope : {ScopeLevel::LEVEL_UNIV, ScopeLevel::LEVEL_STAR,
                     ScopeLevel::LEVEL_PLAN, ScopeLevel::LEVEL_SHIP}) {
    g.set_level(scope);
    g.out.str("");
    test::expect_true(GB::commands::dispatch_command(
        g, GB::commands::update_cmd, {"@@update"}));
  }
}

void test_update_population_growth_persistence() {
  TestContext ctx;
  ctx.with_standard_universe().with_populated_planet(1, 1, 1, 100,
                                                     Coordinates{0, 0});

  // Ensure Player 1 is a deity with viable reproduction traits
  ctx.em.mutate_race(player_t{1}, [](Race& r) {
    r.God = true;
    r.likes[SectorType::SEC_LAND] = 1.0;
    r.likes[SectorType::SEC_PLATED] = 1.0;
    r.birthrate = 0.5;
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_god(true);

  // Initial population before @@update
  const auto* initial_planet = ctx.em.peek_planet(1, 1);
  test::expect_true(initial_planet != nullptr);
  auto initial_popn = initial_planet->popn();
  test::expect_eq(initial_popn, 100);

  // Clear cache before @@update to ensure clean SectorMap loaded from SQLite
  ctx.em.clear_cache();

  // Dispatch @@update through dispatch_command
  g.out.str("");
  test::expect_true(GB::commands::dispatch_command(g, GB::commands::update_cmd,
                                                   {"@@update"}));
  std::string out = g.out.str();
  test::expect_contains(out, "Starting update...");
  test::expect_contains(out, "Update completed.");

  // Clear cache to verify persistence in SQLite
  ctx.em.clear_cache();
  const auto* updated_planet = ctx.em.peek_planet(1, 1);
  test::expect_true(updated_planet != nullptr);
  test::expect_gt(updated_planet->popn(), initial_popn);

  const auto* updated_smap = ctx.em.peek_sectormap(1, 1);
  test::expect_true(updated_smap != nullptr);
  test::expect_gt(updated_smap->get(Coordinates{0, 0}).get_popn(), 100);
}

}  // namespace

int main() {
  test_update_matrix();
  test_update_population_growth_persistence();
  std::println(std::cout, "✓ update_test passed!");
  return 0;
}
