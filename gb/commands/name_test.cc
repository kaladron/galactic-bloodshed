// SPDX-License-Identifier: Apache-2.0

/// \file name_test.cc
/// \brief Test name command database persistence and validation rules

import gb.entities;
import gb.services;
import test;
import commands;
import std;

void test_name_ship_persistence() {
  std::println(std::cout, "Test: name command - ship naming");

  TestContext ctx;
  ctx.with_standard_universe();

  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE, 1)
      .owned_by(1, 1)
      .named("Old Ship Name")
      .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);

  std::println(std::cout, "  Testing: Rename ship to 'USS Enterprise'");
  {
    ctx.assert_dispatch_success(g, {"name", "ship", "USS", "Enterprise"});
    test::expect_contains(g.out.str(), "Name set.");
    std::println(std::cout, "    ✓ Output message correct");

    ctx.em.clear_cache();
    const auto* saved = ctx.em.peek_ship(1);
    test::expect_ne(saved, nullptr);
    test::expect_eq(saved->name(), "USS Enterprise");
    std::println(std::cout, "    ✓ Database: ship name = '{}'", saved->name());
  }

  std::println(std::cout, "  ✅ Ship naming test passed!");
}

void test_name_race_persistence() {
  std::println(std::cout, "Test: name command - race naming");

  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_governor(1);  // Must be leader (governor 1)
  g.set_level(ScopeLevel::LEVEL_UNIV);

  std::println(std::cout, "  Testing: Rename race to 'Klingons'");
  {
    ctx.assert_dispatch_success(g, {"name", "race", "Klingons"});

    ctx.em.clear_cache();
    const auto* saved = ctx.em.peek_race(1);
    test::expect_ne(saved, nullptr);
    test::expect_eq(saved->name, "Klingons");
    std::println(std::cout, "    ✓ Database: race name = '{}'", saved->name);
  }

  std::println(std::cout, "  ✅ Race naming test passed!");
}

void test_name_star_persistence() {
  std::println(std::cout, "Test: name command - star naming");

  TestContext ctx;
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) { r.God = true; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  g.set_god(true);

  std::println(std::cout, "  Testing: Rename star to 'Alpha Centauri'");
  {
    ctx.assert_dispatch_success(g, {"name", "star", "Alpha", "Centauri"});

    ctx.em.clear_cache();
    const auto* saved = ctx.em.peek_star(1);
    test::expect_ne(saved, nullptr);
    test::expect_eq(saved->get_name(), "Alpha Centauri");
    std::println(std::cout, "    ✓ Database: star name = '{}'",
                 saved->get_name());
  }

  std::println(std::cout, "  ✅ Star naming test passed!");
}

void test_name_planet_persistence() {
  std::println(std::cout, "Test: name command - planet naming");

  TestContext ctx;
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) { r.God = true; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);
  g.set_god(true);

  std::println(std::cout, "  Testing: Rename planet to 'New Earth'");
  {
    ctx.assert_dispatch_success(g, {"name", "planet", "New", "Earth"});

    ctx.em.clear_cache();
    const auto* saved = ctx.em.peek_star(1);
    test::expect_ne(saved, nullptr);
    test::expect_eq(saved->get_planet_name(1), "New Earth");
    std::println(std::cout, "    ✓ Database: planet name = '{}'",
                 saved->get_planet_name(1));
  }

  std::println(std::cout, "  ✅ Planet naming test passed!");
}

void test_name_invalid_formats() {
  std::println(std::cout, "Test: name command - invalid input formats");

  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // TEST: Less than 3 arguments
  {
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"name", "ship"});
  }

  // TEST: Empty string in argv[2] -> "Illegal name format."
  {
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"name", "ship", ""});
    test::expect_contains(g.out.str(), "Illegal name format.");
  }

  // TEST: First char of name not alphanumeric -> "Illegal name format."
  {
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"name", "ship", "!invalid"});
    test::expect_contains(g.out.str(), "Illegal name format.");
  }

  // TEST: Name containing slash or invalid special character -> "Illegal name
  // form."
  {
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"name", "race", "Val/halla"});
    test::expect_contains(g.out.str(), "Illegal name form.");
  }

  // TEST: All spaces name (first character is space, fails isalnum) -> "Illegal
  // name format."
  {
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"name", "race", " ", " "});
    test::expect_contains(g.out.str(), "Illegal name format.");
  }

  std::println(std::cout, "  ✅ Invalid format validation test passed!");
}

void test_name_governor() {
  std::println(std::cout, "Test: name command - governor naming");

  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_governor(1);

  // TEST: Rename governor 1 to 'Grand Moff'
  {
    g.out.str("");
    ctx.assert_dispatch_success(g, {"name", "governor", "Grand", "Moff"});
    test::expect_contains(g.out.str(), "Name changed to `Grand Moff'.");

    ctx.em.clear_cache();
    const auto* saved = ctx.em.peek_race(1);
    test::expect_ne(saved, nullptr);
    test::expect_eq(saved->leader().name, "Grand Moff");
  }

  std::println(std::cout, "  ✅ Governor naming test passed!");
}

void test_name_block() {
  std::println(std::cout,
               "Test: name command - block naming and authorization");

  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // TEST: Leader (governor 1) can rename alliance block
  {
    g.set_governor(1);
    g.out.str("");
    ctx.assert_dispatch_success(g, {"name", "block", "United", "Federation"});
    test::expect_contains(g.out.str(), "Done.");

    const auto* saved = ctx.em.peek_block(1);
    test::expect_ne(saved, nullptr);
    test::expect_eq(saved->name(), "United Federation");
  }

  // TEST: Missing block throws EntityNotFoundError -> "Block not found."
  {
    g.set_player(99);
    g.set_governor(1);
    g.out.str("");
    test::expect_false(
        GB::commands::name({"name", "block", "Missing", "Block"}, g));
    test::expect_contains(g.out.str(), "Block not found.");
    g.set_player(1);
  }

  // TEST: Non-leader governor (governor 2) is rejected
  {
    g.set_governor(2);
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"name", "block", "Rebel", "Alliance"});
    test::expect_contains(g.out.str(), "You are not authorized to do this.");
  }

  std::println(std::cout, "  ✅ Block naming test passed!");
}

void test_name_factory_class() {
  std::println(std::cout, "Test: name command - factory ship class naming");

  TestContext ctx;
  ctx.with_standard_universe();

  TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY, 1).owned_by(1, 1).build();
  ctx.em.mutate_ship(1, [](Ship& f) {
    f.on() = false;
    f.shipclass() = "Basic";
  });

  TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER, 2).owned_by(1, 1).build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_SHIP);

  // TEST: Name class at factory off-line -> success
  {
    g.set_shipno(1);
    g.out.str("");
    ctx.assert_dispatch_success(g, {"name", "class", "Battleship"});
    test::expect_contains(g.out.str(), "Class set.");

    ctx.em.clear_cache();
    const auto* saved = ctx.em.peek_ship(1);
    test::expect_ne(saved, nullptr);
    test::expect_eq(saved->shipclass(), "Battleship");
  }

  // TEST: Name class when factory is on-line -> error
  {
    ctx.em.mutate_ship(1, [](Ship& f) { f.on() = true; });
    g.set_shipno(1);
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"name", "class", "Cruiser"});
    test::expect_contains(g.out.str(), "This factory is already on line.");
  }

  // TEST: Name class at non-factory ship -> error
  {
    g.set_shipno(2);
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"name", "class", "Destroyer"});
    test::expect_contains(g.out.str(), "You are not at a factory!");
  }

  // TEST: Name class when not at ship scope -> error
  {
    g.set_level(ScopeLevel::LEVEL_UNIV);
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"name", "class", "Destroyer"});
    test::expect_contains(
        g.out.str(), "You have to 'cs' to a factory to name the ship class.");
  }

  std::println(std::cout, "  ✅ Factory class naming test passed!");
}

void test_name_permissions_scope_and_json() {
  std::println(std::cout,
               "Test: name command - permissions, scope checks, and JSON");

  TestContext ctx;
  ctx.with_standard_universe();

  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE, 1).owned_by(1, 1).build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // TEST: Non-god naming star -> rejected
  {
    g.set_level(ScopeLevel::LEVEL_STAR);
    g.set_snum(1);
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"name", "star", "Forbidden"});
    test::expect_contains(g.out.str(), "Only dieties may name a star.");
  }

  // TEST: Naming star when not at star scope -> rejected
  {
    g.set_level(ScopeLevel::LEVEL_UNIV);
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"name", "star", "Forbidden"});
    test::expect_contains(g.out.str(),
                          "You have to 'cs' to a star to name it.");
  }

  // TEST: Non-god naming planet -> rejected
  {
    g.set_level(ScopeLevel::LEVEL_PLAN);
    g.set_snum(1);
    g.set_pnum(1);
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"name", "planet", "Forbidden"});
    test::expect_contains(g.out.str(), "Only deity can rename planets.");
  }

  // TEST: Naming planet when not at planet scope -> rejected
  {
    g.set_level(ScopeLevel::LEVEL_UNIV);
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"name", "planet", "Forbidden"});
    test::expect_contains(g.out.str(),
                          "You have to 'cs' to a planet to name it.");
  }

  // TEST: Non-leader naming race -> rejected
  {
    g.set_governor(2);
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"name", "race", "Forbidden"});
    test::expect_contains(g.out.str(), "You are not authorized to do this.");
    g.set_governor(1);
  }

  // TEST: Naming ship when not at ship level -> wrong scope message
  {
    g.set_level(ScopeLevel::LEVEL_UNIV);
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"name", "ship", "Enterprise"});
    test::expect_contains(g.out.str(),
                          "You have to 'cs' to a ship to name it.");
  }

  // TEST: Unknown target -> "I don't know what you mean."
  {
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"name", "sector", "Alpha"});
    test::expect_contains(g.out.str(), "I don't know what you mean.");
  }

  // TEST: JSON mode output
  {
    g.set_ui_mode(UiMode::JSON);
    g.set_level(ScopeLevel::LEVEL_SHIP);
    g.set_shipno(1);
    g.out.str("");
    ctx.assert_dispatch_success(g, {"name", "ship", "Defiant"});
    test::expect_contains(g.out.str(), "\"type\":\"name\"");
    test::expect_contains(g.out.str(), "\"name\":\"Defiant\"");
  }

  std::println(std::cout,
               "  ✅ Permissions, scope, and JSON mode test passed!");
}

int main() {
  test_name_ship_persistence();
  test_name_race_persistence();
  test_name_star_persistence();
  test_name_planet_persistence();
  test_name_invalid_formats();
  test_name_governor();
  test_name_block();
  test_name_factory_class();
  test_name_permissions_scope_and_json();

  std::println(std::cout, "\n✅ All name tests passed!");
  return 0;
}
