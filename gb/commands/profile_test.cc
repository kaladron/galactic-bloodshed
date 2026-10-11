// SPDX-License-Identifier: Apache-2.0

/// \file profile_test.cc
/// \brief Test profile command functionality and reporting via
/// CommandDescriptor.

import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_profile_dispatch() {
  std::println(std::cout, "Test: profile command dispatch and reporting");

  TestContext ctx;
  ctx.with_standard_universe();
  TestShipBuilder(ctx.em, ShipType::OTYPE_GOV, 100)
      .owned_by(1, 1)
      .landed_on(1, 1, Coordinates{0, 0})
      .with_crew(100, 0)
      .build();
  ctx.em.mutate_race(1, [](Race& r) { r.Gov_ship = 100; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  // 1. Profile for self (default baseline: capital #100, normal race, mortal)
  ctx.assert_dispatch_success(g, {"profile"});
  std::string out = g.out.str();
  test::expect_contains(out, "Racial profile for Federation");
  test::expect_contains(out, "Default Scope: /Sol/Earth");
  test::expect_contains(out, "Designated Capital: #100");
  test::expect_contains(out, "Normal Race");
  std::println(std::cout, "    ✓ Baseline self profile verified");

  // 2. Profile for self without capital, with metamorphosis, deity, and
  // discoveries
  ctx.em.mutate_race(1, [](Race& r) {
    r.God = true;
    r.Gov_ship = std::nullopt;
    r.morale = 100;
    r.Metamorph = true;
    r.discoveries.crystal = true;
    r.discoveries.hyperdrive = true;
    r.discoveries.laser = true;
    r.discoveries.cew = true;
    r.discoveries.vn = true;
    r.discoveries.tractor_beam = true;
    r.discoveries.transporter = true;
    r.discoveries.avpm = true;
    r.discoveries.cloak = true;
    r.discoveries.wormhole = true;
  });

  ctx.assert_dispatch_success(g, {"profile"});
  out = g.out.str();
  test::expect_contains(out, "*** Diety Status ***");
  test::expect_contains(out, "NO DESIGNATED CAPITAL!!");
  test::expect_contains(out, "Morale: 100");
  test::expect_contains(out, "Metamorphic Race");
  test::expect_contains(out, "Crystals");
  test::expect_contains(out, "Hyper-drive");
  test::expect_contains(out, "Combat Lasers");
  test::expect_contains(out, "Confined Energy Weapons");
  test::expect_contains(out, "Von Neumann Machines");
  test::expect_contains(out, "Tractor Beam");
  test::expect_contains(out, "Transporter");
  test::expect_contains(out, "AVPM");
  test::expect_contains(out, "Cloaking");
  test::expect_contains(out, "Wormhole");
  std::println(std::cout,
               "    ✓ Advanced self profile (capital, deity, tech) verified");

  // 3. Profile for other player by name with high translation (> 80)
  ctx.em.mutate_race(1, [](Race& r) {
    r.God = false;
    r.translate[player_t{2}] = Percentage{90};
  });
  ctx.em.mutate_race(2, [](Race& r) {
    r.info = "Warrior empire";
    r.likesbest = SectorType::SEC_MOUNT;
  });

  ctx.assert_dispatch_success(g, {"profile", "Klingons"});
  out = g.out.str();
  test::expect_contains(out, "Race report on Klingons");
  test::expect_contains(out, "Personal: Warrior empire");
  test::expect_contains(out, "Normal Race");
  test::expect_contains(out, sector_type_name(SectorType::SEC_MOUNT));
  std::println(std::cout,
               "    ✓ Other player profile (translate > 80) verified");

  // 4. Profile for other player with low translation (<= 50)
  ctx.em.mutate_race(
      1, [](Race& r) { r.translate[player_t{2}] = Percentage{30}; });

  ctx.assert_dispatch_success(g, {"profile", "2"});
  out = g.out.str();
  test::expect_contains(out, "Race report on Klingons");
  test::expect_contains(out, "Unknown Race");
  test::expect_contains(out, " ? ");
  std::println(std::cout,
               "    ✓ Other player profile (translate <= 50) verified");

  // 5. Profile for other player when viewer is God and target is mortal
  // Metamorph (race.God && !r.God)
  ctx.em.mutate_race(1, [](Race& r) { r.God = true; });
  ctx.em.mutate_race(2, [](Race& r) {
    r.God = false;
    r.Metamorph = true;
  });

  ctx.assert_dispatch_success(g, {"profile", "2"});
  out = g.out.str();
  test::expect_false(out.contains("*** Deity Status ***"));
  test::expect_contains(out, "Metamorphic Race");
  std::println(std::cout,
               "    ✓ God viewing mortal Metamorph race profile verified");

  // 6. Profile for other player with mutual deity status (race.God && r.God)
  ctx.em.mutate_race(2, [](Race& r) { r.God = true; });

  ctx.assert_dispatch_success(g, {"profile", "2"});
  out = g.out.str();
  test::expect_contains(out, "*** Deity Status ***");
  std::println(std::cout, "    ✓ Mutual deity status verified");

  // 7. JSON mode rendering for both self and other profile
  g.set_ui_mode(UiMode::JSON);
  ctx.assert_dispatch_success(g, {"profile"});
  test::expect_contains(g.out.str(), "\"type\":\"profile_self\"");
  test::expect_contains(g.out.str(), "\"race_name\":\"Federation\"");

  ctx.assert_dispatch_success(g, {"profile", "2"});
  test::expect_contains(g.out.str(), "\"type\":\"profile_other\"");
  test::expect_contains(g.out.str(), "\"race_name\":\"Klingons\"");
  g.set_ui_mode(UiMode::ASCII);
  std::println(std::cout, "    ✓ JSON mode profile rendering verified");

  // 8. Error case: non-existent player
  ctx.assert_dispatch_rejected(g, {"profile", "99"});
  test::expect_contains(g.out.str(), "Player does not exist");
  std::println(std::cout, "    ✓ Non-existent player rejected");

  // 9. Command matrix validation (roles, guests, governor, scopes)
  TestCommandMatrix(ctx, "profile")
      .with_valid_argv({"profile"})
      .with_invalid_argv({"profile", "99"})
      .run_matrix(g);
}

}  // namespace

int main() {
  test_profile_dispatch();
  std::println(std::cout, "\n✅ All profile tests passed!");
  return 0;
}
