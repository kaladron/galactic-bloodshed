// SPDX-License-Identifier: Apache-2.0

/// \file shootblast_test.cc
/// \brief Unit tests for shoot_planet_to_ship and shoot_ship_to_planet.

import dallib;
import gb.entities;
import gb.services;
import gb.turn;
import test;
import std;

void test_shoot_planet_to_ship_invalid_cases() {
  std::println(std::cout, "Test: shoot_planet_to_ship invalid cases");

  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  Race race{};
  race.Playernum = player_t{1};
  race.tech = 100.0;

  Ship ship{};
  ship.number() = 1;
  ship.owner() = player_t{2};
  ship.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship.alive() = true;

  // Test 1: Zero strength -> returns std::nullopt
  auto dam1 = shoot_planet_to_ship(em, race, ship, 0);
  test::expect_false(dam1.has_value());

  // Test 2: Dead ship -> returns std::nullopt
  ship.alive() = false;
  auto dam2 = shoot_planet_to_ship(em, race, ship, 10);
  test::expect_false(dam2.has_value());

  // Test 3: Wrong orbit level -> returns std::nullopt
  ship.alive() = true;
  ship.whatorbits() = ScopeLevel::LEVEL_STAR;
  auto dam3 = shoot_planet_to_ship(em, race, ship, 10);
  test::expect_false(dam3.has_value());

  std::println(std::cout, "  ✓ shoot_planet_to_ship invalid cases passed");
}

void test_shoot_planet_to_ship_valid_attack() {
  std::println(std::cout, "Test: shoot_planet_to_ship valid attack");

  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);
  EntityManager em(db);

  // Create star 0 and planet 0 in db
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "Sol";
  ss.pnames.emplace_back("Terra");
  Star star(ss);
  StarRepository star_repo(store);
  star_repo.save(star);

  Planet planet{PlanetType::EARTH, Coordinates{10, 10}};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  PlanetRepository planet_repo(store);
  planet_repo.save(planet);

  RaceRepository race_repo(store);

  Race race1{};
  race1.Playernum = player_t{1};
  race1.name = "Attacker";
  race1.tech = 10.0;
  race_repo.save(race1);

  Race race2{};
  race2.Playernum = player_t{2};
  race2.name = "Defender";
  race2.tech = 10.0;
  race_repo.save(race2);

  // Create a target ship in planet scope
  Ship ship{};
  ship.number() = 1;
  ship.owner() = player_t{2};
  ship.type() = ShipType::OTYPE_CANIST;
  ship.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship.storbits() = 0;
  ship.pnumorbits() = 0;
  ship.alive() = true;
  ship.on() = true;
  ship.tech() = 10.0;
  ship.size() = 10;
  ship.max_crew() = 10;
  ship.mass() = 10;
  ship.armor() = 5;

  auto res = shoot_planet_to_ship(em, race1, ship, 20);
  test::expect_true(res.has_value());
  auto [damage, short_msg, long_msg] = *res;
  test::expect_ge(damage, 0);
  test::expect_false(short_msg.empty());
  test::expect_false(long_msg.empty());

  std::println(std::cout,
               "  ✓ shoot_planet_to_ship valid attack passed (damage={})",
               damage);
}

void test_shoot_ship_to_planet_invalid_cases() {
  std::println(std::cout, "Test: shoot_ship_to_planet invalid cases");

  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  Planet planet{PlanetType::EARTH, Coordinates{5, 5}};
  planet.star_id() = 0;
  planet.planet_order() = 0;

  SectorMap smap(planet);

  Ship ship{};
  ship.number() = 1;
  ship.owner() = player_t{1};
  ship.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship.alive() = true;
  ship.on() = true;

  // Test 1: Zero strength -> returns std::nullopt
  auto res1 = shoot_ship_to_planet(em, ship, planet, 0, Coordinates{0, 0}, smap,
                                   0, GTYPE_NONE);
  test::expect_false(res1.has_value());

  // Test 2: Dead ship -> returns std::nullopt
  ship.alive() = false;
  auto res2 = shoot_ship_to_planet(em, ship, planet, 10, Coordinates{0, 0},
                                   smap, 0, GTYPE_NONE);
  test::expect_false(res2.has_value());

  // Test 3: Invalid planet coords -> returns std::nullopt
  ship.alive() = true;
  auto res3 = shoot_ship_to_planet(em, ship, planet, 10, Coordinates{10, 10},
                                   smap, 0, GTYPE_NONE);
  test::expect_false(res3.has_value());

  std::println(std::cout, "  ✓ shoot_ship_to_planet invalid cases passed");
}

void test_shoot_ship_to_planet_valid_attack() {
  std::println(std::cout, "Test: shoot_ship_to_planet valid attack");

  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);
  EntityManager em(db);

  star_struct ss{};
  ss.star_id = 0;
  ss.name = "Sol";
  ss.pnames.emplace_back("Terra");
  Star star(ss);
  StarRepository star_repo(store);
  star_repo.save(star);

  RaceRepository race_repo(store);
  Race race1{};
  race1.Playernum = player_t{1};
  race1.name = "Attacker";
  race_repo.save(race1);

  Race race2{};
  race2.Playernum = player_t{2};
  race2.name = "Target";
  race_repo.save(race2);

  Planet planet{PlanetType::EARTH, Coordinates{4, 4}};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  PlanetRepository planet_repo(store);
  planet_repo.save(planet);

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
  ship.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship.storbits() = 0;
  ship.pnumorbits() = 0;
  ship.alive() = true;
  ship.on() = true;
  ship.tech() = 10.0;
  ship.size() = 10;

  auto res = shoot_ship_to_planet(em, ship, planet, 10, Coordinates{1, 1}, smap,
                                  0, GTYPE_HEAVY);
  test::expect_true(res.has_value());
  test::expect_ge(res->sectors_destroyed, 0);
  test::expect_false(res->short_message.empty());
  test::expect_false(res->long_message.empty());
  test::expect_true(res->nuked_players[player_t{2}]);

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

  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);
  EntityManager em(db);

  star_struct ss{};
  ss.star_id = 0;
  ss.name = "Sol";
  ss.pnames.emplace_back("Terra");
  Star star(ss);
  StarRepository(store).save(star);

  Planet planet{PlanetType::EARTH, Coordinates{10, 10}};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  PlanetRepository(store).save(planet);

  Race race{};
  race.Playernum = player_t{1};
  race.name = "Attacker";
  race.tech = 50.0;
  RaceRepository(store).save(race);

  // Target ship with 0 size and 0 armor (shipbody() == 0, effective_armor()
  // == 0)
  Ship ship{};
  ship.number() = 1;
  ship.owner() = player_t{2};
  ship.type() = ShipType::OTYPE_CANIST;
  ship.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship.storbits() = 0;
  ship.pnumorbits() = 0;
  ship.alive() = true;
  ship.on() = true;
  ship.tech() = 10.0;
  ship.size() = 0;
  ship.max_hanger() = 0;
  ship.armor() = 0;
  ship.mass() = 1;

  test::expect_eq(ship.shipbody(), 0u);
  test::expect_eq(ship.effective_armor(), 0u);

  // Attack zero-body ship - must not divide by zero or crash
  auto res = shoot_planet_to_ship(em, race, ship, 25);
  test::expect_true(res.has_value());
  auto [damage, short_msg, long_msg] = *res;
  test::expect_ge(damage, 0);
  test::expect_false(short_msg.empty());

  std::println(std::cout, "  ✓ zero-body ship combat passed (damage={})",
               damage);
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

int main() {
  test_shoot_planet_to_ship_invalid_cases();
  test_shoot_planet_to_ship_valid_attack();
  test_shoot_ship_to_planet_invalid_cases();
  test_shoot_ship_to_planet_valid_attack();
  test_hit_odds_sizing();
  test_zero_body_ship_combat();
  test_penetration_factor_domain();

  std::println(std::cout, "\n✅ All shootblast tests passed!");
  return 0;
}
