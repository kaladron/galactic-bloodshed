// SPDX-License-Identifier: Apache-2.0

import gblib;
import std.compat;

#include <sqlite3.h>

#include <cassert>
#include <cstdio>

int main() {
  // Initialize database using common Sql class (in-memory for testing)
  Sql db(":memory:");

  // Initialize database tables - this will create the tbl_ship_json table
  initsqldata();

  Ship test_ship{};

  // Initialize some basic fields for testing
  test_ship.number = 42;
  test_ship.owner = 1;
  test_ship.governor = 2;
  strcpy(test_ship.name, "TestShip");
  strcpy(test_ship.shipclass, "Cruiser");
  test_ship.race = 1;
  test_ship.xpos = 100.5;
  test_ship.ypos = 200.5;
  test_ship.fuel = 1000.0;
  test_ship.mass = 5000.0;
  test_ship.land_x = 10;
  test_ship.land_y = 20;
  test_ship.destshipno = 0;
  test_ship.nextship = 0;
  test_ship.ships = 0;
  test_ship.armor = 100;
  test_ship.size = 500;
  test_ship.max_crew = 50;
  test_ship.max_resource = 1000;
  test_ship.max_destruct = 200;
  test_ship.max_fuel = 2000;
  test_ship.max_speed = 9;
  test_ship.build_type = ShipType::STYPE_BATTLE;
  test_ship.build_cost = 5000;
  test_ship.base_mass = 4500.0;
  test_ship.tech = 10.0;
  test_ship.complexity = 8.0;
  test_ship.destruct = 150;
  test_ship.resource = 800;
  test_ship.popn = 45;
  test_ship.troops = 10;
  test_ship.crystals = 5;
  test_ship.who_killed = 0;
  test_ship.navigate.on = 0;
  test_ship.navigate.speed = 5;
  test_ship.navigate.turns = 0;
  test_ship.navigate.bearing = 90;
  test_ship.protect.maxrng = 50.0;
  test_ship.protect.on = 1;
  test_ship.protect.planet = 1;
  test_ship.protect.self = 1;
  test_ship.protect.evade = 0;
  test_ship.protect.ship = 0;
  test_ship.mount = 0;
  test_ship.hyper_drive.charge = 0;
  test_ship.hyper_drive.ready = 0;
  test_ship.hyper_drive.on = 0;
  test_ship.hyper_drive.has = 1;
  test_ship.cew = 0;
  test_ship.cew_range = 0;
  test_ship.cloak = 0;
  test_ship.laser = 50;
  test_ship.focus = 0;
  test_ship.fire_laser = 25;
  test_ship.storbits = 1;
  test_ship.deststar = 2;
  test_ship.destpnum = 3;
  test_ship.pnumorbits = 1;
  test_ship.whatdest = ScopeLevel::LEVEL_PLAN;
  test_ship.whatorbits = ScopeLevel::LEVEL_PLAN;
  test_ship.damage = 0;
  test_ship.rad = 0;
  test_ship.retaliate = 1;
  test_ship.target = 0;
  test_ship.type = ShipType::STYPE_BATTLE;
  test_ship.speed = 7;
  test_ship.active = 1;
  test_ship.alive = 1;
  test_ship.mode = 0;
  test_ship.bombard = 0;
  test_ship.mounted = 0;
  test_ship.cloaked = 0;
  test_ship.sheep = 0;
  test_ship.docked = 0;
  test_ship.notified = 0;
  test_ship.examined = 0;
  test_ship.on = 1;
  test_ship.merchant = 0;
  test_ship.guns = 2;
  test_ship.primary = 100;
  test_ship.primtype = 1;
  test_ship.secondary = 50;

  // Test putship - stores in SQLite as JSON
  putship(test_ship);

  // Test getship - reads from SQLite
  Ship* retrieved_ship = nullptr;
  auto result = getship(&retrieved_ship, 42);

  if (result.has_value() && retrieved_ship != nullptr) {
    // Verify key fields
    assert(retrieved_ship->number == test_ship.number);
    assert(retrieved_ship->owner == test_ship.owner);
    assert(retrieved_ship->governor == test_ship.governor);
    assert(strcmp(retrieved_ship->name, test_ship.name) == 0);
    assert(strcmp(retrieved_ship->shipclass, test_ship.shipclass) == 0);
    assert(retrieved_ship->race == test_ship.race);
    assert(abs(retrieved_ship->xpos - test_ship.xpos) < 0.1);
    assert(abs(retrieved_ship->ypos - test_ship.ypos) < 0.1);
    assert(abs(retrieved_ship->fuel - test_ship.fuel) < 0.1);
    assert(abs(retrieved_ship->mass - test_ship.mass) < 0.1);
    assert(retrieved_ship->land_x == test_ship.land_x);
    assert(retrieved_ship->land_y == test_ship.land_y);
    assert(retrieved_ship->armor == test_ship.armor);
    assert(retrieved_ship->size == test_ship.size);
    assert(retrieved_ship->max_crew == test_ship.max_crew);
    assert(retrieved_ship->type == test_ship.type);
    assert(retrieved_ship->active == test_ship.active);
    assert(retrieved_ship->alive == test_ship.alive);

    // Free the allocated memory
    free(retrieved_ship);

    std::println("Ship SQLite JSON storage test passed!");
  } else {
    std::println(::stderr, "Failed to retrieve ship from database");
    return 1;
  }

  // Database connection will be cleaned up automatically by Sql destructor
  return 0;
}