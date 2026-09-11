// SPDX-License-Identifier: Apache-2.0

/// \file doship_test.cc
/// \brief Unit tests for doship() turn simulation actions: domass, doown,
/// habitat population/resource growth, and weapon plant production.

import dallib;
import gb.entities;
import gb.repositories;
import gb.services;
import gb.turn;
import test;
import std;

namespace {

void expect_near(double actual, double expected, double tolerance = 1e-5) {
  test::expect_true(std::abs(actual - expected) <= tolerance,
                    std::format("Expected {} to be near {}, difference is {}",
                                actual, expected, std::abs(actual - expected)));
}

Race createTestRace(player_t playernum = player_t{1}) {
  Race race{};
  race.Playernum = playernum;
  race.mass = 1.0;
  race.birthrate = 0.1;
  race.tech = 50.0;
  return race;
}

Star createTestStar(starnum_t id = starnum_t{1}) {
  star_struct sdata{
      .name = "TestStar",
      .pnames = {"Earth"},
      .star_id = id,
  };
  return Star{sdata};
}

void test_domass_and_doown() {
  TestContext ctx;
  ctx.with_standard_universe();

  shipnum_t parent_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
                            .owned_by(1)
                            .in_star_orbit(0)
                            .build();

  shipnum_t child_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                           .owned_by(2)
                           .docked_to(parent_id, 0)
                           .with_crew(10, 0)
                           .build();

  ctx.em.mutate_ship(parent_id, [&](Ship& parent) {
    parent.ships() = child_id;
    doown(parent, ctx.em);
  });

  const auto* child = ctx.em.peek_ship(child_id);
  test::expect_eq(child->owner(), player_t{1});

  ctx.em.mutate_ship(parent_id, [&](Ship& parent) {
    domass(parent, ctx.em);
    test::expect_gt(parent.mass(), 0.0);
  });
}

void test_do_habitat() {
  seed_rand(42);
  TestContext ctx;
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) { r.birthrate = 0.1; });

  shipnum_t ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                          .owned_by(1)
                          .in_star_orbit(0)
                          .with_fuel(100.0)
                          .with_crew(1000, 0)
                          .with_resource(10)
                          .with_on(true)
                          .build();

  ctx.em.mutate_ship(ship_id, [&](Ship& ship) {
    do_habitat(ship, ctx.em);
    test::expect_gt(ship.resource(), 10);
    test::expect_gt(ship.popn(), 1000);
  });
}

void test_do_weapon_plant() {
  seed_rand(42);
  TestContext ctx;
  ctx.with_standard_universe();

  shipnum_t ship_id = TestShipBuilder(ctx.em, ShipType::OTYPE_WPLANT)
                          .owned_by(1)
                          .in_star_orbit(0)
                          .with_fuel(100.0)
                          .with_crew(100, 0)
                          .with_resource(500)
                          .build();

  ctx.em.mutate_ship(ship_id, [&](Ship& ship) {
    int produced = do_weapon_plant(ship, ctx.em);
    test::expect_gt(produced, 0);
    test::expect_lt(ship.resource(), 500);
  });
}

void test_do_habitat_zero_rate_and_offline() {
  TestContext ctx;
  ctx.with_standard_universe();

  // Case 1: Ship offline (on == 0)
  {
    shipnum_t id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                       .owned_by(1)
                       .in_star_orbit(0)
                       .with_fuel(100.0)
                       .with_crew(1000, 0)
                       .with_resource(10)
                       .with_on(false)
                       .build();

    ctx.em.mutate_ship(id, [&](Ship& ship) {
      do_habitat(ship, ctx.em);
      expect_near(ship.fuel(), 100.0);
      test::expect_eq(ship.resource(), 10);
    });
  }

  // Case 2: Zero crew (crew_ratio == 0)
  {
    shipnum_t id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                       .owned_by(1)
                       .in_star_orbit(0)
                       .with_fuel(100.0)
                       .with_crew(0, 0)
                       .with_resource(10)
                       .with_on(true)
                       .build();

    ctx.em.mutate_ship(id, [&](Ship& ship) {
      do_habitat(ship, ctx.em);
      expect_near(ship.fuel(), 100.0);
      test::expect_eq(ship.resource(), 10);
    });
  }

  // Case 3: 100% damage (hull_efficiency == 0)
  {
    shipnum_t id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                       .owned_by(1)
                       .in_star_orbit(0)
                       .with_fuel(100.0)
                       .with_crew(1000, 0)
                       .with_resource(10)
                       .with_damage(100)
                       .with_on(true)
                       .build();

    ctx.em.mutate_ship(id, [&](Ship& ship) {
      do_habitat(ship, ctx.em);
      expect_near(ship.fuel(), 100.0);
      test::expect_eq(ship.resource(), 10);
    });
  }
}

void test_do_habitat_capacity_capped() {
  TestContext ctx;
  ctx.with_standard_universe();

  // Full resource capacity: available_resource_capacity() == 0
  {
    shipnum_t id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                       .owned_by(1)
                       .in_star_orbit(0)
                       .with_fuel(100.0)
                       .with_crew(2000, 0)
                       .with_resource(5000)
                       .with_on(true)
                       .build();

    ctx.em.mutate_ship(id, [&](Ship& ship) {
      do_habitat(ship, ctx.em);
      expect_near(ship.fuel(), 100.0);
      test::expect_eq(ship.resource(), 5000);
    });
  }

  // Partial capacity: only 1 unit of resource room
  {
    shipnum_t id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                       .owned_by(1)
                       .in_star_orbit(0)
                       .with_fuel(100.0)
                       .with_crew(2000, 0)
                       .with_resource(4999)
                       .with_on(true)
                       .build();

    // fuse would be 100.0, add would be 5, but room is only 1.
    // add = 1, fuse = 20.0, fuel consumed = 20.0 -> remaining fuel 80.0
    ctx.em.mutate_ship(id, [&](Ship& ship) {
      do_habitat(ship, ctx.em);
      test::expect_eq(ship.resource(), 5000);
      expect_near(ship.fuel(), 80.0);
    });
  }
}

void test_do_habitat_nested_weapon_plant() {
  seed_rand(42);
  TestContext ctx;
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) { r.tech = 50.0; });

  shipnum_t hab_id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                         .owned_by(1)
                         .in_star_orbit(0)
                         .with_fuel(100.0)
                         .with_crew(100, 0)
                         .with_resource(500)
                         .with_destruct(0)
                         .with_on(true)
                         .build();

  shipnum_t wplant_id = TestShipBuilder(ctx.em, ShipType::OTYPE_WPLANT)
                            .owned_by(1)
                            .docked_to(hab_id, 0)
                            .with_fuel(50.0)
                            .with_crew(50, 0)
                            .with_resource(100)
                            .build();

  ctx.em.mutate_ship(hab_id, [&](Ship& habitat) {
    habitat.ships() = wplant_id;
    do_habitat(habitat, ctx.em);
    test::expect_gt(habitat.destruct(), 0);
  });

  const auto* wplant = ctx.em.peek_ship(wplant_id);
  test::expect_lt(wplant->resource(), 100);
  test::expect_lt(wplant->fuel(), 50.0);
}

void test_do_weapon_plant_zero_rate_and_shortages() {
  TestContext ctx;
  ctx.with_standard_universe();

  // Case 1: 100% damage (hull_efficiency == 0)
  {
    shipnum_t id = TestShipBuilder(ctx.em, ShipType::OTYPE_WPLANT)
                       .owned_by(1)
                       .in_star_orbit(0)
                       .with_fuel(100.0)
                       .with_crew(100, 0)
                       .with_resource(500)
                       .with_damage(100)
                       .build();

    ctx.em.mutate_ship(id, [&](Ship& ship) {
      int produced = do_weapon_plant(ship, ctx.em);
      test::expect_eq(produced, 0);
      expect_near(ship.fuel(), 100.0);
      test::expect_eq(ship.resource(), 500);
    });
  }

  // Case 2: Zero crew (crew_ratio == 0)
  {
    shipnum_t id = TestShipBuilder(ctx.em, ShipType::OTYPE_WPLANT)
                       .owned_by(1)
                       .in_star_orbit(0)
                       .with_fuel(100.0)
                       .with_crew(0, 0)
                       .with_resource(500)
                       .build();

    ctx.em.mutate_ship(id, [&](Ship& ship) {
      int produced = do_weapon_plant(ship, ctx.em);
      test::expect_eq(produced, 0);
      expect_near(ship.fuel(), 100.0);
      test::expect_eq(ship.resource(), 500);
    });
  }

  // Case 3: Zero fuel
  {
    shipnum_t id = TestShipBuilder(ctx.em, ShipType::OTYPE_WPLANT)
                       .owned_by(1)
                       .in_star_orbit(0)
                       .with_fuel(0.0)
                       .with_crew(100, 0)
                       .with_resource(500)
                       .build();

    ctx.em.mutate_ship(id, [&](Ship& ship) {
      int produced = do_weapon_plant(ship, ctx.em);
      test::expect_eq(produced, 0);
      test::expect_eq(ship.resource(), 500);
    });
  }

  // Case 4: Zero resource
  {
    shipnum_t id = TestShipBuilder(ctx.em, ShipType::OTYPE_WPLANT)
                       .owned_by(1)
                       .in_star_orbit(0)
                       .with_fuel(100.0)
                       .with_crew(100, 0)
                       .with_resource(0)
                       .build();

    ctx.em.mutate_ship(id, [&](Ship& ship) {
      int produced = do_weapon_plant(ship, ctx.em);
      test::expect_eq(produced, 0);
      expect_near(ship.fuel(), 100.0);
    });
  }
}

void test_do_weapon_plant_tech_capping_and_consumption() {
  TestContext ctx;
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) { r.tech = 4.0; });

  shipnum_t id = TestShipBuilder(ctx.em, ShipType::OTYPE_WPLANT)
                     .owned_by(1)
                     .in_star_orbit(0)
                     .with_fuel(100.0)
                     .with_crew(100, 0)
                     .with_resource(500)
                     .build();

  ctx.em.mutate_ship(id, [&](Ship& ship) {
    int produced = do_weapon_plant(ship, ctx.em);
    // Tech = 4.0 caps production to at most 2
    test::expect_eq(produced, 2);
    test::expect_eq(ship.resource(), 500 - 2 * RES_COST_WPLANT);
    expect_near(ship.fuel(), 100.0 - 2.0 * FUEL_COST_WPLANT);
  });
}

void test_do_meta_infect() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race1 = createTestRace(player_t{1});
  race1.number_sexes = 2;
  race1.likesbest = SectorType::SEC_LAND;

  Race race2 = createTestRace(player_t{2});
  race2.number_sexes = 2;
  race2.fighters = 0.0;

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  Star star = createTestStar(starnum_t{1});
  StarRepository(store).save(star);

  Planet planet{PlanetType::EARTH, Coordinates{2, 2}};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  PlanetRepository(store).save(planet);

  SectorMap smap(planet);
  smap.get({0, 0}).set_owner(0);
  smap.get({0, 0}).set_type(SectorType::SEC_LAND);
  smap.get({1, 0}).set_owner(1);
  smap.get({1, 0}).set_type(SectorType::SEC_LAND);
  smap.get({0, 1}).set_owner(2);
  smap.get({0, 1}).set_type(SectorType::SEC_LAND);
  smap.get({0, 1}).set_troops(0);
  smap.get({1, 1}).set_owner(2);
  smap.get({1, 1}).set_type(SectorType::SEC_LAND);
  smap.get({1, 1}).set_troops(1000);

  SectorRepository(store).save_map(smap);

  // Infect planet sector
  do_meta_infect(player_t{1}, starnum_t{1}, planetnum_t{0}, planet, em);
  test::expect_gt(planet.info(player_t{1}).numsectsowned, 0);
  test::expect_eq(planet.info(player_t{1}).explored, 1);
}

void test_intercept_missile_by_pdn() {
  TestContext ctx;
  ctx.with_standard_universe();

  // Create missile targeting Planet (0, 0)
  shipnum_t missile_id = TestShipBuilder(ctx.em, ShipType::STYPE_MISSILE)
                             .owned_by(1)
                             .in_planet_orbit(0, 0)
                             .targeting_planet(0, 0)
                             .build();

  // 1. Non-PDN ship (shuttle) does not intercept
  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
      .owned_by(2)
      .in_planet_orbit(0, 0)
      .build();

  ctx.em.mutate_ship(missile_id, [&](Ship& m) {
    test::expect_false(intercept_missile_by_pdn(m, ctx.em));
    test::expect_eq(m.whatdest(), ScopeLevel::LEVEL_PLAN);
  });

  // 2. Dead PDN does not intercept
  TestShipBuilder(ctx.em, ShipType::OTYPE_PLANDEF)
      .owned_by(2)
      .in_planet_orbit(0, 0)
      .with_alive(false)
      .build();

  ctx.em.mutate_ship(missile_id, [&](Ship& m) {
    test::expect_false(intercept_missile_by_pdn(m, ctx.em));
    test::expect_eq(m.whatdest(), ScopeLevel::LEVEL_PLAN);
  });

  // 3. Active alive PDN intercepts and redirects missile
  shipnum_t pdn_id = TestShipBuilder(ctx.em, ShipType::OTYPE_PLANDEF)
                         .owned_by(2)
                         .in_planet_orbit(0, 0, SystemCoordinates{15.0, 25.0})
                         .build();

  ctx.em.mutate_ship(missile_id, [&](Ship& m) {
    test::expect_true(intercept_missile_by_pdn(m, ctx.em));
    test::expect_eq(m.whatdest(), ScopeLevel::LEVEL_SHIP);
    test::expect_eq(m.destshipno(), pdn_id);
    const auto* pdn = ctx.em.peek_ship(pdn_id);
    test::expect_eq(m.coordinates().x, pdn->coordinates().x);
    test::expect_eq(m.coordinates().y, pdn->coordinates().y);
  });
}

void test_execute_missile_planet_strike() {
  TestContext ctx;
  ctx.with_standard_universe();

  // Colonize sector (5, 3) on Planet (0, 0) owned by Player 2
  ctx.em.mutate_sectormap(0, 0, [](SectorMap& smap) {
    smap.get(Coordinates{5, 3}).colonize(2, 500);
  });

  // 1. Targeted strike with positive coordinate wrapping (15, 3) on 10x10
  // planet -> wraps to (5, 3)
  shipnum_t m1_id = TestShipBuilder(ctx.em, ShipType::STYPE_MISSILE)
                        .owned_by(1)
                        .in_planet_orbit(0, 0)
                        .targeting_planet(0, 0)
                        .with_destruct(20)
                        .with_impact(Coordinates{15, 3}, /*scatter=*/false)
                        .with_on(true)
                        .build();

  ctx.em.mutate_ship(m1_id, [&](Ship& m) {
    execute_missile_planet_strike(m, ctx.em);
    test::expect_false(m.alive());
  });

  const auto& smap_after1 = *ctx.em.peek_sectormap(0, 0);
  const auto& sec1 = smap_after1.get(Coordinates{5, 3});
  test::expect_true(sec1.is_wasted() || sec1.get_popn() < 500);

  // 2. Targeted strike with negative coordinate wrapping (-1, 2) on 10x10
  // planet -> wraps to (9, 2)
  ctx.em.mutate_sectormap(0, 0, [](SectorMap& smap) {
    smap.get(Coordinates{9, 2}).colonize(2, 500);
  });

  shipnum_t m2_id = TestShipBuilder(ctx.em, ShipType::STYPE_MISSILE)
                        .owned_by(1)
                        .in_planet_orbit(0, 0)
                        .targeting_planet(0, 0)
                        .with_destruct(20)
                        .with_impact(Coordinates{-1, 2}, /*scatter=*/false)
                        .with_on(true)
                        .build();

  ctx.em.mutate_ship(m2_id, [&](Ship& m) {
    execute_missile_planet_strike(m, ctx.em);
    test::expect_false(m.alive());
  });

  const auto& smap_after2 = *ctx.em.peek_sectormap(0, 0);
  const auto& sec2 = smap_after2.get(Coordinates{9, 2});
  test::expect_true(sec2.is_wasted() || sec2.get_popn() < 500);

  // 3. Scattered strike
  ctx.em.mutate_sectormap(0, 0, [](SectorMap& smap) {
    for (auto& sec : smap) {
      sec.colonize(2, 500);
    }
  });

  shipnum_t m3_id = TestShipBuilder(ctx.em, ShipType::STYPE_MISSILE)
                        .owned_by(1)
                        .in_planet_orbit(0, 0)
                        .targeting_planet(0, 0)
                        .with_destruct(20)
                        .with_impact(Coordinates{0, 0}, /*scatter=*/true)
                        .with_on(true)
                        .build();

  ctx.em.mutate_ship(m3_id, [&](Ship& m) {
    execute_missile_planet_strike(m, ctx.em);
    test::expect_false(m.alive());
  });

  const auto& smap_after3 = *ctx.em.peek_sectormap(0, 0);
  int damaged = 0;
  for (const auto& sec : smap_after3) {
    if (sec.is_wasted() || sec.get_popn() < 500) {
      ++damaged;
    }
  }
  test::expect_gt(damaged, 0);
}

void test_execute_missile_ship_strike() {
  TestContext ctx;
  ctx.with_standard_universe();

  shipnum_t target_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                            .owned_by(2)
                            .in_star_orbit(0, SystemCoordinates{0.0, 0.0})
                            .with_size(10)
                            .with_tech(10.0)
                            .build();

  // 1. Target out of strike range (dist = 500.0, strike_range = 10 * 5.5 * 1.0
  // = 55.0)
  shipnum_t distant_missile_id =
      TestShipBuilder(ctx.em, ShipType::STYPE_MISSILE)
          .owned_by(1)
          .in_star_orbit(0, SystemCoordinates{500.0, 0.0})
          .targeting_ship(target_id)
          .with_speed(10)
          .with_destruct(20)
          .with_on(true)
          .build();

  ctx.em.mutate_ship(distant_missile_id, [&](Ship& m) {
    execute_missile_ship_strike(m, ctx.em);
    test::expect_true(m.alive());
  });
  const auto* target_before = ctx.em.peek_ship(target_id);
  test::expect_eq(target_before->damage(), 0);

  // 2. Target within strike range (dist = 10.0 <= 55.0)
  shipnum_t close_missile_id =
      TestShipBuilder(ctx.em, ShipType::STYPE_MISSILE)
          .owned_by(1)
          .in_star_orbit(0, SystemCoordinates{10.0, 0.0})
          .targeting_ship(target_id)
          .with_speed(10)
          .with_destruct(20)
          .with_on(true)
          .build();

  ctx.em.mutate_ship(close_missile_id, [&](Ship& m) {
    execute_missile_ship_strike(m, ctx.em);
    test::expect_false(m.alive());
  });
  const auto* target_after = ctx.em.peek_ship(target_id);
  test::expect_gt(target_after->damage(), 0);
}

void test_domissile_integration() {
  TestContext ctx;
  ctx.with_standard_universe();

  // 1. Missile arrives at planet with PDN present -> re-targeted to PDN
  shipnum_t pdn_id = TestShipBuilder(ctx.em, ShipType::OTYPE_PLANDEF)
                         .owned_by(2)
                         .in_planet_orbit(0, 0, SystemCoordinates{10.0, 10.0})
                         .build();

  shipnum_t m1_id = TestShipBuilder(ctx.em, ShipType::STYPE_MISSILE)
                        .owned_by(1)
                        .in_planet_orbit(0, 0)
                        .targeting_planet(0, 0)
                        .with_destruct(20)
                        .with_on(true)
                        .build();

  ctx.em.mutate_ship(m1_id, [&](Ship& m) {
    domissile(m, ctx.em);
    test::expect_true(m.alive());
    test::expect_eq(m.whatdest(), ScopeLevel::LEVEL_SHIP);
    test::expect_eq(m.destshipno(), pdn_id);
  });

  // Remove PDN for subsequent tests
  ctx.em.mutate_ship(pdn_id, [](Ship& s) { s.alive() = false; });

  // 2. Missile arrives at planet without PDN -> strikes planet surface
  ctx.em.mutate_sectormap(0, 0, [](SectorMap& smap) {
    smap.get(Coordinates{2, 2}).colonize(2, 500);
  });

  shipnum_t m2_id = TestShipBuilder(ctx.em, ShipType::STYPE_MISSILE)
                        .owned_by(1)
                        .in_planet_orbit(0, 0)
                        .targeting_planet(0, 0)
                        .with_destruct(20)
                        .with_impact(Coordinates{2, 2}, /*scatter=*/false)
                        .with_on(true)
                        .build();

  ctx.em.mutate_ship(m2_id, [&](Ship& m) {
    domissile(m, ctx.em);
    test::expect_false(m.alive());
  });
  const auto& smap = *ctx.em.peek_sectormap(0, 0);
  const auto& sec = smap.get(Coordinates{2, 2});
  test::expect_true(sec.is_wasted() || sec.get_popn() < 500);

  // 3. Missile arrives targeting ship in range -> strikes target
  shipnum_t victim_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                            .owned_by(2)
                            .in_planet_orbit(0, 0, SystemCoordinates{0.0, 0.0})
                            .with_size(10)
                            .with_tech(10.0)
                            .build();

  shipnum_t m3_id = TestShipBuilder(ctx.em, ShipType::STYPE_MISSILE)
                        .owned_by(1)
                        .in_planet_orbit(0, 0, SystemCoordinates{5.0, 0.0})
                        .targeting_ship(victim_id)
                        .with_speed(10)
                        .with_destruct(20)
                        .with_on(true)
                        .build();

  ctx.em.mutate_ship(m3_id, [&](Ship& m) {
    domissile(m, ctx.em);
    test::expect_false(m.alive());
  });
  const auto* victim = ctx.em.peek_ship(victim_id);
  test::expect_gt(victim->damage(), 0);
}

void test_check_mine_proximity_trigger() {
  TestContext ctx;
  ctx.with_standard_universe();
  TestWorldBuilder(ctx).add_race("Vulcans", 100.0, false, player_t{3});
  ctx.em.mutate_race(1, [](Race& r) { r.declare_alliance_with(3); });
  ctx.em.mutate_race(3, [](Race& r) { r.declare_alliance_with(1); });

  // Player 1 mine at (0.0, 0.0) with trigger radius 20
  shipnum_t mine_id = TestShipBuilder(ctx.em, ShipType::STYPE_MINE)
                          .owned_by(1)
                          .in_star_orbit(0, SystemCoordinates{0.0, 0.0})
                          .with_destruct(50)
                          .with_trigger_radius(20)
                          .with_on(true)
                          .build();

  // 1. Enemy ship out of range (distance 25 > 20)
  shipnum_t enemy_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                           .owned_by(2)
                           .in_star_orbit(0, SystemCoordinates{25.0, 0.0})
                           .build();

  ctx.em.with_ship(mine_id, [&](const Ship& mine) {
    test::expect_false(check_mine_proximity_trigger(mine, ctx.em));
  });

  // 2. Enemy ship moves into range (distance 15 <= 20)
  ctx.em.mutate_ship(enemy_id, [](Ship& s) {
    s.set_coordinates(UniverseCoordinates{15.0, 0.0});
  });
  ctx.em.with_ship(mine_id, [&](const Ship& mine) {
    test::expect_true(check_mine_proximity_trigger(mine, ctx.em));
  });

  // 3. Allied ship in range (distance 5 <= 20) does not trigger
  // Move enemy far away first
  ctx.em.mutate_ship(enemy_id, [](Ship& s) {
    s.set_coordinates(UniverseCoordinates{500.0, 0.0});
  });
  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
      .owned_by(3)
      .in_star_orbit(0, SystemCoordinates{5.0, 0.0})
      .build();
  ctx.em.with_ship(mine_id, [&](const Ship& mine) {
    test::expect_false(check_mine_proximity_trigger(mine, ctx.em));
  });

  // 4. Own ship in range (distance 0 <= 20) does not trigger
  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
      .owned_by(1)
      .in_star_orbit(0, SystemCoordinates{0.0, 0.0})
      .build();
  ctx.em.with_ship(mine_id, [&](const Ship& mine) {
    test::expect_false(check_mine_proximity_trigger(mine, ctx.em));
  });

  // 5. Offline mine (on = false) does not trigger even with enemy nearby
  ctx.em.mutate_ship(enemy_id, [](Ship& s) {
    s.set_coordinates(UniverseCoordinates{5.0, 0.0});
  });
  ctx.em.mutate_ship(mine_id, [](Ship& m) { m.on() = false; });
  ctx.em.with_ship(mine_id, [&](const Ship& mine) {
    test::expect_false(check_mine_proximity_trigger(mine, ctx.em));
  });

  // 6. Dead mine (alive = false) does not trigger
  ctx.em.mutate_ship(mine_id, [](Ship& m) {
    m.on() = true;
    m.alive() = false;
  });
  ctx.em.with_ship(mine_id, [&](const Ship& mine) {
    test::expect_false(check_mine_proximity_trigger(mine, ctx.em));
  });

  // 7. Planet-orbit mine triggers only on ships orbiting the same planet
  shipnum_t plan_mine_id =
      TestShipBuilder(ctx.em, ShipType::STYPE_MINE)
          .owned_by(1)
          .in_planet_orbit(0, 0, SystemCoordinates{0.0, 0.0})
          .with_destruct(50)
          .with_trigger_radius(20)
          .with_on(true)
          .build();

  // Enemy on planet 1 does not trigger planet 0 mine
  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
      .owned_by(2)
      .in_planet_orbit(0, 1, SystemCoordinates{0.0, 0.0})
      .build();
  ctx.em.with_ship(plan_mine_id, [&](const Ship& mine) {
    test::expect_false(check_mine_proximity_trigger(mine, ctx.em));
  });

  // Enemy on planet 0 within range triggers!
  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
      .owned_by(2)
      .in_planet_orbit(0, 0, SystemCoordinates{10.0, 0.0})
      .build();
  ctx.em.with_ship(plan_mine_id, [&](const Ship& mine) {
    test::expect_true(check_mine_proximity_trigger(mine, ctx.em));
  });
}

void test_detonate_mine_against_ships() {
  TestContext ctx;
  ctx.with_standard_universe();

  shipnum_t mine_id = TestShipBuilder(ctx.em, ShipType::STYPE_MINE)
                          .owned_by(1)
                          .in_star_orbit(0)
                          .with_destruct(50)
                          .with_on(true)
                          .build();

  shipnum_t target_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                            .owned_by(2)
                            .in_star_orbit(0)
                            .with_size(10)
                            .with_tech(10.0)
                            .build();

  shipnum_t dead_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                          .owned_by(2)
                          .in_star_orbit(0)
                          .build();
  ctx.em.mutate_ship(dead_id, [](Ship& s) { s.alive() = false; });

  shipnum_t can_id = TestShipBuilder(ctx.em, ShipType::OTYPE_CANIST)
                         .owned_by(2)
                         .in_star_orbit(0)
                         .build();

  ctx.em.mutate_ship(
      mine_id, [&](Ship& mine) { detonate_mine_against_ships(mine, ctx.em); });

  const auto* target = ctx.em.peek_ship(target_id);
  test::expect_gt(target->damage(), 0);

  const auto* dead = ctx.em.peek_ship(dead_id);
  test::expect_false(dead->alive());

  const auto* can = ctx.em.peek_ship(can_id);
  test::expect_eq(can->damage(), 0);
}

void test_detonate_mine_against_planet() {
  TestContext ctx;
  ctx.with_standard_universe();

  // Populate sectors on Planet (0, 0)
  int initial_populated_sectors = 0;
  ctx.em.mutate_sectormap(0, 0, [&](SectorMap& smap) {
    for (auto& sec : smap) {
      sec.colonize(2, 500);
      ++initial_populated_sectors;
    }
  });

  // 1. Star-orbit mine does not damage planet
  shipnum_t star_mine_id = TestShipBuilder(ctx.em, ShipType::STYPE_MINE)
                               .owned_by(1)
                               .in_star_orbit(0)
                               .with_destruct(100)
                               .with_on(true)
                               .build();

  ctx.em.mutate_ship(star_mine_id, [&](Ship& mine) {
    detonate_mine_against_planet(mine, "Test detonation", ctx.em);
  });
  const auto& smap_star = *ctx.em.peek_sectormap(0, 0);
  int populated_sectors = 0;
  for (const auto& sec : smap_star) {
    if (sec.is_populated()) ++populated_sectors;
  }
  test::expect_eq(populated_sectors, initial_populated_sectors);

  // 2. Planet-orbit mine detonates against planet
  shipnum_t plan_mine_id = TestShipBuilder(ctx.em, ShipType::STYPE_MINE)
                               .owned_by(1)
                               .in_planet_orbit(0, 0)
                               .with_destruct(100)
                               .with_on(true)
                               .build();

  ctx.em.mutate_ship(plan_mine_id, [&](Ship& mine) {
    detonate_mine_against_planet(mine, "Orbital detonation", ctx.em);
  });
  const auto& smap_after = *ctx.em.peek_sectormap(0, 0);
  int damaged_sectors = 0;
  for (const auto& sec : smap_after) {
    if (sec.is_wasted() || sec.get_popn() < 500) {
      ++damaged_sectors;
    }
  }
  test::expect_gt(damaged_sectors, 0);
}

void test_domine_trigger_and_detonation() {
  TestContext ctx;
  ctx.with_standard_universe();
  TestWorldBuilder(ctx).add_race("Vulcans", 100.0, false, player_t{3});
  ctx.em.mutate_race(1, [](Race& r) { r.declare_alliance_with(3); });
  ctx.em.mutate_race(3, [](Race& r) { r.declare_alliance_with(1); });

  // 1. Allied ship in trigger range does NOT trigger mine detonation
  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
      .owned_by(3)
      .in_star_orbit(0, SystemCoordinates{5.0, 5.0})
      .with_size(10)
      .with_tech(10.0)
      .build();

  shipnum_t mine_id = TestShipBuilder(ctx.em, ShipType::STYPE_MINE)
                          .owned_by(1)
                          .in_star_orbit(0, SystemCoordinates{0.0, 0.0})
                          .with_size(1)
                          .with_tech(10.0)
                          .with_destruct(50)
                          .with_trigger_radius(20)
                          .with_on(true)
                          .build();

  ctx.em.mutate_ship(mine_id, [&](Ship& m) {
    domine(m, /*detonate=*/false, ctx.em);
    test::expect_true(m.alive());
  });

  // 2. Enemy ship in trigger range triggers natural mine detonation
  shipnum_t enemy_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                           .owned_by(2)
                           .in_star_orbit(0, SystemCoordinates{5.0, 5.0})
                           .with_size(10)
                           .with_tech(10.0)
                           .build();

  ctx.em.mutate_ship(mine_id, [&](Ship& m) {
    domine(m, /*detonate=*/false, ctx.em);
    test::expect_false(m.alive());
  });

  const auto* enemy_after = ctx.em.peek_ship(enemy_id);
  test::expect_gt(enemy_after->damage(), 0);

  // 3. Forced detonation (detonate = true) in planet orbit detonates without
  // proximity
  shipnum_t plan_mine_id =
      TestShipBuilder(ctx.em, ShipType::STYPE_MINE)
          .owned_by(1)
          .in_planet_orbit(0, 0, SystemCoordinates{0.0, 0.0})
          .with_destruct(50)
          .with_trigger_radius(20)
          .with_on(true)
          .build();

  shipnum_t plan_target_id =
      TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
          .owned_by(2)
          .in_planet_orbit(0, 0, SystemCoordinates{100.0, 100.0})
          .with_size(10)
          .with_tech(10.0)
          .build();

  ctx.em.mutate_ship(plan_mine_id, [&](Ship& m) {
    domine(m, /*detonate=*/true, ctx.em);
    test::expect_false(m.alive());
  });

  const auto* plan_target_after = ctx.em.peek_ship(plan_target_id);
  test::expect_gt(plan_target_after->damage(), 0);
}

void test_doabm_intercept() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race1 = createTestRace(player_t{1});
  Race race2 = createTestRace(player_t{2});
  Race race3 = createTestRace(player_t{3});
  race1.declare_alliance_with(player_t{3});
  race3.declare_alliance_with(player_t{1});
  RaceRepository(store).save(race1);
  RaceRepository(store).save(race2);
  RaceRepository(store).save(race3);

  Star star = createTestStar(starnum_t{1});
  StarRepository(store).save(star);

  Planet planet{PlanetType::EARTH, Coordinates{4, 4}};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  PlanetRepository(store).save(planet);

  // 1. Hostile enemy missile in orbit
  auto hostile_handle = TestShipBuilder(em, ShipType::STYPE_MISSILE)
                            .owned_by(2)
                            .with_size(1)
                            .with_tech(10.0)
                            .in_planet_orbit(1, 0)
                            .with_active(true)
                            .with_alive(true)
                            .build_handle();

  // 2. Allied missile in orbit (should be spared)
  auto allied_handle = TestShipBuilder(em, ShipType::STYPE_MISSILE)
                           .owned_by(3)
                           .with_size(1)
                           .with_tech(10.0)
                           .in_planet_orbit(1, 0)
                           .with_active(true)
                           .with_alive(true)
                           .build_handle();

  em.mutate_planet(starnum_t{1}, planetnum_t{0},
                   [&](Planet& p) { p.ships() = hostile_handle->number(); });
  hostile_handle->ships() = allied_handle->number();

  auto abm_handle = TestShipBuilder(em, ShipType::OTYPE_ABM)
                        .owned_by(1)
                        .with_size(1)
                        .with_max_crew(10)
                        .with_tech(10.0)
                        .with_destruct(50)
                        .with_crew(10, 0)
                        .in_planet_orbit(1, 0)
                        .targeting_planet(1, 0)
                        .with_retaliate(50)
                        .with_guns(guntype_t::HEAVY, 10, ActiveBattery::PRIMARY)
                        .with_active(true)
                        .with_alive(true)
                        .build_handle();
  Ship& abm = *abm_handle;
  abm.on() = 1;
  abm.docked() = 1;

  doabm(abm, em);
  test::expect_lt(abm.destruct(), 50);
  const auto* updated_hostile = em.peek_ship(hostile_handle->number());
  test::expect_gt(updated_hostile->damage(), 0);

  const auto* updated_allied = em.peek_ship(allied_handle->number());
  test::expect_eq(updated_allied->damage(), 0);
}

void test_do_canister_and_greenhouse() {
  TestContext ctx;
  ctx.with_standard_universe();
  TurnStats stats{};

  // 1. Test do_canister
  shipnum_t can_id = TestShipBuilder(ctx.em, ShipType::OTYPE_CANIST)
                         .owned_by(1)
                         .in_planet_orbit(0, 0)
                         .with_special(TimerData{.count = 0})
                         .build();

  ctx.em.mutate_ship(can_id, [&](Ship& canister) {
    auto* canist_ship = canister.as<CanisterShip>();
    test::expect_true(canist_ship != nullptr);

    do_canister(canister, ctx.em, stats);
    test::expect_eq(canist_ship->count(), 1);
    test::expect_eq(stats.temp_add(0, 0), -10);

    // Clamped at -100
    stats.set_temp_add(0, 0, -95);
    do_canister(canister, ctx.em, stats);
    test::expect_eq(stats.temp_add(0, 0), -100);

    // Dissipation on timer expiration
    canist_ship->set_count(DISSIPATE);
    do_canister(canister, ctx.em, stats);
    test::expect_false(canister.alive());
  });

  // 2. Test do_greenhouse
  stats.set_temp_add(0, 0, 0);
  shipnum_t gh_id = TestShipBuilder(ctx.em, ShipType::OTYPE_GREEN)
                        .owned_by(1)
                        .in_planet_orbit(0, 0)
                        .with_special(TimerData{.count = 0})
                        .build();

  ctx.em.mutate_ship(gh_id, [&](Ship& gh) {
    auto* gh_ship = gh.as<CanisterShip>();
    test::expect_true(gh_ship != nullptr);

    do_greenhouse(gh, ctx.em, stats);
    test::expect_eq(gh_ship->count(), 1);
    test::expect_eq(stats.temp_add(0, 0), 10);

    // Clamped at +100
    stats.set_temp_add(0, 0, 95);
    do_greenhouse(gh, ctx.em, stats);
    test::expect_eq(stats.temp_add(0, 0), 100);

    // Dissipation on timer expiration
    gh_ship->set_count(DISSIPATE);
    do_greenhouse(gh, ctx.em, stats);
    test::expect_false(gh.alive());
  });

  // 3. Test do_greenhouse scope and landing guards
  {
    stats.set_temp_add(0, 0, 0);
    shipnum_t landed_gh = TestShipBuilder(ctx.em, ShipType::OTYPE_GREEN)
                              .owned_by(1)
                              .landed_on(0, 0, Coordinates{0, 0})
                              .with_special(TimerData{.count = 0})
                              .build();
    ctx.em.mutate_ship(landed_gh, [&](Ship& gh) {
      do_greenhouse(gh, ctx.em, stats);
      test::expect_eq(gh.as<CanisterShip>()->count(), 0);
      test::expect_eq(stats.temp_add(0, 0), 0);
    });

    shipnum_t star_gh = TestShipBuilder(ctx.em, ShipType::OTYPE_GREEN)
                            .owned_by(1)
                            .in_star_orbit(0)
                            .with_special(TimerData{.count = 0})
                            .build();
    ctx.em.mutate_ship(star_gh, [&](Ship& gh) {
      do_greenhouse(gh, ctx.em, stats);
      test::expect_eq(gh.as<CanisterShip>()->count(), 0);
      test::expect_eq(stats.temp_add(0, 0), 0);
    });
  }

  // 4. Test integrated doship() turn update for greenhouse
  {
    stats.set_temp_add(0, 0, 0);
    shipnum_t turn_gh = TestShipBuilder(ctx.em, ShipType::OTYPE_GREEN)
                            .owned_by(1)
                            .in_planet_orbit(0, 0)
                            .with_special(TimerData{.count = 0})
                            .build();

    // Segment pass (update = false) should NOT trigger greenhouse
    ctx.em.mutate_ship(turn_gh, [&](Ship& gh) {
      doship(gh, /*update=*/false, ctx.em, stats);
      test::expect_eq(gh.as<CanisterShip>()->count(), 0);
      test::expect_eq(stats.temp_add(0, 0), 0);
    });

    // Full update pass (update = true) DOES trigger greenhouse
    ctx.em.mutate_ship(turn_gh, [&](Ship& gh) {
      doship(gh, /*update=*/true, ctx.em, stats);
      test::expect_eq(gh.as<CanisterShip>()->count(), 1);
      test::expect_eq(stats.temp_add(0, 0), 10);
    });
  }
}

void test_do_oap() {
  TestContext ctx;
  ctx.with_standard_universe();
  TurnStats stats{};

  // 1. Direct do_oap on orbiting active online OAP
  shipnum_t oap_id = TestShipBuilder(ctx.em, ShipType::STYPE_OAP)
                         .owned_by(1)
                         .in_planet_orbit(0, 0)
                         .with_on(true)
                         .build();

  test::expect_false(stats.is_intimidated(0, 0));
  ctx.em.mutate_ship(oap_id, [&](Ship& oap) {
    do_oap(oap, stats);
    test::expect_true(stats.is_intimidated(0, 0));
  });

  // 2. Integration via doship() update pass
  stats.set_intimidated(0, 0, false);
  ctx.em.mutate_ship(oap_id, [&](Ship& oap) {
    doship(oap, /*update=*/false, ctx.em, stats);
    test::expect_false(stats.is_intimidated(0, 0));

    doship(oap, /*update=*/true, ctx.em, stats);
    test::expect_true(stats.is_intimidated(0, 0));
  });

  // 3. Domain guards: landed, offline, star orbit, inactive
  {
    // Landed OAP does not intimidate
    stats.set_intimidated(0, 0, false);
    shipnum_t landed_oap = TestShipBuilder(ctx.em, ShipType::STYPE_OAP)
                               .owned_by(1)
                               .landed_on(0, 0, Coordinates{0, 0})
                               .with_on(true)
                               .build();
    ctx.em.mutate_ship(landed_oap, [&](Ship& oap) {
      do_oap(oap, stats);
      test::expect_false(stats.is_intimidated(0, 0));
    });

    // Offline OAP does not intimidate
    stats.set_intimidated(0, 0, false);
    shipnum_t offline_oap = TestShipBuilder(ctx.em, ShipType::STYPE_OAP)
                                .owned_by(1)
                                .in_planet_orbit(0, 0)
                                .with_on(false)
                                .build();
    ctx.em.mutate_ship(offline_oap, [&](Ship& oap) {
      do_oap(oap, stats);
      test::expect_false(stats.is_intimidated(0, 0));
    });

    // Star-orbiting OAP does not intimidate a planet
    stats.set_intimidated(0, 0, false);
    shipnum_t star_oap = TestShipBuilder(ctx.em, ShipType::STYPE_OAP)
                             .owned_by(1)
                             .in_star_orbit(0)
                             .with_on(true)
                             .build();
    ctx.em.mutate_ship(star_oap, [&](Ship& oap) {
      do_oap(oap, stats);
      test::expect_false(stats.is_intimidated(0, 0));
    });

    // Inactive OAP does not intimidate
    stats.set_intimidated(0, 0, false);
    shipnum_t inactive_oap = TestShipBuilder(ctx.em, ShipType::STYPE_OAP)
                                 .owned_by(1)
                                 .in_planet_orbit(0, 0)
                                 .with_active(false)
                                 .with_on(true)
                                 .build();
    ctx.em.mutate_ship(inactive_oap, [&](Ship& oap) {
      do_oap(oap, stats);
      test::expect_false(stats.is_intimidated(0, 0));
    });
  }
}

void test_do_ap_and_god() {
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_race(1, [](Race& r) { r.God = 1; });

  // 1. Test do_god fills god ship to maximum capacity
  shipnum_t god_ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                              .owned_by(1)
                              .with_fuel(0.0)
                              .with_destruct(0)
                              .with_resource(0)
                              .build();

  ctx.em.mutate_ship(god_ship_id, [&](Ship& god_ship) {
    do_god(god_ship, ctx.em);
    test::expect_eq(god_ship.fuel(), god_ship.max_fuel_capacity());
    test::expect_eq(god_ship.destruct(), god_ship.max_destruct_capacity());
    test::expect_eq(god_ship.resource(), god_ship.max_resource_capacity());
  });

  // 2. Test do_ap (modifies planetary atmosphere using ship.crew_ratio())
  ctx.em.mutate_planet(0, 0, [](Planet& p) {
    p.conditions(static_cast<Conditions>(RTEMP + 1)) = 10;
  });

  shipnum_t ap_ship_id = TestShipBuilder(ctx.em, ShipType::OTYPE_AP)
                             .owned_by(1)
                             .landed_on(0, 0, Coordinates{0, 0})
                             .with_fuel(10.0)
                             .with_crew(100, 0)
                             .with_on(true)
                             .build();

  ctx.em.mutate_ship(ap_ship_id, [&](Ship& ap_ship) {
    do_ap(ap_ship, ctx.em);
    test::expect_lt(ap_ship.fuel(), 10.0);
  });
}

void test_do_pod() {
  TestContext ctx;
  ctx.with_standard_universe();

  // 1. Pod in star system with temperature below POD_THRESHOLD -> warms up,
  // remains alive
  shipnum_t warming_pod_id = TestShipBuilder(ctx.em, ShipType::STYPE_POD)
                                 .owned_by(1)
                                 .in_star_orbit(0)
                                 .with_pod(10, 0)
                                 .build();

  ctx.em.mutate_ship(warming_pod_id, [&](Ship& pod) {
    do_pod(pod, ctx.em);
    test::expect_true(pod.alive());
    const auto* pod_ship = pod.as<SporePodShip>();
    test::expect_gt(pod_ship->temperature(), 10);
  });

  // 2. Pod in star system with temperature >= POD_THRESHOLD -> warms, explodes,
  // infects planet
  shipnum_t exploding_pod_id = TestShipBuilder(ctx.em, ShipType::STYPE_POD)
                                   .owned_by(1)
                                   .in_star_orbit(0)
                                   .with_pod(POD_THRESHOLD + 10, 0)
                                   .build();

  ctx.em.mutate_ship(exploding_pod_id, [&](Ship& pod) {
    do_pod(pod, ctx.em);
    test::expect_false(pod.alive());
  });

  // 3. Pod in planet orbit with decay < POD_DECAY -> decays incrementally,
  // remains alive
  shipnum_t decaying_pod_id = TestShipBuilder(ctx.em, ShipType::STYPE_POD)
                                  .owned_by(1)
                                  .in_planet_orbit(0, 0)
                                  .with_pod(0, 1)
                                  .build();

  ctx.em.mutate_ship(decaying_pod_id, [&](Ship& pod) {
    do_pod(pod, ctx.em);
    test::expect_true(pod.alive());
    const auto* pod_ship = pod.as<SporePodShip>();
    test::expect_ge(pod_ship->decay(), 1);
  });

  // 4. Pod in planet orbit with decay >= POD_DECAY -> decays to death, killed
  shipnum_t dead_pod_id = TestShipBuilder(ctx.em, ShipType::STYPE_POD)
                              .owned_by(1)
                              .in_planet_orbit(0, 0)
                              .with_pod(0, POD_DECAY)
                              .build();

  ctx.em.mutate_ship(dead_pod_id, [&](Ship& pod) {
    do_pod(pod, ctx.em);
    test::expect_false(pod.alive());
  });
}

void test_do_mirror() {
  TestContext ctx;
  ctx.with_standard_universe();
  TurnStats stats{};

  // 1. Space mirror aimed at another ship in same star system
  shipnum_t target_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                            .owned_by(2)
                            .in_star_orbit(0, SystemCoordinates{10.0, 10.0})
                            .with_size(10)
                            .with_tech(10.0)
                            .build();

  shipnum_t mirror_ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_MIRROR)
                                 .owned_by(1)
                                 .in_star_orbit(0, SystemCoordinates{0.0, 0.0})
                                 .with_aim(AimedAtData{
                                     .shipno = target_id,
                                     .intensity = 100,
                                     .level = ScopeLevel::LEVEL_SHIP,
                                 })
                                 .build();

  ctx.em.mutate_ship(mirror_ship_id,
                     [&](Ship& mirror) { do_mirror(mirror, ctx.em, stats); });
  const auto* target = ctx.em.peek_ship(target_id);
  test::expect_ge(target->damage(), 0);

  // 2. Space mirror aimed at planet (verifies
  // planet.absolute_coordinates(star))
  shipnum_t mirror_plan_id = TestShipBuilder(ctx.em, ShipType::STYPE_MIRROR)
                                 .owned_by(1)
                                 .in_star_orbit(0, SystemCoordinates{0.0, 0.0})
                                 .with_aim(AimedAtData{
                                     .intensity = 50,
                                     .pnum = 0,
                                     .level = ScopeLevel::LEVEL_PLAN,
                                 })
                                 .build();

  ctx.em.mutate_ship(mirror_plan_id,
                     [&](Ship& mirror) { do_mirror(mirror, ctx.em, stats); });
  test::expect_gt(stats.temp_add(0, 0), 0);

  // 3. Space mirror aimed at star
  int initial_stability = ctx.em.peek_star(0)->stability();
  shipnum_t mirror_star_id = TestShipBuilder(ctx.em, ShipType::STYPE_MIRROR)
                                 .owned_by(1)
                                 .in_star_orbit(0)
                                 .with_aim(AimedAtData{
                                     .snum = 0,
                                     .intensity = 50,
                                     .level = ScopeLevel::LEVEL_STAR,
                                 })
                                 .build();

  ctx.em.mutate_ship(mirror_star_id,
                     [&](Ship& mirror) { do_mirror(mirror, ctx.em, stats); });
  test::expect_ge(ctx.em.peek_star(0)->stability(), initial_stability);

  // 4. Unaimed mirror (LEVEL_UNIV) does nothing
  shipnum_t mirror_unaimed_id =
      TestShipBuilder(ctx.em, ShipType::STYPE_MIRROR)
          .owned_by(1)
          .in_star_orbit(0)
          .with_aim(AimedAtData{.level = ScopeLevel::LEVEL_UNIV})
          .build();

  auto temp_before = stats.temp_add(0, 0);
  ctx.em.mutate_ship(mirror_unaimed_id,
                     [&](Ship& mirror) { do_mirror(mirror, ctx.em, stats); });
  test::expect_eq(stats.temp_add(0, 0), temp_before);
}

void test_ship_domain_operations() {
  ship_struct sdata{
      .fuel = 50.0,
      .mass = 100.0,
      .max_crew = 150,
      .max_resource = 500,
      .max_destruct = 100,
      .max_fuel = 100.0,
      .destruct = 10,
      .resource = 200,
      .popn = 50,
      .troops = 20,
      .damage = 10,
      .rad = 30,
  };
  Ship ship{sdata};

  // 1. Damage clamping
  const auto res1 = ship.apply_damage(50);
  test::expect_eq(res1.damage_applied, 50u);
  test::expect_eq(ship.damage(), 60);
  const auto res2 = ship.apply_damage(60);
  test::expect_eq(res2.damage_applied, 40u);
  test::expect_true(res2.destroyed);
  test::expect_eq(ship.damage(), 100);  // Clamped at 100

  ship.repair_damage(40);
  test::expect_eq(ship.damage(), 60);
  ship.repair_damage(80);
  test::expect_eq(ship.damage(), 0);  // Clamped at 0

  // 2. Radiation repair
  ship.repair_radiation(10);
  test::expect_eq(ship.rad(), 20);
  ship.repair_radiation(50);
  test::expect_eq(ship.rad(), 0);  // Clamped at 0

  // 3. Fuel operations & mass tracking
  double initial_mass = ship.mass();
  ship.consume_fuel(10.0);
  test::expect_eq(ship.fuel(), 40.0);
  test::expect_eq(ship.mass(), initial_mass - 10.0 * MASS_FUEL);

  ship.add_fuel(20.0);
  test::expect_eq(ship.fuel(), 60.0);
  test::expect_eq(ship.mass(), initial_mass + 10.0 * MASS_FUEL);

  // 4. Resource operations & mass tracking
  initial_mass = ship.mass();
  ship.consume_resource(50);
  test::expect_eq(ship.resource(), 150);
  test::expect_eq(ship.mass(), initial_mass - 50.0 * MASS_RESOURCE);

  ship.add_resource(100);
  test::expect_eq(ship.resource(), 250);
  test::expect_eq(ship.mass(), initial_mass + 50.0 * MASS_RESOURCE);

  // 5. Destruct ordnance operations & mass tracking
  initial_mass = ship.mass();
  ship.consume_destruct(5);
  test::expect_eq(ship.destruct(), 5);
  test::expect_eq(ship.mass(), initial_mass - 5.0 * MASS_DESTRUCT);

  ship.add_destruct(15);
  test::expect_eq(ship.destruct(), 20);
  test::expect_eq(ship.mass(), initial_mass + 10.0 * MASS_DESTRUCT);

  // 6. Population & troop cargo additions
  initial_mass = ship.mass();
  ship.add_popn(25, 2.0);
  test::expect_eq(ship.popn(), 75);
  test::expect_eq(ship.mass(), initial_mass + 50.0);

  initial_mass = ship.mass();
  ship.add_troops(10, 2.0);
  test::expect_eq(ship.troops(), 30);
  test::expect_eq(ship.mass(), initial_mass + 20.0);
}

void test_do_repair() {
  TestContext ctx;
  ctx.with_standard_universe();

  // 1. Probe with max_crew = 0 (verifies division-by-zero fix, maxrep = 0)
  shipnum_t probe_id = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                           .owned_by(1)
                           .in_star_orbit(0)
                           .with_damage(50)
                           .with_resource(100)
                           .build();

  ctx.em.mutate_ship(probe_id, [&](Ship& probe) {
    do_repair(probe, ctx.em);
    test::expect_eq(probe.damage(), 50);
    test::expect_eq(probe.resource(), 100);
  });

  // 2. Manned ship with crew repairs damage and consumes resources
  shipnum_t shuttle_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                             .owned_by(1)
                             .in_star_orbit(0)
                             .with_crew(10, 0)
                             .with_resource(100)
                             .with_damage(50)
                             .build();

  ctx.em.mutate_ship(shuttle_id, [&](Ship& shuttle) {
    do_repair(shuttle, ctx.em);
    test::expect_lt(shuttle.damage(), 50);
    test::expect_lt(shuttle.resource(), 100);
  });

  // 3. Ship docked with a space station repairs for free (0 cost) even with 0
  // resources and 0 crew!
  shipnum_t station_id = TestShipBuilder(ctx.em, ShipType::STYPE_STATION)
                             .owned_by(1)
                             .in_star_orbit(0)
                             .build();

  shipnum_t docked_ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                                 .owned_by(1)
                                 .in_star_orbit(0)
                                 .docked_to(station_id, 0)
                                 .with_damage(40)
                                 .with_resource(0)
                                 .build();

  ctx.em.mutate_ship(docked_ship_id, [&](Ship& ship) {
    do_repair(ship, ctx.em);
    test::expect_lt(ship.damage(), 40);
    test::expect_eq(ship.resource(), 0);  // Free repairs from space station!
  });

  // 4. Space station itself repairs for free
  ctx.em.mutate_ship(station_id, [](Ship& s) { s.admin_override_damage(30); });
  ctx.em.mutate_ship(station_id, [&](Ship& station) {
    do_repair(station, ctx.em);
    test::expect_lt(station.damage(), 30);
  });
}

void test_process_ship_radiation() {
  seed_rand(42);

  // 1. Ship with 0 rad is mobile (returns true)
  ship_struct clean_data{
      .popn = 100,
      .troops = 50,
      .rad = 0,
  };
  Ship clean_ship{clean_data};
  test::expect_true(process_ship_radiation(clean_ship, true));
  test::expect_eq(clean_ship.popn(), 100);

  // 2. Ship with radiation on update pass decays crew and repairs rad
  ship_struct rad_data{
      .popn = 100,
      .troops = 50,
      .rad = 20,
  };
  Ship rad_ship{rad_data};
  process_ship_radiation(rad_ship, true);
  test::expect_le(rad_ship.popn(), 100);
  test::expect_le(rad_ship.troops(), 50);
  test::expect_le(rad_ship.rad(), 20);
}

void test_process_ship_supernova() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  RaceRepository(store).save(race);

  star_struct sdata{
      .name = "NovaStar",
      .nova_stage = 2,
      .star_id = starnum_t{1},
  };
  Star star{sdata};
  ServerState state{.segments = 1};

  // 1. Surviving ship
  auto ship_handle = TestShipBuilder(em, ShipType::STYPE_BATTLE)
                         .owned_by(1)
                         .with_armor(2)
                         .with_damage(10)
                         .with_alive(true)
                         .build_handle();
  Ship& ship = *ship_handle;

  bool survived = process_ship_supernova(ship, star, state, em);
  test::expect_true(survived);
  test::expect_gt(ship.damage(), 10);
  test::expect_eq(ship.alive(), 1);

  // 2. Ship destroyed by supernova (damage >= 100)
  ship.admin_override_damage(98);
  survived = process_ship_supernova(ship, star, state, em);
  test::expect_false(survived);
  test::expect_eq(ship.alive(), 0);
}

void test_sync_factory_technology() {
  Race race = createTestRace(player_t{1});
  race.tech = 150.0;

  // 1. Offline factory updates tech
  ship_struct offline_factory_data{
      .tech = 50.0,
      .type = ShipType::OTYPE_FACTORY,
      .on = 0,
  };
  Ship offline_factory{offline_factory_data};
  sync_factory_technology(offline_factory, race);
  test::expect_eq(offline_factory.tech(), 150.0);

  // 2. Online factory preserves tech
  ship_struct online_factory_data{
      .tech = 50.0,
      .type = ShipType::OTYPE_FACTORY,
      .on = 1,
  };
  Ship online_factory{online_factory_data};
  sync_factory_technology(online_factory, race);
  test::expect_eq(online_factory.tech(), 50.0);
}

void test_exploration_domain_methods() {
  // Ship capability
  ship_struct probe_data{.popn = 0, .type = ShipType::OTYPE_PROBE};
  Ship probe{probe_data};
  test::expect_true(probe.is_exploration_capable());

  ship_struct manned_data{.popn = 5, .type = ShipType::STYPE_SHUTTLE};
  Ship manned{manned_data};
  test::expect_true(manned.is_exploration_capable());

  ship_struct uncrewed_data{.popn = 0, .type = ShipType::STYPE_CARGO};
  Ship uncrewed{uncrewed_data};
  test::expect_false(uncrewed.is_exploration_capable());

  // Planet exploration
  Planet planet{PlanetType::EARTH, Coordinates{2, 2}};
  test::expect_false(planet.is_explored_by(player_t{1}));
  planet.mark_explored_by(player_t{1});
  test::expect_true(planet.is_explored_by(player_t{1}));
}

void test_update_ship_inhabited_and_exploration() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);
  TurnStats stats{};

  Race race = createTestRace(player_t{1});
  RaceRepository(store).save(race);

  Star star = createTestStar(starnum_t{1});
  StarRepository(store).save(star);

  Planet planet{PlanetType::EARTH, Coordinates{2, 2}};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  PlanetRepository(store).save(planet);

  // 1. Probe in star orbit explores star
  auto probe_handle = TestShipBuilder(em, ShipType::OTYPE_PROBE)
                          .owned_by(1)
                          .with_crew(0, 0)
                          .in_star_orbit(1)
                          .with_alive(true)
                          .build_handle();
  update_ship_inhabited_and_exploration(*probe_handle, em, stats);
  test::expect_eq(stats.StarsInhab[1], 1);
  const auto& star_after_probe = *em.peek_star(starnum_t{1});
  test::expect_true(star_after_probe.is_explored_by(player_t{1}));

  // 2. Manned ship in planet orbit explores star & planet
  auto manned_handle = TestShipBuilder(em, ShipType::STYPE_SHUTTLE)
                           .owned_by(1)
                           .with_crew(10, 0)
                           .in_planet_orbit(1, 0)
                           .with_alive(true)
                           .build_handle();
  update_ship_inhabited_and_exploration(*manned_handle, em, stats);
  const auto& planet_after_manned =
      *em.peek_planet(starnum_t{1}, planetnum_t{0});
  test::expect_true(planet_after_manned.is_explored_by(player_t{1}));

  // 3. Uncrewed cargo ship does not explore
  Planet planet2{PlanetType::EARTH, Coordinates{2, 2}};
  planet2.star_id() = 1;
  planet2.planet_order() = 1;
  PlanetRepository(store).save(planet2);

  auto cargo_handle = TestShipBuilder(em, ShipType::STYPE_CARGO)
                          .owned_by(2)
                          .with_crew(0, 0)
                          .in_planet_orbit(1, 1)
                          .with_alive(true)
                          .build_handle();
  update_ship_inhabited_and_exploration(*cargo_handle, em, stats);
  const auto& planet2_after = *em.peek_planet(starnum_t{1}, planetnum_t{1});
  test::expect_false(planet2_after.is_explored_by(player_t{2}));
}

void test_synchronize_docked_carrier_ownership() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race1 = createTestRace(player_t{1});
  Race race2 = createTestRace(player_t{2});
  RaceRepository(store).save(race1);
  RaceRepository(store).save(race2);

  // Carrier owned by Player 1
  auto carrier_handle = TestShipBuilder(em, ShipType::STYPE_CARRIER)
                            .owned_by(1)
                            .with_alive(true)
                            .build_handle();

  // Docked fighter initially owned by Player 2
  auto fighter_handle = TestShipBuilder(em, ShipType::STYPE_FIGHTER)
                            .owned_by(2)
                            .docked_to(carrier_handle->number(), 0)
                            .with_alive(true)
                            .build_handle();
  Ship& fighter = *fighter_handle;

  synchronize_docked_carrier_ownership(fighter, em);
  test::expect_eq(fighter.owner(), player_t{1});
}

void test_accumulate_ship_power_stats() {
  TurnStats stats{};

  // 1. Star-orbiting ship during update pass
  ship_struct star_ship_data{
      .owner = player_t{1},
      .fuel = 25.0,
      .destruct = 5,
      .resource = 50,
      .popn = 10,
      .troops = 4,
      .storbits = starnum_t{1},
      .whatorbits = ScopeLevel::LEVEL_STAR,
      .type = ShipType::STYPE_BATTLE,
      .alive = 1,
  };
  Ship star_ship{star_ship_data};

  accumulate_ship_power_stats(star_ship, stats, true);
  test::expect_eq(stats.Power[player_t{1}].ships_owned, 1);
  test::expect_eq(stats.Power[player_t{1}].fuel, 25.0);
  test::expect_eq(stats.Power[player_t{1}].destruct, 5);
  test::expect_eq(stats.Power[player_t{1}].resource, 50);
  test::expect_eq(stats.Power[player_t{1}].popn, 10);
  test::expect_eq(stats.Power[player_t{1}].troops, 4);
  test::expect_eq(stats.starnumships[1][player_t{1}], 1);
  test::expect_eq(stats.starpopns[1][player_t{1}], 10);

  // 2. Deep space ship in LEVEL_UNIV
  ship_struct univ_ship_data{
      .owner = player_t{1},
      .popn = 20,
      .whatorbits = ScopeLevel::LEVEL_UNIV,
      .type = ShipType::STYPE_EXPLORER,
      .alive = 1,
  };
  Ship univ_ship{univ_ship_data};

  accumulate_ship_power_stats(univ_ship, stats, false);
  test::expect_eq(stats.Sdatanumships[player_t{1}], 1);
  test::expect_eq(stats.Sdatapopns[player_t{1}], 20);
}

void test_special_subsystems_extended() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);
  TurnStats stats{};

  Race race = createTestRace(player_t{1});
  race.conditions[RTEMP] = 50;
  race.conditions[OXYGEN] = 80;
  RaceRepository(store).save(race);

  Star star = createTestStar(starnum_t{1});
  StarRepository(store).save(star);

  Planet planet{PlanetType::EARTH, Coordinates{2, 2}};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.conditions(RTEMP) = 10;
  planet.conditions(OXYGEN) = 10;
  PlanetRepository(store).save(planet);

  // 1. Habitat with 0 max_crew does not divide by zero
  auto hab_handle = TestShipBuilder(em, ShipType::STYPE_HABITAT)
                        .owned_by(1)
                        .with_fuel(100.0)
                        .with_max_crew(0)
                        .with_alive(true)
                        .with_on(true)
                        .build_handle();
  do_habitat(*hab_handle, em);
  test::expect_eq(hab_handle->resource(), 0);

  // 2. Weapon plant with 0 max_crew does not divide by zero
  auto wplant_handle = TestShipBuilder(em, ShipType::OTYPE_WPLANT)
                           .owned_by(1)
                           .with_fuel(100.0)
                           .with_max_crew(0)
                           .with_resource(100)
                           .with_alive(true)
                           .with_on(true)
                           .build_handle();
  int produced = do_weapon_plant(*wplant_handle, em);
  test::expect_eq(produced, 0);

  // 3. Canister clamped at -100
  auto can_handle = TestShipBuilder(em, ShipType::OTYPE_CANIST)
                        .owned_by(1)
                        .in_planet_orbit(1, 0)
                        .with_active(true)
                        .with_alive(true)
                        .build_handle();
  stats.set_temp_add(1, 0, -95);
  do_canister(*can_handle, em, stats);
  test::expect_eq(stats.temp_add(1, 0), -100);

  // 4. Greenhouse clamped at +100
  auto gh_handle = TestShipBuilder(em, ShipType::OTYPE_GREEN)
                       .owned_by(1)
                       .in_planet_orbit(1, 0)
                       .with_active(true)
                       .with_alive(true)
                       .build_handle();
  stats.set_temp_add(1, 0, 95);
  do_greenhouse(*gh_handle, em, stats);
  test::expect_eq(stats.temp_add(1, 0), 100);

  // 5. Space mirror destroys target ship when damage exceeds 100
  auto target_handle = TestShipBuilder(em, ShipType::STYPE_SHUTTLE)
                           .owned_by(1)
                           .with_size(10)
                           .in_star_orbit(1)
                           .with_damage(99)
                           .with_alive(true)
                           .build_handle();

  auto mirror_handle = TestShipBuilder(em, ShipType::STYPE_MIRROR)
                           .owned_by(1)
                           .in_star_orbit(1)
                           .with_alive(true)
                           .build_handle();
  auto* mirror_ship = mirror_handle->as<SpaceMirrorShip>();
  mirror_ship->aim().level = ScopeLevel::LEVEL_SHIP;
  mirror_ship->aim().shipno = target_handle->number();
  mirror_ship->aim().intensity = 100;

  do_mirror(*mirror_handle, em, stats);
  const auto* target_after = em.peek_ship(target_handle->number());
  // Destroyed ship is killed via em.kill_ship
  test::expect_true(target_after == nullptr || target_after->alive() == 0);
}

void test_prepare_ship_for_flight() {
  TestContext ctx;
  ctx.with_standard_universe();

  // 1. Dead ship returns false
  shipnum_t dead_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                          .owned_by(1)
                          .with_alive(false)
                          .build();
  ctx.em.mutate_ship(dead_id, [&](Ship& s) {
    test::expect_false(prepare_ship_for_flight(s, true));
  });

  // 2. Unowned ship (owner == 0) is marked dead and returns false
  shipnum_t unowned_id =
      TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE).owned_by(0).build();
  ctx.em.mutate_ship(unowned_id, [&](Ship& s) {
    test::expect_false(prepare_ship_for_flight(s, true));
    test::expect_false(s.alive());
  });

  // 3. Derelict uncrewed manned ship gets redirected to LEVEL_UNIV
  shipnum_t derelict_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                              .owned_by(1)
                              .in_star_orbit(0)
                              .with_crew(0, 0)
                              .build();
  ctx.em.mutate_ship(derelict_id, [&](Ship& s) {
    s.whatdest() = ScopeLevel::LEVEL_PLAN;
    test::expect_true(prepare_ship_for_flight(s, true));
    test::expect_eq(s.whatdest(), ScopeLevel::LEVEL_UNIV);
  });

  // 4. Docked uncrewed manned ship is NOT redirected to LEVEL_UNIV
  shipnum_t station_id = TestShipBuilder(ctx.em, ShipType::STYPE_STATION)
                             .owned_by(1)
                             .in_star_orbit(0)
                             .build();
  shipnum_t docked_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                            .owned_by(1)
                            .in_star_orbit(0)
                            .docked_to(station_id, 0)
                            .with_crew(0, 0)
                            .build();
  ctx.em.mutate_ship(docked_id, [&](Ship& s) {
    s.whatdest() = ScopeLevel::LEVEL_SHIP;
    test::expect_true(prepare_ship_for_flight(s, true));
    test::expect_eq(s.whatdest(), ScopeLevel::LEVEL_SHIP);
  });
}

void test_evaluate_ship_hazards() {
  TestContext ctx;
  ctx.with_standard_universe();

  const auto& state = *ctx.em.peek_server_state();
  test::expect_ge(state.segments, 1);

  // 1. Deep space ship (LEVEL_UNIV) bypasses supernova hazards
  shipnum_t deep_id =
      TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE).owned_by(1).build();
  ctx.em.mutate_ship(deep_id, [&](Ship& s) {
    s.whatorbits() = ScopeLevel::LEVEL_UNIV;
    test::expect_true(evaluate_ship_hazards(s, ctx.em));
  });

  // 2. Star with nova_stage == 0 causes no damage
  shipnum_t calm_star_ship = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                                 .owned_by(1)
                                 .in_star_orbit(0)
                                 .build();
  ctx.em.mutate_ship(calm_star_ship, [&](Ship& s) {
    test::expect_true(evaluate_ship_hazards(s, ctx.em));
    test::expect_eq(s.damage(), 0);
  });

  // 3. Star with nova_stage > 0 damages ship; survives if damage < 100
  ctx.em.mutate_star(0, [](Star& star) { star.nova_stage() = 4; });
  shipnum_t armored_ship = TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
                               .owned_by(1)
                               .in_star_orbit(0)
                               .with_armor(5)
                               .build();
  ctx.em.mutate_ship(armored_ship, [&](Ship& s) {
    test::expect_true(evaluate_ship_hazards(s, ctx.em));
    test::expect_gt(s.damage(), 0);
    test::expect_lt(s.damage(), 100);
  });

  // 4. Unarmored ship taking fatal damage (nova_stage high) is destroyed
  ctx.em.mutate_star(0, [](Star& star) { star.nova_stage() = 25; });
  shipnum_t doomed_ship = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                              .owned_by(1)
                              .in_star_orbit(0)
                              .with_armor(0)
                              .with_damage(95)
                              .build();
  ctx.em.mutate_ship(doomed_ship, [&](Ship& s) {
    test::expect_false(evaluate_ship_hazards(s, ctx.em));
    test::expect_false(s.alive());
  });
}

void test_dispatch_ship_subsystems() {
  TestContext ctx;
  ctx.with_standard_universe();
  TurnStats stats{};

  // 1. Bombarding ship in planet orbit marks planet inhabited
  shipnum_t bombardier_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
                                .owned_by(1)
                                .in_planet_orbit(0, 0)
                                .build();
  ctx.em.mutate_ship(bombardier_id, [&](Ship& s) {
    s.whatdest() = ScopeLevel::LEVEL_PLAN;
    s.deststar() = 0;
    s.destpnum() = 0;
    s.bombard() = 1;
    dispatch_ship_subsystems(s, true, ctx.em, stats);
    test::expect_true(stats.is_inhabited(0, 0));
  });

  // 2. Segment pass (update == false) skips update-only subsystems (e.g.
  // canister)
  shipnum_t can_id = TestShipBuilder(ctx.em, ShipType::OTYPE_CANIST)
                         .owned_by(1)
                         .in_planet_orbit(0, 0)
                         .build();
  ctx.em.mutate_ship(can_id, [&](Ship& s) {
    auto* can = s.as<CanisterShip>();
    test::expect_true(can != nullptr);
    can->set_count(DISSIPATE - 1);
    dispatch_ship_subsystems(s, false, ctx.em, stats);
    // Canister is only updated/dissipated during update pass!
    test::expect_true(s.alive());
    test::expect_eq(can->count(), DISSIPATE - 1);
  });

  // 3. Update pass (update == true) executes canister dissipate & destruction
  ctx.em.mutate_ship(can_id, [&](Ship& s) {
    dispatch_ship_subsystems(s, true, ctx.em, stats);
    test::expect_false(s.alive());
  });
}

void test_doship_pipeline_types() {
  TestContext ctx;
  ctx.with_standard_universe();
  TurnStats stats{};

  // Exercise different ship types through the full doship pipeline:
  // Shuttle, Station, Canister, Habitat, Pod
  shipnum_t shuttle_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                             .owned_by(1)
                             .in_star_orbit(0)
                             .build();
  ctx.em.mutate_ship(shuttle_id, [&](Ship& s) {
    doship(s, true, ctx.em, stats);
    test::expect_true(s.alive());
    test::expect_true(s.active());
  });

  shipnum_t station_id = TestShipBuilder(ctx.em, ShipType::STYPE_STATION)
                             .owned_by(1)
                             .in_star_orbit(0)
                             .build();
  ctx.em.mutate_ship(station_id, [&](Ship& s) {
    doship(s, true, ctx.em, stats);
    test::expect_true(s.alive());
  });

  shipnum_t canist_id = TestShipBuilder(ctx.em, ShipType::OTYPE_CANIST)
                            .owned_by(1)
                            .in_planet_orbit(0, 0)
                            .build();
  ctx.em.mutate_ship(canist_id, [&](Ship& s) {
    doship(s, true, ctx.em, stats);
    test::expect_true(s.alive());
  });

  shipnum_t habitat_id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                             .owned_by(1)
                             .in_star_orbit(0)
                             .with_crew(100, 0)
                             .with_fuel(50.0)
                             .with_on(true)
                             .build();
  ctx.em.mutate_ship(habitat_id, [&](Ship& s) {
    doship(s, true, ctx.em, stats);
    test::expect_true(s.alive());
  });

  shipnum_t pod_id = TestShipBuilder(ctx.em, ShipType::STYPE_POD)
                         .owned_by(1)
                         .in_star_orbit(0)
                         .with_pod(10, 0)
                         .build();
  ctx.em.mutate_ship(pod_id, [&](Ship& s) {
    doship(s, true, ctx.em, stats);
    test::expect_true(s.alive());
  });
}

}  // namespace

int main() {
  std::println(std::cout, "Running doship unit tests...\n");

  std::println(std::cout, "  Testing domass and doown... ");
  test_domass_and_doown();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing do_habitat... ");
  test_do_habitat();
  test_do_habitat_zero_rate_and_offline();
  test_do_habitat_capacity_capped();
  test_do_habitat_nested_weapon_plant();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing do_weapon_plant... ");
  test_do_weapon_plant();
  test_do_weapon_plant_zero_rate_and_shortages();
  test_do_weapon_plant_tech_capping_and_consumption();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing do_meta_infect... ");
  test_do_meta_infect();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing intercept_missile_by_pdn... ");
  test_intercept_missile_by_pdn();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing execute_missile_planet_strike... ");
  test_execute_missile_planet_strike();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing execute_missile_ship_strike... ");
  test_execute_missile_ship_strike();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing domissile integration... ");
  test_domissile_integration();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing check_mine_proximity_trigger... ");
  test_check_mine_proximity_trigger();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing detonate_mine_against_ships... ");
  test_detonate_mine_against_ships();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing detonate_mine_against_planet... ");
  test_detonate_mine_against_planet();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing domine trigger and detonation... ");
  test_domine_trigger_and_detonation();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing doabm intercept... ");
  test_doabm_intercept();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing do_canister and do_greenhouse... ");
  test_do_canister_and_greenhouse();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing do_oap intimidation... ");
  test_do_oap();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing do_ap and do_god... ");
  test_do_ap_and_god();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing do_pod... ");
  test_do_pod();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing do_mirror... ");
  test_do_mirror();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing ship domain operations... ");
  test_ship_domain_operations();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing do_repair... ");
  test_do_repair();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing process_ship_radiation... ");
  test_process_ship_radiation();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing process_ship_supernova... ");
  test_process_ship_supernova();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing sync_factory_technology... ");
  test_sync_factory_technology();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing exploration domain methods... ");
  test_exploration_domain_methods();
  std::println(std::cout, "PASS");

  std::println(std::cout,
               "  Testing update_ship_inhabited_and_exploration... ");
  test_update_ship_inhabited_and_exploration();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing synchronize_docked_carrier_ownership... ");
  test_synchronize_docked_carrier_ownership();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing accumulate_ship_power_stats... ");
  test_accumulate_ship_power_stats();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing special subsystems extended... ");
  test_special_subsystems_extended();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing prepare_ship_for_flight... ");
  test_prepare_ship_for_flight();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing evaluate_ship_hazards... ");
  test_evaluate_ship_hazards();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing dispatch_ship_subsystems... ");
  test_dispatch_ship_subsystems();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing doship pipeline across ship types... ");
  test_doship_pipeline_types();
  std::println(std::cout, "PASS");

  std::println(std::cout, "All doship tests passed!");
  return 0;
}
