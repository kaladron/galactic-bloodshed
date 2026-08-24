// SPDX-License-Identifier: Apache-2.0

/// \file berserker_bombard_test.cc
/// \brief Unit tests for Berserker ship planetary bombardment targeting and PDN
/// interception defenses.

import dallib;
import gblib;
import test;
import std;

int main() {
  TestContext ctx;

  // Create Race 1 (Attacker)
  Race race1{};
  race1.Playernum = 1;
  race1.Guest = false;
  race1.governor[0].active = true;
  setbit(race1.atwar, player_t{2});  // At war with Race 2

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
  StarRepository star_repo(store);
  star_repo.save(ss);

  // Create Planet
  Planet planet{};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  PlanetRepository planet_repo(store);
  planet_repo.save(planet);

  // Create Sector Map with sectors for Race 2 and Race 3
  {
    SectorMap smap(planet, true);
    smap.get(3, 3).set_condition(SectorType::SEC_LAND);
    smap.get(3, 3).set_popn_exact(100);
    smap.get(3, 3).set_owner(3);  // Owned by Race 3 (not at war)

    smap.get(5, 5).set_condition(SectorType::SEC_LAND);
    smap.get(5, 5).set_popn_exact(100);
    smap.get(5, 5).set_owner(2);  // Owned by Race 2 (at war)

    SectorRepository smap_repo(store);
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
  b_ship.guns = 1;
  b_ship.primtype = GTYPE_HEAVY;
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

  std::println(std::cout, "berserker_bombard_test: All tests passed!");
  return 0;
}
