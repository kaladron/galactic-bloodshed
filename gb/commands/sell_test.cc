// SPDX-License-Identifier: Apache-2.0

/// \file sell_test.cc
/// \brief Unit tests for sell command

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  Race race1{};
  race1.Playernum = 1;
  race1.name = "Trader";
  race1.Guest = false;
  race1.governor[0].active = true;
  race1.governor[1].active = true;

  Race race2{};
  race2.Playernum = 2;
  race2.name = "GuestTrader";
  race2.Guest = true;
  race2.governor[0].active = true;

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  star_struct ss{};
  ss.star_id = 1;
  ss.name = "TradeHub";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.explored = (1ULL << 1);
  ss.AP[0] = 100;
  ss.governor[0] = 1;  // Star controlled by Governor 1 for Player 1
  ss.pnames.push_back("TradePlanet");
  Star star(ss);

  StarRepository stars_repo(store);
  stars_repo.save(star);

  planet_struct ps{};
  ps.star_id = 1;
  ps.planet_order = 0;
  ps.type = PlanetType::EARTH;
  ps.Maxx = 10;
  ps.Maxy = 10;
  ps.info[0].explored = true;
  ps.info[0].numsectsowned = 5;
  ps.info[0].resource = 1000;
  ps.info[0].fuel = 500;
  ps.info[0].destruct = 200;
  ps.info[0].crystals = 50;
  Planet planet(ps);

  PlanetRepository planets_repo(store);
  planets_repo.save(planet);

  Ship port{};
  port.number() = 1;
  port.owner() = 1;
  port.governor() = 0;
  port.alive() = true;
  port.active() = true;
  port.type() = ShipType::OTYPE_GOV;
  port.damage() = 0.0;
  port.whatorbits() = ScopeLevel::LEVEL_PLAN;
  port.storbits() = 1;
  port.pnumorbits() = 0;

  ShipRepository ships_repo(store);
  ships_repo.save(port);

  // Link ship to planet
  auto planet_handle = ctx.em.get_planet(1, 0);
  planet_handle->ships() = 1;
}

void test_sell_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);

  // 1. Sell resources (min(20, 100) = 20 AP deducted)
  ctx.assert_dispatch_success(g, {"sell", "r", "100"}, 20);
  const auto* p1 = ctx.em.peek_planet(1, 0);
  assert(p1->info(player_t{1}).resource == 900);

  // 2. Sell fuel (min(20, 50) = 20 AP)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"sell", "f", "50"}, 20);
  const auto* p2 = ctx.em.peek_planet(1, 0);
  assert(p2->info(player_t{1}).fuel == 450);

  // 3. Sell destruct (min(20, 25) = 20 AP)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"sell", "d", "25"}, 20);
  const auto* p3 = ctx.em.peek_planet(1, 0);
  assert(p3->info(player_t{1}).destruct == 175);

  // 4. Sell crystals (min(20, 10) = 10 AP)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"sell", "x", "10"}, 10);
  const auto* p4 = ctx.em.peek_planet(1, 0);
  assert(p4->info(player_t{1}).crystals == 40);
}

void test_sell_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set Star AP to 5 (< 20 required for 100 units)
  {
    auto star_handle = ctx.em.get_star(1);
    star_handle->AP(1) = 5;
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);

  ctx.assert_dispatch_rejected(g, {"sell", "r", "100"});
  assert(g.out.str().contains("action points"));
}

void test_sell_role_rejections() {
  TestContext ctx;
  setup_test_world(ctx);

  // 1. Guest race rejection
  {
    auto& registry = get_test_session_registry();
    GameObj g(ctx.em, registry);
    ctx.setup_game_obj(g, 2, 0);  // Player 2 is guest
    g.set_level(ScopeLevel::LEVEL_PLAN);
    g.set_snum(1);
    g.set_pnum(0);

    ctx.assert_dispatch_rejected(g, {"sell", "r", "100"});
    assert(g.out.str().contains("Guest races cannot use this command."));
  }

  // 2. Star control rejection (Governor 2 when star is assigned to Governor 1)
  {
    auto& registry = get_test_session_registry();
    GameObj g(ctx.em, registry);
    ctx.setup_game_obj(g, 1, 2);  // Governor 2
    g.set_level(ScopeLevel::LEVEL_PLAN);
    g.set_snum(1);
    g.set_pnum(0);

    ctx.assert_dispatch_rejected(g, {"sell", "r", "100"});
    assert(g.out.str().contains(
        "You are not authorized to do that in this system."));
  }
}

void test_sell_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);

  // 1. Min args check (< 3 args)
  ctx.assert_dispatch_rejected(g, {"sell", "r"});
  assert(g.out.str().contains("Syntax: sell <r|d|f|x> <amount>"));

  // 2. Invalid commodity
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"sell", "z", "100"});
  assert(g.out.str().contains("Permitted commodities are r, d, f, and x"));

  // 3. Invalid scope (universe level)
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"sell", "r", "100"});
  assert(g.out.str().contains("Invalid scope"));
}

}  // namespace

int main() {
  test_sell_happy_paths();
  test_sell_insufficient_ap();
  test_sell_role_rejections();
  test_sell_domain_errors();

  std::println(std::cout, "✓ sell_test passed!");
  return 0;
}
