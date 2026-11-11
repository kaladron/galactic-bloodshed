// SPDX-License-Identifier: Apache-2.0

/// \file dispatch_pipeline_test.cc
/// \brief Unit tests for dispatch_command preconditions, transactions, and
/// Guest read-only enforcement.

import commands;
import dallib;
import gb.entities;
import gb.presentation;
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

shipnum_t g_vis_sid{0};
shipnum_t g_invis_sid{0};
shipnum_t g_del_sid{0};
commodnum_t g_c1_id{0};
commodnum_t g_c2_id{0};

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

  Universe u{};
  u.set_AP(player_t{1}, 10);
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
  ctx.em.mutate_universe([](Universe& u) { u.set_AP(1, 20); });

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

void test_json_dirty_entity_auto_emission() {
  TestContext ctx;
  ctx.with_standard_universe();
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. ASCII mode: mutations are not tracked or emitted as JSON
  g.set_ui_mode(UiMode::ASCII);
  GB::commands::CommandDescriptor ascii_cmd{
      .name = "ascii_cmd",
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = [](const command_t&, GameObj& game) -> bool {
        game.entity_manager.mutate_star(1, [](Star& s) { s.gravity() = 2.0; });
        return true;
      },
  };
  g.out.clear();
  test::expect_true(
      GB::commands::dispatch_command(g, ascii_cmd, {"ascii_cmd"}));
  test::expect_true(g.out.str().empty());

  // 2. JSON mode with player == 0: tracking disabled
  g.set_ui_mode(UiMode::JSON);
  g.set_player(0);
  g.race = nullptr;
  g.out.clear();
  test::expect_true(
      GB::commands::dispatch_command(g, ascii_cmd, {"ascii_cmd"}));
  test::expect_true(g.out.str().empty());
  ctx.setup_game_obj(g, 1, 1);

  // 3. JSON mode with no mutations: dirty is empty, nothing emitted
  GB::commands::CommandDescriptor noop_cmd{
      .name = "noop_cmd",
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = [](const command_t&, GameObj&) -> bool { return true; },
  };
  g.out.clear();
  test::expect_true(GB::commands::dispatch_command(g, noop_cmd, {"noop_cmd"}));
  test::expect_true(g.out.str().empty());

  // 4. Command returns false: dirty entities cleared, nothing emitted
  GB::commands::CommandDescriptor fail_cmd{
      .name = "fail_cmd",
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = [](const command_t&, GameObj& game) -> bool {
        game.entity_manager.mutate_star(1, [](Star& s) { s.gravity() = 3.0; });
        return false;
      },
  };
  g.out.clear();
  test::expect_false(GB::commands::dispatch_command(g, fail_cmd, {"fail_cmd"}));
  test::expect_true(g.out.str().empty());

  // 5. Exception handling:
  // 5a. UiMode::JSON with exception cleans up dirty entities and stops tracking
  GB::commands::CommandDescriptor throw_json_cmd{
      .name = "throw_json",
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = [](const command_t&, GameObj& game) -> bool {
        game.entity_manager.mutate_star(1, [](Star& s) { s.gravity() = 4.0; });
        throw std::runtime_error("json boom");
      },
  };
  test::expect_throws<std::runtime_error>([&]() {
    GB::commands::dispatch_command(g, throw_json_cmd, {"throw_json"});
  });

  // 5b. UiMode::ASCII with exception (track_json_deltas == false)
  g.set_ui_mode(UiMode::ASCII);
  test::expect_throws<std::runtime_error>([&]() {
    GB::commands::dispatch_command(g, throw_json_cmd, {"throw_json"});
  });
  g.set_ui_mode(UiMode::JSON);

  // 6. Explored vs Unexplored Star and Planet
  // Make Star 2 and Planet 2,1 unexplored by player 1
  ctx.em.mutate_star(2, [](Star& s) { s.clear_explored_by(1); });
  ctx.em.mutate_planet(2, 1, [](Planet& p) {
    p.info(1).explored = 0;
    p.info(1).numsectsowned = 0;
  });

  GB::commands::CommandDescriptor star_planet_cmd{
      .name = "star_planet_cmd",
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = [](const command_t&, GameObj& game) -> bool {
        // Star 1 (explored) and Star 2 (unexplored)
        game.entity_manager.mutate_star(1, [](Star& s) { s.gravity() = 1.1; });
        game.entity_manager.mutate_star(2, [](Star& s) { s.gravity() = 1.2; });
        // Planet 1,1 (explored) and Planet 2,1 (unexplored)
        game.entity_manager.mutate_planet(1, 1,
                                          [](Planet& p) { p.toxic() = 25; });
        game.entity_manager.mutate_planet(2, 1,
                                          [](Planet& p) { p.toxic() = 30; });
        return true;
      },
  };
  g.out.clear();
  test::expect_true(
      GB::commands::dispatch_command(g, star_planet_cmd, {"star_planet_cmd"}));
  std::string out = g.out.str();
  test::expect_contains(out, R"("type":"star")");
  test::expect_contains(out, R"("name":"Sol")");
  test::expect_false(out.contains(R"("name":"Vega")"));
  test::expect_contains(out, R"("type":"planet")");
  test::expect_contains(out, R"("name":"Earth")");
  test::expect_false(out.contains(R"("name":"Vega Prime")"));

  // 7. Explored vs Unexplored Sectors
  GB::commands::CommandDescriptor sector_cmd{
      .name = "sector_cmd",
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = [](const command_t&, GameObj& game) -> bool {
        // Sector on explored star/planet (1, 1)
        game.entity_manager.mutate_sectormap(
            1, 1, [](SectorMap& sm) { sm.get({0, 0}).set_fert(80); });
        // Sector on unexplored star/planet (2, 1)
        game.entity_manager.mutate_sectormap(
            2, 1, [](SectorMap& sm) { sm.get({0, 0}).set_fert(70); });
        return true;
      },
  };
  g.out.clear();
  test::expect_true(
      GB::commands::dispatch_command(g, sector_cmd, {"sector_cmd"}));
  out = g.out.str();
  test::expect_contains(out, R"("type":"sectors")");
  test::expect_contains(out, R"("snum":1)");
  test::expect_false(out.contains(R"("snum":2)"));

  // 8. Ships: visible, invisible, and deleted
  g_vis_sid = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                  .owned_by(1)
                  .in_planet_orbit(1, 1)
                  .build();

  g_invis_sid = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                    .owned_by(2)
                    .in_deep_space()
                    .build();

  g_del_sid = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                  .owned_by(1)
                  .in_planet_orbit(1, 1)
                  .build();

  GB::commands::CommandDescriptor ship_cmd{
      .name = "ship_cmd",
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = [](const command_t&, GameObj& game) -> bool {
        // Mutate visible ship
        game.entity_manager.mutate_ship(g_vis_sid,
                                        [](Ship& s) { s.add_fuel(10.0); });
        // Mutate invisible ship
        game.entity_manager.mutate_ship(g_invis_sid,
                                        [](Ship& s) { s.add_fuel(5.0); });
        // Delete third ship
        {
          auto barrier = game.entity_manager.create_deletion_barrier();
          game.entity_manager.delete_ship(g_del_sid);
        }
        return true;
      },
  };
  g.out.clear();
  test::expect_true(GB::commands::dispatch_command(g, ship_cmd, {"ship_cmd"}));
  out = g.out.str();
  test::expect_contains(out, std::format(R"("id":{})", g_vis_sid.value));
  test::expect_false(
      out.contains(std::format(R"("id":{})", g_invis_sid.value)));
  test::expect_contains(
      out, std::format(R"("id":{},"alive":false)", g_del_sid.value));

  // 9. Races: self vs foreign
  GB::commands::CommandDescriptor race_cmd{
      .name = "race_cmd",
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = [](const command_t&, GameObj& game) -> bool {
        game.entity_manager.mutate_race(1, [](Race& r) { r.morale = 50; });
        game.entity_manager.mutate_race(2, [](Race& r) { r.morale = 60; });
        return true;
      },
  };
  g.set_god(false);
  g.out.clear();
  test::expect_true(GB::commands::dispatch_command(g, race_cmd, {"race_cmd"}));
  out = g.out.str();
  test::expect_contains(out, R"("type":"race")");
  test::expect_contains(out, R"("player":1)");
  test::expect_false(out.contains(R"("player":2)"));

  // 10. Block mutation
  GB::commands::CommandDescriptor block_cmd{
      .name = "block_cmd",
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = [](const command_t&, GameObj& game) -> bool {
        game.entity_manager.mutate_block(
            1, [](Block& b) { b.name() = "AlliedForces"; });
        return true;
      },
  };
  g.out.clear();
  test::expect_true(
      GB::commands::dispatch_command(g, block_cmd, {"block_cmd"}));
  out = g.out.str();
  test::expect_contains(out, R"("type":"block")");
  test::expect_contains(out, R"("name":"AlliedForces")");

  // 11. Commod mutation and deletion across ScopeLevels (LEVEL_STAR,
  // LEVEL_PLAN, LEVEL_UNIV) Setup Star 2 position far enough for shipping cost
  // calculation
  ctx.em.mutate_star(2, [](Star& s) { s.set_coordinates({20000.0, 0.0}); });

  {
    auto commod1 = ctx.em.create_commod({
        .owner = 2,
        .type = CommodType::RESOURCE,
        .amount = 100,
        .bid = 500,
        .bidder = 1,
        .star_from = 1,
        .planet_from = 1,
        .star_to = 1,
        .planet_to = 1,
    });
    g_c1_id = commodnum_t{commod1->id()};
  }

  {
    auto commod2 = ctx.em.create_commod({
        .owner = 1,
        .type = CommodType::FUEL,
        .amount = 200,
        .bid = 300,
        .star_from = 1,
        .planet_from = 1,
    });
    g_c2_id = commodnum_t{commod2->id()};
  }

  // 11a. LEVEL_STAR scope with star 2 as viewer location (calculates shipping
  // cost)
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(2);
  GB::commands::CommandDescriptor commod_star_cmd{
      .name = "commod_star",
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = [](const command_t&, GameObj& game) -> bool {
        game.entity_manager.mutate_commod(g_c1_id.value,
                                          [](Commod& c) { c.set_bid(550); });
        return true;
      },
  };
  g.out.clear();
  test::expect_true(
      GB::commands::dispatch_command(g, commod_star_cmd, {"commod_star"}));
  out = g.out.str();
  test::expect_contains(out, R"("type":"market_lot")");
  test::expect_contains(out, std::format(R"("id":{})", g_c1_id.value));
  test::expect_contains(out, R"("dest_star":1)");
  test::expect_contains(out, R"("dest_planet":1)");
  test::expect_false(out.contains(R"("shipping_cost":0)"));

  // 11b. LEVEL_PLAN scope
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(2);
  g.set_pnum(1);
  GB::commands::CommandDescriptor commod_plan_cmd{
      .name = "commod_plan",
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = [](const command_t&, GameObj& game) -> bool {
        game.entity_manager.mutate_commod(g_c1_id.value,
                                          [](Commod& c) { c.set_bid(600); });
        return true;
      },
  };
  g.out.clear();
  test::expect_true(
      GB::commands::dispatch_command(g, commod_plan_cmd, {"commod_plan"}));
  out = g.out.str();
  test::expect_contains(out, R"("type":"market_lot")");
  test::expect_contains(out, R"("dest_star":1)");

  // 11c. LEVEL_UNIV scope with active and deleted commod
  g.set_level(ScopeLevel::LEVEL_UNIV);
  GB::commands::CommandDescriptor commod_univ_cmd{
      .name = "commod_univ",
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = [](const command_t&, GameObj& game) -> bool {
        game.entity_manager.mutate_commod(g_c1_id.value,
                                          [](Commod& c) { c.set_bid(700); });
        {
          auto barrier = game.entity_manager.create_deletion_barrier();
          game.entity_manager.delete_commod(g_c2_id.value);
        }
        return true;
      },
  };
  g.out.clear();
  test::expect_true(
      GB::commands::dispatch_command(g, commod_univ_cmd, {"commod_univ"}));
  out = g.out.str();
  test::expect_contains(out, std::format(R"("id":{})", g_c1_id.value));
  test::expect_contains(out, R"("shipping_cost":0)");
  test::expect_contains(
      out, std::format(R"("id":{},"active":false)", g_c2_id.value));

  // 12. Fixed AP deduction mutates star and emits updated StarView
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  GB::commands::CommandDescriptor ap_cmd{
      .name = "ap_cmd",
      .scopes = GB::commands::AllowedScopes::any(),
      .ap = GB::commands::APCost::fixed_star(5),
      .handler = [](const command_t&, GameObj&) -> bool { return true; },
  };
  g.out.clear();
  test::expect_true(GB::commands::dispatch_command(g, ap_cmd, {"ap_cmd"}));
  out = g.out.str();
  test::expect_contains(out, R"("type":"star")");
  test::expect_contains(out, R"("name":"Sol")");

  ctx.verify_universe_invariants();
}

void test_guest_read_only_scope_enforcement() {
  TestContext ctx;
  ctx.with_standard_universe().with_populated_planet(1, 1, 1, 1000);
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 10; });
  ctx.em.mutate_race(1, [](Race& r) { r.Guest = true; });
  const auto sid = TestShipBuilder(ctx.em, ShipType::STYPE_DESTROYER)
                       .owned_by(1, 1)
                       .named("GuestProbe")
                       .in_planet_orbit(1, 1)
                       .with_crew(10, 0)
                       .build();

  RecordingSessionRegistry registry;
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Read-only modes succeed under Guest ReadOnlyScope with zero DB writes
  const auto before = ctx.db.total_changes();
  for (command_t cmd : {
           command_t{"order"},
           command_t{"technology"},
           command_t{"mobilize"},
           command_t{"route"},
           command_t{"read"},
           command_t{"read", "news"},
           command_t{"toggle"},
           command_t{"build", "?"},
           command_t{"stars"},
           command_t{"help"},
           command_t{"cs", "/Sol/Earth"},
       }) {
    g.out.clear();
    test::expect_true(
        GB::commands::dispatch_command(g, cmd),
        std::format("Guest read-only {} failed: {}", cmd[0], g.out.str()));
  }
  test::expect_eq(ctx.db.total_changes(), before);

  // 2. Mutating modes without .no_guests abort cleanly (ASCII & JSON),
  // truncating partial g.out headers and rolling back outbox & transactions
  const std::string ship_arg = std::format("#{}", sid);
  const std::array<command_t, 8> mutating_cmds{{
      {"order", ship_arg, "evade", "on"},
      {"technology", "500"},
      {"mobilize", "50"},
      {"toxicity", "50"},
      {"route", "1", "/Sol/Earth"},
      {"motto", "Hacked"},
      {"post", "Hacked"},
      {"examine", ship_arg},
  }};
  for (UiMode mode : {UiMode::ASCII, UiMode::JSON}) {
    g.set_ui_mode(mode);
    for (const auto& cmd : mutating_cmds) {
      g.out.clear();
      test::expect_false(GB::commands::dispatch_command(g, cmd));
      test::expect_contains(g.out.str(),
                            "Guest races cannot modify game state.");
      test::expect_false(g.out.str().contains("Ship Orders"));
    }
  }
  g.set_ui_mode(UiMode::ASCII);

  // Fixed-AP command + staged notification rolls back outbox and deducts 0 AP
  GB::commands::CommandDescriptor fixed_ap_cmd{
      .name = "guest_fixed_ap",
      .scopes = GB::commands::AllowedScopes::any(),
      .ap = GB::commands::APCost::fixed_star(2),
      .handler = [](const command_t&, GameObj& game) -> bool {
        game.session_registry.notify_race(2, "leaked");
        return true;
      },
  };
  g.out.clear();
  test::expect_false(
      GB::commands::dispatch_command(g, fixed_ap_cmd, {"guest_fixed_ap"}));
  test::expect_true(registry.notifications.empty());
  test::expect_eq(ctx.em.peek_star(1)->AP(1), 10U);
  test::expect_false(ctx.em.peek_ship(sid)->protect().evade);
  test::expect_eq(ctx.db.total_changes(), before);

  // 3. Deity override (!g.god()) bypasses Guest read-only enforcement
  g.set_god(true);
  test::expect_true(
      GB::commands::dispatch_command(g, fixed_ap_cmd, {"guest_fixed_ap"}));
  test::expect_eq(ctx.em.peek_star(1)->AP(1), 8U);

  // 4. Non-Guest internal ReadOnlyError rethrows fail-fast instead of masking
  g.set_god(false);
  ctx.em.mutate_race(1, [](Race& r) { r.Guest = false; });
  GB::commands::CommandDescriptor buggy_ro_cmd{
      .name = "buggy_ro",
      .scopes = GB::commands::AllowedScopes::any(),
      .handler = [](const command_t&, GameObj& game) -> bool {
        auto guard = game.entity_manager.create_read_only_scope();
        game.entity_manager.mutate_star(1, [](Star& s) { s.AP(1) = 99; });
        return true;
      },
  };
  test::expect_throws<ReadOnlyError>(
      [&] { GB::commands::dispatch_command(g, buggy_ro_cmd, {"buggy_ro"}); });
  test::expect_eq(ctx.em.peek_star(1)->AP(1), 8U);
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
  test_json_dirty_entity_auto_emission();
  test_guest_read_only_scope_enforcement();

  std::println(std::cout, "✓ dispatch_pipeline_test passed!");
  return 0;
}
