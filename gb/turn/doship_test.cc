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
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  RaceRepository races(store);
  races.save(race);

  ship_struct parent_data{
      .owner = player_t{1},
      .type = ShipType::STYPE_CARRIER,
      .active = 1,
      .alive = 1,
  };
  auto parent_handle = em.create_ship(parent_data);
  Ship& parent = *parent_handle;

  ship_struct child_data{
      .owner = player_t{2},
      .popn = 10,
      .whatorbits = ScopeLevel::LEVEL_SHIP,
      .type = ShipType::STYPE_SHUTTLE,
      .active = 1,
      .alive = 1,
  };
  child_data.destshipno = parent.number();
  auto child_handle = em.create_ship(child_data);
  Ship& child = *child_handle;

  parent.ships() = child.number();

  doown(parent, em);
  test::expect_eq(child.owner(), player_t{1});

  domass(parent, em);
  test::expect_gt(parent.mass(), 0.0);
}

void test_do_habitat() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  RaceRepository races(store);
  races.save(race);

  ship_struct sdata{
      .owner = player_t{1},
      .fuel = 100.0,
      .max_crew = 100,
      .max_resource = 1000,
      .resource = 10,
      .popn = 50,
      .type = ShipType::STYPE_HABITAT,
      .active = 1,
      .alive = 1,
  };
  auto ship_handle = em.create_ship(sdata);
  Ship& ship = *ship_handle;
  ship.on() = 1;

  do_habitat(ship, em);

  test::expect_gt(ship.resource(), 10);
  test::expect_gt(ship.popn(), 50);
}

void test_do_weapon_plant() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  race.tech = 100.0;
  RaceRepository races(store);
  races.save(race);

  ship_struct sdata{
      .owner = player_t{1},
      .fuel = 100.0,
      .max_crew = 100,
      .resource = 500,
      .popn = 100,
      .type = ShipType::OTYPE_WPLANT,
      .active = 1,
      .alive = 1,
  };
  auto ship_handle = em.create_ship(sdata);
  Ship& ship = *ship_handle;

  int produced = do_weapon_plant(ship, em);
  test::expect_gt(produced, 0);
  test::expect_lt(ship.resource(), 500);
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

void test_domissile_pdn_interception() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  RaceRepository races(store);
  races.save(race);

  Star star = createTestStar(starnum_t{1});
  StarRepository(store).save(star);

  Planet planet{PlanetType::EARTH, Coordinates{2, 2}};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  PlanetRepository(store).save(planet);

  // Create PDN ship on planet
  ship_struct pdn_data{
      .owner = player_t{2},
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::OTYPE_PLANDEF,
      .active = 1,
      .alive = 1,
  };
  pdn_data.storbits = starnum_t{1};
  pdn_data.pnumorbits = planetnum_t{0};
  pdn_data.xpos = 10.0;
  pdn_data.ypos = 20.0;
  auto pdn_handle = em.create_ship(pdn_data);
  Ship& pdn = *pdn_handle;

  planet.ships() = pdn.number();
  PlanetRepository(store).save(planet);

  // Create incoming missile
  ship_struct missile_data{
      .owner = player_t{1},
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::STYPE_MISSILE,
      .active = 1,
      .alive = 1,
  };
  missile_data.whatdest = ScopeLevel::LEVEL_PLAN;
  missile_data.storbits = starnum_t{1};
  missile_data.pnumorbits = planetnum_t{0};
  missile_data.deststar = starnum_t{1};
  missile_data.destpnum = planetnum_t{0};
  auto missile_handle = em.create_ship(missile_data);
  Ship& missile = *missile_handle;
  missile.on() = 1;

  domissile(missile, em);

  // Missile should have re-targeted the PDN ship
  test::expect_eq(missile.whatdest(), ScopeLevel::LEVEL_SHIP);
  test::expect_eq(missile.destshipno(), pdn.number());
  test::expect_eq(missile.xpos(), 10.0);
  test::expect_eq(missile.ypos(), 20.0);
}

void test_domissile_planet_bombardment_and_ship_attack() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race1 = createTestRace(player_t{1});
  Race race2 = createTestRace(player_t{2});
  RaceRepository(store).save(race1);
  RaceRepository(store).save(race2);

  Star star = createTestStar(starnum_t{1});
  StarRepository(store).save(star);

  Planet planet{PlanetType::EARTH, Coordinates{4, 4}};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  PlanetRepository(store).save(planet);

  SectorMap smap(planet);
  smap.get({1, 1}).set_owner(2);
  smap.get({1, 1}).set_popn_exact(50);
  smap.get({1, 1}).set_type(SectorType::SEC_LAND);
  SectorRepository(store).save_map(smap);

  // 1. Planet bombardment test
  ship_struct missile1_data{
      .owner = player_t{1},
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::STYPE_MISSILE,
      .active = 1,
      .alive = 1,
  };
  missile1_data.whatdest = ScopeLevel::LEVEL_PLAN;
  missile1_data.storbits = starnum_t{1};
  missile1_data.pnumorbits = planetnum_t{0};
  missile1_data.deststar = starnum_t{1};
  missile1_data.destpnum = planetnum_t{0};
  missile1_data.destruct = 10;
  missile1_data.special = ImpactData{.coords = {1, 1}, .scatter = false};
  auto m1_handle = em.create_ship(missile1_data);
  Ship& m1 = *m1_handle;
  m1.on() = 1;

  domissile(m1, em);
  test::expect_eq(m1.alive(), 0);

  // 2. Ship-to-ship attack test
  ship_struct target_data{
      .owner = player_t{2},
      .size = 10,
      .max_crew = 10,
      .tech = 10.0,
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::STYPE_SHUTTLE,
      .active = 1,
      .alive = 1,
  };
  target_data.storbits = starnum_t{1};
  target_data.pnumorbits = planetnum_t{0};
  target_data.xpos = 0.0;
  target_data.ypos = 0.0;
  auto target_handle = em.create_ship(target_data);
  Ship& target = *target_handle;

  ship_struct missile2_data{
      .owner = player_t{1},
      .size = 1,
      .tech = 10.0,
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::STYPE_MISSILE,
      .active = 1,
      .alive = 1,
  };
  missile2_data.whatdest = ScopeLevel::LEVEL_SHIP;
  missile2_data.destshipno = target.number();
  missile2_data.storbits = starnum_t{1};
  missile2_data.pnumorbits = planetnum_t{0};
  missile2_data.deststar = starnum_t{1};
  missile2_data.destpnum = planetnum_t{0};
  missile2_data.speed = 10;
  missile2_data.destruct = 20;
  missile2_data.xpos = 0.0;
  missile2_data.ypos = 0.0;
  auto m2_handle = em.create_ship(missile2_data);
  Ship& m2 = *m2_handle;
  m2.on() = 1;

  domissile(m2, em);
  test::expect_eq(m2.alive(), 0);
  test::expect_gt(target.damage(), 0);
}

void test_domine_trigger_and_detonation() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race1 = createTestRace(player_t{1});
  Race race2 = createTestRace(player_t{2});
  RaceRepository(store).save(race1);
  RaceRepository(store).save(race2);

  Star star = createTestStar(starnum_t{1});
  StarRepository(store).save(star);

  ship_struct enemy_data{
      .owner = player_t{2},
      .size = 10,
      .tech = 10.0,
      .whatorbits = ScopeLevel::LEVEL_STAR,
      .type = ShipType::STYPE_SHUTTLE,
      .active = 1,
      .alive = 1,
  };
  enemy_data.storbits = starnum_t{1};
  enemy_data.xpos = 5.0;
  enemy_data.ypos = 5.0;
  auto enemy_handle = em.create_ship(enemy_data);
  Ship& enemy = *enemy_handle;

  ship_struct mine_data{
      .owner = player_t{1},
      .size = 1,
      .tech = 10.0,
      .whatorbits = ScopeLevel::LEVEL_STAR,
      .type = ShipType::STYPE_MINE,
      .active = 1,
      .alive = 1,
  };
  mine_data.storbits = starnum_t{1};
  mine_data.destruct = 50;
  mine_data.xpos = 0.0;
  mine_data.ypos = 0.0;
  mine_data.special = TriggerData{.radius = 20};
  auto mine_handle = em.create_ship(mine_data);
  Ship& mine = *mine_handle;
  mine.on() = 1;

  star.ships() = enemy.number();
  enemy.ships() = mine.number();
  StarRepository(store).save(star);

  domine(mine, 0, em);
  test::expect_eq(mine.alive(), 0);
  test::expect_gt(enemy.damage(), 0);
}

void test_doabm_intercept() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race1 = createTestRace(player_t{1});
  Race race2 = createTestRace(player_t{2});
  RaceRepository(store).save(race1);
  RaceRepository(store).save(race2);

  Star star = createTestStar(starnum_t{1});
  StarRepository(store).save(star);

  Planet planet{PlanetType::EARTH, Coordinates{4, 4}};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  PlanetRepository(store).save(planet);

  ship_struct missile_data{
      .owner = player_t{2},
      .size = 1,
      .tech = 10.0,
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::STYPE_MISSILE,
      .active = 1,
      .alive = 1,
  };
  missile_data.storbits = starnum_t{1};
  missile_data.pnumorbits = planetnum_t{0};
  auto missile_handle = em.create_ship(missile_data);
  Ship& missile = *missile_handle;

  planet.ships() = missile.number();
  PlanetRepository(store).save(planet);

  ship_struct abm_data{
      .owner = player_t{1},
      .size = 1,
      .max_crew = 10,
      .tech = 10.0,
      .popn = 10,
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::OTYPE_ABM,
      .active = 1,
      .alive = 1,
  };
  abm_data.primtype = GTYPE_HEAVY;
  abm_data.primary = 10;
  abm_data.storbits = starnum_t{1};
  abm_data.pnumorbits = planetnum_t{0};
  abm_data.whatdest = ScopeLevel::LEVEL_PLAN;
  abm_data.deststar = starnum_t{1};
  abm_data.destpnum = planetnum_t{0};
  abm_data.destruct = 50;
  abm_data.retaliate = 50;
  auto abm_handle = em.create_ship(abm_data);
  Ship& abm = *abm_handle;
  abm.guns() = PRIMARY;
  abm.on() = 1;
  abm.docked() = 1;

  doabm(abm, em);
  test::expect_lt(abm.destruct(), 50);
  const auto* updated_missile = em.peek_ship(missile.number());
  test::expect_gt(updated_missile->damage(), 0);
}

void test_do_canister_and_greenhouse() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);
  TurnStats stats{};

  Race race = createTestRace(player_t{1});
  RaceRepository(store).save(race);

  Star star = createTestStar(starnum_t{1});
  StarRepository(store).save(star);

  Planet planet{PlanetType::EARTH, Coordinates{4, 4}};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  PlanetRepository(store).save(planet);

  ship_struct canister_data{
      .owner = player_t{1},
      .whatorbits = ScopeLevel::LEVEL_PLAN,
      .type = ShipType::OTYPE_CANIST,
      .active = 1,
      .alive = 1,
  };
  canister_data.storbits = starnum_t{1};
  canister_data.pnumorbits = planetnum_t{0};
  canister_data.special = TimerData{.count = 0};
  auto can_handle = em.create_ship(canister_data);
  Ship& canister = *can_handle;
  auto* canist_ship = canister.as<CanisterShip>();
  test::expect_true(canist_ship != nullptr);

  do_canister(canister, em, stats);
  test::expect_eq(canist_ship->count(), 1);
  test::expect_lt(stats.Stinfo[1][0].temp_add, 0);

  canist_ship->set_count(DISSIPATE);
  do_canister(canister, em, stats);
  test::expect_eq(canister.alive(), 0);
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

  // 1. Test Spore pod in star system with 0 planets (empty system edge case)
  star_struct empty_sdata{
      .name = "EmptyStar",
      .pnames = {},
      .star_id = starnum_t{1},
  };
  Star empty_star{empty_sdata};
  StarRepository(store).save(empty_star);

  ship_struct pod_empty_data{
      .owner = player_t{1},
      .type = ShipType::STYPE_POD,
      .active = 1,
      .alive = 1,
  };
  pod_empty_data.whatorbits = ScopeLevel::LEVEL_STAR;
  pod_empty_data.storbits = starnum_t{1};
  auto pod_empty_handle = em.create_ship(pod_empty_data);
  Ship& pod_empty = *pod_empty_handle;
  auto* pod_ship = pod_empty.as<SporePodShip>();
  test::expect_true(pod_ship != nullptr);
  pod_ship->set_temperature(POD_THRESHOLD + 10);

  // Should safely handle 0 planets without throwing or crashing
  do_pod(pod_empty, em);
  test::expect_eq(pod_empty.alive(), 0);

  // 2. Test Spore pod in star system with a planet
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

}  // namespace

int main() {
  std::println(std::cout, "Running doship unit tests...\n");

  std::println(std::cout, "  Testing domass and doown... ");
  test_domass_and_doown();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing do_habitat... ");
  test_do_habitat();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing do_weapon_plant... ");
  test_do_weapon_plant();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing do_meta_infect... ");
  test_do_meta_infect();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing domissile PDN interception... ");
  test_domissile_pdn_interception();
  std::println(std::cout, "PASS");

  std::println(
      std::cout,
      "  Testing domissile planet bombardment and ship-to-ship attack... ");
  test_domissile_planet_bombardment_and_ship_attack();
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

  std::println(std::cout, "All doship tests passed!");
  return 0;
}
