// SPDX-License-Identifier: Apache-2.0

/// \file deferred_write_scope_test.cc
/// \brief Unit tests for EntityManager::DeferredWriteScope turn-batching write
/// guard.

import dallib;
import gblib;
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

  star_struct star_data{};
  star_data.star_id = 1;
  star_data.name = "Sol";
  {
    JsonStore store(ctx.db);
    StarRepository repo(store);
    repo.save(Star(star_data));
  }

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
  // Setup Race 4, Ship 10, Star 2, Planet (2, 0)
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

  star_struct star_data{};
  star_data.star_id = 2;
  Star star(star_data);
  star.AP(player_t{4}) = 0;
  {
    JsonStore store(ctx.db);
    StarRepository repo(store);
    repo.save(star);
  }

  planet_struct p_data{};
  p_data.star_id = 2;
  p_data.planet_order = 0;
  p_data.popn = 1000;
  {
    JsonStore store(ctx.db);
    PlanetRepository repo(store);
    repo.save(Planet(p_data));
  }

  // Run multi-entity turn simulation pass in DeferredWriteScope
  {
    auto scope = ctx.em.create_deferred_write_scope();

    // 1. Race advances tech
    ctx.em.mutate_race(player_t{4}, [](Race& r) { r.tech += 5.0; });

    // 2. Ship consumes fuel
    ctx.em.mutate_ship(shipnum_t{10}, [](Ship& s) { s.fuel() -= 50.0; });

    // 3. Star regenerates AP
    ctx.em.mutate_star(starnum_t{2}, [](Star& s) { s.AP(player_t{4}) += 5; });

    // 4. Planet population grows
    ctx.em.mutate_planet(starnum_t{2}, planetnum_t{0},
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

  const auto* planet_peek = ctx.em.peek_planet(starnum_t{2}, planetnum_t{0});
  test::expect_true(planet_peek != nullptr);
  test::expect_eq(planet_peek->popn(), 1200);

  std::println(std::cout,
               "✓ test_deferred_write_multi_entity_simulation passed");
}

}  // namespace

int main() {
  std::println(std::cout, "Running DeferredWriteScope tests...");

  TestContext ctx;
  test_deferred_write_batch_persistence(ctx);
  test_deferred_write_explicit_rollback(ctx);
  test_deferred_write_raii_rollback_on_exception(ctx);
  test_deferred_write_multi_entity_simulation(ctx);

  std::println(std::cout, "\nAll DeferredWriteScope tests passed!");
  return 0;
}
