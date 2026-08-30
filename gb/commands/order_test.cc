// SPDX-License-Identifier: Apache-2.0

/// \file order_test.cc
/// \brief Unit tests for order command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.governor[0].active = true;

  // Save race via repository
  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  // Create a test ship with default orders - use battleship which can bombard
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.alive() = true;
  ship.active() = true;
  ship.type() = ShipType::STYPE_BATTLE;  // Battleship can bombard
  ship.name() = "TestShip";
  ship.speed() = 5;
  ship.max_speed() = 9;
  ship.max_crew() = 100;
  ship.popn() = 100;  // Crew

  // Save ship via repository
  ShipRepository ships_repo(store);
  ships_repo.save(ship);
}

void test_order_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  std::println(std::cout, "Set ship defense order");
  {
    ctx.assert_dispatch_success(g, {"order", "#1", "defense", "on"});

    // Force reload from database
    ctx.em.clear_cache();

    // Verify defense order was set
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_true(saved_ship->protect().planet);
    std::println(std::cout, "    ✓ Defense order set: protect.planet={}",
                 saved_ship->protect().planet);
  }

  std::println(std::cout, "\nTest 2: Turn defense order off");
  {
    ctx.assert_dispatch_success(g, {"order", "#1", "defense", "off"});

    // Force reload from database
    ctx.em.clear_cache();

    // Verify defense was turned off
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_false(saved_ship->protect().planet);
    std::println(std::cout, "    ✓ Defense order turned off: protect.planet={}",
                 saved_ship->protect().planet);
  }

  std::println(std::cout, "\nTest 3: Set navigation order");
  {
    ctx.assert_dispatch_success(g, {"order", "#1", "navigate", "270", "4"});

    ctx.em.clear_cache();
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_true(saved_ship->navigate().on);
    test::expect_eq(saved_ship->navigate().bearing, 270U);
    test::expect_eq(saved_ship->navigate().turns, 4U);
    std::println(std::cout, "    ✓ Navigation order set: bearing=270, turns=4");
  }

  std::println(std::cout, "\nTest 4: Turn navigation order off");
  {
    ctx.assert_dispatch_success(g, {"order", "#1", "navigate", "off"});

    ctx.em.clear_cache();
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_false(saved_ship->navigate().on);
    std::println(std::cout, "    ✓ Navigation order turned off");
  }

  std::println(std::cout, "\nTest 5: Set evasion order");
  {
    ctx.assert_dispatch_success(g, {"order", "#1", "evade", "on"});

    ctx.em.clear_cache();
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_true(saved_ship->protect().evade);
    std::println(std::cout, "    ✓ Evasion order turned on");
  }

  std::println(std::cout, "\nTest 6: Set retaliation order");
  {
    ctx.assert_dispatch_success(g, {"order", "#1", "retaliate", "on"});

    ctx.em.clear_cache();
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_true(saved_ship->protect().self);
    std::println(std::cout, "    ✓ Retaliation order turned on");
  }

  std::println(std::cout, "\nTest 7: Display all orders (no modifications)");
  {
    ctx.assert_dispatch_success(g, {"order"});

    // Verify ship state unchanged
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_false(saved_ship->protect().planet);
    std::println(std::cout, "    ✓ Display orders works without modification");
  }
}

void test_order_specialty_ships() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  // 1. Missile orders (impact and scatter)
  ship_struct missile_data{};
  missile_data.number = 10;
  missile_data.owner = 1;
  missile_data.governor = 0;
  missile_data.alive = true;
  missile_data.active = true;
  missile_data.type = ShipType::STYPE_MISSILE;
  missile_data.name = "Tomahawk";
  missile_data.whatdest = ScopeLevel::LEVEL_PLAN;
  auto missile_handle = ctx.em.create_ship(missile_data);
  const auto missile_id = missile_handle->number();

  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", missile_id.value), "impact", "12,34"});
  ctx.em.clear_cache();
  const auto* missile = ctx.em.peek_ship(missile_id)->as<MissileShip>();
  test::expect_ne(missile, nullptr);
  test::expect_eq(missile->impact_coords(), (Coordinates{12, 34}));
  test::expect_false(missile->is_scatter());

  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", missile_id.value), "scatter"});
  ctx.em.clear_cache();
  missile = ctx.em.peek_ship(missile_id)->as<MissileShip>();
  test::expect_ne(missile, nullptr);
  test::expect_true(missile->is_scatter());

  // 2. Mine orders (trigger radius, explosive, radiative)
  ship_struct mine_data{};
  mine_data.number = 20;
  mine_data.owner = 1;
  mine_data.governor = 0;
  mine_data.alive = true;
  mine_data.active = true;
  mine_data.type = ShipType::STYPE_MINE;
  mine_data.name = "ProximityMine";
  auto mine_handle = ctx.em.create_ship(mine_data);
  const auto mine_id = mine_handle->number();

  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", mine_id.value), "trigger", "15"});
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", mine_id.value), "radiative"});
  ctx.em.clear_cache();
  const auto* mine = ctx.em.peek_ship(mine_id)->as<MineShip>();
  test::expect_ne(mine, nullptr);
  test::expect_eq(mine->trigger_radius(), 15U);
  test::expect_true(mine->is_radiative());

  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", mine_id.value), "explosive"});
  ctx.em.clear_cache();
  mine = ctx.em.peek_ship(mine_id)->as<MineShip>();
  test::expect_ne(mine, nullptr);
  test::expect_false(mine->is_radiative());

  // 3. Transporter orders (target ship)
  ship_struct trans_data{};
  trans_data.number = 30;
  trans_data.owner = 1;
  trans_data.governor = 0;
  trans_data.alive = true;
  trans_data.active = true;
  trans_data.type = ShipType::OTYPE_TRANSDEV;
  trans_data.name = "Transporter";
  auto trans_handle = ctx.em.create_ship(trans_data);
  const auto trans_id = trans_handle->number();

  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", trans_id.value), "transport", "1"});
  ctx.em.clear_cache();
  const auto* trans = ctx.em.peek_ship(trans_id)->as<TransporterShip>();
  test::expect_ne(trans, nullptr);
  test::expect_eq(trans->target_ship(), shipnum_t{1});

  // 4. Space Mirror intensity
  ship_struct mirror_data{};
  mirror_data.number = 40;
  mirror_data.owner = 1;
  mirror_data.governor = 0;
  mirror_data.alive = true;
  mirror_data.active = true;
  mirror_data.type = ShipType::STYPE_MIRROR;
  mirror_data.name = "Helios";
  auto mirror_handle = ctx.em.create_ship(mirror_data);
  const auto mirror_id = mirror_handle->number();

  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", mirror_id.value), "intensity", "85"});
  ctx.em.clear_cache();
  const auto* mirror = ctx.em.peek_ship(mirror_id)->as<SpaceMirrorShip>();
  test::expect_ne(mirror, nullptr);
  test::expect_eq(mirror->intensity(), 85);

  // 5. Terraformer move sequence
  ship_struct terra_data{};
  terra_data.number = 50;
  terra_data.owner = 1;
  terra_data.governor = 0;
  terra_data.alive = true;
  terra_data.active = true;
  terra_data.type = ShipType::OTYPE_TERRA;
  terra_data.name = "TerraDev";
  auto terra_handle = ctx.em.create_ship(terra_data);
  const auto terra_id = terra_handle->number();

  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", terra_id.value), "move", "1234c"});
  ctx.em.clear_cache();
  const auto* terra = ctx.em.peek_ship(terra_id)->as<TerraformerShip>();
  test::expect_ne(terra, nullptr);
  test::expect_eq(terra->shipclass(), "1234c");
  test::expect_eq(terra->index(), 0U);

  std::println(std::cout, "    ✓ Specialty ship orders verified");
}

}  // namespace

int main() {
  test_order_happy_path();
  test_order_specialty_ships();

  std::println(std::cout, "All order tests passed!");
  return 0;
}
