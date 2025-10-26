// SPDX-License-Identifier: Apache-2.0

import gblib;
import std.compat;

#include <cassert>

int main() {
  // Create a test sector with various values
  Sector test_sector{};
  test_sector.x = 10;
  test_sector.y = 20;
  test_sector.eff = 75;
  test_sector.fert = 50;
  test_sector.mobilization = 25;
  test_sector.crystals = 100;
  test_sector.resource = 500;
  test_sector.popn = 10000;
  test_sector.troops = 250;
  test_sector.owner = 1;
  test_sector.race = 1;
  test_sector.type = SectorType::SEC_LAND;
  test_sector.condition = 0;

  // Test serialization to JSON
  auto json_result = sector_to_json(test_sector);
  assert(json_result.has_value());
  
  std::println("JSON output: {}", json_result.value());

  // Test deserialization from JSON
  auto deserialized_sector_opt = sector_from_json(json_result.value());
  assert(deserialized_sector_opt.has_value());

  Sector deserialized_sector = std::move(deserialized_sector_opt.value());

  // Verify all fields
  assert(deserialized_sector.x == test_sector.x);
  assert(deserialized_sector.y == test_sector.y);
  assert(deserialized_sector.eff == test_sector.eff);
  assert(deserialized_sector.fert == test_sector.fert);
  assert(deserialized_sector.mobilization == test_sector.mobilization);
  assert(deserialized_sector.crystals == test_sector.crystals);
  assert(deserialized_sector.resource == test_sector.resource);
  assert(deserialized_sector.popn == test_sector.popn);
  assert(deserialized_sector.troops == test_sector.troops);
  assert(deserialized_sector.owner == test_sector.owner);
  assert(deserialized_sector.race == test_sector.race);
  assert(deserialized_sector.type == test_sector.type);
  assert(deserialized_sector.condition == test_sector.condition);

  std::println("Sector serialization test passed!");
  return 0;
}
