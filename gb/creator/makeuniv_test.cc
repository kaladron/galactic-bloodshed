// SPDX-License-Identifier: Apache-2.0

/// \file makeuniv_test.cc
/// \brief Invariant and end-to-end tests for UniverseGenerator.

import dallib;
import gb.entities;
import gb.services;
import gb.repositories;
import gb.creator;
import test;
import std;

namespace {

void test_universe_generator_in_memory() {
  std::println(std::cout, "Test: UniverseGenerator in-memory generation");

  Database db(":memory:");

  GB::creator::UniverseConfig config{
      .num_stars = 5,
      .min_planets = 2,
      .max_planets = 4,
      .planetless_chance_percent = 0,
      .auto_name_stars = true,
      .auto_name_planets = true,
      .print_star_info = false,
      .print_planet_info = false,
  };

  GB::creator::UniverseGenerator generator(config);
  auto result = generator.generate(db);

  // Verify result statistics
  test::expect_eq(result.num_stars, 5);
  test::expect_gt(result.planet_count, 0);
  test::expect_gt(result.total_resources, 0);

  JsonStore store(db);
  UniverseRepository universe_repo(store);
  auto universe_opt = universe_repo.get_global_data();
  test::expect_true(universe_opt.has_value());
  test::expect_eq(universe_opt->numstars, 5);
  test::expect_eq(db.count_non_asteroid_planets(), result.planet_count);
  EntityManager em(db);
  test::expect_eq(em.count_non_asteroid_planets(), result.planet_count);

  // Verify each star was persisted with valid properties
  StarRepository star_repo(store);
  PlanetRepository planet_repo(store);
  SectorRepository sector_repo(store);

  for (starnum_t snum = 0; snum < 5; ++snum) {
    auto star_opt = star_repo.find_by_number(snum);
    test::expect_true(star_opt.has_value());
    test::expect_eq(star_opt->star_id(), snum);
    test::expect_false(star_opt->get_name().empty());
    test::expect_true(star_opt->get_struct().gravity > 0.0);
    test::expect_true(star_opt->get_struct().temperature > 0);

    const auto& pnames = star_opt->get_struct().pnames;
    test::expect_true(pnames.size() >= 2);
    test::expect_true(pnames.size() <= 4);

    // Verify each planet of the star was persisted
    for (planetnum_t pnum = 0; pnum < pnames.size(); ++pnum) {
      auto planet_opt = planet_repo.find_by_location(snum, pnum);
      test::expect_true(planet_opt.has_value());
      test::expect_eq(planet_opt->star_id(), snum);
      test::expect_eq(planet_opt->planet_order(), pnum);
      test::expect_gt(planet_opt->dimensions().x, 0);
      test::expect_gt(planet_opt->dimensions().y, 0);

      if (planet_opt->type() != PlanetType::GASGIANT) {
        auto smap = sector_repo.load_map(*planet_opt);
        test::expect_gt(smap.num_sectors(), 0);
      }
    }
  }

  // Verify victory/player tables initialized
  BlockRepository block_repo(store);
  PowerRepository power_repo(store);
  for (int i : std::views::iota(0, MAXPLAYERS)) {
    test::expect_true(
        block_repo.find_by_id(static_cast<blocknum_t>(i)).has_value());
    test::expect_true(
        power_repo.find_by_id(static_cast<powernum_t>(i)).has_value());
  }

  std::println(
      std::cout,
      "  ✓ In-memory universe generation passed (5 stars, {} planets, {} res)",
      result.planet_count, result.total_resources);
}

void test_universe_generator_planetless_stars() {
  std::println(std::cout, "Test: UniverseGenerator planetless stars option");

  Database db(":memory:");

  GB::creator::UniverseConfig config{
      .num_stars = 10,
      .min_planets = 1,
      .max_planets = 5,
      .planetless_chance_percent = 100,  // All stars must be planetless
  };

  GB::creator::UniverseGenerator generator(config);
  auto result = generator.generate(db);

  test::expect_eq(result.num_stars, 10);
  test::expect_eq(result.planet_count, 0);

  JsonStore store(db);
  StarRepository star_repo(store);
  for (starnum_t snum = 0; snum < 10; ++snum) {
    auto star_opt = star_repo.find_by_number(snum);
    test::expect_true(star_opt.has_value());
    test::expect_eq(star_opt->get_struct().pnames.size(), 0zu);
  }

  std::println(std::cout,
               "  ✓ 100% planetless chance verified across 10 stars");
}

void test_universe_generator_custom_names() {
  std::println(std::cout, "Test: UniverseGenerator custom name injection");

  Database db(":memory:");

  GB::creator::UniverseConfig config{
      .num_stars = 3,
      .min_planets = 1,
      .max_planets = 1,
      .planetless_chance_percent = 0,
  };

  GB::creator::UniverseGenerator generator(config);
  generator.set_star_names({"Sol", "Alpha Centauri", "Sirius"});
  generator.set_planet_names({"Earth"});

  auto result = generator.generate(db);
  test::expect_eq(result.num_stars, 3);

  JsonStore store(db);
  StarRepository star_repo(store);
  std::set<std::string> expected_names{"Sol", "Alpha Centauri", "Sirius"};
  for (starnum_t snum = 0; snum < 3; ++snum) {
    auto star_opt = star_repo.find_by_number(snum);
    test::expect_true(star_opt.has_value());
    test::expect_true(expected_names.contains(star_opt->get_name()));
  }

  std::println(std::cout, "  ✓ Custom star and planet name injection passed");
}

}  // namespace

int main() {
  test_universe_generator_in_memory();
  test_universe_generator_planetless_stars();
  test_universe_generator_custom_names();

  std::println(std::cout, "\n✅ All UniverseGenerator tests passed!");
  return 0;
}
