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
  ship_struct hostile_missile_data{
      .owner = player_t{2},
      .size = 1,
      .tech = 10.0,
      .storbits = starnum_t{1},
      .pnumorbits = planetnum_t{0},
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::STYPE_MISSILE,
      .active = 1,
      .alive = 1,
  };
  auto hostile_handle = em.create_ship(hostile_missile_data);

  // 2. Allied missile in orbit (should be spared)
  ship_struct allied_missile_data{
      .owner = player_t{3},
      .size = 1,
      .tech = 10.0,
      .storbits = starnum_t{1},
      .pnumorbits = planetnum_t{0},
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::STYPE_MISSILE,
      .active = 1,
      .alive = 1,
  };
  auto allied_handle = em.create_ship(allied_missile_data);

  em.mutate_planet(starnum_t{1}, planetnum_t{0},
                   [&](Planet& p) { p.ships() = hostile_handle->number(); });
  hostile_handle->ships() = allied_handle->number();

  ship_struct abm_data{
      .owner = player_t{1},
      .size = 1,
      .max_crew = 10,
      .tech = 10.0,
      .destruct = 50,
      .popn = 10,
      .storbits = starnum_t{1},
      .deststar = starnum_t{1},
      .destpnum = planetnum_t{0},
      .pnumorbits = planetnum_t{0},
      .whatdest = ScopeLevel::LEVEL_PLAN,
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .retaliate = 50,
      .type = ShipType::OTYPE_ABM,
      .active = 1,
      .alive = 1,
  };
  abm_data.primary_battery = GunBattery::create(10, guntype_t::HEAVY);
  auto abm_handle = em.create_ship(abm_data);
  Ship& abm = *abm_handle;
  abm.guns() = PRIMARY;
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
    test::expect_eq(stats.Stinfo[0][0].temp_add, -10);

    // Clamped at -100
    stats.Stinfo[0][0].temp_add = -95;
    do_canister(canister, ctx.em, stats);
    test::expect_eq(stats.Stinfo[0][0].temp_add, -100);

    // Dissipation on timer expiration
    canist_ship->set_count(DISSIPATE);
    do_canister(canister, ctx.em, stats);
    test::expect_false(canister.alive());
  });

  // 2. Test do_greenhouse
  stats.Stinfo[0][0].temp_add = 0;
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
    test::expect_eq(stats.Stinfo[0][0].temp_add, 10);

    // Clamped at +100
    stats.Stinfo[0][0].temp_add = 95;
    do_greenhouse(gh, ctx.em, stats);
    test::expect_eq(stats.Stinfo[0][0].temp_add, 100);

    // Dissipation on timer expiration
    gh_ship->set_count(DISSIPATE);
    do_greenhouse(gh, ctx.em, stats);
    test::expect_false(gh.alive());
  });

  // 3. Test do_greenhouse scope and landing guards
  {
    stats.Stinfo[0][0].temp_add = 0;
    shipnum_t landed_gh = TestShipBuilder(ctx.em, ShipType::OTYPE_GREEN)
                              .owned_by(1)
                              .landed_on(0, 0, Coordinates{0, 0})
                              .with_special(TimerData{.count = 0})
                              .build();
    ctx.em.mutate_ship(landed_gh, [&](Ship& gh) {
      do_greenhouse(gh, ctx.em, stats);
      test::expect_eq(gh.as<CanisterShip>()->count(), 0);
      test::expect_eq(stats.Stinfo[0][0].temp_add, 0);
    });

    shipnum_t star_gh = TestShipBuilder(ctx.em, ShipType::OTYPE_GREEN)
                            .owned_by(1)
                            .in_star_orbit(0)
                            .with_special(TimerData{.count = 0})
                            .build();
    ctx.em.mutate_ship(star_gh, [&](Ship& gh) {
      do_greenhouse(gh, ctx.em, stats);
      test::expect_eq(gh.as<CanisterShip>()->count(), 0);
      test::expect_eq(stats.Stinfo[0][0].temp_add, 0);
    });
  }

  // 4. Test integrated doship() turn update for greenhouse
  {
    stats.Stinfo[0][0].temp_add = 0;
    shipnum_t turn_gh = TestShipBuilder(ctx.em, ShipType::OTYPE_GREEN)
                            .owned_by(1)
                            .in_planet_orbit(0, 0)
                            .with_special(TimerData{.count = 0})
                            .build();

    // Segment pass (update = false) should NOT trigger greenhouse
    ctx.em.mutate_ship(turn_gh, [&](Ship& gh) {
      doship(gh, /*update=*/false, ctx.em, stats);
      test::expect_eq(gh.as<CanisterShip>()->count(), 0);
      test::expect_eq(stats.Stinfo[0][0].temp_add, 0);
    });

    // Full update pass (update = true) DOES trigger greenhouse
    ctx.em.mutate_ship(turn_gh, [&](Ship& gh) {
      doship(gh, /*update=*/true, ctx.em, stats);
      test::expect_eq(gh.as<CanisterShip>()->count(), 1);
      test::expect_eq(stats.Stinfo[0][0].temp_add, 10);
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

  test::expect_false(stats.Stinfo[0][0].intimidated);
  ctx.em.mutate_ship(oap_id, [&](Ship& oap) {
    do_oap(oap, stats);
    test::expect_true(stats.Stinfo[0][0].intimidated);
  });

  // 2. Integration via doship() update pass
  stats.Stinfo[0][0].intimidated = false;
  ctx.em.mutate_ship(oap_id, [&](Ship& oap) {
    doship(oap, /*update=*/false, ctx.em, stats);
    test::expect_false(stats.Stinfo[0][0].intimidated);

    doship(oap, /*update=*/true, ctx.em, stats);
    test::expect_true(stats.Stinfo[0][0].intimidated);
  });

  // 3. Domain guards: landed, offline, star orbit, inactive
  {
    // Landed OAP does not intimidate
    stats.Stinfo[0][0].intimidated = false;
    shipnum_t landed_oap = TestShipBuilder(ctx.em, ShipType::STYPE_OAP)
                               .owned_by(1)
                               .landed_on(0, 0, Coordinates{0, 0})
                               .with_on(true)
                               .build();
    ctx.em.mutate_ship(landed_oap, [&](Ship& oap) {
      do_oap(oap, stats);
      test::expect_false(stats.Stinfo[0][0].intimidated);
    });

    // Offline OAP does not intimidate
    stats.Stinfo[0][0].intimidated = false;
    shipnum_t offline_oap = TestShipBuilder(ctx.em, ShipType::STYPE_OAP)
                                .owned_by(1)
                                .in_planet_orbit(0, 0)
                                .with_on(false)
                                .build();
    ctx.em.mutate_ship(offline_oap, [&](Ship& oap) {
      do_oap(oap, stats);
      test::expect_false(stats.Stinfo[0][0].intimidated);
    });

    // Star-orbiting OAP does not intimidate a planet
    stats.Stinfo[0][0].intimidated = false;
    shipnum_t star_oap = TestShipBuilder(ctx.em, ShipType::STYPE_OAP)
                             .owned_by(1)
                             .in_star_orbit(0)
                             .with_on(true)
                             .build();
    ctx.em.mutate_ship(star_oap, [&](Ship& oap) {
      do_oap(oap, stats);
      test::expect_false(stats.Stinfo[0][0].intimidated);
    });

    // Inactive OAP does not intimidate
    stats.Stinfo[0][0].intimidated = false;
    shipnum_t inactive_oap = TestShipBuilder(ctx.em, ShipType::STYPE_OAP)
                                 .owned_by(1)
                                 .in_planet_orbit(0, 0)
                                 .with_active(false)
                                 .with_on(true)
                                 .build();
    ctx.em.mutate_ship(inactive_oap, [&](Ship& oap) {
      do_oap(oap, stats);
      test::expect_false(stats.Stinfo[0][0].intimidated);
    });
  }
}

void test_do_ap_and_god() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race god_race = createTestRace(player_t{1});
  god_race.God = 1;
  Race ap_race = createTestRace(player_t{2});
  ap_race.conditions[RTEMP + 1] = 50;
  RaceRepository(store).save(god_race);
  RaceRepository(store).save(ap_race);

  Star star = createTestStar(starnum_t{1});
  StarRepository(store).save(star);

  Planet planet{PlanetType::EARTH, Coordinates{4, 4}};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.conditions(static_cast<Conditions>(RTEMP + 1)) = 10;
  PlanetRepository(store).save(planet);

  // 1. Test do_god
  ship_struct god_ship_data{
      .owner = player_t{1},
      .max_resource = 2000,
      .max_destruct = 500,
      .max_fuel = 1000,
      .type = ShipType::STYPE_HABITAT,
      .active = 1,
      .alive = 1,
  };
  auto ghandle = em.create_ship(god_ship_data);
  Ship& god_ship = *ghandle;
  do_god(god_ship, em);
  test::expect_eq(god_ship.fuel(), 1000.0);
  test::expect_eq(god_ship.destruct(), 500);
  test::expect_eq(god_ship.resource(), 2000);

  // 2. Test do_ap
  ship_struct ap_ship_data{
      .owner = player_t{2},
      .fuel = 10.0,
      .max_crew = 100,
      .popn = 100,
      .type = ShipType::OTYPE_AP,
      .active = 1,
      .alive = 1,
  };
  ap_ship_data.whatorbits = ScopeLevel::LEVEL_PLAN;
  ap_ship_data.whatdest = ScopeLevel::LEVEL_PLAN;
  ap_ship_data.deststar = starnum_t{1};
  ap_ship_data.destpnum = planetnum_t{0};
  ap_ship_data.storbits = starnum_t{1};
  ap_ship_data.pnumorbits = planetnum_t{0};
  auto aphandle = em.create_ship(ap_ship_data);
  Ship& ap_ship = *aphandle;
  ap_ship.on() = 1;
  ap_ship.docked() = 1;

  do_ap(ap_ship, em);
  test::expect_lt(ap_ship.fuel(), 10.0);
}

void test_do_pod() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  race.number_sexes = 2;
  race.likesbest = SectorType::SEC_LAND;
  RaceRepository(store).save(race);

  // 1. Test Spore pod in star system with a planet
  Star star = createTestStar(starnum_t{2});
  StarRepository(store).save(star);

  Planet planet{PlanetType::EARTH, Coordinates{2, 2}};
  planet.star_id() = 2;
  planet.planet_order() = 0;
  PlanetRepository(store).save(planet);

  SectorMap smap(planet);
  for (int y = 0; y < 2; ++y) {
    for (int x = 0; x < 2; ++x) {
      smap.get({x, y}).set_owner(0);
      smap.get({x, y}).set_type(SectorType::SEC_LAND);
    }
  }
  SectorRepository(store).save_map(smap);

  ship_struct pod_planet_data{
      .owner = player_t{1},
      .type = ShipType::STYPE_POD,
      .active = 1,
      .alive = 1,
  };
  pod_planet_data.whatorbits = ScopeLevel::LEVEL_STAR;
  pod_planet_data.storbits = starnum_t{2};
  auto pod_planet_handle = em.create_ship(pod_planet_data);
  Ship& pod_planet = *pod_planet_handle;
  auto* pod_planet_ship = pod_planet.as<SporePodShip>();
  pod_planet_ship->set_temperature(POD_THRESHOLD + 10);

  do_pod(pod_planet, em);
  test::expect_eq(pod_planet.alive(), 0);

  // 3. Test Spore pod on planet surface decay
  ship_struct pod_decay_data{
      .owner = player_t{1},
      .type = ShipType::STYPE_POD,
      .active = 1,
      .alive = 1,
  };
  pod_decay_data.whatorbits = ScopeLevel::LEVEL_PLAN;
  pod_decay_data.storbits = starnum_t{2};
  pod_decay_data.pnumorbits = planetnum_t{0};
  auto pod_decay_handle = em.create_ship(pod_decay_data);
  Ship& pod_decay = *pod_decay_handle;
  auto* pod_decay_ship = pod_decay.as<SporePodShip>();
  pod_decay_ship->set_decay(POD_DECAY + 5);

  do_pod(pod_decay, em);
  test::expect_eq(pod_decay.alive(), 0);
}

void test_do_mirror() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);
  TurnStats stats{};

  Race race = createTestRace(player_t{1});
  RaceRepository(store).save(race);

  // Set up Universe with 2 stars (star 0 and star 1)
  UniverseRepository(store).save(universe_struct{.id = 1, .numstars = 2});

  star_struct s0_data{
      .name = "StarZero",
      .pnames = {"PlanetZero"},
      .star_id = starnum_t{0},
  };
  Star star0{s0_data};
  star0.stability() = 50;
  StarRepository(store).save(star0);

  Planet p0{PlanetType::EARTH, Coordinates{2, 2}};
  p0.star_id() = 0;
  p0.planet_order() = 0;
  PlanetRepository(store).save(p0);

  // 1. Test Space Mirror aimed at Star 0 (verifying Star 0 is not ignored)
  ship_struct mirror_star_data{
      .owner = player_t{1},
      .whatorbits = ScopeLevel::LEVEL_STAR,
      .type = ShipType::STYPE_MIRROR,
      .active = 1,
      .alive = 1,
  };
  mirror_star_data.storbits = starnum_t{0};
  auto mirror_star_handle = em.create_ship(mirror_star_data);
  Ship& mirror_star = *mirror_star_handle;
  auto* mirror_star_ship = mirror_star.as<SpaceMirrorShip>();
  test::expect_true(mirror_star_ship != nullptr);
  mirror_star_ship->aim().level = ScopeLevel::LEVEL_STAR;
  mirror_star_ship->aim().snum = starnum_t{0};
  mirror_star_ship->aim().intensity = 50;

  do_mirror(mirror_star, em, stats);
  const auto& star0_updated = *em.peek_star(starnum_t{0});
  test::expect_ge(star0_updated.stability(), 50);

  // 2. Test Space Mirror not aimed (LEVEL_UNIV default)
  ship_struct mirror_unaimed_data{
      .owner = player_t{1},
      .whatorbits = ScopeLevel::LEVEL_STAR,
      .type = ShipType::STYPE_MIRROR,
      .active = 1,
      .alive = 1,
  };
  mirror_unaimed_data.storbits = starnum_t{0};
  auto mirror_unaimed_handle = em.create_ship(mirror_unaimed_data);
  Ship& mirror_unaimed = *mirror_unaimed_handle;
  auto* mirror_unaimed_ship = mirror_unaimed.as<SpaceMirrorShip>();
  test::expect_true(mirror_unaimed_ship != nullptr);
  test::expect_eq(mirror_unaimed_ship->aim().level, ScopeLevel::LEVEL_UNIV);
  do_mirror(mirror_unaimed, em, stats);

  // 3. Test Space Mirror aimed at valid Planet 0
  mirror_unaimed_ship->aim().level = ScopeLevel::LEVEL_PLAN;
  mirror_unaimed_ship->aim().pnum = planetnum_t{0};
  mirror_unaimed_ship->aim().intensity = 50;
  do_mirror(mirror_unaimed, em, stats);
  test::expect_gt(stats.Stinfo[0][0].temp_add, 0);

  // 4. Test Space Mirror aimed at another ship
  ship_struct target_data{
      .owner = player_t{1},
      .whatorbits = ScopeLevel::LEVEL_STAR,
      .type = ShipType::STYPE_SHUTTLE,
      .active = 1,
      .alive = 1,
  };
  target_data.storbits = starnum_t{0};
  target_data.damage = 0;
  auto target_handle = em.create_ship(target_data);
  Ship& target = *target_handle;

  mirror_unaimed_ship->aim().level = ScopeLevel::LEVEL_SHIP;
  mirror_unaimed_ship->aim().shipno = target.number();
  mirror_unaimed_ship->aim().intensity = 100;

  do_mirror(mirror_unaimed, em, stats);
  const auto& target_updated = *em.peek_ship(target.number());
  test::expect_ge(target_updated.damage(), 0);
}

void test_ship_domain_operations() {
  ship_struct sdata{
      .fuel = 50.0,
      .mass = 100.0,
      .max_crew = 100,
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
  ship.apply_damage(50);
  test::expect_eq(ship.damage(), 60);
  ship.apply_damage(60);
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

void test_do_repair_zero_crew() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  ServerState state{.segments = 1};
  ServerStateRepository(store).save(state);

  Race race = createTestRace(player_t{1});
  RaceRepository(store).save(race);

  // 1. Probe with max_crew = 0 (verifies division-by-zero fix)
  ship_struct probe_data{
      .owner = player_t{1},
      .max_crew = 0,
      .resource = 100,
      .damage = 50,
      .type = ShipType::OTYPE_PROBE,
      .alive = 1,
  };
  auto probe_handle = em.create_ship(probe_data);
  Ship& probe = *probe_handle;

  do_repair(probe, em);
  // Probe with 0 crew should safely do 0 repairs without crashing or division
  // by zero
  test::expect_eq(probe.damage(), 50);

  // 2. Manned ship with crew repairs damage
  ship_struct manned_data{
      .owner = player_t{1},
      .max_crew = 10,
      .build_cost = 100,
      .resource = 100,
      .popn = 10,
      .damage = 50,
      .type = ShipType::STYPE_SHUTTLE,
      .alive = 1,
  };
  auto manned_handle = em.create_ship(manned_data);
  Ship& manned = *manned_handle;

  do_repair(manned, em);
  test::expect_lt(manned.damage(), 50);
  test::expect_lt(manned.resource(), 100);
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
  ship_struct ship_data{
      .owner = player_t{1},
      .armor = 2,
      .damage = 10,
      .type = ShipType::STYPE_BATTLE,
      .alive = 1,
  };
  auto ship_handle = em.create_ship(ship_data);
  Ship& ship = *ship_handle;

  bool survived = process_ship_supernova(ship, star, state, em);
  test::expect_true(survived);
  test::expect_gt(ship.damage(), 10);
  test::expect_eq(ship.alive(), 1);

  // 2. Ship destroyed by supernova (damage >= 100)
  ship.damage() = 98;
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
  ship_struct probe_data{
      .owner = player_t{1},
      .popn = 0,
      .storbits = starnum_t{1},
      .whatorbits = ScopeLevel::LEVEL_STAR,
      .type = ShipType::OTYPE_PROBE,
      .alive = 1,
  };
  auto probe_handle = em.create_ship(probe_data);
  update_ship_inhabited_and_exploration(*probe_handle, em, stats);
  test::expect_eq(stats.StarsInhab[1], 1);
  const auto& star_after_probe = *em.peek_star(starnum_t{1});
  test::expect_true(star_after_probe.is_explored_by(player_t{1}));

  // 2. Manned ship in planet orbit explores star & planet
  ship_struct manned_data{
      .owner = player_t{1},
      .popn = 10,
      .storbits = starnum_t{1},
      .pnumorbits = planetnum_t{0},
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::STYPE_SHUTTLE,
      .alive = 1,
  };
  auto manned_handle = em.create_ship(manned_data);
  update_ship_inhabited_and_exploration(*manned_handle, em, stats);
  const auto& planet_after_manned =
      *em.peek_planet(starnum_t{1}, planetnum_t{0});
  test::expect_true(planet_after_manned.is_explored_by(player_t{1}));

  // 3. Uncrewed cargo ship does not explore
  Planet planet2{PlanetType::EARTH, Coordinates{2, 2}};
  planet2.star_id() = 1;
  planet2.planet_order() = 1;
  PlanetRepository(store).save(planet2);

  ship_struct cargo_data{
      .owner = player_t{2},
      .popn = 0,
      .storbits = starnum_t{1},
      .pnumorbits = planetnum_t{1},
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::STYPE_CARGO,
      .alive = 1,
  };
  auto cargo_handle = em.create_ship(cargo_data);
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
  ship_struct carrier_data{
      .owner = player_t{1},
      .governor = governor_t{0},
      .type = ShipType::STYPE_CARRIER,
      .alive = 1,
  };
  auto carrier_handle = em.create_ship(carrier_data);

  // Docked fighter initially owned by Player 2
  ship_struct fighter_data{
      .owner = player_t{2},
      .governor = governor_t{0},
      .destshipno = carrier_handle->number(),
      .whatorbits = ScopeLevel::LEVEL_SHIP,
      .type = ShipType::STYPE_FIGHTER,
      .alive = 1,
  };
  auto fighter_handle = em.create_ship(fighter_data);
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
  ship_struct hab_data{
      .owner = player_t{1},
      .fuel = 100.0,
      .max_crew = 0,
      .type = ShipType::STYPE_HABITAT,
      .alive = 1,
      .on = 1,
  };
  auto hab_handle = em.create_ship(hab_data);
  do_habitat(*hab_handle, em);
  test::expect_eq(hab_handle->resource(), 0);

  // 2. Weapon plant with 0 max_crew does not divide by zero
  ship_struct wplant_data{
      .owner = player_t{1},
      .fuel = 100.0,
      .max_crew = 0,
      .resource = 100,
      .type = ShipType::OTYPE_WPLANT,
      .alive = 1,
      .on = 1,
  };
  auto wplant_handle = em.create_ship(wplant_data);
  int produced = do_weapon_plant(*wplant_handle, em);
  test::expect_eq(produced, 0);

  // 3. Canister clamped at -100
  ship_struct can_data{
      .owner = player_t{1},
      .storbits = starnum_t{1},
      .pnumorbits = planetnum_t{0},
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::OTYPE_CANIST,
      .active = true,
      .alive = true,
  };
  auto can_handle = em.create_ship(can_data);
  stats.Stinfo[1][0].temp_add = -95;
  do_canister(*can_handle, em, stats);
  test::expect_eq(stats.Stinfo[1][0].temp_add, -100);

  // 4. Greenhouse clamped at +100
  ship_struct gh_data{
      .owner = player_t{1},
      .storbits = starnum_t{1},
      .pnumorbits = planetnum_t{0},
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::OTYPE_GREEN,
      .active = true,
      .alive = true,
  };
  auto gh_handle = em.create_ship(gh_data);
  stats.Stinfo[1][0].temp_add = 95;
  do_greenhouse(*gh_handle, em, stats);
  test::expect_eq(stats.Stinfo[1][0].temp_add, 100);

  // 5. Space mirror destroys target ship when damage exceeds 100
  ship_struct target_data{
      .owner = player_t{1},
      .size = 10,
      .storbits = starnum_t{1},
      .whatorbits = ScopeLevel::LEVEL_STAR,
      .damage = 99,
      .type = ShipType::STYPE_SHUTTLE,
      .alive = 1,
  };
  auto target_handle = em.create_ship(target_data);

  ship_struct mirror_data{
      .owner = player_t{1},
      .storbits = starnum_t{1},
      .whatorbits = ScopeLevel::LEVEL_STAR,
      .type = ShipType::STYPE_MIRROR,
      .alive = 1,
  };
  auto mirror_handle = em.create_ship(mirror_data);
  auto* mirror_ship = mirror_handle->as<SpaceMirrorShip>();
  mirror_ship->aim().level = ScopeLevel::LEVEL_SHIP;
  mirror_ship->aim().shipno = target_handle->number();
  mirror_ship->aim().intensity = 100;

  do_mirror(*mirror_handle, em, stats);
  const auto* target_after = em.peek_ship(target_handle->number());
  // Destroyed ship is killed via em.kill_ship
  test::expect_true(target_after == nullptr || target_after->alive() == 0);
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

  std::println(std::cout, "  Testing do_repair on zero-crew probe... ");
  test_do_repair_zero_crew();
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

  std::println(std::cout, "All doship tests passed!");
  return 0;
}
