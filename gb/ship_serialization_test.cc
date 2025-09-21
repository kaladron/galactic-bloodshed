// SPDX-License-Identifier: Apache-2.0

import gblib;
import std.compat;

#include <cassert>

int main() {
  Ship test_ship{};

  // Initialize basic fields for testing
  test_ship.number = 1;
  test_ship.owner = 1;
  test_ship.governor = 0;
  strcpy(test_ship.name, "TestShip");
  strcpy(test_ship.shipclass, "TestClass");
  
  test_ship.race = 1;
  test_ship.xpos = 100.5;
  test_ship.ypos = 200.5;
  test_ship.fuel = 500.0;
  test_ship.mass = 1000.0;
  test_ship.land_x = 10;
  test_ship.land_y = 15;
  
  test_ship.destshipno = 2;
  test_ship.nextship = 3;
  test_ship.ships = 0;
  
  test_ship.armor = 10;
  test_ship.size = 500;
  test_ship.max_crew = 50;
  test_ship.max_resource = 1000;
  test_ship.max_destruct = 100;
  test_ship.max_fuel = 2000;
  test_ship.max_speed = 9;
  
  test_ship.build_type = ShipType::STYPE_SHUTTLE;
  test_ship.build_cost = 100;
  test_ship.base_mass = 800.0;
  test_ship.tech = 50.0;
  test_ship.complexity = 25.0;
  
  test_ship.destruct = 50;
  test_ship.resource = 500;
  test_ship.popn = 25;
  test_ship.troops = 10;
  test_ship.crystals = 5;
  
  // Initialize the special union with aimed_at data
  test_ship.special.aimed_at.shipno = 100;
  test_ship.special.aimed_at.snum = 1;
  test_ship.special.aimed_at.intensity = 50;
  test_ship.special.aimed_at.pnum = 2;
  test_ship.special.aimed_at.level = ScopeLevel::LEVEL_PLAN;
  
  test_ship.who_killed = -1;
  
  // Initialize navigate struct
  test_ship.navigate.on = 1;
  test_ship.navigate.speed = 5;
  test_ship.navigate.turns = 10;
  test_ship.navigate.bearing = 180;
  
  // Initialize protect struct
  test_ship.protect.maxrng = 100.0;
  test_ship.protect.on = 0;
  test_ship.protect.planet = 0;
  test_ship.protect.self = 1;
  test_ship.protect.evade = 0;
  test_ship.protect.ship = 0;
  
  test_ship.mount = 1;
  
  // Initialize hyper_drive struct
  test_ship.hyper_drive.charge = 100;
  test_ship.hyper_drive.ready = 1;
  test_ship.hyper_drive.on = 0;
  test_ship.hyper_drive.has = 1;
  
  test_ship.cew = 10;
  test_ship.cew_range = 50;
  test_ship.cloak = 0;
  test_ship.laser = 1;
  test_ship.focus = 0;
  test_ship.fire_laser = 20;
  
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
  
  test_ship.type = ShipType::STYPE_SHUTTLE;
  test_ship.speed = 5;
  
  test_ship.active = 1;
  test_ship.alive = 1;
  test_ship.mode = 0;
  test_ship.bombard = 0;
  test_ship.mounted = 1;
  test_ship.cloaked = 0;
  test_ship.sheep = 0;
  test_ship.docked = 0;
  test_ship.notified = 0;
  test_ship.examined = 0;
  test_ship.on = 1;
  
  test_ship.merchant = 0;
  test_ship.guns = 1;
  test_ship.primary = 123456;
  test_ship.primtype = 1;
  test_ship.secondary = 789012;
  test_ship.sectype = 2;
  
  test_ship.hanger = 10;
  test_ship.max_hanger = 20;

  // Test serialization to JSON
  auto json_result = ship_to_json(test_ship);
  assert(json_result.has_value());

  // Test deserialization from JSON
  auto deserialized_ship_opt = ship_from_json(json_result.value());
  assert(deserialized_ship_opt.has_value());

  Ship deserialized_ship = deserialized_ship_opt.value();

  // Verify key fields
  assert(deserialized_ship.number == test_ship.number);
  assert(deserialized_ship.owner == test_ship.owner);
  assert(deserialized_ship.governor == test_ship.governor);
  assert(strcmp(deserialized_ship.name, test_ship.name) == 0);
  assert(strcmp(deserialized_ship.shipclass, test_ship.shipclass) == 0);
  
  assert(deserialized_ship.race == test_ship.race);
  assert(deserialized_ship.xpos == test_ship.xpos);
  assert(deserialized_ship.ypos == test_ship.ypos);
  assert(deserialized_ship.fuel == test_ship.fuel);
  assert(deserialized_ship.mass == test_ship.mass);
  assert(deserialized_ship.land_x == test_ship.land_x);
  assert(deserialized_ship.land_y == test_ship.land_y);
  
  assert(deserialized_ship.destshipno == test_ship.destshipno);
  assert(deserialized_ship.nextship == test_ship.nextship);
  assert(deserialized_ship.ships == test_ship.ships);
  
  assert(deserialized_ship.armor == test_ship.armor);
  assert(deserialized_ship.size == test_ship.size);
  assert(deserialized_ship.max_crew == test_ship.max_crew);
  assert(deserialized_ship.max_resource == test_ship.max_resource);
  assert(deserialized_ship.max_destruct == test_ship.max_destruct);
  assert(deserialized_ship.max_fuel == test_ship.max_fuel);
  assert(deserialized_ship.max_speed == test_ship.max_speed);
  
  assert(deserialized_ship.build_type == test_ship.build_type);
  assert(deserialized_ship.build_cost == test_ship.build_cost);
  assert(deserialized_ship.base_mass == test_ship.base_mass);
  assert(deserialized_ship.tech == test_ship.tech);
  assert(deserialized_ship.complexity == test_ship.complexity);
  
  assert(deserialized_ship.destruct == test_ship.destruct);
  assert(deserialized_ship.resource == test_ship.resource);
  assert(deserialized_ship.popn == test_ship.popn);
  assert(deserialized_ship.troops == test_ship.troops);
  assert(deserialized_ship.crystals == test_ship.crystals);
  
  // Verify union data (this might be tricky due to the union nature)
  assert(deserialized_ship.special.aimed_at.shipno == test_ship.special.aimed_at.shipno);
  assert(deserialized_ship.special.aimed_at.snum == test_ship.special.aimed_at.snum);
  assert(deserialized_ship.special.aimed_at.intensity == test_ship.special.aimed_at.intensity);
  assert(deserialized_ship.special.aimed_at.pnum == test_ship.special.aimed_at.pnum);
  assert(deserialized_ship.special.aimed_at.level == test_ship.special.aimed_at.level);
  
  assert(deserialized_ship.who_killed == test_ship.who_killed);
  
  // Verify navigate struct
  assert(deserialized_ship.navigate.on == test_ship.navigate.on);
  assert(deserialized_ship.navigate.speed == test_ship.navigate.speed);
  assert(deserialized_ship.navigate.turns == test_ship.navigate.turns);
  assert(deserialized_ship.navigate.bearing == test_ship.navigate.bearing);
  
  // Verify protect struct
  assert(deserialized_ship.protect.maxrng == test_ship.protect.maxrng);
  assert(deserialized_ship.protect.on == test_ship.protect.on);
  assert(deserialized_ship.protect.planet == test_ship.protect.planet);
  assert(deserialized_ship.protect.self == test_ship.protect.self);
  assert(deserialized_ship.protect.evade == test_ship.protect.evade);
  assert(deserialized_ship.protect.ship == test_ship.protect.ship);
  
  assert(deserialized_ship.mount == test_ship.mount);
  
  // Verify hyper_drive struct
  assert(deserialized_ship.hyper_drive.charge == test_ship.hyper_drive.charge);
  assert(deserialized_ship.hyper_drive.ready == test_ship.hyper_drive.ready);
  assert(deserialized_ship.hyper_drive.on == test_ship.hyper_drive.on);
  assert(deserialized_ship.hyper_drive.has == test_ship.hyper_drive.has);
  
  assert(deserialized_ship.type == test_ship.type);
  assert(deserialized_ship.speed == test_ship.speed);
  assert(deserialized_ship.active == test_ship.active);
  assert(deserialized_ship.alive == test_ship.alive);
  
  assert(deserialized_ship.guns == test_ship.guns);
  assert(deserialized_ship.primary == test_ship.primary);
  assert(deserialized_ship.primtype == test_ship.primtype);
  assert(deserialized_ship.secondary == test_ship.secondary);
  assert(deserialized_ship.sectype == test_ship.sectype);
  
  assert(deserialized_ship.hanger == test_ship.hanger);
  assert(deserialized_ship.max_hanger == test_ship.max_hanger);

  std::printf("Ship serialization test passed!\n");
  return 0;
}