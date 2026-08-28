// SPDX-License-Identifier: Apache-2.0

/// \file name_test.cc
/// \brief Test name command database persistence and validation rules

import dallib;
import gb.entities;
import gb.services;
import test;
import commands;
import std;

void test_name_ship_persistence() {
  std::println(std::cout, "Test: name command - ship naming");

  // Create in-memory database
  TestContext ctx;

  // Setup: Create a ship
  Ship ship{};
  ship.number() = 1;
  ship.name() = "Old Ship Name";

  // Setup: Create a race for player 1
  Race race{};
  race.Playernum = 1;
  race.name = "Test Race";

  JsonStore store(ctx.db);
  ShipRepository ships(store);
  RaceRepository races(store);
  ships.save(ship);
  races.save(race);

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);
  g.race =
      ctx.em.peek_race(g.player());  // Set race pointer like production does

  // TEST: Rename ship
  std::println(std::cout, "  Testing: Rename ship to 'USS Enterprise'");
  {
    ctx.assert_dispatch_success(g, {"name", "ship", "USS", "Enterprise"});

    // Verify output message
    std::string out_str = g.out.str();
    test::expect_contains(out_str, "Name set.");
    std::println(std::cout, "    ✓ Output message correct");

    // Verify database
    auto saved = ships.find_by_number(1);
    test::expect_true(saved.has_value());
    test::expect_eq(saved->name(), "USS Enterprise");
    std::println(std::cout, "    ✓ Database: ship name = '{}'", saved->name());
  }

  std::println(std::cout, "  ✅ Ship naming test passed!");
}

void test_name_race_persistence() {
  std::println(std::cout, "Test: name command - race naming");

  // Create in-memory database
  TestContext ctx;

  // Setup: Create a race
  Race race{};
  race.Playernum = 1;
  race.name = "Old Race Name";

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  // Create GameObj for command execution (leader, not governor)
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_governor(0);  // Must be leader (governor 0)
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.race =
      ctx.em.peek_race(g.player());  // Set race pointer like production does

  // TEST: Rename race
  std::println(std::cout, "  Testing: Rename race to 'Klingons'");
  {
    ctx.assert_dispatch_success(g, {"name", "race", "Klingons"});

    // Verify database
    auto saved = races.find_by_player(1);
    test::expect_true(saved.has_value());
    test::expect_eq(saved->name, "Klingons");
    std::println(std::cout, "    ✓ Database: race name = '{}'", saved->name);
  }

  std::println(std::cout, "  ✅ Race naming test passed!");
}

void test_name_star_persistence() {
  std::println(std::cout, "Test: name command - star naming");

  // Create in-memory database
  TestContext ctx;

  // Setup: Create a race (God)
  Race race{};
  race.Playernum = 1;
  race.God = 1;  // Must be deity

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  // Setup: Create a star
  star_struct star_data{};
  star_data.star_id = 1;
  star_data.name = "Old Star Name";
  Star star{star_data};

  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  g.set_god(true);
  g.race =
      ctx.em.peek_race(g.player());  // Set race pointer like production does

  // TEST: Rename star
  std::println(std::cout, "  Testing: Rename star to 'Alpha Centauri'");
  {
    ctx.assert_dispatch_success(g, {"name", "star", "Alpha", "Centauri"});

    // Verify database
    auto saved = stars_repo.find_by_number(1);
    test::expect_true(saved.has_value());
    test::expect_eq(saved->get_name(), "Alpha Centauri");
    std::println(std::cout, "    ✓ Database: star name = '{}'",
                 saved->get_name());
  }

  std::println(std::cout, "  ✅ Star naming test passed!");
}

void test_name_planet_persistence() {
  std::println(std::cout, "Test: name command - planet naming");

  // Create in-memory database
  TestContext ctx;

  // Setup: Create a race (God)
  Race race{};
  race.Playernum = 1;
  race.God = 1;  // Must be deity

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  // Setup: Create a star with planets
  star_struct star_data{};
  star_data.star_id = 1;
  star_data.name = "Test Star";
  star_data.pnames.push_back("Old Planet Name");
  Star star{star_data};

  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);
  g.set_god(true);
  g.race =
      ctx.em.peek_race(g.player());  // Set race pointer like production does

  // TEST: Rename planet
  std::println(std::cout, "  Testing: Rename planet to 'New Earth'");
  {
    ctx.assert_dispatch_success(g, {"name", "planet", "New", "Earth"});

    // Verify database
    auto saved = stars_repo.find_by_number(1);
    test::expect_true(saved.has_value());
    test::expect_eq(saved->get_planet_name(0), "New Earth");
    std::println(std::cout, "    ✓ Database: planet name = '{}'",
                 saved->get_planet_name(0));
  }

  std::println(std::cout, "  ✅ Planet naming test passed!");
}

void test_name_invalid_formats() {
  std::println(std::cout, "Test: name command - invalid input formats");

  // Create in-memory database
  TestContext ctx;

  // Setup: Create a race
  Race race{};
  race.Playernum = 1;

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.race = ctx.em.peek_race(g.player());

  // TEST: Less than 3 arguments -> "Not enough arguments." / "Illegal name
  // format."
  {
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"name", "ship"});
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

  // Create in-memory database
  TestContext ctx;

  // Setup: Create a race with initial governor name
  Race race{};
  race.Playernum = 1;
  race.governor[0].name = "Old Gov";

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_governor(0);
  g.race = ctx.em.peek_race(g.player());

  // TEST: Rename governor 0 to 'Grand Moff'
  {
    g.out.str("");
    ctx.assert_dispatch_success(g, {"name", "governor", "Grand", "Moff"});
    test::expect_contains(g.out.str(), "Name changed to `Grand Moff'.");

    // Verify database update
    auto saved = races.find_by_player(1);
    test::expect_true(saved.has_value());
    test::expect_eq(saved->governor[0].name, "Grand Moff");
  }

  std::println(std::cout, "  ✅ Governor naming test passed!");
}

void test_name_block() {
  std::println(std::cout,
               "Test: name command - block naming and authorization");

  // Create in-memory database
  TestContext ctx;

  // Setup: Create a race and alliance block
  Race race{};
  race.Playernum = 1;

  block bdata{};
  bdata.Playernum = 1;
  bdata.name = "Old Alliance";

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  BlockRepository block_repo(store);
  block_repo.save(bdata);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.race = ctx.em.peek_race(g.player());

  // TEST: Leader (governor 0) can rename alliance block
  {
    g.set_governor(0);
    g.out.str("");
    ctx.assert_dispatch_success(g, {"name", "block", "United", "Federation"});
    test::expect_contains(g.out.str(), "Done.");

    // Verify block name updated in database
    const auto* saved = ctx.em.peek_block(blocknum_t{1});
    test::expect_ne(saved, nullptr);
    test::expect_eq(saved->name, "United Federation");
  }

  // TEST: Non-leader governor (governor 1) is rejected
  {
    g.set_governor(1);
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"name", "block", "Rebel", "Alliance"});
    test::expect_contains(g.out.str(), "You are not authorized to do this.");
  }

  std::println(std::cout, "  ✅ Block naming test passed!");
}

void test_name_factory_class() {
  std::println(std::cout, "Test: name command - factory ship class naming");

  // Create in-memory database
  TestContext ctx;

  // Setup: Create race and ships (factory off-line and fighter)
  Race race{};
  race.Playernum = 1;

  Ship factory{};
  factory.number() = 1;
  factory.owner() = 1;
  factory.type() = ShipType::OTYPE_FACTORY;
  factory.on() = false;
  factory.shipclass() = "Basic";

  Ship fighter{};
  fighter.number() = 2;
  fighter.owner() = 1;
  fighter.type() = ShipType::STYPE_FIGHTER;

  JsonStore store(ctx.db);
  RaceRepository races(store);
  ShipRepository ships(store);
  races.save(race);
  ships.save(factory);
  ships.save(fighter);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.race = ctx.em.peek_race(g.player());

  // TEST: Name class at factory off-line -> success
  {
    g.set_shipno(1);
    g.out.str("");
    ctx.assert_dispatch_success(g, {"name", "class", "Battleship"});
    test::expect_contains(g.out.str(), "Class set.");

    auto saved = ships.find_by_number(1);
    test::expect_true(saved.has_value());
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

  std::println(std::cout, "  ✅ Factory class naming test passed!");
}

void test_name_permissions_and_scope() {
  std::println(std::cout, "Test: name command - permissions and scope checks");

  // Create in-memory database
  TestContext ctx;

  // Setup: Create non-god race and star with planet
  Race race{};
  race.Playernum = 1;
  race.God = 0;  // Non-god race

  star_struct star_data{};
  star_data.star_id = 1;
  star_data.name = "Star 1";
  star_data.pnames.push_back("Planet 1");
  Star star{star_data};

  JsonStore store(ctx.db);
  RaceRepository races(store);
  StarRepository stars(store);
  races.save(race);
  stars.save(star);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.race = ctx.em.peek_race(g.player());

  // TEST: Non-god naming star -> rejected
  {
    g.set_level(ScopeLevel::LEVEL_STAR);
    g.set_snum(1);
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"name", "star", "Forbidden"});
    test::expect_contains(g.out.str(), "Only dieties may name a star.");
  }

  // TEST: Non-god naming planet -> rejected
  {
    g.set_level(ScopeLevel::LEVEL_PLAN);
    g.set_snum(1);
    g.set_pnum(0);
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"name", "planet", "Forbidden"});
    test::expect_contains(g.out.str(), "Only deity can rename planets.");
  }

  // TEST: Naming ship when not at ship level -> wrong scope message
  {
    g.set_level(ScopeLevel::LEVEL_UNIV);
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"name", "ship", "Enterprise"});
    test::expect_contains(g.out.str(),
                          "You have to 'cs' to a ship to name it.");
  }

  std::println(std::cout, "  ✅ Permissions and scope test passed!");
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
  test_name_permissions_and_scope();

  std::println(std::cout, "\n✅ All name tests passed!");
  return 0;
}
