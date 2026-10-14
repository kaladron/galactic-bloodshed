// SPDX-License-Identifier: Apache-2.0

/// \file disarm_test.cc
/// \brief Unit tests for the disarm command and sector militia demobilization.

import gb.entities;
import gb.services;
import gb.presentation;
import test;
import commands;
import std;

namespace {

void test_disarm_command() {
  std::println(std::cout,
               "Test: disarm command dispatch, MC/DC, and JSON output");
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_planet(1, 1, [](Planet& planet) {
    planet.info(player_t{1}).numsectsowned += 1;
    planet.popn() += 900;
    planet.troops() += 100;
    planet.info(player_t{1}).popn += 900;
    planet.info(player_t{1}).troops += 100;
  });

  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    auto& sect = smap.get(Coordinates{5, 5});
    sect.set_owner(1);
    sect.set_popn_exact(900);
    sect.set_troops(100);
    sect.set_condition(SectorType::SEC_MOUNT);
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // 1. Scope rejection at STAR scope
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"disarm", "5,5", "50"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");

  // 2. Bad sector format, illegal coordinates, unowned sector
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"disarm", "bad_coords", "10"});
  test::expect_contains(g.out.str(), "Bad format for sector.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"disarm", "99,99", "10"});
  test::expect_contains(g.out.str(), "Illegal coordinates.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"disarm", "1,1", "10"});
  test::expect_contains(g.out.str(), "You don't own that sector.");

  // 3. Invalid count argument (non-numeric and non-positive)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"disarm", "5,5", "abc"});
  test::expect_contains(g.out.str(),
                        "You must specify a positive number of civs to arm.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"disarm", "5,5", "-5"});
  test::expect_contains(g.out.str(),
                        "You must specify a positive number of civs to arm.");

  // 4. Disarm partial troops in ASCII mode
  g.out.str("");
  ctx.assert_dispatch_success(g, {"disarm", "5,5", "40"});
  test::expect_contains(g.out.str(),
                        "40 troops disarmed (now 940 civilians, 60 military)");

  ctx.em.clear_cache();
  const auto* saved_smap = ctx.em.peek_sectormap(1, 1);
  test::expect_eq(saved_smap->get(Coordinates{5, 5}).get_troops(), 60);
  test::expect_eq(saved_smap->get(Coordinates{5, 5}).get_popn(), 940);

  // 5. Disarm remaining troops (2-arg default) in JSON mode
  g.set_ui_mode(GB::presentation::UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"disarm", "5,5"});
  test::expect_contains(g.out.str(), R"("type":"disarm")");
  test::expect_contains(g.out.str(), R"("disarmed":60)");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_disarm_command();
  std::println(std::cout, "\n✅ All disarm tests passed!");
  return 0;
}
