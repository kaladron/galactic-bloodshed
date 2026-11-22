// SPDX-License-Identifier: Apache-2.0

/// \file bless_test.cc
/// \brief Unit tests for bless command

import commands;
import dallib;
import gb.entities;
import gb.presentation.empire_views;
import gb.services;
import test;
import std;

namespace {

void test_bless_matrix() {
  TestContext ctx;
  const auto races = ctx.with_races();

  const shipnum_t ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_POD)
                                .owned_by(races.deity, 1)
                                .in_planet_orbit(1, 1)
                                .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, races.deity, 1);
  g.set_scope_planet(1, 1);

  TestCommandMatrix(ctx, "bless")
      .with_valid_argv({"bless", "4", "technology", "10"})
      .with_invalid_argv({"bless", "99", "technology", "10"})
      .with_invalid_scopes(
          {Place::star(1), Place::universe(), Place::ship(ship_id)})
      .with_expected_star_ap(0)
      .with_expected_univ_ap(0)
      .run_matrix(g);

  ctx.verify_universe_invariants();
}

void test_bless_role_and_scope_rejection() {
  TestContext ctx;
  const auto races = ctx.with_races();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Role Rejection: Mortal player is rejected
  ctx.setup_game_obj(g, races.empire, 1);
  g.set_scope_planet(1, 1);
  ctx.assert_dispatch_rejected(g, {"bless", "4", "technology", "5"});
  test::expect_contains(g.out.str(), "Only deity can use this command.");

  // 2. Scope Rejection: Deity at LEVEL_UNIV scope is rejected
  ctx.setup_game_obj(g, races.deity, 1);
  g.set_scope_univ();
  ctx.assert_dispatch_rejected(g, {"bless", "4", "technology", "5"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");

  ctx.verify_universe_invariants();
}

void test_bless_race_characteristics() {
  TestContext ctx;
  const auto races = ctx.with_races();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, races.deity, 1);
  g.set_scope_planet(1, 1);

  // Integer attributes
  ctx.assert_dispatch_success(g, {"bless", "4", "money", "500"});
  test::expect_eq(ctx.em.peek_race(races.empire)->leader().money, 10500);
  test::expect_true(ctx.em.has_telegrams(races.empire, Race::leader_id));

  ctx.em.mutate_race(races.empire, [](Race& r) {
    r.morale = 0;
    r.fertilize = 0;
    r.IQ = 0;
    r.fighters = 0;
    r.IQ_limit = 0;
  });
  ctx.assert_dispatch_success(g, {"bless", "4", "morale", "15"});
  test::expect_eq(ctx.em.peek_race(races.empire)->morale, 15);

  ctx.assert_dispatch_success(g, {"bless", "4", "fertility", "75"});
  test::expect_eq(ctx.em.peek_race(races.empire)->fertilize, 75);

  ctx.assert_dispatch_success(g, {"bless", "4", "IQ", "120"});
  test::expect_eq(ctx.em.peek_race(races.empire)->IQ, 120);

  ctx.assert_dispatch_success(g, {"bless", "4", "fight", "80"});
  test::expect_eq(ctx.em.peek_race(races.empire)->fighters, 80);

  ctx.assert_dispatch_success(g, {"bless", "4", "technology", "25"});
  test::expect_eq(ctx.em.peek_race(races.empire)->tech, 125.0);

  ctx.assert_dispatch_success(g, {"bless", "4", "maxiq", "200"});
  test::expect_eq(ctx.em.peek_race(races.empire)->IQ_limit, 200);

  // Float attributes
  ctx.assert_dispatch_success(g, {"bless", "4", "mass", "2.5"});
  test::expect_eq(ctx.em.peek_race(races.empire)->mass, 2.5);

  ctx.assert_dispatch_success(g, {"bless", "4", "metabolism", "1.75"});
  test::expect_eq(ctx.em.peek_race(races.empire)->metabolism, 1.75);

  ctx.assert_dispatch_success(g, {"bless", "4", "adventurism", "0.45"});
  test::expect_eq(ctx.em.peek_race(races.empire)->adventurism, 0.45);

  ctx.assert_dispatch_success(g, {"bless", "4", "birthrate", "1.25"});
  test::expect_eq(ctx.em.peek_race(races.empire)->birthrate, 1.25);

  // Flags & password
  ctx.assert_dispatch_rejected(g, {"bless", "4", "password", ""});
  ctx.assert_dispatch_rejected(g, {"bless", "4", "password", "bad pass"});
  ctx.assert_dispatch_success(g, {"bless", "4", "password", "secret42"});
  test::expect_eq(ctx.em.peek_race(races.empire)->password, "secret42");

  ctx.assert_dispatch_success(g, {"bless", "4", "pods", "0"});
  test::expect_true(ctx.em.peek_race(races.empire)->pods);
  ctx.assert_dispatch_success(g, {"bless", "4", "nopods", "0"});
  test::expect_false(ctx.em.peek_race(races.empire)->pods);

  ctx.assert_dispatch_success(g, {"bless", "4", "collectiveiq", "0"});
  test::expect_true(ctx.em.peek_race(races.empire)->collective_iq);
  ctx.assert_dispatch_success(g, {"bless", "4", "nocollectiveiq", "0"});
  test::expect_false(ctx.em.peek_race(races.empire)->collective_iq);

  ctx.assert_dispatch_success(g, {"bless", "4", "guest", "0"});
  test::expect_true(ctx.em.peek_race(races.empire)->Guest);
  test::expect_false(ctx.em.peek_race(races.empire)->God);

  ctx.assert_dispatch_success(g, {"bless", "4", "god", "0"});
  test::expect_true(ctx.em.peek_race(races.empire)->God);
  test::expect_false(ctx.em.peek_race(races.empire)->Guest);

  ctx.assert_dispatch_success(g, {"bless", "4", "guest", "0"});
  test::expect_false(ctx.em.peek_race(races.empire)->God);
  test::expect_true(ctx.em.peek_race(races.empire)->Guest);

  ctx.assert_dispatch_success(g, {"bless", "4", "mortal", "0"});
  test::expect_false(ctx.em.peek_race(races.empire)->God);
  test::expect_false(ctx.em.peek_race(races.empire)->Guest);

  // Self-blessing mortal drops g.god() immediately and reconciles unexplored
  // planet scope
  ctx.em.mutate_star(1, [&](Star& s) { s.mark_explored_by(races.deity); });
  ctx.em.mutate_planet(1, 1,
                       [&](Planet& p) { p.info(races.deity).explored = 0; });
  ctx.assert_dispatch_success(g, {"bless", "1", "mortal", "0"});
  test::expect_false(g.god());
  test::expect_eq(g.level(), ScopeLevel::LEVEL_STAR);
  ctx.em.mutate_planet(1, 1,
                       [&](Planet& p) { p.info(races.deity).explored = 1; });
  ctx.em.mutate_race(races.deity, [](Race& r) { r.God = true; });
  g.set_god(true);
  g.set_scope_planet(1, 1);

  // Sector preferences
  auto expect_near = [](double actual, double expected, double eps = 1e-6) {
    test::expect_true(std::abs(actual - expected) < eps);
  };

  ctx.assert_dispatch_success(g, {"bless", "4", "water", "60"});
  expect_near(ctx.em.peek_race(races.empire)->likes[SectorType::SEC_SEA], 0.60);
  ctx.assert_dispatch_success(g, {"bless", "4", "ocean", "65"});
  expect_near(ctx.em.peek_race(races.empire)->likes[SectorType::SEC_SEA], 0.65);

  ctx.assert_dispatch_success(g, {"bless", "4", "land", "70"});
  expect_near(ctx.em.peek_race(races.empire)->likes[SectorType::SEC_LAND],
              0.70);

  ctx.assert_dispatch_success(g, {"bless", "4", "mountain", "55"});
  expect_near(ctx.em.peek_race(races.empire)->likes[SectorType::SEC_MOUNT],
              0.55);
  ctx.assert_dispatch_success(g, {"bless", "4", "mountainous", "58"});
  expect_near(ctx.em.peek_race(races.empire)->likes[SectorType::SEC_MOUNT],
              0.58);

  ctx.assert_dispatch_success(g, {"bless", "4", "gas", "10"});
  expect_near(ctx.em.peek_race(races.empire)->likes[SectorType::SEC_GAS], 0.10);

  ctx.assert_dispatch_success(g, {"bless", "4", "ice", "20"});
  expect_near(ctx.em.peek_race(races.empire)->likes[SectorType::SEC_ICE], 0.20);

  ctx.assert_dispatch_success(g, {"bless", "4", "forest", "85"});
  expect_near(ctx.em.peek_race(races.empire)->likes[SectorType::SEC_FOREST],
              0.85);

  ctx.assert_dispatch_success(g, {"bless", "4", "desert", "30"});
  expect_near(ctx.em.peek_race(races.empire)->likes[SectorType::SEC_DESERT],
              0.30);

  ctx.assert_dispatch_success(g, {"bless", "4", "plated", "90"});
  expect_near(ctx.em.peek_race(races.empire)->likes[SectorType::SEC_PLATED],
              0.90);

  ctx.verify_universe_invariants();
}

void test_bless_planet_and_star() {
  TestContext ctx;
  const auto races = ctx.with_races();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, races.deity, 1);
  g.set_scope_planet(1, 1);

  // explorebit / noexplorebit on newcomer (who has not explored Sol/Earth)
  ctx.assert_dispatch_success(g, {"bless", "2", "explorebit", "0"});
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(races.newcomer).explored, 1);
  test::expect_true(ctx.em.peek_star(1)->is_explored_by(races.newcomer));

  ctx.assert_dispatch_success(g, {"bless", "2", "noexplorebit", "0"});
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(races.newcomer).explored, 0);

  // planetpopulation
  ctx.assert_dispatch_success(g, {"bless", "2", "planetpopulation", "2500"});
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(races.newcomer).popn, 2500);

  // inhabited bit - verify latent bug fix (marks target race 2, not deity race
  // 1)
  ctx.assert_dispatch_success(g, {"bless", "2", "inhabited", "0"});
  test::expect_true(ctx.em.peek_star(1)->is_inhabited_by(races.newcomer));

  // numsectsowned
  ctx.assert_dispatch_success(g, {"bless", "2", "numsectsowned", "42"});
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(races.newcomer).numsectsowned,
                  42);

  ctx.verify_universe_invariants();
}

void test_bless_commodities() {
  TestContext ctx;
  const auto races = ctx.with_races();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, races.deity, 1);
  g.set_scope_planet(1, 1);

  // resources: full word, plural, and char
  ctx.assert_dispatch_success(g, {"bless", "4", "resource", "100"});
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(races.empire).resource, 1100);
  test::expect_eq(g.out.str(), "");
  ctx.assert_dispatch_success(g, {"bless", "4", "resources", "25"});
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(races.empire).resource, 1125);
  ctx.assert_dispatch_success(g, {"bless", "4", "r", "50"});
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(races.empire).resource, 1175);

  // destruct
  ctx.assert_dispatch_success(g, {"bless", "4", "destruct", "200"});
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(races.empire).destruct, 1200);
  ctx.assert_dispatch_success(g, {"bless", "4", "d", "50"});
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(races.empire).destruct, 1250);

  // fuel
  ctx.assert_dispatch_success(g, {"bless", "4", "fuel", "300"});
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(races.empire).fuel, 1300);
  ctx.assert_dispatch_success(g, {"bless", "4", "f", "50"});
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(races.empire).fuel, 1350);

  // crystals: singular, plural, and char
  ctx.assert_dispatch_success(g, {"bless", "4", "crystal", "400"});
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(races.empire).crystals, 400);
  ctx.assert_dispatch_success(g, {"bless", "4", "crystals", "25"});
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(races.empire).crystals, 425);
  ctx.assert_dispatch_success(g, {"bless", "4", "x", "50"});
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(races.empire).crystals, 475);

  // action points: ap, char, and full word (by race name in JSON mode)
  ctx.assert_dispatch_success(g, {"bless", "4", "ap", "30"});
  test::expect_eq(ctx.em.peek_star(1)->AP(races.empire), 130);
  ctx.assert_dispatch_success(g, {"bless", "4", "a", "20"});
  test::expect_eq(ctx.em.peek_star(1)->AP(races.empire), 150);

  g.set_ui_mode(UiMode::JSON);
  const std::string empire_name = ctx.em.peek_race(races.empire)->name;
  ctx.assert_dispatch_success(g, {"bless", empire_name, "action", "15"});
  test::expect_eq(ctx.em.peek_star(1)->AP(races.empire), 165);
  test::expect_contains(g.out.str(), R"("type":"star")");
  test::expect_contains(g.out.str(), R"("name":"Sol")");

  ctx.verify_universe_invariants();
}

void test_bless_error_handling() {
  TestContext ctx;
  const auto races = ctx.with_races();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, races.deity, 1);
  g.set_scope_planet(1, 1);

  // Extra trailing arguments
  ctx.assert_dispatch_rejected(g, {"bless", "2", "technology", "10", "extra"});
  test::expect_contains(g.out.str(), "Syntax: bless <player> <what> <+amount>");

  // Invalid player number
  ctx.assert_dispatch_rejected(g, {"bless", "99", "technology", "10"});
  test::expect_contains(g.out.str(), "No such player number.");

  ctx.assert_dispatch_rejected(g, {"bless", "0", "technology", "10"});
  test::expect_contains(g.out.str(), "No such player number.");

  ctx.assert_dispatch_rejected(g, {"bless", "abc", "technology", "10"});
  test::expect_contains(g.out.str(), "No such player number.");

  ctx.assert_dispatch_rejected(g, {"bless", "1abc", "technology", "10"});
  test::expect_contains(g.out.str(), "No such player number.");

  // Unknown property or commodity
  ctx.assert_dispatch_rejected(g, {"bless", "2", "invalidprop", "10"});
  test::expect_contains(g.out.str(), "No such commodity.");

  // Invalid amounts
  ctx.assert_dispatch_rejected(g, {"bless", "2", "money", "bad"});
  test::expect_contains(g.out.str(), "Invalid amount.");

  ctx.assert_dispatch_rejected(g, {"bless", "2", "mass", "bad"});
  test::expect_contains(g.out.str(), "Invalid numeric value.");

  ctx.assert_dispatch_rejected(g, {"bless", "2", "water", "bad"});
  test::expect_contains(g.out.str(), "Invalid preference percentage.");

  ctx.assert_dispatch_rejected(g, {"bless", "2", "planetpopulation", "bad"});
  test::expect_contains(g.out.str(), "Invalid population count.");

  ctx.assert_dispatch_rejected(g, {"bless", "2", "numsectsowned", "bad"});
  test::expect_contains(g.out.str(), "Invalid sector count.");

  ctx.assert_dispatch_rejected(g, {"bless", "2", "fuel", "bad"});
  test::expect_contains(g.out.str(), "Invalid amount.");

  // Documented +amount prefix succeeds while trailing garbage and NaN reject
  ctx.assert_dispatch_success(g, {"bless", "2", "fuel", "+25"});
  ctx.assert_dispatch_rejected(g, {"bless", "2", "fuel", "25abc"});
  test::expect_contains(g.out.str(), "Invalid amount.");
  ctx.assert_dispatch_rejected(g, {"bless", "2", "mass", "2.5abc"});
  test::expect_contains(g.out.str(), "Invalid numeric value.");
  ctx.assert_dispatch_rejected(g, {"bless", "2", "mass", "nan"});
  test::expect_contains(g.out.str(), "Invalid numeric value.");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_bless_matrix();
  test_bless_role_and_scope_rejection();
  test_bless_race_characteristics();
  test_bless_planet_and_star();
  test_bless_commodities();
  test_bless_error_handling();

  std::println(std::cout, "✓ bless_test passed!");
  return 0;
}
