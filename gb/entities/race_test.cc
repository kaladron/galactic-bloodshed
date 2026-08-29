// SPDX-License-Identifier: Apache-2.0

/// \file race_test.cc
/// \brief Unit tests for Race and block entity member functions and diplomatic
/// bitmaps.

import gb.entities;
import test;
import std;

int main() {
  std::println(std::cout, "Running Race entity unit tests...\n");

  // Race alliance methods
  std::println(std::cout, "Race diplomatic alliance methods...");
  {
    Race race{};
    race.Playernum = 1;

    test::expect_false(race.is_allied_with(player_t{2}));
    test::expect_false(race.is_allied_with(player_t{3}));

    race.declare_alliance_with(player_t{2});
    test::expect_true(race.is_allied_with(player_t{2}));
    test::expect_false(race.is_allied_with(player_t{3}));

    race.declare_alliance_with(player_t{3});
    test::expect_true(race.is_allied_with(player_t{2}));
    test::expect_true(race.is_allied_with(player_t{3}));

    race.rescind_alliance_with(player_t{2});
    test::expect_false(race.is_allied_with(player_t{2}));
    test::expect_true(race.is_allied_with(player_t{3}));
    std::println(std::cout, "  ✓ Race alliance methods work as expected");
  }

  // Race war methods
  std::println(std::cout, "Race diplomatic war methods...");
  {
    Race race{};
    race.Playernum = 1;

    test::expect_false(race.is_at_war_with(player_t{2}));
    test::expect_false(race.is_at_war_with(player_t{4}));

    race.declare_war_on(player_t{2});
    test::expect_true(race.is_at_war_with(player_t{2}));
    test::expect_false(race.is_at_war_with(player_t{4}));

    race.declare_war_on(player_t{4});
    test::expect_true(race.is_at_war_with(player_t{2}));
    test::expect_true(race.is_at_war_with(player_t{4}));

    race.make_peace_with(player_t{2});
    test::expect_false(race.is_at_war_with(player_t{2}));
    test::expect_true(race.is_at_war_with(player_t{4}));
    std::println(std::cout, "  ✓ Race war methods work as expected");
  }

  // block invitation and pledge methods
  std::println(std::cout, "block invitation and pledge methods...");
  {
    block b{};
    b.Playernum = 1;
    b.name = "Federation";

    test::expect_false(b.is_invited(player_t{2}));
    test::expect_false(b.is_pledged(player_t{2}));

    b.invite_player(player_t{2});
    test::expect_true(b.is_invited(player_t{2}));
    test::expect_false(b.is_pledged(player_t{2}));

    b.pledge_player(player_t{2});
    test::expect_true(b.is_invited(player_t{2}));
    test::expect_true(b.is_pledged(player_t{2}));

    b.cancel_invite(player_t{2});
    test::expect_false(b.is_invited(player_t{2}));
    test::expect_true(b.is_pledged(player_t{2}));

    b.unpledge_player(player_t{2});
    test::expect_false(b.is_pledged(player_t{2}));
    std::println(std::cout,
                 "  ✓ block invitation and pledge methods work as expected");
  }

  // block alliance and war methods
  std::println(std::cout, "block alliance and war methods...");
  {
    block b{};
    b.Playernum = 1;

    test::expect_false(b.is_allied_with(player_t{3}));
    test::expect_false(b.is_at_war_with(player_t{3}));

    b.declare_alliance_with(player_t{3});
    test::expect_true(b.is_allied_with(player_t{3}));
    test::expect_false(b.is_at_war_with(player_t{3}));

    b.rescind_alliance_with(player_t{3});
    test::expect_false(b.is_allied_with(player_t{3}));

    b.declare_war_on(player_t{3});
    test::expect_true(b.is_at_war_with(player_t{3}));

    b.make_peace_with(player_t{3});
    test::expect_false(b.is_at_war_with(player_t{3}));
    std::println(std::cout,
                 "  ✓ block alliance and war methods work as expected");
  }

  // Race translate and points PlayerVector tests
  std::println(std::cout,
               "Race translate and points PlayerVector accessors...");
  {
    Race race{};
    race.Playernum = 1;
    race.translate[player_t{1}] = 100;
    race.translate[player_t{2}] = 75;
    race.points[player_t{2}] = 500;

    test::expect_eq(race.translate[player_t{1}], 100);
    test::expect_eq(race.translate[player_t{2}], 75);
    test::expect_eq(race.points[player_t{2}], 500u);

    // PlayerVector indexing directly by Race object
    Race race2{};
    race2.Playernum = 2;
    test::expect_eq(race.translate[race2], 75);
    test::expect_eq(race.points[race2], 500u);

    // Out-of-bounds throws std::out_of_range
    test::expect_throws<std::out_of_range>(
        [&]() { (void)race.translate[player_t{0}]; });
    test::expect_throws<std::out_of_range>(
        [&]() { (void)race.translate[player_t{MAXPLAYERS + 1}]; });
    test::expect_throws<std::out_of_range>(
        [&]() { (void)race.points[player_t{0}]; });
    test::expect_throws<std::out_of_range>(
        [&]() { (void)race.points[player_t{MAXPLAYERS + 1}]; });
    std::println(std::cout, "  ✓ Race PlayerVector accessors verified");
  }

  // Race adjust_morale domain method tests
  std::println(std::cout, "Race adjust_morale domain method...");
  {
    Race winner{};
    winner.Playernum = 1;
    winner.morale = 100;

    Race loser{};
    loser.Playernum = 2;
    loser.morale = 50;

    winner.adjust_morale(loser, 25);
    test::expect_eq(winner.morale, 125);
    test::expect_eq(loser.morale, 25);
    test::expect_eq(winner.points[loser], 25u);
    std::println(std::cout, "  ✓ Race::adjust_morale works as expected");
  }

  std::println(std::cout, "\n✓ All Race entity tests passed!");
  return 0;
}
