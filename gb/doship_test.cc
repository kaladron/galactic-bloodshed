// SPDX-License-Identifier: Apache-2.0

/// \file doship_test.cc
/// \brief Unit tests for doship() turn simulation actions: domass, doown,
/// habitat population/resource growth, and weapon plant production.

import dallib;
import gblib;
import test;
import std;

namespace {

Race createTestRace(player_t playernum = player_t{1}) {
  Race race{};
  race.Playernum = playernum;
  race.mass = 1.0;
  race.birthrate = 0.1;
  race.tech = 50.0;
  return race;
}

void test_domass_and_doown() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  RaceRepository races(store);
  races.save(race);

  ship_struct parent_data{
      .owner = player_t{1},
      .type = ShipType::STYPE_CARRIER,
      .active = 1,
      .alive = 1,
  };
  auto parent_handle = em.create_ship(parent_data);
  Ship& parent = *parent_handle;

  ship_struct child_data{
      .owner = player_t{2},
      .popn = 10,
      .whatorbits = ScopeLevel::LEVEL_SHIP,
      .type = ShipType::STYPE_SHUTTLE,
      .active = 1,
      .alive = 1,
  };
  child_data.destshipno = parent.number();
  auto child_handle = em.create_ship(child_data);
  Ship& child = *child_handle;

  parent.ships() = child.number();

  doown(parent, em);
  test::expect_eq(child.owner(), player_t{1});

  domass(parent, em);
  test::expect_gt(parent.mass(), 0.0);
}

void test_do_habitat() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  RaceRepository races(store);
  races.save(race);

  ship_struct sdata{
      .owner = player_t{1},
      .fuel = 100.0,
      .max_crew = 100,
      .max_resource = 1000,
      .resource = 10,
      .popn = 50,
      .type = ShipType::STYPE_HABITAT,
      .active = 1,
      .alive = 1,
  };
  auto ship_handle = em.create_ship(sdata);
  Ship& ship = *ship_handle;
  ship.on() = 1;

  do_habitat(ship, em);

  test::expect_gt(ship.resource(), 10);
  test::expect_gt(ship.popn(), 50);
}

void test_do_weapon_plant() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  race.tech = 100.0;
  RaceRepository races(store);
  races.save(race);

  ship_struct sdata{
      .owner = player_t{1},
      .fuel = 100.0,
      .max_crew = 100,
      .resource = 500,
      .popn = 100,
      .type = ShipType::OTYPE_WPLANT,
      .active = 1,
      .alive = 1,
  };
  auto ship_handle = em.create_ship(sdata);
  Ship& ship = *ship_handle;

  int produced = do_weapon_plant(ship, em);
  test::expect_gt(produced, 0);
  test::expect_lt(ship.resource(), 500);
}

}  // namespace

int main() {
  std::println(std::cout, "Running doship unit tests...\n");

  std::println(std::cout, "  Testing domass and doown... ");
  test_domass_and_doown();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing do_habitat... ");
  test_do_habitat();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing do_weapon_plant... ");
  test_do_weapon_plant();
  std::println(std::cout, "PASS");

  std::println(std::cout, "All doship tests passed!");
  return 0;
}
