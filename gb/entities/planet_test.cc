// SPDX-License-Identifier: Apache-2.0

/// \file planet_test.cc
/// \brief Unit tests for Planet domain methods, dimensions, and toroidal
/// geometry.

import gb.entities;
import test;
import std;

int main() {
  // Test 1: Planet default and mutable dimensions
  {
    Planet planet(PlanetType::EARTH, Coordinates{0, 0});
    test::expect_eq(planet.dimensions(), Coordinates(0, 0));

    planet.dimensions() = Coordinates(20, 10);
    test::expect_eq(planet.dimensions().x, 20);
    test::expect_eq(planet.dimensions().y, 10);
    test::expect_eq(planet.dimensions(), Coordinates(20, 10));

    planet.dimensions().x = 12;
    planet.dimensions().y = 6;
    test::expect_eq(planet.dimensions(), Coordinates(12, 6));
  }

  // Test 2: Bounds checking with is_valid
  {
    Planet planet(PlanetType::EARTH, Coordinates{0, 0});
    planet.dimensions() = Coordinates(10, 8);

    test::expect_true(planet.is_valid({0, 0}));
    test::expect_true(planet.is_valid({9, 7}));
    test::expect_true(planet.is_valid({5, 4}));

    test::expect_false(planet.is_valid({-1, 0}));
    test::expect_false(planet.is_valid({0, -1}));
    test::expect_false(planet.is_valid({10, 0}));
    test::expect_false(planet.is_valid({0, 8}));
    test::expect_false(planet.is_valid({10, 8}));
  }

  // Test 3: Toroidal coordinate wrapping
  {
    Planet planet(PlanetType::EARTH, Coordinates{0, 0});
    planet.dimensions() = Coordinates(10, 8);

    // Within bounds: unchanged
    test::expect_eq(planet.wrap({0, 3}), Coordinates(0, 3));
    test::expect_eq(planet.wrap({9, 3}), Coordinates(9, 3));

    // Horizontal wrap across right boundary
    test::expect_eq(planet.wrap({10, 3}), Coordinates(0, 3));
    test::expect_eq(planet.wrap({15, 3}), Coordinates(5, 3));

    // Horizontal wrap across left boundary
    test::expect_eq(planet.wrap({-1, 3}), Coordinates(9, 3));
    test::expect_eq(planet.wrap({-10, 3}), Coordinates(0, 3));
    test::expect_eq(planet.wrap({-11, 3}), Coordinates(9, 3));

    // Zero width safety check
    Planet uninit_planet(PlanetType::EARTH, Coordinates{0, 0});
    test::expect_eq(uninit_planet.wrap({5, 3}), Coordinates(5, 3));
  }

  // Test 4: Planet gravity calculation
  {
    Planet planet(PlanetType::EARTH, Coordinates{0, 0});
    planet.dimensions() = Coordinates(20, 10);
    const double expected_gravity = 20.0 * 10.0 * GRAV_FACTOR;
    test::expect_true(std::abs(planet.gravity() - expected_gravity) < 1e-6);
  }

  // Test 5: Coordinate adjacency on planet surface
  {
    Planet planet(PlanetType::EARTH, Coordinates{0, 0});
    planet.dimensions() = Coordinates(10, 8);

    // Direct and diagonal neighbors
    test::expect_true(planet.is_adjacent({5, 5}, {5, 5}));
    test::expect_true(planet.is_adjacent({5, 5}, {6, 5}));
    test::expect_true(planet.is_adjacent({5, 5}, {4, 5}));
    test::expect_true(planet.is_adjacent({5, 5}, {5, 6}));
    test::expect_true(planet.is_adjacent({5, 5}, {5, 4}));
    test::expect_true(planet.is_adjacent({5, 5}, {6, 6}));
    test::expect_true(planet.is_adjacent({5, 5}, {4, 4}));

    // Non-adjacent locations
    test::expect_false(planet.is_adjacent({5, 5}, {7, 5}));
    test::expect_false(planet.is_adjacent({5, 5}, {5, 7}));
    test::expect_false(planet.is_adjacent({5, 5}, {7, 7}));

    // Toroidal seam wrapping adjacency
    test::expect_true(planet.is_adjacent({0, 4}, {9, 4}));
    test::expect_true(planet.is_adjacent({0, 4}, {9, 5}));
    test::expect_true(planet.is_adjacent({0, 4}, {9, 3}));
    test::expect_true(planet.is_adjacent({9, 4}, {0, 4}));

    // Non-wrapping vertical separation across polar caps
    test::expect_false(planet.is_adjacent({5, 0}, {5, 7}));
  }

  // Test 5b: CommodityManifest & plroute defaults
  {
    CommodityManifest manifest{};
    test::expect_false(manifest.any());
    manifest.fuel = true;
    test::expect_true(manifest.any());

    plroute route{};
    test::expect_false(route.set);
    test::expect_false(route.load.any());
    test::expect_false(route.unload.any());
    test::expect_eq(route.dest_star, starnum_t{0});
    test::expect_eq(route.dest_planet, planetnum_t{0});
  }

  // Test 6: Planet compatibility with race conditions
  {
    Planet planet(PlanetType::EARTH, Coordinates{0, 0});
    for (int i = 0; i <= TOXIC; ++i) {
      planet.conditions(static_cast<Conditions>(i)) = 50;
    }
    planet.conditions(TEMP) = 100;
    planet.conditions(TOXIC) = 0;

    Race race{};
    for (int i = 0; i <= OTHER; ++i) {
      race.conditions[i] = 50;
    }
    race.conditions[TEMP] = 100;

    const double compat = planet.compatibility(race);
    test::expect_gt(compat, 90.0);
  }

  // Test 7: plinfo defaults and optional tox_thresh behavior
  {
    Planet planet(PlanetType::EARTH, Coordinates{0, 0});
    auto& info = planet.info(player_t{1});

    // Verify initial default states
    test::expect_eq(info.tox_thresh, std::nullopt);
    test::expect_false(info.explored);
    test::expect_eq(info.autorep, 0U);
    test::expect_eq(info.tax, 0U);
    test::expect_eq(info.newtax, 0U);
    test::expect_eq(info.comread, 0U);
    test::expect_eq(info.mob_set, 0U);
    test::expect_eq(info.guns, 0U);

    // Verify mutating tox_thresh with value and resetting to nullopt
    info.tox_thresh = 35U;
    test::expect_true(info.tox_thresh.has_value());
    test::expect_eq(info.tox_thresh.value(), 35U);
    test::expect_eq(info.tox_thresh.value_or(0U), 35U);

    info.tox_thresh = std::nullopt;
    test::expect_false(info.tox_thresh.has_value());
    test::expect_eq(info.tox_thresh.value_or(0U), 0U);

    // Verify 32-bit unsigned fields
    info.tax = 45U;
    info.newtax = 50U;
    info.comread = 85U;
    info.mob_set = 95U;
    info.guns = 19U;
    info.autorep = 63U;
    info.explored = true;

    // Verify 64-bit resource_t stockpile and production fields
    info.resource = 1'000'000'000LL;
    info.fuel = 500'000'000LL;
    info.destruct = 250'000'000LL;
    info.crystals = 100'000'000LL;
    info.prod_res = 100'000LL;
    info.prod_fuel = 80'000LL;
    info.prod_dest = 70'000LL;
    info.prod_crystals = 60'000LL;
    info.numsectsowned = 500;

    test::expect_eq(info.resource, 1'000'000'000LL);
    test::expect_eq(info.fuel, 500'000'000LL);
    test::expect_eq(info.destruct, 250'000'000LL);
    test::expect_eq(info.crystals, 100'000'000LL);
    test::expect_eq(info.prod_res, 100'000LL);
    test::expect_eq(info.prod_fuel, 80'000LL);
    test::expect_eq(info.prod_dest, 70'000LL);
    test::expect_eq(info.prod_crystals, 60'000LL);
    test::expect_eq(info.numsectsowned, 500U);
  }

  // Test 8: Planet exploration timer and explored flag
  {
    Planet planet(PlanetType::EARTH, Coordinates{0, 0});
    test::expect_eq(planet.expltimer(), 0U);
    test::expect_false(planet.explored());

    planet.expltimer() = 5U;
    planet.explored() = true;

    test::expect_eq(planet.expltimer(), 5U);
    test::expect_true(planet.explored());
  }

  // Test 9: PlayerVector 1-indexed access, bounds checking, and iteration
  {
    PlayerVector<int, 4> pvec;
    test::expect_eq(pvec.size(), 4U);
    test::expect_false(pvec.empty());

    // 1-indexed read and write
    pvec[player_t{1}] = 100;
    pvec[player_t{4}] = 400;

    test::expect_eq(pvec[player_t{1}], 100);
    test::expect_eq(pvec.at(player_t{1}), 100);
    test::expect_eq(pvec[player_t{4}], 400);
    test::expect_eq(pvec.at(player_t{4}), 400);

    // Verify underlying array mapping (1-indexed player 1 is index 0)
    test::expect_eq(pvec.raw_array()[0], 100);
    test::expect_eq(pvec.raw_array()[3], 400);

    // Bounds checking throws std::out_of_range
    test::expect_throws<std::out_of_range>([&]() { (void)pvec[player_t{0}]; });
    test::expect_throws<std::out_of_range>([&]() { (void)pvec[player_t{5}]; });
    test::expect_throws<std::out_of_range>(
        [&]() { (void)pvec[player_t{MAXPLAYERS + 1}]; });

    // Iteration over all slots
    int sum = 0;
    for (int val : pvec) {
      sum += val;
    }
    test::expect_eq(sum, 500);
  }

  // Test 10: Planet::info bounds checking
  {
    Planet planet(PlanetType::EARTH, Coordinates{0, 0});
    planet.info(player_t{1}).popn = 5000;
    planet.info(player_t{MAXPLAYERS}).popn = 9999;

    test::expect_eq(planet.info(player_t{1}).popn, 5000);
    test::expect_eq(planet.info(player_t{MAXPLAYERS}).popn, 9999);

    test::expect_throws<std::out_of_range>(
        [&]() { (void)planet.info(player_t{0}); });
    test::expect_throws<std::out_of_range>(
        [&]() { (void)planet.info(player_t{MAXPLAYERS + 1}); });
  }

  // Test 11: Planet::update_climate
  {
    Planet planet(PlanetType::EARTH, Coordinates{0, 0});
    planet.conditions(RTEMP) = 75;

    planet.update_climate(10);
    test::expect_true(planet.conditions(TEMP) >= 80 &&
                      planet.conditions(TEMP) <= 90);

    planet.update_climate(-20);
    test::expect_true(planet.conditions(TEMP) >= 50 &&
                      planet.conditions(TEMP) <= 60);
  }

  // Test 12: Enslavement, revolt threshold, and slave liberation
  {
    Planet planet(PlanetType::EARTH, Coordinates{0, 0});
    test::expect_false(planet.is_enslaved());
    test::expect_false(planet.is_slave_revolt_triggered());

    planet.enslave_to(player_t{2});
    test::expect_true(planet.is_enslaved());
    test::expect_eq(planet.slaved_to(), player_t{2});

    planet.popn() = 100'000;
    // Revolt threshold is planet.popn() / 1000 = 100
    planet.info(player_t{2}).popn = 101;
    test::expect_false(planet.is_slave_revolt_triggered());

    planet.info(player_t{2}).popn = 100;
    test::expect_true(planet.is_slave_revolt_triggered());

    planet.info(player_t{2}).popn = 50;
    test::expect_true(planet.is_slave_revolt_triggered());
    test::expect_eq(planet.calculate_revolt_devastation_count(), 101);

    planet.free_slaves();
    test::expect_false(planet.is_enslaved());
    test::expect_eq(planet.slaved_to(), player_t{0});
    test::expect_false(planet.is_slave_revolt_triggered());
  }

  // Test 13: plinfo::deposit_production
  {
    plinfo info{};
    info.deposit_production(100, 200, 50, 5);
    test::expect_eq(info.fuel, 100);
    test::expect_eq(info.resource, 200);
    test::expect_eq(info.destruct, 50);
    test::expect_eq(info.crystals, 5);

    info.deposit_production(50, 100, 25, 2);
    test::expect_eq(info.fuel, 150);
    test::expect_eq(info.resource, 300);
    test::expect_eq(info.destruct, 75);
    test::expect_eq(info.crystals, 7);
  }

  // Test 14: plinfo::collect_tax
  {
    // Case A: Tax rate increase capped at +5% per update
    plinfo info{};
    info.popn = 10'000;
    info.tax = 10;
    info.newtax = 25;  // Requesting +15%

    Race race{};
    race.Gov_ship = 100;
    race.governor[0].money = 0;
    race.governor[0].income = 0;

    const money_t revenue = info.collect_tax(race.governor[0], race);
    test::expect_gt(revenue, 0);
    test::expect_eq(info.prod_money, revenue);
    test::expect_eq(race.governor[0].money, revenue);
    test::expect_eq(race.governor[0].income, revenue);
    test::expect_eq(info.tax, 15U);  // 10 + 5 max increase

    // Case B: Tax rate decrease applies immediately
    info.newtax = 5;
    info.collect_tax(race.governor[0], race);
    test::expect_eq(info.tax, 5U);

    // Case C: No government center disables tax collection
    Race anarchic_race{};
    anarchic_race.Gov_ship = 0;
    info.collect_tax(anarchic_race.governor[0], anarchic_race);
    test::expect_eq(info.prod_money, 0);
    test::expect_eq(anarchic_race.governor[0].money, 0);
  }

  // Test 15: plinfo::invest_tech
  {
    plinfo info{};
    info.popn = 5'000;
    info.tech_invest = 100;

    Race race{};
    race.Gov_ship = 100;
    race.governor[0].money = 500;
    race.governor[0].cost_tech = 0;
    race.tech = 10.0;

    // Case A: Sufficient treasury with active government center
    const double tech_gain = info.invest_tech(race.governor[0], race);
    test::expect_gt(tech_gain, 0.0);
    test::expect_eq(race.governor[0].money, 400);
    test::expect_eq(race.governor[0].cost_tech, 100UL);
    test::expect_gt(race.tech, 10.0);
    test::expect_eq(info.prod_tech, tech_gain);

    // Case B: Insufficient funds in treasury
    race.governor[0].money = 50;  // Less than 100 needed
    const double zero_gain = info.invest_tech(race.governor[0], race);
    test::expect_eq(zero_gain, 0.0);
    test::expect_eq(race.governor[0].money, 50);
    test::expect_eq(info.prod_tech, 0.0);

    // Case C: No government center
    Race anarchic_race{};
    anarchic_race.Gov_ship = 0;
    anarchic_race.governor[0].money = 500;
    const double no_gov_gain =
        info.invest_tech(anarchic_race.governor[0], anarchic_race);
    test::expect_eq(no_gov_gain, 0.0);
    test::expect_eq(anarchic_race.governor[0].money, 500);
  }

  // Test 16: plinfo::update_combat_readiness
  {
    plinfo info{};
    info.numsectsowned = 4;

    info.update_combat_readiness(2000);  // 2000 mob points across 4 sectors
    test::expect_eq(info.mob_points, 2000);
    test::expect_eq(info.comread, 500U);  // 2000 / 4
    test::expect_eq(info.guns, 2U);       // 2000 / 1000

    info.numsectsowned = 0;
    info.update_combat_readiness(0);
    test::expect_eq(info.comread, 0U);
    test::expect_eq(info.guns, 0U);
  }

  // Test 17: Stockpile value type & plinfo atomic operations
  {
    Stockpile empty_stock{};
    test::expect_true(empty_stock.empty());

    Stockpile a{.resources = 100, .destruct = 50, .fuel = 200, .crystals = 10};
    test::expect_false(a.empty());

    Stockpile b{.resources = 20, .destruct = 30, .fuel = 50, .crystals = 5};
    a += b;
    test::expect_eq(a.resources, 120U);
    test::expect_eq(a.destruct, 80U);
    test::expect_eq(a.fuel, 250U);
    test::expect_eq(a.crystals, 15U);

    Stockpile limit{
        .resources = 100, .destruct = 100, .fuel = 200, .crystals = 10};
    const Stockpile clamped = a.clamp_to(limit);
    test::expect_eq(clamped.resources, 100U);
    test::expect_eq(clamped.destruct, 80U);
    test::expect_eq(clamped.fuel, 200U);
    test::expect_eq(clamped.crystals, 10U);

    a -= clamped;
    test::expect_eq(a.resources, 20U);
    test::expect_eq(a.destruct, 0U);
    test::expect_eq(a.fuel, 50U);
    test::expect_eq(a.crystals, 5U);

    plinfo info{};
    info.deposit_stockpile(clamped);
    test::expect_eq(info.resource, 100U);
    test::expect_eq(info.destruct, 80U);
    test::expect_eq(info.fuel, 200U);
    test::expect_eq(info.crystals, 10U);
    test::expect_eq(info.stockpile(), clamped);

    const Stockpile drained = info.drain_stockpile();
    test::expect_eq(drained, clamped);
    test::expect_eq(info.resource, 0U);
    test::expect_eq(info.destruct, 0U);
    test::expect_eq(info.fuel, 0U);
    test::expect_eq(info.crystals, 0U);
    test::expect_true(info.stockpile().empty());
  }

  std::println("Planet unit tests passed successfully!");
  return 0;
}
