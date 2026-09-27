// SPDX-License-Identifier: Apache-2.0

/// \file deferred_write_scope_test.cc
/// \brief Unit tests for EntityManager::DeferredWriteScope turn-batching write
/// guard.

import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_deferred_write_batch_persistence(TestContext& ctx) {
  // Setup Race 1 and Star 1
  Race r{};
  r.Playernum = 1;
  r.name = "BatchRace";
  r.tech = 100.0;
  {
    JsonStore store(ctx.db);
    RaceRepository repo(store);
    repo.save(r);
  }

  ctx.create_star("Sol", 1).build();

  // Open DeferredWriteScope and perform multi-pass mutations
  {
    auto scope = ctx.em.create_deferred_write_scope();
    test::expect_true(ctx.em.is_deferred_write());
    test::expect_false(scope.is_committed());

    // Pass 1: mutate race tech
    ctx.em.mutate_race(player_t{1}, [](Race& r) { r.tech = 150.0; });

    // Pass 2: mutate star name
    ctx.em.mutate_star(starnum_t{1}, [](Star& s) { s.set_name("Alpha Sol"); });

    // Pass 3: further mutate race tech
    ctx.em.mutate_race(player_t{1}, [](Race& r) { r.tech = 200.0; });

    // Still uncommitted before scope exit
    test::expect_false(scope.is_committed());
  }

  // Scope exited -> auto-committed
  test::expect_false(ctx.em.is_deferred_write());

  // Clear cache and verify DB has final accumulated values
  ctx.em.clear_cache();
  const auto* race = ctx.em.peek_race(player_t{1});
  test::expect_true(race != nullptr);
  test::expect_eq(race->tech, 200.0);

  const auto* star = ctx.em.peek_star(starnum_t{1});
  test::expect_true(star != nullptr);
  test::expect_eq(star->get_name(), "Alpha Sol");

  std::println(std::cout, "✓ test_deferred_write_batch_persistence passed");
}

void test_deferred_write_explicit_rollback(TestContext& ctx) {
  // Setup Race 2
  Race r{};
  r.Playernum = 2;
  r.name = "RollbackRace";
  r.tech = 50.0;
  {
    JsonStore store(ctx.db);
    RaceRepository repo(store);
    repo.save(r);
  }

  // Mutate in DeferredWriteScope then explicitly rollback
  {
    auto scope = ctx.em.create_deferred_write_scope();
    ctx.em.mutate_race(player_t{2}, [](Race& r) { r.tech = 999.0; });

    scope.rollback();
    test::expect_true(scope.is_committed());
    test::expect_false(ctx.em.is_deferred_write());
  }

  // Clear cache and verify DB retains original tech
  ctx.em.clear_cache();
  const auto* race = ctx.em.peek_race(player_t{2});
  test::expect_true(race != nullptr);
  test::expect_eq(race->tech, 50.0);

  std::println(std::cout, "✓ test_deferred_write_explicit_rollback passed");
}

void test_deferred_write_raii_rollback_on_exception(TestContext& ctx) {
  // Setup Race 3
  Race r{};
  r.Playernum = 3;
  r.name = "ExceptionRace";
  r.tech = 75.0;
  {
    JsonStore store(ctx.db);
    RaceRepository repo(store);
    repo.save(r);
  }

  // Mutate in scope and throw exception
  try {
    auto scope = ctx.em.create_deferred_write_scope();
    ctx.em.mutate_race(player_t{3}, [](Race& r) { r.tech = 888.0; });

    throw std::runtime_error("Simulated turn processing failure");
  } catch (const std::runtime_error&) {
    // Exception caught; DeferredWriteScope destructor should have rolled back
  }

  // Verify DB retains original tech
  ctx.em.clear_cache();
  const auto* race = ctx.em.peek_race(player_t{3});
  test::expect_true(race != nullptr);
  test::expect_eq(race->tech, 75.0);

  std::println(std::cout,
               "✓ test_deferred_write_raii_rollback_on_exception passed");
}

void test_deferred_write_multi_entity_simulation(TestContext& ctx) {
  // Setup Race 4, Ship 10, Star 2, Planet (2, 1)
  Race r{};
  r.Playernum = 4;
  r.name = "MultiRace";
  r.tech = 10.0;
  {
    JsonStore store(ctx.db);
    RaceRepository repo(store);
    repo.save(r);
  }

  ship_struct s_data{};
  s_data.number = 10;
  s_data.owner = 4;
  s_data.fuel = 500.0;
  s_data.alive = true;
  {
    JsonStore store(ctx.db);
    ShipRepository repo(store);
    repo.save(Ship(s_data));
  }

  ctx.create_star("Vega", 2).with_ap(4, 0).build();
  ctx.create_planet(2, PlanetType::EARTH, Coordinates{5, 5}, 1)
      .with_colony(4, 1000)
      .build();

  // Run multi-entity turn simulation pass in DeferredWriteScope
  {
    auto scope = ctx.em.create_deferred_write_scope();

    // 1. Race advances tech
    ctx.em.mutate_race(player_t{4}, [](Race& r) { r.tech += 5.0; });

    // 2. Ship consumes fuel
    ctx.em.mutate_ship(shipnum_t{10}, [](Ship& s) { s.consume_fuel(50.0); });

    // 3. Star regenerates AP
    ctx.em.mutate_star(starnum_t{2}, [](Star& s) { s.AP(player_t{4}) += 5; });

    // 4. Planet population grows
    ctx.em.mutate_planet(starnum_t{2}, planetnum_t{1},
                         [](Planet& p) { p.popn() += 200; });
  }

  // Clear cache and verify all 4 entities committed together
  ctx.em.clear_cache();

  const auto* race = ctx.em.peek_race(player_t{4});
  test::expect_true(race != nullptr);
  test::expect_eq(race->tech, 15.0);

  const auto* ship = ctx.em.peek_ship(shipnum_t{10});
  test::expect_true(ship != nullptr);
  test::expect_eq(ship->fuel(), 450.0);

  const auto* star_peek = ctx.em.peek_star(starnum_t{2});
  test::expect_true(star_peek != nullptr);
  test::expect_eq(star_peek->AP(player_t{4}), 5);

  const auto* planet_peek = ctx.em.peek_planet(starnum_t{2}, planetnum_t{1});
  test::expect_true(planet_peek != nullptr);
  test::expect_eq(planet_peek->popn(), 1200);

  std::println(std::cout,
               "✓ test_deferred_write_multi_entity_simulation passed");
}

void test_deferred_write_rejects_nested_transaction(TestContext& ctx) {
  // Opening DeferredWriteScope commit inside an active transaction throws
  // to enforce strict non-reentrancy invariant
  auto outer_txn = ctx.em.begin_transaction();
  {
    auto scope = ctx.em.create_deferred_write_scope();
    test::expect_throws<SqliteError>([&]() { scope.commit(); });
  }
  outer_txn.rollback();

  std::println(std::cout,
               "✓ test_deferred_write_rejects_nested_transaction passed");
}

void test_deferred_write_deletion_rollback_and_commit(TestContext& ctx) {
  // Setup Race 5 with flagship #20, docked fighter #21, escort #22, and market
  // lot #10
  Race r{};
  r.Playernum = 5;
  r.name = "AtomicDeleteRace";
  ctx.em.create_race(r);

  TestShipBuilder(ctx.em, ShipType::OTYPE_GOV, shipnum_t{20})
      .owned_by(5, 1)
      .landed_on(2, 1, Coordinates{0, 0})
      .build();
  TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER, shipnum_t{21})
      .owned_by(5, 1)
      .docked_to(20, 2)
      .build();
  TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER, shipnum_t{22})
      .owned_by(5, 1)
      .in_star_orbit(2)
      .build();
  ctx.em.mutate_ship(22, [](Ship& s) { s.protect().ship = 20; });
  ctx.em.mutate_race(5, [](Race& race) { race.Gov_ship = 20; });

  Commod c{};
  c.id = 10;
  c.owner = 5;
  c.governor = 1;
  c.type = CommodType::RESOURCE;
  c.amount = 250;
  c.star_from = 2;
  c.planet_from = 1;
  {
    JsonStore store(ctx.db);
    CommodRepository commod_repo(store);
    commod_repo.save(c);
  }
  ctx.em.clear_cache();

  // 1. Delete ship #20 (which recursively kills docked fighter #21 and scrubs
  // Gov_ship and escort #22's protect().ship) and commod #10 inside
  // DeferredWriteScope, then rollback
  {
    auto scope = ctx.em.create_deferred_write_scope();
    ctx.em.mutate_ship(20, [&](Ship& s) { ctx.em.kill_ship(5, s); });
    {
      auto barrier = ctx.em.create_deletion_barrier();
      ctx.em.delete_commod(10);
    }

    // Within the scope before rollback, deleted entities are invisible and
    // foreign key references are scrubbed in memory
    test::expect_throws<EntityNotFoundError>(
        [&]() { (void)ctx.em.peek_ship(20); });
    test::expect_throws<EntityNotFoundError>(
        [&]() { (void)ctx.em.peek_ship(21); });
    test::expect_throws<EntityNotFoundError>(
        [&]() { (void)ctx.em.peek_commod(10); });
    test::expect_eq(ctx.em.peek_race(5)->Gov_ship, std::nullopt);
    test::expect_eq(ctx.em.peek_ship(22)->protect().ship, std::nullopt);

    scope.rollback();
  }

  // Verify SQLite retained ships #20 and #21, commod #10, and all FK references
  const auto* restored_ship = ctx.em.peek_ship(20);
  test::expect_ne(restored_ship, nullptr);
  test::expect_true(restored_ship->alive());
  const auto* restored_fighter = ctx.em.peek_ship(21);
  test::expect_ne(restored_fighter, nullptr);
  test::expect_eq(restored_fighter->destshipno(), shipnum_t{20});
  test::expect_eq(ctx.em.peek_race(5)->Gov_ship, shipnum_t{20});
  test::expect_eq(ctx.em.peek_ship(22)->protect().ship, shipnum_t{20});
  const auto* restored_commod = ctx.em.peek_commod(10);
  test::expect_ne(restored_commod, nullptr);
  test::expect_eq(restored_commod->amount, 250);

  // 2. Delete ship #20 and commod #10 inside DeferredWriteScope and commit
  {
    auto scope = ctx.em.create_deferred_write_scope();
    ctx.em.mutate_ship(20, [&](Ship& s) { ctx.em.kill_ship(5, s); });
    {
      auto barrier = ctx.em.create_deletion_barrier();
      ctx.em.delete_commod(10);
    }
  }

  ctx.em.clear_cache();
  test::expect_throws<EntityNotFoundError>(
      [&]() { (void)ctx.em.peek_ship(20); });
  test::expect_throws<EntityNotFoundError>(
      [&]() { (void)ctx.em.peek_ship(21); });
  test::expect_throws<EntityNotFoundError>(
      [&]() { (void)ctx.em.peek_commod(10); });
  test::expect_eq(ctx.em.peek_race(5)->Gov_ship, std::nullopt);
  test::expect_eq(ctx.em.peek_ship(22)->protect().ship, std::nullopt);

  std::println(std::cout,
               "✓ test_deferred_write_deletion_rollback_and_commit passed");
}

}  // namespace

int main() {
  std::println(std::cout, "Running DeferredWriteScope tests...");

  TestContext ctx;
  test_deferred_write_batch_persistence(ctx);
  test_deferred_write_explicit_rollback(ctx);
  test_deferred_write_raii_rollback_on_exception(ctx);
  test_deferred_write_multi_entity_simulation(ctx);
  test_deferred_write_rejects_nested_transaction(ctx);
  test_deferred_write_deletion_rollback_and_commit(ctx);

  std::println(std::cout, "\nAll DeferredWriteScope tests passed!");
  return 0;
}
