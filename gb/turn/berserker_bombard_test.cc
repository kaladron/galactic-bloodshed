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
  race1.governor[0].active = true;
  race1.declare_war_on(player_t{2});  // At war with Race 2

  // Create Race 2 (Target 1 - At War)
  Race race2{};
  race2.Playernum = 2;
  race2.Guest = false;
  race2.governor[0].active = true;

  // Create Race 3 (Target 2 - Not At War)
  Race race3{};
  race3.Playernum = 3;
  race3.Guest = false;
  race3.governor[0].active = true;

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race1);
  races.save(race2);
  races.save(race3);

  // Create Star system
  star_struct ss{};
  ss.star_id = 0;
  ss.pnames.emplace_back("TestPlanet");
  ss.pnames.emplace_back("WastedPlanet");
  StarRepository star_repo(store);
  star_repo.save(ss);

  // Create Planet
  Planet planet{};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.dimensions() = Coordinates{10, 10};
  PlanetRepository planet_repo(store);
  planet_repo.save(planet);
  SectorRepository smap_repo(store);

  // Create Sector Map with sectors for Race 2 and Race 3
  {
    SectorMap smap(planet);
    smap.get(Coordinates{3, 3}).set_condition(SectorType::SEC_LAND);
    smap.get(Coordinates{3, 3}).set_popn_exact(100);
    smap.get(Coordinates{3, 3}).set_owner(3);  // Owned by Race 3 (not at war)

    smap.get(Coordinates{5, 5}).set_condition(SectorType::SEC_LAND);
    smap.get(Coordinates{5, 5}).set_popn_exact(100);
    smap.get(Coordinates{5, 5}).set_owner(2);  // Owned by Race 2 (at war)

    smap_repo.save_map(smap);
  }

  // Create Berserker Ship
  ship_struct b_ship{};
  b_ship.number = 1;
  b_ship.owner = 1;
  b_ship.governor = 0;
  b_ship.alive = true;
  b_ship.on = true;
  b_ship.type = ShipType::OTYPE_BERS;
  b_ship.guns = ActiveBattery::PRIMARY;
  b_ship.primary_battery = GunBattery::create(10, guntype_t::HEAVY);
  b_ship.destruct = 100;
  b_ship.whatorbits = ScopeLevel::LEVEL_PLAN;
  b_ship.storbits = 0;
  b_ship.pnumorbits = 0;

  auto ship_handle = ctx.em.create_ship(b_ship);
  Ship& ship = *ship_handle;

  // Test 1: Bombardment prioritizes war target (Race 2 at 5,5)
  int destroyed = berserker_bombard(ctx.em, ship, planet, race1);
  test::expect_gt(destroyed, 0);

  // Test 2: PDN presence prevents bombardment
  ship_struct pdn{};
  pdn.number = 2;
  pdn.owner = 2;
  pdn.governor = 0;
  pdn.alive = true;
  pdn.on = true;
  pdn.type = ShipType::OTYPE_PLANDEF;
  pdn.whatorbits = ScopeLevel::LEVEL_PLAN;
  pdn.storbits = 0;
  pdn.pnumorbits = 0;
  pdn.nextship = planet.ships();
  auto pdn_handle = ctx.em.create_ship(pdn);
  planet.ships() = pdn_handle->number();

  int pdn_destroyed = berserker_bombard(ctx.em, ship, planet, race1);
  test::expect_eq(pdn_destroyed, 0);

  // Test 3: Planet with only wasted sectors has no valid targets
  {
    Planet peaceful_planet{};
    peaceful_planet.star_id() = 0;
    peaceful_planet.planet_order() = 1;
    peaceful_planet.dimensions() = Coordinates{5, 5};
    planet_repo.save(peaceful_planet);

    SectorMap wasted_smap(peaceful_planet);
    for (Sector& s : wasted_smap) {
      s.set_condition(SectorType::SEC_WASTED);
      s.set_owner(2);
    }
    smap_repo.save_map(wasted_smap);

    ship.pnumorbits() = 1;
    ship.destpnum() = 1;
    ship.notified() = 0;
    int wasted_destroyed =
        berserker_bombard(ctx.em, ship, peaceful_planet, race1);
    test::expect_eq(wasted_destroyed, 0);
    test::expect_eq(ship.notified(), 1);
  }

  // Test 4: Ship with no weapons (destruct == 0) notifies player of lack of
  // weapons
  {
    ship.pnumorbits() = 0;
    ship.destpnum() = 0;
    ship.notified() = 0;
    ship.destruct() = 0;
    // Clear PDNs
    planet.ships() = 0;
    planet_repo.save(planet);

    int no_weapon_destroyed = berserker_bombard(ctx.em, ship, planet, race1);
    test::expect_eq(no_weapon_destroyed, 0);
    test::expect_eq(ship.notified(), 1);
  }

  // =========================================================================
  // Test 5: check_orbital_pdn_defense unit tests
  // =========================================================================
  {
    Planet orbit_planet{};
    orbit_planet.star_id() = 0;
    orbit_planet.planet_order() = 2;
    orbit_planet.dimensions() = Coordinates{5, 5};
    planet_repo.save(orbit_planet);

    // 1. Empty orbit has no PDN defense
    test::expect_false(
        check_orbital_pdn_defense(ctx.em, orbit_planet, player_t{1}));

    // 2. Friendly PDN does not block friendly bombardment
    ship_struct friendly_pdn{};
    friendly_pdn.number = 201;
    friendly_pdn.owner = 1;
    friendly_pdn.alive = true;
    friendly_pdn.type = ShipType::OTYPE_PLANDEF;
    friendly_pdn.whatorbits = ScopeLevel::LEVEL_PLAN;
    friendly_pdn.storbits = 0;
    friendly_pdn.pnumorbits = 2;
    auto f_handle = ctx.em.create_ship(friendly_pdn);
    orbit_planet.ships() = f_handle->number();
    planet_repo.save(orbit_planet);

    test::expect_false(
        check_orbital_pdn_defense(ctx.em, orbit_planet, player_t{1}));

    // 3. Enemy non-PDN does not block bombardment
    ship_struct enemy_cargo{};
    enemy_cargo.number = 202;
    enemy_cargo.owner = 2;
    enemy_cargo.alive = true;
    enemy_cargo.type = ShipType::STYPE_CARGO;
    enemy_cargo.whatorbits = ScopeLevel::LEVEL_PLAN;
    enemy_cargo.storbits = 0;
    enemy_cargo.pnumorbits = 2;
    enemy_cargo.nextship = orbit_planet.ships();
    auto c_handle = ctx.em.create_ship(enemy_cargo);
    orbit_planet.ships() = c_handle->number();
    planet_repo.save(orbit_planet);

    test::expect_false(
        check_orbital_pdn_defense(ctx.em, orbit_planet, player_t{1}));

    // 4. Dead enemy PDN does not block bombardment
    ship_struct dead_pdn{};
    dead_pdn.number = 203;
    dead_pdn.owner = 2;
    dead_pdn.alive = false;
    dead_pdn.type = ShipType::OTYPE_PLANDEF;
    dead_pdn.whatorbits = ScopeLevel::LEVEL_PLAN;
    dead_pdn.storbits = 0;
    dead_pdn.pnumorbits = 2;
    dead_pdn.nextship = orbit_planet.ships();
    auto d_handle = ctx.em.create_ship(dead_pdn);
    orbit_planet.ships() = d_handle->number();
    planet_repo.save(orbit_planet);

    test::expect_false(
        check_orbital_pdn_defense(ctx.em, orbit_planet, player_t{1}));

    // 5. Active hostile PDN blocks bombardment
    ship_struct active_hostile_pdn{};
    active_hostile_pdn.number = 204;
    active_hostile_pdn.owner = 2;
    active_hostile_pdn.alive = true;
    active_hostile_pdn.type = ShipType::OTYPE_PLANDEF;
    active_hostile_pdn.whatorbits = ScopeLevel::LEVEL_PLAN;
    active_hostile_pdn.storbits = 0;
    active_hostile_pdn.pnumorbits = 2;
    active_hostile_pdn.nextship = orbit_planet.ships();
    auto h_handle = ctx.em.create_ship(active_hostile_pdn);
    orbit_planet.ships() = h_handle->number();
    planet_repo.save(orbit_planet);

    test::expect_true(
        check_orbital_pdn_defense(ctx.em, orbit_planet, player_t{1}));
  }

  // =========================================================================
  // Test 6: calculate_bombardment_strength unit tests
  // =========================================================================
  {
    ship_struct b_test{};
    b_test.number = 301;
    b_test.owner = 1;
    b_test.type = ShipType::OTYPE_BERS;  // max_guns = 40
    b_test.destruct = 500;
    b_test.damage = 0;
    auto test_ship = ctx.em.create_ship(b_test);

    // Full guns (40), 0 damage, 500 destruct -> 40
    test::expect_eq(calculate_bombardment_strength(*test_ship), 40);

    // Bounded by available destruct crystals: 40 guns, 15 destruct -> 15
    test_ship->destruct() = 15;
    test::expect_eq(calculate_bombardment_strength(*test_ship), 15);

    // Hull efficiency degradation: 50% damage -> 20 effective guns, 100
    // destruct
    // -> 20
    test_ship->destruct() = 100;
    test_ship->damage() = 50;
    test::expect_eq(calculate_bombardment_strength(*test_ship), 20);

    // Zero destruct crystals -> 0
    test_ship->destruct() = 0;
    test::expect_eq(calculate_bombardment_strength(*test_ship), 0);

    // 100% hull damage -> 0
    test_ship->destruct() = 100;
    test_ship->damage() = 100;
    test::expect_eq(calculate_bombardment_strength(*test_ship), 0);

    // Non-combat ship with 0 guns (e.g. Spore Pod) -> 0
    ship_struct pod_ship_data{};
    pod_ship_data.number = 302;
    pod_ship_data.owner = 1;
    pod_ship_data.type = ShipType::STYPE_POD;
    pod_ship_data.destruct = 100;
    auto pod_test_ship = ctx.em.create_ship(pod_ship_data);
    test::expect_eq(calculate_bombardment_strength(*pod_test_ship), 0);
  }

  // =========================================================================
  // Test 7: find_bombardment_target unit tests
  // =========================================================================
  {
    Planet target_planet{};
    target_planet.star_id() = 0;
    target_planet.planet_order() = 3;
    target_planet.dimensions() = Coordinates{5, 5};
    planet_repo.save(target_planet);

    // Setup sectors on planet 3:
    // (1, 1) = owned by Race 3 (foreign, peaceful)
    // (2, 2) = owned by Race 2 (at war)
    // (3, 3) = owned by Race 1 (friendly)
    {
      SectorMap smap(target_planet);
      smap.get(Coordinates{1, 1}).set_condition(SectorType::SEC_LAND);
      smap.get(Coordinates{1, 1}).set_popn_exact(100);
      smap.get(Coordinates{1, 1}).set_owner(3);

      smap.get(Coordinates{2, 2}).set_condition(SectorType::SEC_LAND);
      smap.get(Coordinates{2, 2}).set_popn_exact(100);
      smap.get(Coordinates{2, 2}).set_owner(2);

      smap.get(Coordinates{3, 3}).set_condition(SectorType::SEC_LAND);
      smap.get(Coordinates{3, 3}).set_popn_exact(100);
      smap.get(Coordinates{3, 3}).set_owner(1);

      smap_repo.save_map(smap);
    }

    // 1. General berserker prioritizes war target (Race 2 at (2, 2)) over Race
    // 3
    ship_struct b_gen{};
    b_gen.number = 401;
    b_gen.owner = 1;
    b_gen.type = ShipType::OTYPE_BERS;
    b_gen.storbits = 0;
    b_gen.pnumorbits = 3;
    auto gen_ship = ctx.em.create_ship(b_gen);

    auto target = find_bombardment_target(ctx.em, *gen_ship, race1);
    test::expect_true(target.has_value());
    test::expect_eq(*target, (Coordinates{2, 2}));

    // 2. Programmed berserker specifically targeting Race 3 prioritizes Race 3
    // at (1, 1)
    ship_struct b_prog{};
    b_prog.number = 402;
    b_prog.owner = 1;
    b_prog.type = ShipType::OTYPE_BERS;
    b_prog.storbits = 0;
    b_prog.pnumorbits = 3;
    b_prog.special = MindData{.target = player_t{3}};
    auto prog_ship = ctx.em.create_ship(b_prog);

    auto prog_target = find_bombardment_target(ctx.em, *prog_ship, race1);
    test::expect_true(prog_target.has_value());
    test::expect_eq(*prog_target, (Coordinates{1, 1}));

    // 3. If no war target exists, falls back to foreign colony
    Race race4{};
    race4.Playernum = 4;
    race4.Guest = false;
    races.save(race4);

    ship_struct b_neutral{};
    b_neutral.number = 403;
    b_neutral.owner = 4;
    b_neutral.type = ShipType::OTYPE_BERS;
    b_neutral.storbits = 0;
    b_neutral.pnumorbits = 3;
    auto neutral_ship = ctx.em.create_ship(b_neutral);

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
    const auto& star = *ctx.em.peek_star(starnum_t{0});
    ship_struct alert_ship_data{};
    alert_ship_data.number = 501;
    alert_ship_data.owner = 1;
    alert_ship_data.governor = 0;
    alert_ship_data.name = "Nemesis";
    alert_ship_data.type = ShipType::OTYPE_BERS;
    alert_ship_data.storbits = 0;
    alert_ship_data.pnumorbits = 0;
    auto alert_ship = ctx.em.create_ship(alert_ship_data);

    BombardResult result{
        .sectors_destroyed = 3,
        .short_message = "Direct kinetic impact on surface.\n",
        .long_message = "",
    };
    result.nuked_players[player_t{2}] = true;

    dispatch_bombardment_alerts(ctx.em, *alert_ship, star, Coordinates{5, 5},
                                player_t{2}, 3, result);

    // Attacker (Player 1) received bombardment report
    test::expect_true(ctx.em.has_telegrams(player_t{1}, governor_t{0}));
    const auto attacker_telegrams =
        ctx.em.get_telegrams(player_t{1}, governor_t{0});
    test::expect_false(attacker_telegrams.empty());
    test::expect_true(attacker_telegrams[0].message.contains(
        std::format("REPORT from ship #{}", alert_ship->number())));
    test::expect_true(
        attacker_telegrams[0].message.contains("3 sectors destroyed"));

    // Victim (Player 2) received alert
    test::expect_true(ctx.em.has_telegrams(player_t{2}, governor_t{0}));
    const auto victim_telegrams =
        ctx.em.get_telegrams(player_t{2}, governor_t{0});
    test::expect_false(victim_telegrams.empty());
    test::expect_true(
        victim_telegrams[0].message.contains("ALERT from planet"));
    test::expect_true(
        victim_telegrams[0].message.contains("bombarded sector 5,5"));
  }

  std::println(std::cout, "berserker_bombard_test: All tests passed!");
  return 0;
}
