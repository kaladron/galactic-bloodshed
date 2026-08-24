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

export namespace test {

template <typename T>
concept Formattable = requires(T& v) { std::format("{}", v); };

template <typename T>
std::string format_or_fallback(const T& val) {
  if constexpr (Formattable<T>) {
    return std::format("{}", val);
  } else if constexpr (requires { std::declval<std::ostream&>() << val; }) {
    std::ostringstream oss;
    oss << val;
    return oss.str();
  } else {
    return "<unprintable object>";
  }
}

template <typename T, typename U>
void expect_eq(const T& actual, const U& expected, std::string_view msg = "",
               std::source_location loc = std::source_location::current()) {
  if (!(actual == expected)) {
    if (!msg.empty()) {
      std::println(std::cerr,
                   "\n❌ [ASSERTION FAILED] {}:{}\n"
                   "    Function: {}\n"
                   "    Expected: {}\n"
                   "    Actual:   {}\n"
                   "    Message:  {}",
                   loc.file_name(), loc.line(), loc.function_name(),
                   format_or_fallback(expected), format_or_fallback(actual),
                   msg);
    } else {
      std::println(std::cerr,
                   "\n❌ [ASSERTION FAILED] {}:{}\n"
                   "    Function: {}\n"
                   "    Expected: {}\n"
                   "    Actual:   {}",
                   loc.file_name(), loc.line(), loc.function_name(),
                   format_or_fallback(expected), format_or_fallback(actual));
    }
    // TODO: Print std::stacktrace::current() once compiler/runtime libc++
    // support is fully standardized across build targets.
    std::abort();
  }
}

template <typename T, typename U>
void expect_ne(const T& actual, const U& expected, std::string_view msg = "",
               std::source_location loc = std::source_location::current()) {
  if (actual == expected) {
    if (!msg.empty()) {
      std::println(std::cerr,
                   "\n❌ [ASSERTION FAILED] {}:{}\n"
                   "    Function: {}\n"
                   "    Expected value not equal to: {}\n"
                   "    Actual:                      {}\n"
                   "    Message:                     {}",
                   loc.file_name(), loc.line(), loc.function_name(),
                   format_or_fallback(expected), format_or_fallback(actual),
                   msg);
    } else {
      std::println(std::cerr,
                   "\n❌ [ASSERTION FAILED] {}:{}\n"
                   "    Function: {}\n"
                   "    Expected value not equal to: {}\n"
                   "    Actual:                      {}",
                   loc.file_name(), loc.line(), loc.function_name(),
                   format_or_fallback(expected), format_or_fallback(actual));
    }
    std::abort();
  }
}

template <typename T, typename U>
void expect_ge(const T& actual, const U& min_expected,
               std::string_view msg = "",
               std::source_location loc = std::source_location::current()) {
  if (actual < min_expected) {
    if (!msg.empty()) {
      std::println(std::cerr,
                   "\n❌ [ASSERTION FAILED] {}:{}\n"
                   "    Function: {}\n"
                   "    Expected >=: {}\n"
                   "    Actual:      {}\n"
                   "    Message:     {}",
                   loc.file_name(), loc.line(), loc.function_name(),
                   format_or_fallback(min_expected), format_or_fallback(actual),
                   msg);
    } else {
      std::println(std::cerr,
                   "\n❌ [ASSERTION FAILED] {}:{}\n"
                   "    Function: {}\n"
                   "    Expected >=: {}\n"
                   "    Actual:      {}",
                   loc.file_name(), loc.line(), loc.function_name(),
                   format_or_fallback(min_expected),
                   format_or_fallback(actual));
    }
    std::abort();
  }
}

template <typename T, typename U>
void expect_le(const T& actual, const U& max_expected,
               std::string_view msg = "",
               std::source_location loc = std::source_location::current()) {
  if (actual > max_expected) {
    if (!msg.empty()) {
      std::println(std::cerr,
                   "\n❌ [ASSERTION FAILED] {}:{}\n"
                   "    Function: {}\n"
                   "    Expected <=: {}\n"
                   "    Actual:      {}\n"
                   "    Message:     {}",
                   loc.file_name(), loc.line(), loc.function_name(),
                   format_or_fallback(max_expected), format_or_fallback(actual),
                   msg);
    } else {
      std::println(std::cerr,
                   "\n❌ [ASSERTION FAILED] {}:{}\n"
                   "    Function: {}\n"
                   "    Expected <=: {}\n"
                   "    Actual:      {}",
                   loc.file_name(), loc.line(), loc.function_name(),
                   format_or_fallback(max_expected),
                   format_or_fallback(actual));
    }
    std::abort();
  }
}

template <typename T, typename U>
void expect_gt(const T& actual, const U& min_expected,
               std::string_view msg = "",
               std::source_location loc = std::source_location::current()) {
  if (actual <= min_expected) {
    if (!msg.empty()) {
      std::println(std::cerr,
                   "\n❌ [ASSERTION FAILED] {}:{}\n"
                   "    Function: {}\n"
                   "    Expected >: {}\n"
                   "    Actual:     {}\n"
                   "    Message:    {}",
                   loc.file_name(), loc.line(), loc.function_name(),
                   format_or_fallback(min_expected), format_or_fallback(actual),
                   msg);
    } else {
      std::println(std::cerr,
                   "\n❌ [ASSERTION FAILED] {}:{}\n"
                   "    Function: {}\n"
                   "    Expected >: {}\n"
                   "    Actual:     {}",
                   loc.file_name(), loc.line(), loc.function_name(),
                   format_or_fallback(min_expected),
                   format_or_fallback(actual));
    }
    std::abort();
  }
}

template <typename T, typename U>
void expect_lt(const T& actual, const U& max_expected,
               std::string_view msg = "",
               std::source_location loc = std::source_location::current()) {
  if (actual >= max_expected) {
    if (!msg.empty()) {
      std::println(std::cerr,
                   "\n❌ [ASSERTION FAILED] {}:{}\n"
                   "    Function: {}\n"
                   "    Expected <: {}\n"
                   "    Actual:     {}\n"
                   "    Message:    {}",
                   loc.file_name(), loc.line(), loc.function_name(),
                   format_or_fallback(max_expected), format_or_fallback(actual),
                   msg);
    } else {
      std::println(std::cerr,
                   "\n❌ [ASSERTION FAILED] {}:{}\n"
                   "    Function: {}\n"
                   "    Expected <: {}\n"
                   "    Actual:     {}",
                   loc.file_name(), loc.line(), loc.function_name(),
                   format_or_fallback(max_expected),
                   format_or_fallback(actual));
    }
    std::abort();
  }
}

inline void
expect_true(bool condition, std::string_view msg = "",
            std::source_location loc = std::source_location::current()) {
  if (!condition) {
    if (!msg.empty()) {
      std::println(std::cerr,
                   "\n❌ [ASSERTION FAILED] {}:{}\n"
                   "    Function: {}\n"
                   "    Expected: true\n"
                   "    Actual:   false\n"
                   "    Message:  {}",
                   loc.file_name(), loc.line(), loc.function_name(), msg);
    } else {
      std::println(std::cerr,
                   "\n❌ [ASSERTION FAILED] {}:{}\n"
                   "    Function: {}\n"
                   "    Expected: true\n"
                   "    Actual:   false",
                   loc.file_name(), loc.line(), loc.function_name());
    }
    std::abort();
  }
}

inline void
expect_false(bool condition, std::string_view msg = "",
             std::source_location loc = std::source_location::current()) {
  if (condition) {
    if (!msg.empty()) {
      std::println(std::cerr,
                   "\n❌ [ASSERTION FAILED] {}:{}\n"
                   "    Function: {}\n"
                   "    Expected: false\n"
                   "    Actual:   true\n"
                   "    Message:  {}",
                   loc.file_name(), loc.line(), loc.function_name(), msg);
    } else {
      std::println(std::cerr,
                   "\n❌ [ASSERTION FAILED] {}:{}\n"
                   "    Function: {}\n"
                   "    Expected: false\n"
                   "    Actual:   true",
                   loc.file_name(), loc.line(), loc.function_name());
    }
    std::abort();
  }
}

inline void
expect_contains(std::string_view haystack, std::string_view needle,
                std::string_view msg = "",
                std::source_location loc = std::source_location::current()) {
  if (!haystack.contains(needle)) {
    if (!msg.empty()) {
      std::println(std::cerr,
                   "\n❌ [STRING MISMATCH] {}:{}\n"
                   "    Function: {}\n"
                   "    Expected to contain: \"{}\"\n"
                   "    Actual output was:\n---\n{}\n---\n"
                   "    Message: {}",
                   loc.file_name(), loc.line(), loc.function_name(), needle,
                   haystack, msg);
    } else {
      std::println(std::cerr,
                   "\n❌ [STRING MISMATCH] {}:{}\n"
                   "    Function: {}\n"
                   "    Expected to contain: \"{}\"\n"
                   "    Actual output was:\n---\n{}\n---",
                   loc.file_name(), loc.line(), loc.function_name(), needle,
                   haystack);
    }
    std::abort();
  }
}

template <typename ExceptionType, typename Func>
void expect_throws(Func&& fn, std::string_view msg = "",
                   std::source_location loc = std::source_location::current()) {
  try {
    fn();
    std::println(std::cerr,
                 "\n❌ [EXPECTED EXCEPTION NOT THROWN] {}:{}\n"
                 "    Function: {}\n"
                 "    Message:  {}",
                 loc.file_name(), loc.line(), loc.function_name(),
                 msg.empty() ? "(none)" : msg);
    std::abort();
  } catch (const ExceptionType&) {
    // Expected exception caught
  } catch (const std::exception& ex) {
    std::println(std::cerr,
                 "\n❌ [WRONG EXCEPTION TYPE] {}:{}\n"
                 "    Function: {}\n"
                 "    Caught std::exception: {}\n"
                 "    Message:  {}",
                 loc.file_name(), loc.line(), loc.function_name(), ex.what(),
                 msg.empty() ? "(none)" : msg);
    std::abort();
  } catch (...) {
    std::println(std::cerr,
                 "\n❌ [WRONG EXCEPTION TYPE] {}:{}\n"
                 "    Function: {}\n"
                 "    Caught non-std exception\n"
                 "    Message:  {}",
                 loc.file_name(), loc.line(), loc.function_name(),
                 msg.empty() ? "(none)" : msg);
    std::abort();
  }
}

template <typename Func>
void expect_no_throw(
    Func&& fn, std::string_view msg = "",
    std::source_location loc = std::source_location::current()) {
  try {
    fn();
  } catch (const std::exception& ex) {
    std::println(std::cerr,
                 "\n❌ [UNEXPECTED EXCEPTION] {}:{}\n"
                 "    Function: {}\n"
                 "    Caught:   {}\n"
                 "    Message:  {}",
                 loc.file_name(), loc.line(), loc.function_name(), ex.what(),
                 msg.empty() ? "(none)" : msg);
    std::abort();
  } catch (...) {
    std::println(std::cerr,
                 "\n❌ [UNEXPECTED NON-STD EXCEPTION] {}:{}\n"
                 "    Function: {}\n"
                 "    Message:  {}",
                 loc.file_name(), loc.line(), loc.function_name(),
                 msg.empty() ? "(none)" : msg);
    std::abort();
  }
}

}  // namespace test

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
    universe_struct u{};
    u.id = 1;
    JsonStore store(db);
    UniverseRepository universe_repo(store);
    universe_repo.save(u);
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
  /// Automatically clears g.out buffer before executing.
  bool dispatch(GameObj& g, const GB::commands::CommandDescriptor& desc,
                const command_t& argv) {
    g.out.str("");
    return GB::commands::dispatch_command(g, desc, argv);
  }

  /// Dispatch a command by resolving its name from the command registry.
  /// Automatically clears g.out buffer before executing.
  bool dispatch(GameObj& g, const command_t& argv) {
    if (argv.empty()) return false;
    const auto* desc = GB::commands::find_command_descriptor(argv[0]);
    if (!desc) return false;
    return dispatch(g, *desc, argv);
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
      initial_univ_ap = 0;
    }

    bool ok = dispatch(g, desc, argv);
    test::expect_true(ok, "Expected command dispatch to succeed");

    if (expected_star_ap_deducted > 0 && snum > 0) {
      ap_t final_star_ap = em.peek_star(snum)->AP(g.player());
      test::expect_eq(final_star_ap,
                      initial_star_ap - expected_star_ap_deducted,
                      "Star AP deduction mismatch");
    }

    if (expected_univ_ap_deducted > 0) {
      ap_t final_univ_ap = em.peek_universe()->AP[g.player().value - 1];
      test::expect_eq(final_univ_ap,
                      initial_univ_ap - expected_univ_ap_deducted,
                      "Universe AP deduction mismatch");
    }
  }

  /// Helper to assert successful dispatch for registered commands.
  void assert_dispatch_success(GameObj& g, const command_t& argv,
                               ap_t expected_star_ap_deducted = 0,
                               ap_t expected_univ_ap_deducted = 0) {
    test::expect_false(argv.empty(), "argv must not be empty");
    const auto* desc = GB::commands::find_command_descriptor(argv[0]);
    test::expect_true(desc != nullptr,
                      "Command descriptor must exist for dispatch");
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
      initial_univ_ap = 0;
    }

    bool ok = dispatch(g, desc, argv);
    test::expect_false(ok, "Expected command dispatch to be rejected");

    if (snum > 0) {
      try {
        if (const auto* star = em.peek_star(snum)) {
          test::expect_eq(star->AP(g.player()), initial_star_ap,
                          "Rejected command must not deduct star AP");
        }
      } catch (const EntityNotFoundError&) {
      }
    }

    try {
      if (const auto* univ = em.peek_universe()) {
        if (g.player().value > 0 && g.player().value <= MAXPLAYERS) {
          test::expect_eq(univ->AP[g.player().value - 1], initial_univ_ap,
                          "Rejected command must not deduct universe AP");
        }
      }
    } catch (const EntityNotFoundError&) {
    }
  }

  /// Helper to assert rejected dispatch for registered commands.
  void assert_dispatch_rejected(GameObj& g, const command_t& argv) {
    test::expect_false(argv.empty(), "argv must not be empty");
    const auto* desc = GB::commands::find_command_descriptor(argv[0]);
    test::expect_true(desc != nullptr,
                      "Command descriptor must exist for dispatch");
    assert_dispatch_rejected(g, *desc, argv);
  }
};
