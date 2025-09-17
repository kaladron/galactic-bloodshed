// SPDX-License-Identifier: Apache-2.0

import gblib;
import std.compat;

#include <cassert>

int main() {
  Race test_race{};

  // Initialize some basic fields for testing
  test_race.Playernum = 1;
  test_race.name = "TestRace";
  test_race.password = "testpass";
  test_race.info = "Test race information";
  test_race.motto = "Test motto";

  test_race.absorb = true;
  test_race.collective_iq = false;
  test_race.pods = true;
  test_race.fighters = 100;
  test_race.IQ = 150;
  test_race.IQ_limit = 200;
  test_race.number_sexes = 2;
  test_race.fertilize = 10;

  test_race.adventurism = 0.5;
  test_race.birthrate = 0.1;
  test_race.mass = 1.0;
  test_race.metabolism = 1.0;

  // Initialize conditions array
  for (int i = 0; i <= OTHER; i++) {
    test_race.conditions[i] = i * 10;
  }

  // Initialize likes array
  for (int i = 0; i <= SectorType::SEC_WASTED; i++) {
    test_race.likes[i] = 0.5 + i * 0.1;
  }

  test_race.likesbest = SectorType::SEC_LAND;
  test_race.dissolved = false;
  test_race.God = false;
  test_race.Guest = false;
  test_race.Metamorph = false;
  test_race.monitor = false;

  // Initialize translate array
  for (int i = 0; i < MAXPLAYERS; i++) {
    test_race.translate[i] = i % 10;
  }

  test_race.atwar = 0;
  test_race.allied = 0;
  test_race.Gov_ship = 1000;
  test_race.morale = 100;

  // Initialize points array
  for (int i = 0; i < MAXPLAYERS; i++) {
    test_race.points[i] = i * 100;
  }

  test_race.controlled_planets = 5;
  test_race.victory_turns = 0;
  test_race.turn = 10;
  test_race.tech = 100.0;

  // Initialize discoveries array
  for (int i = 0; i < NUM_DISCOVERIES; i++) {
    test_race.discoveries[i] = (i % 2);  // alternate 0/1
  }

  test_race.victory_score = 1000;
  test_race.votes = true;
  test_race.planet_points = 50;
  test_race.governors = 2;

  // Initialize one governor for testing
  test_race.governor[0].name = "Governor1";
  test_race.governor[0].password = "govpass1";
  test_race.governor[0].active = true;
  test_race.governor[0].deflevel = ScopeLevel::LEVEL_UNIV;
  test_race.governor[0].defsystem = 0;
  test_race.governor[0].defplanetnum = 0;
  test_race.governor[0].homelevel = ScopeLevel::LEVEL_PLAN;
  test_race.governor[0].homesystem = 1;
  test_race.governor[0].homeplanetnum = 1;

  for (int i = 0; i < 4; i++) {
    test_race.governor[0].newspos[i] = i * 1000;
  }

  // Initialize toggle structure
  test_race.governor[0].toggle.invisible = false;
  test_race.governor[0].toggle.standby = false;
  test_race.governor[0].toggle.color = true;
  test_race.governor[0].toggle.gag = false;
  test_race.governor[0].toggle.double_digits = true;
  test_race.governor[0].toggle.inverse = false;
  test_race.governor[0].toggle.geography = true;
  test_race.governor[0].toggle.autoload = true;
  test_race.governor[0].toggle.highlight = 0;
  test_race.governor[0].toggle.compat = true;

  test_race.governor[0].money = 10000;
  test_race.governor[0].income = 500;
  test_race.governor[0].maintain = 100;
  test_race.governor[0].cost_tech = 200;
  test_race.governor[0].cost_market = 50;
  test_race.governor[0].profit_market = 75;
  test_race.governor[0].login = std::time(nullptr);

  // Test serialization to JSON
  auto json_result = race_to_json(test_race);
  assert(json_result.has_value());

  // Test deserialization from JSON
  auto deserialized_race_opt = race_from_json(json_result.value());
  assert(deserialized_race_opt.has_value());

  Race deserialized_race = deserialized_race_opt.value();

  // Verify key fields
  assert(deserialized_race.Playernum == test_race.Playernum);
  assert(deserialized_race.name == test_race.name);
  assert(deserialized_race.IQ == test_race.IQ);
  assert(deserialized_race.tech == test_race.tech);
  assert(deserialized_race.governors == test_race.governors);

  // Verify array fields
  assert(deserialized_race.conditions[0] == test_race.conditions[0]);
  assert(deserialized_race.conditions[OTHER] == test_race.conditions[OTHER]);
  assert(deserialized_race.likes[0] == test_race.likes[0]);
  assert(deserialized_race.discoveries[0] == test_race.discoveries[0]);

  // Verify governor field
  assert(deserialized_race.governor[0].name == test_race.governor[0].name);
  assert(deserialized_race.governor[0].money == test_race.governor[0].money);
  assert(deserialized_race.governor[0].toggle.color ==
         test_race.governor[0].toggle.color);

  std::printf("Race serialization test passed!\n");
  return 0;
}