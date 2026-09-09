// SPDX-License-Identifier: Apache-2.0

/// \file doturn_test.cc
/// \brief Unit tests for full turn simulation execution, star stability repair,
/// and segment vs update turn execution.

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

Star createTestStar(starnum_t id = 0) {
  star_struct star_data{};
  star_data.name = "TestStar";
  star_data.star_id = id;
  star_data.stability = 50;
  star_data.nova_stage = 0;
  star_data.temperature = 100;
  star_data.gravity = 100.0;
  star_data.pnames.push_back("TestPlanet");
  return Star(star_data);
}

Planet createTestPlanet(starnum_t star_id = 0, planetnum_t pnum = 0) {
  Planet planet(PlanetType::EARTH, Coordinates{5, 5});
  planet.star_id() = star_id;
  planet.planet_order() = pnum;
  planet.xpos() = 1000.0;
  planet.ypos() = 1000.0;
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

void test_fix_stability() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  Star star = createTestStar();
  star.stability() = 99;

  fix_stability(em, star);
  test::expect_true(star.nova_stage() == 1 || star.stability() <= 100);

  star.nova_stage() = 15;
  fix_stability(em, star);
  test::expect_eq(star.nova_stage(), 0);
  test::expect_eq(star.stability(), 20);
}

void test_do_turn_segment_vs_update() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  ServerState state{};
  state.id = 1;
  state.segments = 2;
  ServerStateRepository state_repo(store);
  state_repo.save(state);

  universe_struct u{};
  u.id = 1;
  u.numstars = 1;
  UniverseRepository univ_repo(store);
  univ_repo.save(u);

  Race race = createTestRace(player_t{1});
  race.tech = 10.0;
  race.turn = 1;
  RaceRepository races(store);
  races.save(race);

  Star star = createTestStar(0);
  StarRepository stars(store);
  stars.save(star);

  Planet planet = createTestPlanet(0, 0);
  PlanetRepository planets(store);
  planets.save(planet);

  SectorMap initial_smap(planet);
  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 5; x++) {
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

  NullSessionRegistry session_registry;

  // 1. Run a segment turn (update = false)
  do_turn(em, session_registry, false);

  const auto* race_after_segment = em.peek_race(player_t{1});
  test::expect_ne(race_after_segment, nullptr);
  test::expect_eq(race_after_segment->turn, 1);

  // 2. Run a full update turn (update = true)
  do_turn(em, session_registry, true);

  const auto* race_after_update = em.peek_race(player_t{1});
  test::expect_ne(race_after_update, nullptr);
  test::expect_eq(race_after_update->turn, 2);
}

void test_do_turn_market_and_maintenance() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  universe_struct u{};
  u.id = 1;
  u.numstars = 2;
  UniverseRepository univ_repo(store);
  univ_repo.save(u);

  Race race1 = createTestRace(player_t{1});
  race1.governor[0].money = 1000;
  Race race2 = createTestRace(player_t{2});
  race2.governor[0].money = 2000;
  RaceRepository race_repo(store);
  race_repo.save(race1);
  race_repo.save(race2);

  Star star1 = createTestStar(starnum_t{0});
  Star star2 = createTestStar(starnum_t{1});
  StarRepository star_repo(store);
  star_repo.save(star1);
  star_repo.save(star2);

  Planet planet1 = createTestPlanet(starnum_t{0}, planetnum_t{0});
  Planet planet2 = createTestPlanet(starnum_t{1}, planetnum_t{0});
  PlanetRepository planet_repo(store);
  planet_repo.save(planet1);
  planet_repo.save(planet2);

  SectorMap smap1(planet1);
  SectorMap smap2(planet2);
  SectorRepository sector_repo(store);
  sector_repo.save_map(smap1);
  sector_repo.save_map(smap2);

  // Post a commodity lot: Seller 1, Bidder 2
  Commod commod{};
  commod.id = 1;
  commod.owner = player_t{1};
  commod.governor = governor_t{0};
  commod.type = CommodType::RESOURCE;
  commod.amount = 100;
  commod.star_from = starnum_t{0};
  commod.planet_from = planetnum_t{0};
  commod.star_to = starnum_t{1};
  commod.planet_to = planetnum_t{0};
  commod.bidder = player_t{2};
  commod.bidder_gov = governor_t{0};
  commod.bid = 500;
  commod.deliver = false;
  CommodRepository commod_repo(store);
  commod_repo.save(commod);

  NullSessionRegistry session_registry;

  // Run update turn
  do_turn(em, session_registry, true);

  // First turn delivered lot
  const auto* c1 = em.peek_commod(1);
  test::expect_ne(c1, nullptr);
  test::expect_true(c1->deliver);

  // Second turn processes trade
  do_turn(em, session_registry, true);

  // Commod lot should be deleted after successful purchase
  test::expect_throws<EntityNotFoundError>([&]() { em.peek_commod(1); });

  // Seller 1 gained money
  const auto* seller = em.peek_race(player_t{1});
  test::expect_gt(seller->governor[0].money, 1000);

  // Bidder 2 received resources on planet2
  const auto& p2_after = *em.peek_planet(starnum_t{1}, planetnum_t{0});
  test::expect_eq(p2_after.info(player_t{2}).resource, 100);
}

void test_do_turn_victory_scores_and_discoveries() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  universe_struct u{};
  u.id = 1;
  u.numstars = 1;
  UniverseRepository univ_repo(store);
  univ_repo.save(u);

  Race race = createTestRace(player_t{1});
  race.tech = 49.5;  // Just below TECH_HYPER_DRIVE (50.0)
  race.IQ = 100;     // Will gain +1.0 tech during turn
  race.governor[0].money = 500000;
  RaceRepository race_repo(store);
  race_repo.save(race);

  Star star = createTestStar(starnum_t{0});
  StarRepository star_repo(store);
  star_repo.save(star);

  Planet planet = createTestPlanet(starnum_t{0}, planetnum_t{0});
  planet.info(player_t{1}).numsectsowned = 5;
  planet.info(player_t{1}).explored = 1;
  planet.info(player_t{1}).resource = 100000;
  PlanetRepository planet_repo(store);
  planet_repo.save(planet);

  SectorMap smap(planet);
  SectorRepository sector_repo(store);
  sector_repo.save_map(smap);

  NullSessionRegistry session_registry;

  // Run full update turn
  do_turn(em, session_registry, true);

  const auto* race_after = em.peek_race(player_t{1});
  test::expect_ne(race_after, nullptr);
  // Tech increased
  test::expect_ge(race_after->tech, 20.0);
  // Discovered Hyperdrive
  test::expect_true(race_after->discoveries.hyperdrive);
  // Victory score calculated
  test::expect_gt(race_after->victory_score, 0);
}

void test_do_turn_victory_scores_with_derelict_and_multiple_players() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  universe_struct u{};
  u.id = 1;
  u.numstars = 1;
  UniverseRepository univ_repo(store);
  univ_repo.save(u);

  Race race1 = createTestRace(player_t{1});
  race1.morale = 100;
  race1.governor[0].money = 1000;
  race1.governor[1].active = true;
  race1.governor[1].money = 500;
  RaceRepository race_repo(store);
  race_repo.save(race1);

  Race race2 = createTestRace(player_t{2});
  race2.morale = 100;
  race2.governor[0].money = 2000;
  race_repo.save(race2);

  Star star = createTestStar(starnum_t{0});
  StarRepository star_repo(store);
  star_repo.save(star);

  Planet planet = createTestPlanet(starnum_t{0}, planetnum_t{0});
  planet.info(player_t{1}).numsectsowned = 10;
  planet.info(player_t{1}).explored = 1;
  planet.info(player_t{1}).resource = 50000;
  planet.info(player_t{2}).numsectsowned = 5;
  planet.info(player_t{2}).explored = 1;
  planet.info(player_t{2}).resource = 50000;
  PlanetRepository planet_repo(store);
  planet_repo.save(planet);

  SectorMap smap(planet);
  SectorRepository sector_repo(store);
  sector_repo.save_map(smap);

  ShipRepository ship_repo(store);

  // Player 1 ship
  Ship ship1{};
  ship1.number() = 1;
  ship1.owner() = player_t{1};
  ship1.alive() = true;
  ship1.tech() = 10.0;
  ship1.size() = 10;
  ship1.resource() = 100;
  ship_repo.save(ship1);

  // Derelict/unowned ship (owner == 0) - tests safety against negative indexing
  Ship derelict{};
  derelict.number() = 2;
  derelict.owner() = player_t{0};
  derelict.alive() = true;
  derelict.tech() = 5.0;
  ship_repo.save(derelict);

  NullSessionRegistry session_registry;

  // Run full update turn
  do_turn(em, session_registry, true);

  const auto* r1_after = em.peek_race(player_t{1});
  const auto* r2_after = em.peek_race(player_t{2});
  test::expect_ne(r1_after, nullptr);
  test::expect_ne(r2_after, nullptr);
  test::expect_gt(r1_after->victory_score, 0);
  test::expect_gt(r2_after->victory_score, 0);
}

void test_planet_deposit_commodity() {
  Planet planet(PlanetType::EARTH, Coordinates{5, 5});
  const player_t p{1};

  planet.deposit_commodity(CommodType::RESOURCE, 150, p);
  test::expect_eq(planet.info(p).resource, 150);

  planet.deposit_commodity(CommodType::FUEL, 80, p);
  test::expect_eq(planet.info(p).fuel, 80);

  planet.deposit_commodity(CommodType::DESTRUCT, 45, p);
  test::expect_eq(planet.info(p).destruct, 45);

  planet.deposit_commodity(CommodType::CRYSTAL, 10, p);
  test::expect_eq(planet.info(p).crystals, 10);
}

void test_process_market_transactions_isolated() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race1 = createTestRace(player_t{1});
  race1.governor[0].money = 500;
  Race race2 = createTestRace(player_t{2});
  race2.governor[0].money = 1000;
  RaceRepository race_repo(store);
  race_repo.save(race1);
  race_repo.save(race2);

  Star star1 = createTestStar(starnum_t{0});
  Star star2 = createTestStar(starnum_t{1});
  star2.xpos() = 50000.0;
  StarRepository star_repo(store);
  star_repo.save(star1);
  star_repo.save(star2);

  Planet planet1 = createTestPlanet(starnum_t{0}, planetnum_t{0});
  Planet planet2 = createTestPlanet(starnum_t{1}, planetnum_t{0});
  PlanetRepository planet_repo(store);
  planet_repo.save(planet1);
  planet_repo.save(planet2);

  CommodRepository commod_repo(store);

  // 1. Undelivered lot: delivery flag is updated to true on first pass
  Commod lot1{};
  lot1.id = 1;
  lot1.owner = player_t{1};
  lot1.governor = governor_t{0};
  lot1.type = CommodType::FUEL;
  lot1.amount = 50;
  lot1.star_from = starnum_t{0};
  lot1.planet_from = planetnum_t{0};
  lot1.star_to = starnum_t{1};
  lot1.planet_to = planetnum_t{0};
  lot1.bidder = player_t{2};
  lot1.bidder_gov = governor_t{0};
  lot1.bid = 200;
  lot1.deliver = false;
  commod_repo.save(lot1);

  process_market_transactions(em);
  const auto* lot1_after = em.peek_commod(1);
  test::expect_ne(lot1_after, nullptr);
  test::expect_true(lot1_after->deliver);

  // 2. Insufficient buyer funds: bid exceeds money -> bid is cleared
  lot1_after = nullptr;
  em.mutate_commod(1, [](Commod& c) {
    c.bid = 5000;  // Bidder only has 1000
  });

  process_market_transactions(em);
  const auto* lot1_cleared = em.peek_commod(1);
  test::expect_ne(lot1_cleared, nullptr);
  test::expect_eq(lot1_cleared->bid, 0);
  test::expect_eq(lot1_cleared->bidder, player_t{0});

  // 3. Successful transaction: Valid bid executed, money transferred, lot
  // deleted
  em.mutate_commod(1, [](Commod& c) {
    c.bidder = player_t{2};
    c.bidder_gov = governor_t{0};
    c.bid = 300;
  });

  process_market_transactions(em);

  // Lot deleted
  test::expect_throws<EntityNotFoundError>([&]() { em.peek_commod(1); });

  // Seller received payment
  const auto* seller = em.peek_race(player_t{1});
  test::expect_eq(seller->governor[0].money, 800);  // 500 + 300

  // Buyer charged bid + freight, and received fuel on destination planet
  const auto* buyer = em.peek_race(player_t{2});
  test::expect_lt(buyer->governor[0].money, 700);  // 1000 - 300 - shipping_cost
  const auto& dest_planet = *em.peek_planet(starnum_t{1}, planetnum_t{0});
  test::expect_eq(dest_planet.info(player_t{2}).fuel, 50);
}

void test_compute_governed_status() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  // Case 1: No Gov_ship
  race.Gov_ship = 0;
  test::expect_false(compute_governed_status(race, em));

  // Case 2: Ship exists but dead or undocked
  Ship gov_ship{};
  gov_ship.number() = 1;
  gov_ship.owner() = player_t{1};
  gov_ship.alive() = false;
  gov_ship.docked() = false;
  ShipRepository ship_repo(store);
  ship_repo.save(gov_ship);
  race.Gov_ship = 1;
  test::expect_false(compute_governed_status(race, em));

  // Case 4: Ship alive and docked at planet
  em.mutate_ship(1, [](Ship& s) {
    s.alive() = true;
    s.docked() = true;
    s.whatdest() = ScopeLevel::LEVEL_PLAN;
  });
  test::expect_true(compute_governed_status(race, em));

  // Case 5: Ship docked at habitat orbiting planet or star
  Ship habitat{};
  habitat.number() = 2;
  habitat.owner() = player_t{1};
  habitat.alive() = true;
  habitat.type() = ShipType::STYPE_HABITAT;
  habitat.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship_repo.save(habitat);

  em.mutate_ship(1, [](Ship& s) {
    s.whatdest() = ScopeLevel::LEVEL_SHIP;
    s.whatorbits() = ScopeLevel::LEVEL_SHIP;
    s.destshipno() = 2;
  });
  test::expect_true(compute_governed_status(race, em));
}

void test_action_points_computation_and_distribution() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  race.planet_points = 50;

  // 1. Ungoverned race: APs reduced by 20x
  ap_t ungoverned_ap = compute_star_action_points(10, 10000, race, em);
  test::expect_ge(ungoverned_ap, 0);

  // 2. Setup governed ship
  Ship gov_ship{};
  gov_ship.number() = 1;
  gov_ship.owner() = player_t{1};
  gov_ship.alive() = true;
  gov_ship.docked() = true;
  gov_ship.whatdest() = ScopeLevel::LEVEL_PLAN;
  ShipRepository ship_repo(store);
  ship_repo.save(gov_ship);
  race.Gov_ship = 1;

  ap_t governed_ap = compute_star_action_points(10, 10000, race, em);
  test::expect_gt(governed_ap, ungoverned_ap);

  // 3. Universe Action Point Distribution
  universe_struct u{};
  u.id = 1;
  u.numstars = 1;
  u.AP[player_t{1}] = 100;
  UniverseRepository univ_repo(store);
  univ_repo.save(u);

  RaceRepository race_repo(store);
  race_repo.save(race);

  distribute_universe_action_points(em);

  const auto* u_after = em.peek_universe();
  test::expect_ne(u_after, nullptr);
  test::expect_eq(u_after->AP[player_t{1}], 150);  // 100 + 50
}

void test_output_ground_attacks() {
  seed_rand(42);
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race1 = createTestRace(player_t{1});
  Race race2 = createTestRace(player_t{2});
  RaceRepository race_repo(store);
  race_repo.save(race1);
  race_repo.save(race2);

  Star star = createTestStar(starnum_t{0});
  StarRepository star_repo(store);
  star_repo.save(star);

  universe_struct u{};
  u.id = 1;
  u.numstars = 1;
  UniverseRepository univ_repo(store);
  univ_repo.save(u);

  ground_assaults[player_t{1}][player_t{2}][starnum_t{0}] = 3;

  output_ground_attacks(em);

  test::expect_eq(ground_assaults[player_t{1}][player_t{2}][starnum_t{0}], 0U);
}

void test_race_turn_accounting_and_maintenance() {
  Race race = createTestRace(player_t{1});
  race.controlled_planets = 5;
  race.planet_points = 20;
  race.governor[0].active = true;
  race.governor[0].maintain = 100;
  race.governor[0].income = 50;
  race.governor[0].cost_market = 30;
  race.governor[0].profit_market = 40;
  race.governor[0].cost_tech = 10;
  race.governor[0].money = 500;
  race.morale = 80;

  // 1. Reset turn accounting
  race.reset_turn_accounting();
  test::expect_eq(race.controlled_planets, 0);
  test::expect_eq(race.planet_points, 0);
  test::expect_eq(race.governor[0].maintain, 0);
  test::expect_eq(race.governor[0].income, 0UL);
  test::expect_eq(race.governor[0].cost_market, 0UL);
  test::expect_eq(race.governor[0].profit_market, 0UL);
  test::expect_eq(race.governor[0].cost_tech, 0UL);

  // 2. Deduct maintenance with sufficient funds
  race.governor[0].money = 500;
  race.deduct_maintenance(governor_t{0}, 200);
  test::expect_eq(race.governor[0].money, 300);
  test::expect_eq(race.morale, 80);

  // 3. Deduct maintenance with deficit: deducts remaining money, applies morale
  // penalty clamped to [0, 100]
  race.deduct_maintenance(governor_t{0},
                          500);  // Deficit of 200 -> penalty of 20
  test::expect_eq(race.governor[0].money, 0);
  test::expect_eq(race.morale, 60);

  // Deficit clamping: large deficit clamps morale to 0
  race.deduct_maintenance(governor_t{0},
                          10000);  // Deficit of 10000 -> morale 0
  test::expect_eq(race.morale, 0);

  // 4. Collective Intelligence IQ scaling
  race.collective_iq = true;
  race.IQ_limit = 200;
  race.update_collective_intelligence(50000);
  test::expect_gt(race.IQ, 0);
  test::expect_le(race.IQ, race.IQ_limit);
}

void test_update_von_neumann_target() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race1 = createTestRace(player_t{1});
  Race race2 = createTestRace(player_t{2});
  RaceRepository race_repo(store);
  race_repo.save(race1);
  race_repo.save(race2);

  universe_struct u{};
  u.id = 1;
  u.numstars = 1;
  u.VN_hitlist[player_t{1}] = 10;
  u.VN_hitlist[player_t{2}] = 25;
  UniverseRepository univ_repo(store);
  univ_repo.save(u);

  TurnStats stats{};
  update_von_neumann_target(em, stats);

  test::expect_eq(stats.VN_brain.total_mad, 35);
  test::expect_eq(stats.VN_brain.most_mad, player_t{2});
}

void test_check_technological_discoveries() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race = createTestRace(player_t{1});
  race.tech = 160.0;  // Qualifies for HYPERDRIVE (50), LASER (100), CEW (150)
  RaceRepository race_repo(store);
  race_repo.save(race);

  check_technological_discoveries(em, race);

  test::expect_true(race.discoveries.hyperdrive);
  test::expect_true(race.discoveries.laser);
  test::expect_true(race.discoveries.cew);
  test::expect_true(race.discoveries.vn);
  test::expect_true(race.discoveries.crystal);
  test::expect_false(race.discoveries.avpm);          // TECH_AVPM = 250
  test::expect_false(race.discoveries.tractor_beam);  // TECH_TRACTOR_BEAM = 999
}

void test_calculate_victory_scores_isolated() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race1 = createTestRace(player_t{1});
  race1.morale = 100;
  race1.governor[0].money = 1000;
  RaceRepository race_repo(store);
  race_repo.save(race1);

  universe_struct u{};
  u.id = 1;
  u.numstars = 1;
  UniverseRepository univ_repo(store);
  univ_repo.save(u);

  Star star = createTestStar(starnum_t{0});
  StarRepository star_repo(store);
  star_repo.save(star);

  Planet planet = createTestPlanet(starnum_t{0}, planetnum_t{0});
  planet.info(player_t{1}).explored = true;
  planet.info(player_t{1}).numsectsowned = 10;
  planet.info(player_t{1}).resource = 100;
  planet.info(player_t{1}).fuel = 50;
  PlanetRepository planet_repo(store);
  planet_repo.save(planet);

  calculate_victory_scores(em);

  const auto* race_after = em.peek_race(player_t{1});
  test::expect_ne(race_after, nullptr);
  test::expect_gt(race_after->victory_score, 0UL);
}

void test_do_update_voting_reset_and_scheduling() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race1 = createTestRace(player_t{1});
  race1.votes = true;  // Voted 'go'
  Race race2 = createTestRace(player_t{2});
  race2.votes = true;  // Voted 'go'
  RaceRepository race_repo(store);
  race_repo.save(race1);
  race_repo.save(race2);

  universe_struct u{};
  u.id = 1;
  u.numstars = 1;
  UniverseRepository univ_repo(store);
  univ_repo.save(u);

  ServerState sstate{};
  sstate.id = 1;
  sstate.segments = 1;
  sstate.update_time_minutes = 60;
  ServerStateRepository state_repo(store);
  state_repo.save(sstate);

  auto& registry = get_test_session_registry();

  // Execute full turn update
  do_update(em, registry, true);

  // Verify that all races have their votes reset to false (wait) and persisted
  const auto* r1 = em.peek_race(player_t{1});
  const auto* r2 = em.peek_race(player_t{2});
  test::expect_ne(r1, nullptr);
  test::expect_ne(r2, nullptr);
  test::expect_false(r1->votes);
  test::expect_false(r2->votes);

  // Verify ScheduleInfo updated
  const auto& sched = get_schedule_info();
  test::expect_gt(sched.nupdates_done, 0U);
  test::expect_false(sched.update_buf.empty());
}

void test_handle_victory_disabled() {
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_race(player_t{1}, [](Race& race) {
    race.victory_turns = VICTORY_UPDATES + 1;
  });

  // Victory disabled (false) -> no game over, empty result, no telegrams
  auto result = handle_victory(ctx.em, false);
  test::expect_false(result.game_over);
  test::expect_true(result.big_winners.empty());
  test::expect_true(result.lesser_winners.empty());
  test::expect_false(ctx.em.has_telegrams(player_t{1}, governor_t{0}));
}

void test_handle_victory_single_winner() {
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_race(player_t{1}, [](Race& race) {
    race.name = "GloriousEmpire";
    race.victory_turns = VICTORY_UPDATES;
    race.governor[0].active = true;
  });
  ctx.em.mutate_race(player_t{2}, [](Race& race) {
    race.name = "OtherEmpire";
    race.victory_turns = 0;
    race.governor[0].active = true;
  });

  auto result = handle_victory(ctx.em, true);
  test::expect_true(result.game_over);
  test::expect_eq(result.big_winners.size(), 1U);
  test::expect_eq(result.big_winners[0], player_t{1});
  test::expect_true(result.lesser_winners.empty());

  // Both players receive victory broadcast telegrams
  test::expect_true(ctx.em.has_telegrams(player_t{1}, governor_t{0}));
  test::expect_true(ctx.em.has_telegrams(player_t{2}, governor_t{0}));

  auto tele1 = ctx.em.get_telegrams(player_t{1}, governor_t{0});
  bool found_announcement = false;
  bool found_winner = false;
  for (const auto& t : tele1) {
    if (t.message.contains("This game of Galactic Bloodshed is now *over*")) {
      found_announcement = true;
    }
    if (t.message.contains("The big winner is")) {
      found_winner = true;
    }
  }
  test::expect_true(found_announcement);
  test::expect_true(found_winner);
}

void test_handle_victory_multiple_winners_and_lesser_winners() {
  TestContext ctx;
  ctx.with_standard_universe();

  // with_standard_universe provisions 2 planets (Earth, Vega Prime).
  // VICTORY_PERCENT is 10, so std::max(1, 2 * 10 / 100) = 1 planet threshold
  // for lesser winner. Player 1 & 2 are big winners (victory_turns >=
  // VICTORY_UPDATES)
  ctx.em.mutate_race(player_t{1}, [](Race& race) {
    race.name = "EmpireAlpha";
    race.victory_turns = VICTORY_UPDATES;
    race.governor[0].active = true;
  });
  ctx.em.mutate_race(player_t{2}, [](Race& race) {
    race.name = "EmpireBeta";
    race.victory_turns = VICTORY_UPDATES;
    race.governor[0].active = true;
  });

  // Player 3 is lesser winner (controlled_planets >= 1, but victory_turns <
  // VICTORY_UPDATES)
  JsonStore store(ctx.db);
  Race race3 = createTestRace(player_t{3});
  race3.name = "EmpireGamma";
  race3.controlled_planets = 1;
  race3.victory_turns = 1;
  race3.governor[0].active = true;
  RaceRepository(store).save(race3);

  auto result = handle_victory(ctx.em, true);
  test::expect_true(result.game_over);
  test::expect_eq(result.big_winners.size(), 2U);
  test::expect_eq(result.big_winners[0], player_t{1});
  test::expect_eq(result.big_winners[1], player_t{2});
  test::expect_eq(result.lesser_winners.size(), 1U);
  test::expect_eq(result.lesser_winners[0], player_t{3});

  auto tele3 = ctx.em.get_telegrams(player_t{3}, governor_t{0});
  bool found_plural_winners = false;
  bool found_lesser_winner = false;
  for (const auto& t : tele3) {
    if (t.message.contains("The big winners are")) {
      found_plural_winners = true;
    }
    if (t.message.contains("EmpireGamma")) {
      found_lesser_winner = true;
    }
  }
  test::expect_true(found_plural_winners);
  test::expect_true(found_lesser_winner);
}

void test_calculate_victory_scores_large_accumulation() {
  TestContext ctx;
  ctx.with_standard_universe();

  // Test 64-bit integer overflow protection:
  // Accumulate huge money and resources that exceed 32-bit INT_MAX
  // (2,147,483,647)
  ctx.em.mutate_race(player_t{1}, [](Race& race) {
    race.morale = 100;
    // 3 billion in treasury across governors
    race.governor[0].money = 3'000'000'000LL;
  });

  ctx.em.mutate_planet(starnum_t{0}, planetnum_t{0}, [](Planet& planet) {
    planet.info(player_t{1}).explored = true;
    planet.info(player_t{1}).numsectsowned = 500;
    // 3 billion resources on planet
    planet.info(player_t{1}).resource = 3'000'000'000LL;
    planet.info(player_t{1}).destruct = 500'000'000LL;
    planet.info(player_t{1}).fuel = 100'000'000;
  });

  calculate_victory_scores(ctx.em);

  const auto* race_after = ctx.em.peek_race(player_t{1});
  test::expect_ne(race_after, nullptr);
  // (VICT_RES * (3B + 0.5B) + VICT_MONEY * 3B + ...) / VICT_DIVISOR
  // > 0 and no negative integer overflow
  test::expect_gt(race_after->victory_score, 0LL);
  // Specifically: 3.5B res + 3B money = 6.5B raw, divided by 10000 = ~650,000
  // victory score
  test::expect_ge(race_after->victory_score, 600'000LL);
}

void test_schedule_calculation_pure() {
  ServerState state{};
  state.segments = 4;
  state.update_time_minutes =
      60;  // 3600 seconds total -> 900 seconds per segment
  state.next_update_time = 10'000;
  state.next_segment_time = 9'100;
  state.nsegments_done = 2;

  // 1. Normal Update Schedule (force = false)
  // next_update_time becomes 10000 + 3600 = 13600
  // next_segment_time becomes next_update_time (10000) + 3600 / 4 = 10900
  // nsegments_done becomes 1
  auto upd =
      compute_update_schedule(state, /*current_time=*/9'900, /*force=*/false);
  test::expect_eq(upd.next_update_time, 13'600);
  test::expect_eq(upd.next_segment_time, 10'900);
  test::expect_eq(upd.nsegments_done, 1U);

  // 2. Forced Update Schedule (force = true)
  // based on current_time = 9900
  // next_update_time becomes 9900 + 3600 = 13500
  // next_segment_time becomes 9900 + 900 = 10800
  // nsegments_done becomes 1
  auto forced_upd =
      compute_update_schedule(state, /*current_time=*/9'900, /*force=*/true);
  test::expect_eq(forced_upd.next_update_time, 13'500);
  test::expect_eq(forced_upd.next_segment_time, 10'800);
  test::expect_eq(forced_upd.nsegments_done, 1U);

  // 3. Single-Segment Game (segments = 1) -> movement segments disabled
  ServerState single_seg_state{};
  single_seg_state.segments = 1;
  single_seg_state.update_time_minutes = 60;
  auto single_upd = compute_update_schedule(
      single_seg_state, /*current_time=*/10'000, /*force=*/true);
  test::expect_eq(single_upd.next_segment_time, 10'000 + (144 * 3600));
  test::expect_eq(single_upd.nsegments_done, 1U);

  // 4. Normal Segment Cadence (override = false)
  // next_segment_time advances by 3600 / 4 = 900: 9100 + 900 = 10000
  // nsegments_done increments from 2 to 3
  // next_update_time preserved as 10000
  auto seg = compute_segment_schedule(state, /*current_time=*/9'200,
                                      /*override=*/false);
  test::expect_eq(seg.next_segment_time, 10'000);
  test::expect_eq(seg.nsegments_done, 3U);
  test::expect_eq(seg.next_update_time, 10'000);

  // 5. Override Segment with specific segment (override = true, target_segment
  // = 2) next_segment_time becomes current_time + 900 = 9200 + 900 = 10100
  // nsegments_done becomes 2
  // next_update_time = current_time + (3600 * (4 - 2 + 1)) / 4 = 9200 + 2700 =
  // 11900
  auto override_seg = compute_segment_schedule(
      state, /*current_time=*/9'200, /*override=*/true, /*target_segment=*/2);
  test::expect_eq(override_seg.next_segment_time, 10'100);
  test::expect_eq(override_seg.nsegments_done, 2U);
  test::expect_eq(override_seg.next_update_time, 11'900);

  // 6. format_server_start_time
  std::time_t t = 1'700'000'000;
  std::string formatted = format_server_start_time(t);
  test::expect_contains(formatted, "Server started  : ");
}

void test_do_segment_execution() {
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_server_state([](ServerState& s) {
    s.segments = 3;
    s.update_time_minutes = 30;  // 1800s / 3 = 600s per segment
    s.nsegments_done = 1;
    s.next_segment_time = 1'000'000;
    s.next_update_time = 1'001'200;
  });

  RecordingSessionRegistry reg;

  // Execute normal segment
  do_segment(ctx.em, reg, 0, 0);

  const auto* state = ctx.em.peek_server_state();
  test::expect_ne(state, nullptr);
  test::expect_eq(state->nsegments_done, 2U);
  test::expect_eq(state->next_segment_time, 1'000'600);

  // Notifications broadcast
  test::expect_true(reg.has_broadcast("DOING MOVEMENT"));
  test::expect_true(reg.has_broadcast("Segment finished"));

  // If segments <= 1 and no override, do_segment returns immediately without
  // running
  ctx.em.mutate_server_state([](ServerState& s) {
    s.segments = 1;
    s.nsegments_done = 1;
  });
  reg.clear_notifications();
  do_segment(ctx.em, reg, 0, 0);
  test::expect_false(reg.has_broadcast("DOING MOVEMENT"));
}

void test_do_next_thing_dispatch() {
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_server_state([](ServerState& s) {
    s.segments = 3;
    s.update_time_minutes = 30;
    s.nsegments_done = 1;
    s.next_segment_time = 1'000'000;
    s.next_update_time = 1'001'200;
  });

  RecordingSessionRegistry reg;

  // 1. When nsegments_done (1) < segments (3), do_next_thing dispatches
  // do_segment()
  do_next_thing(ctx.em, reg);
  const auto* state1 = ctx.em.peek_server_state();
  test::expect_eq(state1->nsegments_done, 2U);
  test::expect_true(reg.has_broadcast("DOING MOVEMENT"));

  // Advance to final segment
  ctx.em.mutate_server_state([](ServerState& s) { s.nsegments_done = 3; });
  reg.clear_notifications();

  const unsigned int updates_before = get_schedule_info().nupdates_done;

  // 2. When nsegments_done (3) >= segments (3), do_next_thing dispatches
  // do_update()
  do_next_thing(ctx.em, reg);
  const auto* state2 = ctx.em.peek_server_state();
  test::expect_eq(state2->nsegments_done, 1U);
  test::expect_true(reg.has_broadcast("DOING UPDATE"));
  test::expect_true(reg.has_broadcast("Update"));
  test::expect_eq(get_schedule_info().nupdates_done, updates_before + 1);
}

void test_advance_race_technology() {
  TestContext ctx;
  ctx.with_standard_universe();

  TurnStats stats{};
  stats.Power[player_t{1}].popn = 10'000;
  stats.Power[player_t{1}].planets_owned = 2;

  ctx.em.mutate_race(player_t{1}, [](Race& r) {
    r.IQ = 100;
    r.tech = 49.5;
    r.morale = 10;
    r.turn = 5;
    r.governor[0].active = true;
    r.governor[0].maintain = 100;
    r.governor[0].money = 1000;
  });

  auto r_handle = ctx.em.peek_race(player_t{1});
  Race test_race = *r_handle;

  advance_race_technology(test_race, stats, ctx.em);

  test::expect_gt(test_race.IQ, 0);
  test::expect_gt(test_race.tech, 49.0);
  test::expect_eq(test_race.morale, 12);
  test::expect_eq(test_race.turn, 6);
  test::expect_true(test_race.discoveries.hyperdrive);
  test::expect_eq(test_race.governor[0].money, 900);
}

void test_update_victory_progress() {
  Race r{};
  r.Playernum = 1;
  r.victory_turns = 2;

  // 1. Zero controlled planets -> victory_turns reset to 0
  r.controlled_planets = 0;
  update_victory_progress(r, /*planet_count=*/10);
  test::expect_eq(r.victory_turns, 0);

  // 2. Below threshold (10 planets * 10% = 1 planet threshold, race has 0)
  r.victory_turns = 1;
  r.controlled_planets = 0;
  update_victory_progress(r, /*planet_count=*/10);
  test::expect_eq(r.victory_turns, 0);

  // 3. At or above threshold (10 planets * 10% = 1 planet, race controls 1)
  r.controlled_planets = 1;
  update_victory_progress(r, /*planet_count=*/10);
  test::expect_eq(r.victory_turns, 1);

  // Increments on consecutive turns
  update_victory_progress(r, /*planet_count=*/10);
  test::expect_eq(r.victory_turns, 2);

  // 4. Threshold scaling with 100 planets -> 10 planets required
  r.controlled_planets = 5;
  update_victory_progress(r, /*planet_count=*/100);
  test::expect_eq(r.victory_turns, 0);
}

void test_check_language_translation_unlock() {
  TestContext ctx;
  ctx.with_standard_universe();

  // Reset player 2's translation of player 1 to 0
  ctx.em.mutate_race(player_t{2},
                     [](Race& r) { r.translate[player_t{1}] = 0; });

  // 1. Zero controlled planets: no unlock
  bool unlocked0 = check_language_translation_unlock(
      player_t{1}, /*controlled_planets=*/0, /*planet_count=*/20, ctx.em);
  test::expect_false(unlocked0);
  test::expect_eq(ctx.em.peek_race(player_t{2})->translate[player_t{1}], 0);

  // 2. Below threshold (20 planets * 10% / 2 = 1 planet threshold, player
  // controls 0)
  bool unlocked_below = check_language_translation_unlock(
      player_t{1}, /*controlled_planets=*/0, /*planet_count=*/20, ctx.em);
  test::expect_false(unlocked_below);
  test::expect_eq(ctx.em.peek_race(player_t{2})->translate[player_t{1}], 0);

  // 3. At or above threshold (20 planets * 10% / 2 = 1 planet, player controls
  // 1)
  bool unlocked = check_language_translation_unlock(
      player_t{1}, /*controlled_planets=*/1, /*planet_count=*/20, ctx.em);
  test::expect_true(unlocked);
  test::expect_eq(ctx.em.peek_race(player_t{2})->translate[player_t{1}], 100);
}

void test_sync_power_ratings() {
  TestContext ctx;
  ctx.with_standard_universe();

  JsonStore store(ctx.db);
  PowerRepository power_repo(store);
  power p1{};
  p1.id = 1;
  power_repo.save(p1);

  TurnStats stats{};
  stats.Power[player_t{1}].popn = 5000;
  stats.Power[player_t{1}].planets_owned = 1;

  ctx.em.mutate_race(player_t{1}, [](Race& r) {
    r.governor[0].active = true;
    r.governor[0].money = 12'345;
  });

  sync_power_ratings(ctx.em, stats);

  // Verified aggregated money in stats
  test::expect_eq(stats.Power[player_t{1}].money, 12'345);

  // Verified persisted power record
  const auto* power = ctx.em.peek_power(powernum_t{1});
  test::expect_ne(power, nullptr);
  test::expect_eq(power->money, 12'345);
  test::expect_eq(power->popn, 5000);
}

void test_finalize_turn_update_integration() {
  TestContext ctx;
  ctx.with_standard_universe();

  JsonStore store(ctx.db);
  PowerRepository power_repo(store);
  power p1{};
  p1.id = 1;
  power_repo.save(p1);

  TurnStats stats{};
  stats.Power[player_t{1}].popn = 1000;
  stats.Power[player_t{1}].planets_owned = 1;

  ctx.em.mutate_race(player_t{1}, [](Race& r) {
    r.IQ = 100;
    r.controlled_planets = 1;
    r.tech = 49.5;
  });

  finalize_turn_update(ctx.em, stats);

  const auto* r1 = ctx.em.peek_race(player_t{1});
  test::expect_gt(r1->tech, 49.5);
  test::expect_true(r1->discoveries.hyperdrive);
  test::expect_ge(r1->victory_turns, 1);

  // Other race translation unlocked at 50% threshold
  const auto* r2 = ctx.em.peek_race(player_t{2});
  test::expect_eq(r2->translate[player_t{1}], 100);
}

}  // namespace

int main() {
  std::println(std::cout, "Running doturn unit tests...\n");

  std::println(std::cout, "  Testing advance_race_technology... ");
  test_advance_race_technology();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing update_victory_progress... ");
  test_update_victory_progress();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing check_language_translation_unlock... ");
  test_check_language_translation_unlock();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing sync_power_ratings... ");
  test_sync_power_ratings();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing finalize_turn_update integration... ");
  test_finalize_turn_update_integration();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing schedule calculation pure... ");
  test_schedule_calculation_pure();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing do_segment execution... ");
  test_do_segment_execution();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing do_next_thing dispatch... ");
  test_do_next_thing_dispatch();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing handle_victory disabled... ");
  test_handle_victory_disabled();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing handle_victory single winner... ");
  test_handle_victory_single_winner();
  std::println(std::cout, "PASS");

  std::println(
      std::cout,
      "  Testing handle_victory multiple winners and lesser winners... ");
  test_handle_victory_multiple_winners_and_lesser_winners();
  std::println(std::cout, "PASS");

  std::println(std::cout,
               "  Testing calculate_victory_scores large accumulation... ");
  test_calculate_victory_scores_large_accumulation();
  std::println(std::cout, "PASS");

  std::println(std::cout,
               "  Testing do_update voting reset and scheduling... ");
  test_do_update_voting_reset_and_scheduling();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing planet deposit_commodity... ");
  test_planet_deposit_commodity();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing process_market_transactions isolated... ");
  test_process_market_transactions_isolated();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing compute_governed_status... ");
  test_compute_governed_status();
  std::println(std::cout, "PASS");

  std::println(std::cout,
               "  Testing action points computation and distribution... ");
  test_action_points_computation_and_distribution();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing output_ground_attacks... ");
  test_output_ground_attacks();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing race turn accounting and maintenance... ");
  test_race_turn_accounting_and_maintenance();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing update_von_neumann_target... ");
  test_update_von_neumann_target();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing check_technological_discoveries... ");
  test_check_technological_discoveries();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing calculate_victory_scores isolated... ");
  test_calculate_victory_scores_isolated();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing fix_stability... ");
  test_fix_stability();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing do_turn segment vs update... ");
  test_do_turn_segment_vs_update();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing do_turn market and maintenance... ");
  test_do_turn_market_and_maintenance();
  std::println(std::cout, "PASS");

  std::println(std::cout,
               "  Testing do_turn victory scores and discoveries... ");
  test_do_turn_victory_scores_and_discoveries();
  std::println(std::cout, "PASS");

  std::println(std::cout, "  Testing do_turn victory scores with derelict and "
                          "multiple players... ");
  test_do_turn_victory_scores_with_derelict_and_multiple_players();
  std::println(std::cout, "PASS");

  std::println(std::cout, "All doturn tests passed!");
  return 0;
}
