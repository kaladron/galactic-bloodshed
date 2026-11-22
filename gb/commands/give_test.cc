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

constexpr player_t donor = 2;
constexpr player_t recipient = 3;

void test_give_dispatch() {
  std::println(std::cout, "Test: give command dispatch and ship transfer");
  TestContext ctx;
  ctx.with_galaxy();
  TestWorldBuilder world(ctx);
  world.add_race("Federation", 100.0, false, donor);
  world.add_race("Klingons", 100.0, false, recipient);

  ctx.em.mutate_star(1, [](Star& s) {
    s.mark_explored_by(donor);
    s.mark_inhabited_by(donor);
    s.AP(donor) = 100;
  });
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.mark_explored_by(donor); });
  ctx.em.mutate_star(2, [](Star& s) {
    s.mark_explored_by(donor);
    s.AP(donor) = 100;
  });
  ctx.em.mutate_universe([](Universe& u) { u.set_AP(donor, 100); });

  // Establish mutual alliance between Federation and Klingons
  ctx.em.mutate_race(donor,
                     [](Race& r) { r.declare_alliance_with(recipient); });
  ctx.em.mutate_race(recipient,
                     [](Race& r) { r.declare_alliance_with(donor); });

  const shipnum_t ship_plan = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                                  .owned_by(donor, 1)
                                  .in_planet_orbit(1, 1)
                                  .build();

  const shipnum_t ship_star = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                                  .owned_by(donor, 1)
                                  .in_star_orbit(1)
                                  .build();

  const shipnum_t ship_univ =
      TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE).owned_by(donor, 1).build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, donor, 1);
  g.set_scope_planet(1, 1);

  // 1. Happy path: give ship in planet orbit to mutual ally
  ctx.assert_dispatch_success(
      g, {"give", "Klingons", std::format("#{}", ship_plan.value)});
  test::expect_contains(g.out.str(), "Owner changed.");

  // Verify ownership changed
  ctx.em.clear_cache();
  const auto* transferred = ctx.em.peek_ship(ship_plan);
  test::expect_ne(transferred, nullptr);
  test::expect_eq(transferred->owner(), recipient);
  test::expect_eq(transferred->governor(), 1);

  const auto* planet_verify = ctx.em.peek_planet(1, 1);
  test::expect_ne(planet_verify, nullptr);
  test::expect_eq(planet_verify->info(recipient).explored, 1);

  const auto* star_verify = ctx.em.peek_star(1);
  test::expect_ne(star_verify, nullptr);
  test::expect_true(star_verify->is_explored_by(recipient));
  std::println(std::cout, "    ✓ Ship in planet orbit given to ally");

  // 2. Happy path: give ship in star orbit
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"give", "Klingons", std::format("#{}", ship_star.value)});
  test::expect_contains(g.out.str(), "Owner changed.");
  std::println(std::cout, "    ✓ Ship in star orbit given to ally");

  // 3. Happy path: give ship in universe space
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"give", "Klingons", std::format("#{}", ship_univ.value)});
  test::expect_contains(g.out.str(), "Owner changed.");
  std::println(std::cout, "    ✓ Ship in universe space given to ally");

  // 4. Insufficient AP in star system
  const shipnum_t ship_no_ap = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                                   .owned_by(donor, 1)
                                   .in_star_orbit(1)
                                   .build();
  ctx.em.mutate_star(1, [](Star& s) { s.AP(donor) = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"give", "Klingons", std::format("#{}", ship_no_ap.value)});
  test::expect_contains(g.out.str(),
                        "You don't have enough action points in that system.");

  // 5. Insufficient AP in universe
  const shipnum_t ship_no_uap =
      TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE).owned_by(donor, 1).build();
  ctx.em.mutate_universe([](Universe& u) { u.set_AP(donor, 0); });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"give", "Klingons", std::format("#{}", ship_no_uap.value)});
  test::expect_contains(g.out.str(),
                        "You don't have enough universe action points.");

  // Restore AP for subsequent tests
  ctx.em.mutate_star(1, [](Star& s) { s.AP(donor) = 100; });
  ctx.em.mutate_universe([](Universe& u) { u.set_AP(donor, 100); });

  // 6. Non-existent recipient
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"give", "NonExistent", "#1"});
  test::expect_contains(g.out.str(), "No such player.");

  // 7. Recipient is a guest
  ctx.em.mutate_race(recipient, [](Race& r) { r.Guest = true; });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"give", "Klingons", std::format("#{}", ship_no_ap.value)});
  test::expect_contains(g.out.str(), "You can't give this player anything.");
  ctx.em.mutate_race(recipient, [](Race& r) { r.Guest = false; });

  // 8. Not mutually allied (both directions)
  ctx.em.mutate_race(recipient,
                     [](Race& r) { r.rescind_alliance_with(donor); });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"give", "Klingons", std::format("#{}", ship_no_ap.value)});
  test::expect_contains(g.out.str(), "You two are not mutually allied.");
  ctx.em.mutate_race(recipient,
                     [](Race& r) { r.declare_alliance_with(donor); });

  ctx.em.mutate_race(donor,
                     [](Race& r) { r.rescind_alliance_with(recipient); });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"give", "Klingons", std::format("#{}", ship_no_ap.value)});
  test::expect_contains(g.out.str(), "You two are not mutually allied.");
  ctx.em.mutate_race(donor,
                     [](Race& r) { r.declare_alliance_with(recipient); });

  // 9. Illegal ship number format
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"give", "Klingons", "bad_ship"});
  test::expect_contains(g.out.str(), "Illegal ship number.");

  // 10. Missing ship in database
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"give", "Klingons", "#9999"});
  test::expect_contains(g.out.str(), "No such ship.");

  // 11. Ship not owned by donor
  const shipnum_t ship_p2 = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                                .owned_by(recipient, 1)
                                .in_star_orbit(1)
                                .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"give", "Klingons", std::format("#{}", ship_p2.value)});
  test::expect_contains(g.out.str(),
                        std::format("You don't own ship #{}.", ship_p2));
  test::expect_true(ctx.em.get_telegrams(donor, 1).empty());

  // 12. Spore pod rejection
  const shipnum_t pod_id = TestShipBuilder(ctx.em, ShipType::STYPE_POD)
                               .owned_by(donor, 1)
                               .in_star_orbit(1)
                               .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"give", "Klingons", std::format("#{}", pod_id.value)});
  test::expect_contains(g.out.str(),
                        "You cannot change the ownership of spore pods.");

  // 13. Crewed ship cannot be given away
  const shipnum_t crewed_id = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                                  .owned_by(donor, 1)
                                  .in_star_orbit(1)
                                  .with_crew(10, 0)
                                  .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"give", "Klingons", std::format("#{}", crewed_id.value)});
  test::expect_contains(g.out.str(), "crew/mil on board");

  // 14. Carrier with loaded ships & docked ship scope rejection
  const shipnum_t carrier_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
                                   .owned_by(donor, 1)
                                   .in_star_orbit(1)
                                   .build();
  const shipnum_t fighter_id = TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
                                   .owned_by(donor, 1)
                                   .docked_to(carrier_id, 1)
                                   .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"give", "Klingons", std::format("#{}", carrier_id.value)});
  test::expect_contains(g.out.str(), "loaded on it");

  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"give", "Klingons", std::format("#{}", fighter_id.value)});
  test::expect_contains(g.out.str(), "Something wrong with this ship's scope.");

  // 15. Deity bypasses guest, alliance, crew, and loaded carrier restrictions
  // (and emits UiMode::JSON output)
  ctx.em.mutate_race(donor, [](Race& r) {
    r.God = true;
    r.rescind_alliance_with(recipient);
  });
  ctx.em.mutate_race(recipient, [](Race& r) { r.Guest = true; });
  g.set_god(true);
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"give", "Klingons", std::format("#{}", crewed_id.value)});
  test::expect_contains(g.out.str(), R"("type":"message")");
  test::expect_contains(g.out.str(), R"("type":"ship")");
  test::expect_contains(g.out.str(), "Owner changed.");

  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"give", "Klingons", std::format("#{}", carrier_id.value)});
  test::expect_contains(g.out.str(), R"("type":"message")");
  test::expect_contains(g.out.str(), R"("type":"ship")");
  test::expect_contains(g.out.str(), "Owner changed.");

  ctx.em.mutate_race(donor, [](Race& r) {
    r.God = false;
    r.declare_alliance_with(recipient);
  });
  ctx.em.mutate_race(recipient, [](Race& r) { r.Guest = false; });
  g.set_god(false);
  g.set_ui_mode(UiMode::ASCII);

  // 16. Giving away a race's Gov_ship scrubs donor Gov_ship so destroying it
  // later never trips tbl_race(gov_ship) FK, and gifting an uncrewed canister
  // at an unexplored star does not reveal that star to the recipient
  const shipnum_t can_id = TestShipBuilder(ctx.em, ShipType::OTYPE_CANIST)
                               .owned_by(donor, 1)
                               .in_star_orbit(2)
                               .build();
  ctx.em.mutate_star(2, [](Star& s) {
    s.AP(donor) = 10;
    s.clear_explored_by(recipient);
    s.clear_inhabited_by(recipient);
  });
  ctx.assert_dispatch_success(
      g, {"give", "Klingons", std::format("#{}", can_id.value)});
  test::expect_false(ctx.em.peek_star(2)->is_explored_by(recipient));
  test::expect_false(ctx.em.peek_star(2)->is_inhabited_by(recipient));

  const shipnum_t gov_id = TestShipBuilder(ctx.em, ShipType::OTYPE_GOV)
                               .owned_by(donor, 1)
                               .in_star_orbit(1)
                               .build();
  ctx.em.mutate_race(donor, [&](Race& r) { r.Gov_ship = gov_id; });
  ctx.em.mutate_star(1, [](Star& s) { s.AP(donor) = 10; });
  ctx.assert_dispatch_success(
      g, {"give", "Klingons", std::format("#{}", gov_id.value)});
  test::expect_eq(ctx.em.peek_race(donor)->Gov_ship, std::nullopt);
  ctx.em.mutate_ship(gov_id, [&](Ship& s) { ctx.em.destroy_ship(s); });
  ctx.verify_universe_invariants();

  // 17. Command matrix validation (roles, guests, governor, scopes)
  TestCommandMatrix(ctx, "give")
      .with_valid_argv(
          {"give", "Klingons", std::format("#{}", ship_no_ap.value)})
      .with_invalid_argv({"give", "NonExistent", "#1"})
      .run_matrix(g);
}

}  // namespace

int main() {
  test_give_dispatch();
  std::println(std::cout, "\n✅ All give command tests passed!");
  return 0;
}
