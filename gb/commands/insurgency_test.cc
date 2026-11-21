// SPDX-License-Identifier: Apache-2.0

/// \file insurgency_test.cc
/// \brief Unit tests for insurgency command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

constexpr player_t instigator = 2;
constexpr player_t target = 3;

void setup_test_world(TestContext& ctx) {
  ctx.with_galaxy();
  TestWorldBuilder world(ctx);
  world.add_race("Federation", 100.0, false, instigator);
  world.add_race("Klingons", 100.0, false, target);

  ctx.em.mutate_star(1, [](Star& s) {
    s.mark_explored_by(instigator);
    s.mark_inhabited_by(instigator);
    s.AP(instigator) = 50;
  });

  ctx.em.mutate_race(instigator, [](Race& r) {
    r.leader().money = 1000000;
    r.morale = 100;
    r.fighters = 10;
  });

  ctx.em.mutate_race(target, [](Race& r) {
    r.morale = 50;
    r.fighters = 0;
  });

  ctx.em.mutate_planet(1, 1, [](Planet& p) {
    p.mark_explored_by(instigator);
    p.info(instigator).popn = 1000;
    p.info(instigator).numsectsowned = 1;
    p.info(target).popn = 100;
    p.info(target).troops = 0;
    p.info(target).numsectsowned = 5;
    p.info(target).tax = 100;
  });

  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    auto& inst_sect = smap.get(Coordinates{0, 1});
    inst_sect.set_owner(instigator);
    inst_sect.set_popn_exact(1000);
    inst_sect.set_troops(0);

    for (int i = 0; i < 5; i++) {
      auto& s = smap.get(Coordinates{i, 0});
      s.set_owner(target);
      s.set_popn_exact(200);
      s.set_troops(0);
      s.set_condition(SectorType::SEC_MOUNT);
    }
  });
}

void test_insurgency_happy_path_success() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, instigator, 1);
  g.set_scope_planet(1, 1);

  ctx.assert_dispatch_success(g, {"insurgency", "3", "500000"}, 10);
  test::expect_contains(g.out.str(), "Success!  You liberate");

  // Verify race money decreased
  ctx.em.clear_cache();
  const auto* saved_race = ctx.em.peek_race(instigator);
  test::expect_ne(saved_race, nullptr);
  test::expect_eq(saved_race->leader().money, 500000);

  // Verify planet tax rate inherited
  const auto* saved_planet = ctx.em.peek_planet(1, 1);
  test::expect_ne(saved_planet, nullptr);
  test::expect_eq(saved_planet->info(instigator).tax, 100);
  std::println(std::cout, "    ✓ Insurgency success path verified");
}

void test_insurgency_failed_revolt() {
  TestContext ctx;
  setup_test_world(ctx);

  // Invert morale and troop balance so success chance drops to 0
  ctx.em.mutate_race(instigator, [](Race& r) {
    r.morale = 0;
    r.fighters = 0;
  });
  ctx.em.mutate_race(target, [](Race& r) {
    r.morale = 100;
    r.fighters = 20;
  });
  ctx.em.mutate_planet(1, 1, [](Planet& p) {
    p.info(instigator).popn = 1;
    p.info(instigator).troops = 0;
    p.info(target).troops = 1000;
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, instigator, 1);
  g.set_scope_planet(1, 1);

  ctx.assert_dispatch_success(g, {"insurgency", "3", "100"}, 10);
  test::expect_contains(g.out.str(), "The insurgency failed!");
  std::println(std::cout, "    ✓ Insurgency failure path verified");
}

void test_insurgency_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set AP to 5 (< 10 required)
  ctx.em.mutate_star(1, [](Star& s) { s.AP(instigator) = 5; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, instigator, 1);
  g.set_scope_planet(1, 1);

  ctx.assert_dispatch_rejected(g, {"insurgency", "3", "5000"});
  test::expect_contains(g.out.str(), "action points");

  // Money must not have been deducted
  const auto* race = ctx.em.peek_race(instigator);
  test::expect_ne(race, nullptr);
  test::expect_eq(race->leader().money, 1000000);
  std::println(std::cout, "    ✓ Insufficient AP rejection verified");
}

void test_insurgency_role_and_scope_rejections() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Scope rejection (LEVEL_UNIV)
  ctx.setup_game_obj(g, instigator, 1);
  g.set_scope_univ();
  ctx.assert_dispatch_rejected(g, {"insurgency", "3", "5000"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");

  // 2. Star control rejection (Star governed by Gov 1, tested by Gov 2)
  ctx.em.mutate_race(instigator, [](Race& r) { r.appoint_governor(2); });
  ctx.em.mutate_star(1, [](Star& s) {
    s.set_governor(instigator, 1);  // Star governed by Gov 1
  });
  g.out.str("");
  ctx.setup_game_obj(g, instigator, 2);  // Instigator, Gov 2
  g.set_scope_planet(1, 1);
  ctx.assert_dispatch_rejected(g, {"insurgency", "3", "5000"});
  test::expect_contains(g.out.str(), "not authorized");
  std::println(std::cout, "    ✓ Role and scope rejections verified");
}

void test_insurgency_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, instigator, 1);
  g.set_scope_planet(1, 1);

  // 1. Min args check (< 3 args)
  ctx.assert_dispatch_rejected(g, {"insurgency", "3"});
  test::expect_contains(g.out.str(), "Syntax: insurgency <race> <money>");

  // 2. Non-existent player
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"insurgency", "99", "5000"});
  test::expect_contains(g.out.str(), "No such player.");

  // 3. Guest recipient rejection
  ctx.em.mutate_race(target, [](Race& r) { r.Guest = true; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"insurgency", "3", "5000"});
  test::expect_contains(g.out.str(), "Don't be such a dickweed.");
  ctx.em.mutate_race(target, [](Race& r) { r.Guest = false; });

  // 4. Revolt against self
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"insurgency", "2", "5000"});
  test::expect_contains(g.out.str(), "yourself");

  // 5. Instigator has no population in star system
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.info(instigator).popn = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"insurgency", "3", "5000"});
  test::expect_contains(g.out.str(),
                        "You must have population in the star system");
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.info(instigator).popn = 1000; });

  // 6. Target player does not occupy this planet
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.info(target).popn = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"insurgency", "3", "5000"});
  test::expect_contains(g.out.str(), "does not occupy this planet");
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.info(target).popn = 1000; });

  // 7. Negative, zero, and non-numeric money amount
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"insurgency", "3", "-100"});
  test::expect_contains(g.out.str(), "positive amount of money");
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"insurgency", "3", "0"});
  test::expect_contains(g.out.str(), "positive amount of money");
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"insurgency", "3", "abc"});
  test::expect_contains(g.out.str(), "positive amount of money");

  // 8. Not enough money in treasury
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"insurgency", "3", "2000000"});
  test::expect_contains(g.out.str(), "Nice try");

  // 9. UiMode::JSON verification
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"insurgency", "3", "100"}, 10);
  test::expect_contains(g.out.str(), R"("type":"message")");
  test::expect_contains(g.out.str(), R"(100 money against)");
  test::expect_contains(g.out.str(), R"("type":"planet")");
  test::expect_contains(g.out.str(), R"("type":"race")");
  test::expect_contains(g.out.str(), R"("type":"star")");
  g.set_ui_mode(UiMode::ASCII);
  std::println(std::cout, "    ✓ Domain error rejections verified");

  // 10. Command matrix validation (roles, guests, governor, scopes)
  TestCommandMatrix(ctx, "insurgency")
      .with_valid_argv({"insurgency", "3", "100"})
      .with_invalid_argv({"insurgency", "99", "100"})
      .with_expected_star_ap(10)
      .run_matrix(g);
}

}  // namespace

int main() {
  test_insurgency_happy_path_success();
  test_insurgency_failed_revolt();
  test_insurgency_insufficient_ap();
  test_insurgency_role_and_scope_rejections();
  test_insurgency_domain_errors();

  std::println(std::cout, "\n✅ All insurgency tests passed!");
  return 0;
}
