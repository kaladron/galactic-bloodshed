// SPDX-License-Identifier: Apache-2.0

/// \file scrap_test.cc
/// \brief Unit tests for scrap command

import commands;
import dallib;
import gblib;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.governor[0].active = true;
  race.mass = 1.0;
  race.fighters = 1.0;
  race.tech = 100.0;
  race.morale = 100;
  RaceRepository races(store);
  races.save(race);

  star_struct ss{};
  ss.star_id = 1;
  ss.name = "TestStar";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.explored = (1ULL << 1);
  ss.AP[0] = 10;
  Star star(ss);
  StarRepository stars_repo(store);
  stars_repo.save(star);

  Planet planet{PlanetType::EARTH, Coordinates{10, 10}};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  planet.popn() = 1000;
  planet.info(player_t{1}).numsectsowned = 1;
  planet.info(player_t{1}).popn = 1000;
  planet.info(player_t{1}).resource = 500;
  PlanetRepository planets_repo(store);
  planets_repo.save(planet);

  SectorMap smap(planet);
  auto& sector = smap.get(Coordinates{5, 5});
  sector.set_owner(1);
  sector.set_popn_exact(100);
  sector.set_resource(50);
  sector.set_efficiency_bounded(100);
  SectorRepository sector_repo(store);
  sector_repo.save_map(smap);

  Ship ship1{};
  ship1.number() = 1;
  ship1.owner() = 1;
  ship1.governor() = 0;
  ship1.alive() = true;
  ship1.active() = true;
  ship1.type() = ShipType::STYPE_CARRIER;
  ship1.name() = "Carrier";
  ship1.whatorbits() = ScopeLevel::LEVEL_STAR;
  ship1.storbits() = 1;
  ship1.xpos() = 100.0;
  ship1.ypos() = 200.0;
  ship1.fuel() = 100.0;
  ship1.max_fuel() = 500.0;
  ship1.resource() = 100;
  ship1.max_resource() = 1000;
  ship1.popn() = 10;
  ship1.max_crew() = 100;
  ship1.destruct() = 0;
  ship1.max_destruct() = 100;
  ship1.mass() = 100.0;
  ship1.docked() = 1;
  ship1.whatdest() = ScopeLevel::LEVEL_SHIP;
  ship1.destshipno() = 2;

  Ship ship2{};
  ship2.number() = 2;
  ship2.owner() = 1;
  ship2.governor() = 0;
  ship2.alive() = true;
  ship2.active() = true;
  ship2.type() = ShipType::STYPE_FIGHTER;
  ship2.build_type() = ShipType::STYPE_FIGHTER;
  ship2.name() = "ToScrap";
  ship2.whatorbits() = ScopeLevel::LEVEL_STAR;
  ship2.storbits() = 1;
  ship2.xpos() = 100.0;
  ship2.ypos() = 200.0;
  ship2.fuel() = 50.0;
  ship2.max_fuel() = 100.0;
  ship2.resource() = 20;
  ship2.max_resource() = 50;
  ship2.popn() = 5;
  ship2.max_crew() = 10;
  ship2.destruct() = 10;
  ship2.max_destruct() = 20;
  ship2.mass() = 10.0;
  ship2.build_cost() = 100;
  ship2.docked() = 1;
  ship2.whatdest() = ScopeLevel::LEVEL_SHIP;
  ship2.destshipno() = 1;

  ShipRepository ships_repo(store);
  ships_repo.save(ship1);
  ships_repo.save(ship2);
}

void test_scrap_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1. Scrap docked fighter (1 AP deducted via dynamic AP)
  ctx.assert_dispatch_success(g, {"scrap", "#2"}, 1);

  ctx.em.clear_cache();
  const auto* scrapped = ctx.em.peek_ship(2);
  test::expect_ne(scrapped, nullptr);
  test::expect_eq(scrapped->alive(), 0);

  const auto* carrier_after = ctx.em.peek_ship(1);
  test::expect_ne(carrier_after, nullptr);
  test::expect_gt(carrier_after->resource(), 100);
  test::expect_eq(carrier_after->docked(), 0);
}

void test_scrap_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set Star AP to 0
  {
    auto star_handle = ctx.em.get_star(1);
    star_handle->AP(1) = 0;
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  ctx.assert_dispatch_rejected(g, {"scrap", "#2"});
  test::expect_contains(g.out.str(), "action points");
}

void test_scrap_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"scrap"});
  test::expect_contains(g.out.str(), "Syntax: scrap <ship>");

  // 2. Uncrewed ship rejection
  {
    auto s2 = ctx.em.get_ship(2);
    s2->popn() = 0;
  }
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"scrap", "#2"});
  test::expect_contains(g.out.str(), "no crew");
}

}  // namespace

int main() {
  test_scrap_happy_paths();
  test_scrap_insufficient_ap();
  test_scrap_domain_errors();

  std::println(std::cout, "✓ scrap_test passed!");
  return 0;
}
