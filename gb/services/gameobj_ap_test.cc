// SPDX-License-Identifier: Apache-2.0

/// \file gameobj_ap_test.cc
/// \brief Unit tests for GameObj star and universe action point deduction and
/// validation.

import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_deduct_ap_star() {
  TestContext ctx;
  ctx.create_star("Sol", 1).with_ap(player_t{1}, 20).build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{1});

  // 1. Zero amount deduction succeeds without changing AP
  test::expect_true(g.deduct_ap(starnum_t{1}, 0));
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 20);

  // 2. Non-existent star or star 0 fails fast with EntityNotFoundError
  test::expect_throws<EntityNotFoundError>(
      [&]() { (void)g.deduct_ap(starnum_t{0}, 5); });
  test::expect_throws<EntityNotFoundError>(
      [&]() { (void)g.deduct_ap(starnum_t{999}, 5); });

  // 3. Corrupted player ID (0) fails fast with std::out_of_range
  g.set_player(player_t{0});
  test::expect_throws<std::out_of_range>(
      [&]() { (void)g.deduct_ap(starnum_t{1}, 5); });
  g.set_player(player_t{1});

  // 4. Normal deduction
  test::expect_true(g.deduct_ap(starnum_t{1}, 5));
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 15);

  // 5. Insufficient AP fails and leaves AP unchanged
  test::expect_false(g.deduct_ap(starnum_t{1}, 20));
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 15);

  // 6. Sequential and exact deduction
  test::expect_true(g.deduct_ap(starnum_t{1}, 10));
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 5);
  test::expect_true(g.deduct_ap(starnum_t{1}, 5));
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 0);

  // 7. God mode bypasses AP check and leaves 0 AP intact
  g.set_god(true);
  test::expect_true(g.deduct_ap(starnum_t{1}, 50));
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 0);
}

void test_deduct_univ_ap() {
  TestContext ctx;
  JsonStore store(ctx.db);
  UniverseRepository universe_repo(store);

  universe_struct u{};
  u.set_AP(1, 25);  // Player 1 has 25 Univ AP
  universe_repo.save(u);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{1});

  // 1. Zero amount deduction succeeds
  test::expect_true(g.deduct_univ_ap(0));
  test::expect_eq(ctx.em.peek_universe()->get_AP(1), 25);

  // 2. Corrupted player ID (0) fails fast with std::out_of_range
  g.set_player(player_t{0});
  test::expect_throws<std::out_of_range>([&]() { (void)g.deduct_univ_ap(5); });
  g.set_player(player_t{1});

  // 3. Normal deduction
  test::expect_true(g.deduct_univ_ap(10));
  test::expect_eq(ctx.em.peek_universe()->get_AP(1), 15);

  // 4. Insufficient AP fails and leaves Univ AP unchanged
  test::expect_false(g.deduct_univ_ap(20));
  test::expect_eq(ctx.em.peek_universe()->get_AP(1), 15);

  // 5. Exact deduction to zero
  test::expect_true(g.deduct_univ_ap(15));
  test::expect_eq(ctx.em.peek_universe()->get_AP(1), 0);

  // 6. God mode bypasses Univ AP deduction
  g.set_god(true);
  test::expect_true(g.deduct_univ_ap(50));
  test::expect_eq(ctx.em.peek_universe()->get_AP(1), 0);
}

void test_validate_commandable() {
  TestContext ctx;
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) {
    r.appoint_governor(2);
    r.appoint_governor(3);
  });

  const auto ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_DESTROYER, 42)
                           .owned_by(1, 2)
                           .named("Valkyrie")
                           .in_star_orbit(1)
                           .build();
  const Ship& base_ship = *ctx.em.peek_ship(ship_id);

  struct Case {
    std::string_view label;
    player_t player;
    governor_t governor;
    bool god;
    bool alive;
    bool active;
    std::optional<CommandableError> expected_error;
    std::string_view expected_output;
  };

  const std::array<Case, 9> cases{{
      {"assigned governor succeeds", 1, 2, false, true, true, std::nullopt, ""},
      {"race leader succeeds", 1, 1, false, true, true, std::nullopt, ""},
      {"wrong player fails NotOwner", 2, 1, false, true, true,
       CommandableError::NotOwner, "You don't own ship #42."},
      {"unauthorized governor fails NotAuthorizedGovernor", 1, 3, false, true,
       true, CommandableError::NotAuthorizedGovernor,
       "You don't own ship #42."},
      {"foreign dead ship fails NotOwner without leaking ship name", 2, 1,
       false, false, false, CommandableError::NotOwner,
       "You don't own ship #42."},
      {"unauthorized dead ship fails NotAuthorizedGovernor", 1, 3, false, false,
       false, CommandableError::NotAuthorizedGovernor,
       "You don't own ship #42."},
      {"owned dead ship fails ShipDead before ShipIrradiated", 1, 2, false,
       false, false, CommandableError::ShipDead, "has been destroyed."},
      {"owned irradiated ship fails ShipIrradiated", 1, 2, false, true, false,
       CommandableError::ShipIrradiated, "is irradiated 85% and inactive."},
      {"god mode bypasses ownership and governor authorization", 2, 3, true,
       true, true, std::nullopt, ""},
  }};

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  for (const auto& tc : cases) {
    Ship s{base_ship.to_struct()};
    s.alive() = tc.alive;
    s.active() = tc.active;
    if (!tc.active) {
      s.apply_radiation(85);
    }

    const auto res = validate_commandable(s, tc.player, tc.governor, tc.god);
    g.set_player(tc.player);
    g.set_governor(tc.governor);
    g.set_god(tc.god);
    g.out.str("");

    if (!tc.expected_error.has_value()) {
      test::expect_true(res.has_value());
      test::expect_true(g.check_commandable(s));
      test::expect_eq(g.out.str(), std::string{});
    } else {
      test::expect_false(res.has_value());
      test::expect_true(res.error() == *tc.expected_error);
      test::expect_false(g.check_commandable(s));
      test::expect_contains(g.out.str(), tc.expected_output);
    }
  }
}

}  // namespace

int main() {
  test_deduct_ap_star();
  test_deduct_univ_ap();
  test_validate_commandable();

  std::println(std::cout, "✓ gameobj_ap_test passed!");
  return 0;
}
