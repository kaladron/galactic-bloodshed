// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std;

#include <cassert>

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
  star_data.pnames.push_back("TestPlanet");
  return Star(star_data);
}

Planet createTestPlanet(starnum_t star_id = 0, planetnum_t pnum = 0) {
  Planet planet(PlanetType::EARTH);
  planet.star_id() = star_id;
  planet.planet_order() = pnum;
  planet.Maxx() = 5;
  planet.Maxy() = 5;
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
  assert(star.nova_stage() == 1 || star.stability() <= 100);

  star.nova_stage() = 15;
  fix_stability(em, star);
  assert(star.nova_stage() == 0);
  assert(star.stability() == 20);
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

  SectorMap initial_smap(planet, true);
  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 5; x++) {
      auto& s = initial_smap.get(x, y);
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
  assert(race_after_segment != nullptr);
  assert(race_after_segment->turn == 1);

  // 2. Run a full update turn (update = true)
  do_turn(em, session_registry, true);

  const auto* race_after_update = em.peek_race(player_t{1});
  assert(race_after_update != nullptr);
  assert(race_after_update->turn == 2);
}

}  // namespace

int main() noexcept {
  try {
    std::cout << "Running doturn unit tests...\n";

    std::cout << "  Testing fix_stability... ";
    test_fix_stability();
    std::cout << "PASS\n";

    std::cout << "  Testing do_turn segment vs update... ";
    test_do_turn_segment_vs_update();
    std::cout << "PASS\n";

    std::cout << "All doturn tests passed!\n";
    return 0;
  } catch (const std::exception& e) {
    std::cout << "Test failed with exception: " << e.what() << "\n";
    return 1;
  } catch (...) {
    std::cout << "Test failed with unknown exception!\n";
    return 1;
  }
}
