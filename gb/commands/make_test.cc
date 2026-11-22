// SPDX-License-Identifier: Apache-2.0

/// \file make_test.cc
/// \brief Unit tests for the make command (factory blueprint configuration and
/// specification inspection).

import dallib;
import gb.entities;
import gb.presentation;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_make_command_matrix() {
  TestContext ctx;
  const auto races = ctx.with_races();

  shipnum_t factory_id = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY)
                             .owned_by(races.empire, 1)
                             .named("Factory")
                             .in_star_orbit(1)
                             .with_fuel(100.0)
                             .with_resource(1000)
                             .with_crew(50, 0)
                             .with_on(false)
                             .with_size(100)
                             .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, races.empire, 1);
  g.set_scope_ship(factory_id);

  TestCommandMatrix(ctx, GB::commands::make_cmd)
      .with_invalid_scopes(
          {Place::universe(), Place::star(1), Place::planet(1, 1)})
      .with_valid_argv({"make", "f"})
      .with_invalid_argv({"make", "z"})
      .run_matrix(g);

  std::println(std::cout, "✓ make command matrix tests passed");
}

void test_make_designations_and_errors() {
  TestContext ctx;
  const auto races = ctx.with_races();
  ctx.em.mutate_race(races.empire, [](Race& r) {
    r.tech = 100.0;
    r.pods = false;
    r.discoveries.crystal = true;
    r.discoveries.hyperdrive = true;
    r.discoveries.laser = true;
    r.discoveries.cew = true;
    r.discoveries.cloak = true;
  });

  shipnum_t factory_id = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY)
                             .owned_by(races.empire, 1)
                             .named("Factory")
                             .in_star_orbit(1)
                             .with_fuel(100.0)
                             .with_resource(1000)
                             .with_crew(50, 0)
                             .with_on(false)
                             .with_size(100)
                             .build();

  shipnum_t non_factory_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                                 .owned_by(races.empire, 1)
                                 .in_star_orbit(1)
                                 .with_crew(10, 0)
                                 .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, races.empire, 1);

  // 1. Non-factory ship rejected
  g.set_scope_ship(non_factory_id);
  ctx.assert_dispatch_rejected(g, {"make", "f"});
  test::expect_contains(g.out.str(), "That is not a factory.");

  // 2. Make with 0 args before build_type is set
  g.set_scope_ship(factory_id);
  ctx.assert_dispatch_rejected(g, {"make"});
  test::expect_contains(g.out.str(), "No ship type specified.");

  // 3. Illegal ship letter
  ctx.assert_dispatch_rejected(g, {"make", "z"});
  test::expect_contains(g.out.str(), "Illegal ship letter.");

  // 4. Pod without pod technology vs with pod technology (MC/DC)
  ctx.assert_dispatch_rejected(g, {"make", "p"});
  test::expect_contains(g.out.str(), "Illegal ship letter.");

  ctx.em.mutate_race(races.empire, [](Race& r) { r.pods = true; });
  ctx.setup_game_obj(g, races.empire, 1);
  g.set_scope_ship(factory_id);
  ctx.assert_dispatch_rejected(g, {"make", "p"});
  test::expect_contains(g.out.str(),
                        "This kind of ship does not require a factory");
  ctx.em.mutate_race(races.empire, [](Race& r) { r.pods = false; });
  ctx.setup_game_obj(g, races.empire, 1);
  g.set_scope_ship(factory_id);

  // 5. Non-factory-built ship (probe ':')
  ctx.assert_dispatch_rejected(g, {"make", ":"});
  test::expect_contains(g.out.str(),
                        "This kind of ship does not require a factory");

  // 6. God-only ship rejection for mortal race ('!') and check for Deity race
  ctx.assert_dispatch_rejected(g, {"make", "!"});
  test::expect_contains(g.out.str(), "Nice try!");

  shipnum_t deity_factory_id = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY)
                                   .owned_by(races.deity, 1)
                                   .named("DeityFactory")
                                   .in_star_orbit(1)
                                   .with_crew(50, 0)
                                   .with_on(false)
                                   .build();
  GameObj god_g(ctx.em, registry);
  ctx.setup_game_obj(god_g, races.deity, 1);
  god_g.set_scope_ship(deity_factory_id);
  ctx.assert_dispatch_rejected(god_g, {"make", "!"});
  test::expect_contains(god_g.out.str(),
                        "This kind of ship does not require a factory");
  ctx.assert_dispatch_success(god_g, {"make", "C"});
  test::expect_eq(ctx.em.peek_ship(deity_factory_id)
                      ->as<FactoryShip>()
                      ->blueprint()
                      .build_cost,
                  0);

  // 7. Designate cruiser ('C') and inspect with 0 args
  ctx.assert_dispatch_success(g, {"make", "C"});
  test::expect_contains(g.out.str(), "Factory designated to produce Cruisers.");

  ctx.assert_dispatch_success(g, {"make"});
  test::expect_contains(g.out.str(),
                        "--- Current Production Specifications ---");
  test::expect_contains(g.out.str(), "Cruiser");

  // 8. Low tech warning on display and designate
  ctx.em.mutate_race(races.empire, [](Race& r) { r.tech = 1.0; });
  ctx.setup_game_obj(g, races.empire, 1);
  g.set_scope_ship(factory_id);
  ctx.assert_dispatch_success(g, {"make", "C"});
  test::expect_contains(g.out.str(), "You can't produce this design yet!");
  ctx.assert_dispatch_success(g, {"make"});
  test::expect_contains(g.out.str(),
                        "Your engineering capability is not advanced enough");

  // Restore tech
  ctx.em.mutate_race(races.empire, [](Race& r) { r.tech = 100.0; });
  ctx.setup_game_obj(g, races.empire, 1);
  g.set_scope_ship(factory_id);

  // 9. Print specs for simple ship (Space Mine '+') without guns/mount/cew
  ctx.assert_dispatch_success(g, {"make", "+"});
  ctx.assert_dispatch_success(g, {"make"});
  test::expect_contains(g.out.str(), "Space Mine");

  // 10. UiMode::JSON verification for make and 0-arg make
  g.set_ui_mode(GB::presentation::UiMode::JSON);
  ctx.assert_dispatch_success(g, {"make", "f"});
  test::expect_contains(g.out.str(), R"("type":"message")");
  ctx.assert_dispatch_success(g, {"make"});
  test::expect_contains(g.out.str(), R"("type":"message")");
  g.set_ui_mode(GB::presentation::UiMode::ASCII);

  // 11. Factory already online allows 0-arg inspection but rejects make <type>
  ctx.em.mutate_ship(factory_id, [](Ship& s) { s.on() = true; });
  ctx.assert_dispatch_success(g, {"make"});
  test::expect_contains(g.out.str(), "Online");
  ctx.assert_dispatch_rejected(g, {"make", "f"});
  test::expect_contains(g.out.str(), "This factory is already online.");

  ctx.verify_universe_invariants();
  std::println(std::cout, "✓ make designations and error paths passed");
}

}  // namespace

int main() {
  test_make_command_matrix();
  test_make_designations_and_errors();

  std::println(std::cout, "\n✅ All make tests passed!");
  return 0;
}
