// SPDX-License-Identifier: Apache-2.0

/// \file dump_test.cc
/// \brief Unit tests for dump command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  Race race1{};
  race1.Playernum = 1;
  race1.name = "Explorer";
  race1.Guest = false;
  race1.governor[0].active = true;

  Race race2{};
  race2.Playernum = 2;
  race2.name = "Recipient";
  race2.Guest = true;
  race2.governor[0].active = true;

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  UniverseRepository universe_repo(store);
  universe_struct sdata{};
  sdata.id = 1;
  sdata.numstars = 1;
  universe_repo.save(sdata);

  star_struct ss{};
  ss.star_id = 0;
  ss.name = "TestStar";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.explored = (1ULL << 1);
  ss.AP[player_t{1}] = 20;
  ss.pnames.emplace_back("TestPlanet");
  Star star(ss);

  StarRepository stars_repo(store);
  stars_repo.save(star);

  planet_struct ps{};
  ps.star_id = 0;
  ps.planet_order = 0;
  ps.type = PlanetType::EARTH;
  ps.info[player_t{1}].explored = true;
  ps.info[player_t{2}].explored = false;
  Planet planet(ps);

  PlanetRepository planets_repo(store);
  planets_repo.save(planet);
}

void test_dump_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. Dump exploration data (10 AP deducted via FixedStar)
  ctx.assert_dispatch_success(g, {"dump", "Recipient"}, 10);
  test::expect_contains(g.out.str(), "Exploration Data transferred");

  const auto* p_after = ctx.em.peek_planet(0, 0);
  test::expect_ne(p_after, nullptr);
  test::expect_true(p_after->info(player_t{2}).explored);
}

void test_dump_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set Star AP to 5 (< 10)
  ctx.em.mutate_star(0, [](Star& s) { s.AP(1) = 5; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  ctx.assert_dispatch_rejected(g, {"dump", "Recipient"});
  test::expect_contains(g.out.str(), "You don't have 10 action points there.");
}

void test_dump_role_rejections() {
  TestContext ctx;
  setup_test_world(ctx);

  // 1. Guest race rejection
  {
    auto& registry = get_test_session_registry();
    GameObj g(ctx.em, registry);
    ctx.setup_game_obj(g, 2, 0);
    g.set_level(ScopeLevel::LEVEL_STAR);
    g.set_snum(0);

    ctx.assert_dispatch_rejected(g, {"dump", "Explorer"});
    test::expect_contains(g.out.str(), "Guest races cannot use this command.");
  }

  // 2. Leader-only rejection (Governor > 0)
  {
    auto& registry = get_test_session_registry();
    GameObj g(ctx.em, registry);
    ctx.setup_game_obj(g, 1, 1);
    g.set_level(ScopeLevel::LEVEL_STAR);
    g.set_snum(0);

    ctx.assert_dispatch_rejected(g, {"dump", "Recipient"});
    test::expect_contains(g.out.str(),
                          "Only the leader (Governor 0) may use this command.");
  }
}

void test_dump_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"dump"});
  test::expect_contains(g.out.str(), "Syntax: dump <player> [<place> ...]");

  // 2. Invalid player name
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dump", "NonExistentPlayer"});
  test::expect_contains(g.out.str(), "No such player");
}

}  // namespace

int main() {
  test_dump_happy_paths();
  test_dump_insufficient_ap();
  test_dump_role_rejections();
  test_dump_domain_errors();

  std::println(std::cout, "✓ dump_test passed!");
  return 0;
}
