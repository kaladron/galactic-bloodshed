// SPDX-License-Identifier: Apache-2.0

/// \file prompt_test.cc
/// \brief Comprehensive unit tests for player prompt formatting across all
/// scopes and nested orbit levels.

import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_prompt_universe_scope() {
  TestContext ctx;
  {
    JsonStore store(ctx.db);
    UniverseRepository universe_repo(store);
    universe_struct u{};
    u.id = 1;
    u.AP[player_t{1}] = 100;
    universe_repo.save(u);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{0});
  g.set_level(ScopeLevel::LEVEL_UNIV);

  std::string prompt = do_prompt(g);
  test::expect_eq(prompt, " ( [100] / )\n");
}

void test_prompt_star_scope() {
  TestContext ctx;
  {
    JsonStore store(ctx.db);
    StarRepository star_repo(store);
    star_struct sdata{};
    sdata.star_id = 1;
    sdata.name = "Sol";
    sdata.AP[0] = 50;
    Star star{sdata};
    star_repo.save(star);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{0});
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  std::string prompt = do_prompt(g);
  test::expect_eq(prompt, " ( [50] /Sol )\n");
}

void test_prompt_planet_scope() {
  TestContext ctx;
  {
    JsonStore store(ctx.db);
    StarRepository star_repo(store);
    star_struct sdata{};
    sdata.star_id = 1;
    sdata.name = "Sol";
    sdata.AP[0] = 50;
    sdata.pnames = {"Earth", "Mars"};
    Star star{sdata};
    star_repo.save(star);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{0});
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);

  std::string prompt = do_prompt(g);
  test::expect_eq(prompt, " ( [50] /Sol/Earth )\n");
}

void test_prompt_ship_orbiting_scopes() {
  TestContext ctx;
  {
    JsonStore store(ctx.db);
    UniverseRepository universe_repo(store);
    universe_struct u{};
    u.id = 1;
    u.AP[player_t{1}] = 100;
    universe_repo.save(u);

    StarRepository star_repo(store);
    star_struct sdata{};
    sdata.star_id = 1;
    sdata.name = "Sol";
    sdata.AP[0] = 50;
    sdata.pnames = {"Earth"};
    Star star{sdata};
    star_repo.save(star);

    ShipRepository ship_repo(store);

    // Ship 10: in universe scope
    ship_struct s10{};
    s10.number = 10;
    s10.whatorbits = ScopeLevel::LEVEL_UNIV;
    Ship ship10{s10};
    ship_repo.save(ship10);

    // Ship 11: orbiting star
    ship_struct s11{};
    s11.number = 11;
    s11.whatorbits = ScopeLevel::LEVEL_STAR;
    s11.storbits = 1;
    Ship ship11{s11};
    ship_repo.save(ship11);

    // Ship 12: orbiting planet
    ship_struct s12{};
    s12.number = 12;
    s12.whatorbits = ScopeLevel::LEVEL_PLAN;
    s12.storbits = 1;
    s12.pnumorbits = 0;
    Ship ship12{s12};
    ship_repo.save(ship12);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{0});
  g.set_level(ScopeLevel::LEVEL_SHIP);

  // Missing ship
  g.set_shipno(999);
  test::expect_eq(do_prompt(g), " ( [?] /#? )\n");

  // Ship in universe
  g.set_shipno(10);
  test::expect_eq(do_prompt(g), " ( [100] /#10 )\n");

  // Ship in star
  g.set_shipno(11);
  test::expect_eq(do_prompt(g), " ( [50] /Sol/#11 )\n");

  // Ship in planet
  g.set_shipno(12);
  g.set_pnum(0);
  test::expect_eq(do_prompt(g), " ( [50] /Sol/Earth/#12 )\n");
}

void test_prompt_nested_docked_ships() {
  TestContext ctx;
  {
    JsonStore store(ctx.db);
    UniverseRepository universe_repo(store);
    universe_struct u{};
    u.id = 1;
    u.AP[player_t{1}] = 100;
    universe_repo.save(u);

    StarRepository star_repo(store);
    star_struct sdata{};
    sdata.star_id = 1;
    sdata.name = "Sol";
    sdata.AP[0] = 50;
    sdata.pnames = {"Earth"};
    Star star{sdata};
    star_repo.save(star);

    ShipRepository ship_repo(store);

    // Level 2 nest: Carrier 21 (in universe) -> Fighter 20 (docked in 21)
    ship_struct s21{};
    s21.number = 21;
    s21.whatorbits = ScopeLevel::LEVEL_UNIV;
    Ship ship21{s21};
    ship_repo.save(ship21);

    ship_struct s20{};
    s20.number = 20;
    s20.whatorbits = ScopeLevel::LEVEL_SHIP;
    s20.destshipno = 21;
    Ship ship20{s20};
    ship_repo.save(ship20);

    // Level 2 nest: Carrier 23 (in star) -> Fighter 22 (docked in 23)
    ship_struct s23{};
    s23.number = 23;
    s23.whatorbits = ScopeLevel::LEVEL_STAR;
    s23.storbits = 1;
    Ship ship23{s23};
    ship_repo.save(ship23);

    ship_struct s22{};
    s22.number = 22;
    s22.whatorbits = ScopeLevel::LEVEL_SHIP;
    s22.destshipno = 23;
    s22.storbits = 1;
    Ship ship22{s22};
    ship_repo.save(ship22);

    // Level 2 nest: Carrier 25 (in planet) -> Fighter 24 (docked in 25)
    ship_struct s25{};
    s25.number = 25;
    s25.whatorbits = ScopeLevel::LEVEL_PLAN;
    s25.storbits = 1;
    s25.pnumorbits = 0;
    Ship ship25{s25};
    ship_repo.save(ship25);

    ship_struct s24{};
    s24.number = 24;
    s24.whatorbits = ScopeLevel::LEVEL_SHIP;
    s24.destshipno = 25;
    s24.storbits = 1;
    s24.pnumorbits = 0;
    Ship ship24{s24};
    ship_repo.save(ship24);

    // Level 2 nest with missing parent: Ship 26 -> non-existent 27
    ship_struct s26{};
    s26.number = 26;
    s26.whatorbits = ScopeLevel::LEVEL_SHIP;
    s26.destshipno = 27;
    Ship ship26{s26};
    ship_repo.save(ship26);

    // Level 3 nest: Station 32 (univ) -> Carrier 31 -> Fighter 30
    ship_struct s32{};
    s32.number = 32;
    s32.whatorbits = ScopeLevel::LEVEL_UNIV;
    Ship ship32{s32};
    ship_repo.save(ship32);

    ship_struct s31{};
    s31.number = 31;
    s31.whatorbits = ScopeLevel::LEVEL_SHIP;
    s31.destshipno = 32;
    Ship ship31{s31};
    ship_repo.save(ship31);

    ship_struct s30{};
    s30.number = 30;
    s30.whatorbits = ScopeLevel::LEVEL_SHIP;
    s30.destshipno = 31;
    Ship ship30{s30};
    ship_repo.save(ship30);

    // Level 3 nest: Station 35 (star) -> Carrier 34 -> Fighter 33
    ship_struct s35{};
    s35.number = 35;
    s35.whatorbits = ScopeLevel::LEVEL_STAR;
    s35.storbits = 1;
    Ship ship35{s35};
    ship_repo.save(ship35);

    ship_struct s34{};
    s34.number = 34;
    s34.whatorbits = ScopeLevel::LEVEL_SHIP;
    s34.destshipno = 35;
    s34.storbits = 1;
    Ship ship34{s34};
    ship_repo.save(ship34);

    ship_struct s33{};
    s33.number = 33;
    s33.whatorbits = ScopeLevel::LEVEL_SHIP;
    s33.destshipno = 34;
    s33.storbits = 1;
    Ship ship33{s33};
    ship_repo.save(ship33);

    // Level 3 nest: Station 38 (plan) -> Carrier 37 -> Fighter 36
    ship_struct s38{};
    s38.number = 38;
    s38.whatorbits = ScopeLevel::LEVEL_PLAN;
    s38.storbits = 1;
    s38.pnumorbits = 0;
    Ship ship38{s38};
    ship_repo.save(ship38);

    ship_struct s37{};
    s37.number = 37;
    s37.whatorbits = ScopeLevel::LEVEL_SHIP;
    s37.destshipno = 38;
    s37.storbits = 1;
    s37.pnumorbits = 0;
    Ship ship37{s37};
    ship_repo.save(ship37);

    ship_struct s36{};
    s36.number = 36;
    s36.whatorbits = ScopeLevel::LEVEL_SHIP;
    s36.destshipno = 37;
    s36.storbits = 1;
    s36.pnumorbits = 0;
    Ship ship36{s36};
    ship_repo.save(ship36);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{0});
  g.set_level(ScopeLevel::LEVEL_SHIP);

  // 2-level nested prompts
  g.set_shipno(20);
  test::expect_eq(do_prompt(g), " ( [100] /#21/#20 )\n");

  g.set_shipno(22);
  test::expect_eq(do_prompt(g), " ( [50] /Sol/#23/#22 )\n");

  g.set_shipno(24);
  g.set_pnum(0);
  test::expect_eq(do_prompt(g), " ( [50] /Sol/Earth/#25/#24 )\n");

  g.set_shipno(26);
  test::expect_eq(do_prompt(g), " ( [?] /#?/#? )\n");

  // 3-level nested prompts
  g.set_shipno(30);
  test::expect_eq(do_prompt(g), " ( [100] / /../#31/#30 )\n");

  g.set_shipno(33);
  test::expect_eq(do_prompt(g), " ( [50] /Sol/ /../#34/#33 )\n");

  g.set_shipno(36);
  g.set_pnum(0);
  test::expect_eq(do_prompt(g), " ( [50] /Sol/Earth/ /../#37/#36 )\n");
}

}  // namespace

int main() {
  test_prompt_universe_scope();
  test_prompt_star_scope();
  test_prompt_planet_scope();
  test_prompt_ship_orbiting_scopes();
  test_prompt_nested_docked_ships();

  std::println(std::cout, "✓ prompt_test passed!");
  return 0;
}
