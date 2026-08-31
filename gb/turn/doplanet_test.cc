// SPDX-License-Identifier: Apache-2.0

/// \file doplanet_test.cc
/// \brief Unit tests for doplanet turn loop, ground ship movement,
/// terraforming, resource recovery, and exploration island discovery.

import dallib;
import gb.entities;
import gb.services;
import gb.turn;
import test;
import std;

namespace {

Race createTestRace(player_t playernum = player_t{1}) {
  Race race{};
  race.Playernum = playernum;
  race.metabolism = 1.0;
  race.birthrate = 0.1;
  race.number_sexes = 2;
  race.fertilize = 10;
  race.adventurism = 0.5;
  race.likesbest = SectorType::SEC_LAND;
  for (int i = 0; i <= SectorType::SEC_WASTED; i++) {
    race.likes[i] = 0.8;
  }
  race.likes[SectorType::SEC_PLATED] = 1.0;
  return race;
}

Planet createTestPlanet() {
  Planet planet(PlanetType::EARTH, Coordinates{10, 10});
  planet.slaved_to() = 0;
  planet.conditions(TOXIC) = 0;
  planet.conditions(RTEMP) = 50;
  planet.conditions(TEMP) = 50;
  for (int i = 1; i <= MAXPLAYERS; i++) {
    planet.info(player_t{i}).tax = 10;
    planet.info(player_t{i}).mob_set = 0;
    planet.info(player_t{i}).resource = 0;
    planet.info(player_t{i}).autorep = 0;
  }
  return planet;
}

Star createTestStar() {
  star_struct star_data{};
  star_data.name = "TestStar";
  star_data.star_id = 0;
  star_data.stability = 50;
  star_data.nova_stage = 0;
  star_data.temperature = 100;
  star_data.pnames.push_back("TestPlanet");
  return Star(star_data);
}

void test_moveship_onplanet() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  RaceRepository races(store);
  races.save(race);

  Star star = createTestStar();
  StarRepository stars(store);
  stars.save(star);

  Planet planet = createTestPlanet();

  ship_struct sdata{
      .owner = player_t{1},
      .land_coords = {5, 5},
      .special = TerraformData{.index = 0},
      .storbits = star.star_id(),
      .pnumorbits = 0,
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::OTYPE_TERRA,
      .active = 1,
      .alive = 1,
      .docked = 1,
  };

  auto ship_handle = em.create_ship(sdata);
  Ship& ship = *ship_handle;
  ship.shipclass() = "2222";

  // Test get_ground_order
  auto order0 = get_ground_order(ship, 0);
  test::expect_true(order0.has_value());
  test::expect_eq(*order0, '2');

  auto order_oob = get_ground_order(ship, 10);
  test::expect_false(order_oob.has_value());
  test::expect_eq(order_oob.error(), GroundMovementError::InvalidIndex);

  // Move once: y should increase from 5 to 6
  auto move1 = advance_ground_vehicle(ship, planet, em);
  test::expect_true(move1.has_value());
  test::expect_eq(move1->y, 6);
  test::expect_eq(ship.land_coords().y, 6);
  test::expect_eq(ship.shipclass()[0], '2');

  // Test non-terraform ship error
  ship_struct non_terra_data{
      .owner = player_t{1},
      .land_coords = {5, 5},
      .special = WasteData{},
      .type = ShipType::OTYPE_CANIST,
      .active = 1,
      .alive = 1,
      .docked = 1,
  };
  auto non_terra_handle = em.create_ship(non_terra_data);
  Ship& non_terra = *non_terra_handle;
  auto non_terra_order = get_ground_order(non_terra, 0);
  test::expect_false(non_terra_order.has_value());
  test::expect_eq(non_terra_order.error(),
                  GroundMovementError::NotTerraformVehicle);
  auto non_terra_move = advance_ground_vehicle(non_terra, planet, em);
  test::expect_false(non_terra_move.has_value());
  test::expect_eq(non_terra_move.error(),
                  GroundMovementError::NotTerraformVehicle);
  test::expect_eq(non_terra.on(), 0);

  // Test stopped ground ship ('s')
  ship_struct stopped_data{
      .owner = player_t{1},
      .land_coords = {5, 5},
      .special = TerraformData{.index = 0},
      .type = ShipType::OTYPE_TERRA,
      .active = 1,
      .alive = 1,
      .docked = 1,
  };
  auto stopped_handle = em.create_ship(stopped_data);
  Ship& stopped_ship = *stopped_handle;
  stopped_ship.shipclass() = "s";
  stopped_ship.on() = 1;
  auto stopped_order = get_ground_order(stopped_ship, 0);
  test::expect_false(stopped_order.has_value());
  test::expect_eq(stopped_order.error(), GroundMovementError::Stopped);
  auto stopped_move = advance_ground_vehicle(stopped_ship, planet, em);
  test::expect_false(stopped_move.has_value());
  test::expect_eq(stopped_move.error(), GroundMovementError::Stopped);
  test::expect_eq(stopped_ship.on(), 0);

  // Test polar bouncing at south pole (y >= Maxy -> bounce y -= 2, flip order)
  ship_struct bounce_data{
      .owner = player_t{1},
      .land_coords = {5, 9},
      .special = TerraformData{.index = 0},
      .storbits = star.star_id(),
      .pnumorbits = 0,
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::OTYPE_TERRA,
      .active = 1,
      .alive = 1,
      .docked = 1,
  };
  auto bounce_handle = em.create_ship(bounce_data);
  Ship& bounce_ship = *bounce_handle;
  bounce_ship.shipclass() = "2";  // Single move south from y=9 on 10-high
                                  // planet -> y=10 >= 10 -> bounce to 8
  auto bounce_move = advance_ground_vehicle(bounce_ship, planet, em);
  test::expect_true(bounce_move.has_value());
  test::expect_eq(bounce_move->y, 8);
  test::expect_eq(bounce_ship.shipclass()[0], '8');  // '2' flipped to '8'

  // Test out-of-orders notification on multi-step orders
  ship_struct ooo_data{
      .owner = player_t{1},
      .land_coords = {5, 5},
      .special = TerraformData{.index = 0},
      .storbits = star.star_id(),
      .pnumorbits = 0,
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::OTYPE_TERRA,
      .active = 1,
      .alive = 1,
      .docked = 1,
  };
  auto ooo_handle = em.create_ship(ooo_data);
  Ship& ooo_ship = *ooo_handle;
  ooo_ship.shipclass() = "88";
  test::expect_eq(ooo_ship.notified(), 0);
  auto ooo_move = advance_ground_vehicle(ooo_ship, planet, em);
  test::expect_true(ooo_move.has_value());
  test::expect_eq(ooo_ship.notified(), 1);
}

void test_execute_terraforming() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  race.likesbest = SectorType::SEC_LAND;
  RaceRepository races(store);
  races.save(race);

  Star star = createTestStar();
  StarRepository stars(store);
  stars.save(star);

  Planet planet = createTestPlanet();
  SectorMap smap(planet);

  ship_struct sdata{
      .owner = player_t{1},
      .fuel = 100.0,
      .land_coords = {2, 2},
      .max_crew = 100,
      .popn = 100,
      .special = TerraformData{.index = 0},
      .storbits = star.star_id(),
      .pnumorbits = 0,
      .whatdest = ScopeLevel::LEVEL_PLAN,
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::OTYPE_TERRA,
      .active = 1,
      .alive = 1,
      .docked = 1,
      .on = 1,
  };

  auto ship_handle = em.create_ship(sdata);
  Ship& ship = *ship_handle;
  ship.shipclass() = "2222";  // Moves south: (2, 2) -> (2, 3)

  // 1. Target sector already optimal (likesbest)
  ship.set_land_coords({2, 2});
  ship.as<TerraformerShip>()->set_index(0);
  smap.get(Coordinates{2, 3}).set_condition(SectorType::SEC_LAND);
  auto res_optimal = execute_terraforming(ship, planet, smap, em);
  test::expect_false(res_optimal.has_value());
  test::expect_eq(res_optimal.error(), GroundActionError::SectorAlreadyOptimal);

  // 2. Target sector is gas (incompatible)
  ship.set_land_coords({2, 2});
  ship.as<TerraformerShip>()->set_index(0);
  smap.get(Coordinates{2, 3}).set_condition(SectorType::SEC_GAS);
  auto res_gas = execute_terraforming(ship, planet, smap, em);
  test::expect_false(res_gas.has_value());
  test::expect_eq(res_gas.error(), GroundActionError::IncompatibleSector);

  // 3. Successful terraforming
  ship.set_land_coords({2, 2});
  ship.as<TerraformerShip>()->set_index(0);
  smap.get(Coordinates{2, 3}).set_condition(SectorType::SEC_DESERT);
  double initial_fuel = ship.fuel();
  auto res_success = execute_terraforming(ship, planet, smap, em);
  test::expect_true(res_success.has_value());
  test::expect_true(*res_success);
  test::expect_eq(smap.get(Coordinates{2, 3}).get_condition(),
                  SectorType::SEC_LAND);
  test::expect_eq(ship.fuel(), initial_fuel - FUEL_COST_TERRA);

  // 4. Insufficient fuel
  ship.fuel() = 0.0;
  ship.notified() = 0;
  auto res_fuel = execute_terraforming(ship, planet, smap, em);
  test::expect_false(res_fuel.has_value());
  test::expect_eq(res_fuel.error(), GroundActionError::InsufficientFuel);
  test::expect_eq(ship.notified(), 1);

  // 5. Not switched on
  ship.on() = 0;
  auto res_off = execute_terraforming(ship, planet, smap, em);
  test::expect_false(res_off.has_value());
  test::expect_eq(res_off.error(), GroundActionError::NotSwitchedOn);
}

void test_execute_plowing() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  race.likes[SectorType::SEC_LAND] = 1;
  race.likes[SectorType::SEC_WASTED] = 0;
  RaceRepository races(store);
  races.save(race);

  Star star = createTestStar();
  StarRepository stars(store);
  stars.save(star);

  Planet planet = createTestPlanet();
  SectorMap smap(planet);

  ship_struct sdata{
      .owner = player_t{1},
      .fuel = 50.0,
      .land_coords = {1, 1},
      .max_crew = 100,
      .popn = 100,
      .special = TerraformData{.index = 0},
      .storbits = star.star_id(),
      .pnumorbits = 0,
      .whatdest = ScopeLevel::LEVEL_PLAN,
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::OTYPE_PLOW,
      .active = 1,
      .alive = 1,
      .docked = 1,
      .on = 1,
  };

  auto ship_handle = em.create_ship(sdata);
  Ship& ship = *ship_handle;
  ship.shipclass() = "2222";  // Moves south: (1, 1) -> (1, 2)

  // 1. Target sector condition not liked
  ship.set_land_coords({1, 1});
  ship.as<TerraformerShip>()->set_index(0);
  smap.get(Coordinates{1, 2}).set_condition(SectorType::SEC_WASTED);
  smap.get(Coordinates{1, 2}).set_fert(50);
  auto res_incompat = execute_plowing(ship, planet, smap, em);
  test::expect_false(res_incompat.has_value());
  test::expect_eq(res_incompat.error(), GroundActionError::IncompatibleSector);

  // 2. Target sector already at 100% fertility
  ship.set_land_coords({1, 1});
  ship.as<TerraformerShip>()->set_index(0);
  smap.get(Coordinates{1, 2}).set_condition(SectorType::SEC_LAND);
  smap.get(Coordinates{1, 2}).set_fert(100);
  auto res_optimal = execute_plowing(ship, planet, smap, em);
  test::expect_false(res_optimal.has_value());
  test::expect_eq(res_optimal.error(), GroundActionError::SectorAlreadyOptimal);

  // 3. Successful plowing
  ship.set_land_coords({1, 1});
  ship.as<TerraformerShip>()->set_index(0);
  smap.get(Coordinates{1, 2}).set_fert(60);
  double initial_fuel = ship.fuel();
  auto res_success = execute_plowing(ship, planet, smap, em);
  test::expect_true(res_success.has_value());
  test::expect_gt(*res_success, 0);
  test::expect_gt(smap.get(Coordinates{1, 2}).get_fert(), 60U);
  test::expect_eq(ship.fuel(), initial_fuel - FUEL_COST_PLOW);

  // 4. Insufficient fuel
  ship.fuel() = 0.0;
  ship.notified() = 0;
  auto res_fuel = execute_plowing(ship, planet, smap, em);
  test::expect_false(res_fuel.has_value());
  test::expect_eq(res_fuel.error(), GroundActionError::InsufficientFuel);
  test::expect_eq(ship.notified(), 1);

  // 5. Not switched on
  ship.on() = 0;
  auto res_off = execute_plowing(ship, planet, smap, em);
  test::expect_false(res_off.has_value());
  test::expect_eq(res_off.error(), GroundActionError::NotSwitchedOn);
}

void test_upgrade_sector_dome() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  RaceRepository races(store);
  races.save(race);

  Planet planet = createTestPlanet();
  SectorMap smap(planet);

  ship_struct sdata{
      .owner = player_t{1},
      .land_coords = {2, 2},
      .max_crew = 100,
      .resource = 50,
      .popn = 100,
      .whatdest = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::OTYPE_DOME,
      .active = 1,
      .alive = 1,
      .docked = 1,
      .on = 1,
  };

  auto ship_handle = em.create_ship(sdata);
  Ship& ship = *ship_handle;

  // 1. Sector already optimal (100% efficiency)
  smap.get(Coordinates{2, 2}).set_efficiency_bounded(100);
  auto res_optimal = upgrade_sector_dome(em, ship, smap);
  test::expect_false(res_optimal.has_value());
  test::expect_eq(res_optimal.error(), GroundActionError::SectorAlreadyOptimal);

  // 2. Successful dome upgrade
  smap.get(Coordinates{2, 2}).set_efficiency_bounded(40);
  auto res_success = upgrade_sector_dome(em, ship, smap);
  test::expect_true(res_success.has_value());
  test::expect_gt(*res_success, 0);
  test::expect_gt(smap.get(Coordinates{2, 2}).get_eff(), 40);
  test::expect_eq(ship.resource(), 50 - RES_COST_DOME);

  // 3. Insufficient resources
  ship.resource() = 0;
  auto res_res = upgrade_sector_dome(em, ship, smap);
  test::expect_false(res_res.has_value());
  test::expect_eq(res_res.error(), GroundActionError::InsufficientResources);

  // 4. Not switched on
  ship.on() = 0;
  auto res_off = upgrade_sector_dome(em, ship, smap);
  test::expect_false(res_off.has_value());
  test::expect_eq(res_off.error(), GroundActionError::NotSwitchedOn);
}

void test_strip_mine_quarry() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  race.metabolism = 1.0;
  RaceRepository races(store);
  races.save(race);

  Planet planet = createTestPlanet();
  SectorMap smap(planet);

  ship_struct sdata{
      .owner = player_t{1},
      .fuel = 50.0,
      .land_coords = {3, 3},
      .max_crew = 100,
      .popn = 100,
      .whatdest = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::OTYPE_QUARRY,
      .active = 1,
      .alive = 1,
      .docked = 1,
      .on = 1,
  };

  auto ship_handle = em.create_ship(sdata);
  Ship& ship = *ship_handle;

  smap.get(Coordinates{3, 3}).set_condition(SectorType::SEC_LAND);
  smap.get(Coordinates{3, 3}).set_fert(50);

  TurnStats stats;

  // 1. Successful quarry mining
  auto res_success = strip_mine_quarry(ship, planet, smap, em, stats);
  test::expect_true(res_success.has_value());
  test::expect_gt(*res_success, 0);
  test::expect_eq(smap.get(Coordinates{3, 3}).get_condition(),
                  SectorType::SEC_WASTED);
  test::expect_gt(stats.prod_res[player_t{1}], 0);
  test::expect_eq(ship.fuel(), 50.0 - FUEL_COST_QUARRY);

  // 2. Insufficient fuel
  ship.fuel() = 0.0;
  ship.notified() = 0;
  auto res_fuel = strip_mine_quarry(ship, planet, smap, em, stats);
  test::expect_false(res_fuel.has_value());
  test::expect_eq(res_fuel.error(), GroundActionError::InsufficientFuel);
  test::expect_eq(ship.notified(), 1);
  test::expect_eq(ship.on(), 0);

  // 3. Not switched on
  ship.on() = 0;
  auto res_off = strip_mine_quarry(ship, planet, smap, em, stats);
  test::expect_false(res_off.has_value());
  test::expect_eq(res_off.error(), GroundActionError::NotSwitchedOn);
}

void test_execute_berserker_bombardment() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  // Attacker (Race 1, at war with Race 2)
  Race race1 = createTestRace(player_t{1});
  race1.declare_war_on(player_t{2});
  Race race2 = createTestRace(player_t{2});
  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  // Initialize Universe with VN hitlist for Player 2
  universe_struct udata{};
  udata.id = 1;
  udata.VN_hitlist[player_t{2}] = 5;  // Player 2 hitlist entry = 5
  UniverseRepository univ_repo(store);
  univ_repo.save(udata);

  // Star system with 2 planets
  star_struct ss{};
  ss.star_id = 0;
  ss.pnames.emplace_back("Planet0");
  ss.pnames.emplace_back("Planet1");
  StarRepository star_repo(store);
  star_repo.save(ss);

  // Planet 0
  Planet planet = createTestPlanet();
  planet.star_id() = 0;
  planet.planet_order() = 0;
  PlanetRepository planet_repo(store);
  planet_repo.save(planet);

  // SectorMap on Planet 0 with enemy population
  {
    SectorMap smap(planet);
    smap.get(Coordinates{5, 5}).set_condition(SectorType::SEC_LAND);
    smap.get(Coordinates{5, 5}).set_popn_exact(100);
    smap.get(Coordinates{5, 5}).set_owner(2);
    SectorRepository smap_repo(store);
    smap_repo.save_map(smap);
  }

  // Berserker ship in orbit
  ship_struct b_ship{
      .owner = player_t{1},
      .destruct = 100,
      .special = MindData{.progenitor = player_t{1}, .who_killed = player_t{2}},
      .storbits = 0,
      .deststar = 0,
      .destpnum = 0,
      .pnumorbits = 0,
      .whatdest = ScopeLevel::LEVEL_PLAN,
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::OTYPE_BERS,
      .active = true,
      .alive = true,
      .bombard = true,
      .docked = false,
      .on = true,
      .guns = ActiveBattery::PRIMARY,
      .primtype = GTYPE_HEAVY,
  };

  auto ship_handle = em.create_ship(b_ship);
  Ship& ship = *ship_handle;

  // 1. Landed ship fails preconditions
  ship.docked() = true;
  test::expect_false(execute_berserker_bombardment(em, ship, planet));
  ship.docked() = false;

  // 2. Successful bombardment decrements VN_hitlist
  test::expect_true(execute_berserker_bombardment(em, ship, planet));
  const auto* universe_after = em.peek_universe();
  test::expect_eq(universe_after->VN_hitlist[player_t{2}], 4);

  // 3. No remaining targets on planet causes ship to pick a new destination
  // Clear remaining defenders
  em.mutate_sectormap(planet.star_id(), planet.planet_order(),
                      [](SectorMap& smap) {
                        smap.get(Coordinates{5, 5}).set_popn_exact(0);
                        smap.get(Coordinates{5, 5}).set_owner(0);
                      });
  test::expect_false(execute_berserker_bombardment(em, ship, planet));
}

void test_refuel_gasgiant_orbiters() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  Planet gas_giant(PlanetType::GASGIANT, Coordinates{0, 0});
  Planet earth(PlanetType::EARTH, Coordinates{0, 0});

  ship_struct sdata{
      .owner = player_t{1},
      .fuel = 50.0,
      .max_fuel = 500,
      .whatdest = ScopeLevel::LEVEL_PLAN,
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::STYPE_TANKER,
      .active = 1,
      .alive = 1,
      .docked = 0,
      .on = 1,
  };

  auto ship_handle = em.create_ship(sdata);
  Ship& ship = *ship_handle;

  // 1. Not a gas giant: 0 fuel added
  test::expect_eq(refuel_gasgiant_orbiters(earth, ship), 0.0);
  test::expect_eq(ship.fuel(), 50.0);

  // 2. Landed ship on gas giant: 0 fuel added
  ship.docked() = 1;
  test::expect_eq(refuel_gasgiant_orbiters(gas_giant, ship), 0.0);
  test::expect_eq(ship.fuel(), 50.0);
  ship.docked() = 0;

  // 3. Tanker in orbit around gas giant: FUEL_GAS_ADD_TANKER added
  double added_tanker = refuel_gasgiant_orbiters(gas_giant, ship);
  test::expect_eq(added_tanker, FUEL_GAS_ADD_TANKER);
  test::expect_eq(ship.fuel(), 50.0 + FUEL_GAS_ADD_TANKER);

  // 4. Habitat in orbit around gas giant: FUEL_GAS_ADD_HABITAT added
  ship.type() = ShipType::STYPE_HABITAT;
  ship.fuel() = 50.0;
  double added_hab = refuel_gasgiant_orbiters(gas_giant, ship);
  test::expect_eq(added_hab, FUEL_GAS_ADD_HABITAT);
  test::expect_eq(ship.fuel(), 50.0 + FUEL_GAS_ADD_HABITAT);

  // 5. Standard ship in orbit around gas giant: FUEL_GAS_ADD added
  ship.type() = ShipType::STYPE_POD;
  ship.fuel() = 50.0;
  double added_pod = refuel_gasgiant_orbiters(gas_giant, ship);
  test::expect_eq(added_pod, FUEL_GAS_ADD);
  test::expect_eq(ship.fuel(), 50.0 + FUEL_GAS_ADD);

  // 6. Capacity clamping near max_fuel
  ship.fuel() = 495.0;
  double added_clamp = refuel_gasgiant_orbiters(gas_giant, ship);
  test::expect_eq(added_clamp, 5.0);
  test::expect_eq(ship.fuel(), 500.0);
}

void test_process_planetary_ships() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  race.likes[SectorType::SEC_LAND] = 1;
  race.likes[SectorType::SEC_WASTED] = 0;
  RaceRepository race_repo(store);
  race_repo.save(race);

  Star star = createTestStar();
  StarRepository star_repo(store);
  star_repo.save(star);

  Planet planet = createTestPlanet();
  planet.type() = PlanetType::GASGIANT;
  planet.star_id() = star.star_id();
  planet.planet_order() = 0;

  SectorMap smap(planet);
  smap.get(Coordinates{1, 1}).set_condition(SectorType::SEC_WASTED);
  smap.get(Coordinates{1, 2}).set_condition(SectorType::SEC_LAND);
  smap.get(Coordinates{1, 2}).set_fert(50);

  TurnStats stats{};

  // 1. Dead plow ship (should be skipped)
  ship_struct dead_plow{
      .owner = player_t{1},
      .land_coords = {0, 0},
      .type = ShipType::OTYPE_PLOW,
      .active = 1,
      .alive = 0,
      .docked = 1,
      .on = 1,
  };
  em.create_ship(dead_plow);

  // 2. Active Landed Plow on (1, 1) moving South to (1, 2)
  ship_struct active_plow{
      .owner = player_t{1},
      .shipclass = "2222",
      .fuel = 50.0,
      .land_coords = {1, 1},
      .max_crew = 100,
      .max_fuel = 100,
      .popn = 100,
      .special = TerraformData{.index = 0},
      .storbits = star.star_id(),
      .pnumorbits = 0,
      .whatdest = ScopeLevel::LEVEL_PLAN,
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::OTYPE_PLOW,
      .active = 1,
      .alive = 1,
      .docked = 1,
      .on = 1,
  };
  auto plow_handle = em.create_ship(active_plow);

  // 3. Orbiting Tanker (should receive gas giant fuel)
  ship_struct tanker{
      .owner = player_t{1},
      .fuel = 50.0,
      .nextship = plow_handle->number(),
      .max_fuel = 500,
      .storbits = star.star_id(),
      .pnumorbits = 0,
      .whatdest = ScopeLevel::LEVEL_PLAN,
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::STYPE_TANKER,
      .active = 1,
      .alive = 1,
      .docked = 0,
      .on = 1,
  };
  auto tanker_handle = em.create_ship(tanker);

  // Link ships head to planet
  planet.ships() = tanker_handle->number();

  process_planetary_ships(em, planet, smap, stats);

  // Verify tanker refueled
  const auto* t_after = em.peek_ship(tanker_handle->number());
  test::expect_eq(t_after->fuel(), 50.0 + FUEL_GAS_ADD_TANKER);

  // Verify plow moved and plowed (1, 2)
  const auto* p_after = em.peek_ship(plow_handle->number());
  test::expect_eq(p_after->land_coords().x, 1);
  test::expect_eq(p_after->land_coords().y, 2);
  test::expect_gt(smap.get(Coordinates{1, 2}).get_fert(), 50U);
}

void test_do_recover() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race r1 = createTestRace(player_t{1});
  Race r2 = createTestRace(player_t{2});
  Race r3 = createTestRace(player_t{3});
  r1.declare_alliance_with(player_t{2});
  r2.declare_alliance_with(player_t{1});

  RaceRepository races(store);
  races.save(r1);
  races.save(r2);
  races.save(r3);

  Star star = createTestStar();
  Planet planet = createTestPlanet();

  planet.info(player_t{3}).resource = 100;
  planet.info(player_t{3}).destruct = 50;

  planet.info(player_t{1}).numsectsowned = 5;
  planet.info(player_t{2}).numsectsowned = 5;

  do_recover(em, star, planet);

  test::expect_eq(planet.info(player_t{1}).resource +
                      planet.info(player_t{2}).resource,
                  100);
  test::expect_eq(planet.info(player_t{1}).destruct +
                      planet.info(player_t{2}).destruct,
                  50);
  test::expect_eq(planet.info(player_t{3}).resource, 0);
  test::expect_eq(planet.info(player_t{3}).destruct, 0);
}

void test_doplanet_full_cycle() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  RaceRepository races(store);
  races.save(race);

  Star star = createTestStar();
  StarRepository stars(store);
  stars.save(star);

  Planet planet = createTestPlanet();
  planet.star_id() = star.star_id();
  planet.planet_order() = 0;
  PlanetRepository planets(store);
  planets.save(planet);

  SectorMap initial_smap(planet);
  for (int y = 0; y < 10; y++) {
    for (int x = 0; x < 10; x++) {
      auto& s = initial_smap.get(Coordinates{x, y});
      s.set_x(x);
      s.set_y(y);
      s.set_owner(1);
      s.set_popn_exact(100);
      s.set_efficiency_bounded(50);
      s.set_fert(50);
      s.set_resource(10);
      s.set_condition(SectorType::SEC_LAND);
    }
  }
  SectorRepository sectors(store);
  sectors.save_map(initial_smap);

  TurnStats stats{};
  stats.Compat[player_t{1}] = 1.0;

  doplanet(em, star, planet, stats);

  test::expect_gt(planet.popn(), 0);
  test::expect_eq(planet.info(player_t{1}).numsectsowned, 100);
}

void test_exploration_island_discovery() {
  seed_rand(123);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  race.likesbest = SectorType::SEC_LAND;
  RaceRepository races(store);
  races.save(race);

  Star star = createTestStar();
  StarRepository stars(store);
  stars.save(star);

  Planet planet = createTestPlanet();
  planet.star_id() = star.star_id();
  planet.planet_order() = 0;
  planet.expltimer() = 0;  // Trigger exploration check this cycle
  PlanetRepository planets(store);
  planets.save(planet);

  SectorMap initial_smap(planet);
  for (int y = 0; y < 10; y++) {
    for (int x = 0; x < 10; x++) {
      auto& s = initial_smap.get(Coordinates{x, y});
      s.set_x(x);
      s.set_y(y);
      s.set_owner(0);
      s.clear_popn();
      s.set_condition(SectorType::SEC_SEA);
    }
  }

  // Player 1 owns one sector at (0, 0)
  auto& s_owned = initial_smap.get(Coordinates{0, 0});
  s_owned.set_owner(1);
  s_owned.set_popn_exact(100);
  s_owned.set_condition(SectorType::SEC_LAND);

  // Set up multiple vacant eligible candidate island sectors that have been
  // explored by player 1
  for (int x = 1; x <= 4; ++x) {
    auto& cand = initial_smap.get(Coordinates{x, 0});
    cand.set_condition(SectorType::SEC_LAND);
  }

  SectorRepository sectors(store);
  sectors.save_map(initial_smap);

  TurnStats stats{};
  stats.Compat[player_t{1}] = 1.0;

  doplanet(em, star, planet, stats);

  // Verify that an island was claimed (stats.Claims == true)
  test::expect_true(stats.Claims);
  test::expect_eq(stats.tot_captured, 1);

  // Verify that EXACTLY 1 additional sector was claimed (1 owned + 1 new island
  // = 2 total), proving that iteration stopped after claiming the first island
  test::expect_eq(planet.info(player_t{1}).numsectsowned, 2);
}

void test_64bit_production_and_stockpiles() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  race.metabolism = 500.0;
  RaceRepository races(store);
  races.save(race);

  Star star = createTestStar();
  StarRepository stars(store);
  stars.save(star);

  Planet planet = createTestPlanet();
  planet.star_id() = star.star_id();
  planet.planet_order() = 0;
  planet.info(player_t{1}).numsectsowned = 5;
  planet.info(player_t{1}).resource = 100'000;
  planet.info(player_t{1}).fuel = 200'000;
  planet.info(player_t{1}).destruct = 300'000;
  planet.info(player_t{1}).crystals = 400'000;

  PlanetRepository planets(store);
  planets.save(planet);

  SectorMap smap(planet);
  for (int y = 0; y < 10; ++y) {
    for (int x = 0; x < 10; ++x) {
      auto& sect = smap.get(Coordinates{x, y});
      sect.set_x(x);
      sect.set_y(y);
      if (y == 0 && x < 5) {
        sect.set_owner(1);
        sect.set_popn_exact(100);
        sect.set_efficiency_bounded(100);
        sect.set_resource(100'000);
        sect.set_condition(SectorType::SEC_LAND);
      } else {
        sect.set_owner(0);
        sect.clear_popn();
        sect.set_condition(SectorType::SEC_SEA);
      }
    }
  }
  SectorRepository sectors(store);
  sectors.save_map(smap);

  TurnStats stats{};
  stats.Compat[player_t{1}] = 1.0;

  doplanet(em, star, planet, stats);

  // Verify production recorded as 64-bit resource_t (> 65,535) without 16-bit
  // overflow
  test::expect_gt(planet.info(player_t{1}).prod_res, 65'535);
  test::expect_gt(planet.info(player_t{1}).prod_fuel, 65'535);

  // Verify stockpiles accumulated as 64-bit resource_t without overflow
  test::expect_gt(planet.info(player_t{1}).resource, 165'535);
  test::expect_gt(planet.info(player_t{1}).fuel, 265'535);
  test::expect_eq(planet.info(player_t{1}).destruct, 300'000);
  test::expect_eq(planet.info(player_t{1}).crystals, 400'000);
}

void test_turnstats_playervector_accumulation() {
  TurnStats stats{};

  // Verify initial zero-initialization across PlayerVector members
  test::expect_eq(stats.prod_res[player_t{1}], 0);
  test::expect_eq(stats.prod_fuel[player_t{1}], 0);
  test::expect_eq(stats.prod_destruct[player_t{1}], 0);
  test::expect_eq(stats.prod_crystals[player_t{1}], 0);
  test::expect_eq(stats.Power[player_t{1}].popn, 0U);
  test::expect_eq(stats.starpopns[0][player_t{1}], 0UL);
  test::expect_eq(stats.starnumships[0][player_t{1}], 0U);
  test::expect_eq(stats.Sdatanumships[player_t{1}], 0U);
  test::expect_eq(stats.Sdatapopns[player_t{1}], 0UL);
  test::expect_eq(stats.avg_mob[player_t{1}], 0UL);

  // Mutate player stats using strongly-typed player_t keys
  stats.prod_res[player_t{1}] += 5000;
  stats.prod_fuel[player_t{1}] += 2500;
  stats.Power[player_t{1}].popn = 10000;
  stats.starpopns[1][player_t{1}] = 8000;
  stats.starnumships[1][player_t{1}] = 12;

  test::expect_eq(stats.prod_res[player_t{1}], 5000);
  test::expect_eq(stats.prod_fuel[player_t{1}], 2500);
  test::expect_eq(stats.Power[player_t{1}].popn, 10000U);
  test::expect_eq(stats.starpopns[1][player_t{1}], 8000UL);
  test::expect_eq(stats.starnumships[1][player_t{1}], 12U);

  // Verify bounds safety on 0 and > MAXPLAYERS
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.prod_res[player_t{0}]; });
  test::expect_throws<std::out_of_range>(
      [&]() { (void)stats.Power[player_t{MAXPLAYERS + 1}]; });
}

void test_process_planet_climate() {
  star_struct ss{};
  ss.star_id = 3;
  Star star(ss);

  Planet planet(PlanetType::EARTH, Coordinates{10, 10});
  planet.star_id() = 3;
  planet.planet_order() = 1;
  planet.conditions(TEMP) = 20;
  planet.conditions(RTEMP) = 20;

  TurnStats stats{};
  stats.Stinfo[3][1].temp_add = 10;

  process_planet_climate(planet, star, stats);

  test::expect_ge(planet.conditions(TEMP), 25);
  test::expect_le(planet.conditions(TEMP), 35);
}

void test_process_toxic_environmental_damage() {
  Planet planet(PlanetType::EARTH, Coordinates{10, 10});
  planet.star_id() = 1;
  planet.planet_order() = 0;

  SectorMap smap(planet);
  for (int y = 0; y < planet.dimensions().y; ++y) {
    for (int x = 0; x < planet.dimensions().x; ++x) {
      auto& sect = smap.get(Coordinates{x, y});
      sect.set_coords(Coordinates{x, y});
      sect.set_condition(SectorType::SEC_LAND);
      sect.set_type(SectorType::SEC_LAND);
      sect.set_fert(80);
      sect.set_resource(50);
    }
  }

  // 1. Below or at toxic threshold (ENVIR_DAMAGE_TOX = 70) -> no damage
  planet.conditions(TOXIC) = ENVIR_DAMAGE_TOX;
  auto safe_res = process_toxic_environmental_damage(planet, smap);
  test::expect_false(safe_res.has_value());

  // 2. Above toxic threshold -> sector devastated
  planet.conditions(TOXIC) = ENVIR_DAMAGE_TOX + 1;
  auto damage_res = process_toxic_environmental_damage(planet, smap);
  test::expect_true(damage_res.has_value());
  test::expect_true(smap.in_bounds(*damage_res));

  const auto& devastated_sector = smap.get(*damage_res);
  test::expect_eq(devastated_sector.get_condition(), SectorType::SEC_WASTED);
}

void test_process_supernova_sector_devastation() {
  Planet planet(PlanetType::EARTH, Coordinates{10, 10});
  planet.star_id() = 1;
  planet.planet_order() = 0;

  SectorMap smap(planet);
  for (int y = 0; y < planet.dimensions().y; ++y) {
    for (int x = 0; x < planet.dimensions().x; ++x) {
      auto& sect = smap.get(Coordinates{x, y});
      sect.set_coords(Coordinates{x, y});
      sect.set_condition(SectorType::SEC_LAND);
      sect.set_type(SectorType::SEC_LAND);
      sect.set_fert(80);
      sect.set_resource(50);
    }
  }

  auto& inhabited = smap.get(Coordinates{2, 3});
  inhabited.colonize(player_t{1}, 1000);
  inhabited.set_troops(200);

  // 1. Star NOT in supernova (nova_stage = 0) -> no devastation
  star_struct normal_star_data{};
  normal_star_data.nova_stage = 0;
  Star normal_star(normal_star_data);
  test::expect_false(process_supernova_sector_devastation(normal_star, smap));
  test::expect_eq(inhabited.get_popn(), 1000);

  // 2. Active radiation stage (nova_stage = 5) -> casualties, mineral deposits,
  // fertility loss
  star_struct active_star_data{};
  active_star_data.nova_stage = 5;
  Star active_star(active_star_data);
  test::expect_true(process_supernova_sector_devastation(active_star, smap));
  test::expect_lt(inhabited.get_popn(), 1000);
  test::expect_gt(inhabited.get_resource(), 50);
  test::expect_lt(inhabited.get_fert(), 80);

  // 3. Terminal explosion (nova_stage = 14) -> total incineration and cleared
  // ownership
  star_struct terminal_star_data{};
  terminal_star_data.nova_stage = 14;
  Star terminal_star(terminal_star_data);
  test::expect_true(process_supernova_sector_devastation(terminal_star, smap));
  test::expect_eq(inhabited.get_popn(), 0);
  test::expect_eq(inhabited.get_owner(), player_t{0});
  test::expect_eq(inhabited.get_troops(), 0);
}

void test_build_automated_waste_can() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  Star star = createTestStar();
  Planet planet = createTestPlanet();
  planet.star_id() = star.star_id();
  planet.planet_order() = 0;
  planet.conditions(TOXIC) = 80;

  SectorMap smap(planet);
  for (int y = 0; y < planet.dimensions().y; ++y) {
    for (int x = 0; x < planet.dimensions().x; ++x) {
      auto& sect = smap.get(Coordinates{x, y});
      sect.set_coords(Coordinates{x, y});
      sect.set_condition(SectorType::SEC_LAND);
    }
  }

  Race race = createTestRace(player_t{1});

  // 1. No threshold set -> returns nullopt, toxicity unmodified
  planet.info(race).tox_thresh = std::nullopt;
  planet.info(race).resource = 1000;
  auto res1 = build_automated_waste_can(em, star, planet, smap, race);
  test::expect_false(res1.has_value());
  test::expect_eq(planet.conditions(TOXIC), 80);

  // 2. Threshold higher than toxicity (90 > 80) -> returns nullopt
  planet.info(race).tox_thresh = 90;
  auto res2 = build_automated_waste_can(em, star, planet, smap, race);
  test::expect_false(res2.has_value());
  test::expect_eq(planet.conditions(TOXIC), 80);

  // 3. Threshold met (50 <= 80), but insufficient resources (0) -> returns
  // nullopt
  planet.info(race).tox_thresh = 50;
  planet.info(race).resource = 0;
  auto res3 = build_automated_waste_can(em, star, planet, smap, race);
  test::expect_false(res3.has_value());
  test::expect_eq(planet.conditions(TOXIC), 80);

  // 4. Threshold met and resources available -> builds toxic waste can holding
  // min(TOXMAX, 80) = 20
  planet.info(race).resource = 1000;
  auto res4 = build_automated_waste_can(em, star, planet, smap, race);
  test::expect_true(res4.has_value());
  test::expect_eq(planet.conditions(TOXIC), 80 - TOXMAX);

  const auto* ship = em.peek_ship(*res4);
  test::expect_true(ship != nullptr);
  test::expect_eq(ship->type(), ShipType::OTYPE_TOXWC);
  test::expect_eq(ship->owner(), player_t{1});
  test::expect_true(ship->docked());
  test::expect_true(smap.in_bounds(ship->land_coords()));
  test::expect_eq(ship->as<ToxicWasteShip>()->toxic_level(),
                  static_cast<unsigned char>(TOXMAX));
}

void test_check_mutual_alliances() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);
  RaceRepository races(store);

  Race r1 = createTestRace(player_t{1});
  Race r2 = createTestRace(player_t{2});
  Race r3 = createTestRace(player_t{3});

  // Base state: no alliances set
  races.save(r1);
  races.save(r2);
  races.save(r3);

  // 1. Empty player list -> true (trivially allied)
  test::expect_true(check_mutual_alliances(em, {}));

  // 2. Single player list -> true (single conqueror)
  std::vector<player_t> single_player = {player_t{1}};
  test::expect_true(check_mutual_alliances(em, single_player));

  // 3. Two unallied players -> false
  std::vector<player_t> pair = {player_t{1}, player_t{2}};
  test::expect_false(check_mutual_alliances(em, pair));

  // 4. One-way alliance: 1 allies 2, but 2 does not ally 1 -> false
  em.mutate_race(player_t{1},
                 [](Race& r) { r.declare_alliance_with(player_t{2}); });
  test::expect_false(check_mutual_alliances(em, pair));

  // 5. Mutual alliance: 1 allies 2, 2 allies 1 -> true
  em.mutate_race(player_t{2},
                 [](Race& r) { r.declare_alliance_with(player_t{1}); });
  test::expect_true(check_mutual_alliances(em, pair));

  // 6. Three players: 1-2 allied, 2-3 allied, but 1-3 unallied -> false
  em.mutate_race(player_t{2},
                 [](Race& r) { r.declare_alliance_with(player_t{3}); });
  em.mutate_race(player_t{3},
                 [](Race& r) { r.declare_alliance_with(player_t{2}); });
  std::vector<player_t> trio = {player_t{1}, player_t{2}, player_t{3}};
  test::expect_false(check_mutual_alliances(em, trio));

  // 7. Three players: complete mutual alliance triangle (1-2, 2-3, 1-3) -> true
  em.mutate_race(player_t{1},
                 [](Race& r) { r.declare_alliance_with(player_t{3}); });
  em.mutate_race(player_t{3},
                 [](Race& r) { r.declare_alliance_with(player_t{1}); });
  test::expect_true(check_mutual_alliances(em, trio));

  // 8. Non-existent player ID (unallied -> false)
  std::vector<player_t> invalid_pair = {player_t{1}, player_t{5}};
  test::expect_false(check_mutual_alliances(em, invalid_pair));

  // 9. Allied with non-existent player -> throws EntityNotFoundError when
  // loading missing race
  em.mutate_race(player_t{1},
                 [](Race& r) { r.declare_alliance_with(player_t{5}); });
  test::expect_throws<EntityNotFoundError>(
      [&]() { (void)check_mutual_alliances(em, invalid_pair); });
}

void test_calculate_plunder_distribution() {
  // 1. Error case: No conquerors
  {
    Stockpile loot{
        .resources = 100, .destruct = 50, .fuel = 20, .crystals = 10};
    auto res = calculate_plunder_distribution(loot, {});
    test::expect_false(res.has_value());
    test::expect_eq(res.error(), PlunderError::NoConquerors);
  }

  // 2. Error case: Empty loot
  {
    std::vector<player_t> conquerors = {player_t{1}, player_t{2}};
    auto res = calculate_plunder_distribution(Stockpile{}, conquerors);
    test::expect_false(res.has_value());
    test::expect_eq(res.error(), PlunderError::EmptyLoot);
  }

  // 3. Single conqueror receives 100% of the loot
  {
    Stockpile loot{
        .resources = 150, .destruct = 75, .fuel = 300, .crystals = 42};
    std::vector<player_t> conquerors = {player_t{2}};
    auto res = calculate_plunder_distribution(loot, conquerors);
    test::expect_true(res.has_value());
    test::expect_eq(res->shares.size(), 1UL);
    test::expect_eq(res->shares[0].player, player_t{2});
    test::expect_eq(res->shares[0].share, loot);
    test::expect_eq(res->total_loot, loot);
  }

  // 4. Two conquerors: even split & conservation
  {
    Stockpile loot{
        .resources = 100, .destruct = 60, .fuel = 40, .crystals = 20};
    std::vector<player_t> conquerors = {player_t{1}, player_t{2}};
    auto res = calculate_plunder_distribution(loot, conquerors);
    test::expect_true(res.has_value());
    test::expect_eq(res->shares.size(), 2UL);
    test::expect_eq(res->shares[0].player, player_t{1});
    test::expect_eq(res->shares[1].player, player_t{2});

    // Invariant: sum of all shares matches total_loot exactly
    Stockpile sum = res->shares[0].share;
    sum += res->shares[1].share;
    test::expect_eq(sum, loot);
  }

  // 5. Three conquerors: odd division and remainder conservation
  {
    Stockpile loot{.resources = 100, .destruct = 50, .fuel = 7, .crystals = 1};
    std::vector<player_t> conquerors = {player_t{1}, player_t{2}, player_t{3}};
    auto res = calculate_plunder_distribution(loot, conquerors);
    test::expect_true(res.has_value());
    test::expect_eq(res->shares.size(), 3UL);

    Stockpile sum{};
    for (const auto& share : res->shares) {
      sum += share.share;
    }
    test::expect_eq(sum, loot);
    test::expect_eq(sum.resources, 100U);
    test::expect_eq(sum.destruct, 50U);
    test::expect_eq(sum.fuel, 7U);
    test::expect_eq(sum.crystals, 1U);
  }
}

void test_recover_conquered_stockpiles() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race r1 = createTestRace(player_t{1});
  r1.God = true;
  Race r2 = createTestRace(player_t{2});
  Race r3 = createTestRace(player_t{3});

  RaceRepository races(store);
  races.save(r1);
  races.save(r2);
  races.save(r3);

  Star star = createTestStar();
  Planet planet = createTestPlanet();

  // 1. No conquerors -> returns nullopt, defeated stockpiles untouched
  planet.info(player_t{3})
      .deposit_stockpile(Stockpile{
          .resources = 100, .destruct = 50, .fuel = 20, .crystals = 5});
  auto report1 = recover_conquered_stockpiles(em, star, planet);
  test::expect_false(report1.has_value());
  test::expect_eq(planet.info(player_t{3}).resource, 100U);

  // 2. Conqueror present, but unallied with another conqueror -> returns
  // nullopt
  planet.info(player_t{1}).numsectsowned = 5;
  planet.info(player_t{2}).numsectsowned = 5;
  auto report2 = recover_conquered_stockpiles(em, star, planet);
  test::expect_false(report2.has_value());
  test::expect_eq(planet.info(player_t{3}).resource, 100U);

  // 3. Mutual alliance established -> plunder distributed, defeated player
  // drained
  em.mutate_race(player_t{1},
                 [](Race& r) { r.declare_alliance_with(player_t{2}); });
  em.mutate_race(player_t{2},
                 [](Race& r) { r.declare_alliance_with(player_t{1}); });

  auto report3 = recover_conquered_stockpiles(em, star, planet);
  test::expect_true(report3.has_value());
  const auto& rep = *report3;
  test::expect_eq(rep.recipients.size(), 2UL);
  test::expect_eq(rep.allocated_shares.size(), 2UL);
  test::expect_eq(rep.total_stolen.resources, 100U);
  test::expect_eq(rep.total_stolen.destruct, 50U);
  test::expect_eq(rep.total_stolen.fuel, 20U);
  test::expect_eq(rep.total_stolen.crystals, 5U);

  // Conquerors received their shares
  test::expect_eq(planet.info(player_t{1}).resource +
                      planet.info(player_t{2}).resource,
                  100U);
  test::expect_eq(planet.info(player_t{1}).destruct +
                      planet.info(player_t{2}).destruct,
                  50U);
  test::expect_eq(planet.info(player_t{1}).fuel + planet.info(player_t{2}).fuel,
                  20U);
  test::expect_eq(planet.info(player_t{1}).crystals +
                      planet.info(player_t{2}).crystals,
                  5U);

  // Defeated player's stockpiles drained
  test::expect_true(planet.info(player_t{3}).stockpile().empty());

  // 4. God race (Player 1 as God) without sectors -> protected from theft
  {
    Planet god_planet = createTestPlanet();
    god_planet.info(player_t{1})
        .deposit_stockpile(Stockpile{
            .resources = 500, .destruct = 500, .fuel = 500, .crystals = 500});
    god_planet.info(player_t{2}).numsectsowned = 5;

    auto god_report = recover_conquered_stockpiles(em, star, god_planet);
    test::expect_false(god_report.has_value());
    test::expect_eq(god_planet.info(player_t{1}).resource, 500U);
    test::expect_eq(god_planet.info(player_t{2}).resource, 0U);
  }
}

void test_format_recovery_report() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race r1 = createTestRace(player_t{1});
  r1.name = "Humanoid";
  Race r2 = createTestRace(player_t{2});
  r2.name = "Klingon";

  RaceRepository races(store);
  races.save(r1);
  races.save(r2);

  // 1. Empty report -> empty lines vector
  RecoveryReport empty_rep{};
  test::expect_true(format_recovery_report(empty_rep, em).empty());

  // 2. Full report -> correct ASCII header, body, and summary lines
  RecoveryReport report{
      .star_id = starnum_t{1},
      .star_name = "Sol",
      .planet_name = "Earth",
      .planet_num = planetnum_t{0},
      .recipients = {player_t{1}, player_t{2}},
      .allocated_shares =
          {
              PlayerLootShare{.player = player_t{1},
                              .share = Stockpile{.resources = 60,
                                                 .destruct = 30,
                                                 .fuel = 10,
                                                 .crystals = 3}},
              PlayerLootShare{.player = player_t{2},
                              .share = Stockpile{.resources = 40,
                                                 .destruct = 20,
                                                 .fuel = 10,
                                                 .crystals = 2}},
          },
      .total_stolen =
          Stockpile{
              .resources = 100, .destruct = 50, .fuel = 20, .crystals = 5},
  };

  const auto output = format_recovery_report(report, em);
  test::expect_false(output.empty());
  test::expect_true(output.contains("Recovery Report: Planet /Sol/Earth\n"));
  test::expect_true(output.contains("res"));
  test::expect_true(output.contains("destr"));
  test::expect_true(output.contains("fuel"));
  test::expect_true(output.contains("xtal"));
  test::expect_true(output.contains("Humanoid"));
  test::expect_true(output.contains("Klingon"));
  test::expect_true(output.contains("Total:"));
  test::expect_true(output.contains("60"));
  test::expect_true(output.contains("40"));
  test::expect_true(output.contains("100"));
}

void test_planet_exploration_context() {
  PlanetExplorationContext ctx(Coordinates{5, 3});
  test::expect_eq(ctx.dimensions().x, 5);
  test::expect_eq(ctx.dimensions().y, 3);
  test::expect_eq(ctx.maxx(), 5);
  test::expect_eq(ctx.maxy(), 3);

  // Initially all sectors are unexplored (0)
  test::expect_false(ctx.all_explored());
  test::expect_false(ctx.all_explored(player_t{1}));
  for (int y = 0; y < 3; ++y) {
    for (int x = 0; x < 5; ++x) {
      test::expect_false(ctx.is_explored(Coordinates{x, y}));
      test::expect_false(ctx.is_explored(Coordinates{x, y}, player_t{1}));
    }
  }

  // Set explored for player 1
  ctx.set_explored(Coordinates{2, 1}, player_t{1});
  test::expect_true(ctx.is_explored(Coordinates{2, 1}));
  test::expect_true(ctx.is_explored(Coordinates{2, 1}, player_t{1}));
  test::expect_false(ctx.is_explored(Coordinates{2, 1}, player_t{2}));
  test::expect_false(ctx.all_explored());
  test::expect_false(ctx.all_explored(player_t{1}));

  // explore_sector on an already-explored sector propagates to 4 neighbors:
  // (2, 1) -> (1, 1), (3, 1), (2, 0), (2, 2)
  Sector s_at_2_1{};
  s_at_2_1.set_coords(Coordinates{2, 1});
  ctx.explore_sector(s_at_2_1, player_t{1});
  test::expect_true(ctx.is_explored(Coordinates{1, 1}, player_t{1}));
  test::expect_true(ctx.is_explored(Coordinates{3, 1}, player_t{1}));
  test::expect_true(ctx.is_explored(Coordinates{2, 0}, player_t{1}));
  test::expect_true(ctx.is_explored(Coordinates{2, 2}, player_t{1}));

  // Toroidal boundary propagation in X:
  // Set (0, 0) explored, then propagate for player 2 wrapping left to (4, 0)
  // and down to (0, 1)
  ctx.set_explored(Coordinates{0, 0}, player_t{2});
  Sector s_at_0_0{};
  s_at_0_0.set_coords(Coordinates{0, 0});
  ctx.explore_sector(s_at_0_0, player_t{2});
  test::expect_true(ctx.is_explored(Coordinates{4, 0}, player_t{2}));
  test::expect_true(ctx.is_explored(Coordinates{1, 0}, player_t{2}));
  test::expect_true(ctx.is_explored(Coordinates{0, 1}, player_t{2}));

  // Explore by sector ownership when unpopulated
  Sector s_owned{};
  s_owned.set_coords(Coordinates{4, 2});
  s_owned.set_owner(3);
  test::expect_false(ctx.is_explored(Coordinates{4, 2}, player_t{3}));
  ctx.explore_sector(s_owned, player_t{3});
  test::expect_true(ctx.is_explored(Coordinates{4, 2}, player_t{3}));

  // Filling all remaining cells for player 1 makes all_explored(p1) true
  for (int y = 0; y < 3; ++y) {
    for (int x = 0; x < 5; ++x) {
      ctx.set_explored(Coordinates{x, y}, player_t{1});
    }
  }
  test::expect_true(ctx.all_explored(player_t{1}));
}

void test_process_island_exploration() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race1 = createTestRace(1);
  race1.name = "Human";
  race1.likesbest = SectorType::SEC_LAND;
  race1.number_sexes = 2;
  RaceRepository races(store);
  races.save(race1);

  Star star = createTestStar();
  StarRepository stars(store);
  stars.save(star);

  Planet planet = createTestPlanet();
  planet.star_id() = star.star_id();
  planet.planet_order() = 0;
  planet.expltimer() = 3;
  PlanetRepository planets(store);
  planets.save(planet);

  SectorMap smap(planet);
  for (int y = 0; y < 5; ++y) {
    for (int x = 0; x < 5; ++x) {
      auto& s = smap.get(Coordinates{x, y});
      s.set_coords(Coordinates{x, y});
      s.set_owner(0);
      s.clear_popn();
      s.set_condition(SectorType::SEC_SEA);
    }
  }

  // Player 1 owns (0, 0)
  auto& s00 = smap.get(Coordinates{0, 0});
  s00.set_owner(1);
  s00.set_popn_exact(100);
  s00.set_condition(SectorType::SEC_LAND);
  planet.info(player_t{1}).numsectsowned = 1;

  // Neighbor (1, 0) is land matching race1.likesbest
  auto& s10 = smap.get(Coordinates{1, 0});
  s10.set_condition(SectorType::SEC_LAND);

  TurnStats stats{};

  // 1. Timer countdown test (expltimer = 3 -> 2, no exploration)
  auto disc1 = process_island_exploration(em, star, planet, smap, stats);
  test::expect_false(disc1.has_value());
  test::expect_eq(planet.expltimer(), 2);
  test::expect_false(stats.Claims);

  // 2. Nova stage test (nova_stage = 1, skips exploration even if expltimer =
  // 0)
  planet.expltimer() = 0;
  Star nova_star = star;
  nova_star.nova_stage() = 1;
  auto disc2 = process_island_exploration(em, nova_star, planet, smap, stats);
  test::expect_false(disc2.has_value());
  test::expect_false(stats.Claims);

  // 3. Discovery test (expltimer = 0, normal star)
  planet.expltimer() = 0;
  auto disc3 = process_island_exploration(em, star, planet, smap, stats);
  test::expect_true(disc3.has_value());
  if (disc3.has_value()) {
    test::expect_eq(disc3->player, player_t{1});
    test::expect_true(stats.Claims);
    test::expect_eq(stats.tot_captured, 1);
    test::expect_eq(planet.expltimer(), 5);

    // Sector (1, 0) is now colonized by player 1 with population = number_sexes
    // (2)
    auto& s_discovered = smap.get(disc3->coords);
    test::expect_eq(s_discovered.get_owner(), player_t{1});
    test::expect_eq(s_discovered.get_popn(), 2);
  }
}

void test_process_enslavement_and_revolts() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race master_race = createTestRace(1);
  master_race.name = "MasterRace";
  Race slave_race = createTestRace(2);
  slave_race.name = "SlaveRace";
  RaceRepository races(store);
  races.save(master_race);
  races.save(slave_race);

  Star star = createTestStar();
  StarRepository stars(store);
  stars.save(star);

  Planet planet = createTestPlanet();
  planet.star_id() = star.star_id();
  planet.planet_order() = 0;
  PlanetRepository planets(store);
  planets.save(planet);

  SectorMap smap(planet);
  for (int y = 0; y < 5; ++y) {
    for (int x = 0; x < 5; ++x) {
      auto& s = smap.get(Coordinates{x, y});
      s.set_coords(Coordinates{x, y});
      s.set_owner(0);
      s.clear_popn();
    }
  }

  // 1. Not enslaved: outcome is None
  planet.free_slaves();
  TurnStats stats1{};
  auto res1 = process_enslavement_and_revolts(em, star, planet, smap, stats1);
  test::expect_eq(res1.outcome, EnslavementOutcome::None);

  // 2. Peaceful production diversion:
  // Planet is enslaved to Player 1. Master has ample population (10,000) so no
  // revolt triggers.
  planet.enslave_to(1);
  planet.popn() = 10000;
  planet.info(player_t{1}).popn = 10000;
  planet.info(player_t{2}).numsectsowned = 5;
  planet.info(player_t{1}).resource = 10;
  planet.info(player_t{1}).fuel = 5;
  planet.info(player_t{1}).destruct = 2;

  TurnStats stats2{};
  stats2.prod_res[player_t{2}] = 50;
  stats2.prod_fuel[player_t{2}] = 25;
  stats2.prod_destruct[player_t{2}] = 8;

  auto res2 = process_enslavement_and_revolts(em, star, planet, smap, stats2);
  test::expect_eq(res2.outcome, EnslavementOutcome::ProductionDiverted);
  test::expect_eq(res2.master, player_t{1});
  test::expect_eq(planet.info(player_t{1}).resource, 60);
  test::expect_eq(planet.info(player_t{1}).fuel, 30);
  test::expect_eq(planet.info(player_t{1}).destruct, 10);
  test::expect_eq(stats2.prod_res[player_t{2}], 0);
  test::expect_eq(stats2.prod_fuel[player_t{2}], 0);
  test::expect_eq(stats2.prod_destruct[player_t{2}], 0);
  test::expect_true(planet.is_enslaved());

  // 3. Slave revolt:
  // Master population drops below threshold (popn = 5 vs total popn = 10,000 /
  // 1000 = 10)
  planet.enslave_to(1);
  planet.popn() = 10000;
  planet.info(player_t{1}).popn = 5;
  planet.info(player_t{1}).numsectsowned = 1;
  planet.info(player_t{2}).numsectsowned = 5;

  auto& s_master = smap.get(Coordinates{0, 0});
  s_master.set_owner(1);
  s_master.set_popn_exact(5);

  TurnStats stats3{};
  stats3.Stinfo[star.star_id().value][planet.planet_order().value].intimidated =
      true;

  auto res3 = process_enslavement_and_revolts(em, star, planet, smap, stats3);
  test::expect_eq(res3.outcome, EnslavementOutcome::SlaveRevolt);
  test::expect_eq(res3.master, player_t{1});
  test::expect_false(planet.is_enslaved());
  test::expect_eq(planet.slaved_to(), 0);

  // Verify telegram was pushed to slave player
  auto telegrams = em.get_telegrams(player_t{2}, governor_t{0});
  test::expect_false(telegrams.empty());
  test::expect_true(telegrams[0].message.contains("SLAVE REVOLT"));
}

void test_divert_slave_tribute() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race master_race = createTestRace(1);
  master_race.name = "MasterRace";
  Race slave_race = createTestRace(2);
  slave_race.name = "SlaveRace";
  RaceRepository races(store);
  races.save(master_race);
  races.save(slave_race);

  Planet planet = createTestPlanet();
  planet.info(player_t{1}).resource = 100;
  planet.info(player_t{1}).fuel = 50;
  planet.info(player_t{1}).destruct = 10;
  planet.info(player_t{2}).numsectsowned = 3;

  TurnStats stats{};
  stats.prod_res[player_t{2}] = 30;
  stats.prod_fuel[player_t{2}] = 15;
  stats.prod_destruct[player_t{2}] = 5;

  divert_slave_tribute(em, planet, stats, player_t{1});

  test::expect_eq(planet.info(player_t{1}).resource, 130);
  test::expect_eq(planet.info(player_t{1}).fuel, 65);
  test::expect_eq(planet.info(player_t{1}).destruct, 15);
  test::expect_eq(stats.prod_res[player_t{2}], 0);
  test::expect_eq(stats.prod_fuel[player_t{2}], 0);
  test::expect_eq(stats.prod_destruct[player_t{2}], 0);
}

void test_notify_slave_revolt() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race master_race = createTestRace(1);
  master_race.name = "MasterRace";
  Race slave_race = createTestRace(2);
  slave_race.name = "SlaveRace";
  RaceRepository races(store);
  races.save(master_race);
  races.save(slave_race);

  Star star = createTestStar();
  Planet planet = createTestPlanet();
  planet.info(player_t{2}).numsectsowned = 1;

  notify_slave_revolt(em, star, planet, player_t{1});

  // Verify telegram was pushed to player 2
  auto telegrams = em.get_telegrams(player_t{2}, governor_t{0});
  test::expect_false(telegrams.empty());
  test::expect_true(telegrams[0].message.contains("SLAVE REVOLT"));
}

void test_recalculate_census() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race1 = createTestRace(1);
  race1.name = "Player1Race";
  Race race2 = createTestRace(2);
  race2.name = "Player2Race";
  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  Star star = createTestStar();
  StarRepository stars(store);
  stars.save(star);

  Planet planet = createTestPlanet();
  planet.star_id() = star.star_id();
  planet.planet_order() = 0;
  planet.dimensions() = Coordinates{2, 2};
  planet.conditions(TOXIC) = 10;
  PlanetRepository planets(store);
  planets.save(planet);

  SectorMap smap(planet);
  // (0,0): unowned sector with resources
  auto& s00 = smap.get(Coordinates{0, 0});
  s00.set_owner(0);
  s00.clear_popn();
  s00.clear_troops();
  s00.set_resource(50);

  // (0,1): owned by player 1
  auto& s01 = smap.get(Coordinates{0, 1});
  s01.set_owner(1);
  s01.set_popn_exact(100);
  s01.set_troops(20);
  s01.set_efficiency_bounded(80);
  s01.set_mobilization(50);
  s01.set_resource(25);
  s01.set_fert(10);
  s01.set_condition(SectorType::SEC_LAND);

  // (1,0): owned by player 1
  auto& s10 = smap.get(Coordinates{1, 0});
  s10.set_owner(1);
  s10.set_popn_exact(200);
  s10.set_troops(30);
  s10.set_efficiency_bounded(90);
  s10.set_mobilization(60);
  s10.set_resource(15);
  s10.set_fert(15);
  s10.set_condition(SectorType::SEC_LAND);

  // (1,1): owned by player 2
  auto& s11 = smap.get(Coordinates{1, 1});
  s11.set_owner(2);
  s11.set_popn_exact(300);
  s11.set_troops(40);
  s11.set_efficiency_bounded(70);
  s11.set_mobilization(40);
  s11.set_resource(10);
  s11.set_fert(20);
  s11.set_condition(SectorType::SEC_LAND);

  TurnStats stats{};
  stats.Compat[player_t{1}] = 100.0;
  stats.Compat[player_t{2}] = 80.0;

  recalculate_census(em, star, planet, smap, stats);

  test::expect_eq(planet.total_resources(), 100);
  test::expect_eq(planet.popn(), 600);
  test::expect_eq(planet.troops(), 90);

  test::expect_eq(planet.info(player_t{1}).numsectsowned, 2);
  test::expect_eq(planet.info(player_t{1}).popn, 300);
  test::expect_eq(planet.info(player_t{1}).troops, 50);

  test::expect_eq(planet.info(player_t{2}).numsectsowned, 1);
  test::expect_eq(planet.info(player_t{2}).popn, 300);
  test::expect_eq(planet.info(player_t{2}).troops, 40);

  test::expect_eq(stats.Power[player_t{1}].popn, 300);
  test::expect_eq(stats.Power[player_t{1}].troops, 50);
  test::expect_eq(stats.Power[player_t{1}].sum_eff, 170);
  test::expect_eq(stats.Power[player_t{1}].sum_mob, 110);

  test::expect_eq(stats.Power[player_t{2}].popn, 300);
  test::expect_eq(stats.Power[player_t{2}].troops, 40);
  test::expect_eq(stats.Power[player_t{2}].sum_eff, 70);
  test::expect_eq(stats.Power[player_t{2}].sum_mob, 40);

  test::expect_eq(stats.starpopns[star.star_id().value][player_t{1}], 300);
  test::expect_eq(stats.starpopns[star.star_id().value][player_t{2}], 300);
  test::expect_true(planet.maxpopn() > 0);
}

void test_update_planet_toxicity() {
  Planet planet = createTestPlanet();
  planet.conditions(TOXIC) = 10;
  planet.popn() = 250;
  planet.maxpopn() = 100;

  update_planet_toxicity(planet);
  // 10 + (250 / 100) = 12
  test::expect_eq(planet.conditions(TOXIC), 12);

  // Severe overpopulation clamping to 100
  planet.conditions(TOXIC) = 90;
  planet.popn() = 2000;
  planet.maxpopn() = 100;
  update_planet_toxicity(planet);
  test::expect_eq(planet.conditions(TOXIC), 100);

  // Zero maxpopn edge case - no division by zero
  planet.conditions(TOXIC) = 15;
  planet.popn() = 50;
  planet.maxpopn() = 0;
  update_planet_toxicity(planet);
  test::expect_eq(planet.conditions(TOXIC), 15);
}

void test_process_planet_economy() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race1 = createTestRace(1);
  race1.name = "Player1Race";
  Race race2 = createTestRace(2);
  race2.name = "Player2Race";
  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  Star star = createTestStar();
  StarRepository stars(store);
  stars.save(star);

  Planet planet = createTestPlanet();
  planet.star_id() = star.star_id();
  planet.planet_order() = 0;
  planet.dimensions() = Coordinates{2, 2};
  planet.conditions(TOXIC) = 5;
  planet.popn() = 200;
  planet.maxpopn() = 100;
  planet.info(player_t{1}).numsectsowned = 2;
  planet.info(player_t{1}).popn = 200;
  planet.info(player_t{1}).tax = 10;
  planet.info(player_t{1}).tech_invest = 10;
  planet.info(player_t{1}).resource = 100;
  planet.info(player_t{1}).fuel = 50;
  planet.info(player_t{1}).destruct = 20;
  planet.info(player_t{1}).crystals = 5;

  PlanetRepository planets(store);
  planets.save(planet);

  SectorMap smap(planet);

  TurnStats stats{};
  stats.prod_fuel[player_t{1}] = 25;
  stats.prod_res[player_t{1}] = 15;
  stats.prod_destruct[player_t{1}] = 10;
  stats.prod_crystals[player_t{1}] = 2;
  stats.avg_mob[player_t{1}] = 50;

  process_planet_economy(em, star, planet, smap, stats);

  // Stockpiles increased by production deposits
  test::expect_eq(planet.info(player_t{1}).fuel, 75);
  test::expect_eq(planet.info(player_t{1}).resource, 115);
  test::expect_eq(planet.info(player_t{1}).destruct, 30);
  test::expect_eq(planet.info(player_t{1}).crystals, 7);

  // Toxicity updated: 5 + (200 / 100) = 7
  test::expect_eq(planet.conditions(TOXIC), 7);

  // Power metrics accumulated
  test::expect_eq(stats.Power[player_t{1}].resource, 115);
  test::expect_eq(stats.Power[player_t{1}].fuel, 75);
  test::expect_eq(stats.Power[player_t{1}].destruct, 30);
  test::expect_eq(stats.Power[player_t{1}].sectors_owned, 2);
  test::expect_eq(stats.Power[player_t{1}].planets_owned, 1);

  // Player 2 with 0 sectors owned has 0 power accumulation
  test::expect_eq(stats.Power[player_t{2}].sectors_owned, 0);
  test::expect_eq(stats.Power[player_t{2}].planets_owned, 0);
}

void test_reset_planet_turn_state() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race1 = createTestRace(1);
  race1.name = "Player1Race";
  Race race2 = createTestRace(2);
  race2.name = "Player2Race";
  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  Planet planet = createTestPlanet();
  planet.maxpopn() = 500;
  planet.popn() = 250;
  planet.troops() = 50;
  planet.total_resources() = 100;
  planet.info(player_t{1}).numsectsowned = 5;
  planet.info(player_t{1}).popn = 200;
  planet.info(player_t{1}).troops = 30;
  planet.info(player_t{1}).est_production = 50.0;

  TurnStats stats{};
  stats.Claims = true;
  stats.prod_fuel[player_t{1}] = 10;
  stats.prod_res[player_t{1}] = 20;

  reset_planet_turn_state(em, planet, stats);

  test::expect_false(stats.Claims);
  test::expect_eq(planet.maxpopn(), 0);
  test::expect_eq(planet.popn(), 0);
  test::expect_eq(planet.troops(), 0);
  test::expect_eq(planet.total_resources(), 0);

  test::expect_eq(planet.info(player_t{1}).numsectsowned, 0);
  test::expect_eq(planet.info(player_t{1}).popn, 0);
  test::expect_eq(planet.info(player_t{1}).troops, 0);
  test::expect_eq(planet.info(player_t{1}).est_production, 0.0);
  test::expect_eq(stats.prod_fuel[player_t{1}], 0);
  test::expect_eq(stats.prod_res[player_t{1}], 0);
  test::expect_true(stats.Compat[player_t{1}] > 0.0);
}

void test_process_planet_production() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(1);
  race.name = "ProdRace";
  race.likesbest = SectorType::SEC_LAND;
  race.likes[SectorType::SEC_LAND] = 1.0;
  RaceRepository races(store);
  races.save(race);

  Star star = createTestStar();
  StarRepository stars(store);
  stars.save(star);

  Planet planet = createTestPlanet();
  planet.star_id() = star.star_id();
  planet.planet_order() = 0;
  planet.dimensions() = Coordinates{2, 2};
  PlanetRepository planets(store);
  planets.save(planet);

  SectorMap smap(planet);
  auto& s00 = smap.get(Coordinates{0, 0});
  s00.set_owner(1);
  s00.set_popn_exact(100);
  s00.set_efficiency_bounded(100);
  s00.set_condition(SectorType::SEC_LAND);
  s00.set_resource(50);
  s00.set_fert(20);

  TurnStats stats{};
  stats.Compat[player_t{1}] = 100.0;

  process_planet_production(em, star, planet, smap, stats);
  test::expect_gt(stats.prod_res[player_t{1}], 0);
}

void test_send_planet_turn_telegrams() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(1);
  race.name = "TelegRace";
  RaceRepository races(store);
  races.save(race);

  Star star = createTestStar();
  StarRepository stars(store);
  stars.save(star);

  Planet planet = createTestPlanet();
  planet.star_id() = star.star_id();
  planet.planet_order() = 0;
  planet.info(player_t{1}).autorep = 2;

  TurnStats stats{};
  stats.prod_res[player_t{1}] = 40;
  stats.prod_fuel[player_t{1}] = 20;
  stats.prod_destruct[player_t{1}] = 10;
  stats.prod_crystals[player_t{1}] = 3;

  send_planet_turn_telegrams(em, star, planet, Coordinates{1, 1}, stats);

  test::expect_eq(planet.info(player_t{1}).autorep, 1);
  auto telegrams = em.get_telegrams(1, 0);
  test::expect_eq(telegrams.size(), 1);
  test::expect_true(
      telegrams[0].message.contains("Total      Prod: 40r 20f 10d"));
  test::expect_true(telegrams[0].message.contains("3 crystals found"));
  test::expect_true(
      telegrams[0].message.contains("Environmental damage on sector 1,1"));
}

void test_stinfo_simulation_defaults_and_types() {
  Stinfo info{};
  test::expect_eq(info.temp_add, 0);
  test::expect_false(info.thing_add);
  test::expect_false(info.inhab);
  test::expect_false(info.intimidated);

  info.temp_add = -25;
  info.thing_add = true;
  info.inhab = true;
  info.intimidated = true;

  test::expect_eq(info.temp_add, -25);
  test::expect_true(info.thing_add);
  test::expect_true(info.inhab);
  test::expect_true(info.intimidated);
}

}  // namespace

int main() {
  std::println(std::cout, "Running doplanet unit tests...\n");

  std::println(std::cout, "  Testing moveship_onplanet... ");
  test_moveship_onplanet();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing execute_terraforming... ");
  test_execute_terraforming();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing execute_plowing... ");
  test_execute_plowing();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing upgrade_sector_dome... ");
  test_upgrade_sector_dome();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing strip_mine_quarry... ");
  test_strip_mine_quarry();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing execute_berserker_bombardment... ");
  test_execute_berserker_bombardment();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing refuel_gasgiant_orbiters... ");
  test_refuel_gasgiant_orbiters();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing process_planetary_ships... ");
  test_process_planetary_ships();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing process_planet_climate... ");
  test_process_planet_climate();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing process_toxic_environmental_damage... ");
  test_process_toxic_environmental_damage();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing process_supernova_sector_devastation... ");
  test_process_supernova_sector_devastation();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing build_automated_waste_can... ");
  test_build_automated_waste_can();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing check_mutual_alliances... ");
  test_check_mutual_alliances();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing calculate_plunder_distribution... ");
  test_calculate_plunder_distribution();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing recover_conquered_stockpiles... ");
  test_recover_conquered_stockpiles();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing format_recovery_report... ");
  test_format_recovery_report();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing PlanetExplorationContext... ");
  test_planet_exploration_context();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing process_island_exploration... ");
  test_process_island_exploration();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing process_enslavement_and_revolts... ");
  test_process_enslavement_and_revolts();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing divert_slave_tribute... ");
  test_divert_slave_tribute();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing notify_slave_revolt... ");
  test_notify_slave_revolt();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing recalculate_census... ");
  test_recalculate_census();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing update_planet_toxicity... ");
  test_update_planet_toxicity();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing process_planet_economy... ");
  test_process_planet_economy();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing reset_planet_turn_state... ");
  test_reset_planet_turn_state();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing process_planet_production... ");
  test_process_planet_production();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing send_planet_turn_telegrams... ");
  test_send_planet_turn_telegrams();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing do_recover... ");
  test_do_recover();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing doplanet full cycle... ");
  test_doplanet_full_cycle();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing exploration island discovery... ");
  test_exploration_island_discovery();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing 64-bit production and stockpiles... ");
  test_64bit_production_and_stockpiles();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing TurnStats PlayerVector accumulation... ");
  test_turnstats_playervector_accumulation();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing Stinfo simulation defaults and types... ");
  test_stinfo_simulation_defaults_and_types();
  std::println(std::cout, "PASS");

  std::println(std::cout, "All doplanet tests passed!");
  return 0;
}
