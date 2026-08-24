// SPDX-License-Identifier: Apache-2.0

/// \file test_context_test.cc
/// \brief Unit tests for TestContext and test expectation assertion utilities.

import commands;
import dallib;
import gblib;
import test;
import std;

namespace {

bool mock_success_cmd(const command_t&, GameObj& g) {
  g.out << "mock success\n";
  return true;
}

bool mock_failure_cmd(const command_t&, GameObj& g) {
  g.out << "mock failure\n";
  return false;
}

void test_expectation_utilities() {
  std::println(std::cout,
               "Test: Modern expectation and diagnostic assertion utilities");

  // 1. Basic equality & inequality
  test::expect_eq(42, 42, "Integer equality");
  test::expect_eq(std::string("hello"), std::string("hello"),
                  "String equality");
  test::expect_ne(10, 20, "Integer inequality");

  // 2. Strong ID type comparisons (formats seamlessly)
  player_t p1{1};
  player_t p2{1};
  player_t p3{2};
  test::expect_eq(p1, p2, "Strong ID equality");
  test::expect_ne(p1, p3, "Strong ID inequality");

  starnum_t s1{0};
  test::expect_eq(s1, starnum_t{0}, "Starnum equality");

  // 3. Relational comparisons (ge, le, gt, lt)
  test::expect_ge(10, 10, "Greater or equal (equal case)");
  test::expect_ge(15, 10, "Greater or equal (greater case)");
  test::expect_le(10, 10, "Less or equal (equal case)");
  test::expect_le(5, 10, "Less or equal (less case)");
  test::expect_gt(15, 10, "Strictly greater");
  test::expect_lt(5, 10, "Strictly less");

  // 4. Boolean assertions
  test::expect_true(true, "True condition");
  test::expect_true(1 + 1 == 2, "Expression true");
  test::expect_false(false, "False condition");
  test::expect_false(1 + 1 == 3, "Expression false");

  // 5. String contains
  std::string output = "Sector 5,5 colonized by player 1 (Federation)";
  test::expect_contains(output, "colonized by player 1", "Sub-string matching");
  test::expect_contains(output, "Federation");

  // 6. Exception expectations
  test::expect_throws<std::runtime_error>(
      []() { throw std::runtime_error("Simulated domain error"); },
      "Expected std::runtime_error");

  test::expect_throws<EntityNotFoundError>(
      []() { throw EntityNotFoundError("Race not found: player=999"); },
      "Expected EntityNotFoundError");

  test::expect_no_throw(
      []() {
        int x = 10 + 20;
        (void)x;
      },
      "Expected safe lambda without exception");

  std::println(std::cout,
               "  ✓ All expectation utilities verified successfully");
}

void test_test_context_dispatch_helpers() {
  std::println(std::cout,
               "Test: TestContext dispatch and AP accounting helpers");
  TestContext ctx;

  // Setup Star 1 with 20 AP
  {
    JsonStore store(ctx.db);
    StarRepository star_repo(store);
    star_struct sdata{};
    sdata.star_id = 1;
    sdata.AP[0] = 20;
    Star star{sdata};
    star_repo.save(star);
  }

  // Setup Universe with 30 AP
  {
    JsonStore store(ctx.db);
    UniverseRepository universe_repo(store);
    universe_struct u{};
    u.id = 1;
    u.AP[0] = 30;
    universe_repo.save(u);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{0});
  g.set_snum(1);

  GB::commands::CommandDescriptor star_cost_cmd{
      .name = "star_cost",
      .scopes = GB::commands::AllowedScopes::any(),
      .ap = GB::commands::APCost::fixed_star(5),
      .handler = &mock_success_cmd,
  };

  GB::commands::CommandDescriptor univ_cost_cmd{
      .name = "univ_cost",
      .scopes = GB::commands::AllowedScopes::any(),
      .ap = GB::commands::APCost::fixed_univ(10),
      .handler = &mock_success_cmd,
  };

  GB::commands::CommandDescriptor fail_cmd{
      .name = "fail_cmd",
      .scopes = GB::commands::AllowedScopes::any(),
      .ap = GB::commands::APCost::fixed_star(5),
      .handler = &mock_failure_cmd,
  };

  // 1. Success dispatch with star AP verification (20 -> 15)
  ctx.assert_dispatch_success(g, star_cost_cmd, {"star_cost"},
                              /*expected_star_ap_deducted=*/5);
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 15);
  test::expect_contains(g.out.str(), "mock success");

  // 2. Success dispatch with universe AP verification (30 -> 20)
  ctx.assert_dispatch_success(g, univ_cost_cmd, {"univ_cost"},
                              /*expected_star_ap_deducted=*/0,
                              /*expected_univ_ap_deducted=*/10);
  test::expect_eq(ctx.em.peek_universe()->AP[0], 20);

  // 3. Rejected dispatch due to handler returning false (AP remains 15)
  ctx.assert_dispatch_rejected(g, fail_cmd, {"fail_cmd"});
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 15);

  // 4. Rejected dispatch due to insufficient AP (have 15, need 50 -> remains
  // 15)
  GB::commands::CommandDescriptor expensive_cmd{
      .name = "expensive_cmd",
      .scopes = GB::commands::AllowedScopes::any(),
      .ap = GB::commands::APCost::fixed_star(50),
      .handler = &mock_success_cmd,
  };
  ctx.assert_dispatch_rejected(g, expensive_cmd, {"expensive_cmd"});
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 15);
}

void test_test_world_builder() {
  std::println(std::cout, "Test: TestWorldBuilder fixture creation");
  TestContext ctx;

  // Build standard solar system (2 races, 1 star, 1 planet)
  TestWorldBuilder::create_standard_solar_system(ctx);

  // 1. Verify auto-assigned Player 1 & 2 races
  const auto* r1 = ctx.em.peek_race(1);
  test::expect_true(r1 != nullptr, "Race 1 must exist");
  test::expect_eq(r1->name, "Federation");
  test::expect_eq(r1->Playernum, player_t{1});
  test::expect_eq(r1->tech, 100.0);
  test::expect_false(r1->Guest);

  const auto* r2 = ctx.em.peek_race(2);
  test::expect_true(r2 != nullptr, "Race 2 must exist");
  test::expect_eq(r2->name, "Klingons");
  test::expect_eq(r2->Playernum, player_t{2});

  // 2. Verify Star 0 auto-exploration and AP
  const auto* star = ctx.em.peek_star(0);
  test::expect_true(star != nullptr, "Star 0 must exist");
  test::expect_eq(star->get_name(), "Sol");
  test::expect_true(isset(star->explored(), player_t{1}),
                    "Player 1 explored Star 0");
  test::expect_true(isset(star->explored(), player_t{2}),
                    "Player 2 explored Star 0");
  test::expect_eq(star->AP(player_t{1}), 100);
  test::expect_eq(star->AP(player_t{2}), 100);

  // 3. Verify Planet 0,0 auto-exploration and SectorMap
  const auto* planet = ctx.em.peek_planet(0, 0);
  test::expect_true(planet != nullptr, "Planet /0/0 must exist");
  test::expect_eq(planet->type(), PlanetType::EARTH);
  test::expect_eq(planet->info(player_t{1}).explored, 1);
  test::expect_eq(planet->info(player_t{2}).explored, 1);
  test::expect_eq(planet->info(player_t{1}).destruct, 1000);

  const auto* smap = ctx.em.peek_sectormap(0, 0);
  test::expect_true(smap != nullptr, "SectorMap for /0/0 must exist");
  test::expect_eq(smap->get(0, 0).get_x(), 0);
  test::expect_eq(smap->get(0, 0).get_y(), 0);

  std::println(std::cout, "  ✓ TestWorldBuilder verified successfully");
}

void test_test_ship_builder() {
  std::println(std::cout,
               "Test: TestShipBuilder canonical baseline construction");
  TestContext ctx;
  TestWorldBuilder::create_standard_solar_system(ctx);

  // 1. Battleship built with Shipdata template defaults and star orbit
  shipnum_t bb_num = TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE)
                         .owned_by(1)
                         .named("USS Enterprise")
                         .in_star_orbit(0, 10.0, 20.0)
                         .build();

  test::expect_eq(bb_num, shipnum_t{1});
  const auto* bb = ctx.em.peek_ship(bb_num);
  test::expect_true(bb != nullptr, "Battleship must exist");
  test::expect_eq(bb->name(), "USS Enterprise");
  test::expect_eq(bb->type(), ShipType::STYPE_BATTLE);
  test::expect_eq(bb->owner(), player_t{1});
  test::expect_eq(
      bb->armor(),
      static_cast<unsigned char>(Shipdata[ShipType::STYPE_BATTLE][ABIL_ARMOR]));
  test::expect_eq(bb->max_crew(),
                  static_cast<unsigned short>(
                      Shipdata[ShipType::STYPE_BATTLE][ABIL_MAXCREW]));
  test::expect_eq(bb->max_fuel(),
                  static_cast<unsigned short>(
                      Shipdata[ShipType::STYPE_BATTLE][ABIL_FUELCAP]));
  test::expect_eq(
      bb->fuel(),
      static_cast<double>(Shipdata[ShipType::STYPE_BATTLE][ABIL_FUELCAP]));
  test::expect_eq(bb->whatorbits(), ScopeLevel::LEVEL_STAR);
  test::expect_eq(bb->storbits(), starnum_t{0});
  test::expect_eq(bb->xpos(), 10.0);
  test::expect_eq(bb->ypos(), 20.0);
  test::expect_false(bb->docked());

  // 2. Landed transport with customized crew, resource, and damage
  Coordinates land_loc{3, 4};
  shipnum_t lander_num = TestShipBuilder(ctx.em, ShipType::STYPE_LANDER)
                             .owned_by(2)
                             .named("Bird of Prey")
                             .landed_on(0, 0, land_loc)
                             .with_crew(100, 50)
                             .with_resource(500)
                             .with_damage(15)
                             .with_cew(20, 1500)
                             .build();

  test::expect_eq(lander_num, shipnum_t{2});
  const auto* lander = ctx.em.peek_ship(lander_num);
  test::expect_true(lander != nullptr, "Lander must exist");
  test::expect_eq(lander->name(), "Bird of Prey");
  test::expect_eq(lander->whatorbits(), ScopeLevel::LEVEL_PLAN);
  test::expect_eq(lander->storbits(), starnum_t{0});
  test::expect_eq(lander->pnumorbits(), planetnum_t{0});
  test::expect_true(lander->docked());
  test::expect_eq(lander->land_coords().x, 3);
  test::expect_eq(lander->land_coords().y, 4);
  test::expect_eq(lander->popn(), 100);
  test::expect_eq(lander->troops(), 50);
  test::expect_eq(lander->resource(), 500);
  test::expect_eq(lander->damage(), 15);
  test::expect_eq(lander->cew(), 20);
  test::expect_eq(lander->cew_range(), 1500);

  // 3. Docked ship attached to parent ship
  shipnum_t fighter_num = TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
                              .owned_by(1)
                              .docked_to(bb_num, 0)
                              .build();

  test::expect_eq(fighter_num, shipnum_t{3});
  const auto* fighter = ctx.em.peek_ship(fighter_num);
  test::expect_true(fighter != nullptr, "Fighter must exist");
  test::expect_eq(fighter->whatorbits(), ScopeLevel::LEVEL_SHIP);
  test::expect_eq(fighter->destshipno(), bb_num);
  test::expect_true(fighter->docked());

  std::println(std::cout, "  ✓ TestShipBuilder verified successfully");
}

void test_recording_session_registry() {
  std::println(
      std::cout,
      "Test: RecordingSessionRegistry session and notification recording");
  RecordingSessionRegistry registry;

  // 1. Initial state
  test::expect_false(registry.update_in_progress());
  test::expect_true(registry.get_connected_sessions().empty());
  test::expect_true(registry.notifications.empty());

  // 2. Notification recording
  registry.notify_race(player_t{1}, "Planetary invasion detected!");
  registry.notify_player(player_t{2}, governor_t{0},
                         "Your treasury balance is low.");

  test::expect_eq(registry.notifications.size(), 2);
  test::expect_true(registry.has_received(player_t{1}, "Planetary invasion"));
  test::expect_true(registry.has_broadcast("invasion detected"));
  test::expect_true(registry.has_received(player_t{2}, "treasury balance"));
  test::expect_false(registry.has_received(player_t{3}, "Planetary invasion"));

  auto p1_msgs = registry.messages_for(player_t{1});
  test::expect_eq(p1_msgs.size(), 1);
  test::expect_eq(p1_msgs[0], "Planetary invasion detected!");

  registry.clear_notifications();
  test::expect_true(registry.notifications.empty());
  test::expect_false(registry.has_received(player_t{1}, "Planetary invasion"));

  // 3. Connected session queries
  registry.sessions = {
      SessionInfo{.player = 1, .governor = 0, .connected = true},
      SessionInfo{.player = 2, .governor = 1, .connected = false},
  };

  test::expect_true(registry.is_connected(player_t{1}, governor_t{0}));
  test::expect_false(registry.is_connected(player_t{2}, governor_t{1}));
  test::expect_false(registry.is_connected(player_t{3}, governor_t{0}));
  test::expect_eq(registry.get_connected_sessions().size(), 2);

  // 4. Update in progress flag
  registry.set_update_in_progress(true);
  test::expect_true(registry.update_in_progress());

  std::println(std::cout, "  ✓ RecordingSessionRegistry verified successfully");
}

void test_test_command_matrix() {
  std::println(std::cout, "Test: TestCommandMatrix 4-way runner");
  TestContext ctx;
  TestWorldBuilder::create_standard_solar_system(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{0});
  g.set_snum(0);
  g.set_pnum(0);

  static auto matrix_cmd_handler = [](const command_t& argv, GameObj& g) {
    if (argv.size() > 1 && argv[1] == "bad") {
      g.out << "Domain error occurred\n";
      return false;
    }
    g.out << "Command succeeded\n";
    return true;
  };

  GB::commands::CommandDescriptor matrix_cmd{
      .name = "matrix_test_cmd",
      .roles = {.no_guests = true},
      .scopes = GB::commands::AllowedScopes::planet_only(),
      .ap = GB::commands::APCost::fixed_star(5),
      .min_args = 2,
      .handler = matrix_cmd_handler,
  };

  TestCommandMatrix(ctx, matrix_cmd)
      .with_valid_argv({"matrix_test_cmd", "good"})
      .with_invalid_argv({"matrix_test_cmd", "bad"})
      .with_valid_scope(ScopeLevel::LEVEL_PLAN)
      .with_invalid_scopes({ScopeLevel::LEVEL_UNIV, ScopeLevel::LEVEL_STAR,
                            ScopeLevel::LEVEL_SHIP})
      .with_expected_star_ap(5)
      .run_matrix(g);

  // Star AP was originally 100, deducted 5 on happy path
  test::expect_eq(ctx.em.peek_star(0)->AP(player_t{1}), 95);
  std::println(std::cout, "  ✓ TestCommandMatrix verified successfully");
}

void test_universe_invariants() {
  std::println(std::cout, "Test: verify_universe_invariants integrity checker");
  TestContext ctx;
  TestWorldBuilder::create_standard_solar_system(ctx);

  // Add a ship using TestShipBuilder
  TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
      .owned_by(player_t{1}, governor_t{0})
      .named("Enterprise")
      .in_star_orbit(starnum_t{0}, 0.0, 0.0)
      .build();

  // Add a commodity
  Commod commod{};
  commod.id = 1;
  commod.owner = player_t{1};
  commod.amount = 500;
  JsonStore store(ctx.db);
  CommodRepository(store).save(commod);

  // Standard world satisfies all invariants
  test::expect_no_throw([&]() { ctx.verify_universe_invariants(); },
                        "Standard test world must satisfy universe invariants");

  // Verify that population in planet matches sectors
  {
    auto planet_handle = ctx.em.get_planet(starnum_t{0}, planetnum_t{0});
    planet_handle->popn() = 1234;
    auto smap_handle = ctx.em.get_sectormap(starnum_t{0}, planetnum_t{0});
    smap_handle->get(0, 0).set_popn_exact(1234);
  }
  test::expect_no_throw(
      [&]() { ctx.verify_universe_invariants(); },
      "Aligned planet and sector populations must satisfy invariants");

  std::println(std::cout,
               "  ✓ verify_universe_invariants verified successfully");
}

}  // namespace

int main() {
  test_expectation_utilities();
  test_test_context_dispatch_helpers();
  test_test_world_builder();
  test_test_ship_builder();
  test_recording_session_registry();
  test_test_command_matrix();
  test_universe_invariants();
  std::println(std::cout, "✓ test_context_test passed!");
  return 0;
}
