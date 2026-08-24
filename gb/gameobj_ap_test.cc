// SPDX-License-Identifier: Apache-2.0

/// \file gameobj_ap_test.cc
/// \brief Unit tests for GameObj star and universe action point deduction and
/// validation.

import dallib;
import gblib;
import test;
import std;

namespace {

void test_deduct_ap_star() {
  TestContext ctx;
  JsonStore store(ctx.db);
  StarRepository star_repo(store);

  star_struct sdata{};
  sdata.star_id = 1;
  sdata.AP[0] = 20;  // Player 1 has 20 AP
  Star star{sdata};
  star_repo.save(star);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{0});

  // 1. Zero amount deduction succeeds without changing AP
  test::expect_true(g.deduct_ap(starnum_t{1}, 0));
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 20);

  // 2. Non-existent star or star 0 returns false
  test::expect_false(g.deduct_ap(starnum_t{0}, 5));
  test::expect_false(g.deduct_ap(starnum_t{999}, 5));

  // 3. Normal deduction
  test::expect_true(g.deduct_ap(starnum_t{1}, 5));
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 15);

  // 4. Insufficient AP fails and leaves AP unchanged
  test::expect_false(g.deduct_ap(starnum_t{1}, 20));
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 15);

  // 5. Sequential and exact deduction
  test::expect_true(g.deduct_ap(starnum_t{1}, 10));
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 5);
  test::expect_true(g.deduct_ap(starnum_t{1}, 5));
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 0);

  // 6. God mode bypasses AP check and leaves 0 AP intact
  g.set_god(true);
  test::expect_true(g.deduct_ap(starnum_t{1}, 50));
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 0);
}

void test_deduct_univ_ap() {
  TestContext ctx;
  JsonStore store(ctx.db);
  UniverseRepository universe_repo(store);

  universe_struct u{};
  u.id = 1;
  u.AP[0] = 25;  // Player 1 has 25 Univ AP
  universe_repo.save(u);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{0});

  // 1. Zero amount deduction succeeds
  test::expect_true(g.deduct_univ_ap(0));
  test::expect_eq(ctx.em.peek_universe()->AP[0], 25);

  // 2. Normal deduction
  test::expect_true(g.deduct_univ_ap(10));
  test::expect_eq(ctx.em.peek_universe()->AP[0], 15);

  // 3. Insufficient AP fails and leaves Univ AP unchanged
  test::expect_false(g.deduct_univ_ap(20));
  test::expect_eq(ctx.em.peek_universe()->AP[0], 15);

  // 4. Exact deduction to zero
  test::expect_true(g.deduct_univ_ap(15));
  test::expect_eq(ctx.em.peek_universe()->AP[0], 0);

  // 5. God mode bypasses Univ AP deduction
  g.set_god(true);
  test::expect_true(g.deduct_univ_ap(50));
  test::expect_eq(ctx.em.peek_universe()->AP[0], 0);
}

}  // namespace

int main() {
  test_deduct_ap_star();
  test_deduct_univ_ap();

  std::println(std::cout, "✓ gameobj_ap_test passed!");
  return 0;
}
