// SPDX-License-Identifier: Apache-2.0

/// \file server_command_test.cc
/// \brief Unit tests for server boundary privilege propagation, guest
/// restrictions, and fail-fast fatal crash logging.

import asio;
import commands;
import dallib;
import gb.entities;
import gb.log;
import gb.services;
import gb.server;
import server;
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
  ctx.setup_game_obj(g, 2, 1);
  g.set_god(false);
  ctx.assert_dispatch_rejected(g, no_guest_cmd, {"mock_no_guest"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");

  // 2. Normal non-guest race executes standard command successfully
  g.out.str("");
  ctx.setup_game_obj(g, 3, 1);
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

void test_output_buffer() {
  OutputBuffer buf;
  test::expect_true(buf.empty());
  test::expect_eq(buf.view(), "");
  test::expect_true(buf.spans().empty());

  buf << "Hello ";
  buf.append("world\n");
  buf.append_json("{\"type\":\"star\",\"data\":{}}\n");
  buf.append_json("{\"type\":\"planet\",\"data\":{}}\n");
  buf << "Tail\n";
  test::expect_false(buf.empty());
  test::expect_eq(buf.spans().size(), 3UZ);
  test::expect_false(buf.spans()[0].is_json);
  test::expect_true(buf.spans()[1].is_json);
  test::expect_false(buf.spans()[2].is_json);

  // Truncate across span boundaries trims trailing spans cleanly
  const std::size_t mid =
      std::string_view{"Hello world\n{\"type\":\"star\""}.size();
  buf.truncate(mid);
  test::expect_eq(buf.spans().size(), 2UZ);
  test::expect_true(buf.spans()[1].is_json);
  test::expect_eq(buf.span_view(buf.spans()[1]), "{\"type\":\"star\"");

  buf.str("Replaced");
  test::expect_eq(buf.view(), "Replaced");
  test::expect_eq(buf.spans().size(), 1UZ);
  test::expect_false(buf.spans()[0].is_json);

  buf.clear();
  test::expect_true(buf.empty());
  test::expect_eq(buf.str(), "");
  test::expect_true(buf.spans().empty());
}

void test_process_command_fail_fast_logging_and_rollback() {
  // 1. EntityNotFoundError at planet scope logs [FATAL], stops server, rethrows
  {
    TestContext ctx;
    ctx.with_races();

    asio::io_context io;
    Server server(io, 0, ctx.em);
    GameObj g(ctx.em, server);
    ctx.setup_game_obj(g, 2, 1);
    g.set_scope_planet(99,
                       1);  // Non-existent star triggers EntityNotFoundError

    std::ostringstream log_buf;
    GB::log::ScopedSink sink{log_buf};

    bool caught = false;
    try {
      server.process_command(g, {"production"});
    } catch (const EntityNotFoundError&) {
      caught = true;
    }

    test::expect_true(caught);
    test::expect_true(g.race == nullptr);
    test::expect_true(server.is_shutdown());
    test::expect_true(server.has_fatal_error());
    test::expect_true(io.stopped());
    test::expect_true(log_buf.str().starts_with("<2>"));
    test::expect_contains(
        log_buf.str(),
        "[FATAL] Unhandled exception in command execution: player=2 governor=1 "
        "scope=PLAN(snum=99,pnum=1) raw_command=\"production\":");
  }

  // 2. SqliteError (FK constraint failure) during command execution rolls back
  //    prior SQL writes, logs [FATAL], stops server, and propagates fail-fast
  {
    TestContext ctx;
    const auto races = ctx.with_races();
    ctx.with_populated_planet(1, 1, races.nomad, 100, Coordinates{1, 1});
    ctx.em.mutate_star(1, [&](Star& star) { star.AP(races.nomad) = 60; });

    const shipnum_t gov_ship = TestShipBuilder(ctx.em, ShipType::OTYPE_GOV)
                                   .owned_by(races.nomad, 1)
                                   .landed_on(1, 1, Coordinates{1, 1})
                                   .build();
    // Cache gov_ship in EntityManager, then delete its row from tbl_ship so
    // saving Race.Gov_ship triggers SQLITE_CONSTRAINT_FOREIGNKEY after Star AP
    // has already been written inside the active transaction.
    (void)ctx.em.peek_ship(gov_ship);
    test::expect_true(JsonStore(ctx.db).remove("tbl_ship", gov_ship));

    asio::io_context io;
    Server server(io, 0, ctx.em);
    GameObj g(ctx.em, server);
    ctx.setup_game_obj(g, races.nomad, 1);
    g.set_scope_planet(1, 1);

    std::ostringstream log_buf;
    GB::log::ScopedSink sink{log_buf};

    const std::string arg = std::format("#{}", gov_ship);
    const std::string raw = std::format("capital #{}", gov_ship);
    bool caught = false;
    try {
      server.process_command(g, {"capital", arg}, raw);
    } catch (const SqliteError&) {
      caught = true;
    }

    test::expect_true(caught);
    test::expect_true(server.is_shutdown());
    test::expect_true(server.has_fatal_error());
    test::expect_true(io.stopped());
    test::expect_contains(
        log_buf.str(),
        std::format(
            "[FATAL] Unhandled exception in command execution: player={} "
            "governor=1 scope=PLAN(snum=1,pnum=1) raw_command=\"{}\":",
            races.nomad.value, raw));
    test::expect_contains(log_buf.str(), "FOREIGN KEY");

    // Verify ReleaseGuard released race_refcount so Transaction::rollback()
    // evicted the dirty in-memory Race AND rolled back Star(1) AP in SQLite.
    test::expect_false(ctx.em.peek_race(races.nomad)->Gov_ship.has_value());
    ctx.em.clear_cache();
    test::expect_eq(ctx.em.peek_star(1)->AP(races.nomad), 60);
  }

  // 3. Scope context formatting covers UNIV, STAR, and SHIP scopes
  for (const ScopeLevel level : {ScopeLevel::LEVEL_UNIV, ScopeLevel::LEVEL_STAR,
                                 ScopeLevel::LEVEL_SHIP}) {
    TestContext ctx;
    ctx.with_races();
    const shipnum_t ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_POD)
                                  .owned_by(1, 1)
                                  .in_planet_orbit(1, 1)
                                  .build();

    asio::io_context io;
    Server server(io, 0, ctx.em);
    GameObj g(ctx.em, server);
    g.set_player(3);
    g.set_governor(2);
    if (level == ScopeLevel::LEVEL_UNIV) {
      g.set_scope_univ();
    } else if (level == ScopeLevel::LEVEL_STAR) {
      g.set_scope_star(4);
    } else {
      g.set_scope_ship(ship_id);
    }

    {
      Database closed = std::move(ctx.db);
    }
    ctx.em.clear_cache();

    std::ostringstream log_buf;
    GB::log::ScopedSink sink{log_buf};

    bool caught = false;
    try {
      server.process_command(g, {"cs", "/Sol"});
    } catch (const SqliteError&) {
      caught = true;
    }

    test::expect_true(caught);
    test::expect_contains(log_buf.str(), "raw_command=\"cs /Sol\"");
    if (level == ScopeLevel::LEVEL_UNIV) {
      test::expect_contains(log_buf.str(), "scope=UNIV");
    } else if (level == ScopeLevel::LEVEL_STAR) {
      test::expect_contains(log_buf.str(), "scope=STAR(snum=4)");
    } else {
      test::expect_contains(
          log_buf.str(),
          std::format("scope=SHIP(snum=1,pnum=1,shipno={})", ship_id));
    }
  }

  // 4. Scrapping the currently scoped ship reconciles scope before do_prompt
  {
    TestContext ctx;
    ctx.with_races();
    ctx.with_populated_planet(1, 1, 2, 500, Coordinates{2, 2});
    ctx.em.mutate_star(1, [](Star& star) {
      star.mark_explored_by(2);
      star.AP(2) = 20;
    });
    ctx.em.mutate_planet(1, 1, [](Planet& p) { p.mark_explored_by(2); });
    const shipnum_t fac_id = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY)
                                 .owned_by(2, 1)
                                 .landed_on(1, 1, Coordinates{2, 2})
                                 .with_crew(5, 0)
                                 .build();
    asio::io_context io;
    Server server(io, 0, ctx.em);
    GameObj g(ctx.em, server);
    ctx.setup_game_obj(g, 2, 1);
    g.set_scope_planet(1, 1);

    server.process_command(g, {"cs", std::format("#{}", fac_id)});
    test::expect_eq(g.level(), ScopeLevel::LEVEL_SHIP);
    g.out.clear();
    server.process_command(g, {"scrap", std::format("#{}", fac_id)});
    test::expect_false(server.has_fatal_error());
    test::expect_eq(g.level(), ScopeLevel::LEVEL_PLAN);
    test::expect_contains(g.out.str(), "/Sol/Earth");
    g.out.clear();
    server.process_command(g, {"ship", std::format("{}", fac_id)});
    test::expect_false(server.has_fatal_error());
    test::expect_contains(g.out.str(), "no such ship");

    // 5. Revoked or dissolved session skips illegal-command & JSON prompt
    // crashes, while an emulating Deity restores identity on both illegal
    // commands and mid-command target dissolution
    ctx.em.mutate_race(2, [](Race& r) { r.appoint_governor(2); });
    ctx.setup_game_obj(g, 1, 1);
    g.set_god(true);
    g.emulate(2, 2);
    ctx.em.mutate_race(2, [](Race& r) { (void)r.revoke_governor(2, 1); });
    g.out.clear();
    server.process_command(g, {"unknown_after_revoke"});
    test::expect_false(g.disconnect_requested());
    test::expect_eq(g.player(), 1);
    test::expect_true(g.god());
    test::expect_contains(g.out.str(), "Emulated session ended; restored");

    g.set_ui_mode(UiMode::JSON);
    ctx.em.mutate_race(2, [](Race& r) { r.appoint_governor(2); });
    ctx.setup_game_obj(g, 2, 2);
    g.set_god(false);
    ctx.em.mutate_race(2, [](Race& r) { (void)r.revoke_governor(2, 1); });
    g.out.clear();
    server.process_command(g, {"unknown_after_revoke"});
    test::expect_true(g.disconnect_requested());
    test::expect_true(g.out.str().empty());

    // Mid-command session invalidation suppresses post-command do_prompt for
    // mortals and restores native Deity identity when emulating
    constexpr GB::commands::CommandDescriptor self_dissolve_desc{
        .name = "time",
        .scopes = GB::commands::AllowedScopes::any(),
        .handler = [](const command_t&, GameObj& game) -> bool {
          game.entity_manager.mutate_race(game.player(),
                                          [](Race& r) { r.dissolved = true; });
          return true;
        },
    };
    g.set_disconnect_requested(false);
    g.set_ui_mode(UiMode::ASCII);
    ctx.setup_game_obj(g, 1, 1);
    g.set_god(true);
    g.emulate(2, 1);
    g.out.clear();
    test::expect_true(
        GB::commands::dispatch_command(g, self_dissolve_desc, {"time"}));
    test::expect_false(g.disconnect_requested());
    test::expect_eq(g.player(), 1);
    test::expect_true(g.god());
    test::expect_contains(g.out.str(), "Emulated session ended; restored");

    ctx.em.mutate_race(2, [](Race& r) { r.dissolved = false; });
    ctx.setup_game_obj(g, 2, 1);
    g.set_god(false);
    test::expect_true(
        GB::commands::dispatch_command(g, self_dissolve_desc, {"time"}));
    test::expect_true(g.disconnect_requested());
  }
}

}  // namespace

int main() {
  test_god_privilege_propagation();
  test_emulation_privilege_drop_and_guest_restrictions();
  test_server_boundary_dispatch_lifecycle();
  test_output_buffer();
  test_process_command_fail_fast_logging_and_rollback();

  std::println(std::cout, "✓ server_command_test passed!");
  return 0;
}
