// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import commands;
import std.compat;

#include <cassert>

int main() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "Jettisoner";
  race.Guest = false;
  race.governor[0].active = true;
  race.mass = 1.0;  // Used for crew/troop mass calculations

  RaceRepository races(store);
  races.save(race);

  // Create test star
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "JettisonStar";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.AP[0] = 10;
  Star star(ss);

  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create a ship with cargo
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.alive() = true;
  ship.active() = true;
  ship.type() = ShipType::STYPE_SHUTTLE;
  ship.name() = "CargoShip";
  ship.whatorbits() = ScopeLevel::LEVEL_STAR;
  ship.storbits() = 0;
  ship.fuel() = 100.0;
  ship.resource() = 50;
  ship.destruct() = 20;
  ship.crystals() = 5;
  ship.popn() = 10;
  ship.troops() = 8;
  ship.mass() = 100.0;

  ShipRepository ships_repo(store);
  ships_repo.save(ship);

  // Create GameObj
  GameObj g(em);
  g.set_player(1);
  g.set_governor(0);
  g.race = em.peek_race(1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);
  g.set_shipno(1);

  std::println("Test 1: Jettison crystals");
  {
    const auto* s_before = em.peek_ship(1);
    int initial_crystals = s_before->crystals();

    command_t argv = {"jettison", "#1", "x", "3"};
    GB::commands::jettison(argv, g);

    const auto* s_after = em.peek_ship(1);
    assert(s_after->crystals() == initial_crystals - 3);
    std::println("✓ Crystals jettisoned");
  }

  std::println("Test 2: Jettison crew");
  {
    const auto* s_before = em.peek_ship(1);
    int initial_popn = s_before->popn();
    double initial_mass = s_before->mass();

    command_t argv = {"jettison", "#1", "c", "5"};
    GB::commands::jettison(argv, g);

    const auto* s_after = em.peek_ship(1);
    assert(s_after->popn() == initial_popn - 5);
    assert(s_after->mass() == initial_mass - (5 * race.mass));
    std::println("✓ Crew jettisoned with mass reduction");
  }

  std::println("Test 3: Jettison military");
  {
    const auto* s_before = em.peek_ship(1);
    int initial_troops = s_before->troops();
    double initial_mass = s_before->mass();

    command_t argv = {"jettison", "#1", "m", "4"};
    GB::commands::jettison(argv, g);

    const auto* s_after = em.peek_ship(1);
    assert(s_after->troops() == initial_troops - 4);
    assert(s_after->mass() == initial_mass - (4 * race.mass));
    std::println("✓ Military jettisoned with mass reduction");
  }

  std::println("Test 4: Jettison destruct");
  {
    const auto* s_before = em.peek_ship(1);
    int initial_destruct = s_before->destruct();

    command_t argv = {"jettison", "#1", "d", "10"};
    GB::commands::jettison(argv, g);

    const auto* s_after = em.peek_ship(1);
    assert(s_after->destruct() == initial_destruct - 10);
    std::println("✓ Destruct jettisoned");
  }

  std::println("Test 5: Jettison fuel");
  {
    const auto* s_before = em.peek_ship(1);
    double initial_fuel = s_before->fuel();

    command_t argv = {"jettison", "#1", "f", "25"};
    GB::commands::jettison(argv, g);

    const auto* s_after = em.peek_ship(1);
    assert(s_after->fuel() == initial_fuel - 25);
    std::println("✓ Fuel jettisoned");
  }

  std::println("Test 6: Jettison resources");
  {
    const auto* s_before = em.peek_ship(1);
    int initial_resource = s_before->resource();

    command_t argv = {"jettison", "#1", "r", "30"};
    GB::commands::jettison(argv, g);

    const auto* s_after = em.peek_ship(1);
    assert(s_after->resource() == initial_resource - 30);
    std::println("✓ Resources jettisoned");
  }

  std::println("All jettison tests passed!");
  return 0;
}
