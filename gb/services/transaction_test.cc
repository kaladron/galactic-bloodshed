// SPDX-License-Identifier: Apache-2.0

/// \file transaction_test.cc
/// \brief Unit tests for EntityManager::Transaction Unit of Work and
/// dispatch_command transactional integration.

import dallib;
import gb.entities;
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
    ctx.em.mutate_ship(shipnum_t{10}, [](Ship& ship) { ship.fuel() = 9999.0; });
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
  star_struct star_data{};
  star_data.star_id = 1;
  Star star(star_data);
  star.AP(player_t{3}) = 5;
  {
    JsonStore store(ctx.db);
    StarRepository repo(store);
    repo.save(star);
  }

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
  ctx.setup_game_obj(g, 3, 0);
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
  star_struct star_data{};
  star_data.star_id = 2;
  Star star(star_data);
  star.AP(player_t{4}) = 5;
  {
    JsonStore store(ctx.db);
    StarRepository repo(store);
    repo.save(star);
  }

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
  ctx.setup_game_obj(g, 4, 0);
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

}  // namespace

int main() {
  std::println(std::cout, "Running Transaction tests...");

  TestContext ctx;
  test_transaction_commit(ctx);
  test_transaction_explicit_rollback(ctx);
  test_transaction_raii_rollback(ctx);
  test_dispatch_command_transaction_success(ctx);
  test_dispatch_command_transaction_failure(ctx);

  std::println(std::cout, "\nAll Transaction tests passed!");
  return 0;
}
