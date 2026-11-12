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

void test_makeplanet_determinism() {
  std::println(std::cout, "Test: makeplanet determinism under seed_rand");

  seed_rand(42);
  std::optional<SectorMap> smap1;
  Planet p1 =
      GB::creator::makeplanet(500.0, 6000, PlanetType::EARTH, 1, 1, smap1);

  seed_rand(42);
  std::optional<SectorMap> smap2;
  Planet p2 =
      GB::creator::makeplanet(500.0, 6000, PlanetType::EARTH, 1, 1, smap2);

  test::expect_eq(p1.dimensions(), p2.dimensions());
  test::expect_eq(p1.temp(), p2.temp());
  test::expect_eq(p1.conditions().oxygen, p2.conditions().oxygen);
  test::expect_eq(p1.conditions().nitrogen, p2.conditions().nitrogen);
  test::expect_true(smap1.has_value());
  test::expect_true(smap2.has_value());

  for (const Coordinates c : smap1->coordinates()) {
    const auto& s1 = smap1->get(c);
    const auto& s2 = smap2->get(c);
    test::expect_eq(s1.get_type(), s2.get_type());
    test::expect_eq(s1.get_condition(), s2.get_condition());
    test::expect_eq(s1.get_resource(), s2.get_resource());
    test::expect_eq(s1.get_fert(), s2.get_fert());
    test::expect_eq(s1.get_crystals(), s2.get_crystals());
  }
  std::println(std::cout, "  ✓ makeplanet determinism passed");
}

void test_asteroid_generation() {
  std::println(std::cout,
               "Test: ASTEROID generation across seeds (including maxy == 1)");

  bool saw_maxy_1 = false;
  for (unsigned int s = 1; s <= 50; ++s) {
    seed_rand(s);
    std::optional<SectorMap> smap;
    Planet p =
        GB::creator::makeplanet(200.0, 5000, PlanetType::ASTEROID, 1, 1, smap);
    test::expect_ge(p.dimensions().x, 2);
    test::expect_le(p.dimensions().x, 4);
    test::expect_ge(p.dimensions().y, 1);
    test::expect_le(p.dimensions().y, 3);
    test::expect_true(smap.has_value());
    test::expect_eq(smap->num_sectors(), p.num_sectors());
    if (p.dimensions().y == 1) {
      saw_maxy_1 = true;
    }
  }
  test::expect_true(saw_maxy_1);
  std::println(std::cout,
               "  ✓ ASTEROID generation across seeds passed (saw maxy == 1)");
}

void test_cold_planet_polar_icing() {
  std::println(
      std::cout,
      "Test: cold planet polar icing type and resource synchronization");

  seed_rand(12345);
  std::optional<SectorMap> smap;
  // Very distant from cold star -> extreme cold
  Planet p = GB::creator::makeplanet(3000.0, 0, PlanetType::EARTH, 1, 1, smap);
  test::expect_true(smap.has_value());

  const int maxy = p.dimensions().y;
  bool saw_ice = false;
  for (const Coordinates c : smap->coordinates()) {
    if (c.y == 0 || c.y == maxy - 1) {
      const auto& s = smap->get(c);
      if (s.get_condition() == SectorType::SEC_ICE) {
        saw_ice = true;
        // Verify underlying type is also SEC_ICE
        test::expect_eq(s.get_type(), SectorType::SEC_ICE);
        // Verify resources within Earth ice bounds: 200..250
        test::expect_ge(s.get_resource(), 200);
        test::expect_le(s.get_resource(), 250);
        // Verify fertility within Earth ice bounds: 20..40
        test::expect_ge(s.get_fert(), 20);
        test::expect_le(s.get_fert(), 40);
      }
    }
  }
  test::expect_true(saw_ice);
  std::println(std::cout,
               "  ✓ polar icing type and resource synchronization passed");
}

void test_multipass_surface_generation() {
  std::println(std::cout, "Test: multi-pass Earth/Forest/Desert generation");

  for (const PlanetType type :
       {PlanetType::EARTH, PlanetType::FOREST, PlanetType::DESERT}) {
    seed_rand(999);
    std::optional<SectorMap> smap;
    // Warm star so equatorial/temperate continents are not iced over
    Planet p = GB::creator::makeplanet(100.0, 9000, type, 1, 1, smap);
    test::expect_true(smap.has_value());
    test::expect_eq(smap->num_sectors(), p.num_sectors());

    int land_count = 0;
    for (const Sector& s : *smap) {
      // Every generated sector has synchronized underlying type and condition
      test::expect_eq(s.get_type(), s.get_condition());
      if (s.get_type() == SectorType::SEC_LAND) {
        ++land_count;
      }
    }
    if (type == PlanetType::EARTH) {
      // Initial seed is at most num_sectors() / 20; multi-pass continent growth
      // expands land well beyond the seed count.
      test::expect_gt(land_count, p.num_sectors() / 20);
    }
  }
  std::println(std::cout, "  ✓ multi-pass surface generation passed");
}

void test_atmosphere_rng_invariants() {
  std::println(std::cout, "Test: Forest oxygen & non-inverted gas bounds");

  seed_rand(2026);
  for (int i = 0; i < 500; ++i) {
    std::optional<SectorMap> smap;
    Planet forest =
        GB::creator::makeplanet(300.0, 5000, PlanetType::FOREST, 1, 1, smap);
    test::expect_ge(forest.conditions().oxygen, 10);
    test::expect_le(forest.conditions().methane, 1);
  }

  for (const PlanetType type :
       {PlanetType::GASGIANT, PlanetType::MARS, PlanetType::ICEBALL}) {
    for (int i = 0; i < 100; ++i) {
      std::optional<SectorMap> smap;
      Planet p = GB::creator::makeplanet(400.0, 5000, type, 1, 1, smap);
      const auto& c = p.conditions();
      test::expect_ge(c.other, 0);
      const int sum = c.hydrogen.value() + c.helium.value() +
                      c.methane.value() + c.oxygen.value() + c.co2.value() +
                      c.nitrogen.value() + c.sulfur.value() + c.other.value();
      test::expect_true(sum == 100 || sum == 0);
    }
  }
  std::println(std::cout, "  ✓ atmosphere RNG invariants passed");
}

int main() {
  test_temperature_calculation();
  test_makeplanet_types();
  test_shuffled_indices();
  test_makeplanet_determinism();
  test_asteroid_generation();
  test_cold_planet_polar_icing();
  test_multipass_surface_generation();
  test_atmosphere_rng_invariants();

  std::println(std::cout, "\n✅ All makeplanet tests passed!");
  return 0;
}
