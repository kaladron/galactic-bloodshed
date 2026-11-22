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

  {
    auto ship10 = TestShipBuilder(ShipType::STYPE_SHUTTLE)
                      .numbered(10)
                      .owned_by(4)
                      .with_max_fuel(1000.0)
                      .with_fuel(500.0)
                      .build_in_memory();
    JsonStore store(ctx.db);
    ShipRepository repo(store);
    repo.save(*ship10);
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
  // Setup Race 5 with flagship, docked fighter, escort, and market lot #10
  Race r{};
  r.Playernum = 5;
  r.name = "AtomicDeleteRace";
  ctx.em.create_race(r);

  const shipnum_t ship20_id = TestShipBuilder(ctx.em, ShipType::OTYPE_GOV)
                                  .owned_by(5, 1)
                                  .landed_on(2, 1, Coordinates{0, 0})
                                  .build();
  const shipnum_t ship21_id = TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
                                  .owned_by(5, 1)
                                  .docked_to(ship20_id, 2)
                                  .build();
  const shipnum_t ship22_id = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                                  .owned_by(5, 1)
                                  .in_star_orbit(2)
                                  .build();
  ctx.em.mutate_ship(ship22_id, [&](Ship& s) { s.protect().ship = ship20_id; });
  ctx.em.mutate_race(5, [&](Race& race) { race.Gov_ship = ship20_id; });

  Commod c{{
      .id = 10,
      .owner = 5,
      .governor = 1,
      .type = CommodType::RESOURCE,
      .amount = 250,
      .star_from = 2,
      .planet_from = 1,
  }};
  {
    JsonStore store(ctx.db);
    CommodRepository commod_repo(store);
    commod_repo.save(c);
  }
  ctx.em.clear_cache();

  // 1. Delete ship20_id (which recursively kills docked fighter ship21_id and
  // scrubs Gov_ship and escort ship22_id's protect().ship) and commod #10
  // inside DeferredWriteScope, then rollback
  {
    auto scope = ctx.em.create_deferred_write_scope();
    ctx.em.mutate_ship(ship20_id, [&](Ship& s) { ctx.em.destroy_ship(s); });
    {
      auto barrier = ctx.em.create_deletion_barrier();
      ctx.em.delete_commod(10);
    }

    // Within the scope before rollback, deleted entities are invisible and
    // foreign key references are scrubbed in memory
    test::expect_throws<EntityNotFoundError>(
        [&]() { (void)ctx.em.peek_ship(ship20_id); });
    test::expect_throws<EntityNotFoundError>(
        [&]() { (void)ctx.em.peek_ship(ship21_id); });
    test::expect_throws<EntityNotFoundError>(
        [&]() { (void)ctx.em.peek_commod(10); });
    test::expect_eq(ctx.em.peek_race(5)->Gov_ship, std::nullopt);
    test::expect_eq(ctx.em.peek_ship(ship22_id)->protect().ship, std::nullopt);

    scope.rollback();
  }

  // Verify SQLite retained ships and commod #10, and all FK references
  const auto* restored_ship = ctx.em.peek_ship(ship20_id);
  test::expect_ne(restored_ship, nullptr);
  test::expect_true(restored_ship->alive());
  const auto* restored_fighter = ctx.em.peek_ship(ship21_id);
  test::expect_ne(restored_fighter, nullptr);
  test::expect_eq(restored_fighter->destshipno(), ship20_id);
  test::expect_eq(ctx.em.peek_race(5)->Gov_ship, ship20_id);
  test::expect_eq(ctx.em.peek_ship(ship22_id)->protect().ship, ship20_id);
  const auto* restored_commod = ctx.em.peek_commod(10);
  test::expect_ne(restored_commod, nullptr);
  test::expect_eq(restored_commod->amount(), 250);

  // 2. Delete ship20_id and commod #10 inside DeferredWriteScope and commit
  {
    auto scope = ctx.em.create_deferred_write_scope();
    ctx.em.mutate_ship(ship20_id, [&](Ship& s) { ctx.em.destroy_ship(s); });
    {
      auto barrier = ctx.em.create_deletion_barrier();
      ctx.em.delete_commod(10);
    }
  }

  ctx.em.clear_cache();
  test::expect_throws<EntityNotFoundError>(
      [&]() { (void)ctx.em.peek_ship(ship20_id); });
  test::expect_throws<EntityNotFoundError>(
      [&]() { (void)ctx.em.peek_ship(ship21_id); });
  test::expect_throws<EntityNotFoundError>(
      [&]() { (void)ctx.em.peek_commod(10); });
  test::expect_eq(ctx.em.peek_race(5)->Gov_ship, std::nullopt);
  test::expect_eq(ctx.em.peek_ship(ship22_id)->protect().ship, std::nullopt);

  std::println(std::cout,
               "✓ test_deferred_write_deletion_rollback_and_commit passed");
}

void test_deferred_write_spatial_and_ownership_cache_overlay() {
  TestContext ctx;
  const auto races = ctx.with_races();

  // Same-owner governor transfer preserves Gov_ship; ReadOnlyScope & unnumbered
  // local ships are guarded cleanly.
  ctx.em.mutate_ship(races.newcomer_gov_ship, [&](Ship& s) {
    ctx.em.transfer_ship_ownership(s, races.newcomer, Race::leader_id);
  });
  test::expect_eq(ctx.em.peek_race(races.newcomer)->Gov_ship,
                  races.newcomer_gov_ship);
  auto local_unnumbered =
      ShipFactory::create_from_template(ShipType::STYPE_SHUTTLE, 0);
  ctx.em.transfer_ship_ownership(*local_unnumbered, races.nomad,
                                 Race::leader_id);
  {
    auto ro = ctx.em.create_read_only_scope();
    test::expect_throws<ReadOnlyError>([&]() {
      ctx.em.transfer_ship_ownership(*local_unnumbered, races.empire,
                                     Race::leader_id);
    });
  }

  {
    auto scope = ctx.em.create_deferred_write_scope();

    // 1. Mid-turn spatial move: nomad cruiser moves from /Sol star orbit to
    // /Vega/Vega Prime planet orbit without flushing SQLite.
    ctx.em.mutate_ship(races.nomad_cruiser,
                       [](Ship& s) { s.enter_planet_orbit(2, 1); });
    test::expect_false(
        std::ranges::contains(ctx.em.ships_in_star(1), races.nomad_cruiser));
    test::expect_true(std::ranges::contains(ctx.em.ships_on_planet(2, 1),
                                            races.nomad_cruiser));
    test::expect_true(std::ranges::contains(ctx.em.ships_in_star_system(2),
                                            races.nomad_cruiser));
    test::expect_true(std::ranges::is_sorted(ctx.em.ships_on_planet(2, 1)));

    // 2. Mid-turn carrier berth: nomad shuttle moves to /Sol/Earth and docks
    // into empire cruiser.
    ctx.em.mutate_ship(races.nomad_shuttle,
                       [](Ship& s) { s.enter_planet_orbit(1, 1); });
    test::expect_true(
        ctx.em.dock_carrier(races.nomad_shuttle, races.empire_cruiser)
            .has_value());
    test::expect_false(std::ranges::contains(
        ctx.em.ships_at_scope(ScopeLevel::LEVEL_UNIV), races.nomad_shuttle));
    test::expect_eq(ctx.em.ships_in_hangar(races.empire_cruiser),
                    std::vector<shipnum_t>{races.nomad_shuttle});

    // 3. Ownership transfer of empire's Gov_ship scrubs empire.Gov_ship and
    // updates ships_by_owner immediately so destroying it later succeeds.
    ctx.em.mutate_ship(races.empire_gov_ship, [&](Ship& s) {
      ctx.em.transfer_ship_ownership(s, races.nomad, Race::leader_id);
    });
    test::expect_eq(ctx.em.peek_race(races.empire)->Gov_ship, std::nullopt);
    test::expect_false(std::ranges::contains(
        ctx.em.ships_by_owner(races.empire), races.empire_gov_ship));
    test::expect_true(std::ranges::contains(ctx.em.ships_by_owner(races.nomad),
                                            races.empire_gov_ship));

    // 4. Both destroy_ship and DeletionBarrier-queued delete_ship hide ships
    // from ships_alive() before SQLite commit.
    ctx.em.mutate_ship(races.empire_gov_ship,
                       [&](Ship& s) { ctx.em.destroy_ship(s); });
    test::expect_false(
        std::ranges::contains(ctx.em.ships_alive(), races.empire_gov_ship));
    {
      auto barrier = ctx.em.create_deletion_barrier();
      ctx.em.mutate_ship(races.empire_cruiser, [&](Ship& c) {
        const Ship& shuttle = *ctx.em.peek_ship(races.nomad_shuttle);
        c.unload_docked_craft(shuttle);
      });
      ctx.em.delete_ship(races.nomad_shuttle);
      test::expect_false(
          std::ranges::contains(ctx.em.ships_alive(), races.nomad_shuttle));
    }
  }

  ctx.em.clear_cache();
  ctx.verify_universe_invariants();
  std::println(
      std::cout,
      "✓ test_deferred_write_spatial_and_ownership_cache_overlay passed");
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
  test_deferred_write_spatial_and_ownership_cache_overlay();

  std::println(std::cout, "\nAll DeferredWriteScope tests passed!");
  return 0;
}
