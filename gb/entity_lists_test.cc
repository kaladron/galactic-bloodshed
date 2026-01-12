// SPDX-License-Identifier: Apache-2.0

// Test for RaceList, StarList, and PlanetList iteration helpers

import dallib;
import dallib;
import gblib;
import std.compat;

#include <cassert>

int main() {
  // Create in-memory database and initialize schema
  Database db(":memory:");
  initialize_schema(db);

  EntityManager em(db);
  JsonStore store(db);

  // Create some test races
  RaceRepository races(store);
  for (player_t i{1}; i <= 3; ++i) {
    Race race{};
    race.Playernum = i;
    race.name = std::format("TestRace{}", i.value);
    race.Guest = false;
    race.governor[0].money = i.value * 1000;
    races.save(race);
  }

  // Create universe with stars
  UniverseRepository universe_repo(store);
  universe_struct ud{};
  ud.id = 1;  // Singleton ID
  ud.numstars = 2;
  universe_repo.save(ud);

  // Create stars with planets
  StarRepository star_repo(store);
  for (starnum_t s = 0; s < 2; s++) {
    star_struct ss{};
    ss.star_id = s;
    ss.name = std::format("Star{}", s);
    // Add planet names to set numplanets
    for (planetnum_t p = 0; p <= s; p++) {
      ss.pnames.push_back(std::format("Planet{}-{}", s, p));
    }
    Star star(ss);
    star_repo.save(star);
  }

  // Create planets
  PlanetRepository planet_repo(store);
  // Star 0: 1 planet
  {
    Planet p{};
    p.star_id() = 0;
    p.planet_order() = 0;
    planet_repo.save(p);
  }
  // Star 1: 2 planets
  for (planetnum_t pn = 0; pn < 2; pn++) {
    Planet p{};
    p.star_id() = 1;
    p.planet_order() = pn;
    planet_repo.save(p);
  }

  // Clear cache to force reloading from database
  em.clear_cache();

  // ============ Test RaceList ============
  std::println("Testing RaceList...");
  {
    int count = 0;
    std::vector<player_t> seen_players;

    for (player_t i{1}; i <= em.num_races(); ++i) {
      auto race_handle = em.get_race(i);
      if (!race_handle.get()) continue;

      count++;
      seen_players.push_back(race_handle->Playernum);

      // Verify the race knows its own player number
      assert(race_handle->Playernum == i);
      assert(race_handle->governor[0].money == i.value * 1000);
    }

    assert(count == 3);
    assert(seen_players.size() == 3);
    assert(seen_players[0] == player_t{1});
    assert(seen_players[1] == player_t{2});
    assert(seen_players[2] == player_t{3});
    std::println("  RaceList: iterated {} races, all have correct Playernum",
                 count);
  }

  // ============ Test StarList ============
  std::println("Testing StarList...");
  {
    int count = 0;
    std::vector<starnum_t> seen_stars;

    const auto* universe = em.peek_universe();
    assert(universe != nullptr);

    for (starnum_t s = 0; s < universe->numstars; s++) {
      auto star_handle = em.get_star(s);
      if (!star_handle.get()) continue;

      count++;
      // Get underlying struct to access star_id
      seen_stars.push_back(star_handle->get_struct().star_id);

      // Verify the star knows its own ID
      assert(star_handle->get_struct().star_id == s);
    }

    assert(count == 2);
    assert(seen_stars.size() == 2);
    assert(seen_stars[0] == 0);
    assert(seen_stars[1] == 1);
    std::println("  StarList: iterated {} stars, all have correct star_id",
                 count);
  }

  // ============ Test PlanetList ============
  std::println("Testing PlanetList...");
  {
    int total_planets = 0;

    const auto* universe = em.peek_universe();
    for (starnum_t s = 0; s < universe->numstars; s++) {
      const auto* star = em.peek_star(s);
      if (!star) continue;

      int star_planet_count = 0;
      for (planetnum_t p = 0; p < star->numplanets(); p++) {
        auto planet_handle = em.get_planet(s, p);
        if (!planet_handle.get()) continue;

        star_planet_count++;
        total_planets++;

        // Verify the planet knows its own order
        assert(planet_handle->planet_order() == p);
      }

      // Star 0 should have 1 planet, Star 1 should have 2
      assert(star_planet_count == static_cast<int>(s + 1));
    }

    assert(total_planets == 3);  // 1 + 2 = 3 total planets
    std::println("  PlanetList: iterated {} total planets across all stars",
                 total_planets);
  }

  // ============ Test using Playernum for array indexing ============
  std::println("Testing array indexing via Playernum...");
  {
    // Simulate what doturncmd.cc does with state.Power[i-1]
    std::array<int, 3> power_values{};

    for (player_t i{1}; i <= em.num_races(); ++i) {
      auto race_handle = em.get_race(i);
      if (!race_handle.get()) continue;

      // Use Playernum for array indexing instead of loop variable
      power_values[race_handle->Playernum.value - 1] =
          race_handle->governor[0].money;
    }

    assert(power_values[0] == 1000);
    assert(power_values[1] == 2000);
    assert(power_values[2] == 3000);
    std::println("  Array indexing via Playernum works correctly");
  }

  // ============ Test ShipList with const vs mutable iteration ============
  std::println("Testing ShipList iteration patterns...");

  // Create some test ships
  ShipRepository ship_repo(store);
  for (shipnum_t i = 1; i <= 3; i++) {
    Ship ship{};
    ship.number() = i;
    ship.name() = std::format("Ship{}", i);
    ship.owner() = 1;
    ship.alive() = true;
    ship.fuel() = 100.0 * i;
    // Set up linked list: ship 1 -> ship 2 -> ship 3 -> 0
    ship.nextship() = (i < 3) ? i + 1 : 0;
    ship_repo.save(ship);
  }

  // Clear cache to force reload
  em.clear_cache();

  // Test 1: Const ShipList - should use ConstIterator returning const Ship*
  // Pattern: const ShipList shiplist{em, start}; for (const Ship* s : shiplist)
  // Note: Inline ShipList{em, start} is a non-const rvalue, so must use named
  // const
  std::println("  Testing const ShipList (read-only)...");
  {
    int count = 0;
    double total_fuel = 0.0;

    const ShipList shiplist{em, 1};
    for (const Ship* ship : shiplist) {
      // This should be a const Ship* - compile-time verification
      static_assert(std::is_same_v<decltype(ship), const Ship*>,
                    "ConstIterator should return const Ship*");
      count++;
      total_fuel += ship->fuel();
      // Cannot modify: ship->fuel() = 0;  // Would fail to compile
    }

    assert(count == 3);
    assert(total_fuel == 100.0 + 200.0 + 300.0);
    std::println("    Const iteration: {} ships, total fuel = {}", count,
                 total_fuel);
  }

  // Test 2: Mutable ShipList - should use MutableIterator returning ShipHandle
  // PREFERRED PATTERN: for (auto ship : ShipList{em, start}) - inline works!
  std::println("  Testing mutable ShipList (with modifications)...");
  {
    int count = 0;

    for (auto ship : ShipList{em, 1}) {
      // This should be a ShipHandle - compile-time verification
      static_assert(std::is_same_v<decltype(ship), ShipHandle>,
                    "MutableIterator should return ShipHandle");

      // Use -> operator directly (preferred pattern)
      ship->fuel() += 50.0;
      count++;
    }

    assert(count == 3);
    std::println("    Mutable iteration: modified {} ships", count);
  }

  // Verify modifications were saved
  em.clear_cache();
  {
    const Ship* s1 = em.peek_ship(1);
    const Ship* s2 = em.peek_ship(2);
    const Ship* s3 = em.peek_ship(3);
    assert(s1 && s1->fuel() == 150.0);  // 100 + 50
    assert(s2 && s2->fuel() == 250.0);  // 200 + 50
    assert(s3 && s3->fuel() == 350.0);  // 300 + 50
    std::println("    Verified modifications were auto-saved");
  }

  // Test 3: Alternative mutable pattern with dereference
  std::println("  Testing mutable ShipList with dereference pattern...");
  {
    ShipList shiplist(em, 1);

    for (auto ship_handle : shiplist) {
      // Alternative pattern: dereference to get Ship&
      Ship& s = *ship_handle;
      s.fuel() += 25.0;
    }
  }

  // Verify second round of modifications
  em.clear_cache();
  {
    const Ship* s1 = em.peek_ship(1);
    assert(s1 && s1->fuel() == 175.0);  // 150 + 25
    std::println("    Verified dereference pattern modifications");
  }

  std::println("All entity list tests passed!");
  return 0;
}
