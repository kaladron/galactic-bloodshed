// SPDX-License-Identifier: Apache-2.0

import commands;
import dallib;
import gblib;
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
  ctx.setup_game_obj(g, player_t{1}, governor_t{1});

  GB::commands::CommandDescriptor desc{
      .name = "mock_leader",
      .roles = {.leader_only = true},
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = &mock_success_handler,
  };

  // Governor 1 is rejected
  g.out.str("");
  test::expect_false(GB::commands::dispatch_command(g, desc, {"mock_leader"}));
  test::expect_contains(g.out.str(),
                        "Only the leader (Governor 0) may use this command");

  // Governor 0 is allowed
  g.set_governor(0);
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
  sdata.governor[0] = 0;  // controlled by player 1 gov 0
  Star star{sdata};
  stars.save(star);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{1});
  g.set_snum(1);

  GB::commands::CommandDescriptor desc{
      .name = "mock_star_control",
      .roles = {.star_control = true},
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = &mock_success_handler,
  };

  // Gov 1 does not control system
  g.out.str("");
  test::expect_false(
      GB::commands::dispatch_command(g, desc, {"mock_star_control"}));
  test::expect_contains(g.out.str(),
                        "You are not authorized to do that in this system");

  // Gov 0 controls system
  g.set_governor(0);
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
  sdata.AP[0] = 10;
  Star star{sdata};
  stars.save(star);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{0});
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
  u.id = 1;
  u.AP[0] = 10;
  universe_repo.save(u);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{0});

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
  test::expect_eq(ctx.em.peek_universe()->AP[0], 10);

  // Set Univ AP to 20
  ctx.em.mutate_universe([](universe_struct& u) { u.AP[0] = 20; });

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
  test::expect_eq(ctx.em.peek_universe()->AP[0], 20);

  // Case 3: Handler returns true -> Success, 15 AP deducted
  g.out.str("");
  test::expect_true(
      GB::commands::dispatch_command(g, success_desc, {"mock_univ_cost"}));
  test::expect_eq(ctx.em.peek_universe()->AP[0], 5);

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

  std::println(std::cout, "✓ dispatch_pipeline_test passed!");
  return 0;
}
