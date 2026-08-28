// SPDX-License-Identifier: Apache-2.0

/// \file doship_test.cc
/// \brief Unit tests for doship() turn simulation actions: domass, doown,
/// habitat population/resource growth, and weapon plant production.

import dallib;
import gblib;
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
  missile1_data.special = ImpactData{.x = 1, .y = 1, .scatter = 0};
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

  do_canister(canister, em, stats);
  test::expect_eq(std::get<TimerData>(canister.special()).count, 1);
  test::expect_lt(stats.Stinfo[1][0].temp_add, 0);

  canister.special() = TimerData{.count = DISSIPATE};
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

  std::println(std::cout, "All doship tests passed!");
  return 0;
}
