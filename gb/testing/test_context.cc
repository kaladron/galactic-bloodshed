// SPDX-License-Identifier: Apache-2.0

/// \file test_context.cc
/// \brief Implementation of TestContext fixture and dispatch assertion helpers.

module;

#include <cassert>

module test;

import commands;
import dallib;
import gblib;
import std;

TestContext::TestContext() : db(":memory:"), em(db) {
  initialize_schema(db);
  universe_struct u{};
  u.id = 1;
  JsonStore store(db);
  UniverseRepository universe_repo(store);
  universe_repo.save(u);

  Race default_race{};
  default_race.Playernum = 1;
  default_race.name = "TestRace";
  default_race.governor[0].active = true;
  RaceRepository race_repo(store);
  race_repo.save(default_race);
}

void TestContext::setup_game_obj(GameObj& g, player_t player, governor_t gov) {
  g.set_player(player);
  g.set_governor(gov);
  if (player > 0) {
    g.race = em.peek_race(player);
  } else {
    g.race = nullptr;
  }
}

bool TestContext::dispatch(GameObj& g,
                           const GB::commands::CommandDescriptor& desc,
                           const command_t& argv) {
  g.out.str("");
  return GB::commands::dispatch_command(g, desc, argv);
}

bool TestContext::dispatch(GameObj& g, const command_t& argv) {
  if (argv.empty()) return false;
  const auto* desc = GB::commands::find_command_descriptor(argv[0]);
  if (!desc) return false;
  return dispatch(g, *desc, argv);
}

void TestContext::assert_dispatch_success(
    GameObj& g, const GB::commands::CommandDescriptor& desc,
    const command_t& argv, ap_t expected_star_ap_deducted,
    ap_t expected_univ_ap_deducted) {
  ap_t initial_star_ap = 0;
  starnum_t snum = g.snum();
  bool has_star = false;
  try {
    if (const auto* star = em.peek_star(snum)) {
      initial_star_ap = star->AP(g.player());
      has_star = true;
    }
  } catch (const EntityNotFoundError&) {
  }

  ap_t initial_univ_ap = 0;
  try {
    if (const auto* univ = em.peek_universe()) {
      if (g.player().value > 0 && g.player().value <= MAXPLAYERS) {
        initial_univ_ap = univ->AP[g.player().value - 1];
      }
    }
  } catch (const EntityNotFoundError&) {
    initial_univ_ap = 0;
  }

  bool ok = dispatch(g, desc, argv);
  test::expect_true(
      ok, std::format("Expected command dispatch to succeed, output was: {}",
                      g.out.str()));

  if (expected_star_ap_deducted > 0 && has_star) {
    ap_t final_star_ap = em.peek_star(snum)->AP(g.player());
    test::expect_eq(final_star_ap, initial_star_ap - expected_star_ap_deducted,
                    "Star AP deduction mismatch");
  }

  if (expected_univ_ap_deducted > 0) {
    ap_t final_univ_ap = em.peek_universe()->AP[g.player().value - 1];
    test::expect_eq(final_univ_ap, initial_univ_ap - expected_univ_ap_deducted,
                    "Universe AP deduction mismatch");
  }
}

void TestContext::assert_dispatch_success(GameObj& g, const command_t& argv,
                                          ap_t expected_star_ap_deducted,
                                          ap_t expected_univ_ap_deducted) {
  test::expect_false(argv.empty(), "argv must not be empty");
  const auto* desc = GB::commands::find_command_descriptor(argv[0]);
  test::expect_true(desc != nullptr,
                    "Command descriptor must exist for dispatch");
  assert_dispatch_success(g, *desc, argv, expected_star_ap_deducted,
                          expected_univ_ap_deducted);
}

void TestContext::assert_dispatch_rejected(
    GameObj& g, const GB::commands::CommandDescriptor& desc,
    const command_t& argv) {
  ap_t initial_star_ap = 0;
  starnum_t snum = g.snum();
  bool has_star = false;
  try {
    if (const auto* star = em.peek_star(snum)) {
      initial_star_ap = star->AP(g.player());
      has_star = true;
    }
  } catch (const EntityNotFoundError&) {
  }

  ap_t initial_univ_ap = 0;
  bool has_univ = false;
  try {
    if (const auto* univ = em.peek_universe()) {
      if (g.player().value > 0 && g.player().value <= MAXPLAYERS) {
        initial_univ_ap = univ->AP[g.player().value - 1];
        has_univ = true;
      }
    }
  } catch (const EntityNotFoundError&) {
  }

  bool ok = dispatch(g, desc, argv);
  test::expect_false(
      ok,
      std::format("Expected command dispatch to be rejected, output was: {}",
                  g.out.str()));

  if (has_star && desc.ap.model == GB::commands::APModel::FixedStar) {
    try {
      if (const auto* star = em.peek_star(snum)) {
        test::expect_eq(star->AP(g.player()), initial_star_ap,
                        "Rejected command must not deduct star AP");
      }
    } catch (const EntityNotFoundError&) {
      (void)0;
    }
  }

  if (has_univ && desc.ap.model == GB::commands::APModel::FixedUniv) {
    try {
      if (const auto* univ = em.peek_universe()) {
        test::expect_eq(univ->AP[g.player().value - 1], initial_univ_ap,
                        "Rejected command must not deduct universe AP");
      }
    } catch (const EntityNotFoundError&) {
      (void)0;
    }
  }
}

void TestContext::assert_dispatch_rejected(GameObj& g, const command_t& argv) {
  test::expect_false(argv.empty(), "argv must not be empty");
  const auto* desc = GB::commands::find_command_descriptor(argv[0]);
  test::expect_true(desc != nullptr,
                    "Command descriptor must exist for dispatch");
  assert_dispatch_rejected(g, *desc, argv);
}

void TestContext::verify_universe_invariants(std::source_location loc) {
  test::verify_universe_invariants(em, loc);
}
