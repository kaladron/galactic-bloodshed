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
  race1.translate[player_t{1}] = 100;
  race1.translate[player_t{2}] = 50;
  race1.translate[player_t{3}] = 75;

  Race race2{};
  race2.Playernum = 2;
  race2.name = "TestRace2";

  Race race3{};
  race3.Playernum = 3;
  race3.name = "TestRace3";
  race3.dissolved = true;

  Race race4{};
  race4.Playernum = 4;
  race4.name = "NoBlockRace";

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);
  races.save(race3);
  races.save(race4);

  PowerRepository(store).save(Power{{
      .id = 1,
      .troops = 100,
      .popn = 1000,
      .ships_owned = 5,
      .planets_owned = 2,
      .money = 500,
  }});

  // Create blocks for each player using designated initializers
  const Block block1{{
      .Playernum = 1,
      .name = "ZeroVPBlock",
      .motto = "We have no VPs yet",
      .invited = {player_t{1}, player_t{2}, player_t{3}},
      .pledged = {player_t{1}, player_t{2}, player_t{3}},
      .members = 1,
      .popn = 10000,
      .ships_owned = 5,
      .VPs = 0,
      .money = 1000,
  }};

  // Block 2: Has VPs and members, plus player 1 invited-only
  const Block block2{{
      .Playernum = 2,
      .name = "HasVPsBlock",
      .motto = "We are winning",
      .invited = {player_t{1}, player_t{2}},
      .pledged = {player_t{2}},
      .members = 1,
      .popn = 50000,
      .ships_owned = 20,
      .systems_owned = 5,
      .VPs = 100,
      .money = 5000,
  }};

  // Block 3: Has VPs but NO members, plus player 1 pledged-only
  const Block block3{{
      .Playernum = 3,
      .name = "EmptyBlock",
      .motto = "Nobody home",
      .pledged = {player_t{1}},
      .members = 0,
      .systems_owned = 1,
      .VPs = 50,
      .money = 500,
  }};

  BlockRepository blocks(store);
  blocks.save(block1);
  blocks.save(block2);
  blocks.save(block3);

  ctx.em.mutate_server_state(
      [](ServerState& state) { state.last_update_time = std::time(nullptr); });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  // 1. List all alliance blocks
  ctx.assert_dispatch_success(g, {"block"});
  std::string output = g.out.str();
  test::expect_contains(output, "ZeroVPBlock");
  test::expect_contains(output, "HasVPsBlock");
  test::expect_false(output.contains("EmptyBlock"));
  std::println(std::cout, "    ✓ All alliance blocks listing succeeded");

  // 2. Query player block membership (member, invited-only, pledged-only)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"block", "player", "1"});
  test::expect_contains(g.out.str(), "TestRace1");
  test::expect_contains(g.out.str(), "has been invited to join  2");
  test::expect_contains(g.out.str(), "has pledged  3");
  std::println(std::cout, "    ✓ Player block membership query succeeded");

  // 3. Query specific block power report (also covers 3-arg non-"player" path)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"block", "1", "extra"});
  test::expect_contains(g.out.str(), "ZeroVPBlock");
  test::expect_contains(g.out.str(), "TestRace1");
  test::expect_false(g.out.str().contains("TestRace3"));
  std::println(std::cout, "    ✓ Specific block report query succeeded");

  // 4. Error paths and JSON mode
  ctx.assert_dispatch_rejected(g, {"block", "player", "99"});
  ctx.assert_dispatch_rejected(g, {"block", "99"});
  ctx.assert_dispatch_rejected(g, {"block", "4"});

  g.set_ui_mode(UiMode::JSON);
  ctx.assert_dispatch_success(g, {"block"});
  test::expect_contains(g.out.str(), "\"list_report\"");
  ctx.assert_dispatch_success(g, {"block", "player", "1"});
  test::expect_contains(g.out.str(), "\"player_report\"");
  ctx.assert_dispatch_success(g, {"block", "1"});
  test::expect_contains(g.out.str(), "\"detail_report\"");
}

}  // namespace

int main() {
  test_block_dispatch();
  std::println(std::cout, "\n✅ All block tests passed!");
  return 0;
}
