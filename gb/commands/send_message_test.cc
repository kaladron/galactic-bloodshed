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

constexpr player_t p1 = 2;
constexpr player_t p2 = 3;

void setup_test_world(TestContext& ctx) {
  ctx.with_galaxy();
  TestWorldBuilder world(ctx);
  world.add_race("Federation", 100.0, false, p1);
  world.add_race("Klingons", 100.0, false, p2);

  ctx.em.mutate_race(p1, [](Race& r) {
    r.God = false;
    r.leader().name = "TestGovernor";
    r.translate[p1] = 50;
  });

  ctx.em.mutate_race(p2, [](Race& r) {
    r.God = false;
    r.leader().name = "TargetGovernor";
    r.translate[p1] = 50;
  });

  ctx.em.mutate_star(1, [](Star& s) {
    s.AP(p1) = 10;
    s.mark_explored_by(p1);
    s.mark_explored_by(p2);
    s.mark_inhabited_by(p1);
    s.mark_inhabited_by(p2);
  });

  Block b{{
      .Playernum = p1,
      .name = "Federation",
      .motto = "Peace",
      .invited = {p1, p2},
      .pledged = {p1, p2},
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
  ctx.setup_game_obj(g, p1, 1);
  g.set_scope_star(1);
  g.set_god(false);

  // Regular message: send 3 Hello World (costs 1 AP, increments translation by
  // 2)
  ctx.assert_dispatch_success(g, {"send", "3", "Hello", "World"}, 1);
  test::expect_contains(g.out.str(), "Message sent.");

  const auto* updated_receiver = ctx.em.peek_race(p2);
  test::expect_true(updated_receiver != nullptr);
  test::expect_eq(updated_receiver->translation_for(p1), 52);

  // Verify translation modifier caps at 100
  ctx.em.mutate_race(p2, [](Race& r) { r.translate[p1] = 99; });
  ctx.assert_dispatch_success(g, {"send", "3", "Cap", "Check"}, 1);
  test::expect_eq(ctx.em.peek_race(p2)->translation_for(p1), 100);

  ctx.verify_universe_invariants();
}

void test_send_to_governor_self_deity_and_json() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, p1, 1);
  g.set_scope_star(1);
  g.set_god(false);

  // Send to specific governor 1 of player 3 (costs 1 AP)
  ctx.assert_dispatch_success(g, {"send", "3", "1", "Direct", "Order"}, 1);
  test::expect_contains(g.out.str(), "Message sent.");

  // Sending to oneself costs 0 AP (including empty first message token)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"send", "2", "", "Note"}, 0);
  test::expect_contains(g.out.str(), "Message sent.");

  // Sending to a deity race costs 0 AP
  ctx.em.mutate_race(p2, [](Race& r) { r.God = true; });
  g.out.str("");
  ctx.assert_dispatch_success(g, {"send", "3", "Petition"}, 0);
  ctx.em.mutate_race(p2, [](Race& r) { r.God = false; });

  // Sending as deity costs 0 AP and renders JSON in UiMode::JSON
  g.set_god(true);
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"send", "3", "Decree"}, 0);
  test::expect_contains(g.out.str(), R"("type":"message")");
  test::expect_contains(g.out.str(), "Message sent.\\n");

  ctx.verify_universe_invariants();
}

void test_send_block_and_star() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, p1, 1);
  g.set_scope_star(1);
  g.set_god(false);

  // Send to alliance block 2 (costs 1 AP)
  ctx.assert_dispatch_success(
      g, {"send", "block", "2", "Allied", "fleet", "mobilize"}, 1);
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
  ctx.assert_dispatch_success(g, {"send", "block", "2", "Broadcast"}, 0);
  test::expect_contains(g.out.str(), R"("type":"message")");
  test::expect_contains(
      g.out.str(), "Sending message to alliance block.\\nMessage sent.\\n");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"send", "star", "/Sol", "Ping"}, 0);
  test::expect_contains(g.out.str(), R"("type":"message")");
  test::expect_contains(g.out.str(),
                        "Sending message to star system.\\nMessage sent.\\n");

  ctx.verify_universe_invariants();
}

void test_send_validation_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, p1, 1);
  g.set_scope_star(1);
  g.set_god(false);

  // Missing message arguments for block, star, and governor forms
  ctx.assert_dispatch_rejected(g, {"send", "block", "2"});
  test::expect_contains(g.out.str(), "Syntax: send block");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "star", "/Sol"});
  test::expect_contains(g.out.str(), "Syntax: send star");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "3", "1"});
  test::expect_contains(g.out.str(), "Syntax: send <race>");

  // Invalid block, player, governor, and star targets
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "block", "99", "Hello"});
  test::expect_eq(g.out.str(), "No such alliance block.\n");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "99", "Hello"});
  test::expect_contains(g.out.str(), "No such player.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "3", "0", "Hello"});
  test::expect_contains(g.out.str(), "No such governor.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g,
                               {"send", "3", "99999999999999999999", "Hello"});
  test::expect_contains(g.out.str(), "No such governor.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "3", "1abc", "Hello"});
  test::expect_contains(g.out.str(), "No such governor.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "3", "99", "Hello"});
  test::expect_contains(g.out.str(), "No such governor.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "star", "/Nowhere", "Hello"});

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "star", "/Sol/Earth", "Hello"});
  test::expect_eq(g.out.str(), "No such star.\n");

  // Insufficient AP at star scope (no spurious "Sending message..." output)
  ctx.em.mutate_star(1, [](Star& s) { s.AP(p1) = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "3", "No", "AP"});
  test::expect_eq(g.out.str(), "You don't have 1 action points there.\n");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "block", "2", "No", "AP"});
  test::expect_eq(g.out.str(), "You don't have 1 action points there.\n");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "star", "/Sol", "No", "AP"});
  test::expect_eq(g.out.str(), "You don't have 1 action points there.\n");

  ctx.verify_universe_invariants();
}

void test_send_universe_scope_deity_messaging() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, p1, 1);
  g.set_scope_univ();
  g.set_god(false);

  // 1. Mortal sending to mortal (costs 1 AP): rejected at LEVEL_UNIV
  ctx.assert_dispatch_rejected(g, {"send", "3", "Hello", "Mortal"});
  test::expect_eq(g.out.str(), "Invalid scope for this command.\n");

  // Mortal sending to block at LEVEL_UNIV or deep-space LEVEL_SHIP (snum == 0):
  // rejected
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "block", "2", "Allied", "fleet"});
  test::expect_eq(g.out.str(), "Invalid scope for this command.\n");

  const shipnum_t deep_ship = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                                  .owned_by(p1, 1)
                                  .in_deep_space()
                                  .build();
  ctx.assert_dispatch_success(g, {"cs", std::format("#{}", deep_ship)});
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"send", "3", "Deep", "Message"});
  test::expect_eq(g.out.str(), "Invalid scope for this command.\n");
  g.set_scope_univ();

  // 2. Mortal sending to Deity (alien.God == true, ap_cost == 0): succeeds from
  // LEVEL_UNIV
  ctx.em.mutate_race(p2, [](Race& r) { r.God = true; });
  g.out.str("");
  ctx.assert_dispatch_success(g, {"send", "3", "Prayer", "to", "Deity"}, 0);
  test::expect_contains(g.out.str(), "Message sent.");

  // 3. Deity sending from LEVEL_UNIV with snum == 0: succeeds with 0 AP
  g.set_god(true);
  ctx.em.mutate_race(p2, [](Race& r) { r.God = false; });

  // Deity to mortal
  g.out.str("");
  ctx.assert_dispatch_success(g, {"send", "3", "Divine", "Edict"}, 0);
  test::expect_contains(g.out.str(), "Message sent.");

  // Deity to alliance block
  g.out.str("");
  ctx.assert_dispatch_success(g, {"send", "block", "2", "Divine", "Blessing"},
                              0);
  test::expect_contains(g.out.str(), "Message sent.");

  // Deity to star system /Sol
  g.out.str("");
  ctx.assert_dispatch_success(g, {"send", "star", "/Sol", "Solar", "Flare"}, 0);
  test::expect_contains(g.out.str(), "Message sent.");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_send_message_and_translation_cap();
  test_send_to_governor_self_deity_and_json();
  test_send_block_and_star();
  test_send_validation_errors();
  test_send_universe_scope_deity_messaging();

  std::println(std::cout, "✓ send_message_test passed!");
  return 0;
}
