// SPDX-License-Identifier: Apache-2.0

/// \file send_message_test.cc
/// \brief Unit tests for send message command and translation updates

import dallib;
import gb.entities;
import gb.presentation;
import gb.repositories;
import gb.services;
import test;
import commands;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();

  ctx.em.mutate_race(1, [](Race& r) {
    r.God = false;
    r.leader().name = "TestGovernor";
    r.translate[player_t{1}] = 50;
  });

  ctx.em.mutate_race(2, [](Race& r) {
    r.God = false;
    r.leader().name = "TargetGovernor";
    r.translate[player_t{1}] = 50;
  });

  ctx.em.mutate_star(1, [](Star& s) {
    s.AP(1) = 10;
    s.mark_inhabited_by(player_t{1});
    s.mark_inhabited_by(player_t{2});
  });

  Block b{{
      .Playernum = 1,
      .name = "Federation",
      .motto = "Peace",
      .invited = {player_t{1}, player_t{2}},
      .pledged = {player_t{1}, player_t{2}},
  }};
  JsonStore store(ctx.db);
  BlockRepository blocks(store);
  blocks.save(b);
}

void test_send_message_and_translation_cap() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  g.set_god(false);

  // Regular message: send 2 Hello World (costs 1 AP, increments translation by
  // 2)
  ctx.assert_dispatch_success(g, {"send", "2", "Hello", "World"}, 1);
  test::expect_contains(g.out.str(), "Message sent.");

  const auto* updated_receiver = ctx.em.peek_race(2);
  test::expect_true(updated_receiver != nullptr);
  test::expect_eq(updated_receiver->translation_for(player_t{1}), 52);

  // Verify translation modifier caps at 100
  ctx.em.mutate_race(2, [](Race& r) { r.translate[player_t{1}] = 99; });
  ctx.assert_dispatch_success(g, {"send", "2", "Cap", "Check"}, 1);
  test::expect_eq(ctx.em.peek_race(2)->translation_for(player_t{1}), 100);

  ctx.verify_universe_invariants();
}

void test_send_to_governor_self_deity_and_json() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  g.set_god(false);

  // Send to specific governor 1 of player 2 (costs 1 AP)
  ctx.assert_dispatch_success(g, {"send", "2", "1", "Direct", "Order"}, 1);
  test::expect_contains(g.out.str(), "Message sent.");

  // Sending to oneself costs 0 AP (including empty first message token)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"send", "1", "", "Note"}, 0);
  test::expect_contains(g.out.str(), "Message sent.");

  // Sending to a deity race costs 0 AP
  ctx.em.mutate_race(2, [](Race& r) { r.God = true; });
  g.out.str("");
  ctx.assert_dispatch_success(g, {"send", "2", "Petition"}, 0);
  ctx.em.mutate_race(2, [](Race& r) { r.God = false; });

  // Sending as deity costs 0 AP and renders JSON in UiMode::JSON
  g.set_god(true);
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"send", "2", "Decree"}, 0);
  test::expect_contains(g.out.str(), "\"type\":\"send\"");
  test::expect_contains(g.out.str(), "\"target\":0");

  ctx.verify_universe_invariants();
}

void test_send_block_and_star() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  g.set_god(false);

  // Send to alliance block 1 (costs 1 AP)
  ctx.assert_dispatch_success(
      g, {"send", "block", "1", "Allied", "fleet", "mobilize"}, 1);
  test::expect_contains(g.out.str(), "Sending message to alliance block.");
  test::expect_contains(g.out.str(), "Message sent.");

  // Send to star system /Sol (costs 1 AP)
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"send", "star", "/Sol", "System", "wide", "broadcast"}, 1);
  test::expect_contains(g.out.str(), "Sending message to star system.");
  test::expect_contains(g.out.str(), "Message sent.");

  // Deity send in JSON mode costs 0 AP
  g.set_god(true);
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"send", "block", "1", "Broadcast"}, 0);
  test::expect_contains(g.out.str(), "\"target\":1");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"send", "star", "/Sol", "Ping"}, 0);
  test::expect_contains(g.out.str(), "\"target\":2");

  ctx.verify_universe_invariants();
}

void test_send_validation_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  g.set_god(false);

  // Missing message arguments for block, star, and governor forms
  ctx.assert_dispatch_rejected(g, {"send", "block", "1"});
  test::expect_contains(g.out.str(), "Syntax: send block");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "star", "/Sol"});
  test::expect_contains(g.out.str(), "Syntax: send star");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "2", "1"});
  test::expect_contains(g.out.str(), "Syntax: send <race>");

  // Invalid block, player, governor, and star targets
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "block", "99", "Hello"});
  test::expect_eq(g.out.str(), "No such alliance block.\n");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "99", "Hello"});
  test::expect_contains(g.out.str(), "No such player.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "2", "0", "Hello"});
  test::expect_contains(g.out.str(), "No such governor.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g,
                               {"send", "2", "99999999999999999999", "Hello"});
  test::expect_contains(g.out.str(), "No such governor.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "2", "99", "Hello"});
  test::expect_contains(g.out.str(), "No such governor.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "star", "/Nowhere", "Hello"});

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "star", "/Sol/Earth", "Hello"});
  test::expect_eq(g.out.str(), "No such star.\n");

  // Insufficient AP at star scope (no spurious "Sending message..." output)
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "2", "No", "AP"});
  test::expect_eq(g.out.str(), "You don't have 1 action points there.\n");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "block", "1", "No", "AP"});
  test::expect_eq(g.out.str(), "You don't have 1 action points there.\n");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "star", "/Sol", "No", "AP"});
  test::expect_eq(g.out.str(), "You don't have 1 action points there.\n");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_send_message_and_translation_cap();
  test_send_to_governor_self_deity_and_json();
  test_send_block_and_star();
  test_send_validation_errors();

  std::println(std::cout, "✓ send_message_test passed!");
  return 0;
}
