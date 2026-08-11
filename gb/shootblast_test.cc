// SPDX-License-Identifier: Apache-2.0

/// \file shootblast_test.cc
/// \brief Pre-refactoring unit tests for shoot_planet_to_ship and
/// shoot_ship_to_planet.

import dallib;
import gblib;
import std;

#include <cassert>

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
  assert(!dam1.has_value());

  // Test 2: Dead ship -> returns std::nullopt
  ship.alive() = false;
  auto dam2 = shoot_planet_to_ship(em, race, ship, 10);
  assert(!dam2.has_value());

  // Test 3: Wrong orbit level -> returns std::nullopt
  ship.alive() = true;
  ship.whatorbits() = ScopeLevel::LEVEL_STAR;
  auto dam3 = shoot_planet_to_ship(em, race, ship, 10);
  assert(!dam3.has_value());

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

  Planet planet{PlanetType::EARTH};
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
  assert(res.has_value());
  auto [damage, short_msg, long_msg] = *res;
  assert(damage >= 0);
  assert(!short_msg.empty());
  assert(!long_msg.empty());

  std::println(std::cout,
               "  ✓ shoot_planet_to_ship valid attack passed (damage={})",
               damage);
}

void test_shoot_ship_to_planet_invalid_cases() {
  std::println(std::cout, "Test: shoot_ship_to_planet invalid cases");

  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  Planet planet{PlanetType::EARTH};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.Maxx() = 5;
  planet.Maxy() = 5;

  SectorMap smap(planet, true);

  Ship ship{};
  ship.number() = 1;
  ship.owner() = player_t{1};
  ship.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship.alive() = true;
  ship.on() = true;

  // Test 1: Zero strength -> returns std::nullopt
  auto res1 =
      shoot_ship_to_planet(em, ship, planet, 0, Coordinates{0, 0}, smap, 0, 0);
  assert(!res1.has_value());

  // Test 2: Dead ship -> returns std::nullopt
  ship.alive() = false;
  auto res2 =
      shoot_ship_to_planet(em, ship, planet, 10, Coordinates{0, 0}, smap, 0, 0);
  assert(!res2.has_value());

  // Test 3: Invalid planet coords -> returns std::nullopt
  ship.alive() = true;
  auto res3 = shoot_ship_to_planet(em, ship, planet, 10, Coordinates{10, 10},
                                   smap, 0, 0);
  assert(!res3.has_value());

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

  Planet planet{PlanetType::EARTH};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.Maxx() = 4;
  planet.Maxy() = 4;
  PlanetRepository planet_repo(store);
  planet_repo.save(planet);

  SectorMap smap(planet, true);
  auto& s = smap.get(1, 1);
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
  assert(res.has_value());
  auto [numdest, nuked, short_msg, long_msg] = *res;
  assert(numdest >= 0);
  assert(!short_msg.empty());
  assert(!long_msg.empty());

  std::println(std::cout,
               "  ✓ shoot_ship_to_planet valid attack passed (numdest={})",
               numdest);
}

int main() {
  test_shoot_planet_to_ship_invalid_cases();
  test_shoot_planet_to_ship_valid_attack();
  test_shoot_ship_to_planet_invalid_cases();
  test_shoot_ship_to_planet_valid_attack();

  std::println(std::cout, "\n✅ All shootblast pre-refactoring tests passed!");
  return 0;
}
