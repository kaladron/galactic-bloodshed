// SPDX-License-Identifier: Apache-2.0

/// \file test.cppm
/// \brief Test utilities for command tests (standalone module - not part of
/// gblib)
///
/// Provides TestContext helper to reduce boilerplate in test files.
/// This is a standalone module to avoid linking test utilities into production
/// binaries.

module;

#include <cassert>

export module test;

import commands;
import dallib; // For Database, initialize_schema
import gblib;  // For SessionRegistry, types, EntityManager
import std;

// Get singleton test registry
// Uses NullSessionRegistry from gblib - a no-op registry for tests
export inline SessionRegistry& get_test_session_registry() {
  return get_null_session_registry();
}

/// Test context providing database, entity manager, GameObj setup, and dispatch
/// assertion helpers
///
/// Usage pattern:
/// ```cpp
/// TestContext ctx;
/// auto& registry = get_test_session_registry();
/// GameObj g(ctx.em, registry);
/// ctx.setup_game_obj(g);
/// ctx.assert_dispatch_success(g, some_cmd, {"some", "arg"}, 1);
/// ```
export class TestContext {
public:
  Database db;
  EntityManager em;

  TestContext() : db(":memory:"), em(db) {
    initialize_schema(db);
  }

  /// Setup a GameObj for testing.
  /// Automatically sets up player, governor, and race pointer.
  /// If the race for the player does not exist yet, g.race remains null.
  void setup_game_obj(GameObj& g, player_t player = 1, governor_t gov = 0) {
    g.set_player(player);
    g.set_governor(gov);
    if (player > 0) {
      try {
        g.race = em.peek_race(player);
      } catch (const EntityNotFoundError&) {
        // Race not yet created in test - g.race remains null
      }
    }
  }

  /// Dispatch a command using an explicit CommandDescriptor.
  bool dispatch(GameObj& g, const GB::commands::CommandDescriptor& desc,
                const command_t& argv) {
    return GB::commands::dispatch_command(g, desc, argv);
  }

  /// Dispatch a command by resolving its name from the command registry.
  bool dispatch(GameObj& g, const command_t& argv) {
    if (argv.empty()) return false;
    const auto* desc = GB::commands::find_command_descriptor(argv[0]);
    if (!desc) return false;
    return GB::commands::dispatch_command(g, *desc, argv);
  }

  /// Helper to assert successful dispatch and verify expected AP deductions.
  void assert_dispatch_success(GameObj& g,
                               const GB::commands::CommandDescriptor& desc,
                               const command_t& argv,
                               ap_t expected_star_ap_deducted = 0,
                               ap_t expected_univ_ap_deducted = 0) {
    ap_t initial_star_ap = 0;
    starnum_t snum = g.snum();
    if (snum > 0) {
      try {
        if (const auto* star = em.peek_star(snum)) {
          initial_star_ap = star->AP(g.player());
        }
      } catch (const EntityNotFoundError&) {
      }
    }

    ap_t initial_univ_ap = 0;
    try {
      if (const auto* univ = em.peek_universe()) {
        if (g.player().value > 0 && g.player().value <= MAXPLAYERS) {
          initial_univ_ap = univ->AP[g.player().value - 1];
        }
      }
    } catch (const EntityNotFoundError&) {
    }

    bool ok = dispatch(g, desc, argv);
    assert(ok && "Expected command dispatch to succeed");

    if (expected_star_ap_deducted > 0 && snum > 0) {
      ap_t final_star_ap = em.peek_star(snum)->AP(g.player());
      assert(final_star_ap == initial_star_ap - expected_star_ap_deducted &&
             "Star AP deduction mismatch");
    }

    if (expected_univ_ap_deducted > 0) {
      ap_t final_univ_ap = em.peek_universe()->AP[g.player().value - 1];
      assert(final_univ_ap == initial_univ_ap - expected_univ_ap_deducted &&
             "Universe AP deduction mismatch");
    }
  }

  /// Helper to assert successful dispatch for registered commands.
  void assert_dispatch_success(GameObj& g, const command_t& argv,
                               ap_t expected_star_ap_deducted = 0,
                               ap_t expected_univ_ap_deducted = 0) {
    assert(!argv.empty() && "argv must not be empty");
    const auto* desc = GB::commands::find_command_descriptor(argv[0]);
    assert(desc != nullptr && "Command descriptor must exist for dispatch");
    assert_dispatch_success(g, *desc, argv, expected_star_ap_deducted,
                            expected_univ_ap_deducted);
  }

  /// Helper to assert rejected dispatch and verify 0 AP was deducted.
  void assert_dispatch_rejected(GameObj& g,
                                const GB::commands::CommandDescriptor& desc,
                                const command_t& argv) {
    ap_t initial_star_ap = 0;
    starnum_t snum = g.snum();
    if (snum > 0) {
      try {
        if (const auto* star = em.peek_star(snum)) {
          initial_star_ap = star->AP(g.player());
        }
      } catch (const EntityNotFoundError&) {
      }
    }

    ap_t initial_univ_ap = 0;
    try {
      if (const auto* univ = em.peek_universe()) {
        if (g.player().value > 0 && g.player().value <= MAXPLAYERS) {
          initial_univ_ap = univ->AP[g.player().value - 1];
        }
      }
    } catch (const EntityNotFoundError&) {
    }

    bool ok = dispatch(g, desc, argv);
    assert(!ok && "Expected command dispatch to be rejected");

    if (snum > 0) {
      try {
        if (const auto* star = em.peek_star(snum)) {
          assert(star->AP(g.player()) == initial_star_ap &&
                 "Rejected command must not deduct star AP");
        }
      } catch (const EntityNotFoundError&) {
      }
    }

    try {
      if (const auto* univ = em.peek_universe()) {
        if (g.player().value > 0 && g.player().value <= MAXPLAYERS) {
          assert(univ->AP[g.player().value - 1] == initial_univ_ap &&
                 "Rejected command must not deduct universe AP");
        }
      }
    } catch (const EntityNotFoundError&) {
    }
  }

  /// Helper to assert rejected dispatch for registered commands.
  void assert_dispatch_rejected(GameObj& g, const command_t& argv) {
    assert(!argv.empty() && "argv must not be empty");
    const auto* desc = GB::commands::find_command_descriptor(argv[0]);
    assert(desc != nullptr && "Command descriptor must exist for dispatch");
    assert_dispatch_rejected(g, *desc, argv);
  }
};
