// SPDX-License-Identifier: Apache-2.0

/// \file berserker_bombard_test.cc
/// \brief Unit tests for Berserker ship planetary bombardment targeting and PDN
/// interception defenses.

import dallib;
import gb.entities;
import gb.services;
import gb.turn;
import test;
import std;

int main() {
  TestContext ctx;

  // Create Race 1 (Attacker)
  Race race1{};
  race1.Playernum = 1;
  race1.Guest = false;
  race1.declare_war_on(player_t{2});  // At war with Race 2

  // Create Race 2 (Target 1 - At War)
  Race race2{};
  race2.Playernum = 2;
  race2.Guest = false;

  // Create Race 3 (Target 2 - Not At War)
  Race race3{};
  race3.Playernum = 3;
  race3.Guest = false;

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race1);
  races.save(race2);
  races.save(race3);

  // Create Star system and Planet with sectors for Race 2 and Race 3
  ctx.create_star("Sol", 1).build();
  ctx.create_planet(1, PlanetType::EARTH, Coordinates{10, 10}, 1)
      .named("TestPlanet")
      .with_colony(3, 100, Coordinates{3, 3})
      .with_colony(2, 100, Coordinates{5, 5})
      .build();
  Planet planet(ctx.em.peek_planet(1, 1)->get_struct());

  // Create Berserker Ship
  auto ship_handle =
      TestShipBuilder(ctx.em, ShipType::OTYPE_BERS, 1)
          .owned_by(1)
          .with_alive(true)
          .with_on(true)
          .with_guns(guntype_t::HEAVY, 10, ActiveBattery::PRIMARY)
          .with_destruct(100)
          .in_planet_orbit(1, 1)
          .build_handle();
  Ship& ship = *ship_handle;

  // Test 1: Bombardment prioritizes war target (Race 2 at 5,5)
  int destroyed = berserker_bombard(ctx.em, ship, planet, race1);
  test::expect_gt(destroyed, 0);

  // Test 2: PDN presence prevents bombardment
  TestShipBuilder(ctx.em, ShipType::OTYPE_PLANDEF, 2)
      .owned_by(2)
      .with_alive(true)
      .with_on(true)
      .in_planet_orbit(1, 1)
      .build();

  int pdn_destroyed = berserker_bombard(ctx.em, ship, planet, race1);
  test::expect_eq(pdn_destroyed, 0);

  // Test 3: Planet with only wasted sectors has no valid targets
  {
    ctx.create_planet(1, PlanetType::EARTH, Coordinates{5, 5}, 2)
        .named("WastedPlanet")
        .with_all_sectors(SectorType::SEC_WASTED, 100, 100, 100, 2)
        .build();
    Planet peaceful_planet(ctx.em.peek_planet(1, 2)->get_struct());

    ship.enter_planet_orbit(1, 2);
    ship.set_planet_destination(1, 2);
    ship.notified() = 0;
    int wasted_destroyed =
        berserker_bombard(ctx.em, ship, peaceful_planet, race1);
    test::expect_eq(wasted_destroyed, 0);
    test::expect_eq(ship.notified(), 1);
  }

  // Test 4: Ship with no weapons (destruct == 0) notifies player of lack of
  // weapons
  {
    ship.enter_planet_orbit(1, 1);
    ship.set_planet_destination(1, 1);
    ship.notified() = 0;
    ship.destruct() = 0;
    // Disable PDN defense
    ctx.em.mutate_ship(2, [&](Ship& s) { ctx.em.kill_ship(1, s); });

    int no_weapon_destroyed = berserker_bombard(ctx.em, ship, planet, race1);
    test::expect_eq(no_weapon_destroyed, 0);
    test::expect_eq(ship.notified(), 1);
  }

  // =========================================================================
  // Test 5: check_orbital_pdn_defense unit tests
  // =========================================================================
  {
    const auto& orbit_planet =
        *ctx.create_planet(1, PlanetType::EARTH, Coordinates{5, 5}, 3)
             .named("OrbitPlanet")
             .build_and_peek();

    // 1. Empty orbit has no PDN defense
    test::expect_false(
        check_orbital_pdn_defense(ctx.em, orbit_planet, player_t{1}));

    // 2. Friendly PDN does not block friendly bombardment
    auto f_handle = TestShipBuilder(ctx.em, ShipType::OTYPE_PLANDEF, 201)
                        .owned_by(1)
                        .with_alive(true)
                        .in_planet_orbit(1, 3)
                        .build_handle();

    test::expect_false(
        check_orbital_pdn_defense(ctx.em, orbit_planet, player_t{1}));

    // 3. Enemy non-PDN does not block bombardment
    auto c_handle = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO, 202)
                        .owned_by(2)
                        .with_alive(true)
                        .in_planet_orbit(1, 3)
                        .build_handle();

    test::expect_false(
        check_orbital_pdn_defense(ctx.em, orbit_planet, player_t{1}));

    // 4. Dead enemy PDN does not block bombardment
    TestShipBuilder(ctx.em, ShipType::OTYPE_PLANDEF, 203)
        .owned_by(2)
        .with_alive(true)
        .in_planet_orbit(1, 3)
        .build();
    ctx.em.mutate_ship(203, [&](Ship& s) { ctx.em.kill_ship(1, s); });

    test::expect_false(
        check_orbital_pdn_defense(ctx.em, orbit_planet, player_t{1}));

    // 5. Active hostile PDN blocks bombardment
    auto h_handle = TestShipBuilder(ctx.em, ShipType::OTYPE_PLANDEF, 204)
                        .owned_by(2)
                        .with_alive(true)
                        .in_planet_orbit(1, 3)
                        .build_handle();

    test::expect_true(
        check_orbital_pdn_defense(ctx.em, orbit_planet, player_t{1}));
  }

  // =========================================================================
  // Test 6: Ship::bombardment_strength unit tests
  // =========================================================================
  {
    auto test_ship = TestShipBuilder(ctx.em, ShipType::OTYPE_BERS, 301)
                         .owned_by(1)
                         .with_guns(guntype_t::HEAVY, 40)
                         .with_destruct(500)
                         .with_damage(0)
                         .build_handle();

    // Full guns (40), 0 damage, 500 destruct -> 40
    test::expect_eq(test_ship->bombardment_strength(), 40U);

    // Bounded by available destruct crystals: 40 guns, 15 destruct -> 15
    test_ship->destruct() = 15;
    test::expect_eq(test_ship->bombardment_strength(), 15U);

    // Hull efficiency degradation: 50% damage -> 20 effective guns, 100
    // destruct
    // -> 20
    test_ship->destruct() = 100;
    test_ship->admin_override_damage(50);
    test::expect_eq(test_ship->bombardment_strength(), 20U);

    // Zero destruct crystals -> 0
    test_ship->destruct() = 0;
    test::expect_eq(test_ship->bombardment_strength(), 0U);

    // 100% hull damage -> 0
    test_ship->destruct() = 100;
    test_ship->admin_override_damage(100);
    test::expect_eq(test_ship->bombardment_strength(), 0U);

    // Non-combat ship with 0 guns (e.g. Spore Pod) -> 0
    auto pod_test_ship = TestShipBuilder(ctx.em, ShipType::STYPE_POD, 302)
                             .owned_by(1)
                             .with_destruct(100)
                             .build_handle();
    test::expect_eq(pod_test_ship->bombardment_strength(), 0U);
  }

  // =========================================================================
  // Test 7: find_bombardment_target unit tests
  // =========================================================================
  {
    // Setup sectors on planet 4:
    // (1, 1) = owned by Race 3 (foreign, peaceful)
    // (2, 2) = owned by Race 2 (at war)
    // (3, 3) = owned by Race 1 (friendly)
    ctx.create_planet(1, PlanetType::EARTH, Coordinates{5, 5}, 4)
        .named("TargetPlanet")
        .with_colony(3, 100, Coordinates{1, 1})
        .with_colony(2, 100, Coordinates{2, 2})
        .with_colony(1, 100, Coordinates{3, 3})
        .build();

    // 1. General berserker prioritizes war target (Race 2 at (2, 2)) over Race
    // 3
    auto gen_ship = TestShipBuilder(ctx.em, ShipType::OTYPE_BERS, 401)
                        .owned_by(1)
                        .in_planet_orbit(1, 4)
                        .build_handle();

    auto target = find_bombardment_target(ctx.em, *gen_ship, race1);
    test::expect_true(target.has_value());
    test::expect_eq(*target, (Coordinates{2, 2}));

    // 2. Programmed berserker specifically targeting Race 3 prioritizes Race 3
    // at (1, 1)
    auto prog_ship = TestShipBuilder(ctx.em, ShipType::OTYPE_BERS, 402)
                         .owned_by(1)
                         .in_planet_orbit(1, 4)
                         .with_special(MindData{.target_player = player_t{3}})
                         .build_handle();

    auto prog_target = find_bombardment_target(ctx.em, *prog_ship, race1);
    test::expect_true(prog_target.has_value());
    test::expect_eq(*prog_target, (Coordinates{1, 1}));

    // 3. If no war target exists, falls back to foreign colony
    Race race4{};
    race4.Playernum = 4;
    race4.Guest = false;
    races.save(race4);

    auto neutral_ship = TestShipBuilder(ctx.em, ShipType::OTYPE_BERS, 403)
                            .owned_by(4)
                            .in_planet_orbit(1, 4)
                            .build_handle();

    auto neutral_target = find_bombardment_target(ctx.em, *neutral_ship, race4);
    test::expect_true(neutral_target.has_value());
    test::expect_true(*neutral_target == (Coordinates{1, 1}) ||
                      *neutral_target == (Coordinates{2, 2}) ||
                      *neutral_target == (Coordinates{3, 3}));
  }

  // =========================================================================
  // Test 8: dispatch_bombardment_alerts unit tests
  // =========================================================================
  {
    ctx.em.purge_all_telegrams();
    const auto& star = *ctx.em.peek_star(starnum_t{1});
    auto alert_ship = TestShipBuilder(ctx.em, ShipType::OTYPE_BERS, 501)
                          .owned_by(1)
                          .named("Nemesis")
                          .in_planet_orbit(1, 1)
                          .build_handle();

    BombardResult result{
        .sectors_destroyed = 3,
        .short_message = "Direct kinetic impact on surface.\n",
        .long_message = "",
    };
    result.nuked_players.insert(player_t{2});

    dispatch_bombardment_alerts(ctx.em, *alert_ship, star, Coordinates{5, 5},
                                player_t{2}, 3, result);

    // Attacker (Player 1) received bombardment report
    test::expect_true(ctx.em.has_telegrams(player_t{1}, governor_t{1}));
    const auto attacker_telegrams =
        ctx.em.get_telegrams(player_t{1}, governor_t{1});
    test::expect_false(attacker_telegrams.empty());
    test::expect_true(attacker_telegrams[0].message.contains(
        std::format("REPORT from ship #{}", alert_ship->number())));
    test::expect_true(
        attacker_telegrams[0].message.contains("3 sectors destroyed"));

    // Victim (Player 2) received alert
    test::expect_true(ctx.em.has_telegrams(player_t{2}, governor_t{1}));
    const auto victim_telegrams =
        ctx.em.get_telegrams(player_t{2}, governor_t{1});
    test::expect_false(victim_telegrams.empty());
    test::expect_true(
        victim_telegrams[0].message.contains("ALERT from planet"));
    test::expect_true(
        victim_telegrams[0].message.contains("bombarded sector 5,5"));
  }

  std::println(std::cout, "berserker_bombard_test: All tests passed!");
  return 0;
}
