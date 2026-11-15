// SPDX-License-Identifier: Apache-2.0

/// \file transaction_test.cc
/// \brief Unit tests for EntityManager::Transaction Unit of Work and
/// dispatch_command transactional integration.

import dallib;
import gb.entities;
import gb.presentation;
import gb.services;
import commands;
import test;
import std;

namespace {

void test_transaction_commit(TestContext& ctx) {
  // Create test race
  Race r{};
  r.Playernum = 1;
  r.name = "CommitRace";
  r.tech = 100.0;
  {
    JsonStore store(ctx.db);
    RaceRepository repo(store);
    repo.save(r);
  }

  // Mutate within transaction and commit
  {
    auto txn = ctx.em.begin_transaction();
    test::expect_true(txn.is_active());

    ctx.em.mutate_race(player_t{1}, [](Race& r) { r.tech = 250.0; });

    txn.commit();
    test::expect_false(txn.is_active());
  }

  // Clear in-memory cache and verify DB contains updated tech
  ctx.em.clear_cache();
  const auto* race = ctx.em.peek_race(player_t{1});
  test::expect_true(race != nullptr);
  test::expect_eq(race->tech, 250.0);

  std::println(std::cout, "✓ test_transaction_commit passed");
}

void test_transaction_explicit_rollback(TestContext& ctx) {
  // Create test race
  Race r{};
  r.Playernum = 2;
  r.name = "RollbackRace";
  r.tech = 100.0;
  {
    JsonStore store(ctx.db);
    RaceRepository repo(store);
    repo.save(r);
  }

  // Mutate within transaction and explicitly rollback
  {
    auto txn = ctx.em.begin_transaction();
    test::expect_true(txn.is_active());

    ctx.em.mutate_race(player_t{2}, [](Race& r_mut) { r_mut.tech = 999.0; });

    txn.rollback();
    test::expect_false(txn.is_active());
  }

  // Verify that peek returns original tech (cache was cleared and DB rolled
  // back)
  const auto* race = ctx.em.peek_race(player_t{2});
  test::expect_true(race != nullptr);
  test::expect_eq(race->tech, 100.0);

  std::println(std::cout, "✓ test_transaction_explicit_rollback passed");
}

void test_transaction_raii_rollback(TestContext& ctx) {
  // Create test ship
  ship_struct s_data{};
  s_data.number = 10;
  s_data.owner = 1;
  s_data.fuel = 500.0;
  s_data.alive = true;
  {
    JsonStore store(ctx.db);
    ShipRepository repo(store);
    repo.save(Ship(s_data));
  }

  // Open transaction in scope, mutate ship, but do NOT call commit()
  {
    auto txn = ctx.em.begin_transaction();
    ctx.em.mutate_ship(shipnum_t{10},
                       [](Ship& ship) { ship.consume_fuel(100.0); });
    // txn goes out of scope here without commit -> RAII rollback triggered
  }

  // Verify ship retains original fuel in DB and cache
  const auto* ship = ctx.em.peek_ship(shipnum_t{10});
  test::expect_true(ship != nullptr);
  test::expect_eq(ship->fuel(), 500.0);

  std::println(std::cout, "✓ test_transaction_raii_rollback passed");
}

void test_dispatch_command_transaction_success(TestContext& ctx) {
  ctx.em.clear_cache();

  // Setup star 1 with AP
  ctx.create_star("Sol", 1).with_ap(player_t{3}, 5).build();

  // Setup Race 3
  Race r{};
  r.Playernum = 3;
  r.name = "DispatchRace";
  r.tech = 50.0;
  {
    JsonStore store(ctx.db);
    RaceRepository repo(store);
    repo.save(r);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 3, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // Custom command descriptor that mutates tech and returns true
  static const GB::commands::CommandDescriptor success_cmd{
      .name = "mock_txn_success",
      .roles = {},
      .scopes = GB::commands::AllowedScopes::any(),
      .ap = GB::commands::APCost::fixed_star(2),
      .min_args = 1,
      .syntax = "mock_txn_success",
      .description = "Mock success command",
      .handler = [](const command_t&, GameObj& game) -> bool {
        game.entity_manager.mutate_race(player_t{3},
                                        [](Race& r) { r.tech = 75.0; });
        return true;
      },
  };

  bool res =
      GB::commands::dispatch_command(g, success_cmd, {"mock_txn_success"});
  test::expect_true(res);

  // Verify tech is updated and 2 AP was deducted from Star
  ctx.em.clear_cache();
  const auto* race = ctx.em.peek_race(player_t{3});
  test::expect_true(race != nullptr);
  test::expect_eq(race->tech, 75.0);

  const auto* star_peek = ctx.em.peek_star(starnum_t{1});
  test::expect_true(star_peek != nullptr);
  test::expect_eq(star_peek->AP(player_t{3}), 3);

  std::println(std::cout, "✓ test_dispatch_command_transaction_success passed");
}

void test_dispatch_command_transaction_failure(TestContext& ctx) {
  ctx.em.clear_cache();

  // Setup star 2 with AP
  ctx.create_star("Vega", 2).with_ap(player_t{4}, 5).build();

  // Setup Race 4
  Race r{};
  r.Playernum = 4;
  r.name = "FailRace";
  r.tech = 50.0;
  {
    JsonStore store(ctx.db);
    RaceRepository repo(store);
    repo.save(r);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 4, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(2);

  // Custom command descriptor that mutates tech and returns false (domain
  // error)
  static const GB::commands::CommandDescriptor fail_cmd{
      .name = "mock_txn_fail",
      .roles = {},
      .scopes = GB::commands::AllowedScopes::any(),
      .ap = GB::commands::APCost::fixed_star(2),
      .min_args = 1,
      .syntax = "mock_txn_fail",
      .description = "Mock fail command",
      .handler = [](const command_t&, GameObj& game) -> bool {
        game.entity_manager.mutate_race(player_t{4},
                                        [](Race& r) { r.tech = 999.0; });
        return false;  // Fails!
      },
  };

  bool res = GB::commands::dispatch_command(g, fail_cmd, {"mock_txn_fail"});
  test::expect_false(res);

  // Verify tech was rolled back to 50.0 and 0 AP was deducted
  const auto* race = ctx.em.peek_race(player_t{4});
  test::expect_true(race != nullptr);
  test::expect_eq(race->tech, 50.0);

  const auto* star_peek = ctx.em.peek_star(starnum_t{2});
  test::expect_true(star_peek != nullptr);
  test::expect_eq(star_peek->AP(player_t{4}), 5);

  std::println(std::cout, "✓ test_dispatch_command_transaction_failure passed");
}

void test_transaction_rollback_clears_pending_deletions(TestContext& ctx) {
  ctx.em.clear_cache();

  ctx.em.mutate_star(starnum_t{1}, [](Star& star) {
    star.set_planet_name(planetnum_t{1}, "Earth");
  });
  {
    JsonStore store(ctx.db);
    PlanetRepository(store).save(Planet{{.star_id = 1, .planet_order = 1}});
  }

  const shipnum_t ship_id = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                                .owned_by(1, 1)
                                .in_deep_space()
                                .build();
  int commod_id = 0;
  {
    auto ch = ctx.em.create_commod(commod_struct{
        .owner = 1,
        .governor = 1,
        .type = CommodType::FUEL,
        .amount = 10,
        .star_from = 1,
        .planet_from = 1,
    });
    commod_id = ch->id();
  }

  // Queue ship and commodity deletions behind a DeletionBarrier, then rollback
  // the transaction before the barrier drains.
  {
    auto txn = ctx.em.begin_transaction();
    auto barrier = ctx.em.create_deletion_barrier();
    ctx.em.mutate_ship(ship_id, [&](Ship& ship) { ctx.em.destroy_ship(ship); });
    ctx.em.delete_commod(commod_id);
    txn.rollback();
  }

  // Subsequent DeletionBarrier drain must NOT delete the rolled-back entities.
  {
    auto barrier = ctx.em.create_deletion_barrier();
  }
  ctx.em.clear_cache();
  const auto* ship = ctx.em.peek_ship(ship_id);
  test::expect_ne(ship, nullptr);
  test::expect_true(ship->alive());
  test::expect_ne(ctx.em.peek_commod(commod_id), nullptr);

  std::println(std::cout,
               "✓ test_transaction_rollback_clears_pending_deletions passed");
}

void test_dispatch_rebinds_g_race_in_json_mode(TestContext& ctx) {
  ctx.em.clear_cache();

  Race mortal{};
  mortal.Playernum = 5;
  mortal.name = "JsonRollbackRace";
  mortal.governor(governor_t{1}).name = "JsonLeader";

  Race guest{};
  guest.Playernum = 6;
  guest.name = "JsonGuestRace";
  guest.Guest = true;
  guest.governor(governor_t{1}).name = "GuestLeader";

  {
    JsonStore store(ctx.db);
    RaceRepository repo(store);
    repo.save(mortal);
    repo.save(guest);
    UniverseRepository(store).save(Universe{});
  }

  auto& registry = get_test_session_registry();

  // 1. Failed transactional command rebinds g.race for JSON do_prompt
  {
    GameObj g(ctx.em, registry);
    ctx.setup_game_obj(g, 5, 1);
    g.set_ui_mode(UiMode::JSON);

    static const GB::commands::CommandDescriptor fail_json_cmd{
        .name = "mock_json_fail",
        .scopes = GB::commands::AllowedScopes::any(),
        .handler = [](const command_t&, GameObj& game) -> bool {
          game.entity_manager.mutate_race(player_t{5},
                                          [](Race& r) { r.tech = 123.0; });
          return false;
        },
    };

    test::expect_false(
        GB::commands::dispatch_command(g, fail_json_cmd, {"mock_json_fail"}));
    test::expect_eq(g.race, ctx.em.peek_race(player_t{5}));
    test::expect_contains(GB::presentation::do_prompt(g),
                          R"("race_name":"JsonRollbackRace")");
  }

  // 2. Guest ReadOnlyError rollback rebinds g.race for JSON do_prompt
  {
    GameObj g(ctx.em, registry);
    ctx.setup_game_obj(g, 6, 1);
    g.set_ui_mode(UiMode::JSON);

    static const GB::commands::CommandDescriptor guest_mutate_cmd{
        .name = "mock_guest_mutate",
        .scopes = GB::commands::AllowedScopes::any(),
        .handler = [](const command_t&, GameObj& game) -> bool {
          game.entity_manager.mutate_race(player_t{6},
                                          [](Race& r) { r.tech = 50.0; });
          return true;
        },
    };

    test::expect_false(GB::commands::dispatch_command(g, guest_mutate_cmd,
                                                      {"mock_guest_mutate"}));
    test::expect_eq(g.race, ctx.em.peek_race(player_t{6}));
    test::expect_contains(GB::presentation::do_prompt(g),
                          R"("race_name":"JsonGuestRace")");
  }

  // 3. Non-transactional cache-clearing command rebinds g.race
  {
    GameObj g(ctx.em, registry);
    ctx.setup_game_obj(g, 5, 1);
    g.set_ui_mode(UiMode::JSON);

    static const GB::commands::CommandDescriptor clear_cache_cmd{
        .name = "mock_clear_cache",
        .scopes = GB::commands::AllowedScopes::any(),
        .handler = [](const command_t&, GameObj& game) -> bool {
          game.entity_manager.clear_cache();
          return true;
        },
        .transactional = false,
    };

    test::expect_true(GB::commands::dispatch_command(g, clear_cache_cmd,
                                                     {"mock_clear_cache"}));
    test::expect_eq(g.race, ctx.em.peek_race(player_t{5}));
    test::expect_contains(GB::presentation::do_prompt(g),
                          R"("race_name":"JsonRollbackRace")");
  }

  std::println(std::cout, "✓ test_dispatch_rebinds_g_race_in_json_mode passed");
}

}  // namespace

int main() {
  std::println(std::cout, "Running Transaction tests...");

  TestContext ctx;
  test_transaction_commit(ctx);
  test_transaction_explicit_rollback(ctx);
  test_transaction_raii_rollback(ctx);
  test_dispatch_command_transaction_success(ctx);
  test_dispatch_command_transaction_failure(ctx);
  test_transaction_rollback_clears_pending_deletions(ctx);
  test_dispatch_rebinds_g_race_in_json_mode(ctx);

  std::println(std::cout, "\nAll Transaction tests passed!");
  return 0;
}
