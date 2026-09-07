// SPDX-License-Identifier: Apache-2.0

/// \file survey_test.cc
/// \brief Unit tests for survey and client_survey commands

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();
}

void test_survey_no_args_planet_scope() {
  std::println(std::cout, "Test: survey (no args) at planet scope");

  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  ctx.assert_dispatch_success(g, {"survey"});

  std::string out_str = g.out.str();
  test::expect_contains(out_str, "======== Planetary conditions: ========");
  test::expect_contains(out_str, "atmosphere concentrations");
  std::println(std::cout, "    ✓ Output contains planet survey information");
}

void test_survey_sector_range_with_header() {
  std::println(std::cout,
               "Test: survey command with sector range shows header");

  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  ctx.assert_dispatch_success(g, {"survey", "0:2,0:2"});

  std::string out_str = g.out.str();
  test::expect_contains(out_str, "x,y");
  test::expect_contains(out_str, "cond/type");
  test::expect_contains(out_str, "owner");
  test::expect_contains(out_str, "xtals");
  test::expect_contains(out_str, "0,0");
  std::println(std::cout, "    ✓ Output contains header and sector data");
}

void test_survey_star_scope() {
  std::println(std::cout, "Test: survey command at star scope");

  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  ctx.assert_dispatch_success(g, {"survey"});

  std::string out_str = g.out.str();
  test::expect_contains(out_str, "Star Vega");
  test::expect_contains(out_str, "300,400");
  test::expect_contains(out_str, "Gravity");
  test::expect_contains(out_str, "Instability");
  test::expect_contains(out_str, "45%");
  test::expect_contains(out_str, "planets are");
  std::println(std::cout, "    ✓ Output contains star information");
}

void test_survey_universe_scope() {
  std::println(std::cout, "Test: survey command at universe scope");

  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  ctx.assert_dispatch_success(g, {"survey"});
  test::expect_contains(g.out.str(), "It's just _there_, you know?");
  std::println(std::cout, "    ✓ Universe scope survey succeeded");
}

void test_client_survey_dispatch() {
  std::println(std::cout, "Test: client_survey command dispatch");

  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  ctx.assert_dispatch_success(g, {"client_survey", "0:2,0:2"});
  test::expect_false(g.out.str().empty());
  std::println(std::cout, "    ✓ client_survey dispatched successfully");
}

}  // namespace

int main() {
  test_survey_no_args_planet_scope();
  test_survey_sector_range_with_header();
  test_survey_star_scope();
  test_survey_universe_scope();
  test_client_survey_dispatch();

  std::println(std::cout, "\n✅ All survey tests passed!");
  return 0;
}
