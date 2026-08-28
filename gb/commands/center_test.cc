// SPDX-License-Identifier: Apache-2.0

/// \file center_test.cc
/// \brief Unit tests for center command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  Race race{};
  race.Playernum = 1;
  race.name = "Stargazers";
  race.Guest = false;
  race.governor[0].active = true;

  RaceRepository races(store);
  races.save(race);

  universe_struct us{};
  us.id = 1;
  us.numstars = 1;
  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  star_struct ss{};
  ss.star_id = 0;
  ss.name = "Alpha";
  ss.xpos = 150.0;
  ss.ypos = 250.0;
  ss.explored = (1ULL << 1);

  StarRepository stars(store);
  stars.save(ss);
}

void test_center_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  ctx.assert_dispatch_success(g, {"center", "/Alpha"});
  test::expect_eq(g.lastx[1], 150.0);
  test::expect_eq(g.lasty[1], 250.0);
}

void test_center_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"center"});
  test::expect_contains(g.out.str(), "Syntax: center <star>");

  // 2. Non-existent star
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"center", "/NonexistentStar"});
  test::expect_contains(g.out.str(), "center: bad scope.");
}

}  // namespace

int main() {
  test_center_happy_path();
  test_center_domain_errors();

  std::println(std::cout, "✓ center_test passed!");
  return 0;
}
