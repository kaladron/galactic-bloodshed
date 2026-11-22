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

constexpr player_t donor = 2;
constexpr player_t recipient = 3;

void setup_dump_world(TestContext& ctx) {
  ctx.with_galaxy();
  TestWorldBuilder world(ctx);
  world.add_race("Federation", 100.0, false, donor);
  world.add_race("Klingons", 100.0, false, recipient);
  ctx.em.mutate_star(1, [](Star& s) {
    s.mark_explored_by(donor);
    s.mark_inhabited_by(donor);
    s.AP(donor) = 100;
  });
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.mark_explored_by(donor); });
  ctx.em.mutate_star(2, [](Star& s) { s.mark_explored_by(donor); });
  ctx.em.mutate_planet(2, 1, [](Planet& p) { p.mark_explored_by(donor); });
}

void test_dump_happy_paths() {
  TestContext ctx;
  setup_dump_world(ctx);

  // Mark Earth and Sol as unvisited by Klingons to test exploration data
  // transfer
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.info(recipient).explored = 0; });
  ctx.em.mutate_star(1, [](Star& s) { s.clear_explored_by(recipient); });

  // Mark Star 3 as unexplored by Federation to test skipping unexplored stars
  ctx.em.mutate_star(3, [](Star& s) { s.clear_explored_by(donor); });
  // Mark Planet (2, 1) as unexplored by Federation to test skipping unexplored
  // planets
  ctx.em.mutate_planet(2, 1, [](Planet& p) { p.info(donor).explored = false; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, donor, 1);
  g.set_scope_star(1);

  // 1. Dump exploration data (10 AP deducted via FixedStar)
  ctx.assert_dispatch_success(g, {"dump", "Klingons"}, 10);
  test::expect_contains(g.out.str(), "Exploration Data transferred");

  const auto* p_after = ctx.em.peek_planet(1, 1);
  test::expect_ne(p_after, nullptr);
  test::expect_true(p_after->info(recipient).explored);

  const auto* s_after = ctx.em.peek_star(1);
  test::expect_ne(s_after, nullptr);
  test::expect_true(s_after->is_explored_by(recipient));

  // Deity dump transfers even stars/planets not marked explored on donor_id
  ctx.em.mutate_star(3, [](Star& s) {
    s.clear_explored_by(donor);
    s.clear_explored_by(recipient);
  });
  g.set_god(true);
  ctx.assert_dispatch_success(g, {"dump", "Klingons"}, 0);
  test::expect_true(ctx.em.peek_star(3)->is_explored_by(recipient));
  g.set_god(false);

  // Verify persistence after clearing cache
  ctx.em.clear_cache();
  const auto* p_persisted = ctx.em.peek_planet(1, 1);
  test::expect_ne(p_persisted, nullptr);
  test::expect_true(p_persisted->info(recipient).explored);
}

void test_dump_insufficient_ap() {
  TestContext ctx;
  setup_dump_world(ctx);

  // Set Star AP to 5 (< 10)
  ctx.em.mutate_star(1, [](Star& s) { s.AP(donor) = 5; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, donor, 1);
  g.set_scope_star(1);

  ctx.assert_dispatch_rejected(g, {"dump", "Klingons"});
  test::expect_contains(g.out.str(), "You don't have 10 action points there.");
}

void test_dump_role_rejections() {
  TestContext ctx;
  setup_dump_world(ctx);

  // 1. Guest race rejection
  {
    ctx.em.mutate_race(recipient, [](Race& r) { r.Guest = true; });

    auto& registry = get_test_session_registry();
    GameObj g(ctx.em, registry);
    ctx.setup_game_obj(g, recipient, 1);
    g.set_scope_star(1);

    ctx.assert_dispatch_rejected(g, {"dump", "Federation"});
    test::expect_contains(g.out.str(), "Guest races cannot use this command.");
  }

  // 2. Leader-only rejection (Governor > 1)
  {
    auto& registry = get_test_session_registry();
    GameObj g(ctx.em, registry);
    ctx.setup_game_obj(g, donor, 2);
    g.set_scope_star(1);

    ctx.assert_dispatch_rejected(g, {"dump", "Klingons"});
    test::expect_contains(g.out.str(),
                          "Only the leader (Governor 1) may use this command.");
  }
}

void test_dump_domain_errors() {
  TestContext ctx;
  setup_dump_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, donor, 1);
  g.set_scope_star(1);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"dump"});
  test::expect_contains(g.out.str(), "Syntax: dump <player> [<place> ...]");

  // 2. Invalid player name
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dump", "NonExistentPlayer"});
  test::expect_contains(g.out.str(), "No such player");
}

void test_dump_specific_places() {
  TestContext ctx;
  setup_dump_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, donor, 1);
  g.set_scope_star(1);

  // Reset explored status on recipient for further tests
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.info(recipient).explored = 0; });
  ctx.em.mutate_star(1, [](Star& s) { s.clear_explored_by(recipient); });

  // 1. Dump specific star (/Sol)
  ctx.assert_dispatch_success(g, {"dump", "Klingons", "/Sol"}, 10);
  test::expect_contains(g.out.str(), "Exploration Data transferred");
  test::expect_true(ctx.em.peek_planet(1, 1)->info(recipient).explored);
  test::expect_true(ctx.em.peek_star(1)->is_explored_by(recipient));

  // Reset explored status on recipient for further tests
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.info(recipient).explored = 0; });
  ctx.em.mutate_star(1, [](Star& s) { s.clear_explored_by(recipient); });

  // 2. Dump specific planet (/Sol/Earth)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"dump", "Klingons", "/Sol/Earth"}, 10);
  test::expect_contains(g.out.str(), "Exploration Data transferred");
  test::expect_true(ctx.em.peek_planet(1, 1)->info(recipient).explored);

  // 3. Dump with universe, valid ship (#1), and invalid scopes (safely ignored)
  TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
      .owned_by(donor)
      .in_deep_space()
      .build();
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"dump", "Klingons", "/", "#1", "/NonExistentStar"}, 10);
  test::expect_contains(g.out.str(), "Exploration Data transferred");

  // 4. JSON mode
  g.set_ui_mode(UiMode::JSON);
  ctx.assert_dispatch_success(g, {"dump", "Klingons", "/Sol"}, 10);
  test::expect_contains(g.out.str(), R"("type":"message")");
  test::expect_contains(g.out.str(), "Exploration Data transferred");
}

void test_dump_matrix() {
  TestContext ctx;
  setup_dump_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, donor, 1);
  g.set_scope_star(1);

  TestCommandMatrix(ctx, "dump")
      .with_valid_argv({"dump", "Klingons"})
      .with_invalid_argv({"dump", "NonExistentPlayer"})
      .with_expected_star_ap(10)
      .run_matrix(g);
}

}  // namespace

int main() {
  test_dump_matrix();
  test_dump_happy_paths();
  test_dump_specific_places();
  test_dump_insufficient_ap();
  test_dump_role_rejections();
  test_dump_domain_errors();

  std::println(std::cout, "✓ dump_test passed!");
  return 0;
}
