// SPDX-License-Identifier: Apache-2.0

/// \file vn_test.cc
/// \brief Unit tests for select_victim_to_steal_from candidate priority
/// ordering.

import dallib;
import gb.entities;
import gb.repositories;
import gb.services;
import gb.turn;
import test;
import std;

int main() {
  // Fixed RNG seed for test determinism
  seed_rand(42);

  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // =========================================================================
  // 1. select_victim_to_steal_from candidate priority ordering tests
  // =========================================================================
  {
    Planet planet{};
    planet.star_id() = 1;
    planet.planet_order() = 1;

    // Set up resources on planet info for various players
    planet.info(player_t{1}).resource = 0;
    planet.info(player_t{2}).resource = 500;
    planet.info(player_t{3}).resource = 1000;
    planet.info(player_t{4}).resource = 0;

    // 1. Order [1, 2, 3, 4] -> Should pick player 2 (first with resources > 0)
    std::vector<player_t> order1 = {player_t{1}, player_t{2}, player_t{3},
                                    player_t{4}};
    auto victim1 = select_victim_to_steal_from(planet, order1);
    test::expect_true(victim1.has_value());
    test::expect_eq(victim1->value, 2);

    // 2. Order [3, 2, 1, 4] -> Should pick player 3
    std::vector<player_t> order2 = {player_t{3}, player_t{2}, player_t{1},
                                    player_t{4}};
    auto victim2 = select_victim_to_steal_from(planet, order2);
    test::expect_true(victim2.has_value());
    test::expect_eq(victim2->value, 3);

    // 3. Order [1, 4] -> None have resources -> Should return std::nullopt
    std::vector<player_t> order3 = {player_t{1}, player_t{4}};
    auto victim3 = select_victim_to_steal_from(planet, order3);
    test::expect_false(victim3.has_value());

    // 4. Test shuffled_indices produces valid race IDs permutation
    auto rand_ids = shuffled_indices(1, 5);  // 1 to 4 inclusive
    test::expect_eq(rand_ids.size(), 4);
    std::set<int> seen(rand_ids.begin(), rand_ids.end());
    test::expect_eq(seen.size(), 4);
    test::expect_true(seen.contains(1) && seen.contains(2) &&
                      seen.contains(3) && seen.contains(4));

    std::println(std::cout, "✓ select_victim_to_steal_from unit tests passed");
  }

  JsonStore store(db);
  StarRepository star_repo(store);
  PlanetRepository planet_repo(store);
  RaceRepository race_repo(store);
  UniverseRepository universe_repo(store);

  Race r1{};
  r1.Playernum = 1;
  r1.name = "VN Machine";
  race_repo.save(r1);

  Race r2{};
  r2.Playernum = 2;
  r2.name = "Terran";
  race_repo.save(r2);

  planet_struct p0_data{};
  p0_data.star_id = 0;
  p0_data.planet_order = 0;
  planet_repo.save(Planet{p0_data});

  // =========================================================================
  // 2. find_closest_stars tests (including Bug 1: Star 0 Orbit Search Fix)
  // =========================================================================
  {
    std::println(std::cout, "\nTest: find_closest_stars");

    // Setup 4 stars:
    // Star 0 at (0, 0)
    // Star 1 at (10, 0) -> dist to Star 0 is 10
    // Star 2 at (25, 0) -> dist to Star 0 is 25
    // Star 3 at (100, 0) -> dist to Star 0 is 100
    universe_struct udata{};
    udata.id = 1;
    udata.numstars = 4;
    universe_repo.save(udata);

    star_struct s0{};
    s0.star_id = 0;
    s0.xpos = 0.0;
    s0.ypos = 0.0;
    s0.pnames = {"P1", "P2"};

    star_struct s1{};
    s1.star_id = 1;
    s1.xpos = 10.0;
    s1.ypos = 0.0;
    s1.pnames = {"P1", "P2"};

    star_struct s2{};
    s2.star_id = 2;
    s2.xpos = 25.0;
    s2.ypos = 0.0;
    s2.pnames = {"P1"};

    star_struct s3{};
    s3.star_id = 3;
    s3.xpos = 100.0;
    s3.ypos = 0.0;
    s3.pnames = {};

    star_repo.save(Star{s0});
    star_repo.save(Star{s1});
    star_repo.save(Star{s2});
    star_repo.save(Star{s3});

    // Test search from Star 0: Closest is Star 1 (dist 10), second closest is
    // Star 2 (dist 25)
    auto res0 = find_closest_stars(em, starnum_t{0}, 0.0, 0.0);
    test::expect_eq(res0.closest, starnum_t{1});
    test::expect_eq(res0.second_closest, starnum_t{2});

    // Test search from Star 1: Closest is Star 0 (dist 10), second closest is
    // Star 2 (dist 15)
    auto res1 = find_closest_stars(em, starnum_t{1}, 10.0, 0.0);
    test::expect_eq(res1.closest, starnum_t{0});
    test::expect_eq(res1.second_closest, starnum_t{2});

    std::println(
        std::cout,
        "  ✓ find_closest_stars accurately finds nearest non-orbiting stars");
  }

  // =========================================================================
  // 3. select_berserker_destination tests (including Bug 2: Zero-Planet Guard)
  // =========================================================================
  {
    std::println(std::cout, "\nTest: select_berserker_destination");

    TurnStats stats{};
    stats.VN_brain.most_mad = player_t{2};

    em.mutate_universe([](universe_struct& u) {
      u.VN_index1[player_t{2}] = 1;
      u.VN_index2[player_t{2}] = 1;
      u.VN_index1[player_t{3}] = 3;  // Star 3 has 0 planets
      u.VN_index2[player_t{3}] = 3;
    });

    ship_struct bers_data{};
    bers_data.number = 200;
    bers_data.owner = 1;
    bers_data.type = ShipType::OTYPE_BERS;
    bers_data.hyper_drive.has = true;
    bers_data.mounted = true;

    auto bers_ship = ShipFactory::create(bers_data);
    auto* bers = bers_ship->as<BerserkerShip>();
    test::expect_true(bers != nullptr);

    select_berserker_destination(em, *bers, stats);

    test::expect_true(bers->bombard());
    test::expect_eq(bers->whatdest(), ScopeLevel::LEVEL_PLAN);
    test::expect_eq(bers->deststar(), starnum_t{1});
    test::expect_eq(bers->mind().target, player_t{2});
    test::expect_true(bers->is_busy());
    test::expect_true(bers->hyper_drive().on);
    test::expect_eq(bers->hyper_drive().charge, HYPER_DRIVE_READY_CHARGE);

    // Test zero-planet star target (Regression test for Bug 2)
    stats.VN_brain.most_mad = player_t{3};
    select_berserker_destination(em, *bers, stats);
    test::expect_eq(bers->deststar(), starnum_t{3});
    test::expect_eq(bers->destpnum(), planetnum_t{0});
    test::expect_eq(bers->whatdest(), ScopeLevel::LEVEL_STAR);

    std::println(std::cout, "  ✓ select_berserker_destination targets hitlist "
                            "and guards 0-planet systems");
  }

  // =========================================================================
  // 4. select_vn_destination tests
  // =========================================================================
  {
    std::println(std::cout, "\nTest: select_vn_destination");

    ship_struct vn_data{};
    vn_data.number = 201;
    vn_data.owner = 1;
    vn_data.type = ShipType::OTYPE_VN;
    vn_data.storbits = 0;
    vn_data.xpos = 0.0;
    vn_data.ypos = 0.0;

    auto vn_ship = ShipFactory::create(vn_data);
    auto* vn = vn_ship->as<VonNeumannShip>();
    test::expect_true(vn != nullptr);

    // Case A: Closest star (Star 1) is not inhabited by Player 1 -> routes to
    // Star 1
    select_vn_destination(em, *vn);
    test::expect_eq(vn->deststar(), starnum_t{1});
    test::expect_eq(vn->whatdest(), ScopeLevel::LEVEL_PLAN);
    test::expect_true(vn->is_busy());
    test::expect_eq(vn->speed(), Shipdata[ShipType::OTYPE_VN][ABIL_SPEED]);

    // Case B: Star 1 is inhabited by Player 1 -> routes to Star 2 (second
    // closest)
    em.mutate_star(1, [](Star& s) { s.mark_inhabited_by(player_t{1}); });

    select_vn_destination(em, *vn);
    test::expect_eq(vn->deststar(), starnum_t{2});
    test::expect_eq(vn->whatdest(), ScopeLevel::LEVEL_PLAN);
    test::expect_true(vn->is_busy());

    std::println(
        std::cout,
        "  ✓ select_vn_destination navigates to optimal uninhabited stars");
  }

  // =========================================================================
  // 5. mine_sector tests
  // =========================================================================
  {
    std::println(std::cout, "\nTest: mine_sector");

    ship_struct vn_data{};
    vn_data.number = 301;
    vn_data.owner = 1;
    vn_data.type = ShipType::OTYPE_VN;
    vn_data.resource = 0;
    vn_data.fuel = 0.0;
    auto vn_ship = ShipFactory::create(vn_data);
    auto* vn = vn_ship->as<VonNeumannShip>();
    test::expect_true(vn != nullptr);

    sector_struct s_data{};
    s_data.resource = 100;
    Sector sector(s_data);

    // Mine sector with 100 resource: takes 50%, yields 50
    resource_t yield = mine_sector(*vn, sector);
    test::expect_eq(yield, 50);
    test::expect_eq(sector.get_resource(), 50);
    test::expect_eq(vn->resource(), 50);
    test::expect_eq(vn->fuel(), 50.0);

    // Mine depleted sector
    sector.set_resource(0);
    resource_t empty_yield = mine_sector(*vn, sector);
    test::expect_eq(empty_yield, 0);
    test::expect_eq(vn->resource(), 50);

    // Berserker mining converts to destruct (5x) and fuel
    ship_struct bers_data{};
    bers_data.number = 302;
    bers_data.owner = 1;
    bers_data.type = ShipType::OTYPE_BERS;
    bers_data.destruct = 0;
    bers_data.fuel = 0.0;
    auto bers_ship = ShipFactory::create(bers_data);
    auto* bers = bers_ship->as<BerserkerShip>();
    test::expect_true(bers != nullptr);

    sector.set_resource(200);
    resource_t bers_yield = mine_sector(*bers, sector);
    test::expect_eq(bers_yield, 100);
    test::expect_eq(sector.get_resource(), 100);
    test::expect_eq(bers->destruct(), 500);  // 5 * 100
    test::expect_eq(bers->fuel(), 100.0);

    std::println(std::cout,
                 "  ✓ mine_sector extracts resources and fuel correctly");
  }

  // =========================================================================
  // 6. roam_to_adjacent_sector tests
  // =========================================================================
  {
    std::println(std::cout, "\nTest: roam_to_adjacent_sector");

    Planet planet(PlanetType::EARTH, Coordinates{10, 10});

    ship_struct vn_data{};
    vn_data.number = 303;
    vn_data.owner = 1;
    vn_data.type = ShipType::OTYPE_VN;
    vn_data.land_coords = {5, 5};
    auto vn_ship = ShipFactory::create(vn_data);
    auto* vn = vn_ship->as<VonNeumannShip>();
    test::expect_true(vn != nullptr);

    seed_rand(42);
    Coordinates new_c = roam_to_adjacent_sector(*vn, planet);
    test::expect_true(std::abs(new_c.x - 5) <= 1 || new_c.x == 9 ||
                      new_c.x == 0);
    test::expect_true(std::abs(new_c.y - 5) <= 1);
    test::expect_eq(vn->land_coords(), new_c);

    // North pole clamping: at y == 0, the machine cannot move north (y < 0),
    // so y is always in [0, 1] and strictly adjacent to {0, 0}.
    vn->set_land_coords({0, 0});
    Coordinates north_polar_c = roam_to_adjacent_sector(*vn, planet);
    test::expect_true(north_polar_c.y >= 0 && north_polar_c.y <= 1);
    test::expect_true(planet.is_adjacent({0, 0}, north_polar_c));

    // South pole clamping: at y == Maxy - 1 (y == 9), the machine cannot move
    // south (y >= 10), so y is always in [8, 9] and strictly adjacent to {5, 9}.
    vn->set_land_coords({5, 9});
    Coordinates south_polar_c = roam_to_adjacent_sector(*vn, planet);
    test::expect_true(south_polar_c.y >= 8 && south_polar_c.y <= 9);
    test::expect_true(planet.is_adjacent({5, 9}, south_polar_c));

    std::println(
        std::cout,
        "  ✓ roam_to_adjacent_sector wanders safely with polar clamping");
  }

  // =========================================================================
  // 7. steal_planetary_resources tests
  // =========================================================================
  {
    std::println(std::cout, "\nTest: steal_planetary_resources");

    // Add Player 2 colony with resources on Star 0, Planet 0
    em.mutate_planet(0, 0,
                     [](Planet& p) { p.info(player_t{2}).resource = 1000; });

    ship_struct vn_data{};
    vn_data.number = 304;
    vn_data.owner = 1;
    vn_data.type = ShipType::OTYPE_VN;
    vn_data.storbits = 0;
    vn_data.pnumorbits = 0;
    vn_data.resource = 0;
    auto vn_ship = ShipFactory::create(vn_data);
    auto* vn = vn_ship->as<VonNeumannShip>();
    test::expect_true(vn != nullptr);

    auto result = steal_planetary_resources(em, *vn);
    test::expect_eq(result.victim, player_t{2});
    test::expect_eq(result.amount, Shipdata[ShipType::OTYPE_VN][ABIL_COST]);
    test::expect_eq(vn->resource(), Shipdata[ShipType::OTYPE_VN][ABIL_COST]);

    const auto& planet_after = *em.peek_planet(0, 0);
    test::expect_eq(planet_after.info(player_t{2}).resource,
                    1000 - Shipdata[ShipType::OTYPE_VN][ABIL_COST]);

    std::println(
        std::cout,
        "  ✓ steal_planetary_resources steals resources from alien colony");
  }

  std::println(std::cout,
               "\nAll VN navigation and turn tests passed successfully!");
  return 0;
}
