// SPDX-License-Identifier: Apache-2.0

/// \file scrap_test.cc
/// \brief Unit tests for scrap command

import commands;
import gb.entities;
import gb.presentation;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe().with_populated_planet(1, 1, 1, 1000,
                                                     Coordinates{5, 5});

  auto carrier_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
                        .owned_by(player_t{1}, governor_t{1})
                        .named("Carrier")
                        .in_star_orbit(1)
                        .with_crew(10, 0)
                        .with_fuel(100.0)
                        .with_resource(100)
                        .build();

  auto fighter_id = TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
                        .owned_by(player_t{1}, governor_t{1})
                        .named("ToScrap")
                        .in_star_orbit(1)
                        .with_crew(5, 0)
                        .with_fuel(50.0)
                        .with_resource(20)
                        .with_destruct(10)
                        .build();

  ctx.em.mutate_ship(carrier_id,
                     [&](Ship& s) { s.dock_with_ship(fighter_id); });
  ctx.em.mutate_ship(fighter_id, [&](Ship& s) {
    s.dock_with_ship(carrier_id);
    s.build_cost() = 100;
  });
}

void test_scrap_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1. Scrap docked fighter (1 AP deducted via dynamic AP)
  ctx.assert_dispatch_success(g, {"scrap", "#2"}, 1);

  ctx.em.clear_cache();
  test::expect_throws<EntityNotFoundError>([&]() { ctx.em.peek_ship(2); });

  const auto* carrier_after = ctx.em.peek_ship(1);
  test::expect_ne(carrier_after, nullptr);
  test::expect_gt(carrier_after->resource(), 100);
  test::expect_eq(carrier_after->docked(), 0);

  ctx.verify_universe_invariants();
}

void test_scrap_docked_capacity_clamping() {
  TestContext ctx;
  ctx.with_standard_universe();

  // Host cruiser with tight remaining capacities:
  // max_resource=140 (has 100 -> room for 40)
  // max_fuel=120 (has 100 -> room for 20)
  // max_destruct=25 (has 20 -> room for 5)
  // max_crew=20 (has 5 popn, 5 troops -> 10 berths free total)
  // max_crystals_capacity=127 (has 126 -> room for 1)
  auto host_id = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                     .owned_by(player_t{1}, governor_t{1})
                     .named("Host")
                     .in_star_orbit(1)
                     .with_crew(5, 5)
                     .with_fuel(100.0)
                     .with_resource(100)
                     .with_destruct(20)
                     .with_crystals(126)
                     .build();
  ctx.em.mutate_ship(host_id, [](Ship& s) {
    s.max_resource() = 140;
    s.max_fuel() = 120.0;
    s.max_destruct() = 25;
    s.max_crew() = 20;
  });

  // Ship to scrap has:
  // build_cost=100, resource=30 -> scrapval=80 (clamped to 40)
  // fuel=50 -> clamped to 20
  // destruct=15 -> clamped to 5
  // troops=12, popn=8 -> troops clamped to 10, leaving 0 berths for crew!
  // crystals=2, mounted=1 -> 3 total (clamped to 1)
  auto scrap_id = TestShipBuilder(ctx.em, ShipType::STYPE_DESTROYER)
                      .owned_by(player_t{1}, governor_t{1})
                      .named("LoadedScrap")
                      .in_star_orbit(1)
                      .with_crew(8, 12)
                      .with_fuel(50.0)
                      .with_resource(30)
                      .with_destruct(15)
                      .with_crystals(2)
                      .build();
  ctx.em.mutate_ship(host_id, [&](Ship& s) { s.dock_with_ship(scrap_id); });
  ctx.em.mutate_ship(scrap_id, [&](Ship& s) {
    s.dock_with_ship(host_id);
    s.build_cost() = 100;
    s.mounted() = 1;
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  ctx.assert_dispatch_success(g, {"scrap", std::format("#{}", scrap_id.value)},
                              1);

  test::expect_contains(g.out.str(), "scrap value(with stockpile) : 80 rp's.");
  test::expect_contains(g.out.str(), "(There is only room for 40 resources.)");
  test::expect_contains(g.out.str(), "Fuel recovery: 50.");
  test::expect_contains(g.out.str(), "(There is only room for 20.00 fuel.)");
  test::expect_contains(g.out.str(), "Weapons recovery: 15.");
  test::expect_contains(g.out.str(), "(There is only room for 5 destruct.)");
  test::expect_contains(g.out.str(), "Population/Troops recovery: 8/12.");
  test::expect_contains(g.out.str(), "(There is only room for 10 troops.)");
  test::expect_contains(g.out.str(), "(There is only room for 0 crew.)");
  test::expect_contains(g.out.str(), "(There is only room for 1 crystals.)");
  test::expect_contains(g.out.str(), "Crystal recovery: 1.");
  test::expect_contains(g.out.str(), "Destroyed.");

  const auto* host_after = ctx.em.peek_ship(host_id);
  test::expect_ne(host_after, nullptr);
  test::expect_eq(host_after->resource(), 140);
  test::expect_eq(host_after->fuel(), 120.0);
  test::expect_eq(host_after->destruct(), 25);
  test::expect_eq(host_after->troops(), 15);
  test::expect_eq(host_after->popn(), 5);
  test::expect_eq(host_after->crystals(), 127);

  // Also test Shuttle host (can_strap_cargo_to_hull() == true, so resources are
  // NOT clamped) and carrier hangar docking (LEVEL_SHIP)
  auto shuttle_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                        .owned_by(player_t{1}, governor_t{1})
                        .in_star_orbit(1)
                        .with_crew(2, 0)
                        .with_resource(50)
                        .build();
  auto pod_id = TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
                    .owned_by(player_t{1}, governor_t{1})
                    .in_star_orbit(1)
                    .with_crew(2, 0)
                    .with_resource(100)
                    .build();
  ctx.em.mutate_ship(shuttle_id, [&](Ship& s) { s.dock_with_ship(pod_id); });
  ctx.em.mutate_ship(pod_id, [&](Ship& s) {
    s.dock_with_ship(shuttle_id);
    s.build_cost() = 50;
  });

  g.out.str("");
  ctx.assert_dispatch_success(g, {"scrap", std::format("#{}", pod_id.value)},
                              1);
  test::expect_eq(ctx.em.peek_ship(shuttle_id)->resource(), 175);

  ctx.verify_universe_invariants();
}

void test_scrap_landed_sectors_and_spaceborne() {
  TestContext ctx;
  ctx.with_standard_universe().with_populated_planet(1, 1, 1, 1000,
                                                     Coordinates{5, 5});

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Landed on owned sector (5, 5) with crew, troops, fuel, destruct,
  // crystals
  auto owned_ship = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                        .owned_by(player_t{1}, governor_t{1})
                        .landed_on(1, 1, Coordinates{5, 5})
                        .with_crew(6, 4)
                        .with_fuel(20.0)
                        .with_resource(10)
                        .with_destruct(5)
                        .with_crystals(2)
                        .build();
  ctx.em.mutate_ship(owned_ship, [](Ship& s) { s.build_cost() = 40; });

  ctx.assert_dispatch_success(
      g, {"scrap", std::format("#{}", owned_ship.value)}, 1);
  test::expect_contains(g.out.str(), "Population/Troops recovery: 6/4.");
  test::expect_contains(g.out.str(), "Crystal recovery: 2.");
  test::expect_contains(g.out.str(), "Scrapped.");
  ctx.verify_universe_invariants();

  // 2. Landed on unowned sector (2, 2) with crew + troops -> colonizes sector
  auto col_ship = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                      .owned_by(player_t{1}, governor_t{1})
                      .landed_on(1, 1, Coordinates{2, 2})
                      .with_crew(5, 3)
                      .with_resource(10)
                      .build();
  g.out.str("");
  ctx.assert_dispatch_success(g, {"scrap", std::format("#{}", col_ship.value)},
                              1);
  test::expect_contains(g.out.str(), "Sector 2,2 Colonized.");
  ctx.verify_universe_invariants();

  // 3. Crewless ship landed on unowned sector (3, 3) -> reclaims resources
  // without claiming empty 0-pop sector
  auto canister_id = TestShipBuilder(ctx.em, ShipType::OTYPE_CANIST)
                         .owned_by(player_t{1}, governor_t{1})
                         .landed_on(1, 1, Coordinates{3, 3})
                         .with_crew(0, 0)
                         .with_resource(15)
                         .build();
  ctx.em.mutate_ship(canister_id, [](Ship& s) { s.max_crew() = 0; });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"scrap", std::format("#{}", canister_id.value)}, 1);
  test::expect_eq(
      ctx.em.peek_sectormap(1, 1)->get(Coordinates{3, 3}).get_owner(),
      player_t{0});
  ctx.verify_universe_invariants();

  // 4. Landed on foreign-owned sector (4, 4) -> crew and crystals blocked
  ctx.em.mutate_sectormap(1, 1, [&](SectorMap& smap) {
    ctx.em.mutate_planet(1, 1, [&](Planet& p) {
      p.adjust_sector_population(smap.get(Coordinates{4, 4}), player_t{2}, 50,
                                 10);
    });
  });
  auto foreign_sect_ship = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                               .owned_by(player_t{1}, governor_t{1})
                               .landed_on(1, 1, Coordinates{4, 4})
                               .with_crew(5, 2)
                               .with_crystals(1)
                               .with_resource(10)
                               .build();
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"scrap", std::format("#{}", foreign_sect_ship.value)}, 1);
  test::expect_contains(g.out.str(),
                        "You don't own this sector; no crew can be recovered.");
  test::expect_contains(
      g.out.str(), "You don't own this sector; no crystals can be recovered.");
  ctx.verify_universe_invariants();

  // 5. Spaceborne ship in star orbit (neither landed nor docked) -> Destroyed,
  // no resources reclaimed
  g.set_level(ScopeLevel::LEVEL_STAR);
  auto space_ship = TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
                        .owned_by(player_t{1}, governor_t{1})
                        .in_star_orbit(1)
                        .with_crew(2, 0)
                        .build();
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"scrap", std::format("#{}", space_ship.value)}, 1);
  test::expect_contains(g.out.str(), "is not landed or docked.");
  test::expect_contains(g.out.str(), "No resources can be reclaimed.");
  test::expect_contains(g.out.str(), "Destroyed.");
  ctx.verify_universe_invariants();
}

void test_scrap_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set Star AP to 0
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 0; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
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
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"scrap"});
  test::expect_contains(g.out.str(), "Syntax: scrap <ship>");

  // 2. Uncrewed ship rejection
  ctx.em.mutate_ship(2, [](Ship& s) { s.popn() = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"scrap", "#2"});
  test::expect_contains(g.out.str(), "no crew");

  // Restore crew, break reciprocal docking on carrier #1 -> OtherShipNotDocked
  // (and verify AP is NOT deducted!)
  const ap_t ap_before = ctx.em.peek_star(1)->AP(1);
  ctx.em.mutate_ship(2, [](Ship& s) { s.popn() = 5; });
  ctx.em.mutate_ship(1, [](Ship& s) { s.undock_from_ship(); });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"scrap", "#2"});
  test::expect_contains(g.out.str(), "Warning, other ship not docked..");
  test::expect_eq(ctx.em.peek_star(1)->AP(1), ap_before);

  // 3. Deep space (LEVEL_UNIV) ship with 0 universe AP vs 1 universe AP
  auto deep_ship = TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
                       .owned_by(player_t{1}, governor_t{1})
                       .in_deep_space()
                       .with_crew(2, 0)
                       .build();
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.em.mutate_universe([](universe_struct& u) { u.set_AP(player_t{1}, 0); });
  g.out.str("");
  ctx.assert_dispatch_rejected(g,
                               {"scrap", std::format("#{}", deep_ship.value)});
  test::expect_contains(g.out.str(), "You need 1 universe action point.");

  ctx.em.mutate_universe([](universe_struct& u) { u.set_AP(player_t{1}, 2); });
  g.out.str("");
  g.set_ui_mode(GB::presentation::UiMode::JSON);
  ctx.assert_dispatch_success(g,
                              {"scrap", std::format("#{}", deep_ship.value)});
  test::expect_contains(g.out.str(), "\"type\":\"scrap_ship\"");
  test::expect_eq(ctx.em.peek_universe()->get_AP(player_t{1}), ap_t{1});
}

void test_scrap_toxic_waste_warning() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // Create Toxic Waste Canister landed on planet
  auto tox_id = TestShipBuilder(ctx.em, ShipType::OTYPE_TOXWC)
                    .owned_by(player_t{1}, governor_t{1})
                    .named("HazMat")
                    .landed_on(1, 1, Coordinates{5, 5})
                    .with_crew(1, 0)
                    .with_special(WasteData{.toxic = 25})
                    .build();

  ctx.assert_dispatch_success(g, {"scrap", std::format("#{}", tox_id.value)},
                              1);
  test::expect_contains(g.out.str(),
                        "WARNING: This will release 25 toxin points");

  ctx.em.clear_cache();
  test::expect_throws<EntityNotFoundError>([&]() { ctx.em.peek_ship(tox_id); });

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_scrap_happy_paths();
  test_scrap_docked_capacity_clamping();
  test_scrap_landed_sectors_and_spaceborne();
  test_scrap_toxic_waste_warning();
  test_scrap_insufficient_ap();
  test_scrap_domain_errors();

  std::println(std::cout, "✓ scrap_test passed!");
  return 0;
}
