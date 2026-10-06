// SPDX-License-Identifier: Apache-2.0

/// \file ground_attack_test.cc
/// \brief Unit tests for mech_attack_people and people_attack_mech ground
/// combat calculations.

import dallib;
import gb.entities;
import gb.mechanics;
import gb.presentation;
import gb.services;
import gb.turn;
import test;
import std;

void test_mech_attack_people() {
  std::println(std::cout, "Test: mech_attack_people");

  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  Race race{};
  race.Playernum = player_t{1};
  race.name = "AttackerRace";
  race.tech = 10.0;
  race.morale = 10;
  race.likes[SectorType::SEC_LAND] = 1.0;

  Race alien{};
  alien.Playernum = player_t{2};
  alien.name = "DefenderRace";
  alien.tech = 10.0;
  alien.morale = 10;
  alien.likes[SectorType::SEC_LAND] = 1.0;

  Sector sect{};
  sect.set_condition(SectorType::SEC_LAND);

  Ship ship{};
  ship.number() = 1;
  ship.owner() = player_t{1};
  ship.type() = ShipType::OTYPE_AFV;
  ship.tech() = 10.0;
  ship.armor() = 10;
  ship.alive() = true;
  ship.popn() = 10;
  ship.retaliate() = 100;
  ship.destruct() = 100;
  ship.guns() = PRIMARY;
  ship.set_primary_battery(10, guntype_t::HEAVY);

  population_t civ = 100;
  population_t mil = 50;

  const auto res =
      mech_attack_people(em, ship, &civ, &mil, race, alien, sect, true);
  const auto short_buf = GB::presentation::render_mech_attack_people_short(res);
  const auto long_buf = GB::presentation::render_mech_attack_people_long(res);

  test::expect_false(short_buf.empty());
  test::expect_false(long_buf.empty());
  test::expect_contains(long_buf, "Battle at");
  test::expect_eq(res.initial_civ, 100u);
  test::expect_eq(res.initial_mil, 50u);
  test::expect_eq(res.surviving_civ, civ);
  test::expect_eq(res.surviving_mil, mil);

  // Zero-defense and zero-attack edge cases
  population_t zero_civ = 0;
  population_t zero_mil = 0;
  const auto zero_def_res = mech_attack_people(em, ship, &zero_civ, &zero_mil,
                                               race, alien, sect, false);
  test::expect_contains(
      GB::presentation::render_mech_attack_people_short(zero_def_res),
      "slaughtered");

  ship.tech() = 0.0;
  const auto zero_both_res = mech_attack_people(em, ship, &zero_civ, &zero_mil,
                                                race, alien, sect, true);
  test::expect_eq(zero_both_res.attack_strength, 0.0);
  test::expect_eq(zero_both_res.defense_strength, 0.0);

  std::println(
      std::cout,
      "  ✓ mech_attack_people passed (civ remaining={}, mil remaining={})", civ,
      mil);
}

void test_people_attack_mech() {
  std::println(std::cout, "Test: people_attack_mech");

  TestContext ctx;
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& race) {
    race.name = "AttackerPeople";
    race.tech = 10.0;
    race.fighters = 5;
    race.morale = 10;
    race.likes[SectorType::SEC_LAND] = 1.0;
  });
  ctx.em.mutate_race(2, [](Race& alien) {
    alien.name = "MechOwner";
    alien.tech = 10.0;
    alien.morale = 10;
    alien.likes[SectorType::SEC_LAND] = 1.0;
  });

  const auto& race = *ctx.em.peek_race(1);
  const auto& alien = *ctx.em.peek_race(2);

  Sector sect{};
  sect.set_condition(SectorType::SEC_LAND);

  Ship ship{};
  ship.number() = 0;
  ship.owner() = player_t{2};
  ship.type() = ShipType::OTYPE_AFV;
  ship.tech() = 10.0;
  ship.armor() = 5;
  ship.alive() = true;
  ship.popn() = 10;
  ship.retaliate() = 100;
  ship.destruct() = 100;
  ship.guns() = PRIMARY;
  ship.set_primary_battery(5, guntype_t::HEAVY);

  const auto res = people_attack_mech(ctx.em, ship, 100, 50, race, alien, sect,
                                      Coordinates{1, 1});
  const auto short_buf = GB::presentation::render_people_attack_mech_short(res);
  const auto long_buf = GB::presentation::render_people_attack_mech_long(res);

  test::expect_false(short_buf.empty());
  test::expect_false(long_buf.empty());
  test::expect_contains(long_buf, "assault");
  test::expect_eq(res.attacker_civ, 100u);
  test::expect_eq(res.attacker_mil, 50u);

  // Zero-defense and zero-attack edge cases (including mech destruction)
  ship.tech() = 0.0;
  std::ignore = ship.apply_damage(99);
  const auto zero_def_res = people_attack_mech(ctx.em, ship, 100, 50, race,
                                               alien, sect, Coordinates{1, 1});
  test::expect_eq(zero_def_res.defense_strength, 0.0);

  ship.alive() = true;
  ship.repair_damage(100);
  const auto zero_both_res = people_attack_mech(ctx.em, ship, 0, 0, race, alien,
                                                sect, Coordinates{1, 1});
  test::expect_eq(zero_both_res.attack_strength, 0.0);
  test::expect_eq(zero_both_res.defense_strength, 0.0);

  std::println(std::cout, "  ✓ people_attack_mech passed (ship damage={})",
               ship.damage());
}

void test_mech_defend_mcdc() {
  std::println(std::cout, "Test: mech_defend_mcdc");

  TestContext ctx;
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) {
    r.tech = 10.0;
    r.fighters = 1;
    r.morale = 100;
    r.likes[SectorType::SEC_LAND] = 1.0;
  });
  ctx.em.mutate_race(2, [](Race& r) {
    r.tech = 500.0;
    r.fighters = 10;
    r.morale = 5000;
    r.likes[SectorType::SEC_LAND] = 1.0;
  });

  // 1. Friendly AFV at target (ship.owner() == attacker_race.Playernum) ->
  // skipped
  TestShipBuilder(ctx.em, ShipType::OTYPE_AFV)
      .owned_by(1, 1)
      .landed_on(1, 1, Coordinates{3, 3})
      .with_crew(1, 0)
      .with_guns(guntype_t::MEDIUM, 2)
      .with_retaliate(2)
      .with_destruct(5)
      .build();

  // 2. Enemy non-AFV ship at target (ship.type() != ShipType::OTYPE_AFV) ->
  // skipped
  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
      .owned_by(2, 1)
      .landed_on(1, 1, Coordinates{3, 3})
      .with_crew(5, 0)
      .with_guns(guntype_t::MEDIUM, 2)
      .with_retaliate(2)
      .with_destruct(5)
      .build();

  // 3. Enemy AFV in orbit (!ship.is_landed()) -> skipped
  TestShipBuilder(ctx.em, ShipType::OTYPE_AFV)
      .owned_by(2, 1)
      .in_planet_orbit(1, 1)
      .with_crew(1, 0)
      .with_guns(guntype_t::MEDIUM, 2)
      .with_retaliate(2)
      .with_destruct(5)
      .build();

  // 4. Enemy landed AFV with 0 destruct (ship.retal_strength() == 0) -> skipped
  TestShipBuilder(ctx.em, ShipType::OTYPE_AFV)
      .owned_by(2, 1)
      .landed_on(1, 1, Coordinates{3, 3})
      .with_crew(1, 0)
      .with_guns(guntype_t::MEDIUM, 2)
      .with_retaliate(2)
      .with_destruct(0)
      .build();

  // 5. Enemy landed AFV on different sector (ship.land_coords() !=
  // target_coords) -> skipped
  TestShipBuilder(ctx.em, ShipType::OTYPE_AFV)
      .owned_by(2, 1)
      .landed_on(1, 1, Coordinates{4, 4})
      .with_crew(1, 0)
      .with_guns(guntype_t::MEDIUM, 2)
      .with_retaliate(2)
      .with_destruct(5)
      .build();

  // 6. Hostile enemy landed AFV #1 at (3, 3) with overwhelming firepower
  const shipnum_t hostile_afv1 = TestShipBuilder(ctx.em, ShipType::OTYPE_AFV)
                                     .owned_by(2, 1)
                                     .landed_on(1, 1, Coordinates{3, 3})
                                     .with_crew(1, 0)
                                     .with_guns(guntype_t::HEAVY, 2)
                                     .with_retaliate(2)
                                     .with_destruct(10)
                                     .with_tech(500.0)
                                     .build();

  // 7. Trailing ship on planet so `if (civ + mil == 0) break;` is exercised
  // after hostile_afv1 wipes out the entering civilian force
  TestShipBuilder(ctx.em, ShipType::OTYPE_AFV)
      .owned_by(2, 1)
      .landed_on(1, 1, Coordinates{3, 3})
      .with_crew(1, 0)
      .with_guns(guntype_t::HEAVY, 2)
      .with_retaliate(2)
      .with_destruct(10)
      .with_tech(500.0)
      .build();

  const auto& p_earth = *ctx.em.peek_planet(1, 1);
  const auto& smap = *ctx.em.peek_sectormap(1, 1);
  const Sector& target_sect = smap.get(Coordinates{3, 3});

  // One-way alliance: Player 1 allied with Player 2, but Player 2 NOT allied
  // with Player 1 -> AFV still defends and slaughters the 1 entering civilian!
  ctx.em.mutate_race(1, [](Race& r) { r.declare_alliance_with(player_t{2}); });
  ctx.em.mutate_race(2, [](Race& r) { r.rescind_alliance_with(player_t{1}); });

  population_t entering_civs = 1;
  const auto defend_res =
      mech_defend(ctx.em, *ctx.em.peek_race(1), &entering_civs,
                  PopulationType::CIV, p_earth, Coordinates{3, 3}, target_sect);
  test::expect_eq(entering_civs, 0u);
  test::expect_eq(defend_res.surviving_people, 0u);
  test::expect_eq(defend_res.rounds.size(), 1u);
  test::expect_false(defend_res.rounds[0].people_counterattack.has_value());
  test::expect_true(ctx.em.peek_ship(hostile_afv1)->alive());

  std::println(std::cout, "  ✓ mech_defend_mcdc passed");
}

void test_ground_assault_matrix() {
  std::println(std::cout, "Test: ground_assault_matrix");

  player_t attacker{1};
  player_t defender{2};
  Star star(star_struct{.star_id = 5});

  test::expect_eq(star.ground_assault_count(attacker, defender), 0U);
  star.record_ground_assault(attacker, defender, 3);
  test::expect_eq(star.ground_assault_count(attacker, defender), 3U);

  // Bounds rejection tests
  test::expect_throws<std::out_of_range>(
      [&] { (void)star.ground_assault_count(player_t{0}, defender); });
  test::expect_throws<std::out_of_range>(
      [&] { (void)star.ground_assault_count(attacker, player_t{0}); });

  // Clear specific pair and all tallies
  star.clear_ground_assaults(attacker, defender);
  test::expect_eq(star.ground_assault_count(attacker, defender), 0U);

  star.record_ground_assault(attacker, defender, 2);
  star.clear_all_ground_assaults();
  test::expect_eq(star.ground_assault_count(attacker, defender), 0U);
  std::println(std::cout, "  ✓ ground_assault_matrix passed");
}

int main() {
  test_mech_attack_people();
  test_people_attack_mech();
  test_mech_defend_mcdc();
  test_ground_assault_matrix();

  std::println(std::cout, "\n✅ All ground attack tests passed!");
  return 0;
}
