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
  setbit<std::uint64_t>(race1.allied, 2U);  // Mutually allied with race 2

  Race race2{};
  race2.Playernum = 2;
  race2.governor[0].active = true;
  race2.name = "Receiver";
  race2.Guest = false;
  race2.God = false;
  setbit<std::uint64_t>(race2.allied, 1U);  // Mutually allied with race 1

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  // Create a test star
  star_struct star_data{};
  star_data.star_id = 1;
  star_data.governor[0] = 0;
  star_data.name = "TestStar";
  star_data.xpos = 100.0;
  star_data.ypos = 100.0;
  star_data.pnames = {"TestPlanet"};
  Star star{star_data};
  star.AP(player_t{1}) = 100;
  setbit<std::uint64_t>(star.explored(), 1U);
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
  Ship ship{};
  ship.owner() = 1;
  ship.governor() = 0;
  ship.type() = ShipType::OTYPE_PROBE;
  ship.alive() = 1;
  ship.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship.storbits() = star_id;
  ship.pnumorbits() = 0;
  ship.popn() = 0;
  ship.troops() = 0;
  ship.ships() = 0;
  ShipRepository ships_repo(store);
  ships_repo.save(ship);
  const shipnum_t ship_id = ship.number();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(star_id);
  g.set_pnum(0);

  // 1. Give ship to allied player
  ctx.assert_dispatch_success(
      g, {"give", "Receiver", std::format("#{}", ship_id.value)});
  const auto* ship_verify = ctx.em.peek_ship(ship_id);
  test::expect_ne(ship_verify, nullptr);
  test::expect_eq(ship_verify->owner(), 2);
  test::expect_eq(ship_verify->governor(), 0);

  const auto* planet_verify = ctx.em.peek_planet(star_id, 0);
  test::expect_ne(planet_verify, nullptr);
  test::expect_eq(planet_verify->info(player_t{2}).explored, 1);

  const auto* star_verify = ctx.em.peek_star(star_id);
  test::expect_ne(star_verify, nullptr);
  test::expect_true(star_verify->is_explored_by(player_t{2}));
  std::println(std::cout, "    ✓ Ship ownership transferred to ally");

  // 2. Non-leader governor rejected
  auto ship2_handle = ctx.em.create_ship();
  auto& ship2 = *ship2_handle;
  ship2.owner() = 1;
  ship2.governor() = 0;
  ship2.type() = ShipType::OTYPE_PROBE;
  ship2.alive() = 1;
  ship2.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship2.storbits() = star_id;
  ship2.pnumorbits() = 0;
  const shipnum_t ship2_id = ship2.number();

  g.set_governor(1);
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"give", "Receiver", std::format("#{}", ship2_id.value)});
  test::expect_contains(g.out.str(),
                        "Only the leader (Governor 0) may use this command.");
  std::println(std::cout, "    ✓ Governor rejection verified");

  // 3. Crewed ship cannot be given away
  g.set_governor(0);
  auto ship3_handle = ctx.em.create_ship();
  auto& ship3 = *ship3_handle;
  ship3.owner() = 1;
  ship3.governor() = 0;
  ship3.type() = ShipType::OTYPE_PROBE;
  ship3.alive() = 1;
  ship3.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship3.storbits() = star_id;
  ship3.pnumorbits() = 0;
  ship3.popn() = 10;
  const shipnum_t ship3_id = ship3.number();

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
