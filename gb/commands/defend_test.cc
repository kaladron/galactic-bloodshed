// SPDX-License-Identifier: Apache-2.0

/// \file defend_test.cc
/// \brief Unit tests for defend command

import commands;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();

  ctx.em.mutate_sectormap(0, 0, [](SectorMap& smap) {
    auto& sect = smap.get(Coordinates{5, 5});
    sect.set_owner(1);
    sect.set_popn_exact(1000);
    sect.set_troops(500);
    sect.set_condition(SectorType::SEC_MOUNT);

    auto& sect_enemy = smap.get(Coordinates{0, 0});
    sect_enemy.set_owner(2);
    sect_enemy.set_popn_exact(200);
    sect_enemy.set_condition(SectorType::SEC_LAND);
  });

  // Setup planet info and sectors for player 1 defense
  ctx.em.mutate_planet(0, 0, [](Planet& planet) {
    planet.info(player_t{1}).numsectsowned = 5;
    planet.info(player_t{1}).guns = 50;
    planet.info(player_t{1}).destruct = 100;
    planet.info(player_t{1}).popn = 1000;
    planet.info(player_t{1}).troops = 500;
    planet.info(player_t{2}).popn = 200;
    planet.popn() = 1200;
    planet.troops() = 500;
  });

  // Create first attacking enemy ship in planet orbit
  TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY, 1)
      .owned_by(2, 0)
      .named("Factory")
      .in_planet_orbit(0, 0)
      .with_armor(100)
      .build();

  // Create second attacking enemy ship in planet orbit
  TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY, 2)
      .owned_by(2, 0)
      .named("Cargo")
      .in_planet_orbit(0, 0)
      .with_armor(100)
      .build();
}

void test_defend_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Defend with explicit strength (1 star AP deducted)
  ctx.assert_dispatch_success(g, {"defend", "1", "5,5", "25"}, 1);

  ctx.em.clear_cache();
  const auto* saved_planet = ctx.em.peek_planet(0, 0);
  test::expect_ne(saved_planet, nullptr);
  test::expect_lt(saved_planet->info(player_t{1}).destruct, 100);
  std::println(std::cout, "    ✓ Defend with explicit strength succeeded");

  // 2. Defend with default strength (omitted 4th argument, 1 star AP deducted)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"defend", "2", "5,5"}, 1);
  test::expect_contains(g.out.str(), "Cargo");
  std::println(std::cout, "    ✓ Defend with default strength succeeded");

  // 3. Firing on destroyed ship 1 is rejected
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"defend", "1", "5,5", "25"});
  test::expect_contains(g.out.str(), "That ship is already destroyed.");
  std::println(std::cout,
               "    ✓ Defend rejected against already destroyed ship");

  ctx.verify_universe_invariants();
}

void test_defend_retaliation_and_escort() {
  TestContext ctx;
  setup_test_world(ctx);

  // Target ship with guns, destruct, and self-retaliation enabled
  const shipnum_t target_id = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                                  .owned_by(2, 0)
                                  .named("Attacker")
                                  .in_planet_orbit(0, 0)
                                  .with_guns(guntype_t::HEAVY, 10)
                                  .with_destruct(100)
                                  .with_armor(50)
                                  .build();

  ctx.em.mutate_ship(target_id, [](Ship& s) { s.protect().self = true; });

  // Escort ship on planet protecting target_id
  const shipnum_t escort_id = TestShipBuilder(ctx.em, ShipType::STYPE_DESTROYER)
                                  .owned_by(2, 0)
                                  .named("Escort")
                                  .in_planet_orbit(0, 0)
                                  .with_guns(guntype_t::LIGHT, 5)
                                  .with_destruct(50)
                                  .build();

  ctx.em.mutate_ship(escort_id, [&](Ship& s) {
    s.protect().on = true;
    s.protect().ship = target_id;
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"defend", std::format("#{}", target_id.value), "5,5", "10"});
  std::string out = g.out.str();
  test::expect_contains(out, "Attacker");
  std::println(std::cout,
               "    ✓ Defend with target and escort retaliation verified");

  ctx.verify_universe_invariants();
}

void test_defend_target_scope_validations() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Ship in star orbit (not planet orbit)
  const shipnum_t star_ship = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                                  .owned_by(2, 0)
                                  .in_star_orbit(0)
                                  .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"defend", std::format("#{}", star_ship.value), "5,5", "10"});
  test::expect_contains(g.out.str(), "The ship is not in planet orbit.");

  // 2. Ship in orbit around different planet (Planet 1 instead of Planet 0)
  const shipnum_t other_planet_ship =
      TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
          .owned_by(2, 0)
          .in_planet_orbit(0, 1)
          .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"defend", std::format("#{}", other_planet_ship.value), "5,5", "10"});
  test::expect_contains(g.out.str(),
                        "Target is not in orbit around this planet.");

  // 3. Ship is landed on planet
  const shipnum_t landed_ship = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                                    .owned_by(2, 0)
                                    .landed_on(0, 0, Coordinates{0, 0})
                                    .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"defend", std::format("#{}", landed_ship.value), "5,5", "10"});
  test::expect_contains(g.out.str(), "Planet guns can't fire on landed ships.");

  // 4. Ship does not exist
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"defend", "#9999", "5,5", "10"});
  test::expect_contains(g.out.str(), "Ship not found.");

  std::println(std::cout, "    ✓ Target ship validation rejections verified");
}

void test_defend_planetary_validations() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Player owns no sectors
  ctx.em.mutate_planet(
      0, 0, [](Planet& p) { p.info(player_t{1}).numsectsowned = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"defend", "1", "5,5", "10"});
  test::expect_contains(g.out.str(), "You do not occupy any sectors here.");
  ctx.em.mutate_planet(
      0, 0, [](Planet& p) { p.info(player_t{1}).numsectsowned = 5; });

  // 2. Planet enslaved to another player
  ctx.em.mutate_planet(0, 0, [](Planet& p) { p.slaved_to() = player_t{2}; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"defend", "1", "5,5", "10"});
  test::expect_contains(g.out.str(), "This planet is enslaved.");
  ctx.em.mutate_planet(0, 0, [](Planet& p) { p.slaved_to() = player_t{0}; });

  // 3. Illegal sector coordinates (out of bounds)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"defend", "1", "99,99", "10"});
  test::expect_contains(g.out.str(), "Illegal sector.");

  // 4. Sector not owned by player (enemy sector 0,0)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"defend", "1", "0,0", "10"});
  test::expect_contains(g.out.str(), "Nice try.");

  // 5. Zero guns available
  ctx.em.mutate_planet(0, 0, [](Planet& p) { p.info(player_t{1}).guns = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"defend", "1", "5,5", "10"});
  test::expect_contains(g.out.str(), "No attack - 0 guns");

  std::println(std::cout,
               "    ✓ Planetary defense condition rejections verified");
}

void test_defend_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set AP to 0
  ctx.em.mutate_star(0, [](Star& s) { s.AP(player_t{1}) = 0; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  ctx.assert_dispatch_rejected(g, {"defend", "1", "5,5", "25"});
  test::expect_contains(g.out.str(), "action points");

  ctx.verify_universe_invariants();
}

void test_defend_role_and_scope_rejections() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Scope rejection (LEVEL_UNIV)
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"defend", "1", "5,5", "25"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");

  // 2. Star control rejection
  ctx.em.mutate_star(0, [](Star& s) {
    s.governor(player_t{1}) = 2;  // Star governed by Gov 2
  });
  ctx.setup_game_obj(g, 1, 1);  // Player 1, Gov 1
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);
  ctx.assert_dispatch_rejected(g, {"defend", "1", "5,5", "25"});
  test::expect_contains(g.out.str(), "not authorized");

  ctx.verify_universe_invariants();
}

void test_defend_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Min args (< 3 args)
  ctx.assert_dispatch_rejected(g, {"defend", "1"});
  test::expect_contains(g.out.str(),
                        "Syntax: defend <ship> <sector> [<strength>]");

  // 2. Bad ship number
  ctx.assert_dispatch_rejected(g, {"defend", "abc", "5,5"});
  test::expect_contains(g.out.str(), "Bad ship number");

  // 3. Bad sector format
  ctx.assert_dispatch_rejected(g, {"defend", "1", "bad_coords"});
  test::expect_contains(g.out.str(), "Bad format");

  // 4. Command matrix validation (roles, guests, governor, scopes)
  TestCommandMatrix(ctx, "defend")
      .with_valid_argv({"defend", "1", "5,5", "10"})
      .with_invalid_argv({"defend", "9999", "5,5", "10"})
      .with_valid_scope(ScopeLevel::LEVEL_PLAN)
      .with_expected_star_ap(1)
      .run_matrix(g);

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_defend_happy_path();
  test_defend_retaliation_and_escort();
  test_defend_target_scope_validations();
  test_defend_planetary_validations();
  test_defend_insufficient_ap();
  test_defend_role_and_scope_rejections();
  test_defend_domain_errors();

  std::println(std::cout, "\n✅ All defend tests passed!");
  return 0;
}
