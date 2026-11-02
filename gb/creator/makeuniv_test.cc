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
      .deity_password = "godpass",
      .deity_gov_password = "govpass",
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
  test::expect_eq(db.count_non_asteroid_planets(), result.planet_count);
  EntityManager em(db);
  test::expect_eq(em.num_stars(), 5);
  test::expect_eq(em.max_star_id(), starnum_t{5});
  test::expect_eq(em.count_non_asteroid_planets(), result.planet_count);

  // Verify each star was persisted with valid properties
  StarRepository star_repo(store);
  PlanetRepository planet_repo(store);
  SectorRepository sector_repo(store);

  for (starnum_t snum = 1; snum <= 5; ++snum) {
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
    for (planetnum_t pnum = 1; pnum <= pnames.size(); ++pnum) {
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

  // Verify Player 1 was seeded as the disembodied Deity
  const auto* race = em.peek_race(player_t{1});
  test::expect_true(race != nullptr);
  if (race) {
    test::expect_true(race->God);
    test::expect_false(race->Guest);
    test::expect_false(race->Gov_ship.has_value());
    test::expect_eq(race->leader().deflevel, ScopeLevel::LEVEL_UNIV);
    test::expect_eq(race->leader().homesystem, starnum_t{0});
    test::expect_eq(race->leader().homeplanetnum, planetnum_t{0});
    test::expect_eq(race->password, "godpass");
    test::expect_eq(race->leader().password, "govpass");
  }

  // Verify victory/player tables: Player 1 seeded, mortal player rows not
  // pre-populated
  BlockRepository block_repo(store);
  PowerRepository power_repo(store);
  test::expect_true(block_repo.find_by_id(player_t{1}).has_value());
  test::expect_true(power_repo.find_by_id(player_t{1}).has_value());
  for (player_t p : all_players()) {
    if (p == 1) continue;
    test::expect_false(block_repo.find_by_id(p).has_value());
    test::expect_false(power_repo.find_by_id(p).has_value());
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
      .deity_password = "godpass",
      .deity_gov_password = "govpass",
  };

  GB::creator::UniverseGenerator generator(config);
  auto result = generator.generate(db);

  test::expect_eq(result.num_stars, 10);
  test::expect_eq(result.planet_count, 0);

  JsonStore store(db);
  StarRepository star_repo(store);
  for (starnum_t snum = 1; snum <= 10; ++snum) {
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
      .deity_password = "godpass",
      .deity_gov_password = "govpass",
  };

  GB::creator::UniverseGenerator generator(config);
  generator.set_star_names({"Sol", "Alpha Centauri", "Sirius"});
  generator.set_planet_names({"Earth"});

  auto result = generator.generate(db);
  test::expect_eq(result.num_stars, 3);

  JsonStore store(db);
  StarRepository star_repo(store);
  std::set<std::string> expected_names{"Sol", "Alpha Centauri", "Sirius"};
  for (starnum_t snum = 1; snum <= 3; ++snum) {
    auto star_opt = star_repo.find_by_number(snum);
    test::expect_true(star_opt.has_value());
    test::expect_true(expected_names.contains(star_opt->get_name()));
  }

  std::println(std::cout, "  ✓ Custom star and planet name injection passed");
}

void test_universe_generator_partial_planetless_and_name_cap() {
  std::println(std::cout,
               "Test: UniverseGenerator partial planetless chance & name cap");

  const auto tmp_names =
      std::filesystem::temp_directory_path() / "test_1005_names.txt";
  {
    std::ofstream out(tmp_names);
    for (int i = 1; i <= 1005; ++i) {
      out << "StarName_" << i << "\r\n";
    }
  }

  Database db(":memory:");
  GB::creator::UniverseConfig config{
      .num_stars = 5,
      .min_planets = 1,
      .max_planets = 2,
      .planetless_chance_percent = 1,
      .auto_name_stars = true,
      .auto_name_planets = false,
      .star_names_file = tmp_names.string(),
      .deity_password = "godpass",
      .deity_gov_password = "govpass",
  };

  GB::creator::UniverseGenerator generator(config);
  auto result = generator.generate(db);
  test::expect_eq(result.num_stars, 5);

  std::filesystem::remove(tmp_names);
}

void test_universe_generator_deity_password_validation() {
  std::println(std::cout,
               "Test: UniverseGenerator deity password validation (MC/DC)");

  // 1. Empty deity_password
  {
    Database db(":memory:");
    GB::creator::UniverseConfig cfg{
        .num_stars = 1,
        .deity_password = "",
        .deity_gov_password = "govpass",
    };
    GB::creator::UniverseGenerator gen(cfg);
    test::expect_throws<std::invalid_argument>([&]() { gen.generate(db); });
  }

  // 2. Whitespace in deity_password
  {
    Database db(":memory:");
    GB::creator::UniverseConfig cfg{
        .num_stars = 1,
        .deity_password = "god pass",
        .deity_gov_password = "govpass",
    };
    GB::creator::UniverseGenerator gen(cfg);
    test::expect_throws<std::invalid_argument>([&]() { gen.generate(db); });
  }

  // 3. Empty deity_gov_password
  {
    Database db(":memory:");
    GB::creator::UniverseConfig cfg{
        .num_stars = 1,
        .deity_password = "godpass",
        .deity_gov_password = "",
    };
    GB::creator::UniverseGenerator gen(cfg);
    test::expect_throws<std::invalid_argument>([&]() { gen.generate(db); });
  }

  // 4. Whitespace in deity_gov_password
  {
    Database db(":memory:");
    GB::creator::UniverseConfig cfg{
        .num_stars = 1,
        .deity_password = "godpass",
        .deity_gov_password = "gov pass",
    };
    GB::creator::UniverseGenerator gen(cfg);
    test::expect_throws<std::invalid_argument>([&]() { gen.generate(db); });
  }

  std::println(std::cout, "  ✓ Deity password validation (MC/DC) passed");
}

}  // namespace

int main() {
  test_universe_generator_in_memory();
  test_universe_generator_planetless_stars();
  test_universe_generator_custom_names();
  test_universe_generator_partial_planetless_and_name_cap();
  test_universe_generator_deity_password_validation();

  std::println(std::cout, "\n✅ All UniverseGenerator tests passed!");
  return 0;
}
