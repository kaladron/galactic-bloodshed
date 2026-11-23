// SPDX-License-Identifier: Apache-2.0

/// \file build_test.cc
/// \brief Unit tests for build command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

constexpr player_t builder_player = 2;
constexpr player_t rival_player = 3;

void setup_test_world(TestContext& ctx) {
  ctx.with_galaxy();
  TestWorldBuilder world(ctx);
  world.add_race("Builders", 500.0, false, builder_player);
  world.add_race("Rivals", 100.0, false, rival_player);
  ctx.with_populated_planet(1, 1, builder_player, 100, Coordinates{5, 5});
  ctx.em.mutate_race(builder_player, [](Race& r) { r.tech = 500.0; });
  ctx.em.mutate_universe([](Universe& u) { u.set_AP(builder_player, 50); });
  ctx.em.mutate_star(1, [](Star& s) {
    s.mark_explored_by(builder_player);
    s.mark_inhabited_by(builder_player);
    s.AP(builder_player) = 50;
  });
  ctx.em.mutate_planet(1, 1, [](Planet& p) {
    p.mark_explored_by(builder_player);
    p.info(builder_player).resource = 10000;
    p.info(builder_player).fuel = 1000;
  });
  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    smap.get(Coordinates{5, 5}).set_condition(SectorType::SEC_LAND);
  });
}

void test_build_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  // Create GameObj for testing
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, builder_player, 1);
  g.set_scope_planet(1, 1);

  // 1. Build info queries (0 AP, zero DB writes including peek_ship_exam)
  const auto before_info = ctx.db.total_changes();
  ctx.assert_dispatch_success(g, {"build", "?"}, 0);
  test::expect_contains(g.out.str(), "Default ship parameters");
  g.out.str("");
  ctx.assert_dispatch_success(g, {"build", "?", ":"}, 0);
  test::expect_contains(g.out.str(), "Space Probe");
  test::expect_eq(ctx.db.total_changes(), before_info);

  // 2. Test: Build a probe on planet (1 AP deducted dynamically)
  g.out.str("");
  // ":" = Probe
  ctx.assert_dispatch_success(g, {"build", ":", "5,5", "1"}, 1);
  test::expect_contains(g.out.str(), "built at a cost of");
  test::expect_contains(g.out.str(), "is on sector 5,5.");
  test::expect_true(ctx.em.get_telegrams(builder_player, 1).empty());

  // Verify planet resources were deducted
  ctx.em.clear_cache();
  const auto* planet_verify = ctx.em.peek_planet(1, 1);
  test::expect_ne(planet_verify, nullptr);
  test::expect_lt(planet_verify->info(builder_player).resource,
                  10000);  // Resources should be deducted

  // Verify ship was created (assigned lowest available ship ID #1)
  const auto* ship = ctx.em.peek_ship(1);
  test::expect_ne(ship, nullptr);
  test::expect_eq(ship->type(), ShipType::OTYPE_PROBE);
  test::expect_eq(ship->owner(), builder_player);
  test::expect_eq(ship->whatorbits(), ScopeLevel::LEVEL_PLAN);
  test::expect_eq(ship->storbits(), 1);
  test::expect_eq(ship->pnumorbits(), 1);
  test::expect_eq(ship->land_coords(), Coordinates(5, 5));
}

void test_build_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set Star AP to 0
  ctx.em.mutate_star(1, [](Star& s) { s.AP(builder_player) = 0; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, builder_player, 1);
  g.set_scope_planet(1, 1);

  ctx.assert_dispatch_rejected(g, {"build", ":", "5,5", "1"});
  test::expect_contains(g.out.str(), "action points");
}

void test_build_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, builder_player, 1);
  g.set_scope_planet(1, 1);

  // 1. Missing type argument at planet scope
  ctx.assert_dispatch_rejected(g, {"build"});
  test::expect_contains(g.out.str(), "Build what?");

  // 2. Missing coordinates argument at planet scope
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"build", ":"});
  test::expect_contains(g.out.str(), "Build where?");

  // 3. Test: Build on enslaved planet rejected with diagnostic in g.out
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.enslave_to(rival_player); });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"build", ":", "5,5", "1"});
  test::expect_contains(g.out.str(), "This planet is enslaved by player 3.");
  test::expect_true(ctx.em.get_telegrams(builder_player, 1).empty());
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.free_slaves(); });

  // 4. Test: Build with insufficient resources
  // Drain resources completely
  ctx.em.mutate_planet(1, 1,
                       [](Planet& p) { p.info(builder_player).resource = 0; });
  g.out.str("");
  // Try to build probe with no resources
  ctx.assert_dispatch_rejected(g, {"build", ":", "5,5", "1"});
  test::expect_contains(g.out.str(), "You need");
}

void test_build_info_queries() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, builder_player, 1);

  // Specific ship type query (":" = Space Probe)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"build", "?", ":"}, 0);
  test::expect_contains(g.out.str(), "Space Probe");
  test::expect_contains(g.out.str(), "Can be constructed on planet.");

  // Invalid ship type query
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"build", "?", "9"});
  test::expect_contains(g.out.str(), "No such ship type.");
}

void test_build_from_ships() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, builder_player, 1);

  shipnum_t initial_ships = ctx.em.num_ships();

  // 1. Factory building with 0 args ("build") and 2 args ("build 2")
  shipnum_t factory_id = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY)
                             .owned_by(builder_player, 1)
                             .landed_on(1, 1, {5, 5})
                             .with_resource(5000)
                             .with_crew(100, 0)
                             .with_on(true)
                             .with_build_type(ShipType::STYPE_SHUTTLE)
                             .build();

  g.set_scope_ship(factory_id);

  // "build" with no arguments in factory scope builds 1 ship
  g.out.str("");
  ctx.assert_dispatch_success(g, {"build"}, 1);
  test::expect_eq(ctx.em.num_ships(), shipnum_t{initial_ships.value + 2});

  // "build 2" in factory scope builds 2 ships
  g.out.str("");
  ctx.assert_dispatch_success(g, {"build", "2"}, 2);
  test::expect_eq(ctx.em.num_ships(), shipnum_t{initial_ships.value + 4});

  // 2. Shuttle building outside in star orbit ("build H 1")
  shipnum_t shuttle_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                             .owned_by(builder_player, 1)
                             .in_star_orbit(1)
                             .with_resource(50000)
                             .with_crew(50, 0)
                             .build();

  g.set_scope_ship(shuttle_id);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"build", "H", "1"}, 1);
  test::expect_eq(ctx.em.num_ships(), shipnum_t{initial_ships.value + 6});

  // 3. Habitat building inside hangar in universe orbit
  shipnum_t hab_id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                         .owned_by(builder_player, 1)
                         .in_deep_space()
                         .with_resource(5000)
                         .with_max_hanger(500)
                         .with_hanger(0)
                         .with_crew(100, 0)
                         .build();

  g.set_scope_ship(hab_id);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"build", ":", "1"}, 0, 1);
  test::expect_eq(ctx.em.num_ships(), shipnum_t{initial_ships.value + 8});
}

void test_factory_all_classes_and_toxic_waste() {
  TestContext ctx;
  setup_test_world(ctx);
  ctx.em.mutate_star(1, [](Star& s) { s.AP(builder_player) = 200; });
  ctx.em.mutate_planet(1, 1, [](Planet& p) {
    p.info(builder_player).resource = 500000;
    p.toxic() = 50;
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, builder_player, 1);

  // 1. Planetary `build w` preserves extracted 20% toxic payload on new ship
  g.set_scope_planet(1, 1);
  ctx.assert_dispatch_success(g, {"build", "w", "5,5", "1"}, 1);
  test::expect_eq(static_cast<int>(ctx.em.peek_planet(1, 1)->toxic()), 30);
  const auto* waste_ship = ctx.em.peek_ship(1)->as<ToxicWasteShip>();
  test::expect_ne(waste_ship, nullptr);
  test::expect_eq(waste_ship->toxic_level(), 20);

  // 2. Mortal Factory `make` -> `order on` -> `build` succeeds for all
  //    factory-buildable ship classes (buildable_at.factory), preserving
  //    shipclass, cloak, secondary batteries, and secondary-only
  //    active_battery/retaliate.
  ctx.em.mutate_race(builder_player,
                     [](Race& r) { r.discoveries.cloak = true; });
  g.race = ctx.em.peek_race(builder_player);

  const auto& fac_tmpl = ship_template(ShipType::OTYPE_FACTORY);
  for (const auto& tmpl : ship_templates) {
    if (!tmpl.can_be_built_by(fac_tmpl) || tmpl.is_god_only ||
        tmpl.type == ShipType::STYPE_POD) {
      continue;
    }
    const shipnum_t fac_id = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY)
                                 .owned_by(builder_player, 1)
                                 .landed_on(1, 1, {5, 5})
                                 .with_damage(0)
                                 .with_on(false)
                                 .with_crew(20, 0)
                                 .build();
    g.set_scope_ship(fac_id);
    ctx.assert_dispatch_success(g, {"make", std::string(1, tmpl.letter)});
    if (tmpl.type == ShipType::STYPE_CRUISER) {
      ctx.em.mutate_ship(fac_id, [](Ship& f) {
        auto& bp = f.as<FactoryShip>()->blueprint_mut();
        bp.cloak = true;
        bp.primary_battery = GunBattery{};
      });
    }
    ctx.assert_dispatch_success(
        g, {"order", std::format("#{}", fac_id.value), "on"});
    const shipnum_t next_id = ctx.em.next_available_ship_number();
    ctx.assert_dispatch_success(g, {"build"}, 1);

    const auto* built = ctx.em.peek_ship(next_id);
    test::expect_ne(built, nullptr);
    test::expect_eq(built->type(), tmpl.type);
    test::expect_eq(built->shipclass(), std::format("mod {}", fac_id.value));
    test::expect_true(built->base_mass() > 0.0);
    if (tmpl.type == ShipType::STYPE_CRUISER) {
      test::expect_true(built->cloak());
      test::expect_eq(built->guns(), ActiveBattery::SECONDARY);
      test::expect_eq(built->retaliate(), tmpl.max_guns);
    } else if (tmpl.has_secondary()) {
      test::expect_eq(built->secondary_battery().count, tmpl.max_guns);
    }
  }
}

}  // namespace

int main() {
  test_build_happy_paths();
  test_build_insufficient_ap();
  test_build_domain_errors();
  test_build_info_queries();
  test_build_from_ships();
  test_factory_all_classes_and_toxic_waste();

  std::println(std::cout, "✓ build_test passed!");
  return 0;
}
