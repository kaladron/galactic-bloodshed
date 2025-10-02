// SPDX-License-Identifier: Apache-2.0

import gblib;
import std.compat;

#include <sqlite3.h>

#include <cassert>
#include <sstream>

namespace {

// Helper to capture output from GameObj
std::string captureOutput(GameObj& g) {
  return g.out.str();
}

// Helper to create a test GameObj with output stream
GameObj createTestGameObj() {
  GameObj g{};
  g.player = 1;
  g.governor = 0;
  g.level = ScopeLevel::LEVEL_UNIV;
  return g;
}

// Helper to create a basic test ship
Ship createTestShip(ShipType type = ShipType::OTYPE_CARGO) {
  Ship ship{};
  ship.owner = 1;
  ship.number = 100;
  ship.type = type;
  ship.active = 1;
  ship.alive = 1;
  ship.popn = 100;
  ship.mass = 100.0;
  ship.tech = 100.0;
  return ship;
}

// Test speed order handler via give_orders
void test_order_speed() {
  auto g = createTestGameObj();
  auto ship = createTestShip();
  ship.max_speed = 9;
  ship.speed = 3;
  
  command_t argv = {"order", "100", "speed", "5"};
  
  // Call give_orders which dispatches to handle_order_speed
  give_orders(g, argv, 1, ship);
  
  // Verify speed was set correctly
  assert(ship.speed == 5);
  std::cout << "  PASS: speed order sets ship speed correctly\n";
}

// Test speed order with invalid (too high) speed
void test_order_speed_clamped() {
  auto g = createTestGameObj();
  auto ship = createTestShip();
  ship.max_speed = 5;
  ship.speed = 0;
  
  command_t argv = {"order", "100", "speed", "10"};
  
  give_orders(g, argv, 1, ship);
  
  // Speed should be clamped to max_speed
  assert(ship.speed == 5);
  std::cout << "  PASS: speed order clamps to max speed\n";
}

// Test speed order rejects negative speed
void test_order_speed_negative() {
  auto g = createTestGameObj();
  auto ship = createTestShip();
  ship.max_speed = 5;
  ship.speed = 3;
  
  command_t argv = {"order", "100", "speed", "-1"};
  
  give_orders(g, argv, 1, ship);
  
  // Speed should remain unchanged
  assert(ship.speed == 3);
  std::string output = captureOutput(g);
  assert(output.find("positive speed") != std::string::npos);
  std::cout << "  PASS: speed order rejects negative values\n";
}

// Test merchant order
void test_order_merchant() {
  auto g = createTestGameObj();
  auto ship = createTestShip();
  ship.merchant = 0;
  
  command_t argv = {"order", "100", "merchant", "3"};
  
  give_orders(g, argv, 1, ship);
  
  assert(ship.merchant == 3);
  std::cout << "  PASS: merchant order sets route number\n";
}

// Test merchant order off
void test_order_merchant_off() {
  auto g = createTestGameObj();
  auto ship = createTestShip();
  ship.merchant = 5;
  
  command_t argv = {"order", "100", "merchant", "off"};
  
  give_orders(g, argv, 1, ship);
  
  assert(ship.merchant == 0);
  std::cout << "  PASS: merchant order turns off routing\n";
}

// Test scatter order for missiles
void test_order_scatter() {
  auto g = createTestGameObj();
  auto ship = createTestShip(ShipType::STYPE_MISSILE);
  
  command_t argv = {"order", "100", "scatter"};
  
  give_orders(g, argv, 1, ship);
  
  // Verify scatter was set in special data
  assert(std::holds_alternative<ImpactData>(ship.special));
  auto impact = std::get<ImpactData>(ship.special);
  assert(impact.scatter == 1);
  std::cout << "  PASS: scatter order sets missile scatter mode\n";
}

// Test scatter order rejects non-missiles
void test_order_scatter_wrong_type() {
  auto g = createTestGameObj();
  auto ship = createTestShip(ShipType::OTYPE_BATTLESHIP);
  
  command_t argv = {"order", "100", "scatter"};
  
  give_orders(g, argv, 1, ship);
  
  std::string output = captureOutput(g);
  assert(output.find("Only missiles") != std::string::npos);
  std::cout << "  PASS: scatter order rejects non-missile ships\n";
}

// Test impact order for missiles
void test_order_impact() {
  auto g = createTestGameObj();
  auto ship = createTestShip(ShipType::STYPE_MISSILE);
  
  command_t argv = {"order", "100", "impact", "15,25"};
  
  give_orders(g, argv, 1, ship);
  
  assert(std::holds_alternative<ImpactData>(ship.special));
  auto impact = std::get<ImpactData>(ship.special);
  assert(impact.x == 15);
  assert(impact.y == 25);
  assert(impact.scatter == 0);
  std::cout << "  PASS: impact order sets missile target coordinates\n";
}

// Test explosive mode for mines
void test_order_explosive() {
  auto g = createTestGameObj();
  auto ship = createTestShip(ShipType::STYPE_MINE);
  ship.mode = 1;
  
  command_t argv = {"order", "100", "explosive"};
  
  give_orders(g, argv, 1, ship);
  
  assert(ship.mode == 0);
  std::cout << "  PASS: explosive order sets mine to explosive mode\n";
}

// Test radiative mode for mines
void test_order_radiative() {
  auto g = createTestGameObj();
  auto ship = createTestShip(ShipType::STYPE_MINE);
  ship.mode = 0;
  
  command_t argv = {"order", "100", "radiative"};
  
  give_orders(g, argv, 1, ship);
  
  assert(ship.mode == 1);
  std::cout << "  PASS: radiative order sets mine to radiative mode\n";
}

// Test trigger radius for mines
void test_order_trigger() {
  auto g = createTestGameObj();
  auto ship = createTestShip(ShipType::STYPE_MINE);
  
  command_t argv = {"order", "100", "trigger", "50"};
  
  give_orders(g, argv, 1, ship);
  
  assert(std::holds_alternative<TriggerData>(ship.special));
  auto trigger = std::get<TriggerData>(ship.special);
  assert(trigger.radius == 50);
  std::cout << "  PASS: trigger order sets mine trigger radius\n";
}

// Test navigate order
void test_order_navigate() {
  auto g = createTestGameObj();
  auto ship = createTestShip();
  ship.navigate.on = 0;
  
  command_t argv = {"order", "100", "navigate", "45", "10"};
  
  give_orders(g, argv, 1, ship);
  
  assert(ship.navigate.on == 1);
  assert(ship.navigate.bearing == 45);
  assert(ship.navigate.turns == 10);
  std::cout << "  PASS: navigate order sets bearing and turns\n";
}

// Test navigate off
void test_order_navigate_off() {
  auto g = createTestGameObj();
  auto ship = createTestShip();
  ship.navigate.on = 1;
  ship.navigate.bearing = 90;
  
  command_t argv = {"order", "100", "navigate"};
  
  give_orders(g, argv, 1, ship);
  
  assert(ship.navigate.on == 0);
  std::cout << "  PASS: navigate order with no args turns navigation off\n";
}

// Test focus on
void test_order_focus() {
  auto g = createTestGameObj();
  auto ship = createTestShip();
  ship.laser = 1;
  ship.focus = 0;
  
  command_t argv = {"order", "100", "focus", "on"};
  
  give_orders(g, argv, 1, ship);
  
  assert(ship.focus == 1);
  std::cout << "  PASS: focus order enables laser focus\n";
}

// Test focus off
void test_order_focus_off() {
  auto g = createTestGameObj();
  auto ship = createTestShip();
  ship.laser = 1;
  ship.focus = 1;
  
  command_t argv = {"order", "100", "focus", "off"};
  
  give_orders(g, argv, 1, ship);
  
  assert(ship.focus == 0);
  std::cout << "  PASS: focus order disables laser focus\n";
}

// Test evade on
void test_order_evade() {
  auto g = createTestGameObj();
  auto ship = createTestShip();
  ship.max_crew = 100;
  ship.max_speed = 5;
  ship.protect.evade = 0;
  
  command_t argv = {"order", "100", "evade", "on"};
  
  give_orders(g, argv, 1, ship);
  
  assert(ship.protect.evade == 1);
  std::cout << "  PASS: evade order enables evasion\n";
}

// Test bombard on
void test_order_bombard() {
  auto g = createTestGameObj();
  auto ship = createTestShip(ShipType::OTYPE_BATTLESHIP);
  ship.guns = PRIMARY;
  ship.primary = 10;
  ship.bombard = 0;
  
  command_t argv = {"order", "100", "bombard", "on"};
  
  give_orders(g, argv, 1, ship);
  
  assert(ship.bombard == 1);
  std::cout << "  PASS: bombard order enables bombardment\n";
}

// Test retaliate on
void test_order_retaliate() {
  auto g = createTestGameObj();
  auto ship = createTestShip(ShipType::OTYPE_BATTLESHIP);
  ship.guns = PRIMARY;
  ship.primary = 10;
  ship.protect.self = 0;
  
  command_t argv = {"order", "100", "retaliate", "on"};
  
  give_orders(g, argv, 1, ship);
  
  assert(ship.protect.self == 1);
  std::cout << "  PASS: retaliate order enables retaliation\n";
}

// Test primary gun selection
void test_order_primary() {
  auto g = createTestGameObj();
  auto ship = createTestShip(ShipType::OTYPE_BATTLESHIP);
  ship.primary = 10;
  ship.guns = SECONDARY;
  
  command_t argv = {"order", "100", "primary"};
  
  give_orders(g, argv, 1, ship);
  
  assert(ship.guns == PRIMARY);
  std::cout << "  PASS: primary order selects primary guns\n";
}

// Test secondary gun selection
void test_order_secondary() {
  auto g = createTestGameObj();
  auto ship = createTestShip(ShipType::OTYPE_BATTLESHIP);
  ship.secondary = 5;
  ship.guns = PRIMARY;
  
  command_t argv = {"order", "100", "secondary"};
  
  give_orders(g, argv, 1, ship);
  
  assert(ship.guns == SECONDARY);
  std::cout << "  PASS: secondary order selects secondary guns\n";
}

// Test salvo setting
void test_order_salvo() {
  auto g = createTestGameObj();
  auto ship = createTestShip(ShipType::OTYPE_BATTLESHIP);
  ship.guns = PRIMARY;
  ship.primary = 10;
  ship.retaliate = 0;
  
  command_t argv = {"order", "100", "salvo", "7"};
  
  give_orders(g, argv, 1, ship);
  
  assert(ship.retaliate == 7);
  std::cout << "  PASS: salvo order sets retaliation gun count\n";
}

// Test inactive ship rejection
void test_inactive_ship() {
  auto g = createTestGameObj();
  auto ship = createTestShip();
  ship.active = 0;
  ship.rad = 50;
  ship.speed = 0;
  
  command_t argv = {"order", "100", "speed", "5"};
  
  give_orders(g, argv, 1, ship);
  
  // Speed should remain unchanged for inactive ship
  assert(ship.speed == 0);
  std::string output = captureOutput(g);
  assert(output.find("irradiated") != std::string::npos);
  std::cout << "  PASS: inactive ships are rejected\n";
}

// Test ship with no crew rejection
void test_no_crew_ship() {
  auto g = createTestGameObj();
  auto ship = createTestShip();
  ship.popn = 0;
  ship.max_crew = 100;  // Requires crew
  ship.speed = 0;
  
  command_t argv = {"order", "100", "speed", "5"};
  
  give_orders(g, argv, 1, ship);
  
  assert(ship.speed == 0);
  std::string output = captureOutput(g);
  assert(output.find("no crew") != std::string::npos);
  std::cout << "  PASS: ships without crew are rejected\n";
}

}  // namespace

int main() {
  // Initialize database for testing
  Sql db(":memory:");
  initsqldata();
  
  try {
    std::cout << "Running order handler tests...\n\n";
    
    // Speed tests
    test_order_speed();
    test_order_speed_clamped();
    test_order_speed_negative();
    
    // Merchant tests
    test_order_merchant();
    test_order_merchant_off();
    
    // Missile tests
    test_order_scatter();
    test_order_scatter_wrong_type();
    test_order_impact();
    
    // Mine tests
    test_order_explosive();
    test_order_radiative();
    test_order_trigger();
    
    // Navigation tests
    test_order_navigate();
    test_order_navigate_off();
    
    // Combat tests
    test_order_focus();
    test_order_focus_off();
    test_order_evade();
    test_order_bombard();
    test_order_retaliate();
    test_order_primary();
    test_order_secondary();
    test_order_salvo();
    
    // Validation tests
    test_inactive_ship();
    test_no_crew_ship();
    
    std::cout << "\n✅ All order handler tests passed!\n";
    std::cout << "Tested 24 different order types and validation cases.\n";
    
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "❌ Test failed with exception: " << e.what() << "\n";
    return 1;
  }
}
