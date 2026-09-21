// SPDX-License-Identifier: Apache-2.0

/// \file build_test.cc
/// \brief Unit tests for can_build_on_sector rules: population presence, wasted
/// sectors, sector ownership, God privileges, planetary build restrictions, and
/// quarry uniqueness.

import dallib;
import gb.entities;
import gb.repositories;
import gb.services;
import gb.mechanics;
import test;
import std;

int main() {
  // Initialize database
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // Create a test race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.God = false;
  race.tech = 50.0;

  // Create a test planet with sectors
  Planet planet{};
  planet.dimensions() = Coordinates{10, 10};

  // Create a normal sector owned by the race with population
  Sector good_sector{};
  good_sector.set_owner(1);
  good_sector.set_popn_exact(100);
  good_sector.set_condition(SectorType::SEC_LAND);

  // Success case - can build on owned sector with population
  {
    auto result = can_build_on_sector(em, ShipType::OTYPE_PROBE, race, planet,
                                      good_sector, {0, 0});
    test::expect_true(result.has_value());
    std::println(std::cout, "Test 1 passed: Can build on valid sector");
  }

  // Fail - no population
  {
    Sector no_pop_sector{};
    no_pop_sector.set_owner(1);
    no_pop_sector.clear_popn();
    no_pop_sector.set_condition(SectorType::SEC_LAND);
    auto result = can_build_on_sector(em, ShipType::OTYPE_PROBE, race, planet,
                                      no_pop_sector, {0, 0});
    test::expect_false(result.has_value());
    test::expect_eq(result.error(), "You have no more civs in the sector!\n");
    std::println(std::cout, "Test 2 passed: Rejects sector with no population");
  }

  // Fail - wasted sector
  {
    Sector wasted_sector{};
    wasted_sector.set_owner(1);
    wasted_sector.set_popn_exact(100);
    wasted_sector.set_condition(SectorType::SEC_WASTED);
    auto result = can_build_on_sector(em, ShipType::OTYPE_PROBE, race, planet,
                                      wasted_sector, {0, 0});
    test::expect_false(result.has_value());
    test::expect_eq(result.error(), "You can't build on wasted sectors.\n");
    std::println(std::cout, "Test 3 passed: Rejects wasted sector");
  }

  // Fail - sector not owned by race
  {
    Sector alien_sector{};
    alien_sector.set_owner(2);  // Different player
    alien_sector.set_popn_exact(100);
    alien_sector.set_condition(SectorType::SEC_LAND);
    auto result = can_build_on_sector(em, ShipType::OTYPE_PROBE, race, planet,
                                      alien_sector, {0, 0});
    test::expect_false(result.has_value());
    test::expect_eq(result.error(), "You don't own that sector.\n");
    std::println(std::cout,
                 "Test 4 passed: Rejects sector owned by another player");
  }

  // Success - God can build on alien sector
  {
    Race god_race = race;
    god_race.God = true;
    Sector alien_sector{};
    alien_sector.set_owner(2);
    alien_sector.set_popn_exact(100);
    alien_sector.set_condition(SectorType::SEC_LAND);
    auto result = can_build_on_sector(em, ShipType::OTYPE_PROBE, god_race,
                                      planet, alien_sector, {0, 0});
    test::expect_true(result.has_value());
    std::println(std::cout, "Test 5 passed: God can build on alien sector");
  }

  // Fail - ship type cannot be built on planet (non-God)
  {
    // Find a ship type that cannot be built on planets
    // Using STYPE_HABITAT which typically can't be built on planets
    if (!ship_template(ShipType::STYPE_HABITAT).can_build_on_planet()) {
      auto result = can_build_on_sector(em, ShipType::STYPE_HABITAT, race,
                                        planet, good_sector, {0, 0});
      test::expect_false(result.has_value());
      test::expect_contains(result.error(), "cannot be built on a planet");
      std::println(std::cout,
                   "Test 6 passed: Rejects ship type that can't be built on "
                   "planets");
    } else {
      std::println(std::cout,
                   "Test 6 skipped: HABITAT can be built on planets in this "
                   "configuration");
    }
  }

  // Success - quarry at new location
  {
    auto result = can_build_on_sector(em, ShipType::OTYPE_QUARRY, race, planet,
                                      good_sector, {5, 5});
    test::expect_true(result.has_value());
    std::println(std::cout,
                 "Test 7 passed: Can build quarry at empty location");
  }

  // Fail - quarry already exists at location (3rd ship in list)
  {
    // Create first ship - a probe at different location
    Ship probe1{};
    probe1.number() = 100;
    probe1.type() = ShipType::OTYPE_PROBE;
    probe1.owner() = 1;
    probe1.alive() = true;
    probe1.whatorbits() = ScopeLevel::LEVEL_PLAN;
    probe1.storbits() = planet.star_id();
    probe1.pnumorbits() = planet.planet_order();
    probe1.set_land_coords({1, 1});

    // Create second ship - another probe at different location
    Ship probe2{};
    probe2.number() = 101;
    probe2.type() = ShipType::OTYPE_PROBE;
    probe2.owner() = 1;
    probe2.alive() = true;
    probe2.whatorbits() = ScopeLevel::LEVEL_PLAN;
    probe2.storbits() = planet.star_id();
    probe2.pnumorbits() = planet.planet_order();
    probe2.set_land_coords({2, 2});

    // Create third ship - a quarry at coordinates (3, 3)
    Ship quarry{};
    quarry.number() = 102;
    quarry.type() = ShipType::OTYPE_QUARRY;
    quarry.owner() = 1;
    quarry.alive() = true;
    quarry.whatorbits() = ScopeLevel::LEVEL_PLAN;
    quarry.storbits() = planet.star_id();
    quarry.pnumorbits() = planet.planet_order();
    quarry.set_land_coords({3, 3});

    // Add all ships to repository
    JsonStore store(db);
    ShipRepository ships(store);
    ships.save(probe1);
    ships.save(probe2);
    ships.save(quarry);

    // Try to build another quarry at same location as the 3rd ship
    auto result = can_build_on_sector(em, ShipType::OTYPE_QUARRY, race, planet,
                                      good_sector, {3, 3});
    test::expect_false(result.has_value());
    test::expect_eq(result.error(), "There already is a quarry here.\n");
    std::println(
        std::cout,
        "Test 8 passed: Rejects duplicate quarry at same location (3rd ship "
        "in list)");
  }

  // Success - quarry at different location than existing
  {
    // Quarry exists at (3,3), try building at (4,4)
    auto result = can_build_on_sector(em, ShipType::OTYPE_QUARRY, race, planet,
                                      good_sector, {4, 4});
    test::expect_true(result.has_value());
    std::println(
        std::cout,
        "Test 9 passed: Can build quarry at different location than existing");
  }

  // Success - dead quarry at location doesn't block
  {
    // Create a dead quarry at (7, 7)
    Ship dead_quarry{};
    dead_quarry.number() = 2;
    dead_quarry.type() = ShipType::OTYPE_QUARRY;
    dead_quarry.owner() = 1;
    dead_quarry.alive() = false;  // Dead
    dead_quarry.whatorbits() = ScopeLevel::LEVEL_PLAN;
    dead_quarry.storbits() = planet.star_id();
    dead_quarry.pnumorbits() = planet.planet_order();
    dead_quarry.set_land_coords({7, 7});

    JsonStore store(db);
    ShipRepository ships(store);
    ships.save(dead_quarry);

    // Should be able to build at (7,7) since existing quarry is dead
    auto result = can_build_on_sector(em, ShipType::OTYPE_QUARRY, race, planet,
                                      good_sector, {7, 7});
    test::expect_true(result.has_value());
    std::println(std::cout,
                 "Test 10 passed: Dead quarry doesn't block new construction");
  }

  // Test 11: autoload_at_planet synchronizes planetary demographics and
  // sector abandonment
  {
    TestContext ctx;
    ctx.with_standard_universe();

    ctx.em.mutate_planet(0, 0, [](Planet& p) {
      p.popn() = 100;
      p.info(1).popn = 100;
      p.info(1).numsectsowned = 1;
      p.info(1).fuel = 500;
    });
    ctx.em.mutate_sectormap(0, 0, [](SectorMap& smap) {
      for (auto& s : smap) {
        s.clear_popn();
        s.set_troops_exact(0);
        s.set_owner(0);
      }
      auto& sect = smap.get(Coordinates{1, 1});
      sect.set_owner(1);
      sect.set_popn_exact(100);
    });

    auto ship = getship(ShipType::STYPE_SHUTTLE, *ctx.em.peek_race(1));
    ship->max_crew() = 100;
    ship->max_fuel() = 50;
    std::pair<population_t, fuel_t> loaded{};

    ctx.em.mutate_planet(0, 0, [&](Planet& p) {
      ctx.em.mutate_sectormap(0, 0, [&](SectorMap& smap) {
        auto& sect = smap.get(Coordinates{1, 1});
        loaded = autoload_at_planet(1, *ship, p, sect);
      });
    });

    const auto [crew, fuel] = loaded;
    test::expect_eq(crew, 100);
    test::expect_eq(fuel, 50.0);
    const auto* p_after = ctx.em.peek_planet(0, 0);
    const auto* smap_after = ctx.em.peek_sectormap(0, 0);
    test::expect_eq(p_after->popn(), 0);
    test::expect_eq(p_after->info(1).popn, 0);
    test::expect_eq(p_after->info(1).numsectsowned, 0);
    test::expect_eq(p_after->info(1).fuel, 450);
    test::expect_eq(smap_after->get(Coordinates{1, 1}).get_owner(), 0);
    std::println(std::cout,
                 "Test 11 passed: autoload_at_planet synchronizes planetary "
                 "demographics");
  }

  // Test 12: autoload_at_ship transfers crew and fuel from builder ship
  {
    TestContext ctx;
    ctx.with_standard_universe();
    const auto& r1 = *ctx.em.peek_race(1);
    auto builder = getship(ShipType::STYPE_CARRIER, r1);
    builder->popn() = 50;
    builder->admin_override_fuel(200.0, r1.mass);

    auto target = getship(ShipType::STYPE_FIGHTER, r1);
    auto [crew, fuel] = autoload_at_ship(*target, *builder, r1.mass);
    test::expect_gt(crew, 0);
    test::expect_gt(fuel, 0.0);
    test::expect_eq(builder->popn(), 50 - crew);
    std::println(std::cout,
                 "Test 12 passed: autoload_at_ship transfers crew and fuel");
  }

  // Test 13: can_build_this technology and discovery checks
  {
    Race low_tech{};
    low_tech.Playernum = 1;
    low_tech.tech = 0.0;
    low_tech.pods = false;

    auto pod_res = can_build_this(ShipType::STYPE_POD, low_tech);
    test::expect_false(pod_res.has_value());
    test::expect_contains(pod_res.error(), "Metamorphic");

    auto god_res = can_build_this(ShipType::STYPE_GOD, low_tech);
    test::expect_false(god_res.has_value());
    test::expect_contains(god_res.error(), "Only Gods");

    auto vn_res = can_build_this(ShipType::OTYPE_VN, low_tech);
    test::expect_false(vn_res.has_value());
    test::expect_contains(vn_res.error(), "VN technology");

    auto trans_res = can_build_this(ShipType::OTYPE_TRANSDEV, low_tech);
    test::expect_false(trans_res.has_value());
    test::expect_contains(trans_res.error(), "AVPM technology");

    low_tech.discoveries.vn = true;
    auto vn_tech_res = can_build_this(ShipType::OTYPE_VN, low_tech);
    test::expect_false(vn_tech_res.has_value());
    test::expect_contains(vn_tech_res.error(), "not advanced enough");
    std::println(
        std::cout,
        "Test 13 passed: can_build_this enforces tech and discoveries");
  }

  // Test 14: initialize_new_ship special ship types and diagnostics
  {
    TestContext ctx;
    ctx.with_standard_universe();
    auto& registry = get_test_session_registry();
    GameObj g(ctx.em, registry);
    ctx.setup_game_obj(g, 1, 0);
    const auto& r1 = *ctx.em.peek_race(1);

    // VN Ship
    auto vn = getship(ShipType::OTYPE_VN, r1);
    initialize_new_ship(g, r1, *vn, 0.0, 0);
    test::expect_contains(g.out.str(), "robotic");

    // Mine Ship
    g.out.str("");
    auto mine = getship(ShipType::STYPE_MINE, r1);
    initialize_new_ship(g, r1, *mine, 0.0, 0);
    test::expect_contains(g.out.str(), "Mine disarmed");

    // Transporter Ship
    g.out.str("");
    auto trans = getship(ShipType::OTYPE_TRANSDEV, r1);
    initialize_new_ship(g, r1, *trans, 0.0, 0);
    test::expect_contains(g.out.str(), "Receive OFF");

    // Atmospheric Processor
    g.out.str("");
    auto ap = getship(ShipType::OTYPE_AP, r1);
    initialize_new_ship(g, r1, *ap, 10.0, 5);
    test::expect_contains(g.out.str(), "Processor OFF");

    // Space Telescope
    g.out.str("");
    auto tele = getship(ShipType::OTYPE_STELE, r1);
    initialize_new_ship(g, r1, *tele, 0.0, 0);
    test::expect_contains(g.out.str(), "Telescope range");

    // Factory
    g.out.str("");
    auto fact = getship(ShipType::OTYPE_FACTORY, r1);
    initialize_new_ship(g, r1, *fact, 10.0, 5);
    test::expect_contains(g.out.str(),
                          "Warning: This ship is constructed with");
    test::expect_contains(g.out.str(), "factory may not begin repairs");
    std::println(std::cout,
                 "Test 14 passed: initialize_new_ship special types and logs");
  }

  // Test 15: create_ship_by_planet with ToxicWasteShip and shipping_cost
  {
    TestContext ctx;
    ctx.with_standard_universe();
    const auto& r1 = *ctx.em.peek_race(1);

    ctx.em.mutate_planet(0, 0, [](Planet& p) {
      p.toxic() = 80;
      p.info(player_t{1}).resource = 1000;
    });

    auto tox_ship = getship(ShipType::OTYPE_TOXWC, r1);
    ctx.em.mutate_planet(0, 0, [&](Planet& p) {
      create_ship_by_planet(ctx.em, 1, 0, r1, *tox_ship, p, 0, 0,
                            Coordinates{2, 2});
    });

    const auto* p_after = ctx.em.peek_planet(0, 0);
    test::expect_lt(p_after->toxic(), 80);

    auto [scost, sdist] = shipping_cost(ctx.em, 0, 1, 1000);
    test::expect_gt(sdist, 0.0);
    test::expect_ge(scost, 0);
    std::println(std::cout,
                 "Test 15 passed: ToxicWasteShip extraction and shipping_cost");
  }

  // Test 16: can_build_at_planet, can_build_on_ship, build_at_ship,
  // create_ship_by_ship, and getfactship
  {
    TestContext ctx;
    ctx.with_standard_universe();
    auto& registry = get_test_session_registry();
    GameObj g(ctx.em, registry);
    ctx.setup_game_obj(g, 1, 0);
    const auto& r1 = *ctx.em.peek_race(1);

    // can_build_at_planet: enslaved planet
    ctx.em.mutate_planet(0, 0, [](Planet& p) { p.enslave_to(2); });
    test::expect_false(can_build_at_planet(g, *ctx.em.peek_star(0),
                                           *ctx.em.peek_planet(0, 0)));
    ctx.em.mutate_planet(0, 0, [](Planet& p) { p.free_slaves(); });

    // can_build_at_planet: unauthorized governor
    ctx.setup_game_obj(g, 1, 2);
    test::expect_false(can_build_at_planet(g, *ctx.em.peek_star(0),
                                           *ctx.em.peek_planet(0, 0)));
    ctx.setup_game_obj(g, 1, 0);

    // can_build_on_ship
    auto probe = getship(ShipType::OTYPE_PROBE, r1);
    auto shuttle = getship(ShipType::STYPE_SHUTTLE, r1);
    test::expect_false(
        can_build_on_ship(ShipType::STYPE_FIGHTER, r1, *probe).has_value());
    test::expect_true(
        can_build_on_ship(ShipType::STYPE_STATION, r1, *shuttle).has_value());

    // build_at_ship error paths
    starnum_t snum = 0;
    planetnum_t pnum = 0;
    probe->owner() = 1;
    probe->alive() = true;
    probe->active() = true;
    test::expect_false(build_at_ship(g, *probe, snum, pnum).has_value());

    shuttle->owner() = 1;
    shuttle->alive() = true;
    shuttle->active() = true;
    shuttle->popn() = 0;
    test::expect_false(build_at_ship(g, *shuttle, snum, pnum).has_value());

    shuttle->popn() = 10;
    shuttle->dock_with_ship(99);
    test::expect_false(build_at_ship(g, *shuttle, snum, pnum).has_value());
    shuttle->undock_from_ship();

    shuttle->admin_override_damage(50);
    test::expect_false(build_at_ship(g, *shuttle, snum, pnum).has_value());
    shuttle->admin_override_damage(0);

    auto factory = getship(ShipType::OTYPE_FACTORY, r1);
    factory->owner() = 1;
    factory->alive() = true;
    factory->active() = true;
    factory->popn() = 10;
    factory->admin_override_damage(0);
    factory->on() = false;
    test::expect_false(build_at_ship(g, *factory, snum, pnum).has_value());
    factory->on() = true;
    factory->launch_to_orbit(ScopeLevel::LEVEL_PLAN);
    test::expect_false(build_at_ship(g, *factory, snum, pnum).has_value());
    factory->land_on_planet();
    test::expect_true(build_at_ship(g, *factory, snum, pnum).has_value());

    // create_ship_by_ship (outside and hangar) and getfactship
    factory->build_type() = ShipType::OTYPE_PROBE;
    auto fact_product = getfactship(*factory);
    test::expect_eq(fact_product->type(), ShipType::OTYPE_PROBE);

    shuttle->number() = 10;
    shuttle->resource() = 1000;
    auto built_station = getship(ShipType::STYPE_STATION, r1);
    create_ship_by_ship(ctx.em, 1, 0, r1, true, *built_station, *shuttle);
    test::expect_true(built_station->is_spaceborne());

    auto carrier = getship(ShipType::STYPE_CARRIER, r1);
    carrier->number() = 11;
    carrier->resource() = 1000;
    auto built_fighter = getship(ShipType::STYPE_FIGHTER, r1);
    create_ship_by_ship(ctx.em, 1, 0, r1, false, *built_fighter, *carrier);
    test::expect_true(built_fighter->is_docked());
    std::println(std::cout,
                 "Test 16 passed: build_at_ship, create_ship_by_ship, and "
                 "getfactship");
  }

  std::println(std::cout,
               "\nAll can_build_on_sector and build entity tests passed!");
  return 0;
}
