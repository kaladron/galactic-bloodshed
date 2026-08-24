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
std::string format_or_fallback(const T& val) {
  if constexpr (std::formattable<T, char>) {
    return std::format("{}", val);
  } else if constexpr (std::is_enum_v<T>) {
    return std::format("{}", static_cast<std::underlying_type_t<T>>(val));
  } else if constexpr (requires(std::ostream& os) { os << val; }) {
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

/// Recorded notification structure for test assertion
export struct SentNotification {
  player_t player{0};
  governor_t governor{0};
  std::string message;
  bool is_broadcast{false};
};

/// Recording implementation of SessionRegistry for verifying async
/// notifications, broadcasts, and session queries in unit tests.
export class RecordingSessionRegistry : public NullSessionRegistry {
public:
  std::vector<SessionInfo> sessions;
  std::vector<SentNotification> notifications;
  bool update_in_progress_flag{false};

  [[nodiscard]] std::vector<SessionInfo>
  get_connected_sessions() const override {
    return sessions;
  }

  [[nodiscard]] bool is_connected(player_t player,
                                  governor_t gov) const override {
    return std::ranges::any_of(sessions, [&](const auto& s) {
      return s.player == player && s.governor == gov && s.connected;
    });
  }

  void notify_race(player_t race, const std::string& message) override {
    notifications.push_back({
        .player = race,
        .governor = 0,
        .message = message,
        .is_broadcast = true,
    });
  }

  bool notify_player(player_t race, governor_t gov,
                     const std::string& message) override {
    notifications.push_back({
        .player = race,
        .governor = gov,
        .message = message,
        .is_broadcast = false,
    });
    return true;
  }

  [[nodiscard]] bool update_in_progress() const override {
    return update_in_progress_flag;
  }

  void set_update_in_progress(bool val) override {
    update_in_progress_flag = val;
  }

  /// Check if a notification containing needle was sent to a specific player.
  [[nodiscard]] bool has_received(player_t player,
                                  std::string_view needle) const {
    return std::ranges::any_of(notifications, [&](const auto& n) {
      return n.player == player && n.message.contains(needle);
    });
  }

  /// Check if any broadcast notification containing needle was sent.
  [[nodiscard]] bool has_broadcast(std::string_view needle) const {
    return std::ranges::any_of(notifications, [&](const auto& n) {
      return n.is_broadcast && n.message.contains(needle);
    });
  }

  /// Retrieve all messages sent to a specific player.
  [[nodiscard]] std::vector<std::string> messages_for(player_t player) const {
    std::vector<std::string> msgs;
    for (const auto& n : notifications) {
      if (n.player == player) {
        msgs.push_back(n.message);
      }
    }
    return msgs;
  }

  void clear_notifications() {
    notifications.clear();
  }
};

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
      }
    }

    if (has_univ && desc.ap.model == GB::commands::APModel::FixedUniv) {
      try {
        if (const auto* univ = em.peek_universe()) {
          test::expect_eq(univ->AP[g.player().value - 1], initial_univ_ap,
                          "Rejected command must not deduct universe AP");
        }
      } catch (const EntityNotFoundError&) {
      }
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

/// Helper runner to execute standardized 4-way command matrix tests:
/// 1. Insufficient AP rejection (and 0 AP deducted)
/// 2. Scope rejection across invalid scope levels (and 0 AP deducted)
/// 3. Guest role rejection (and 0 AP deducted)
/// 4. Domain error rejection (and 0 AP deducted)
/// 5. Happy path execution (with exact AP deduction verified)
export class TestCommandMatrix {
public:
  TestCommandMatrix(TestContext& ctx,
                    const GB::commands::CommandDescriptor& desc)
      : ctx_(ctx), desc_(desc) {}

  TestCommandMatrix(TestContext& ctx, std::string_view cmd_name)
      : ctx_(ctx), desc_(*GB::commands::find_command_descriptor(cmd_name)) {}

  TestCommandMatrix& with_valid_argv(command_t argv) {
    valid_argv_ = std::move(argv);
    return *this;
  }

  TestCommandMatrix& with_invalid_argv(command_t argv) {
    invalid_argv_ = std::move(argv);
    return *this;
  }

  TestCommandMatrix& with_valid_scope(ScopeLevel scope) {
    valid_scope_ = scope;
    return *this;
  }

  TestCommandMatrix& with_invalid_scopes(std::vector<ScopeLevel> scopes) {
    invalid_scopes_ = std::move(scopes);
    return *this;
  }

  TestCommandMatrix& with_expected_star_ap(ap_t ap) {
    expected_star_ap_ = ap;
    return *this;
  }

  TestCommandMatrix& with_expected_univ_ap(ap_t ap) {
    expected_univ_ap_ = ap;
    return *this;
  }

  /// Run Happy Path: executes valid_argv in valid_scope and asserts exact AP
  /// deduction.
  void run_happy_path(GameObj& g) const {
    g.set_level(valid_scope_);
    ctx_.assert_dispatch_success(g, desc_, valid_argv_, expected_star_ap_,
                                 expected_univ_ap_);
  }

  /// Run Insufficient AP rejection: sets star/univ AP to 0, asserts rejection +
  /// 0 AP deduction, then restores AP.
  void run_insufficient_ap_check(GameObj& g) const {
    if (expected_star_ap_ == 0 && expected_univ_ap_ == 0) return;

    g.set_level(valid_scope_);
    starnum_t snum = g.snum();
    ap_t orig_star_ap = 0;
    if (expected_star_ap_ > 0) {
      try {
        {
          auto star_handle = ctx_.em.get_star(snum);
          orig_star_ap = star_handle->AP(g.player());
          star_handle->AP(g.player()) = 0;
        }
      } catch (const EntityNotFoundError&) {
      }
    }

    ap_t orig_univ_ap = 0;
    if (g.player().value > 0 && g.player().value <= MAXPLAYERS &&
        expected_univ_ap_ > 0) {
      {
        auto univ_handle = ctx_.em.get_universe();
        orig_univ_ap = univ_handle->AP[g.player().value - 1];
        univ_handle->AP[g.player().value - 1] = 0;
      }
    }

    ctx_.assert_dispatch_rejected(g, desc_, valid_argv_);

    if (expected_star_ap_ > 0) {
      try {
        {
          auto star_handle = ctx_.em.get_star(snum);
          star_handle->AP(g.player()) = orig_star_ap;
        }
      } catch (const EntityNotFoundError&) {
      }
    }
    if (g.player().value > 0 && g.player().value <= MAXPLAYERS &&
        expected_univ_ap_ > 0) {
      {
        auto univ_handle = ctx_.em.get_universe();
        univ_handle->AP[g.player().value - 1] = orig_univ_ap;
      }
    }
  }

  /// Run Invalid Scopes rejection: tests each invalid scope level, asserting
  /// rejection + 0 AP deduction.
  void run_scope_checks(GameObj& g) const {
    for (ScopeLevel scope : invalid_scopes_) {
      g.set_level(scope);
      ctx_.assert_dispatch_rejected(g, desc_, valid_argv_);
    }
    g.set_level(valid_scope_);
  }

  /// Run Guest Rejection check: marks race as guest or tests guest player,
  /// asserting rejection + 0 AP deduction.
  void run_guest_check(GameObj& g) const {
    if (!desc_.roles.no_guests) return;

    player_t orig_player = g.player();
    governor_t orig_gov = g.governor();
    ScopeLevel orig_scope = g.level();

    if (orig_player > 0) {
      try {
        bool orig_guest = false;
        {
          auto race_handle = ctx_.em.get_race(orig_player);
          orig_guest = race_handle->Guest;
          race_handle->Guest = true;
        }
        ctx_.setup_game_obj(g, orig_player, orig_gov);

        g.set_level(valid_scope_);
        ctx_.assert_dispatch_rejected(g, desc_, valid_argv_);

        {
          auto race_handle = ctx_.em.get_race(orig_player);
          race_handle->Guest = orig_guest;
        }
        ctx_.setup_game_obj(g, orig_player, orig_gov);
      } catch (const EntityNotFoundError&) {
      }
    }

    ctx_.setup_game_obj(g, orig_player, orig_gov);
    g.set_level(orig_scope);
  }

  /// Run Domain Error check: executes invalid_argv and asserts rejection + 0 AP
  /// deduction.
  void run_domain_error_check(GameObj& g) const {
    if (invalid_argv_.empty()) return;
    g.set_level(valid_scope_);
    ctx_.assert_dispatch_rejected(g, desc_, invalid_argv_);
  }

  /// Run standard 4-way command matrix tests in sequence.
  void run_matrix(GameObj& g) const {
    run_insufficient_ap_check(g);
    run_scope_checks(g);
    run_guest_check(g);
    run_domain_error_check(g);
    run_happy_path(g);
  }

private:
  TestContext& ctx_;
  const GB::commands::CommandDescriptor& desc_;
  command_t valid_argv_;
  command_t invalid_argv_;
  ScopeLevel valid_scope_{ScopeLevel::LEVEL_UNIV};
  std::vector<ScopeLevel> invalid_scopes_;
  ap_t expected_star_ap_{0};
  ap_t expected_univ_ap_{0};
};

/// Fluent builder for constructing consistent test ship entities populated
/// with canonical template defaults from Shipdata[type] and Shipnames[type].
export class TestShipBuilder {
public:
  TestShipBuilder(EntityManager& em, ShipType type = ShipType::STYPE_BATTLE,
                  std::optional<shipnum_t> explicit_number = std::nullopt)
      : em_(em) {
    shipnum_t number = explicit_number.value_or(shipnum_t{
        static_cast<shipnum_t::value_type>(em.num_ships().value + 1)});
    ship_.number = number;
    ship_.type = type;
    ship_.build_type = type;
    ship_.alive = true;
    ship_.active = true;
    ship_.on = true;
    ship_.owner = 1;
    ship_.governor = 0;
    ship_.tech = 100.0;
    ship_.name = Shipnames[type];

    // Canonical baseline initialization from Shipdata
    ship_.armor = static_cast<unsigned char>(Shipdata[type][ABIL_ARMOR]);
    ship_.max_crew = static_cast<unsigned short>(Shipdata[type][ABIL_MAXCREW]);
    ship_.max_resource = static_cast<resource_t>(Shipdata[type][ABIL_CARGO]);
    ship_.max_destruct =
        static_cast<unsigned short>(Shipdata[type][ABIL_DESTCAP]);
    ship_.max_fuel = static_cast<unsigned short>(Shipdata[type][ABIL_FUELCAP]);
    ship_.max_speed = static_cast<unsigned short>(Shipdata[type][ABIL_SPEED]);
    ship_.build_cost = static_cast<unsigned short>(Shipdata[type][ABIL_COST]);
    ship_.fuel = static_cast<double>(ship_.max_fuel);
    ship_.destruct = static_cast<unsigned short>(ship_.max_destruct);
    ship_.hanger = 0;
    ship_.max_hanger = static_cast<unsigned short>(Shipdata[type][ABIL_HANGER]);
    ship_.primtype = static_cast<guntype_t>(Shipdata[type][ABIL_PRIMARY]);
    ship_.sectype = static_cast<guntype_t>(Shipdata[type][ABIL_SECONDARY]);
    ship_.guns = static_cast<unsigned char>(Shipdata[type][ABIL_GUNS]);
    ship_.primary = static_cast<unsigned long>(Shipdata[type][ABIL_GUNS]);

    // Calculate baseline size and mass using canonical ship functions
    Ship temp_ship{ship_};
    ship_.size = static_cast<unsigned short>(ship_size(temp_ship));
    ship_.base_mass = getmass(temp_ship);
    ship_.mass = ship_.base_mass;
  }

  TestShipBuilder& owned_by(player_t owner, governor_t gov = 0) {
    ship_.owner = owner;
    ship_.governor = gov;
    return *this;
  }

  TestShipBuilder& named(std::string_view name) {
    ship_.name = name;
    return *this;
  }

  TestShipBuilder& with_tech(double tech) {
    ship_.tech = tech;
    return *this;
  }

  TestShipBuilder& with_alive(bool alive) {
    ship_.alive = alive;
    return *this;
  }

  TestShipBuilder& with_active(bool active) {
    ship_.active = active;
    return *this;
  }

  TestShipBuilder& in_star_orbit(starnum_t snum, double x = 0.0,
                                 double y = 0.0) {
    ship_.whatorbits = ScopeLevel::LEVEL_STAR;
    ship_.storbits = snum;
    ship_.pnumorbits = 0;
    ship_.xpos = x;
    ship_.ypos = y;
    ship_.docked = 0;
    return *this;
  }

  TestShipBuilder& in_planet_orbit(starnum_t snum, planetnum_t pnum,
                                   double x = 0.0, double y = 0.0) {
    ship_.whatorbits = ScopeLevel::LEVEL_PLAN;
    ship_.storbits = snum;
    ship_.pnumorbits = pnum;
    ship_.xpos = x;
    ship_.ypos = y;
    ship_.docked = 0;
    return *this;
  }

  TestShipBuilder& landed_on(starnum_t snum, planetnum_t pnum,
                             Coordinates coords) {
    ship_.whatorbits = ScopeLevel::LEVEL_PLAN;
    ship_.whatdest = ScopeLevel::LEVEL_PLAN;
    ship_.storbits = snum;
    ship_.pnumorbits = pnum;
    ship_.docked = 1;
    ship_.land_coords = coords;
    return *this;
  }

  TestShipBuilder& docked_to(shipnum_t dest_ship, starnum_t snum) {
    ship_.whatorbits = ScopeLevel::LEVEL_SHIP;
    ship_.destshipno = dest_ship;
    ship_.storbits = snum;
    ship_.docked = 1;
    return *this;
  }

  TestShipBuilder& with_guns(guntype_t primtype, unsigned long count,
                             unsigned char guns_flag = PRIMARY) {
    ship_.guns = guns_flag;
    ship_.primtype = primtype;
    ship_.primary = count;
    return *this;
  }

  TestShipBuilder& with_cew(unsigned short cew_power,
                            unsigned short range = 1000) {
    ship_.cew = static_cast<unsigned char>(cew_power);
    ship_.cew_range = range;
    ship_.mounted = 1;
    return *this;
  }

  TestShipBuilder& with_crew(population_t civilians, population_t military) {
    ship_.popn = civilians;
    ship_.troops = military;
    ship_.mass = ship_.base_mass + (civilians + military);
    return *this;
  }

  TestShipBuilder& with_speed(unsigned short speed) {
    ship_.speed = speed;
    return *this;
  }

  TestShipBuilder& with_fuel(double fuel) {
    ship_.fuel = fuel;
    return *this;
  }

  TestShipBuilder& with_resource(resource_t res) {
    ship_.resource = res;
    return *this;
  }

  TestShipBuilder& with_destruct(unsigned short destruct) {
    ship_.destruct = destruct;
    return *this;
  }

  TestShipBuilder& with_damage(unsigned short damage) {
    ship_.damage = static_cast<unsigned char>(damage);
    return *this;
  }

  TestShipBuilder& with_armor(unsigned char armor) {
    ship_.armor = armor;
    return *this;
  }

  shipnum_t build() {
    auto handle = em_.create_ship(ship_);
    return handle->number();
  }

private:
  EntityManager& em_;
  ship_struct ship_{};
};

/// Fluent fixture builder for configuring test universes, races, stars, and
/// planets with auto-assigned IDs and semantic exploration bitmask
/// initialization.
export class TestWorldBuilder {
public:
  explicit TestWorldBuilder(TestContext& ctx) : store_(ctx.db) {}
  explicit TestWorldBuilder(Database& db) : store_(db) {}

  /// Add a race to the test world.
  /// If explicit_id is std::nullopt, auto-assigns the next player ID (1, 2,
  /// ...).
  TestWorldBuilder&
  add_race(std::string_view name = "Federation", double tech = 100.0,
           bool guest = false,
           std::optional<player_t> explicit_id = std::nullopt) {
    player_t id = explicit_id.value_or(
        player_t{static_cast<player_t::value_type>(next_player_id_++)});
    Race race{};
    race.Playernum = id;
    race.name = name;
    race.tech = tech;
    race.Guest = guest;
    race.governor[0].active = true;
    race.governor[0].money = 10'000;
    race.mass = 1.0;
    race.metabolism = 1.0;
    RaceRepository(store_).save(race);
    registered_races_.push_back(id);
    return *this;
  }

  /// Add a star to the test world.
  /// If explicit_snum is std::nullopt, auto-assigns the next star ID (0, 1,
  /// ...). Automatically marks explored by all races added to this builder.
  TestWorldBuilder&
  add_star(std::string_view name = "Sol", ap_t initial_ap = 100,
           std::optional<starnum_t> explicit_snum = std::nullopt) {
    starnum_t snum = explicit_snum.value_or(
        starnum_t{static_cast<starnum_t::value_type>(next_star_id_++)});
    star_struct ss{};
    ss.star_id = snum;
    ss.name = name;
    for (int i = 0; i < MAXPLAYERS; ++i) {
      ss.AP[i] = initial_ap;
    }
    Star star{ss};
    for (player_t pid : registered_races_) {
      setbit(star.explored(), pid);
      setbit(star.inhabited(), pid);
    }
    StarRepository(store_).save(star);
    registered_stars_.push_back(snum);

    UniverseRepository univ_repo(store_);
    auto u = univ_repo.find(1);
    if (u) {
      if (snum.value + 1 > u->numstars) {
        u->numstars = snum.value + 1;
        univ_repo.save(*u);
      }
    }
    return *this;
  }

  /// Add a planet to a star.
  /// If explicit_pnum is std::nullopt, auto-assigns the next planet order for
  /// this star. Automatically initializes an empty SectorMap and marks explored
  /// by registered races.
  TestWorldBuilder&
  add_planet(starnum_t snum = 0, PlanetType type = PlanetType::EARTH,
             unsigned char maxx = 10, unsigned char maxy = 10,
             std::optional<planetnum_t> explicit_pnum = std::nullopt) {
    planetnum_t pnum{0};
    if (explicit_pnum) {
      pnum = *explicit_pnum;
    } else {
      StarRepository stars(store_);
      auto star_opt = stars.find(snum);
      pnum = planetnum_t{static_cast<planetnum_t::value_type>(
          star_opt ? star_opt->numplanets() : 0)};
    }
    Planet p(type);
    p.star_id() = snum;
    p.planet_order() = pnum;
    p.Maxx() = maxx;
    p.Maxy() = maxy;
    p.explored() = true;
    for (player_t pid : registered_races_) {
      p.info(pid).explored = 1;
      p.info(pid).destruct = 1000;
      p.info(pid).fuel = 1000;
      p.info(pid).resource = 1000;
    }
    PlanetRepository(store_).save(p);

    // Keep star planet names synchronized
    StarRepository stars(store_);
    auto star_opt = stars.find(snum);
    if (star_opt) {
      star_opt->set_planet_name(pnum, std::format("Planet-{}", pnum.value));
      stars.save(*star_opt);
    }

    // Save initial SectorMap with coordinate indexing
    SectorMap smap(p, true);
    for (int y = 0; y < maxy; ++y) {
      for (int x = 0; x < maxx; ++x) {
        smap.get(x, y).set_x(x);
        smap.get(x, y).set_y(y);
      }
    }
    SectorRepository(store_).save_map(smap);
    return *this;
  }

  /// Preset for standard 2-player solar system setup
  static void create_standard_solar_system(TestContext& ctx) {
    TestWorldBuilder(ctx)
        .add_race("Federation", 100.0)
        .add_race("Klingons", 100.0)
        .add_star("Sol", 100)
        .add_planet(0, PlanetType::EARTH);
  }

private:
  JsonStore store_;
  int next_player_id_{1};
  int next_star_id_{0};
  std::vector<player_t> registered_races_;
  std::vector<starnum_t> registered_stars_;
};
