// SPDX-License-Identifier: Apache-2.0

/// \file give_test.cc
/// \brief Test give command functionality, ship ownership transfer, and
/// validation rules.

import dallib;
import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_give_dispatch() {
  std::println(std::cout, "Test: give command dispatch and ship transfer");
  TestContext ctx;
  JsonStore store(ctx.db);

  // Create two test races - one giving, one receiving
  Race race1{};
  race1.Playernum = 1;
  race1.governor[0].active = true;
  race1.name = "Giver";
  race1.Guest = false;
  race1.God = false;
  race1.declare_alliance_with(player_t{2});  // Mutually allied with race 2

  Race race2{};
  race2.Playernum = 2;
  race2.governor[0].active = true;
  race2.name = "Receiver";
  race2.Guest = false;
  race2.God = false;
  race2.declare_alliance_with(player_t{1});  // Mutually allied with race 1

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  // Create a test star
  star_struct star_data{};
  star_data.star_id = 1;
  star_data.governor[player_t{1}] = 0;
  star_data.name = "TestStar";
  star_data.xpos = 100.0;
  star_data.ypos = 100.0;
  star_data.pnames = {"TestPlanet"};
  Star star{star_data};
  star.AP(player_t{1}) = 100;
  star.mark_explored_by(player_t{1});
  StarRepository stars_repo(store);
  stars_repo.save(star);
  const starnum_t star_id = star_data.star_id;

  // Create a test planet
  Planet planet{};
  planet.star_id() = star_id;
  planet.planet_order() = 0;
  planet.dimensions() = Coordinates{10, 10};
  PlanetRepository planets_repo(store);
  planets_repo.save(planet);

  // Create a test ship owned by race 1
  const shipnum_t ship_id = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                                .owned_by(1, 0)
                                .with_alive(true)
                                .in_planet_orbit(star_id, 0)
                                .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(star_id);
  g.set_pnum(0);

  // 1. Happy path: give ship to allied race
  ctx.assert_dispatch_success(
      g, {"give", "Receiver", std::format("#{}", ship_id.value)});
  test::expect_contains(g.out.str(), "Owner changed.");

  // Verify ownership changed
  ctx.em.clear_cache();
  const auto* transferred = ctx.em.peek_ship(ship_id);
  test::expect_ne(transferred, nullptr);
  test::expect_eq(transferred->owner(), 2);

  const auto* planet_verify = ctx.em.peek_planet(star_id, 0);
  test::expect_ne(planet_verify, nullptr);
  test::expect_eq(planet_verify->info(player_t{2}).explored, 1);

  // Verify recipient explored the system
  const auto* star_verify = ctx.em.peek_star(star_id);
  test::expect_ne(star_verify, nullptr);
  test::expect_true(star_verify->is_explored_by(player_t{2}));
  std::println(std::cout, "    ✓ Ship ownership transferred to ally");

  // 2. Non-leader governor rejected
  const shipnum_t ship2_id = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                                 .owned_by(1, 0)
                                 .with_alive(true)
                                 .in_planet_orbit(star_id, 0)
                                 .build();

  g.set_governor(1);
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"give", "Receiver", std::format("#{}", ship2_id.value)});
  test::expect_contains(g.out.str(),
                        "Only the leader (Governor 0) may use this command.");
  std::println(std::cout, "    ✓ Governor rejection verified");

  // 3. Crewed ship cannot be given away
  g.set_governor(0);
  const shipnum_t ship3_id = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                                 .owned_by(1, 0)
                                 .with_alive(true)
                                 .in_planet_orbit(star_id, 0)
                                 .with_crew(10, 0)
                                 .build();

  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"give", "Receiver", std::format("#{}", ship3_id.value)});
  test::expect_contains(g.out.str(), "crew/mil on board");
  std::println(std::cout, "    ✓ Crewed ship rejection verified");
}

}  // namespace

int main() {
  test_give_dispatch();
  std::println(std::cout, "\n✅ All give command tests passed!");
  return 0;
}
