// SPDX-License-Identifier: Apache-2.0

/// \file dissolve_test.cc
/// \brief Unit tests for dissolve command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();

  ctx.em.mutate_race(1, [](Race& race) {
    race.password = "testpass";
    race.leader().password = "govpass";
    race.appoint_governor(2, {.password = "subpass"});
    race.dissolved = false;
  });

  ctx.em.mutate_race(2, [](Race& race2) {
    race2.password = "otherpass";
    race2.leader().password = "othergov";
  });

  TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER, 1)
      .owned_by(1, 1)
      .in_planet_orbit(1, 1)
      .build();
  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE, 2)
      .owned_by(2, 1)
      .in_planet_orbit(1, 1)
      .build();

  // Load race into EntityManager cache to ensure getracenum can find it
  const auto* loaded_race = ctx.em.peek_race(1);
  test::expect_ne(loaded_race, nullptr);
  test::expect_eq(loaded_race->password, "testpass");
  test::expect_eq(loaded_race->leader().password, "govpass");
}

void test_dissolve_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  std::println(std::cout, "Dissolve race with correct passwords and waste");
  {
    ctx.assert_dispatch_success(g,
                                {"dissolve", "testpass", "govpass", "waste"});
    test::expect_contains(g.out.str(), "Ship #1, self-destruct enabled");

    // Clear cache to force reload from database
    ctx.em.clear_cache();

    // Verify race was dissolved
    const auto* saved_race = ctx.em.peek_race(1);
    test::expect_ne(saved_race, nullptr);
    test::expect_true(saved_race->dissolved);

    // Verify ship #1 was destroyed while player 2's ship #2 remains alive
    const auto* saved_ship1 = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship1, nullptr);
    test::expect_false(saved_ship1->alive());
    const auto* saved_ship2 = ctx.em.peek_ship(2);
    test::expect_ne(saved_ship2, nullptr);
    test::expect_true(saved_ship2->alive());

    // Verify Earth sector (0,0) was cleared and wasted, planet demographics
    // synced, and star inhabitation cleared for player 1
    const auto* smap = ctx.em.peek_sectormap(1, 1);
    test::expect_eq(smap->get({0, 0}).get_owner(), 0);
    test::expect_eq(smap->get({0, 0}).get_popn(), 0);
    test::expect_eq(smap->get({0, 0}).get_troops(), 0);
    test::expect_eq(smap->get({0, 0}).get_condition(), SectorType::SEC_WASTED);

    const auto* pl = ctx.em.peek_planet(1, 1);
    test::expect_eq(pl->popn(), 0);
    test::expect_eq(pl->troops(), 0);
    test::expect_eq(pl->info(player_t{1}).numsectsowned, 0);

    const auto* star0 = ctx.em.peek_star(1);
    test::expect_false(star0->is_inhabited_by(1));

    // Verify Vega Prime (Player 2 colony) remains intact
    const auto* vega_pl = ctx.em.peek_planet(2, 1);
    test::expect_eq(vega_pl->popn(), 1000);
    test::expect_eq(vega_pl->info(player_t{2}).numsectsowned, 1);

    ctx.verify_universe_invariants();
  }
}

void test_dissolve_role_rejections() {
  TestContext ctx;
  setup_test_world(ctx);

  // Create Guest Race
  Race guest_race{};
  guest_race.Playernum = 3;
  guest_race.name = "GuestRace";
  guest_race.password = "guestpass";
  guest_race.Guest = true;
  guest_race.leader().password = "guestgov";
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(guest_race);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Guest race rejection
  ctx.setup_game_obj(g, 3, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"dissolve", "guestpass", "guestgov"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");

  // 2. Leader-only rejection (Governor 2 via dispatcher)
  g.out.str("");
  ctx.setup_game_obj(g, 1, 2);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"dissolve", "testpass", "subpass"});
  test::expect_contains(g.out.str(), "leader (Governor 1)");

  // 3. Direct handler governor != 1 leader notification check
  g.out.str("");
  test::expect_false(
      GB::commands::dissolve({"dissolve", "testpass", "subpass"}, g));
  test::expect_contains(g.out.str(), "The leader has been notified");
}

void test_dissolve_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  // 1. Min args (< 3 args via dispatcher and direct handler)
  ctx.assert_dispatch_rejected(g, {"dissolve", "testpass"});
  test::expect_contains(
      g.out.str(),
      "Syntax: dissolve <race password> <leader password> [waste]");
  g.out.str("");
  test::expect_false(GB::commands::dissolve({"dissolve", "testpass"}, g));
  test::expect_contains(g.out.str(),
                        "Self-Destruct sequence requires passwords.");

  // 2. Password mismatch (non-existent credentials)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dissolve", "wrongpass", "wronggov"});
  test::expect_contains(g.out.str(), "Password mismatch");

  // 3. Cross-player password rejection (Player 1 supplying Player 2's valid
  // leader credentials must be rejected!)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dissolve", "otherpass", "othergov"});
  test::expect_contains(g.out.str(), "Password mismatch");
  test::expect_false(ctx.em.peek_race(1)->dissolved);

  // 4. Subordinate governor password rejection (Player 1 supplying their own
  // Governor 2 password instead of Governor 1 leader password)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dissolve", "testpass", "subpass"});
  test::expect_contains(g.out.str(), "Password mismatch");
  test::expect_false(ctx.em.peek_race(1)->dissolved);
}

}  // namespace

int main() {
  test_dissolve_happy_path();
  test_dissolve_role_rejections();
  test_dissolve_domain_errors();

  std::println(std::cout, "\n✅ All dissolve tests passed!");
  return 0;
}
