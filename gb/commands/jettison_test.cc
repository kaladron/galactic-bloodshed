// SPDX-License-Identifier: Apache-2.0

/// \file jettison_test.cc
/// \brief Unit tests for jettison command

import commands;
import dallib;
import gblib;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "Jettisoner";
  race.Guest = false;
  race.governor[0].active = true;
  race.mass = 1.0;  // Used for crew/troop mass calculations

  RaceRepository races(store);
  races.save(race);

  // Create test star
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "JettisonStar";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.AP[0] = 10;
  Star star(ss);

  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create a ship with cargo
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.alive() = true;
  ship.active() = true;
  ship.type() = ShipType::STYPE_SHUTTLE;
  ship.name() = "CargoShip";
  ship.whatorbits() = ScopeLevel::LEVEL_STAR;
  ship.storbits() = 0;
  ship.fuel() = 100.0;
  ship.resource() = 50;
  ship.destruct() = 20;
  ship.crystals() = 5;
  ship.popn() = 10;
  ship.troops() = 8;
  ship.mass() = 100.0;

  ShipRepository ships_repo(store);
  ships_repo.save(ship);
}

void test_jettison_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);
  g.set_shipno(1);

  std::println(std::cout, "Jettison crystals");
  {
    const auto* s_before = ctx.em.peek_ship(1);
    test::expect_ne(s_before, nullptr);
    int initial_crystals = s_before->crystals();

    ctx.assert_dispatch_success(g, {"jettison", "#1", "x", "3"});

    const auto* s_after = ctx.em.peek_ship(1);
    test::expect_ne(s_after, nullptr);
    test::expect_eq(s_after->crystals(), initial_crystals - 3);
    std::println(std::cout, "✓ Crystals jettisoned");
  }

  std::println(std::cout, "Jettison crew");
  {
    const auto* s_before = ctx.em.peek_ship(1);
    test::expect_ne(s_before, nullptr);
    int initial_popn = s_before->popn();
    double initial_mass = s_before->mass();

    ctx.assert_dispatch_success(g, {"jettison", "#1", "c", "5"});

    const auto* s_after = ctx.em.peek_ship(1);
    test::expect_ne(s_after, nullptr);
    test::expect_eq(s_after->popn(), initial_popn - 5);
    test::expect_eq(s_after->mass(), initial_mass - 5.0);
    std::println(std::cout, "✓ Crew jettisoned with mass reduction");
  }

  std::println(std::cout, "Jettison military");
  {
    const auto* s_before = ctx.em.peek_ship(1);
    test::expect_ne(s_before, nullptr);
    int initial_troops = s_before->troops();
    double initial_mass = s_before->mass();

    ctx.assert_dispatch_success(g, {"jettison", "#1", "m", "4"});

    const auto* s_after = ctx.em.peek_ship(1);
    test::expect_ne(s_after, nullptr);
    test::expect_eq(s_after->troops(), initial_troops - 4);
    test::expect_eq(s_after->mass(), initial_mass - 4.0);
    std::println(std::cout, "✓ Military jettisoned with mass reduction");
  }

  std::println(std::cout, "Jettison destruct");
  {
    const auto* s_before = ctx.em.peek_ship(1);
    test::expect_ne(s_before, nullptr);
    int initial_destruct = s_before->destruct();

    ctx.assert_dispatch_success(g, {"jettison", "#1", "d", "10"});

    const auto* s_after = ctx.em.peek_ship(1);
    test::expect_ne(s_after, nullptr);
    test::expect_eq(s_after->destruct(), initial_destruct - 10);
    std::println(std::cout, "✓ Destruct jettisoned");
  }

  std::println(std::cout, "Jettison fuel");
  {
    const auto* s_before = ctx.em.peek_ship(1);
    test::expect_ne(s_before, nullptr);
    double initial_fuel = s_before->fuel();

    ctx.assert_dispatch_success(g, {"jettison", "#1", "f", "25"});

    const auto* s_after = ctx.em.peek_ship(1);
    test::expect_ne(s_after, nullptr);
    test::expect_eq(s_after->fuel(), initial_fuel - 25);
    std::println(std::cout, "✓ Fuel jettisoned");
  }

  std::println(std::cout, "Jettison resources");
  {
    const auto* s_before = ctx.em.peek_ship(1);
    test::expect_ne(s_before, nullptr);
    int initial_resource = s_before->resource();

    ctx.assert_dispatch_success(g, {"jettison", "#1", "r", "30"});

    const auto* s_after = ctx.em.peek_ship(1);
    test::expect_ne(s_after, nullptr);
    test::expect_eq(s_after->resource(), initial_resource - 30);
    std::println(std::cout, "✓ Resources jettisoned");
  }
}

void test_jettison_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);
  g.set_shipno(1);

  // 1. Min args check (< 3 args)
  ctx.assert_dispatch_rejected(g, {"jettison"});
  test::expect_contains(g.out.str(),
                        "Syntax: jettison <ship> <commodity> [<amount>]");

  // 2. Unknown commodity
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"jettison", "#1", "z", "10"});
  test::expect_contains(g.out.str(), "No such commodity valid");

  // 3. Jettison when landed
  {
    auto ship_handle = ctx.em.get_ship(1);
    ship_handle->docked() = true;
    ship_handle->whatorbits() = ScopeLevel::LEVEL_PLAN;
    ship_handle->whatdest() = ScopeLevel::LEVEL_PLAN;
  }
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"jettison", "#1", "r", "10"});
  test::expect_contains(g.out.str(), "Ship is landed, cannot jettison");
}

}  // namespace

int main() {
  test_jettison_happy_path();
  test_jettison_domain_errors();

  std::println(std::cout, "All jettison tests passed!");
  return 0;
}
