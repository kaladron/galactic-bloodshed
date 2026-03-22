// SPDX-License-Identifier: Apache-2.0

/// \file gblib-test.cppm
/// \brief Test utilities for command tests (standalone module - not part of
/// gblib)
///
/// Provides TestContext helper to reduce boilerplate in test files.
/// This is a standalone module to avoid linking test utilities into production
/// binaries.

export module test;

import gblib;  // For SessionRegistry, types, EntityManager
import dallib; // For Database, initialize_schema
import std;

// Get singleton test registry
// Uses NullSessionRegistry from gblib - a no-op registry for tests
export inline SessionRegistry& get_test_session_registry() {
  return get_null_session_registry();
}

/// Test context providing database, entity manager, and GameObj factory
///
/// Usage pattern:
/// ```cpp
/// TestContext ctx;
/// auto& registry = get_test_session_registry();
/// GameObj g(ctx.em, registry);
/// ctx.setup_game_obj(g);
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
};
