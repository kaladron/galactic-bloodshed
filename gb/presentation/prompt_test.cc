// SPDX-License-Identifier: Apache-2.0

/// \file prompt_test.cc
/// \brief Comprehensive unit tests for player prompt formatting across all
/// scopes and nested orbit levels.

import dallib;
import gb.entities;
import gb.presentation.session;
import gb.services;
import test;
import std;

namespace {

using GB::presentation::build_session_view;
using GB::presentation::do_prompt;
using GB::presentation::format_ship_prompt;

void test_prompt_universe_scope() {
  TestContext ctx;
  {
    JsonStore store(ctx.db);
    UniverseRepository universe_repo(store);
    Universe u{};
    u.set_AP(player_t{1}, 100);
    universe_repo.save(u);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{1});
  g.set_scope_univ();

  std::string prompt = do_prompt(g);
  test::expect_eq(prompt, " ( [100] / )\n");
}

void test_prompt_star_scope() {
  TestContext ctx;
  ctx.create_star("Sol", 1).with_ap(player_t{1}, 50).build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{1});
  g.set_scope_star(1);

  std::string prompt = do_prompt(g);
  test::expect_eq(prompt, " ( [50] /Sol )\n");
}

void test_prompt_planet_scope() {
  TestContext ctx;
  ctx.create_star("Sol", 1)
      .with_ap(player_t{1}, 50)
      .with_planet_names({"Earth", "Mars"})
      .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{1});
  g.set_scope_planet(1, 1);

  std::string prompt = do_prompt(g);
  test::expect_eq(prompt, " ( [50] /Sol/Earth )\n");
}

void test_prompt_ship_orbiting_scopes() {
  TestContext ctx;
  ctx.create_star("Sol", 1)
      .with_ap(player_t{1}, 50)
      .with_planet_names({"Earth"})
      .build();
  {
    JsonStore store(ctx.db);
    PlanetRepository(store).save(Planet{1, 1});
    UniverseRepository universe_repo(store);
    Universe u{};
    u.set_AP(player_t{1}, 100);
    universe_repo.save(u);

    ShipRepository ship_repo(store);

    // Ship 10: in universe scope
    ship_repo.save(*TestShipBuilder(ShipType::STYPE_EXPLORER)
                        .numbered(10)
                        .owned_by(1)
                        .in_deep_space()
                        .build_in_memory());

    // Ship 11: orbiting star
    ship_repo.save(*TestShipBuilder(ShipType::STYPE_EXPLORER)
                        .numbered(11)
                        .owned_by(1)
                        .in_star_orbit(1)
                        .build_in_memory());

    // Ship 12: orbiting planet
    ship_repo.save(*TestShipBuilder(ShipType::STYPE_EXPLORER)
                        .numbered(12)
                        .owned_by(1)
                        .in_planet_orbit(1, 1)
                        .build_in_memory());
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{1});

  // Missing ship throws EntityNotFoundError on set_scope_ship and when ship is
  // deleted while scoped
  test::expect_throws<EntityNotFoundError>([&]() { g.set_scope_ship(999); });
  const shipnum_t doomed = TestShipBuilder(ctx.em, ShipType::STYPE_POD)
                               .owned_by(1, 1)
                               .in_deep_space()
                               .build();
  g.set_scope_ship(doomed);
  ctx.em.mutate_ship(doomed, [&](Ship& s) { ctx.em.destroy_ship(s); });
  test::expect_throws<EntityNotFoundError>(
      [&]() { test::expect_true(do_prompt(g).empty()); });

  // Ship in universe
  g.set_scope_ship(10);
  test::expect_eq(do_prompt(g), " ( [100] /#10 )\n");

  // Ship in star
  g.set_scope_ship(11);
  test::expect_eq(do_prompt(g), " ( [50] /Sol/#11 )\n");

  // Ship in planet
  g.set_scope_ship(12);
  test::expect_eq(do_prompt(g), " ( [50] /Sol/Earth/#12 )\n");
}

void test_prompt_nested_docked_ships() {
  TestContext ctx;
  ctx.create_star("Sol", 1)
      .with_ap(player_t{1}, 50)
      .with_planet_names({"Earth"})
      .build();
  {
    JsonStore store(ctx.db);
    PlanetRepository(store).save(Planet{1, 1});
    UniverseRepository universe_repo(store);
    Universe u{};
    u.set_AP(player_t{1}, 100);
    universe_repo.save(u);

    ShipRepository ship_repo(store);

    const auto save_carrier = [&](shipnum_t num,
                                  std::optional<starnum_t> star = std::nullopt,
                                  std::optional<planetnum_t> planet =
                                      std::nullopt) {
      auto builder =
          TestShipBuilder(ShipType::STYPE_CARRIER).numbered(num).owned_by(1);
      if (planet) {
        builder.in_planet_orbit(*star, *planet);
      } else if (star) {
        builder.in_star_orbit(*star);
      } else {
        builder.in_deep_space();
      }
      ship_repo.save(*builder.build_in_memory());
    };

    const auto save_docked = [&](shipnum_t num, shipnum_t carrier,
                                 std::optional<starnum_t> star = std::nullopt,
                                 std::optional<planetnum_t> planet =
                                     std::nullopt) {
      auto builder =
          TestShipBuilder(ShipType::STYPE_FIGHTER).numbered(num).owned_by(1);
      if (planet) {
        builder.in_planet_orbit(*star, *planet);
      } else if (star) {
        builder.in_star_orbit(*star);
      } else {
        builder.in_deep_space();
      }
      auto ship = builder.build_in_memory();
      ship->dock_into_carrier(carrier);
      ship_repo.save(*ship);
    };

    // Level 2 nest: Carrier 21 (in universe) -> Fighter 20 (docked in 21)
    save_carrier(21);
    save_docked(20, 21);

    // Level 2 nest: Carrier 23 (in star) -> Fighter 22 (docked in 23)
    save_carrier(23, 1);
    save_docked(22, 23, 1);

    // Level 2 nest: Carrier 25 (in planet) -> Fighter 24 (docked in 25)
    save_carrier(25, 1, 1);
    save_docked(24, 25, 1, 1);

    // Level 3 nest: Station 32 (univ) -> Carrier 31 -> Fighter 30
    save_carrier(32);
    save_docked(31, 32);
    save_docked(30, 31);

    // Level 3 nest: Station 35 (star) -> Carrier 34 -> Fighter 33
    save_carrier(35, 1);
    save_docked(34, 35, 1);
    save_docked(33, 34, 1);

    // Level 3 nest: Station 38 (plan) -> Carrier 37 -> Fighter 36
    save_carrier(38, 1, 1);
    save_docked(37, 38, 1, 1);
    save_docked(36, 37, 1, 1);

    // Level 4 nest: Station 44 (plan) -> Carrier 43 -> Cruiser 42 -> Fighter 41
    // -> Drone 40
    save_carrier(44, 1, 1);
    save_docked(43, 44, 1, 1);
    save_docked(42, 43, 1, 1);
    save_docked(41, 42, 1, 1);
    save_docked(40, 41, 1, 1);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{1});

  // 2-level nested prompts
  g.set_scope_ship(20);
  test::expect_eq(do_prompt(g), " ( [100] /#21/#20 )\n");

  g.set_scope_ship(22);
  test::expect_eq(do_prompt(g), " ( [50] /Sol/#23/#22 )\n");

  g.set_scope_ship(24);
  test::expect_eq(do_prompt(g), " ( [50] /Sol/Earth/#25/#24 )\n");

  // 3-level nested prompts
  g.set_scope_ship(30);
  test::expect_eq(do_prompt(g), " ( [100] /#32/#31/#30 )\n");

  g.set_scope_ship(33);
  test::expect_eq(do_prompt(g), " ( [50] /Sol/#35/#34/#33 )\n");

  g.set_scope_ship(36);
  test::expect_eq(do_prompt(g), " ( [50] /Sol/Earth/#38/#37/#36 )\n");

  // 4-level nested prompt (arbitrary depth)
  g.set_scope_ship(40);
  test::expect_eq(do_prompt(g), " ( [50] /Sol/Earth/#44/#43/#42/#41/#40 )\n");

  // Direct format_ship_prompt testing
  test::expect_eq(format_ship_prompt(ctx.em, player_t{1}, 30),
                  " ( [100] /#32/#31/#30 )\n");
  test::expect_eq(format_ship_prompt(ctx.em, player_t{1}, 40),
                  " ( [50] /Sol/Earth/#44/#43/#42/#41/#40 )\n");
}

void test_prompt_json_mode() {
  TestContext ctx;
  const auto races = ctx.with_races();

  ctx.em.mutate_universe(
      [&](Universe& u) { u.set_AP(races.empire, ap_t{42}); });
  ctx.em.mutate_star(1, [&](Star& star) { star.AP(races.empire) = ap_t{17}; });
  ctx.em.mutate_server_state([](ServerState& state) {
    state.update_time_minutes = 45;
    state.segments = 3;
    state.nsegments_done = 2;
    state.nupdates_done = 7;
    state.record_server_start(1700000000);
    state.record_update_completed(1700003600, false);
    state.record_segment_completed(1700005400);
    state.next_segment_time = 1700007200;
    state.next_update_time = 1700009000;
  });

  const auto ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE)
                           .owned_by(races.empire, 1)
                           .in_planet_orbit(1, 1)
                           .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, races.empire, governor_t{1});
  g.set_ui_mode(UiMode::JSON);

  g.set_scope_univ();
  std::string out = do_prompt(g);
  test::expect_contains(out, "\"type\":\"session\"");
  test::expect_contains(out, "\"ap\":42");
  test::expect_contains(out, "\"global_ap\":42");
  test::expect_contains(out, "\"race_name\":\"Imperium\"");
  test::expect_contains(out, "\"scope\":");
  test::expect_contains(out, "\"viewport\":");
  test::expect_contains(out, "\"toggles\":");
  test::expect_contains(out, "\"level\":0");
  test::expect_contains(out, "\"path\":\"/\"");
  test::expect_contains(out, "\"server_start_time\":1700000000");
  test::expect_contains(out, "\"last_update_time\":1700003600");
  test::expect_contains(out, "\"last_segment_time\":1700005400");
  test::expect_contains(out, "\"update_interval_minutes\":45");
  test::expect_contains(out, "\"segments_per_update\":3");
  test::expect_contains(out, "\"segments_done\":2");
  test::expect_contains(out, "\"current_segment\":3");
  test::expect_contains(out, "\"updates_done\":7");
  test::expect_contains(out, "\"next_update_number\":8");

  // Verify current_segment wraps to 1 when segments_done == segments
  ctx.em.mutate_server_state(
      [](ServerState& state) { state.nsegments_done = state.segments; });
  test::expect_contains(do_prompt(g), "\"current_segment\":1");

  g.set_scope_star(1);
  out = do_prompt(g);
  test::expect_contains(out, "\"type\":\"session\"");
  test::expect_contains(out, "\"ap\":17");
  test::expect_contains(out, "\"global_ap\":42");
  test::expect_contains(out, "\"level\":1");
  test::expect_contains(out, "\"star\":\"Sol\"");
  test::expect_contains(out, "\"path\":\"/Sol\"");

  g.set_scope_planet(1, 1);
  out = do_prompt(g);
  test::expect_contains(out, "\"type\":\"session\"");
  test::expect_contains(out, "\"level\":2");
  test::expect_contains(out, "\"star\":\"Sol\"");
  test::expect_contains(out, "\"planet\":\"Earth\"");
  test::expect_contains(out, "\"path\":\"/Sol/Earth\"");

  g.set_scope_ship(ship_id);
  out = do_prompt(g);
  test::expect_contains(out, "\"type\":\"session\"");
  test::expect_contains(out, "\"level\":3");
  test::expect_contains(out, std::format("\"ship\":\"#{}\"", ship_id));
  test::expect_contains(out,
                        std::format("\"path\":\"/Sol/Earth/#{}\"", ship_id));

  // Exercise build_session_view and do_prompt when g.race is nullptr
  g.race = nullptr;
  out = do_prompt(g);
  test::expect_contains(out, "\"type\":\"session\"");
  test::expect_contains(out, "\"race_name\":\"\"");
  test::expect_contains(out, "\"governor_name\":\"\"");
  const auto session_view = build_session_view(g);
  test::expect_eq(session_view.race_name, "");
  test::expect_eq(session_view.governor_name, "");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_prompt_universe_scope();
  test_prompt_star_scope();
  test_prompt_planet_scope();
  test_prompt_ship_orbiting_scopes();
  test_prompt_nested_docked_ships();
  test_prompt_json_mode();

  std::println(std::cout, "✓ prompt_test passed!");
  return 0;
}
