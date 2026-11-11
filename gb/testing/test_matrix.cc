// SPDX-License-Identifier: Apache-2.0

/// \file test_matrix.cc
/// \brief Implementation of TestCommandMatrix 4-way test runner helpers.

module;

#include <cassert>

module test;

import commands;
import dallib;
import gb.entities;
import gb.presentation;
import gb.repositories;
import gb.services;
import glaze.json;
import std;

struct JsonEnvelopeProbe {
  std::string type{};
  glz::generic data{};
};

namespace {

struct SavedGameObjState {
  player_t player;
  governor_t governor;
  bool had_race;
  bool god;
  bool disconnect_requested;
  bool shutdown_requested;
  starnum_t snum;
  planetnum_t pnum;
  shipnum_t shipno;
  ScopeLevel level;
  UiMode ui_mode;
  bool color;
  UniverseCoordinates universe_center;
  SystemCoordinates system_center;
  double zoom0;
  double zoom1;
  RecordingSessionRegistry* rec;
  std::vector<SentNotification> notifications;

  static SavedGameObjState capture(GameObj& g) {
    auto* reg = dynamic_cast<RecordingSessionRegistry*>(&g.session_registry);
    return {
        .player = g.player(),
        .governor = g.governor(),
        .had_race = (g.race != nullptr),
        .god = g.god(),
        .disconnect_requested = g.disconnect_requested(),
        .shutdown_requested = g.shutdown_requested(),
        .snum = g.snum(),
        .pnum = g.pnum(),
        .shipno = g.shipno(),
        .level = g.level(),
        .ui_mode = g.ui_mode(),
        .color = g.color(),
        .universe_center = g.universe_center(),
        .system_center = g.system_center(),
        .zoom0 = g.zoom[0],
        .zoom1 = g.zoom[1],
        .rec = reg,
        .notifications =
            reg ? reg->notifications : std::vector<SentNotification>{},
    };
  }

  void restore(EntityManager& em, GameObj& g) const {
    g.set_player(player);
    g.set_governor(governor);
    g.set_god(god);
    g.set_disconnect_requested(disconnect_requested);
    g.set_shutdown_requested(shutdown_requested);
    g.set_snum(snum);
    g.set_pnum(pnum);
    g.set_shipno(shipno);
    g.set_level(level);
    g.set_ui_mode(ui_mode);
    g.set_color(color);
    g.set_universe_center(universe_center);
    g.set_system_center(system_center);
    g.zoom[0] = zoom0;
    g.zoom[1] = zoom1;
    g.race = (had_race && player > 0) ? em.peek_race(player) : nullptr;
    if (rec) rec->notifications = notifications;
  }
};

void validate_ndjson_output(std::string_view out, std::string_view cmd_name) {
  test::expect_true(out.empty() || out.ends_with('\n'),
                    "NDJSON command output must be newline-terminated");
  std::size_t session_stanzas = 0;
  for (std::string_view rem = out; !rem.empty();) {
    const auto nl = rem.find('\n');
    const std::string_view line = rem.substr(0, nl);
    rem.remove_prefix(nl + 1);
    test::expect_false(line.empty(),
                       "NDJSON output must not contain blank lines");
    JsonEnvelopeProbe probe{};
    test::expect_false(
        static_cast<bool>(glz::read_json(probe, line)),
        std::format("Invalid NDJSON line in {}: {}", cmd_name, line));
    test::expect_false(probe.type.empty(), "JSON envelope type empty");
    test::expect_false(probe.data.is_null(), "JSON envelope data null");
    if (probe.type == "session") ++session_stanzas;
  }
  test::expect_true(session_stanzas <= 1,
                    "Commands must emit at most one session stanza");
}

}  // namespace

TestCommandMatrix::TestCommandMatrix(
    TestContext& ctx, const GB::commands::CommandDescriptor& desc)
    : ctx_(ctx), desc_(desc) {}

TestCommandMatrix::TestCommandMatrix(TestContext& ctx,
                                     std::string_view cmd_name)
    : ctx_(ctx), desc_(*GB::commands::find_command_descriptor(cmd_name)) {}

TestCommandMatrix& TestCommandMatrix::with_valid_argv(command_t argv) {
  valid_argv_ = std::move(argv);
  return *this;
}

TestCommandMatrix& TestCommandMatrix::with_invalid_argv(command_t argv) {
  invalid_argv_ = std::move(argv);
  return *this;
}

TestCommandMatrix& TestCommandMatrix::with_valid_scope(ScopeLevel scope) {
  valid_scope_ = scope;
  return *this;
}

TestCommandMatrix&
TestCommandMatrix::with_invalid_scopes(std::vector<ScopeLevel> scopes) {
  invalid_scopes_ = std::move(scopes);
  return *this;
}

TestCommandMatrix& TestCommandMatrix::with_expected_star_ap(ap_t ap) {
  expected_star_ap_ = ap;
  return *this;
}

TestCommandMatrix& TestCommandMatrix::with_expected_univ_ap(ap_t ap) {
  expected_univ_ap_ = ap;
  return *this;
}

void TestCommandMatrix::run_happy_path(GameObj& g) const {
  g.set_level(valid_scope_);
  ctx_.assert_dispatch_success(g, desc_, valid_argv_, expected_star_ap_,
                               expected_univ_ap_);
}

void TestCommandMatrix::run_insufficient_ap_check(GameObj& g) const {
  if (expected_star_ap_ == 0 && expected_univ_ap_ == 0) return;

  g.set_level(valid_scope_);
  starnum_t snum = g.snum();
  ap_t orig_star_ap = 0;
  if (expected_star_ap_ > 0) {
    try {
      ctx_.em.mutate_star(snum, [&](Star& s) {
        orig_star_ap = s.AP(g.player());
        s.AP(g.player()) = 0;
      });
    } catch (const EntityNotFoundError&) {
    }
  }

  ap_t orig_univ_ap = 0;
  if (expected_univ_ap_ > 0) {
    ctx_.em.mutate_universe([&](Universe& u) {
      orig_univ_ap = u.get_AP(g.player());
      u.set_AP(g.player(), 0);
    });
  }

  ctx_.assert_dispatch_rejected(g, desc_, valid_argv_);

  if (expected_star_ap_ > 0) {
    try {
      ctx_.em.mutate_star(snum,
                          [&](Star& s) { s.AP(g.player()) = orig_star_ap; });
    } catch (const EntityNotFoundError&) {
    }
  }
  if (expected_univ_ap_ > 0) {
    ctx_.em.mutate_universe(
        [&](Universe& u) { u.set_AP(g.player(), orig_univ_ap); });
  }
}

void TestCommandMatrix::run_scope_checks(GameObj& g) const {
  for (ScopeLevel scope : invalid_scopes_) {
    g.set_level(scope);
    ctx_.assert_dispatch_rejected(g, desc_, valid_argv_);
  }
  g.set_level(valid_scope_);
}

bool TestCommandMatrix::run_json_mode_check(GameObj& g) const {
  const auto saved = SavedGameObjState::capture(g);
  Database snapshot(":memory:");
  ctx_.db.copy_to(snapshot);

  g.set_ui_mode(UiMode::JSON);
  g.set_level(valid_scope_);
  const auto before_changes = ctx_.db.total_changes();
  ctx_.assert_dispatch_success(g, desc_, valid_argv_, expected_star_ap_,
                               expected_univ_ap_);
  const bool mutated_db = (ctx_.db.total_changes() > before_changes);
  validate_ndjson_output(g.out.view(), desc_.name);

  snapshot.copy_to(ctx_.db);
  ctx_.em.clear_cache();
  saved.restore(ctx_.em, g);
  return mutated_db;
}

void TestCommandMatrix::run_guest_check(GameObj& g,
                                        bool expect_rejected) const {
  const auto saved = SavedGameObjState::capture(g);
  if (!saved.had_race || saved.player == 0) return;

  bool orig_guest = false;
  ctx_.em.mutate_race(saved.player, [&](Race& r) {
    orig_guest = r.Guest;
    r.Guest = true;
  });
  g.race = ctx_.em.peek_race(saved.player);
  g.set_god(false);
  g.set_level(valid_scope_);

  const auto guest_before = ctx_.db.total_changes();
  if (expect_rejected) {
    ctx_.assert_dispatch_rejected(g, desc_, valid_argv_);
  } else {
    ctx_.assert_dispatch_success(g, desc_, valid_argv_, 0, 0);
  }
  test::expect_eq(ctx_.db.total_changes(), guest_before,
                  "Guest execution must never modify SQLite");

  ctx_.em.mutate_race(saved.player, [&](Race& r) { r.Guest = orig_guest; });
  saved.restore(ctx_.em, g);
}

void TestCommandMatrix::run_guest_check(GameObj& g) const {
  const bool role_blocked = desc_.roles.no_guests || desc_.roles.god_only;
  const bool ap_blocked = expected_star_ap_ > 0 || expected_univ_ap_ > 0;
  if (role_blocked || ap_blocked) {
    run_guest_check(g, true);
    return;
  }
  const bool mutated_db = run_json_mode_check(g);
  run_guest_check(g, mutated_db && desc_.name != "read");
}

void TestCommandMatrix::run_domain_error_check(GameObj& g) const {
  if (invalid_argv_.empty()) return;
  g.set_level(valid_scope_);
  ctx_.assert_dispatch_rejected(g, desc_, invalid_argv_);
}

void TestCommandMatrix::run_matrix(GameObj& g) const {
  run_insufficient_ap_check(g);
  run_scope_checks(g);
  const bool mutated_db = run_json_mode_check(g);
  const bool role_blocked = desc_.roles.no_guests || desc_.roles.god_only;
  const bool ap_blocked = expected_star_ap_ > 0 || expected_univ_ap_ > 0;
  const bool db_blocked = mutated_db && desc_.name != "read";
  run_guest_check(g, role_blocked || ap_blocked || db_blocked);
  run_domain_error_check(g);
  run_happy_path(g);
}
