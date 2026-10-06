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
import gb.presentation;
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
  Planet planet{planet_struct{
      .dimensions = {10, 10},
      .star_id = 1,
      .planet_order = 1,
  }};
  {
    JsonStore store(db);
    RaceRepository(store).save(race);
    StarRepository(store).save(Star{1, "Sol"});
    PlanetRepository(store).save(planet);
  }

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

  // Fail - ship type cannot be built on planet (non-God), but God can
  {
    auto result = can_build_on_sector(em, ShipType::STYPE_HABITAT, race, planet,
                                      good_sector, {0, 0});
    test::expect_false(result.has_value());
    test::expect_contains(result.error(), "cannot be built on a planet");

    Race god_race = race;
    god_race.God = true;
    auto god_result = can_build_on_sector(em, ShipType::STYPE_HABITAT, god_race,
                                          planet, good_sector, {0, 0});
    test::expect_true(god_result.has_value());
    std::println(std::cout,
                 "Test 6 passed: Rejects ship type that can't be built on "
                 "planets for mortals, allows for Gods");
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
    probe1.enter_planet_orbit(planet.star_id(), planet.planet_order());
    probe1.set_land_coords({1, 1});

    // Create second ship - another probe at different location
    Ship probe2{};
    probe2.number() = 101;
    probe2.type() = ShipType::OTYPE_PROBE;
    probe2.owner() = 1;
    probe2.alive() = true;
    probe2.enter_planet_orbit(planet.star_id(), planet.planet_order());
    probe2.set_land_coords({2, 2});

    // Create third ship - a quarry at coordinates (3, 3)
    Ship quarry{};
    quarry.number() = 102;
    quarry.type() = ShipType::OTYPE_QUARRY;
    quarry.owner() = 1;
    quarry.alive() = true;
    quarry.enter_planet_orbit(planet.star_id(), planet.planet_order());
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

  // Success - destroyed quarry at location doesn't block
  {
    // Create and then destroy a quarry at (7, 7)
    Ship dead_quarry{};
    dead_quarry.number() = 2;
    dead_quarry.type() = ShipType::OTYPE_QUARRY;
    dead_quarry.owner() = 1;
    dead_quarry.alive() = true;
    dead_quarry.enter_planet_orbit(planet.star_id(), planet.planet_order());
    dead_quarry.set_land_coords({7, 7});

    JsonStore store(db);
    ShipRepository ships(store);
    ships.save(dead_quarry);
    em.kill_ship(1, dead_quarry);

    // Should be able to build at (7,7) since existing quarry is destroyed
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

    ctx.em.mutate_planet(1, 1, [](Planet& p) {
      p.popn() = 100;
      p.info(1).popn = 100;
      p.info(1).numsectsowned = 1;
      p.info(1).fuel = 500;
    });
    ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
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

    ctx.em.mutate_planet(1, 1, [&](Planet& p) {
      ctx.em.mutate_sectormap(1, 1, [&](SectorMap& smap) {
        auto& sect = smap.get(Coordinates{1, 1});
        loaded = autoload_at_planet(1, *ship, p, sect);
      });
    });

    const auto [crew, fuel] = loaded;
    test::expect_eq(crew, 100);
    test::expect_eq(fuel, 50.0);
    const auto* p_after = ctx.em.peek_planet(1, 1);
    const auto* smap_after = ctx.em.peek_sectormap(1, 1);
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

    Race pod_race = low_tech;
    pod_race.pods = true;
    pod_race.tech = 500.0;
    test::expect_true(
        can_build_this(ShipType::STYPE_POD, pod_race).has_value());

    auto unprog_res = can_build_this(ShipType::OTYPE_TRACT, pod_race);
    test::expect_false(unprog_res.has_value());
    test::expect_contains(unprog_res.error(), "not been programmed");

    auto god_res = can_build_this(ShipType::STYPE_GOD, low_tech);
    test::expect_false(god_res.has_value());
    test::expect_contains(god_res.error(), "Only Gods");

    Race god_race = low_tech;
    god_race.God = true;
    test::expect_true(
        can_build_this(ShipType::STYPE_GOD, god_race).has_value());

    auto vn_res = can_build_this(ShipType::OTYPE_VN, low_tech);
    test::expect_false(vn_res.has_value());
    test::expect_contains(vn_res.error(), "VN technology");

    auto trans_res = can_build_this(ShipType::OTYPE_TRANSDEV, low_tech);
    test::expect_false(trans_res.has_value());
    test::expect_contains(trans_res.error(), "AVPM technology");

    Race avpm_race = low_tech;
    avpm_race.discoveries.avpm = true;
    avpm_race.tech = 500.0;
    test::expect_true(
        can_build_this(ShipType::OTYPE_TRANSDEV, avpm_race).has_value());

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
    const auto& r1 = *ctx.em.peek_race(1);

    // VN Ship (robotic ship: max_crew_capacity == 0)
    auto vn = getship(ShipType::OTYPE_VN, r1);
    auto vn_rep = initialize_new_ship(r1, 1, *vn, 0.0, 0);
    std::string vn_text =
        GB::presentation::render_initialized_ship_report(vn_rep);
    test::expect_contains(vn_text, "robotic");

    // Mine Ship
    auto mine = getship(ShipType::STYPE_MINE, r1);
    auto mine_rep = initialize_new_ship(r1, 1, *mine, 0.0, 0);
    test::expect_contains(
        GB::presentation::render_initialized_ship_report(mine_rep),
        "Mine disarmed");

    // Transporter Ship
    auto trans = getship(ShipType::OTYPE_TRANSDEV, r1);
    auto trans_rep = initialize_new_ship(r1, 1, *trans, 0.0, 0);
    test::expect_contains(
        GB::presentation::render_initialized_ship_report(trans_rep),
        "Receive OFF");

    // Atmospheric Processor
    auto ap = getship(ShipType::OTYPE_AP, r1);
    auto ap_rep = initialize_new_ship(r1, 1, *ap, 10.0, 5);
    test::expect_contains(
        GB::presentation::render_initialized_ship_report(ap_rep),
        "Processor OFF");

    // Space Telescope & Ground Telescope
    auto stele = getship(ShipType::OTYPE_STELE, r1);
    auto stele_rep = initialize_new_ship(r1, 1, *stele, 0.0, 0);
    test::expect_contains(
        GB::presentation::render_initialized_ship_report(stele_rep),
        "Telescope range");

    auto gtele = getship(ShipType::OTYPE_GTELE, r1);
    auto gtele_rep = initialize_new_ship(r1, 1, *gtele, 0.0, 0);
    test::expect_contains(
        GB::presentation::render_initialized_ship_report(gtele_rep),
        "Telescope range");

    // Factory (damaged, can_repair, max_crew_capacity > 0)
    auto fact = getship(ShipType::OTYPE_FACTORY, r1);
    auto fact_rep = initialize_new_ship(r1, 1, *fact, 10.0, 5);
    std::string fact_text =
        GB::presentation::render_initialized_ship_report(fact_rep);
    test::expect_contains(fact_text, "Warning: This ship is constructed with");
    test::expect_contains(fact_text, "factory may not begin repairs");
    test::expect_contains(fact_text,
                          "This ship does not need resources to repair.");

    // Dreadnaught (damaged, !can_repair, max_crew_capacity > 0)
    auto dread = getship(ShipType::STYPE_DREADNT, r1);
    auto dread_rep = initialize_new_ship(r1, 1, *dread, 10.0, 5);
    std::string dread_text =
        GB::presentation::render_initialized_ship_report(dread_rep);
    test::expect_contains(
        dread_text, "It will need resources to become fully operational.");
    std::println(std::cout,
                 "Test 14 passed: initialize_new_ship special types and logs");
  }

  // Test 15: create_ship_by_planet with ToxicWasteShip, Terra, Plow, and
  // shipping_cost
  {
    TestContext ctx;
    ctx.with_standard_universe();
    const auto& r1 = *ctx.em.peek_race(1);

    ctx.em.mutate_planet(1, 1, [](Planet& p) {
      p.toxic() = 80;
      p.info(player_t{1}).resource = 1000;
    });

    auto tox_ship = getship(ShipType::OTYPE_TOXWC, r1);
    CreatedShipSummary tox_summary{};
    ctx.em.mutate_planet(1, 1, [&](Planet& p) {
      tox_summary = create_ship_by_planet(ctx.em, 1, 1, r1, *tox_ship, p, 1, 1,
                                          Coordinates{2, 2});
    });
    test::expect_true(tox_summary.previous_toxicity.has_value());
    test::expect_true(tox_summary.updated_toxicity.has_value());
    std::string tox_text =
        GB::presentation::render_created_ship_summary(tox_summary);
    test::expect_contains(tox_text, "Toxin concentration on planet was");

    const auto* p_after = ctx.em.peek_planet(1, 1);
    test::expect_lt(p_after->toxic(), 80);

    // Terra and Plow built on planet
    auto terra_ship = getship(ShipType::OTYPE_TERRA, r1);
    auto plow_ship = getship(ShipType::OTYPE_PLOW, r1);
    ctx.em.mutate_planet(1, 1, [&](Planet& p) {
      create_ship_by_planet(ctx.em, 1, 1, r1, *terra_ship, p, 1, 1,
                            Coordinates{2, 2});
      create_ship_by_planet(ctx.em, 1, 1, r1, *plow_ship, p, 1, 1,
                            Coordinates{2, 2});
    });
    test::expect_eq(terra_ship->shipclass(), "5");
    test::expect_eq(plow_ship->shipclass(), "5");

    auto [scost, sdist] = shipping_cost(ctx.em, 1, 2, 1000);
    test::expect_gt(sdist, 0.0);
    test::expect_ge(scost, 0);
    std::println(std::cout,
                 "Test 15 passed: ToxicWasteShip extraction and shipping_cost");
  }

  // Test 16: can_build_at_planet, can_build_on_ship, build_at_ship,
  // create_ship_by_ship, getship (God), and getfactship
  {
    TestContext ctx;
    ctx.with_standard_universe();
    const auto& r1 = *ctx.em.peek_race(1);
    Race god_race = r1;
    god_race.God = true;

    // getship with God race covering mount/hyperdrive/laser templates
    auto god_probe = getship(ShipType::OTYPE_PROBE, god_race);
    auto god_dread = getship(ShipType::STYPE_DREADNT, god_race);
    test::expect_false(god_probe->mount());
    test::expect_true(god_dread->mount());
    test::expect_true(god_dread->hyper_drive().has);
    test::expect_true(god_dread->laser());

    // can_build_at_planet: enslaved planet
    ctx.em.mutate_planet(1, 1, [](Planet& p) { p.enslave_to(2); });
    auto enslaved_res = can_build_at_planet(1, 1, *ctx.em.peek_star(1),
                                            *ctx.em.peek_planet(1, 1));
    test::expect_false(enslaved_res.has_value());
    test::expect_contains(
        GB::presentation::format_planet_build_error(enslaved_res.error()),
        "This planet is enslaved by player 2.");
    test::expect_true(ctx.em.get_telegrams(1, 1).empty());
    ctx.em.mutate_planet(1, 1, [](Planet& p) { p.free_slaves(); });

    // can_build_at_planet: unauthorized governor
    auto unauth_res = can_build_at_planet(1, 2, *ctx.em.peek_star(1),
                                          *ctx.em.peek_planet(1, 1));
    test::expect_false(unauth_res.has_value());
    test::expect_contains(
        GB::presentation::format_planet_build_error(unauth_res.error()),
        "You are not authorized in this system.");

    // can_build_at_planet: success
    test::expect_true(can_build_at_planet(1, 1, *ctx.em.peek_star(1),
                                          *ctx.em.peek_planet(1, 1))
                          .has_value());

    // can_build_on_ship (mortal vs God)
    auto probe = getship(ShipType::OTYPE_PROBE, r1);
    auto shuttle = getship(ShipType::STYPE_SHUTTLE, r1);
    test::expect_false(
        can_build_on_ship(ShipType::STYPE_FIGHTER, r1, *probe).has_value());
    test::expect_true(
        can_build_on_ship(ShipType::STYPE_FIGHTER, god_race, *probe)
            .has_value());
    test::expect_true(
        can_build_on_ship(ShipType::STYPE_STATION, r1, *shuttle).has_value());

    // build_at_ship error paths
    probe->owner() = 1;
    probe->governor() = 1;
    probe->alive() = false;
    auto dead_res = build_at_ship(1, 1, false, *probe);
    test::expect_false(dead_res.has_value());
    test::expect_contains(
        GB::presentation::format_ship_build_error(dead_res.error()),
        "Has been destroyed.");
    probe->alive() = true;

    probe->owner() = 2;
    auto unowned_res = build_at_ship(1, 1, false, *probe);
    test::expect_false(unowned_res.has_value());
    test::expect_contains(
        GB::presentation::format_ship_build_error(unowned_res.error()),
        "You do not own this ship.");
    probe->owner() = 1;

    auto gov_res = build_at_ship(1, 2, false, *probe);
    test::expect_false(gov_res.has_value());
    test::expect_contains(
        GB::presentation::format_ship_build_error(gov_res.error()),
        "You are not authorized to do this.");

    probe->active() = false;
    auto irrad_res = build_at_ship(1, 1, false, *probe);
    test::expect_false(irrad_res.has_value());
    test::expect_contains(
        GB::presentation::format_ship_build_error(irrad_res.error()),
        "irradiated");

    probe->active() = true;
    auto cannot_build_res = build_at_ship(1, 1, false, *probe);
    test::expect_false(cannot_build_res.has_value());
    test::expect_contains(
        GB::presentation::format_ship_build_error(cannot_build_res.error()),
        "This ship cannot construct other ships.");

    shuttle->owner() = 1;
    shuttle->governor() = 1;
    shuttle->alive() = true;
    shuttle->active() = true;
    shuttle->popn() = 0;
    auto no_crew_res = build_at_ship(1, 1, false, *shuttle);
    test::expect_false(no_crew_res.has_value());
    test::expect_contains(
        GB::presentation::format_ship_build_error(no_crew_res.error()),
        "no crew");

    shuttle->popn() = 10;
    shuttle->dock_with_ship(99);
    auto docked_res = build_at_ship(1, 1, false, *shuttle);
    test::expect_false(docked_res.has_value());
    test::expect_contains(
        GB::presentation::format_ship_build_error(docked_res.error()),
        "Undock this ship first.");
    shuttle->undock_from_ship();

    shuttle->admin_override_damage(50);
    auto damaged_res = build_at_ship(1, 1, false, *shuttle);
    test::expect_false(damaged_res.has_value());
    test::expect_contains(
        GB::presentation::format_ship_build_error(damaged_res.error()),
        "damaged");
    shuttle->admin_override_damage(0);

    // Non-factory ship with on() = false and in orbit succeeds
    shuttle->on() = false;
    shuttle->launch_to_orbit(ScopeLevel::LEVEL_PLAN);
    test::expect_true(build_at_ship(1, 1, false, *shuttle).has_value());

    auto factory = getship(ShipType::OTYPE_FACTORY, r1);
    factory->owner() = 1;
    factory->governor() = 1;
    factory->alive() = true;
    factory->active() = true;
    factory->popn() = 10;
    factory->admin_override_damage(0);
    factory->on() = false;
    auto offline_res = build_at_ship(1, 1, false, *factory);
    test::expect_false(offline_res.has_value());
    test::expect_contains(
        GB::presentation::format_ship_build_error(offline_res.error()),
        "online");
    factory->on() = true;
    factory->launch_to_orbit(ScopeLevel::LEVEL_PLAN);
    auto not_landed_res = build_at_ship(1, 1, false, *factory);
    test::expect_false(not_landed_res.has_value());
    test::expect_contains(
        GB::presentation::format_ship_build_error(not_landed_res.error()),
        "landed on a planet");
    factory->land_on_planet();
    test::expect_true(build_at_ship(1, 1, false, *factory).has_value());

    // create_ship_by_ship (outside and hangar) and getfactship (unarmed +
    // armed)
    factory->build_type() = ShipType::OTYPE_PROBE;
    auto fact_product = getfactship(*factory);
    test::expect_eq(fact_product->type(), ShipType::OTYPE_PROBE);
    test::expect_eq(fact_product->guns(), ActiveBattery::NONE);

    factory->build_type() = ShipType::STYPE_FIGHTER;
    factory->set_primary_battery(2, guntype_t::LIGHT);
    auto armed_product = getfactship(*factory);
    test::expect_eq(armed_product->guns(), ActiveBattery::PRIMARY);

    shuttle->number() = 10;
    shuttle->resource() = 1000;
    auto built_station = getship(ShipType::STYPE_STATION, r1);
    create_ship_by_ship(ctx.em, 1, 1, r1, true, *built_station, *shuttle);
    test::expect_true(built_station->is_spaceborne());

    auto built_terra = getship(ShipType::OTYPE_TERRA, r1);
    auto built_plow = getship(ShipType::OTYPE_PLOW, r1);
    create_ship_by_ship(ctx.em, 1, 1, r1, true, *built_terra, *shuttle);
    create_ship_by_ship(ctx.em, 1, 1, r1, true, *built_plow, *shuttle);
    test::expect_eq(built_terra->shipclass(), "5");
    test::expect_eq(built_plow->shipclass(), "5");

    auto carrier = getship(ShipType::STYPE_CARRIER, r1);
    carrier->number() = 11;
    carrier->owner() = 1;
    carrier->resource() = 1000;
    {
      JsonStore store(ctx.db);
      ShipRepository(store).save(*carrier);
    }
    auto built_fighter = getship(ShipType::STYPE_FIGHTER, r1);
    create_ship_by_ship(ctx.em, 1, 1, r1, false, *built_fighter, *carrier);
    test::expect_true(built_fighter->is_docked());
    std::println(std::cout,
                 "Test 16 passed: build_at_ship, create_ship_by_ship, and "
                 "getfactship");
  }

  std::println(std::cout,
               "\nAll can_build_on_sector and build entity tests passed!");
  return 0;
}
