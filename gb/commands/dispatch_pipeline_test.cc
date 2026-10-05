// SPDX-License-Identifier: Apache-2.0

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

bool mock_success_handler(const command_t&, GameObj& g) {
  g.out << "handler executed successfully\n";
  return true;
}

bool mock_failure_handler(const command_t&, GameObj& g) {
  g.out << "handler failed\n";
  return false;
}

void test_role_god_only() {
  TestContext ctx;
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  GB::commands::CommandDescriptor desc{
      .name = "mock_god",
      .roles = {.god_only = true},
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = &mock_success_handler,
  };

  // Mortal cannot run god command
  g.set_god(false);
  g.out.str("");
  test::expect_false(GB::commands::dispatch_command(g, desc, {"mock_god"}));
  test::expect_contains(g.out.str(), "Only deity can use this command");

  // God can run god command
  g.set_god(true);
  g.out.str("");
  test::expect_true(GB::commands::dispatch_command(g, desc, {"mock_god"}));
  test::expect_contains(g.out.str(), "handler executed successfully");

  ctx.verify_universe_invariants();
}

void test_role_no_guests() {
  TestContext ctx;
  JsonStore store(ctx.db);
  RaceRepository races(store);

  Race guest_race{};
  guest_race.Playernum = 1;
  guest_race.Guest = true;
  races.save(guest_race);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  GB::commands::CommandDescriptor desc{
      .name = "mock_no_guests",
      .roles = {.no_guests = true},
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = &mock_success_handler,
  };

  // Guest race is rejected
  g.out.str("");
  test::expect_false(
      GB::commands::dispatch_command(g, desc, {"mock_no_guests"}));
  test::expect_contains(g.out.str(), "Guest races cannot use this command");

  // Non-guest race is allowed
  ctx.em.mutate_race(g.player(), [](Race& r) { r.Guest = false; });
  g.race = ctx.em.peek_race(g.player());

  g.out.str("");
  test::expect_true(
      GB::commands::dispatch_command(g, desc, {"mock_no_guests"}));
  test::expect_contains(g.out.str(), "handler executed successfully");

  ctx.verify_universe_invariants();
}

void test_role_leader_only() {
  TestContext ctx;
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 2);

  GB::commands::CommandDescriptor desc{
      .name = "mock_leader",
      .roles = {.leader_only = true},
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = &mock_success_handler,
  };

  // Governor 2 is rejected
  g.out.str("");
  test::expect_false(GB::commands::dispatch_command(g, desc, {"mock_leader"}));
  test::expect_contains(g.out.str(),
                        "Only the leader (Governor 1) may use this command");

  // Governor 1 is allowed
  g.set_governor(1);
  g.out.str("");
  test::expect_true(GB::commands::dispatch_command(g, desc, {"mock_leader"}));
  test::expect_contains(g.out.str(), "handler executed successfully");

  ctx.verify_universe_invariants();
}

void test_role_star_control() {
  TestContext ctx;
  JsonStore store(ctx.db);
  StarRepository stars(store);

  star_struct sdata{};
  sdata.star_id = 1;
  sdata.governor[player_t{1}] = 1;  // controlled by player 1 gov 1
  Star star{sdata};
  stars.save(star);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 2);
  g.set_snum(1);

  GB::commands::CommandDescriptor desc{
      .name = "mock_star_control",
      .roles = {.star_control = true},
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = &mock_success_handler,
  };

  // Gov 2 does not control system
  g.out.str("");
  test::expect_false(
      GB::commands::dispatch_command(g, desc, {"mock_star_control"}));
  test::expect_contains(g.out.str(),
                        "You are not authorized to do that in this system");

  // Gov 1 controls system
  g.set_governor(1);
  g.out.str("");
  test::expect_true(
      GB::commands::dispatch_command(g, desc, {"mock_star_control"}));
  test::expect_contains(g.out.str(), "handler executed successfully");

  ctx.verify_universe_invariants();
}

void test_scope_validation() {
  TestContext ctx;
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  GB::commands::CommandDescriptor desc{
      .name = "mock_plan_only",
      .scopes = GB::commands::AllowedScopes::planet_only(),
      .handler = &mock_success_handler,
  };

  // Rejected at UNIV
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  test::expect_false(
      GB::commands::dispatch_command(g, desc, {"mock_plan_only"}));
  test::expect_contains(g.out.str(), "Invalid scope for this command");

  // Allowed at PLAN
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.out.str("");
  test::expect_true(
      GB::commands::dispatch_command(g, desc, {"mock_plan_only"}));
  test::expect_contains(g.out.str(), "handler executed successfully");

  ctx.verify_universe_invariants();
}

void test_argument_validation() {
  TestContext ctx;
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  GB::commands::CommandDescriptor desc{
      .name = "mock_args",
      .scopes = GB::commands::AllowedScopes::any(),
      .min_args = 3,
      .syntax = "mock_args <arg1> <arg2>",
      .handler = &mock_success_handler,
  };

  // Too few arguments
  g.out.str("");
  test::expect_false(
      GB::commands::dispatch_command(g, desc, {"mock_args", "foo"}));
  test::expect_contains(g.out.str(), "Syntax: mock_args <arg1> <arg2>");

  // Sufficient arguments
  g.out.str("");
  test::expect_true(
      GB::commands::dispatch_command(g, desc, {"mock_args", "foo", "bar"}));
  test::expect_contains(g.out.str(), "handler executed successfully");

  ctx.verify_universe_invariants();
}

void test_fixed_star_ap_transactions() {
  TestContext ctx;
  JsonStore store(ctx.db);
  StarRepository stars(store);

  star_struct sdata{};
  sdata.star_id = 1;
  sdata.AP[player_t{1}] = 10;
  Star star{sdata};
  stars.save(star);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_snum(1);

  GB::commands::CommandDescriptor success_desc{
      .name = "mock_cost",
      .scopes = GB::commands::AllowedScopes::any(),
      .ap = GB::commands::APCost::fixed_star(15),
      .handler = &mock_success_handler,
  };

  // Case 1: Insufficient AP (have 10, need 15) -> Rejected, AP unchanged
  g.out.str("");
  test::expect_false(
      GB::commands::dispatch_command(g, success_desc, {"mock_cost"}));
  test::expect_contains(g.out.str(), "You don't have 15 action points there");
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 10);

  // Set AP to 20
  ctx.em.mutate_star(1, [](Star& s) { s.AP(player_t{1}) = 20; });

  // Case 2: Sufficient AP, Handler returns false -> Rejected, AP unchanged
  GB::commands::CommandDescriptor fail_desc{
      .name = "mock_fail",
      .scopes = GB::commands::AllowedScopes::any(),
      .ap = GB::commands::APCost::fixed_star(15),
      .handler = &mock_failure_handler,
  };
  g.out.str("");
  test::expect_false(
      GB::commands::dispatch_command(g, fail_desc, {"mock_fail"}));
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 20);

  // Case 3: Sufficient AP, Handler returns true -> Success, 15 AP deducted
  g.out.str("");
  test::expect_true(
      GB::commands::dispatch_command(g, success_desc, {"mock_cost"}));
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 5);

  ctx.verify_universe_invariants();
}

void test_fixed_univ_ap_transactions() {
  TestContext ctx;
  JsonStore store(ctx.db);
  UniverseRepository universe_repo(store);

  universe_struct u{};
  u.AP[player_t{1}] = 10;
  universe_repo.save(u);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  GB::commands::CommandDescriptor success_desc{
      .name = "mock_univ_cost",
      .scopes = GB::commands::AllowedScopes::any(),
      .ap = GB::commands::APCost::fixed_univ(15),
      .handler = &mock_success_handler,
  };

  // Case 1: Insufficient Univ AP (have 10, need 15) -> Rejected, AP unchanged
  g.out.str("");
  test::expect_false(
      GB::commands::dispatch_command(g, success_desc, {"mock_univ_cost"}));
  test::expect_contains(g.out.str(), "You need 15 universe action points");
  test::expect_eq(ctx.em.peek_universe()->get_AP(1), 10);

  // Set Univ AP to 20
  ctx.em.mutate_universe([](universe_struct& u) { u.set_AP(1, 20); });

  // Case 2: Handler returns false -> AP unchanged (still 20)
  GB::commands::CommandDescriptor fail_desc{
      .name = "mock_univ_fail",
      .scopes = GB::commands::AllowedScopes::any(),
      .ap = GB::commands::APCost::fixed_univ(15),
      .handler = &mock_failure_handler,
  };
  g.out.str("");
  test::expect_false(
      GB::commands::dispatch_command(g, fail_desc, {"mock_univ_fail"}));
  test::expect_eq(ctx.em.peek_universe()->get_AP(1), 20);

  // Case 3: Handler returns true -> Success, 15 AP deducted
  g.out.str("");
  test::expect_true(
      GB::commands::dispatch_command(g, success_desc, {"mock_univ_cost"}));
  test::expect_eq(ctx.em.peek_universe()->get_AP(1), 5);

  ctx.verify_universe_invariants();
}

void test_dispatch_by_command_name() {
  TestContext ctx;
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // Empty argv returns false
  test::expect_false(GB::commands::dispatch_command(g, {}));

  // Unknown command name returns false
  test::expect_false(
      GB::commands::dispatch_command(g, {"nonexistent_command"}));

  // Known command ("treasury") dispatches through registry and succeeds
  g.out.str("");
  test::expect_true(GB::commands::dispatch_command(g, {"treasury"}));
  test::expect_contains(g.out.str(), "You have:");

  ctx.verify_universe_invariants();
}

void test_edge_cases_and_socket_outbox() {
  TestContext ctx;
  ctx.with_standard_universe();

  RecordingSessionRegistry registry;
  registry.sessions = {
      SessionInfo{.player = 1, .governor = 1, .connected = true},
      SessionInfo{.player = 2, .governor = 1, .connected = true},
      SessionInfo{.player = 2, .governor = 2, .connected = false},
  };
  ctx.em.mutate_race(2, [](Race& r) { r.appoint_governor(2); });

  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  // 1. player() == 0 sets g.race = nullptr and passes no_guests check
  g.set_player(0);
  GB::commands::CommandDescriptor no_guest_desc{
      .name = "mock_player0",
      .roles = {.no_guests = true},
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = &mock_success_handler,
  };
  test::expect_true(
      GB::commands::dispatch_command(g, no_guest_desc, {"mock_player0"}));
  test::expect_true(g.race == nullptr);
  g.set_player(1);

  // 2. Missing star in star_control and FixedStar catches EntityNotFoundError
  g.set_snum(999);
  GB::commands::CommandDescriptor star_ctrl_desc{
      .name = "mock_star_ctrl_missing",
      .roles = {.star_control = true},
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = &mock_success_handler,
  };
  test::expect_false(GB::commands::dispatch_command(
      g, star_ctrl_desc, {"mock_star_ctrl_missing"}));

  GB::commands::CommandDescriptor fixed_star_desc{
      .name = "mock_fixed_star_missing",
      .scopes = GB::commands::AllowedScopes::any(),
      .ap = GB::commands::APCost::fixed_star(1),
      .handler = &mock_success_handler,
  };
  test::expect_false(GB::commands::dispatch_command(
      g, fixed_star_desc, {"mock_fixed_star_missing"}));
  g.set_snum(1);

  // 3. Null handler returns false
  GB::commands::CommandDescriptor null_handler_desc{
      .name = "mock_null",
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = nullptr,
  };
  test::expect_false(
      GB::commands::dispatch_command(g, null_handler_desc, {"mock_null"}));

  // 4. Failed command rolls back both SQLite telegrams and staged SocketOutbox
  GB::commands::CommandDescriptor fail_outbox_desc{
      .name = "mock_fail_outbox",
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = [](const command_t&, GameObj& game) -> bool {
        d_broadcast(game.session_registry, game.entity_manager, 1, 1,
                    "Leaked broadcast!\n");
        warn_player(game.session_registry, game.entity_manager, 2, 1,
                    "Leaked online warning!\n");
        // Disconnect (2, 1) temporarily to force an offline telegram for (2, 2)
        push_telegram(game.entity_manager, 2, 2, "Leaked offline telegram!\n");
        return false;
      },
  };
  registry.clear_notifications();
  test::expect_false(GB::commands::dispatch_command(g, fail_outbox_desc,
                                                    {"mock_fail_outbox"}));
  test::expect_true(registry.notifications.empty());
  test::expect_eq(ctx.em.get_telegrams(2, 2).size(), 0u);

  // 5. Throwing command (transactional and non-transactional) rolls back outbox
  GB::commands::CommandDescriptor throw_txn_desc{
      .name = "mock_throw_txn",
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = [](const command_t&, GameObj& game) -> bool {
        warn_player(game.session_registry, game.entity_manager, 2, 1,
                    "Leaked on exception!\n");
        throw std::runtime_error("boom");
      },
  };
  test::expect_throws<std::runtime_error>([&]() {
    GB::commands::dispatch_command(g, throw_txn_desc, {"mock_throw_txn"});
  });
  test::expect_true(registry.notifications.empty());

  GB::commands::CommandDescriptor throw_nontxn_desc{
      .name = "mock_throw_nontxn",
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = [](const command_t&, GameObj&) -> bool {
        throw std::runtime_error("nontxn boom");
      },
      .transactional = false,
  };
  test::expect_throws<std::runtime_error>([&]() {
    GB::commands::dispatch_command(g, throw_nontxn_desc, {"mock_throw_nontxn"});
  });

  // 6. Successful command flushes outbox after commit and falls back to
  // push_telegram if a recipient disconnected before commit_outbox()
  GB::commands::CommandDescriptor commit_outbox_desc{
      .name = "mock_commit_outbox",
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = [](const command_t&, GameObj& game) -> bool {
        warn_player(game.session_registry, game.entity_manager, 1, 1,
                    "Delivered live!\n");
        warn_player(game.session_registry, game.entity_manager, 2, 1,
                    "Fallback after disconnect!\n");
        static_cast<RecordingSessionRegistry&>(game.session_registry)
            .sessions[1]
            .connected = false;
        return true;
      },
  };
  test::expect_true(GB::commands::dispatch_command(g, commit_outbox_desc,
                                                   {"mock_commit_outbox"}));
  test::expect_true(registry.has_received(1, "Delivered live!"));
  test::expect_eq(ctx.em.get_telegrams(2, 1).size(), 1u);

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_role_god_only();
  test_role_no_guests();
  test_role_leader_only();
  test_role_star_control();
  test_scope_validation();
  test_argument_validation();
  test_fixed_star_ap_transactions();
  test_fixed_univ_ap_transactions();
  test_dispatch_by_command_name();
  test_edge_cases_and_socket_outbox();

  std::println(std::cout, "✓ dispatch_pipeline_test passed!");
  return 0;
}
