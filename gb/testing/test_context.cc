// SPDX-License-Identifier: Apache-2.0

/// \file test_context.cc
/// \brief Implementation of TestContext fixture and dispatch assertion helpers.

module;

#include <cassert>

module test;

import commands;
import dallib;
import gb.entities;
import gb.services;
import gb.repositories;
import gb.creator;
import std;

TestContext::TestContext() : db(":memory:"), em(db) {
  initialize_schema(db);
  universe_struct u{};
  u.id = 1;
  JsonStore store(db);
  UniverseRepository universe_repo(store);
  universe_repo.save(u);

  Race default_race{};
  default_race.Playernum = 1;
  default_race.name = "TestRace";
  default_race.governor[0].active = true;
  RaceRepository race_repo(store);
  race_repo.save(default_race);
}

void TestContext::setup_game_obj(GameObj& g, player_t player, governor_t gov) {
  g.set_player(player);
  g.set_governor(gov);
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

  // 1. Setup standard races: Player 1 (Federation) and Player 2 (Klingons)
  RaceRepository race_repo(store);
  Race r1{};
  r1.Playernum = 1;
  r1.name = "Federation";
  r1.tech = 100.0;
  r1.Guest = false;
  r1.governor[0].active = true;
  r1.governor[0].money = 10'000;
  r1.Gov_ship = 100;
  r1.mass = 1.0;
  r1.metabolism = 1.0;
  race_repo.save(r1);

  Race r2{};
  r2.Playernum = 2;
  r2.name = "Klingons";
  r2.tech = 100.0;
  r2.Guest = false;
  r2.governor[0].active = true;
  r2.governor[0].money = 10'000;
  r2.mass = 1.0;
  r2.metabolism = 1.0;
  race_repo.save(r2);

  // 2. Setup Star 0 (Sol) with 100 AP for both races, explored and inhabited
  star_struct ss0{};
  ss0.star_id = 0;
  ss0.name = "Sol";
  ss0.xpos = 0.0;
  ss0.ypos = 0.0;
  ss0.stability = 15;
  ss0.gravity = 1.0;
  ss0.temperature = 50;
  ss0.AP[player_t{1}] = 100;
  ss0.AP[player_t{2}] = 100;
  ss0.pnames.push_back("Earth");
  Star star0{ss0};
  star0.mark_explored_by(player_t{1});
  star0.mark_explored_by(player_t{2});
  star0.mark_inhabited_by(player_t{1});
  star0.mark_inhabited_by(player_t{2});
  StarRepository(store).save(star0);

  // 3. Setup Planet 0 on Star 0 (Earth)
  Planet planet0{PlanetType::EARTH, Coordinates{10, 10}};
  planet0.star_id() = 0;
  planet0.planet_order() = 0;
  planet0.xpos() = 100.0;
  planet0.ypos() = 0.0;
  planet0.explored() = true;
  for (player_t pid : {player_t{1}, player_t{2}}) {
    planet0.info(pid).explored = 1;
    planet0.info(pid).destruct = 1000;
    planet0.info(pid).fuel = 1000;
    planet0.info(pid).resource = 1000;
    planet0.info(pid).tax = 10;
    planet0.info(pid).newtax = 10;
  }
  PlanetRepository(store).save(planet0);

  // 4. Setup SectorMap for Earth with valid coordinates
  SectorMap smap0(planet0);
  for (int y = 0; y < 10; ++y) {
    for (int x = 0; x < 10; ++x) {
      smap0.get(Coordinates{x, y}).set_x(x);
      smap0.get(Coordinates{x, y}).set_y(y);
    }
  }
  SectorRepository(store).save_map(smap0);

  // 5. Setup Star 1 (Vega) at (300, 400) -> distance 500 from Sol
  star_struct ss1{};
  ss1.star_id = 1;
  ss1.name = "Vega";
  ss1.xpos = 300.0;
  ss1.ypos = 400.0;
  ss1.stability = 45;
  ss1.gravity = 1.0;
  ss1.temperature = 40;
  ss1.AP[player_t{1}] = 100;
  ss1.AP[player_t{2}] = 100;
  ss1.pnames.push_back("Vega Prime");
  Star star1{ss1};
  star1.mark_explored_by(player_t{1});
  star1.mark_explored_by(player_t{2});
  star1.mark_inhabited_by(player_t{1});
  star1.mark_inhabited_by(player_t{2});
  StarRepository(store).save(star1);

  // 6. Setup Planet 0 on Star 1 (Vega Prime)
  Planet planet1{PlanetType::EARTH, Coordinates{10, 10}};
  planet1.star_id() = 1;
  planet1.planet_order() = 0;
  planet1.xpos() = 100.0;
  planet1.ypos() = 0.0;
  planet1.explored() = true;
  for (player_t pid : {player_t{1}, player_t{2}}) {
    planet1.info(pid).explored = 1;
    planet1.info(pid).destruct = 1000;
    planet1.info(pid).fuel = 1000;
    planet1.info(pid).resource = 1000;
    planet1.info(pid).tax = 10;
    planet1.info(pid).newtax = 10;
  }
  PlanetRepository(store).save(planet1);

  // 7. Setup SectorMap for Vega Prime with valid coordinates
  SectorMap smap1(planet1);
  for (int y = 0; y < 10; ++y) {
    for (int x = 0; x < 10; ++x) {
      smap1.get(Coordinates{x, y}).set_x(x);
      smap1.get(Coordinates{x, y}).set_y(y);
    }
  }
  SectorRepository(store).save_map(smap1);

  // 8. Setup Universe record with numstars = 2, 100 AP for both races
  UniverseRepository univ_repo(store);
  auto u = univ_repo.find(1);
  if (!u) {
    universe_struct new_u{};
    new_u.id = 1;
    new_u.numstars = 2;
    new_u.AP[player_t{1}] = 100;
    new_u.AP[player_t{2}] = 100;
    univ_repo.save(new_u);
  } else {
    u->numstars = std::max(u->numstars, 2u);
    u->AP[player_t{1}] = 100;
    u->AP[player_t{2}] = 100;
    univ_repo.save(*u);
  }

  return *this;
}

TestContext& TestContext::with_populated_planet(starnum_t snum,
                                                planetnum_t pnum,
                                                player_t owner,
                                                population_t popn,
                                                Coordinates capital_coords) {
  em.mutate_planet(snum, pnum, [&](Planet& p) {
    p.popn() = popn;
    p.info(owner).numsectsowned = 1;
  });

  em.mutate_sectormap(snum, pnum, [&](SectorMap& smap) {
    smap.get(capital_coords).colonize(owner, popn);
    smap.get(capital_coords).set_condition(SectorType::SEC_LAND);
    smap.get(capital_coords).set_fert(100);
    smap.get(capital_coords).set_resource(100);
    smap.get(capital_coords).set_efficiency_bounded(100);
  });

  return *this;
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

  // Setup standard races
  RaceRepository race_repo(store);
  Race r1{};
  r1.Playernum = 1;
  r1.name = "Federation";
  r1.tech = 100.0;
  r1.Guest = false;
  r1.governor[0].active = true;
  r1.governor[0].money = 10'000;
  r1.Gov_ship = 100;
  r1.mass = 1.0;
  r1.metabolism = 1.0;
  race_repo.save(r1);

  Race r2{};
  r2.Playernum = 2;
  r2.name = "Klingons";
  r2.tech = 100.0;
  r2.Guest = false;
  r2.governor[0].active = true;
  r2.governor[0].money = 10'000;
  r2.mass = 1.0;
  r2.metabolism = 1.0;
  race_repo.save(r2);

  // Mark all generated stars explored and with 100 AP
  StarRepository star_repo(store);
  for (starnum_t snum = 0; snum < cfg.num_stars; ++snum) {
    auto star_opt = star_repo.find(snum);
    if (star_opt) {
      star_opt->mark_explored_by(player_t{1});
      star_opt->mark_explored_by(player_t{2});
      star_opt->mark_inhabited_by(player_t{1});
      star_opt->mark_inhabited_by(player_t{2});
      star_opt->AP(player_t{1}) = 100;
      star_opt->AP(player_t{2}) = 100;
      star_repo.save(*star_opt);
    }
  }

  // Set universe AP
  UniverseRepository univ_repo(store);
  auto u = univ_repo.find(1);
  if (u) {
    u->AP[player_t{1}] = 100;
    u->AP[player_t{2}] = 100;
    univ_repo.save(*u);
  }

  return *this;
}
