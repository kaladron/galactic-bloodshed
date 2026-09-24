// SPDX-License-Identifier: Apache-2.0

/// \file move_test.cc
/// \brief Unit tests for get_move() numeric (1-9) and vi-key directional
/// mappings, boundary conditions, and cylindrical wrapping.

import dallib;
import gb.entities;
import gb.services;
import gb.turn;
import test;
import std;

int main() {
  // Create a test planet with known dimensions
  Planet planet(PlanetType::EARTH, Coordinates{10, 8});

  // Test numeric direction mappings (1-9, excluding 5)

  // Direction '1' (southwest): x-1, y+1 with x-wrapping
  {
    auto result = get_move(planet, '1', {5, 3});
    test::expect_eq(result.x, 4);
    test::expect_eq(result.y, 4);

    // Test x-wrapping at left boundary
    result = get_move(planet, '1', {0, 3});
    test::expect_eq(result.x, 9);  // wraps to Maxx-1
    test::expect_eq(result.y, 4);
  }

  // Direction '2' (south): x unchanged, y+1
  {
    auto result = get_move(planet, '2', {5, 3});
    test::expect_eq(result.x, 5);
    test::expect_eq(result.y, 4);
  }

  // Direction '3' (southeast): x+1, y+1 with x-wrapping
  {
    auto result = get_move(planet, '3', {5, 3});
    test::expect_eq(result.x, 6);
    test::expect_eq(result.y, 4);

    // Test x-wrapping at right boundary
    result = get_move(planet, '3', {9, 3});
    test::expect_eq(result.x, 0);  // wraps to 0
    test::expect_eq(result.y, 4);
  }

  // Direction '4' (west): x-1, y unchanged with x-wrapping
  {
    auto result = get_move(planet, '4', {5, 3});
    test::expect_eq(result.x, 4);
    test::expect_eq(result.y, 3);

    // Test x-wrapping at left boundary
    result = get_move(planet, '4', {0, 3});
    test::expect_eq(result.x, 9);  // wraps to Maxx-1
    test::expect_eq(result.y, 3);
  }

  // Direction '6' (east): x+1, y unchanged with x-wrapping
  {
    auto result = get_move(planet, '6', {5, 3});
    test::expect_eq(result.x, 6);
    test::expect_eq(result.y, 3);

    // Test x-wrapping at right boundary
    result = get_move(planet, '6', {9, 3});
    test::expect_eq(result.x, 0);  // wraps to 0
    test::expect_eq(result.y, 3);
  }

  // Direction '7' (northwest): x-1, y-1 with x-wrapping
  {
    auto result = get_move(planet, '7', {5, 3});
    test::expect_eq(result.x, 4);
    test::expect_eq(result.y, 2);

    // Test x-wrapping at left boundary
    result = get_move(planet, '7', {0, 3});
    test::expect_eq(result.x, 9);  // wraps to Maxx-1
    test::expect_eq(result.y, 2);
  }

  // Direction '8' (north): x unchanged, y-1
  {
    auto result = get_move(planet, '8', {5, 3});
    test::expect_eq(result.x, 5);
    test::expect_eq(result.y, 2);
  }

  // Direction '9' (northeast): x+1, y-1 with x-wrapping
  {
    auto result = get_move(planet, '9', {5, 3});
    test::expect_eq(result.x, 6);
    test::expect_eq(result.y, 2);

    // Test x-wrapping at right boundary
    result = get_move(planet, '9', {9, 3});
    test::expect_eq(result.x, 0);  // wraps to 0
    test::expect_eq(result.y, 2);
  }

  // Test letter direction mappings (vi-like movement keys)

  // 'b' maps to '1' (southwest)
  {
    auto result = get_move(planet, 'b', {5, 3});
    test::expect_eq(result.x, 4);
    test::expect_eq(result.y, 4);
  }

  // 'k' maps to '2' (south)
  {
    auto result = get_move(planet, 'k', {5, 3});
    test::expect_eq(result.x, 5);
    test::expect_eq(result.y, 4);
  }

  // 'n' maps to '3' (southeast)
  {
    auto result = get_move(planet, 'n', {5, 3});
    test::expect_eq(result.x, 6);
    test::expect_eq(result.y, 4);
  }

  // 'h' maps to '4' (west)
  {
    auto result = get_move(planet, 'h', {5, 3});
    test::expect_eq(result.x, 4);
    test::expect_eq(result.y, 3);
  }

  // 'l' maps to '6' (east)
  {
    auto result = get_move(planet, 'l', {5, 3});
    test::expect_eq(result.x, 6);
    test::expect_eq(result.y, 3);
  }

  // 'y' maps to '7' (northwest)
  {
    auto result = get_move(planet, 'y', {5, 3});
    test::expect_eq(result.x, 4);
    test::expect_eq(result.y, 2);
  }

  // 'j' maps to '8' (north)
  {
    auto result = get_move(planet, 'j', {5, 3});
    test::expect_eq(result.x, 5);
    test::expect_eq(result.y, 2);
  }

  // 'u' maps to '9' (northeast)
  {
    auto result = get_move(planet, 'u', {5, 3});
    test::expect_eq(result.x, 6);
    test::expect_eq(result.y, 2);
  }

  // Test boundary conditions for y-coordinates (no wrapping)

  // Moving south from bottom edge
  {
    auto result = get_move(planet, '2', {5, 7});  // Maxy-1
    test::expect_eq(result.x, 5);
    test::expect_eq(result.y, 8);  // Can go beyond Maxy
  }

  // Moving north from top edge
  {
    auto result = get_move(planet, '8', {5, 0});
    test::expect_eq(result.x, 5);
    test::expect_eq(result.y, -1);  // Can go below 0
  }

  // Test edge cases with different planet sizes

  // Test with minimal planet size
  Planet small_planet(PlanetType::ASTEROID, Coordinates{2, 3});

  {
    // Test wrapping on small planet
    auto result = get_move(small_planet, '6', {1, 1});  // east from x=1
    test::expect_eq(result.x, 0);                       // wraps to 0
    test::expect_eq(result.y, 1);

    result = get_move(small_planet, '4', {0, 1});  // west from x=0
    test::expect_eq(result.x, 1);                  // wraps to Maxx-1
    test::expect_eq(result.y, 1);
  }

  // Test invalid directions (should return original coordinates)
  {
    auto result =
        get_move(planet, '5', {5, 3});  // '5' is not a valid direction
    test::expect_eq(result.x, 5);
    test::expect_eq(result.y, 3);  // unchanged

    result = get_move(planet, 'z', {5, 3});  // 'z' is not a valid direction
    test::expect_eq(result.x, 5);
    test::expect_eq(result.y, 3);  // unchanged

    result = get_move(planet, '0', {5, 3});  // '0' is not a valid direction
    test::expect_eq(result.x, 5);
    test::expect_eq(result.y, 3);  // unchanged
  }

  // Test at exact boundary conditions
  {
    // Test at planet.dimensions().x-1 boundary (rightmost valid position)
    auto result =
        get_move(planet, '6', {9, 3});  // x == dimensions().x-1, moving east
    test::expect_eq(result.x, 0);       // wraps to 0
    test::expect_eq(result.y, 3);

    // Test at x == 0 boundary (leftmost position)
    result = get_move(planet, '4', {0, 3});  // west from x=0
    test::expect_eq(result.x, 9);            // wraps to dimensions().x-1
    test::expect_eq(result.y, 3);
  }

  // Test coordinates preservation in Coordinates struct
  {
    Coordinates start{7, 2};
    auto result = get_move(planet, '6', start);
    test::expect_eq(result.x, 8);
    test::expect_eq(result.y, 2);
    // Verify original coordinates unchanged
    test::expect_eq(start.x, 7);
    test::expect_eq(start.y, 2);
  }

  // Test mech_defend, mech_attack_people, people_attack_mech, and ground_attack
  {
    TestContext ctx;
    ctx.with_standard_universe();
    ctx.em.mutate_race(1, [](Race& r) {
      r.tech = 50.0;
      r.fighters = 5;
      r.morale = 1000;
      r.likes[SectorType::SEC_LAND] = 1.0;
    });
    ctx.em.mutate_race(2, [](Race& r) {
      r.tech = 20.0;
      r.fighters = 2;
      r.morale = 500;
      r.likes[SectorType::SEC_LAND] = 1.0;
    });

    auto& registry = get_test_session_registry();
    GameObj g(ctx.em, registry);
    ctx.setup_game_obj(g, 1, 1);
    g.set_level(ScopeLevel::LEVEL_PLAN);
    g.set_snum(1);
    g.set_pnum(1);

    // Create a hostile Player 2 AFV landed at (3, 3) with 1 destruct
    const shipnum_t afv_id = TestShipBuilder(ctx.em, ShipType::OTYPE_AFV)
                                 .owned_by(2, 1)
                                 .landed_on(1, 1, Coordinates{3, 3})
                                 .with_crew(1, 0)
                                 .with_guns(guntype_t::MEDIUM, 2)
                                 .with_retaliate(2)
                                 .with_destruct(1)
                                 .with_tech(20.0)
                                 .build();

    const auto* p_earth = ctx.em.peek_planet(1, 1);
    const auto* smap = ctx.em.peek_sectormap(1, 1);
    const Sector& target_sect = smap->get(Coordinates{3, 3});

    // 1. Allied AFVs do not fire on allied troops
    ctx.em.mutate_race(1, [](Race& r) { r.allied.set(player_t{2}); });
    ctx.em.mutate_race(2, [](Race& r) { r.allied.set(player_t{1}); });
    ctx.setup_game_obj(g, 1, 1);
    population_t entering_troops = 1000;
    mech_defend(g, &entering_troops, PopulationType::MIL, *p_earth,
                Coordinates{3, 3}, target_sect);
    test::expect_eq(entering_troops, 1000u);

    // 2. Hostile AFV engages entering troops and takes counter-attack damage
    // even when its destruct drops to 0 after firing its last shell
    ctx.em.mutate_race(1, [](Race& r) { r.allied.reset(player_t{2}); });
    ctx.em.mutate_race(2, [](Race& r) { r.allied.reset(player_t{1}); });
    ctx.setup_game_obj(g, 1, 1);
    seed_rand(42);
    mech_defend(g, &entering_troops, PopulationType::MIL, *p_earth,
                Coordinates{3, 3}, target_sect);
    const auto* afv_after = ctx.em.peek_ship(afv_id);
    test::expect_eq(afv_after->destruct(), 0);
    test::expect_true(afv_after->damage() > 0 || !afv_after->alive());

    // 3. Direct mech_attack_people with ignore=false (consumes full salvo)
    ctx.em.mutate_ship(afv_id, [](Ship& s) {
      s.alive() = true;
      s.repair_damage(100);
      s.destruct() = 5;
    });
    population_t civ = 20;
    population_t mil = 10;
    ctx.em.mutate_ship(afv_id, [&](Ship& s) {
      auto [short_msg, long_msg] =
          mech_attack_people(ctx.em, s, &civ, &mil, *ctx.em.peek_race(2),
                             *ctx.em.peek_race(1), target_sect, false);
      test::expect_contains(long_msg, "Battle at 3,3");
      test::expect_eq(s.destruct(), 3);  // 5 - 2 salvo
    });

    // 4. ground_attack with CIV and MIL attackers
    const auto outcome = ground_attack({
        .attacker = *ctx.em.peek_race(1),
        .defender = *ctx.em.peek_race(2),
        .attacker_force = 100,
        .attacker_type = PopulationType::CIV,
        .defender_civ = 20,
        .defender_mil = 5,
        .attacker_defense_bonus = 1,
        .defender_defense_bonus = 1,
        .attacker_compatibility = 1.0,
        .defender_compatibility = 1.0,
    });
    test::expect_true(outcome.attack_strength > 0.0);
    test::expect_true(outcome.defense_strength > 0.0);
    test::expect_eq(outcome.surviving_attackers + outcome.attacker_casualties,
                    100);
    test::expect_eq(
        outcome.surviving_defender_civ + outcome.defender_civ_casualties, 20);
    test::expect_eq(
        outcome.surviving_defender_mil + outcome.defender_mil_casualties, 5);
  }

  std::println(std::cout, "All get_move tests passed!");
  return 0;
}
