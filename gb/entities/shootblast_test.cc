// SPDX-License-Identifier: Apache-2.0

/// \file shootblast_test.cc
/// \brief Unit tests for shoot_planet_to_ship and shoot_ship_to_planet.

import gb.entities;
import gb.services;
import gb.turn;
import test;
import std;

void test_shoot_planet_to_ship_invalid_cases() {
  std::println(std::cout, "Test: shoot_planet_to_ship invalid cases");

  TestContext ctx;

  Race race{};
  race.Playernum = player_t{1};
  race.tech = 100.0;

  Ship ship{};
  ship.number() = 1;
  ship.owner() = player_t{2};
  ship.enter_planet_orbit(1, 1);
  ship.alive() = true;

  // Test 1: Zero strength -> returns std::nullopt
  auto dam1 = shoot_planet_to_ship(ctx.em, race, ship, 0);
  test::expect_false(dam1.has_value());

  // Test 2: Dead ship -> returns std::nullopt
  ship.alive() = false;
  auto dam2 = shoot_planet_to_ship(ctx.em, race, ship, 10);
  test::expect_false(dam2.has_value());

  // Test 3: Wrong orbit level -> returns std::nullopt
  ship.alive() = true;
  ship.enter_star_orbit(1);
  auto dam3 = shoot_planet_to_ship(ctx.em, race, ship, 10);
  test::expect_false(dam3.has_value());

  std::println(std::cout, "  ✓ shoot_planet_to_ship invalid cases passed");
}

void test_shoot_planet_to_ship_valid_attack() {
  std::println(std::cout, "Test: shoot_planet_to_ship valid attack");

  TestContext ctx;
  ctx.with_standard_universe();

  // Create a target ship in planet scope
  Ship ship{};
  ship.number() = 1;
  ship.owner() = player_t{2};
  ship.type() = ShipType::OTYPE_CANIST;
  ship.enter_planet_orbit(1, 1);
  ship.alive() = true;
  ship.on() = true;
  ship.tech() = 10.0;
  ship.size() = 10;
  ship.max_crew() = 10;
  ship.set_mass(10.0);
  ship.armor() = 5;

  const auto* race1 = ctx.em.peek_race(player_t{1});
  auto res = shoot_planet_to_ship(ctx.em, *race1, ship, 20);
  test::expect_true(res.has_value());
  test::expect_ge(res->damage, 0);
  test::expect_eq(res->attacker_kind, ShipShotAttackerKind::Planet);
  test::expect_eq(res->weapon, ShipShotWeaponKind::MediumGuns);
  test::expect_eq(res->attacker_player, player_t{1});
  test::expect_false(res->target_location_display.empty());
  test::expect_false(res->target_display.empty());

  std::println(std::cout,
               "  ✓ shoot_planet_to_ship valid attack passed (damage={})",
               res->damage);
}

void test_shoot_ship_to_planet_invalid_cases() {
  std::println(std::cout, "Test: shoot_ship_to_planet invalid cases");

  TestContext ctx;

  Planet planet{1, 1, PlanetType::EARTH, Coordinates{5, 5}};

  SectorMap smap(planet);

  Ship ship{};
  ship.number() = 1;
  ship.owner() = player_t{1};
  ship.enter_planet_orbit(1, 1);
  ship.alive() = true;
  ship.on() = true;

  // Test 1: Zero strength -> returns std::nullopt
  auto res1 = shoot_ship_to_planet(ctx.em, ship, planet, 0, Coordinates{0, 0},
                                   smap, 0, guntype_t::NONE);
  test::expect_false(res1.has_value());

  // Test 2: Dead ship -> returns std::nullopt
  ship.alive() = false;
  auto res2 = shoot_ship_to_planet(ctx.em, ship, planet, 10, Coordinates{0, 0},
                                   smap, 0, guntype_t::NONE);
  test::expect_false(res2.has_value());

  // Test 3: Invalid planet coords -> returns std::nullopt
  ship.alive() = true;
  auto res3 = shoot_ship_to_planet(
      ctx.em, ship, planet, 10, Coordinates{10, 10}, smap, 0, guntype_t::NONE);
  test::expect_false(res3.has_value());

  // Test 4: Non-planet orbit -> returns std::nullopt
  ship.enter_star_orbit(1);
  auto res4 = shoot_ship_to_planet(ctx.em, ship, planet, 10, Coordinates{0, 0},
                                   smap, 0, guntype_t::HEAVY);
  test::expect_false(res4.has_value());

  // Test 5: Ship with switch turned off and ignore=0 -> returns std::nullopt
  ship.enter_planet_orbit(1, 1);
  ship.type() = ShipType::STYPE_MINE;
  ship.on() = false;
  auto res5 = shoot_ship_to_planet(ctx.em, ship, planet, 10, Coordinates{0, 0},
                                   smap, 0, guntype_t::HEAVY);
  test::expect_false(res5.has_value());

  std::println(std::cout, "  ✓ shoot_ship_to_planet invalid cases passed");
}

void test_shoot_ship_to_planet_valid_attack() {
  std::println(std::cout, "Test: shoot_ship_to_planet valid attack");

  TestContext ctx;
  ctx.with_standard_universe();

  Planet planet(ctx.em.peek_planet(1, 1)->get_struct());

  SectorMap smap(planet);
  auto& s = smap.get(Coordinates{1, 1});
  s.set_owner(player_t{2});
  s.set_popn_exact(100);
  s.set_condition(SectorType::SEC_LAND);
  s.set_type(SectorType::SEC_LAND);

  Ship ship{};
  ship.number() = 1;
  ship.owner() = player_t{1};
  ship.type() = ShipType::OTYPE_CANIST;
  ship.enter_planet_orbit(1, 1);
  ship.alive() = true;
  ship.on() = true;
  ship.tech() = 10.0;
  ship.size() = 10;

  auto res = shoot_ship_to_planet(ctx.em, ship, planet, 20, Coordinates{1, 1},
                                  smap, 0, guntype_t::HEAVY);
  test::expect_true(res.has_value());
  test::expect_ge(res->sectors_destroyed, 0);
  test::expect_false(res->ship_display.empty());
  test::expect_false(res->location_display.empty());
  test::expect_eq(res->previous_sector_owner, player_t{2});
  test::expect_true(res->nuked_players.contains(player_t{2}));

  std::println(std::cout,
               "  ✓ shoot_ship_to_planet valid attack passed (numdest={})",
               res->sectors_destroyed);
}

void test_hit_odds_sizing() {
  std::println(std::cout, "Test: hit_odds sizing with zero and extreme body");

  // Caliber NONE always returns 0 odds
  auto [odds_none, factor_none] =
      hit_odds(100.0, 10.0, 0, false, false, 0, 0, 0, guntype_t::NONE, 0);
  test::expect_eq(odds_none, 0);
  test::expect_eq(factor_none, 0);

  // Zero body size should calculate cleanly without NaN or division by zero
  auto [odds_zero, factor_zero] =
      hit_odds(100.0, 10.0, 0, false, false, 0, 0, 0, guntype_t::LIGHT, 0);
  test::expect_ge(odds_zero, 0);
  test::expect_ge(factor_zero, 0);

  // Standard body size
  auto [odds_std, factor_std] =
      hit_odds(100.0, 10.0, 0, false, false, 0, 0, 100, guntype_t::LIGHT, 0);
  test::expect_ge(odds_std, 0);
  test::expect_gt(factor_std, 0);

  // Huge body size
  auto [odds_huge, factor_huge] = hit_odds(100.0, 10.0, 0, false, false, 0, 0,
                                           1'000'000, guntype_t::LIGHT, 0);
  test::expect_ge(odds_huge, odds_std);
  test::expect_gt(factor_huge, factor_std);

  std::println(std::cout, "  ✓ hit_odds sizing passed");
}

void test_zero_body_ship_combat() {
  std::println(std::cout, "Test: zero-body ship combat safety");

  TestContext ctx;
  ctx.with_standard_universe();

  // Target ship with 0 size and 0 armor (shipbody() == 0, effective_armor()
  // == 0)
  Ship ship{};
  ship.number() = 1;
  ship.owner() = player_t{2};
  ship.type() = ShipType::OTYPE_CANIST;
  ship.enter_planet_orbit(1, 1);
  ship.alive() = true;
  ship.on() = true;
  ship.tech() = 10.0;
  ship.size() = 0;
  ship.max_hanger() = 0;
  ship.armor() = 0;
  ship.set_mass(1.0);

  test::expect_eq(ship.shipbody(), 0u);
  test::expect_eq(ship.effective_armor(), 0u);

  // Attack zero-body ship - must not divide by zero or crash
  const auto* race = ctx.em.peek_race(player_t{1});
  auto res = shoot_planet_to_ship(ctx.em, *race, ship, 25);
  test::expect_true(res.has_value());
  test::expect_ge(res->damage, 0);
  test::expect_false(res->target_location_display.empty());

  std::println(std::cout, "  ✓ zero-body ship combat passed (damage={})",
               res->damage);
}

void test_penetration_factor_domain() {
  std::println(std::cout,
               "Test: penetration factor (p_factor) domain formulas");

  // Constant verification
  test::expect_eq(HITS_PER_ARMOR_PENETRATION, 5u);
  test::expect_eq(TECH_PENETRATION_SCALE, 5.0);

  // Parity tech: (2 / pi) * atan(5) ~= 0.8744
  const double parity_factor = p_factor(10.0, 10.0);
  test::expect_gt(parity_factor, 0.87);
  test::expect_lt(parity_factor, 0.88);

  // Attacker dominance: tech 100 vs 1
  const double attacker_factor = p_factor(100.0, 1.0);
  test::expect_gt(attacker_factor, 0.98);

  // Defender dominance: tech 1 vs 100
  const double defender_factor = p_factor(1.0, 100.0);
  test::expect_lt(defender_factor, 0.15);
  test::expect_gt(defender_factor, 0.0);

  // Extreme defender dominance: tech 0 vs 1000
  const double extreme_factor = p_factor(0.0, 1000.0);
  test::expect_lt(extreme_factor, 0.01);
  test::expect_gt(extreme_factor, 0.0);

  // Cumulative penetration probability r = fac^arm
  const double r0 = std::pow(parity_factor, 0.0);
  test::expect_eq(r0, 1.0);

  const double r1 = std::pow(parity_factor, 1.0);
  test::expect_eq(r1, parity_factor);

  const double r5 = std::pow(parity_factor, 5.0);
  test::expect_lt(r5, r1);
  test::expect_gt(r5, 0.45);
  test::expect_lt(r5, 0.55);  // 0.8744^5 ~= 0.508

  std::println(std::cout, "  ✓ penetration factor domain formulas passed");
}

void test_do_collateral_casualties() {
  std::println(
      std::cout,
      "Test: do_collateral casualty tracking and mass synchronization");

  ship_struct sdata{
      .max_crew = 100,
      .popn = 50,
      .troops = 30,
  };
  Ship ship{sdata};
  const double initial_mass = ship.local_mass(2.0);
  ship.set_mass(initial_mass);

  // Damage = 0: no collateral damage or casualties
  CollateralDamage res0 = do_collateral(ship, 0, 2.0);
  test::expect_eq(res0.civilian_casualties, 0);
  test::expect_eq(res0.military_casualties, 0);
  test::expect_eq(res0.primary_guns_lost, 0u);
  test::expect_eq(res0.secondary_guns_lost, 0u);
  test::expect_eq(ship.popn(), 50);
  test::expect_eq(ship.troops(), 30);
  test::expect_eq(ship.mass(), initial_mass);

  // Damage = 100: guaranteed collateral casualties
  CollateralDamage res100 = do_collateral(ship, 100, 2.0);
  test::expect_eq(res100.civilian_casualties, 50);
  test::expect_eq(res100.military_casualties, 30);
  test::expect_eq(ship.popn(), 0);
  test::expect_eq(ship.troops(), 0);
  test::expect_eq(ship.mass(), initial_mass - 80.0 * 2.0);

  std::println(std::cout, "  ✓ do_collateral casualty tracking passed");
}

void test_shoot_ship_to_ship_and_cew_caliber() {
  std::println(std::cout,
               "Test: shoot_ship_to_ship guard clauses and CEW caliber");

  TestContext ctx;
  ctx.with_standard_universe();

  auto attacker_h = TestShipBuilder(ctx.em, ShipType::STYPE_DESTROYER, 1)
                        .owned_by(1, 1)
                        .named("Attacker")
                        .in_star_orbit(1, SystemCoordinates{10.0, 10.0})
                        .with_size(100)
                        .with_tech(100.0)
                        .with_on(true)
                        .build_handle();
  Ship& attacker = *attacker_h;
  attacker.guns() = ActiveBattery::NONE;
  attacker.cew() = 20;
  attacker.cew_range() = 50;

  auto target_h = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER, 2)
                      .owned_by(2, 1)
                      .named("Target")
                      .in_star_orbit(1, SystemCoordinates{12.0, 10.0})
                      .with_size(100)
                      .with_tech(10.0)
                      .with_armor(2)
                      .with_crew(50, 0)
                      .with_on(true)
                      .build_handle();
  Ship& target = *target_h;
  target.set_mass(100.0);

  // 1. Conventional attack with ActiveBattery::NONE fails (caliber == NONE)
  test::expect_false(
      shoot_ship_to_ship(ctx.em, attacker, target, 5, 0, false).has_value());

  // 2. CEW attack (range != 0) succeeds even when conventional guns == NONE,
  //    using guntype_t::LIGHT equivalent destruct units.
  auto cew_res = shoot_ship_to_ship(ctx.em, attacker, target, 10, 2, false);
  test::expect_true(cew_res.has_value());
  test::expect_eq(cew_res->weapon, ShipShotWeaponKind::Cew);
  test::expect_eq(cew_res->strength, 10);

  // 3. Out of range fails
  const auto orig_coords = target.coordinates();
  target.coordinates() = UniverseCoordinates{10000.0, 10000.0};
  test::expect_false(
      shoot_ship_to_ship(ctx.em, attacker, target, 10, 0, false).has_value());

  // 4. Radiative weapon mode (mode == 1)
  target.coordinates() = orig_coords;
  target.alive() = true;
  attacker.type() = ShipType::STYPE_MINE;
  attacker.mode() = 1;
  auto rad_res = shoot_ship_to_ship(ctx.em, attacker, target, 10, 0, true);
  test::expect_true(rad_res.has_value());
  test::expect_eq(rad_res->weapon, ShipShotWeaponKind::Radiation);
  test::expect_eq(rad_res->damage,
                  static_cast<damage_t>(rad_res->radiation_dosage));

  std::println(std::cout,
               "  ✓ shoot_ship_to_ship guard clauses and CEW caliber passed");
}

int main() {
  test_shoot_planet_to_ship_invalid_cases();
  test_shoot_planet_to_ship_valid_attack();
  test_shoot_ship_to_planet_invalid_cases();
  test_shoot_ship_to_planet_valid_attack();
  test_hit_odds_sizing();
  test_zero_body_ship_combat();
  test_penetration_factor_domain();
  test_do_collateral_casualties();
  test_shoot_ship_to_ship_and_cew_caliber();

  std::println(std::cout, "\n✅ All shootblast tests passed!");
  return 0;
}
