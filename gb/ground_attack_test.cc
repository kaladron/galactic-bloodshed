// SPDX-License-Identifier: Apache-2.0

/// \file ground_attack_test.cc
/// \brief Pre-refactoring unit tests for mech_attack_people and
/// people_attack_mech in move.cc.

import dallib;
import gblib;
import std;

#include <cassert>

void test_mech_attack_people() {
  std::println(std::cout, "Test: mech_attack_people");

  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  Race race{};
  race.Playernum = player_t{1};
  race.name = "AttackerRace";
  race.tech = 10.0;
  race.morale = 10;
  std::ranges::fill(race.likes, 1.0);

  Race alien{};
  alien.Playernum = player_t{2};
  alien.name = "DefenderRace";
  alien.tech = 10.0;
  alien.morale = 10;
  std::ranges::fill(alien.likes, 1.0);

  Sector sect{};
  sect.set_condition(SectorType::SEC_LAND);

  Ship ship{};
  ship.number() = 1;
  ship.owner() = player_t{1};
  ship.type() = ShipType::OTYPE_AFV;
  ship.tech() = 10.0;
  ship.armor() = 10;
  ship.damage() = 0;
  ship.alive() = true;
  ship.popn() = 10;
  ship.retaliate() = 100;
  ship.destruct() = 100;
  ship.guns() = PRIMARY;
  ship.primary() = 10;
  ship.primtype() = GTYPE_HEAVY;

  population_t civ = 100;
  population_t mil = 50;

  char long_buf[1024] = {0};
  char short_buf[256] = {0};

  mech_attack_people(em, ship, &civ, &mil, race, alien, sect, true, long_buf,
                     short_buf);

  assert(std::strlen(short_buf) > 0);
  assert(std::strlen(long_buf) > 0);
  assert(std::string(long_buf).find("Battle at") != std::string::npos);

  std::println(
      std::cout,
      "  ✓ mech_attack_people passed (civ remaining={}, mil remaining={})", civ,
      mil);
}

void test_people_attack_mech() {
  std::println(std::cout, "Test: people_attack_mech");

  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  Race race{};
  race.Playernum = player_t{1};
  race.name = "AttackerPeople";
  race.tech = 10.0;
  race.fighters = 5;
  race.morale = 10;
  std::ranges::fill(race.likes, 1.0);

  Race alien{};
  alien.Playernum = player_t{2};
  alien.name = "MechOwner";
  alien.tech = 10.0;
  alien.morale = 10;
  std::ranges::fill(alien.likes, 1.0);

  Sector sect{};
  sect.set_condition(SectorType::SEC_LAND);

  Ship ship{};
  ship.number() = 1;
  ship.owner() = player_t{2};
  ship.type() = ShipType::OTYPE_AFV;
  ship.tech() = 10.0;
  ship.armor() = 5;
  ship.damage() = 0;
  ship.alive() = true;
  ship.popn() = 10;
  ship.retaliate() = 100;
  ship.destruct() = 100;
  ship.guns() = PRIMARY;
  ship.primary() = 5;

  char long_buf[1024] = {0};
  char short_buf[256] = {0};

  people_attack_mech(em, ship, 100, 50, race, alien, sect, Coordinates{1, 1},
                     long_buf, short_buf);

  assert(std::strlen(short_buf) > 0);
  assert(std::strlen(long_buf) > 0);
  assert(std::string(long_buf).find("assault") != std::string::npos);

  std::println(std::cout, "  ✓ people_attack_mech passed (ship damage={})",
               ship.damage());
}

int main() {
  test_mech_attack_people();
  test_people_attack_mech();

  std::println(std::cout,
               "\n✅ All ground attack pre-refactoring tests passed!");
  return 0;
}
