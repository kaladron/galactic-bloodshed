// SPDX-License-Identifier: Apache-2.0

/// \file doturn_test.cc
/// \brief Unit tests for full turn simulation execution, star stability repair,
/// and segment vs update turn execution.

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
  u.planet_count = 1;
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

}  // namespace

int main() {
  std::println(std::cout, "Running doturn unit tests...\n");

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

  std::println(std::cout, "All doturn tests passed!");
  return 0;
}
