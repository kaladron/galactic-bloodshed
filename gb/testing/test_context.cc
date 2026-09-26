// SPDX-License-Identifier: Apache-2.0

/// \file test_context.cc
/// \brief Implementation of TestContext fixture and dispatch assertion helpers.

module test;

import commands;
import dallib;
import gb.entities;
import gb.services;
import gb.repositories;
import gb.creator;
import std;

namespace {

void seed_standard_races(EntityManager& em) {
  GB::creator::EnrollmentService enrollment(em);
  struct StandardRaceDef {
    player_t id;
    std::string_view name;
    starnum_t home_star;
  };
  constexpr std::array<StandardRaceDef, 4> standard_races = {{
      {1, "Federation", 1},
      {2, "Klingons", 2},
      {3, "Romulans", 3},
      {4, "Cardassians", 1},
  }};

  for (const auto& def : standard_races) {
    GB::creator::RaceEnrollmentSpec spec{};
    spec.name = std::string(def.name);
    Race r = enrollment.build_race(def.id, spec, def.home_star, 1);
    r.tech = 100.0;
    r.leader().money = 10'000;
    em.create_race(r);
  }
}

}  // namespace

TestContext::TestContext() : db(":memory:"), em(db) {
  initialize_schema(db);
  universe_struct u{};
  JsonStore store(db);
  UniverseRepository universe_repo(store);
  universe_repo.save(u);

  GB::creator::EnrollmentService enrollment(em);
  GB::creator::RaceEnrollmentSpec spec{};
  spec.name = "TestRace";
  em.create_race(enrollment.build_race(1, spec));
  em.clear_cache();
}

void TestContext::setup_game_obj(GameObj& g, player_t player, governor_t gov) {
  g.set_player(player);
  g.set_governor(gov);
  if (g.snum() == 0) g.set_snum(1);
  if (g.pnum() == 0) g.set_pnum(1);
  if (player > 0) {
    g.race = em.peek_race(player);
  } else {
    g.race = nullptr;
  }
}

bool TestContext::dispatch(GameObj& g,
                           const GB::commands::CommandDescriptor& desc,
                           const command_t& argv) {
  g.out.str("");
  return GB::commands::dispatch_command(g, desc, argv);
}

bool TestContext::dispatch(GameObj& g, const command_t& argv) {
  if (argv.empty()) return false;
  const auto* desc = GB::commands::find_command_descriptor(argv[0]);
  if (!desc) return false;
  return dispatch(g, *desc, argv);
}

void TestContext::assert_dispatch_success(
    GameObj& g, const GB::commands::CommandDescriptor& desc,
    const command_t& argv, ap_t expected_star_ap_deducted,
    ap_t expected_univ_ap_deducted) {
  ap_t initial_star_ap = 0;
  starnum_t snum = g.snum();
  bool has_star = false;
  try {
    if (const auto* star = em.peek_star(snum)) {
      initial_star_ap = star->AP(g.player());
      has_star = true;
    }
  } catch (const EntityNotFoundError&) {
  }

  ap_t initial_univ_ap = 0;
  try {
    if (const auto* univ = em.peek_universe()) {
      initial_univ_ap = univ->AP[g.player()];
    }
  } catch (const EntityNotFoundError&) {
    initial_univ_ap = 0;
  }

  bool ok = dispatch(g, desc, argv);
  test::expect_true(
      ok, std::format("Expected command dispatch to succeed, output was: {}",
                      g.out.str()));

  if (expected_star_ap_deducted > 0 && has_star) {
    ap_t final_star_ap = em.peek_star(snum)->AP(g.player());
    test::expect_eq(final_star_ap, initial_star_ap - expected_star_ap_deducted,
                    "Star AP deduction mismatch");
  }

  if (expected_univ_ap_deducted > 0) {
    ap_t final_univ_ap = em.peek_universe()->AP[g.player()];
    test::expect_eq(final_univ_ap, initial_univ_ap - expected_univ_ap_deducted,
                    "Universe AP deduction mismatch");
  }
}

void TestContext::assert_dispatch_success(GameObj& g, const command_t& argv,
                                          ap_t expected_star_ap_deducted,
                                          ap_t expected_univ_ap_deducted) {
  test::expect_false(argv.empty(), "argv must not be empty");
  const auto* desc = GB::commands::find_command_descriptor(argv[0]);
  test::expect_true(desc != nullptr,
                    "Command descriptor must exist for dispatch");
  assert_dispatch_success(g, *desc, argv, expected_star_ap_deducted,
                          expected_univ_ap_deducted);
}

void TestContext::assert_dispatch_rejected(
    GameObj& g, const GB::commands::CommandDescriptor& desc,
    const command_t& argv) {
  ap_t initial_star_ap = 0;
  starnum_t snum = g.snum();
  bool has_star = false;
  try {
    if (const auto* star = em.peek_star(snum)) {
      initial_star_ap = star->AP(g.player());
      has_star = true;
    }
  } catch (const EntityNotFoundError&) {
  }

  ap_t initial_univ_ap = 0;
  bool has_univ = false;
  try {
    if (const auto* univ = em.peek_universe()) {
      initial_univ_ap = univ->AP[g.player()];
      has_univ = true;
    }
  } catch (const EntityNotFoundError&) {
  }

  bool ok = dispatch(g, desc, argv);
  test::expect_false(
      ok,
      std::format("Expected command dispatch to be rejected, output was: {}",
                  g.out.str()));

  if (has_star && desc.ap.model == GB::commands::APModel::FixedStar) {
    try {
      if (const auto* star = em.peek_star(snum)) {
        test::expect_eq(star->AP(g.player()), initial_star_ap,
                        "Rejected command must not deduct star AP");
      }
    } catch (const EntityNotFoundError&) {
      (void)0;
    }
  }

  if (has_univ && desc.ap.model == GB::commands::APModel::FixedUniv) {
    try {
      if (const auto* univ = em.peek_universe()) {
        test::expect_eq(univ->AP[g.player()], initial_univ_ap,
                        "Rejected command must not deduct universe AP");
      }
    } catch (const EntityNotFoundError&) {
      (void)0;
    }
  }
}

void TestContext::assert_dispatch_rejected(GameObj& g, const command_t& argv) {
  test::expect_false(argv.empty(), "argv must not be empty");
  const auto* desc = GB::commands::find_command_descriptor(argv[0]);
  test::expect_true(desc != nullptr,
                    "Command descriptor must exist for dispatch");
  assert_dispatch_rejected(g, *desc, argv);
}

void TestContext::verify_universe_invariants(std::source_location loc) {
  test::verify_universe_invariants(em, loc);
}

TestContext& TestContext::with_standard_universe() {
  JsonStore store(db);

  // 1. Setup 4 standard races via EnrollmentService::build_race:
  // Player 1 (Federation), Player 2 (Klingons), Player 3 (Romulans),
  // Player 4 (Cardassians)
  seed_standard_races(em);

  // 2. Setup Star 1 (Sol) with 100 AP for all 4 races, explored and inhabited
  create_star("Sol", 1)
      .with_position({0.0, 0.0})
      .with_stability(15)
      .with_gravity(1.0)
      .with_temperature(50)
      .with_ap(1, 100)
      .with_ap(2, 100)
      .with_ap(3, 100)
      .with_ap(4, 100)
      .with_explored(1)
      .with_explored(2)
      .with_explored(3)
      .with_explored(4)
      .with_inhabited(1)
      .with_inhabited(2)
      .with_inhabited(3)
      .with_inhabited(4)
      .build();

  // 3. Setup Planet 1 on Star 1 (Earth)
  create_planet(1, PlanetType::EARTH, Coordinates{10, 10}, 1)
      .named("Earth")
      .with_position(SystemCoordinates{100.0, 0.0})
      .with_stockpiles(1, 1000, 1000, 1000)
      .with_stockpiles(2, 1000, 1000, 1000)
      .with_stockpiles(3, 1000, 1000, 1000)
      .with_stockpiles(4, 1000, 1000, 1000)
      .with_tax(1, 10)
      .with_tax(2, 10)
      .with_tax(3, 10)
      .with_tax(4, 10)
      .with_explored(1, true)
      .with_explored(2, true)
      .with_explored(3, true)
      .with_explored(4, true)
      .with_colony(1, 1000, Coordinates{0, 0})
      .build();

  // 4. Setup Star 2 (Vega) at (300, 400) -> distance 500 from Sol
  create_star("Vega", 2)
      .with_position({300.0, 400.0})
      .with_stability(45)
      .with_gravity(1.0)
      .with_temperature(40)
      .with_ap(1, 100)
      .with_ap(2, 100)
      .with_ap(3, 100)
      .with_ap(4, 100)
      .with_explored(1)
      .with_explored(2)
      .with_explored(3)
      .with_explored(4)
      .with_inhabited(1)
      .with_inhabited(2)
      .with_inhabited(3)
      .with_inhabited(4)
      .build();

  // 5. Setup Planet 1 on Star 2 (Vega Prime)
  create_planet(2, PlanetType::EARTH, Coordinates{10, 10}, 1)
      .named("Vega Prime")
      .with_position(SystemCoordinates{100.0, 0.0})
      .with_stockpiles(1, 1000, 1000, 1000)
      .with_stockpiles(2, 1000, 1000, 1000)
      .with_stockpiles(3, 1000, 1000, 1000)
      .with_stockpiles(4, 1000, 1000, 1000)
      .with_tax(1, 10)
      .with_tax(2, 10)
      .with_tax(3, 10)
      .with_tax(4, 10)
      .with_explored(1, true)
      .with_explored(2, true)
      .with_explored(3, true)
      .with_explored(4, true)
      .with_colony(2, 1000, Coordinates{0, 0})
      .build();

  // 6. Setup Star 3 (Antares) at (-300, -400) -> distance 500 from Sol, 1000
  // from Vega
  create_star("Antares", 3)
      .with_position({-300.0, -400.0})
      .with_stability(25)
      .with_gravity(1.2)
      .with_temperature(60)
      .with_ap(1, 100)
      .with_ap(2, 100)
      .with_ap(3, 100)
      .with_ap(4, 100)
      .with_explored(1)
      .with_explored(2)
      .with_explored(3)
      .with_explored(4)
      .with_inhabited(1)
      .with_inhabited(2)
      .with_inhabited(3)
      .with_inhabited(4)
      .build();

  // 7. Setup Planet 1 on Star 3 (Antares Prime)
  create_planet(3, PlanetType::EARTH, Coordinates{10, 10}, 1)
      .named("Antares Prime")
      .with_position(SystemCoordinates{100.0, 0.0})
      .with_stockpiles(1, 1000, 1000, 1000)
      .with_stockpiles(2, 1000, 1000, 1000)
      .with_stockpiles(3, 1000, 1000, 1000)
      .with_stockpiles(4, 1000, 1000, 1000)
      .with_tax(1, 10)
      .with_tax(2, 10)
      .with_tax(3, 10)
      .with_tax(4, 10)
      .with_explored(1, true)
      .with_explored(2, true)
      .with_explored(3, true)
      .with_explored(4, true)
      .with_colony(1, 1000, Coordinates{0, 0})
      .build();

  // 8. Setup Universe record with 100 AP for all 4 races
  UniverseRepository univ_repo(store);
  auto u = univ_repo.find(1);
  if (!u) {
    universe_struct new_u{};
    for (player_t p = 1; p <= 4; ++p) {
      new_u.AP[p] = 100;
    }
    univ_repo.save(new_u);
  } else {
    for (player_t p = 1; p <= 4; ++p) {
      u->AP[p] = 100;
    }
    univ_repo.save(*u);
  }

  em.clear_cache();
  return *this;
}

TestContext& TestContext::with_populated_planet(starnum_t snum,
                                                planetnum_t pnum,
                                                player_t owner,
                                                population_t popn,
                                                Coordinates capital_coords) {
  em.mutate_sectormap(snum, pnum, [&](SectorMap& smap) {
    smap.get(capital_coords).colonize(owner, popn);
    smap.get(capital_coords).set_condition(SectorType::SEC_LAND);
    smap.get(capital_coords).set_fert(100);
    smap.get(capital_coords).set_resource(100);
    smap.get(capital_coords).set_efficiency_bounded(100);
  });

  em.mutate_planet(snum, pnum, [&](Planet& p) {
    const auto* smap = em.peek_sectormap(snum, pnum);
    p.sync_demographics(*smap);
  });

  return *this;
}

TestStarBuilder
TestContext::create_star(std::string_view name,
                         std::optional<starnum_t> explicit_snum) {
  return TestStarBuilder(*this, name, explicit_snum);
}

TestPlanetBuilder
TestContext::create_planet(starnum_t snum, PlanetType type, Coordinates dims,
                           std::optional<planetnum_t> explicit_pnum) {
  return TestPlanetBuilder(*this, snum, type, dims, explicit_pnum);
}

TestContext&
TestContext::with_universe(std::optional<GB::creator::UniverseConfig> config) {
  db = Database(":memory:");
  GB::creator::UniverseConfig cfg = config.value_or(GB::creator::UniverseConfig{
      .num_stars = 3,
      .min_planets = 1,
      .max_planets = 3,
      .planetless_chance_percent = 0,
      .auto_name_stars = true,
      .auto_name_planets = true,
      .print_star_info = false,
      .print_planet_info = false,
  });

  GB::creator::UniverseGenerator generator(cfg);
  generator.generate(db);

  JsonStore store(db);

  // Setup standard races via EnrollmentService::build_race
  seed_standard_races(em);

  // Mark all generated stars explored and with 100 AP
  StarRepository star_repo(store);
  for (starnum_t snum = 1; snum <= cfg.num_stars; ++snum) {
    auto star_opt = star_repo.find(snum);
    if (star_opt) {
      for (player_t p = 1; p <= 4; ++p) {
        star_opt->mark_explored_by(p);
        star_opt->mark_inhabited_by(p);
        star_opt->AP(p) = 100;
      }
      star_repo.save(*star_opt);
    }
  }

  // Set universe AP
  UniverseRepository univ_repo(store);
  auto u = univ_repo.find(1);
  if (u) {
    for (player_t p = 1; p <= 4; ++p) {
      u->AP[p] = 100;
    }
    univ_repo.save(*u);
  }

  em.clear_cache();
  return *this;
}
