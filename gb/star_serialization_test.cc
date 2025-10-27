// SPDX-License-Identifier: Apache-2.0

import gblib;
import std.compat;

#include <cassert>
#include <cstring>

int main() {
  star_struct test_star{};

  // Initialize scalar fields
  test_star.ships = 42;
  std::strncpy(test_star.name, "TestStar", NAMESIZE - 1);
  test_star.xpos = 100.5;
  test_star.ypos = 200.75;
  test_star.numplanets = 5;
  test_star.stability = 10;
  test_star.nova_stage = 0;
  test_star.temperature = 15;
  test_star.gravity = 1.0;
  test_star.star_id = 123;

  // Initialize governor array
  for (int i = 0; i < MAXPLAYERS; i++) {
    test_star.governor[i] = i + 1;
  }

  // Initialize AP array
  for (int i = 0; i < MAXPLAYERS; i++) {
    test_star.AP[i] = i * 100;
  }

  // Initialize explored and inhabited bitmasks
  test_star.explored = 0b101010;   // Binary pattern for testing
  test_star.inhabited = 0b110011;  // Binary pattern for testing

  // Initialize planet names
  for (int i = 0; i < MAXPLANETS; i++) {
    std::snprintf(test_star.pnames[i], NAMESIZE, "Planet%d", i);
  }

  // Initialize dummy array
  for (int i = 0; i < 1; i++) {
    test_star.dummy[i] = 0;
  }

  // Test serialization to JSON
  auto json_result = star_to_json(test_star);
  assert(json_result.has_value());

  std::printf("JSON output:\n%s\n", json_result.value().c_str());

  // Test deserialization from JSON
  auto deserialized_star_opt = star_from_json(json_result.value());
  assert(deserialized_star_opt.has_value());

  star_struct deserialized_star = deserialized_star_opt.value();

  // Verify scalar fields
  assert(deserialized_star.ships == test_star.ships);
  assert(std::strcmp(deserialized_star.name, test_star.name) == 0);
  assert(deserialized_star.xpos == test_star.xpos);
  assert(deserialized_star.ypos == test_star.ypos);
  assert(deserialized_star.numplanets == test_star.numplanets);
  assert(deserialized_star.stability == test_star.stability);
  assert(deserialized_star.nova_stage == test_star.nova_stage);
  assert(deserialized_star.temperature == test_star.temperature);
  assert(deserialized_star.gravity == test_star.gravity);
  assert(deserialized_star.star_id == test_star.star_id);

  // Verify governor array
  for (int i = 0; i < MAXPLAYERS; i++) {
    assert(deserialized_star.governor[i] == test_star.governor[i]);
  }

  // Verify AP array
  for (int i = 0; i < MAXPLAYERS; i++) {
    assert(deserialized_star.AP[i] == test_star.AP[i]);
  }

  // Verify bitmasks
  assert(deserialized_star.explored == test_star.explored);
  assert(deserialized_star.inhabited == test_star.inhabited);

  // Verify planet names
  for (int i = 0; i < MAXPLANETS; i++) {
    assert(std::strcmp(deserialized_star.pnames[i], test_star.pnames[i]) == 0);
  }

  std::println("Star serialization test passed!");
  return 0;
}
