// SPDX-License-Identifier: Apache-2.0

/// \file server_command_test.cc
/// \brief Unit tests for server boundary privilege propagation and guest
/// restrictions

import commands;
import dallib;
import gblib;
import session;
import test;
import std;

namespace {

bool mock_god_handler(const command_t&, GameObj& g) {
  g.out << "God command executed.\n";
  return true;
}

bool mock_mortal_handler(const command_t&, GameObj& g) {
  g.out << "Mortal command executed.\n";
  return true;
}

constexpr GB::commands::CommandDescriptor god_cmd{
    .name = "mock_god",
    .roles = {.god_only = true},
    .scopes = GB::commands::AllowedScopes::any(),
    .handler = &mock_god_handler,
};

constexpr GB::commands::CommandDescriptor no_guest_cmd{
    .name = "mock_no_guest",
    .roles = {.no_guests = true},
    .scopes = GB::commands::AllowedScopes::any(),
    .handler = &mock_mortal_handler,
};

// Test god privilege propagation from session to GameObj
void test_god_privilege_propagation() {
  TestContext ctx;
  Race god_race{};
  god_race.Playernum = 1;
  god_race.name = "DeityRace";
  god_race.God = true;
  {
    JsonStore store(ctx.db);
    RaceRepository race_repo(store);
    race_repo.save(god_race);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // 1. Session with god = true executes god_cmd
  g.set_god(true);
  ctx.assert_dispatch_success(g, god_cmd, {"mock_god"});
  test::expect_contains(g.out.str(), "God command executed.");

  // 2. Session with god = false is rejected
  g.out.str("");
  g.set_god(false);
  ctx.assert_dispatch_rejected(g, god_cmd, {"mock_god"});
  test::expect_contains(g.out.str(), "Only deity can use this command.");
}

// Test guest restrictions on guest-restricted commands
void test_emulation_privilege_drop_and_guest_restrictions() {
  TestContext ctx;
  Race guest_race{};
  guest_race.Playernum = 2;
  guest_race.name = "GuestRace";
  guest_race.Guest = true;

  Race normal_race{};
  normal_race.Playernum = 3;
  normal_race.name = "NormalRace";
  normal_race.Guest = false;

  {
    JsonStore store(ctx.db);
    RaceRepository race_repo(store);
    race_repo.save(guest_race);
    race_repo.save(normal_race);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Guest race cannot execute commands restricted with no_guests = true
  ctx.setup_game_obj(g, 2, 0);
  g.set_god(false);
  ctx.assert_dispatch_rejected(g, no_guest_cmd, {"mock_no_guest"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");

  // 2. Normal non-guest race executes standard command successfully
  g.out.str("");
  ctx.setup_game_obj(g, 3, 0);
  g.set_god(false);
  ctx.assert_dispatch_success(g, no_guest_cmd, {"mock_no_guest"});
  test::expect_contains(g.out.str(), "Mortal command executed.");
}

// Test server boundary dispatch lifecycle
void test_server_boundary_dispatch_lifecycle() {
  TestContext ctx;
  Race normal_race{};
  normal_race.Playernum = 1;
  normal_race.name = "NormalRace";
  {
    JsonStore store(ctx.db);
    RaceRepository race_repo(store);
    race_repo.save(normal_race);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // Normal player executes standard command
  ctx.assert_dispatch_success(g, no_guest_cmd, {"mock_no_guest"});
  test::expect_contains(g.out.str(), "Mortal command executed.");
}

}  // namespace

int main() {
  test_god_privilege_propagation();
  test_emulation_privilege_drop_and_guest_restrictions();
  test_server_boundary_dispatch_lifecycle();

  std::println(std::cout, "✓ server_command_test passed!");
  return 0;
}
