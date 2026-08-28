// SPDX-License-Identifier: Apache-2.0

/// \file entity_lists_test.cc
/// \brief Unit tests for RaceList, StarList, PlanetList, CommodList, and
/// ShipList iteration helpers.

import dallib;
import gblib;
import test;
import std;

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
    for (planetnum_t p = 0; p.value <= s.value; p++) {
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
  std::println(std::cout, "Testing RaceList...");
  int count = 0;
  std::vector<player_t> seen_players;

  auto readonly_races = RaceList::readonly(em);
  for (const Race& race : std::as_const(readonly_races)) {
    static_assert(std::is_same_v<decltype(race), const Race&>,
                  "RaceList::readonly() should yield const Race&");

    count++;
    seen_players.push_back(race.Playernum);
    test::expect_eq(race.Playernum.value, count);
    test::expect_eq(race.governor[0].money,
                    static_cast<money_t>(race.Playernum.value) * 1000L);
  }

  test::expect_eq(count, 3);
  test::expect_eq(seen_players.size(), 3);
  test::expect_eq(seen_players[0], player_t{1});
  test::expect_eq(seen_players[1], player_t{2});
  test::expect_eq(seen_players[2], player_t{3});
  std::println(std::cout,
               "  RaceList: iterated {} races, all have correct Playernum",
               count);
}

void test_star_list_readonly(EntityManager& em) {
  std::println(std::cout, "Testing StarList...");
  int count = 0;
  std::vector<starnum_t> seen_stars;

  for (const Star& star : StarList::readonly(em)) {
    static_assert(std::is_same_v<decltype(star), const Star&>,
                  "StarList::readonly() should yield const Star&");

    count++;
    seen_stars.push_back(star.get_struct().star_id);
    test::expect_eq(star.get_struct().star_id,
                    static_cast<starnum_t>(count - 1));
  }

  test::expect_eq(count, 2);
  test::expect_eq(seen_stars.size(), 2);
  test::expect_eq(seen_stars[0], 0);
  test::expect_eq(seen_stars[1], 1);
  std::println(std::cout,
               "  StarList: iterated {} stars, all have correct star_id",
               count);
}

void test_planet_list_readonly(EntityManager& em) {
  std::println(std::cout, "Testing PlanetList...");
  int total_planets = 0;

  for (const Star& star : StarList::readonly(em)) {
    auto star_id = star.get_struct().star_id;

    int star_planet_count = 0;
    for (const Planet& planet : PlanetList::readonly(em, star_id, star)) {
      static_assert(std::is_same_v<decltype(planet), const Planet&>,
                    "PlanetList::readonly() should yield const Planet&");

      star_planet_count++;
      total_planets++;
      test::expect_eq(planet.star_id(), star_id);
      test::expect_eq(planet.planet_order(),
                      static_cast<planetnum_t>(star_planet_count - 1));
    }

    test::expect_eq(star_planet_count, static_cast<int>(star_id.value + 1));
  }

  test::expect_eq(total_planets, 3);
  std::println(std::cout,
               "  PlanetList: iterated {} total planets across all stars",
               total_planets);
}

void test_commod_list_readonly(EntityManager& em) {
  std::println(std::cout, "Testing CommodList...");
  int count = 0;
  std::uint64_t total_amount = 0;
  std::vector<int> seen_ids;

  for (const Commod& commod : CommodList::readonly(em)) {
    static_assert(std::is_same_v<decltype(commod), const Commod&>,
                  "CommodList::readonly() should yield const Commod&");

    count++;
    total_amount += commod.amount;
    seen_ids.push_back(commod.id);
    test::expect_ne(commod.owner.value, 0);
    test::expect_ne(commod.amount, 0);
  }

  test::expect_eq(count, 2);
  test::expect_eq(total_amount, 500);
  test::expect_eq(seen_ids.size(), 2);
  test::expect_eq(seen_ids[0], 1);
  test::expect_eq(seen_ids[1], 4);
  std::println(std::cout, "  CommodList: iterated {} valid commodities", count);
}

void test_playernum_indexing(EntityManager& em) {
  std::println(std::cout, "Testing array indexing via Playernum...");
  std::array<int, 3> power_values{};

  for (const Race& race : RaceList::readonly(em)) {
    power_values[race.Playernum.value - 1] = race.governor[0].money;
  }

  test::expect_eq(power_values[0], 1000);
  test::expect_eq(power_values[1], 2000);
  test::expect_eq(power_values[2], 3000);
  std::println(std::cout, "  Array indexing via Playernum works correctly");
}

void populate_ships(EntityManager& em, JsonStore& store) {
  ShipRepository ship_repo(store);
  for (shipnum_t i = 1; i <= 3; i++) {
    Ship ship{};
    ship.number() = i;
    ship.name() = std::format("Ship{}", i);
    ship.owner() = 1;
    ship.alive() = true;
    ship.fuel() = 100.0 * static_cast<double>(i.value);
    ship.nextship() = (i.value < 3) ? shipnum_t{i.value + 1} : shipnum_t{0};
    ship_repo.save(ship);
  }

  em.clear_cache();
}

void test_ship_list_patterns(EntityManager& em) {
  std::println(std::cout, "Testing ShipList iteration patterns...");

  std::println(std::cout, "  Testing ShipList::readonly()...");
  int count = 0;
  double total_fuel = 0.0;

  for (const Ship& ship : ShipList::readonly(em, shipnum_t{1})) {
    static_assert(std::is_same_v<decltype(ship), const Ship&>,
                  "ShipList::readonly() should yield const Ship&");
    count++;
    total_fuel += ship.fuel();
  }

  test::expect_eq(count, 3);
  test::expect_eq(total_fuel, 100.0 + 200.0 + 300.0);
  std::println(std::cout, "    Read-only iteration: {} ships, total fuel = {}",
               count, total_fuel);

  std::println(std::cout, "  Testing mutable ShipList (with modifications)...");
  count = 0;

  for (auto ship : ShipList{em, shipnum_t{1}}) {
    static_assert(std::is_same_v<decltype(ship), ShipHandle>,
                  "MutableIterator should return ShipHandle");

    ship->fuel() += 50.0;
    count++;
  }

  test::expect_eq(count, 3);
  std::println(std::cout, "    Mutable iteration: modified {} ships", count);

  em.clear_cache();
  {
    const Ship* s1 = em.peek_ship(1);
    const Ship* s2 = em.peek_ship(2);
    const Ship* s3 = em.peek_ship(3);
    test::expect_eq(s1->fuel(), 150.0);
    test::expect_eq(s2->fuel(), 250.0);
    test::expect_eq(s3->fuel(), 350.0);
    std::println(std::cout, "    Verified modifications were auto-saved");
  }

  std::println(std::cout,
               "  Testing mutable ShipList with dereference pattern...");
  ShipList shiplist(em, shipnum_t{1});

  for (auto ship_handle : shiplist) {
    Ship& s = *ship_handle;
    s.fuel() += 25.0;
  }

  em.clear_cache();
  {
    const Ship* s1 = em.peek_ship(1);
    test::expect_eq(s1->fuel(), 175.0);
    std::println(std::cout, "    Verified dereference pattern modifications");
  }

  std::println(std::cout, "  Testing spatial ShipList constructors...");
  {
    int univ_count = 0;
    for (const Ship& s : ShipList::readonly(em, ScopeLevel::LEVEL_UNIV)) {
      (void)s;
      univ_count++;
    }
    std::println(std::cout, "    Univ scope ship count: {}", univ_count);

    int star_count = 0;
    for (const Ship& s : ShipList::readonly(em, starnum_t{0})) {
      (void)s;
      star_count++;
    }
    std::println(std::cout, "    Star 0 ship count: {}", star_count);
  }
}

void test_sparse_entity_lists() {
  std::println(std::cout, "Testing sparse ID iteration in entity lists...");
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  // 1. Sparse races: player 1 and player 4 (gaps at 2, 3)
  {
    RaceRepository races(store);
    Race r1{};
    r1.Playernum = 1;
    r1.name = "Empire1";
    races.save(r1);

    Race r4{};
    r4.Playernum = 4;
    r4.name = "Empire4";
    races.save(r4);

    std::vector<player_t> visited_races;
    for (const Race& r : RaceList::readonly(em)) {
      visited_races.push_back(r.Playernum);
    }
    test::expect_eq(visited_races.size(), 2);
    test::expect_eq(visited_races[0], player_t{1});
    test::expect_eq(visited_races[1], player_t{4});
    std::println(std::cout, "  ✓ Sparse RaceList iteration passed");
  }

  // 2. Sparse commods: lot 1 and lot 6 (gaps at 2, 3, 4, 5)
  {
    CommodRepository commods(store);
    Commod c1{};
    c1.id = 1;
    c1.owner = 1;
    c1.amount = 50;
    c1.type = CommodType::RESOURCE;
    commods.save(c1);

    Commod c6{};
    c6.id = 6;
    c6.owner = 4;
    c6.amount = 200;
    c6.type = CommodType::CRYSTAL;
    commods.save(c6);

    std::vector<int> visited_commods;
    for (const Commod& c : CommodList::readonly(em)) {
      visited_commods.push_back(c.id);
    }
    test::expect_eq(visited_commods.size(), 2);
    test::expect_eq(visited_commods[0], 1);
    test::expect_eq(visited_commods[1], 6);
    std::println(std::cout, "  ✓ Sparse CommodList iteration passed");
  }
}

void test_block_list(EntityManager& em, JsonStore& store) {
  BlockRepository repo(store);
  {
    block b1{};
    b1.Playernum = player_t{1};
    b1.name = "Federation";
    b1.VPs = 100;
    repo.save(b1);

    block b3{};
    b3.Playernum = player_t{3};
    b3.name = "Empire";
    b3.VPs = 250;
    repo.save(b3);
  }

  // Readonly iteration (sparse: blocks 1 and 3, skipping 2)
  std::vector<std::string> names;
  for (const block& b : BlockList::readonly(em)) {
    names.push_back(b.name);
  }
  test::expect_eq(names.size(), 2);
  test::expect_eq(names[0], "Federation");
  test::expect_eq(names[1], "Empire");
  std::println(std::cout, "  ✓ BlockList::readonly passed");

  // Mutable iteration with auto-save
  for (auto block_handle : BlockList(em)) {
    block_handle->VPs += 50;
  }

  em.clear_cache();
  const auto* b1_peek = em.peek_block(blocknum_t{1});
  test::expect_true(b1_peek != nullptr);
  test::expect_eq(b1_peek->VPs, 150);

  const auto* b3_peek = em.peek_block(blocknum_t{3});
  test::expect_true(b3_peek != nullptr);
  test::expect_eq(b3_peek->VPs, 300);
  std::println(std::cout, "  ✓ Mutable BlockList auto-save passed");
}

void test_power_list(EntityManager& em, JsonStore& store) {
  PowerRepository repo(store);
  {
    power p1{};
    p1.id = 1;
    p1.troops = 1000;
    p1.popn = 50000;
    p1.money = 20000;
    repo.save(p1);

    power p2{};
    p2.id = 2;
    p2.troops = 2000;
    p2.popn = 80000;
    p2.money = 40000;
    repo.save(p2);
  }

  // Readonly iteration
  std::vector<population_t> troops;
  for (const power& p : PowerList::readonly(em)) {
    troops.push_back(p.troops);
  }
  test::expect_eq(troops.size(), 2);
  test::expect_eq(troops[0], 1000);
  test::expect_eq(troops[1], 2000);
  std::println(std::cout, "  ✓ PowerList::readonly passed");

  // Mutable iteration with auto-save
  for (auto power_handle : PowerList(em)) {
    power_handle->troops += 500;
  }

  em.clear_cache();
  const auto* p1_peek = em.peek_power(powernum_t{1});
  test::expect_true(p1_peek != nullptr);
  test::expect_eq(p1_peek->troops, 1500);

  const auto* p2_peek = em.peek_power(powernum_t{2});
  test::expect_true(p2_peek != nullptr);
  test::expect_eq(p2_peek->troops, 2500);
  std::println(std::cout, "  ✓ Mutable PowerList auto-save passed");
}

}  // namespace

int main() {
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
  test_block_list(em, store);
  test_power_list(em, store);
  populate_ships(em, store);
  test_ship_list_patterns(em);
  test_sparse_entity_lists();

  std::println(std::cout, "All entity list tests passed!");
  return 0;
}
