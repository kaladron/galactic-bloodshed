// SPDX-License-Identifier: Apache-2.0

/// \file send_message_test.cc
/// \brief Unit tests for send message command and translation updates

import dallib;
import gb.entities;
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
    r.leader().active = true;
    r.leader().name = "TargetGovernor";
    r.translate[player_t{1}] = 50;
  });

  ctx.em.mutate_star(1, [](Star& s) {
    s.AP(1) = 10;
    s.inhabited().set(player_t{1});
    s.inhabited().set(player_t{2});
  });

  block b{};
  b.Playernum = 1;
  b.name = "Federation";
  b.motto = "Peace";
  b.invite(player_t{1});
  b.invite(player_t{2});
  b.pledge(player_t{1});
  b.pledge(player_t{2});
  JsonStore store(ctx.db);
  BlockRepository blocks(store);
  blocks.save(b);
}

void test_send_message_and_translation_cap() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  g.set_god(false);

  // Regular message: send 2 Hello World (costs 1 AP, increments translation by
  // 2)
  ctx.assert_dispatch_success(g, {"send", "2", "Hello", "World"}, 1);
  test::expect_contains(g.out.str(), "Message sent.");

  const auto* updated_receiver = ctx.em.peek_race(2);
  test::expect_true(updated_receiver != nullptr);
  test::expect_eq(updated_receiver->translate[player_t{1}], 52);

  // Verify translation modifier caps at 100
  ctx.em.mutate_race(2, [](Race& r) { r.translate[player_t{1}] = 99; });
  ctx.assert_dispatch_success(g, {"send", "2", "Cap", "Check"}, 1);
  test::expect_eq(ctx.em.peek_race(2)->translate[player_t{1}], 100);

  ctx.verify_universe_invariants();
}

void test_send_to_governor_and_self() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  g.set_god(false);

  // Send to specific governor 0 of player 2 (costs 1 AP)
  ctx.assert_dispatch_success(g, {"send", "2", "0", "Direct", "Order"}, 1);
  test::expect_contains(g.out.str(), "Message sent.");

  // Sending to oneself costs 0 AP
  g.out.str("");
  ctx.assert_dispatch_success(g, {"send", "1", "Note", "to", "self"}, 0);
  test::expect_contains(g.out.str(), "Message sent.");

  ctx.verify_universe_invariants();
}

void test_send_block_star_and_post() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  g.set_god(false);

  // Send to alliance block 1
  ctx.assert_dispatch_success(
      g, {"send", "block", "1", "Allied", "fleet", "mobilize"}, 1);
  test::expect_contains(g.out.str(), "Message sent.");

  // Send to star system /Sol
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"send", "star", "/Sol", "System", "wide", "broadcast"}, 1);
  test::expect_contains(g.out.str(), "Message sent.");

  // Post public message from star scope (post is free, 0 AP)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"post", "Galactic", "news", "bulletin"}, 0);

  // Post public message from universe scope (costs 0 AP)
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"post", "Universe", "bulletin"}, 0);

  ctx.verify_universe_invariants();
}

void test_send_validation_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
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
  ctx.assert_dispatch_rejected(g, {"send", "2", "0"});
  test::expect_contains(g.out.str(), "Syntax: send <race>");

  // Invalid block, player, governor, and star targets
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "block", "99", "Hello"});
  test::expect_contains(g.out.str(), "No such alliance block.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "99", "Hello"});
  test::expect_contains(g.out.str(), "No such player.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "2", "99", "Hello"});
  test::expect_contains(g.out.str(), "No such governor.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "star", "/Nowhere", "Hello"});

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "star", ".", "Hello"});
  test::expect_contains(g.out.str(), "No such star.");

  // Insufficient AP at star scope
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "2", "No", "AP"});
  test::expect_contains(g.out.str(), "You don't have 1 action points there.");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_send_message_and_translation_cap();
  test_send_to_governor_and_self();
  test_send_block_star_and_post();
  test_send_validation_errors();

  std::println(std::cout, "✓ send_message_test passed!");
  return 0;
}
