// SPDX-License-Identifier: Apache-2.0

/// \file dock_test.cc
/// \brief Unit tests for dock and assault commands

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

void setup_test_world(TestContext& ctx) {
  // Player 1 (Normal Race)
  Race race1{};
  race1.Playernum = 1;
  race1.name = "NormalRace";
  race1.governor[0].active = true;
  race1.mass = 1.0;
  race1.fighters = 1.0;
  race1.tech = 10.0;
  race1.morale = 100;
  race1.Guest = false;

  // Player 2 (Guest Race)
  Race race2{};
  race2.Playernum = 2;
  race2.name = "GuestRace";
  race2.governor[0].active = true;
  race2.mass = 1.0;
  race2.fighters = 1.0;
  race2.tech = 10.0;
  race2.morale = 100;
  race2.Guest = true;

  // Star 1
  star_struct ss{};
  ss.star_id = 1;
  ss.name = "TestStar";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.explored = (1ULL << 1) | (1ULL << 2);
  ss.AP[0] = 10;  // Player 1 has 10 AP
  ss.AP[1] = 10;  // Player 2 has 10 AP
  Star star(ss);

  // Ship 1: Player 1 Fighter
  Ship ship1{};
  ship1.number() = 1;
  ship1.owner() = 1;
  ship1.governor() = 0;
  ship1.alive() = true;
  ship1.active() = true;
  ship1.type() = ShipType::STYPE_FIGHTER;
  ship1.name() = "Docker";
  ship1.whatorbits() = ScopeLevel::LEVEL_STAR;
  ship1.storbits() = 1;
  ship1.xpos() = 100.0;
  ship1.ypos() = 200.0;
  ship1.fuel() = 100.0;
  ship1.mass() = 10.0;
  ship1.troops() = 10;
  ship1.docked() = 0;

  // Ship 2: Player 1 Carrier (close to ship 1)
  Ship ship2{};
  ship2.number() = 2;
  ship2.owner() = 1;
  ship2.governor() = 0;
  ship2.alive() = true;
  ship2.active() = true;
  ship2.type() = ShipType::STYPE_CARRIER;
  ship2.name() = "Carrier";
  ship2.whatorbits() = ScopeLevel::LEVEL_STAR;
  ship2.storbits() = 1;
  ship2.xpos() = 100.0;
  ship2.ypos() = 200.0;
  ship2.fuel() = 100.0;
  ship2.mass() = 100.0;
  ship2.max_crew() = 50;
  ship2.docked() = 0;

  // Ship 3: Player 2 Cargo Ship (target for assault)
  Ship ship3{};
  ship3.number() = 3;
  ship3.owner() = 2;
  ship3.governor() = 0;
  ship3.alive() = true;
  ship3.active() = true;
  ship3.type() = ShipType::STYPE_CARGO;
  ship3.name() = "Target";
  ship3.whatorbits() = ScopeLevel::LEVEL_STAR;
  ship3.storbits() = 1;
  ship3.xpos() = 100.0;
  ship3.ypos() = 200.0;
  ship3.fuel() = 100.0;
  ship3.mass() = 50.0;
  ship3.max_crew() = 50;
  ship3.popn() = 0;
  ship3.troops() = 0;
  ship3.docked() = 0;

  // Ship 4: Far away ship
  Ship ship4{};
  ship4.number() = 4;
  ship4.owner() = 1;
  ship4.governor() = 0;
  ship4.alive() = true;
  ship4.active() = true;
  ship4.type() = ShipType::STYPE_CARRIER;
  ship4.name() = "FarTarget";
  ship4.whatorbits() = ScopeLevel::LEVEL_STAR;
  ship4.storbits() = 1;
  ship4.xpos() = 500.0;
  ship4.ypos() = 500.0;
  ship4.fuel() = 100.0;
  ship4.mass() = 100.0;
  ship4.docked() = 0;

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race1);
  races.save(race2);
  StarRepository stars_repo(store);
  stars_repo.save(star);
  ShipRepository ships_repo(store);
  ships_repo.save(ship1);
  ships_repo.save(ship2);
  ships_repo.save(ship3);
  ships_repo.save(ship4);
}

void test_dock_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1. Successful dock (0 AP)
  ctx.assert_dispatch_success(g, {"dock", "#1", "#2"}, 0);
  assert(g.out.str().contains("docked with"));

  const auto* s1 = ctx.em.peek_ship(1);
  const auto* s2 = ctx.em.peek_ship(2);
  assert(s1->docked() == 1);
  assert(s1->whatdest() == ScopeLevel::LEVEL_SHIP);
  assert(s1->destshipno() == 2);
  assert(s2->docked() == 1);
  assert(s2->whatdest() == ScopeLevel::LEVEL_SHIP);
  assert(s2->destshipno() == 1);

  // 2. Successful assault (1 AP deducted via dynamic AP)
  g.out.str("");
  // Undock first for assault test
  {
    auto s1_handle = ctx.em.get_ship(1);
    s1_handle->docked() = 0;
    s1_handle->destshipno() = 0;
    s1_handle->whatdest() = ScopeLevel::LEVEL_UNIV;
  }
  ctx.assert_dispatch_success(g, {"assault", "#1", "#3"}, 1);
  assert(g.out.str().contains("VICTORY") || g.out.str().contains("CAPTURED"));
}

void test_assault_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set Star AP to 0 for Player 1
  {
    auto star_handle = ctx.em.get_star(1);
    star_handle->AP(1) = 0;
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  ctx.assert_dispatch_rejected(g, {"assault", "#1", "#3"});
  assert(g.out.str().contains("action points"));
}

void test_assault_guest_rejection() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 2, 0);  // Player 2 is guest
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  ctx.assert_dispatch_rejected(g, {"assault", "#3", "#1"});
  assert(g.out.str().contains("Guest races cannot use this command."));
}

void test_dock_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1. Min args violation (< 3 args)
  ctx.assert_dispatch_rejected(g, {"dock", "#1"});
  assert(g.out.str().contains("Syntax: dock <ship> <target_ship>"));

  // 2. Docking with self
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dock", "#1", "#1"});
  assert(g.out.str().contains("You can't dock with yourself!"));

  // 3. Out of range docking
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dock", "#1", "#4"});
  assert(g.out.str().contains("10.00 or closer"));
}

}  // namespace

int main() {
  test_dock_happy_paths();
  test_assault_insufficient_ap();
  test_assault_guest_rejection();
  test_dock_domain_errors();

  std::println(std::cout, "✓ dock_test passed!");
  return 0;
}
