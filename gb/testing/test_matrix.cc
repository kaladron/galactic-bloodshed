// SPDX-License-Identifier: Apache-2.0

/// \file test_matrix.cc
/// \brief Implementation of TestCommandMatrix 4-way test runner helpers.

module;

#include <cassert>

module test;

import commands;
import dallib;
import gb.entities;
import gb.services;
import gb.repositories;
import std;

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
  if (g.player().value > 0 && g.player().value <= MAXPLAYERS &&
      expected_univ_ap_ > 0) {
    ctx_.em.mutate_universe([&](universe_struct& u) {
      orig_univ_ap = u.AP[g.player()];
      u.AP[g.player()] = 0;
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
  if (g.player().value > 0 && g.player().value <= MAXPLAYERS &&
      expected_univ_ap_ > 0) {
    ctx_.em.mutate_universe(
        [&](universe_struct& u) { u.AP[g.player()] = orig_univ_ap; });
  }
}

void TestCommandMatrix::run_scope_checks(GameObj& g) const {
  for (ScopeLevel scope : invalid_scopes_) {
    g.set_level(scope);
    ctx_.assert_dispatch_rejected(g, desc_, valid_argv_);
  }
  g.set_level(valid_scope_);
}

void TestCommandMatrix::run_guest_check(GameObj& g) const {
  if (!desc_.roles.no_guests) return;

  player_t orig_player = g.player();
  governor_t orig_gov = g.governor();
  ScopeLevel orig_scope = g.level();

  if (orig_player > 0) {
    try {
      bool orig_guest = false;
      ctx_.em.mutate_race(orig_player, [&](Race& r) {
        orig_guest = r.Guest;
        r.Guest = true;
      });
      ctx_.setup_game_obj(g, orig_player, orig_gov);

      g.set_level(valid_scope_);
      ctx_.assert_dispatch_rejected(g, desc_, valid_argv_);

      ctx_.em.mutate_race(orig_player, [&](Race& r) { r.Guest = orig_guest; });
      ctx_.setup_game_obj(g, orig_player, orig_gov);
    } catch (const EntityNotFoundError&) {
    }
  }

  ctx_.setup_game_obj(g, orig_player, orig_gov);
  g.set_level(orig_scope);
}

void TestCommandMatrix::run_domain_error_check(GameObj& g) const {
  if (invalid_argv_.empty()) return;
  g.set_level(valid_scope_);
  ctx_.assert_dispatch_rejected(g, desc_, invalid_argv_);
}

void TestCommandMatrix::run_matrix(GameObj& g) const {
  run_insufficient_ap_check(g);
  run_scope_checks(g);
  run_guest_check(g);
  run_domain_error_check(g);
  run_happy_path(g);
}
