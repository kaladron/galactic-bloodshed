// SPDX-License-Identifier: Apache-2.0

/// \file dissolve_test.cc
/// \brief Unit tests for dissolve command

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  // Create test race via repository
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.password = "testpass";
  race.Guest = false;
  race.governor[0].active = true;
  race.governor[0].password = "govpass";
  race.governor[1].active = true;
  race.governor[1].password = "subpass";
  race.dissolved = false;

  // NOTE: Not creating ships for this test because kill_ship() still uses
  // global races[] We're just testing that the dissolved flag gets set
  // correctly

  // Save via repositories
  RaceRepository races(store);
  races.save(race);

  // Setup universe_struct (required by dissolve command)
  UniverseRepository universe_repo(store);
  universe_struct sdata{};
  sdata.id = 1;
  sdata.numstars = 0;  // No stars, simplifies test
  universe_repo.save(sdata);

  // Load race into EntityManager cache to ensure getracenum can find it
  const auto* loaded_race = ctx.em.peek_race(1);
  assert(loaded_race != nullptr);
  assert(loaded_race->password == "testpass");
  std::println(std::cout,
               "Race loaded into EntityManager: player={}, password={}",
               loaded_race->Playernum, loaded_race->password);
  std::println(std::cout, "Governor 0: active={}, password='{}'",
               loaded_race->governor[0].active,
               loaded_race->governor[0].password);
  assert(loaded_race->governor[0].password == "govpass");
}

void test_dissolve_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  std::println(std::cout, "Dissolve race with correct passwords");
  {
    ctx.assert_dispatch_success(g, {"dissolve", "testpass", "govpass"});
    std::println(std::cout, "Command output: {}", g.out.str());

    // Clear cache to force reload from database
    ctx.em.clear_cache();

    // Verify race was dissolved
    const auto* saved_race = ctx.em.peek_race(1);
    assert(saved_race != nullptr);
    std::println(std::cout, "DEBUG: Race dissolved = {}",
                 saved_race->dissolved);
    std::println(std::cout, "DEBUG: Race name = {}", saved_race->name);
    assert(saved_race->dissolved == true);
    std::println(std::cout, "    ✓ Race dissolved flag set to true");

    // TODO: Re-enable ship destruction test after kill_ship() migrated to
    // EntityManager (Phase 3.7) Currently disabled because kill_ship() uses
    // global races[] array Expected behavior: ship->alive should be false or
    // ship->owner should be 0
    //
    // Verify ship was destroyed (alive flag should be false)
    // const auto* saved_ship = ctx.em.peek_ship(1);
    // assert(saved_ship != nullptr);
    // assert(saved_ship->alive == false || saved_ship->owner == 0);
    // std::println(std::cout, "    ✓ Ship destroyed or ownership removed");
  }
}

void test_dissolve_role_rejections() {
  TestContext ctx;
  setup_test_world(ctx);

  // Create Guest Race
  Race guest_race{};
  guest_race.Playernum = 2;
  guest_race.name = "GuestRace";
  guest_race.password = "guestpass";
  guest_race.Guest = true;
  guest_race.governor[0].active = true;
  guest_race.governor[0].password = "guestgov";
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(guest_race);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Guest race rejection
  ctx.setup_game_obj(g, 2, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"dissolve", "guestpass", "guestgov"});
  assert(g.out.str().contains("Guest races cannot use this command."));

  // 2. Leader-only rejection (Governor 1)
  g.out.str("");
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"dissolve", "testpass", "subpass"});
  assert(g.out.str().contains("leader (Governor 0)"));
}

void test_dissolve_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  // 1. Min args (< 3 args)
  ctx.assert_dispatch_rejected(g, {"dissolve", "testpass"});
  assert(g.out.str().contains(
      "Syntax: dissolve <race password> <leader password> [waste]"));

  // 2. Password mismatch
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dissolve", "wrongpass", "wronggov"});
  assert(g.out.str().contains("Password mismatch"));
}

}  // namespace

int main() {
  test_dissolve_happy_path();
  test_dissolve_role_rejections();
  test_dissolve_domain_errors();

  std::println(std::cout, "\n✅ All dissolve tests passed!");
  return 0;
}
