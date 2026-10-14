// SPDX-License-Identifier: Apache-2.0

/// \file arm_test.cc
/// \brief Unit tests for the arm command and sector militia mobilization.

import gb.entities;
import gb.services;
import gb.presentation;
import test;
import commands;
import std;

namespace {

void test_arm_command() {
  std::println(std::cout, "Test: arm command dispatch, MC/DC, and JSON output");
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_race(1, [](Race& r) { r.fighters = 100; });

  ctx.em.mutate_planet(1, 1, [](Planet& planet) {
    planet.info(player_t{1}).numsectsowned += 1;
    planet.info(player_t{1}).destruct = 1000;
    planet.popn() += 1000;
  });

  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    auto& sect = smap.get(Coordinates{5, 5});
    sect.set_owner(1);
    sect.set_popn_exact(1000);
    sect.set_troops(0);
    sect.set_mobilization(1);
    sect.set_condition(SectorType::SEC_MOUNT);
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // 1. Scope rejection at UNIV scope
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.set_snum(1);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"arm", "5,5", "100"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");

  // 2. Guest rejection
  ctx.em.mutate_race(1, [](Race& r) { r.Guest = true; });
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"arm", "5,5", "100"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");

  ctx.em.mutate_race(1, [](Race& r) { r.Guest = false; });
  ctx.setup_game_obj(g);

  // 3. Bad sector format, out-of-bounds coordinates, unowned sector
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"arm", "bad_coords", "10"});
  test::expect_contains(g.out.str(), "Bad format for sector.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"arm", "99,99", "10"});
  test::expect_contains(g.out.str(), "Illegal coordinates.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"arm", "1,1", "10"});
  test::expect_contains(g.out.str(), "You don't own that sector.");

  // 4. Enslaved planet rejection
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.slaved_to() = player_t{2}; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"arm", "5,5", "10"});
  test::expect_contains(g.out.str(), "That planet has been enslaved!");
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.slaved_to() = std::nullopt; });

  // 5. Invalid count argument (non-numeric and non-positive)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"arm", "5,5", "abc"});
  test::expect_contains(g.out.str(),
                        "You must specify a positive number of civs to arm.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"arm", "5,5", "0"});
  test::expect_contains(g.out.str(),
                        "You must specify a positive number of civs to arm.");

  // 6. Zero destruct / zero civs -> cannot arm
  ctx.em.mutate_planet(1, 1,
                       [](Planet& p) { p.info(player_t{1}).destruct = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"arm", "5,5", "10"});
  test::expect_contains(g.out.str(), "You can't arm any civilians now.");
  ctx.em.mutate_planet(1, 1,
                       [](Planet& p) { p.info(player_t{1}).destruct = 1000; });

  // 7. Insufficient money
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"arm", "5,5", "500"});
  test::expect_contains(g.out.str(),
                        "You need 50000 money to enlist 500 troops.");

  // 8. Arm command success in ASCII mode
  g.out.str("");
  ctx.assert_dispatch_success(g, {"arm", "5,5", "100"});
  test::expect_contains(g.out.str(), "100 population armed at a cost of 50");
  test::expect_contains(g.out.str(), "This mobilization cost 10000 money.");

  ctx.em.clear_cache();
  const auto* saved_smap = ctx.em.peek_sectormap(1, 1);
  const auto& saved_sect = saved_smap->get(Coordinates{5, 5});
  test::expect_eq(saved_sect.get_troops(), 100);
  test::expect_eq(saved_sect.get_popn(), 900);

  // 9. Default max amount (2-arg invocation) in JSON mode
  ctx.em.mutate_race(1, [](Race& r) { r.leader().money = 500000; });
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);
  g.set_ui_mode(GB::presentation::UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"arm", "5,5"});
  test::expect_contains(g.out.str(), R"("type":"arm")");
  test::expect_contains(g.out.str(), R"("armed":900)");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_arm_command();
  std::println(std::cout, "\n✅ All arm tests passed!");
  return 0;
}
