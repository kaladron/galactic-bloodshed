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
  } else if constexpr (std::is_enum<T>::value) {
    return std::format("{}", static_cast<std::underlying_type_t<T>>(val));
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

/// Verifies cross-entity universe integrity invariants using range-based
/// entity collections (RaceList, StarList, PlanetList, SectorMap, ShipList,
/// CommodList). Aborts via test::expect_* if any domain invariant is violated.
void verify_universe_invariants(
    EntityManager& em,
    std::source_location loc = std::source_location::current());

}  // namespace test

// Get singleton test registry
// Uses NullSessionRegistry from gblib - a no-op registry for tests
export SessionRegistry& get_test_session_registry();

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
  get_connected_sessions() const override;

  [[nodiscard]] bool is_connected(player_t player,
                                  governor_t gov) const override;

  void notify_race(player_t race, const std::string& message) override;

  bool notify_player(player_t race, governor_t gov,
                     const std::string& message) override;

  [[nodiscard]] bool update_in_progress() const override;

  void set_update_in_progress(bool val) override;

  /// Check if a notification containing needle was sent to a specific player.
  [[nodiscard]] bool has_received(player_t player,
                                  std::string_view needle) const;

  /// Check if any broadcast notification containing needle was sent.
  [[nodiscard]] bool has_broadcast(std::string_view needle) const;

  /// Retrieve all messages sent to a specific player.
  [[nodiscard]] std::vector<std::string> messages_for(player_t player) const;

  void clear_notifications();
};

/// Test context providing database, entity manager, GameObj setup, and
/// dispatch assertion helpers
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

  TestContext();

  /// Setup a GameObj for testing.
  /// Automatically sets up player, governor, and race pointer.
  void setup_game_obj(GameObj& g, player_t player = 1, governor_t gov = 0);

  /// Dispatch a command using an explicit CommandDescriptor.
  /// Automatically clears g.out buffer before executing.
  bool dispatch(GameObj& g, const GB::commands::CommandDescriptor& desc,
                const command_t& argv);

  /// Dispatch a command by resolving its name from the command registry.
  /// Automatically clears g.out buffer before executing.
  bool dispatch(GameObj& g, const command_t& argv);

  /// Helper to assert successful dispatch and verify expected AP deductions.
  void assert_dispatch_success(GameObj& g,
                               const GB::commands::CommandDescriptor& desc,
                               const command_t& argv,
                               ap_t expected_star_ap_deducted = 0,
                               ap_t expected_univ_ap_deducted = 0);

  /// Helper to assert successful dispatch for registered commands.
  void assert_dispatch_success(GameObj& g, const command_t& argv,
                               ap_t expected_star_ap_deducted = 0,
                               ap_t expected_univ_ap_deducted = 0);

  /// Helper to assert rejected dispatch and verify 0 AP was deducted.
  void assert_dispatch_rejected(GameObj& g,
                                const GB::commands::CommandDescriptor& desc,
                                const command_t& argv);

  /// Helper to assert rejected dispatch for registered commands.
  void assert_dispatch_rejected(GameObj& g, const command_t& argv);

  /// Helper to verify universe domain invariants across all entities.
  void verify_universe_invariants(
      std::source_location loc = std::source_location::current());
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
                    const GB::commands::CommandDescriptor& desc);

  TestCommandMatrix(TestContext& ctx, std::string_view cmd_name);

  TestCommandMatrix& with_valid_argv(command_t argv);
  TestCommandMatrix& with_invalid_argv(command_t argv);
  TestCommandMatrix& with_valid_scope(ScopeLevel scope);
  TestCommandMatrix& with_invalid_scopes(std::vector<ScopeLevel> scopes);
  TestCommandMatrix& with_expected_star_ap(ap_t ap);
  TestCommandMatrix& with_expected_univ_ap(ap_t ap);

  /// Run Happy Path: executes valid_argv in valid_scope and asserts exact AP
  /// deduction.
  void run_happy_path(GameObj& g) const;

  /// Run Insufficient AP rejection: sets star/univ AP to 0, asserts rejection
  /// + 0 AP deduction, then restores AP.
  void run_insufficient_ap_check(GameObj& g) const;

  /// Run Invalid Scopes rejection: tests each invalid scope level, asserting
  /// rejection + 0 AP deduction.
  void run_scope_checks(GameObj& g) const;

  /// Run Guest Rejection check: marks race as guest or tests guest player,
  /// asserting rejection + 0 AP deduction.
  void run_guest_check(GameObj& g) const;

  /// Run Domain Error check: executes invalid_argv and asserts rejection + 0
  /// AP deduction.
  void run_domain_error_check(GameObj& g) const;

  /// Run standard 4-way command matrix tests in sequence.
  void run_matrix(GameObj& g) const;

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
                  std::optional<shipnum_t> explicit_number = std::nullopt);

  TestShipBuilder& owned_by(player_t owner, governor_t gov = 0);
  TestShipBuilder& named(std::string_view name);
  TestShipBuilder& with_tech(double tech);
  TestShipBuilder& with_alive(bool alive);
  TestShipBuilder& with_active(bool active);
  TestShipBuilder& in_star_orbit(starnum_t snum, double x = 0.0,
                                 double y = 0.0);
  TestShipBuilder& in_planet_orbit(starnum_t snum, planetnum_t pnum,
                                   double x = 0.0, double y = 0.0);
  TestShipBuilder& landed_on(starnum_t snum, planetnum_t pnum,
                             Coordinates coords);
  TestShipBuilder& docked_to(shipnum_t dest_ship, starnum_t snum);
  TestShipBuilder& with_guns(guntype_t primtype, unsigned long count,
                             unsigned char guns_flag = PRIMARY);
  TestShipBuilder& with_retaliate(unsigned char retaliate);
  TestShipBuilder& with_cew(unsigned short cew_power,
                            unsigned short range = 1000);
  TestShipBuilder& with_crew(population_t civilians, population_t military);
  TestShipBuilder& with_speed(unsigned short speed);
  TestShipBuilder& with_fuel(double fuel);
  TestShipBuilder& with_resource(resource_t res);
  TestShipBuilder& with_destruct(unsigned short destruct);
  TestShipBuilder& with_damage(unsigned short damage);
  TestShipBuilder& with_armor(unsigned char armor);

  shipnum_t build();

private:
  EntityManager& em_;
  ship_struct ship_{};
};

/// Fluent fixture builder for configuring test universes, races, stars, and
/// planets with auto-assigned IDs and semantic exploration bitmask
/// initialization.
export class TestWorldBuilder {
public:
  explicit TestWorldBuilder(TestContext& ctx);
  explicit TestWorldBuilder(Database& db);

  /// Add a race to the test world.
  /// If explicit_id is std::nullopt, auto-assigns the next player ID (1, 2,
  /// ...).
  TestWorldBuilder&
  add_race(std::string_view name = "Federation", double tech = 100.0,
           bool guest = false,
           std::optional<player_t> explicit_id = std::nullopt);

  /// Add a star to the test world.
  /// If explicit_snum is std::nullopt, auto-assigns the next star ID (0, 1,
  /// ...). Automatically marks explored by all races added to this builder.
  TestWorldBuilder&
  add_star(std::string_view name = "Sol", ap_t initial_ap = 100,
           std::optional<starnum_t> explicit_snum = std::nullopt);

  /// Add a planet to a star.
  /// If explicit_pnum is std::nullopt, auto-assigns the next planet order for
  /// this star. Automatically initializes an empty SectorMap and marks explored
  /// by registered races.
  TestWorldBuilder&
  add_planet(starnum_t snum = 0, PlanetType type = PlanetType::EARTH,
             std::string_view name = "", unsigned char maxx = 10,
             unsigned char maxy = 10,
             std::optional<planetnum_t> explicit_pnum = std::nullopt);

  /// Preset for standard 2-player solar system setup
  static void create_standard_solar_system(TestContext& ctx);

private:
  JsonStore store_;
  int next_player_id_{1};
  int next_star_id_{0};
  std::vector<player_t> registered_races_;
  std::vector<starnum_t> registered_stars_;
};
