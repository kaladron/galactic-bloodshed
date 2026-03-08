// SPDX-License-Identifier: Apache-2.0

// Test for RaceList, StarList, and PlanetList iteration helpers

import dallib;
import gblib;
import std.compat;

#include <cassert>

namespace {

void populate_base_entities(EntityManager& em, JsonStore& store) {
  RaceRepository races(store);
  for (player_t i{1}; i <= 3; ++i) {
    Race race{};
    race.Playernum = i;
    race.name = std::format("TestRace{}", i.value);
    race.Guest = false;
    race.governor[0].money = static_cast<money_t>(i.value) * 1000L;
    races.save(race);
  }

  UniverseRepository universe_repo(store);
  universe_struct ud{};
  ud.id = 1;
  ud.numstars = 2;
  universe_repo.save(ud);

  StarRepository star_repo(store);
  for (starnum_t s = 0; s < 2; s++) {
    star_struct ss{};
    ss.star_id = s;
    ss.name = std::format("Star{}", s);
    for (planetnum_t p = 0; p <= s; p++) {
      ss.pnames.push_back(std::format("Planet{}-{}", s, p));
    }
    Star star(ss);
    star_repo.save(star);
  }

  PlanetRepository planet_repo(store);
  {
    Planet p{};
    p.star_id() = 0;
    p.planet_order() = 0;
    planet_repo.save(p);
  }
  for (planetnum_t pn = 0; pn < 2; pn++) {
    Planet p{};
    p.star_id() = 1;
    p.planet_order() = pn;
    planet_repo.save(p);
  }

  CommodRepository commod_repo(store);
  {
    Commod commod{};
    commod.id = 1;
    commod.owner = 1;
    commod.governor = 0;
    commod.type = CommodType::RESOURCE;
    commod.amount = 100;
    commod_repo.save(commod);
  }
  {
    Commod commod{};
    commod.id = 2;
    commod.owner = 0;
    commod.governor = 0;
    commod.type = CommodType::DESTRUCT;
    commod.amount = 250;
    commod_repo.save(commod);
  }
  {
    Commod commod{};
    commod.id = 3;
    commod.owner = 2;
    commod.governor = 0;
    commod.type = CommodType::FUEL;
    commod.amount = 0;
    commod_repo.save(commod);
  }
  {
    Commod commod{};
    commod.id = 4;
    commod.owner = 3;
    commod.governor = 0;
    commod.type = CommodType::CRYSTAL;
    commod.amount = 400;
    commod_repo.save(commod);
  }

  em.clear_cache();
}

void test_race_list_readonly(EntityManager& em) {
  std::println("Testing RaceList...");
  int count = 0;
  std::vector<player_t> seen_players;

  auto readonly_races = RaceList::readonly(em);
  for (const Race* race : std::as_const(readonly_races)) {
    static_assert(std::is_same_v<decltype(race), const Race*>,
                  "RaceList::readonly() should yield const Race*");

    count++;
    seen_players.push_back(race->Playernum);
    assert(race->Playernum.value == count);
    assert(race->governor[0].money ==
           static_cast<money_t>(race->Playernum.value) * 1000L);
  }

  assert(count == 3);
  assert(seen_players.size() == 3);
  assert(seen_players[0] == player_t{1});
  assert(seen_players[1] == player_t{2});
  assert(seen_players[2] == player_t{3});
  std::println("  RaceList: iterated {} races, all have correct Playernum",
               count);
}

void test_star_list_readonly(EntityManager& em) {
  std::println("Testing StarList...");
  int count = 0;
  std::vector<starnum_t> seen_stars;

  auto readonly_stars = StarList::readonly(em);
  for (const Star* star : std::as_const(readonly_stars)) {
    static_assert(std::is_same_v<decltype(star), const Star*>,
                  "StarList::readonly() should yield const Star*");

    count++;
    seen_stars.push_back(star->get_struct().star_id);
    assert(star->get_struct().star_id == static_cast<starnum_t>(count - 1));
  }

  assert(count == 2);
  assert(seen_stars.size() == 2);
  assert(seen_stars[0] == 0);
  assert(seen_stars[1] == 1);
  std::println("  StarList: iterated {} stars, all have correct star_id",
               count);
}

void test_planet_list_readonly(EntityManager& em) {
  std::println("Testing PlanetList...");
  int total_planets = 0;

  for (const Star* star : StarList::readonly(em)) {
    auto star_id = star->get_struct().star_id;
    auto readonly_planets = PlanetList::readonly(em, star_id, *star);

    int star_planet_count = 0;
    for (const Planet* planet : std::as_const(readonly_planets)) {
      static_assert(std::is_same_v<decltype(planet), const Planet*>,
                    "PlanetList::readonly() should yield const Planet*");

      star_planet_count++;
      total_planets++;
      assert(planet->star_id() == star_id);
      assert(planet->planet_order() ==
             static_cast<planetnum_t>(star_planet_count - 1));
    }

    assert(star_planet_count == static_cast<int>(star_id + 1));
  }

  assert(total_planets == 3);
  std::println("  PlanetList: iterated {} total planets across all stars",
               total_planets);
}

void test_commod_list_readonly(EntityManager& em) {
  std::println("Testing CommodList...");
  int count = 0;
  uint64_t total_amount = 0;
  std::vector<int> seen_ids;

  auto readonly_commods = CommodList::readonly(em);
  for (const Commod* commod : std::as_const(readonly_commods)) {
    static_assert(std::is_same_v<decltype(commod), const Commod*>,
                  "CommodList::readonly() should yield const Commod*");

    count++;
    total_amount += commod->amount;
    seen_ids.push_back(commod->id);
    assert(commod->owner.value != 0);
    assert(commod->amount != 0);
  }

  assert(count == 2);
  assert(total_amount == 500);
  assert(seen_ids.size() == 2);
  assert(seen_ids[0] == 1);
  assert(seen_ids[1] == 4);
  std::println("  CommodList: iterated {} valid commodities", count);
}

void test_playernum_indexing(EntityManager& em) {
  std::println("Testing array indexing via Playernum...");
  std::array<int, 3> power_values{};

  for (const Race* race : RaceList::readonly(em)) {
    power_values[race->Playernum.value - 1] = race->governor[0].money;
  }

  assert(power_values[0] == 1000);
  assert(power_values[1] == 2000);
  assert(power_values[2] == 3000);
  std::println("  Array indexing via Playernum works correctly");
}

void populate_ships(EntityManager& em, JsonStore& store) {
  ShipRepository ship_repo(store);
  for (shipnum_t i = 1; i <= 3; i++) {
    Ship ship{};
    ship.number() = i;
    ship.name() = std::format("Ship{}", i);
    ship.owner() = 1;
    ship.alive() = true;
    ship.fuel() = 100.0 * i;
    ship.nextship() = (i < 3) ? i + 1 : 0;
    ship_repo.save(ship);
  }

  em.clear_cache();
}

void test_ship_list_patterns(EntityManager& em) {
  std::println("Testing ShipList iteration patterns...");

  std::println("  Testing ShipList::readonly()...");
  int count = 0;
  double total_fuel = 0.0;

  auto readonly_ships = ShipList::readonly(em, 1);
  for (const Ship* ship : std::as_const(readonly_ships)) {
    static_assert(std::is_same_v<decltype(ship), const Ship*>,
                  "ShipList::readonly() should yield const Ship*");
    count++;
    total_fuel += ship->fuel();
  }

  assert(count == 3);
  assert(total_fuel == 100.0 + 200.0 + 300.0);
  std::println("    Read-only iteration: {} ships, total fuel = {}", count,
               total_fuel);

  std::println("  Testing mutable ShipList (with modifications)...");
  count = 0;

  for (auto ship : ShipList{em, 1}) {
    static_assert(std::is_same_v<decltype(ship), ShipHandle>,
                  "MutableIterator should return ShipHandle");

    ship->fuel() += 50.0;
    count++;
  }

  assert(count == 3);
  std::println("    Mutable iteration: modified {} ships", count);

  em.clear_cache();
  {
    const Ship* s1 = em.peek_ship(1);
    const Ship* s2 = em.peek_ship(2);
    const Ship* s3 = em.peek_ship(3);
    assert(s1 && s1->fuel() == 150.0);
    assert(s2 && s2->fuel() == 250.0);
    assert(s3 && s3->fuel() == 350.0);
    std::println("    Verified modifications were auto-saved");
  }

  std::println("  Testing mutable ShipList with dereference pattern...");
  ShipList shiplist(em, 1);

  for (auto ship_handle : shiplist) {
    Ship& s = *ship_handle;
    s.fuel() += 25.0;
  }

  em.clear_cache();
  {
    const Ship* s1 = em.peek_ship(1);
    assert(s1 && s1->fuel() == 175.0);
    std::println("    Verified dereference pattern modifications");
  }
}

}  // namespace

int run_all_tests() noexcept {
  try {
    Database db(":memory:");
    initialize_schema(db);

    EntityManager em(db);
    JsonStore store(db);

    populate_base_entities(em, store);
    test_race_list_readonly(em);
    test_star_list_readonly(em);
    test_planet_list_readonly(em);
    test_commod_list_readonly(em);
    test_playernum_indexing(em);
    populate_ships(em, store);
    test_ship_list_patterns(em);

    std::println("All entity list tests passed!");
    return 0;
  } catch (const std::exception& ex) {
    std::println(std::cerr, "entity_lists_test failed: {}", ex.what());
    return 1;
  }
}

int main() noexcept {
  return run_all_tests();
}
