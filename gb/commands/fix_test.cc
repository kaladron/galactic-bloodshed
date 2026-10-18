// SPDX-License-Identifier: Apache-2.0

/// \file fix_test.cc
/// \brief Unit tests for fix command (deity utilities)

import commands;
import gb.entities;
import gb.services;
import test;
import std;

// Database persistence for fixing ship fuel
void test_fix_ship_fuel_persistence() {
  TestContext ctx;
  ctx.with_standard_universe();

  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE, 1)
      .owned_by(1, 1)
      .with_max_fuel(200.0)
      .with_fuel(50.0)
      .build();

  ctx.em.clear_cache();
  {
    const auto* s = ctx.em.peek_ship(1);
    test::expect_ne(s, nullptr);
    test::expect_eq(s->fuel(), 50.0);
  }

  ctx.em.mutate_ship(1, [](Ship& s) { s.admin_override_fuel(200.0); });

  ctx.em.clear_cache();
  const auto* final_ship = ctx.em.peek_ship(1);
  test::expect_ne(final_ship, nullptr);
  test::expect_eq(final_ship->fuel(), 200.0);

  std::println(std::cout, "✓ fix ship fuel persistence test passed");
}

// Database persistence for fixing ship damage
void test_fix_ship_damage_persistence() {
  TestContext ctx;
  ctx.with_standard_universe();

  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE, 1)
      .owned_by(1, 1)
      .with_damage(75)
      .build();

  ctx.em.clear_cache();
  {
    const auto* s = ctx.em.peek_ship(1);
    test::expect_ne(s, nullptr);
    test::expect_eq(s->damage(), 75);
  }

  ctx.em.mutate_ship(1, [](Ship& s) { s.admin_override_damage(0); });

  ctx.em.clear_cache();
  const auto* final_ship = ctx.em.peek_ship(1);
  test::expect_ne(final_ship, nullptr);
  test::expect_eq(final_ship->damage(), 0);

  std::println(std::cout, "✓ fix ship damage persistence test passed");
}

// Database persistence for fixing ship dead status (hard-deletes ship)
void test_fix_ship_dead_persistence() {
  TestContext ctx;
  ctx.with_standard_universe();

  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE, 1).owned_by(1, 1).build();

  ctx.em.clear_cache();
  {
    const auto* s = ctx.em.peek_ship(1);
    test::expect_ne(s, nullptr);
    test::expect_true(s->alive());
    test::expect_eq(s->damage(), 0);
  }

  ctx.em.mutate_ship(1, [&](Ship& s) {
    s.admin_destroy();
    ctx.em.kill_ship(1, s);
  });

  ctx.em.clear_cache();
  test::expect_throws<EntityNotFoundError>([&]() { ctx.em.peek_ship(1); });

  std::println(std::cout, "✓ fix ship dead persistence test passed");
}

// Database persistence for fixing planet temperature
void test_fix_planet_temp_persistence() {
  TestContext ctx;
  ctx.with_standard_universe();
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.temp() = 50; });

  ctx.em.clear_cache();
  {
    const auto* p = ctx.em.peek_planet(1, 1);
    test::expect_ne(p, nullptr);
    test::expect_eq(p->temp(), 50);
  }

  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.temp() = 100; });

  ctx.em.clear_cache();
  const auto* final_planet = ctx.em.peek_planet(1, 1);
  test::expect_ne(final_planet, nullptr);
  test::expect_eq(final_planet->temp(), 100);

  std::println(std::cout, "✓ fix planet temperature persistence test passed");
}

// Database persistence for fixing planet oxygen
void test_fix_planet_oxygen_persistence() {
  TestContext ctx;
  ctx.with_standard_universe();
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.conditions().oxygen = 10; });

  ctx.em.clear_cache();
  {
    const auto* p = ctx.em.peek_planet(1, 1);
    test::expect_ne(p, nullptr);
    test::expect_eq(p->conditions(OXYGEN), 10);
  }

  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.conditions().oxygen = 50; });

  ctx.em.clear_cache();
  const auto* final_planet = ctx.em.peek_planet(1, 1);
  test::expect_ne(final_planet, nullptr);
  test::expect_eq(final_planet->conditions(OXYGEN), 50);

  std::println(std::cout, "✓ fix planet oxygen persistence test passed");
}

// Database persistence for fixing planet position
void test_fix_planet_position_persistence() {
  TestContext ctx;
  ctx.with_standard_universe();
  ctx.em.mutate_planet(
      1, 1, [](Planet& p) { p.set_system_coordinates({100.0, 200.0}); });

  ctx.em.clear_cache();
  {
    const auto* p = ctx.em.peek_planet(1, 1);
    test::expect_ne(p, nullptr);
    test::expect_eq(p->system_coordinates(), SystemCoordinates{100.0, 200.0});
  }

  ctx.em.mutate_planet(
      1, 1, [](Planet& p) { p.set_system_coordinates({500.0, 600.0}); });

  ctx.em.clear_cache();
  const auto* final_planet = ctx.em.peek_planet(1, 1);
  test::expect_ne(final_planet, nullptr);
  test::expect_eq(final_planet->system_coordinates(),
                  SystemCoordinates{500.0, 600.0});

  std::println(std::cout, "✓ fix planet position persistence test passed");
}

void test_fix_command_dispatch() {
  TestContext ctx;
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) { r.God = true; });
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.temp() = 50; });

  // Create ship
  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE, 1)
      .owned_by(1, 1)
      .with_max_fuel(200.0)
      .with_fuel(50.0)
      .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Mortal rejection (Player 2 is non-deity in standard universe)
  ctx.setup_game_obj(g, 2, 1);
  ctx.assert_dispatch_rejected(g, {"fix", "planet", "temperature", "100"});
  test::expect_contains(g.out.str(), "Only deity can use this command");

  // 2. Deity happy path - planet fix and read-only inspection
  ctx.setup_game_obj(g, 1, 1);
  g.set_god(true);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"fix", "planet", "temperature", "100"});
  test::expect_contains(g.out.str(), "temperature = 100");
  ctx.assert_dispatch_success(g, {"fix", "planet", "temperature"});
  test::expect_contains(g.out.str(), "temperature = 100");

  // 3. Deity happy path - ship fix and read-only inspection
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"fix", "ship", "fuel", "200"});
  test::expect_contains(g.out.str(), "fuel = 200");
  ctx.assert_dispatch_success(g, {"fix", "ship", "fuel"});
  test::expect_contains(g.out.str(), "fuel = 200");

  // 4. Min args check (< 3 args)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fix", "planet"});
  test::expect_contains(g.out.str(),
                        "Syntax: fix <planet|ship> <property> [<value>]");

  // 5. Unknown target ("Fix what?")
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fix", "star", "temperature"});
  test::expect_contains(g.out.str(), "Fix what?");

  // 6. Planet scope error & invalid numeric value & all condition options
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fix", "planet", "temperature", "100"});
  test::expect_contains(g.out.str(), "Change scope to the planet first.");

  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fix", "planet", "temperature", "abc"});
  test::expect_contains(g.out.str(), "Invalid numeric value.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fix", "planet", "nonexistent", "10"});
  test::expect_contains(g.out.str(), "No such option for 'fix planet'.");

  // Exercise xpos, ypos, rtemp, toxic (both set and read-only)
  ctx.assert_dispatch_success(g, {"fix", "planet", "xpos", "250"});
  test::expect_contains(g.out.str(), "xpos = 250");
  ctx.assert_dispatch_success(g, {"fix", "planet", "xpos"});
  test::expect_contains(g.out.str(), "xpos = 250");

  ctx.assert_dispatch_success(g, {"fix", "planet", "ypos", "-125"});
  test::expect_contains(g.out.str(), "ypos = -125");
  ctx.assert_dispatch_success(g, {"fix", "planet", "ypos"});
  test::expect_contains(g.out.str(), "ypos = -125");

  ctx.assert_dispatch_success(g, {"fix", "planet", "rtemp", "42"});
  test::expect_contains(g.out.str(), "rtemp = 42");
  ctx.assert_dispatch_success(g, {"fix", "planet", "rtemp"});
  test::expect_contains(g.out.str(), "rtemp = 42");

  ctx.assert_dispatch_success(g, {"fix", "planet", "toxic", "35"});
  test::expect_contains(g.out.str(), "toxic = 35");
  ctx.assert_dispatch_success(g, {"fix", "planet", "toxic"});
  test::expect_contains(g.out.str(), "toxic = 35");

  for (AtmosphereConditions cond : all_atmosphere_conditions) {
    const std::string opt{to_string(cond)};
    const int val = static_cast<int>(cond) + 10;
    g.out.str("");
    ctx.assert_dispatch_success(g, {"fix", "planet", opt, std::to_string(val)});
    test::expect_contains(g.out.str(), std::format("{} = {}", cond, val));

    // Read-only inspection without value argument
    g.out.str("");
    ctx.assert_dispatch_success(g, {"fix", "planet", opt});
    test::expect_contains(g.out.str(), std::format("{} = {}", cond, val));
  }

  // 7. Ship scope error, invalid numeric value, and all ship options
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fix", "ship", "fuel", "100"});
  test::expect_contains(g.out.str(),
                        "Change scope to the ship you wish to fix.");

  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fix", "ship", "fuel", "notanumber"});
  test::expect_contains(g.out.str(), "Invalid numeric value.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fix", "ship", "shields", "10"});
  test::expect_contains(g.out.str(), "No such option for 'fix ship'.");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"fix", "ship", "max_fuel", "350"});
  test::expect_contains(g.out.str(), "fuel = 350");
  ctx.assert_dispatch_success(g, {"fix", "ship", "max_fuel"});
  test::expect_contains(g.out.str(), "fuel = 350");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"fix", "ship", "destruct", "2"});
  test::expect_contains(g.out.str(), "destruct = 2");
  ctx.assert_dispatch_success(g, {"fix", "ship", "destruct"});
  test::expect_contains(g.out.str(), "destruct = 2");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"fix", "ship", "resource", "20"});
  test::expect_contains(g.out.str(), "resource = 20");
  ctx.assert_dispatch_success(g, {"fix", "ship", "resource"});
  test::expect_contains(g.out.str(), "resource = 20");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"fix", "ship", "damage", "35"});
  test::expect_contains(g.out.str(), "damage = 35");
  ctx.assert_dispatch_success(g, {"fix", "ship", "damage"});
  test::expect_contains(g.out.str(), "damage = 35");

  // 8. JSON mode for property inspection and ship destruction
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"fix", "ship", "damage", "10"});
  test::expect_contains(g.out.str(), "\"type\":\"fix\"");
  test::expect_contains(g.out.str(), "\"property\":\"damage\"");
  test::expect_contains(g.out.str(), "\"value\":10");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"fix", "ship", "dead"});
  test::expect_contains(g.out.str(), "\"type\":\"fix\"");
  test::expect_contains(g.out.str(), "\"ship_destroyed\":true");
  test::expect_throws<EntityNotFoundError>([&]() { ctx.em.peek_ship(1); });

  // Also verify ASCII "destroyed" output on a second ship
  g.set_ui_mode(UiMode::ASCII);
  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE, 2).owned_by(1, 1).build();
  g.set_shipno(2);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"fix", "ship", "dead"});
  test::expect_contains(g.out.str(), "destroyed");
}

int main() {
  test_fix_ship_fuel_persistence();
  test_fix_ship_damage_persistence();
  test_fix_ship_dead_persistence();
  test_fix_planet_temp_persistence();
  test_fix_planet_oxygen_persistence();
  test_fix_planet_position_persistence();
  test_fix_command_dispatch();

  std::println(std::cout, "\n✅ All fix tests passed!");
  return 0;
}
