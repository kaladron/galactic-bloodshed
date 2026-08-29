// SPDX-License-Identifier: Apache-2.0

/// \file block_test.cc
/// \brief Unit tests for block command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_block_dispatch() {
  TestContext ctx;
  JsonStore store(ctx.db);

  // Create test races
  Race race1{};
  race1.Playernum = 1;
  race1.name = "TestRace1";
  race1.governor[0].active = true;
  race1.translate[player_t{1}] = 100;
  race1.translate[player_t{2}] = 50;
  race1.translate[player_t{3}] = 75;

  Race race2{};
  race2.Playernum = 2;
  race2.name = "TestRace2";
  race2.governor[0].active = true;

  Race race3{};
  race3.Playernum = 3;
  race3.name = "TestRace3";
  race3.governor[0].active = true;

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);
  races.save(race3);

  // Create blocks for each player
  block block1{};
  block1.Playernum = 1;
  block1.name = "ZeroVPBlock";
  block1.motto = "We have no VPs yet";
  block1.VPs = 0;
  block1.invite = (1ULL << 1);
  block1.pledge = (1ULL << 1);

  block block2{};
  block2.Playernum = 2;
  block2.name = "HasVPsBlock";
  block2.motto = "We have some VPs";
  block2.VPs = 100;
  block2.invite = (1ULL << 2);
  block2.pledge = (1ULL << 2);

  block block3{};
  block3.Playernum = 3;
  block3.name = "EmptyBlock";
  block3.motto = "Nobody here";
  block3.VPs = 50;
  block3.invite = 0;
  block3.pledge = 0;

  BlockRepository blocks(store);
  blocks.save(block1);
  blocks.save(block2);
  blocks.save(block3);

  // Setup Power_blocks global with member counts
  Power_blocks.time = std::time(nullptr);
  Power_blocks.members[0] = 1;
  Power_blocks.members[1] = 1;
  Power_blocks.members[2] = 0;

  Power_blocks.VPs[0] = 0;
  Power_blocks.VPs[1] = 100;
  Power_blocks.VPs[2] = 50;

  Power_blocks.money[0] = 1000;
  Power_blocks.money[1] = 5000;
  Power_blocks.popn[0] = 10000;
  Power_blocks.popn[1] = 50000;
  Power_blocks.ships_owned[0] = 5;
  Power_blocks.ships_owned[1] = 20;

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  // 1. List all alliance blocks
  ctx.assert_dispatch_success(g, {"block"});
  std::string output = g.out.str();
  test::expect_contains(output, "ZeroVPBlock");
  test::expect_contains(output, "HasVPsBlock");
  test::expect_false(output.contains("EmptyBlock"));
  std::println(std::cout, "    ✓ All alliance blocks listing succeeded");

  // 2. Query player block membership
  g.out.str("");
  ctx.assert_dispatch_success(g, {"block", "player", "1"});
  test::expect_contains(g.out.str(), "TestRace1");
  std::println(std::cout, "    ✓ Player block membership query succeeded");

  // 3. Query specific block power report
  g.out.str("");
  ctx.assert_dispatch_success(g, {"block", "1"});
  test::expect_contains(g.out.str(), "ZeroVPBlock");
  std::println(std::cout, "    ✓ Specific block report query succeeded");
}

}  // namespace

int main() {
  test_block_dispatch();
  std::println(std::cout, "\n✅ All block tests passed!");
  return 0;
}
