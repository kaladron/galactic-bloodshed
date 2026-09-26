// SPDX-License-Identifier: Apache-2.0

/// \file makeplanet_test.cc
/// \brief Test temperature calculation and planet generation

import dallib;
import gb.entities;
import gb.services;
import gb.creator;
import test;
import std;

void test_temperature_calculation() {
  std::println(std::cout, "Test: Temperature calculation");

  // TEST: Calculate temperatures at increasing orbital distances from star
  int t1 = GB::creator::calculate_temperature(100.0, 5000);
  int t2 = GB::creator::calculate_temperature(500.0, 5000);
  int t3 = GB::creator::calculate_temperature(1500.0, 5000);

  // Verify: Farther planets must be colder than closer planets
  test::expect_gt(t1, t2);
  test::expect_gt(t2, t3);

  std::println(
      std::cout,
      "  ✓ Temperature calculation formula works (t1={}, t2={}, t3={})", t1, t2,
      t3);
}

void test_makeplanet_types() {
  std::println(std::cout, "Test: makeplanet for all PlanetTypes");

  // Setup: Target star ID and starting 1-based planet number
  const starnum_t star_id = 1;
  planetnum_t pnum = 1;

  for (const PlanetType ptype : all_planet_types) {
    std::optional<SectorMap> smap;

    // TEST: Generate planet with makeplanet()
    Planet planet =
        GB::creator::makeplanet(500.0, 6000, ptype, star_id, pnum, smap);

    // Verify: Planet type, location, and dimensions
    test::expect_eq(planet.type(), ptype);
    test::expect_eq(planet.star_id(), star_id);
    test::expect_eq(planet.planet_order(), pnum);
    test::expect_gt(planet.dimensions().x, 0);
    test::expect_gt(planet.dimensions().y, 0);

    // Verify: Solid planets have generated sector maps
    if (ptype != PlanetType::GASGIANT) {
      test::expect_true(smap.has_value());
    }

    std::println(
        std::cout, "  ✓ Planet type {} generated with dimensions {}x{}",
        planet.type_name(), planet.dimensions().x, planet.dimensions().y);
    ++pnum;
  }
}

void test_shuffled_indices() {
  std::println(std::cout, "Test: shuffled_indices permutation validity");

  // Test that shuffled_indices generates complete permutation
  auto rand_perm = shuffled_indices(10);
  test::expect_eq(rand_perm.size(), 10zu);
  std::set<int> seen(rand_perm.begin(), rand_perm.end());
  test::expect_eq(seen.size(), 10zu);

  std::println(std::cout, "  ✓ shuffled_indices passed");
}

int main() {
  test_temperature_calculation();
  test_makeplanet_types();
  test_shuffled_indices();

  std::println(std::cout, "\n✅ All makeplanet tests passed!");
  return 0;
}
