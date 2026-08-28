// SPDX-License-Identifier: Apache-2.0

/// \file sector_test.cc
/// \brief Unit tests for Sector domain methods, devastation, supernova
/// radiation, and invariant handling.

import gb.entities;
import test;
import std;

namespace {

void test_sector_devastate() {
  Sector sector(sector_struct{
      .coords = {2, 3},
      .eff = 85,
      .fert = 90,
      .mobilization = 30,
      .crystals = 5,
      .resource = 250,
      .popn = 1500,
      .troops = 200,
      .owner = player_t{1},
      .race = player_t{1},
      .type = SectorType::SEC_LAND,
      .condition = SectorType::SEC_LAND,
  });

  test::expect_true(sector.is_owned());
  test::expect_false(sector.is_empty());
  test::expect_false(sector.is_wasted());

  sector.devastate();

  test::expect_eq(sector.get_condition(), SectorType::SEC_WASTED);
  test::expect_true(sector.is_wasted());
  test::expect_eq(sector.get_owner(), player_t{0});
  test::expect_eq(sector.get_popn(), 0);
  test::expect_eq(sector.get_troops(), 0);
  test::expect_eq(sector.get_mobilization(), 0);
  test::expect_eq(sector.get_eff(), 0);
  test::expect_true(sector.is_empty());
  test::expect_false(sector.is_owned());

  // Coordinates and geology remain intact
  test::expect_eq(sector.coords().x, 2);
  test::expect_eq(sector.coords().y, 3);
  test::expect_eq(sector.get_type(), SectorType::SEC_LAND);
}

void test_sector_apply_supernova() {
  seed_rand(42);

  // Test active supernova stage (< 14): resource + 1, fert - 20%, ~50% popn
  // killed
  Sector sector(sector_struct{
      .fert = 100,
      .resource = 50,
      .popn = 1000,
      .owner = player_t{1},
  });

  sector.apply_supernova(5);
  test::expect_eq(sector.get_resource(), 51);
  test::expect_eq(sector.get_fert(), 80);
  test::expect_true(sector.get_popn() > 400 && sector.get_popn() < 600);
  test::expect_eq(sector.get_owner(), player_t{1});

  // Test terminal supernova stage 14: resource + 1, fert - 20%, full wipe
  Sector dying_sector(sector_struct{
      .coords = {1, 1},
      .fert = 80,
      .resource = 10,
      .popn = 500,
      .troops = 50,
      .owner = player_t{1},
  });

  dying_sector.apply_supernova(14);
  test::expect_eq(dying_sector.get_resource(), 11);
  test::expect_eq(dying_sector.get_fert(), 64);
  test::expect_eq(dying_sector.get_popn(), 0);
  test::expect_eq(dying_sector.get_troops(), 0);
  test::expect_eq(dying_sector.get_owner(), player_t{0});
  test::expect_true(dying_sector.is_empty());
}

void test_sector_plating_and_ownership() {
  // Land sector plating
  Sector land(sector_struct{
      .eff = 40,
      .condition = SectorType::SEC_LAND,
  });
  land.plate();
  test::expect_eq(land.get_eff(), 100);
  test::expect_eq(land.get_condition(), SectorType::SEC_PLATED);
  test::expect_true(land.is_plated());

  // Gas sector plating (efficiency goes to 100, but condition remains SEC_GAS)
  Sector gas(sector_struct{
      .eff = 40,
      .condition = SectorType::SEC_GAS,
  });
  gas.plate();
  test::expect_eq(gas.get_eff(), 100);
  test::expect_eq(gas.get_condition(), SectorType::SEC_GAS);
  test::expect_false(gas.is_plated());

  // clear_owner_if_empty
  Sector populated(sector_struct{
      .popn = 100,
      .owner = player_t{1},
  });
  populated.clear_owner_if_empty();
  test::expect_eq(populated.get_owner(), player_t{1});

  Sector empty(sector_struct{
      .owner = player_t{1},
  });
  empty.clear_owner_if_empty();
  test::expect_eq(empty.get_owner(), player_t{0});
}

void test_sector_invariants() {
  Sector s1(sector_struct{
      .coords = {0, 0},
      .eff = 50,
      .fert = 50,
      .resource = 100,
      .popn = 500,
      .owner = player_t{1},
  });
  Sector s2(sector_struct{
      .coords = {0, 1},
      .eff = 50,
      .fert = 50,
      .resource = 100,
      .popn = 200,
      .owner = player_t{1},
  });

  // Population transfer
  s1.transfer_popn_to(s2, 100);
  test::expect_eq(s1.get_popn(), 400);
  test::expect_eq(s2.get_popn(), 300);

  // Transfer clamping when exceeding source
  s1.transfer_popn_to(s2, 1000);
  test::expect_eq(s1.get_popn(), 0);
  test::expect_eq(s2.get_popn(), 700);

  // Efficiency bounds (0-100)
  s1.set_efficiency_bounded(150);
  test::expect_eq(s1.get_eff(), 100);
  s1.set_efficiency_bounded(-20);
  test::expect_eq(s1.get_eff(), 0);

  s1.improve_efficiency(50);
  test::expect_eq(s1.get_eff(), 50);
  s1.improve_efficiency(80);  // Saturates at 100
  test::expect_eq(s1.get_eff(), 100);

  s1.degrade_efficiency(30);
  test::expect_eq(s1.get_eff(), 70);
  s1.degrade_efficiency(100);  // Bottoms at 0
  test::expect_eq(s1.get_eff(), 0);

  // Resources
  s1.add_resource(50);
  test::expect_eq(s1.get_resource(), 150);
  s1.subtract_resource(200);  // Clamps to 0
  test::expect_eq(s1.get_resource(), 0);
}

void test_sector_terraform() {
  Sector sector(sector_struct{
      .eff = 80,
      .mobilization = 50,
      .popn = 1000,
      .troops = 100,
      .owner = player_t{1},
      .race = player_t{1},
      .type = SectorType::SEC_SEA,
      .condition = SectorType::SEC_ICE,
  });

  sector.terraform(SectorType::SEC_LAND);
  test::expect_eq(sector.get_condition(), SectorType::SEC_LAND);
  test::expect_eq(sector.get_type(), SectorType::SEC_SEA);  // Geology preserved
  test::expect_eq(sector.get_eff(), 0);
  test::expect_eq(sector.get_mobilization(), 0);
  test::expect_eq(sector.get_popn(), 0);
  test::expect_eq(sector.get_troops(), 0);
  test::expect_eq(sector.get_owner(), player_t{0});
  test::expect_true(sector.is_empty());
}

void test_sector_colonize_and_claim() {
  Sector sector(sector_struct{
      .condition = SectorType::SEC_FOREST,
  });

  sector.colonize(player_t{2}, 2, player_t{2});
  test::expect_eq(sector.get_owner(), player_t{2});
  test::expect_eq(sector.get_race(), player_t{2});
  test::expect_eq(sector.get_popn(), 2);
  test::expect_eq(sector.get_troops(), 0);
  test::expect_true(sector.is_owned());

  sector.claim(player_t{3});
  test::expect_eq(sector.get_owner(), player_t{3});
  test::expect_eq(sector.get_race(), player_t{3});
}

void test_sector_troops_and_mobilization() {
  Sector sector(sector_struct{
      .mobilization = 20,
      .troops = 50,
  });

  // Mobilization adjustment & clamping
  sector.adjust_mobilization(15);
  test::expect_eq(sector.get_mobilization(), 35);
  sector.adjust_mobilization(-40);  // Bottoms at 0
  test::expect_eq(sector.get_mobilization(), 0);
  sector.adjust_mobilization(150);  // Caps at 100
  test::expect_eq(sector.get_mobilization(), 100);

  sector.set_mobilization_bounded(45);
  test::expect_eq(sector.get_mobilization(), 45);
  sector.set_mobilization_bounded(200);  // Clamps to 100
  test::expect_eq(sector.get_mobilization(), 100);
  sector.set_mobilization_bounded(-10);  // Clamps to 0
  test::expect_eq(sector.get_mobilization(), 0);

  // Troops operations
  sector.add_troops(25);
  test::expect_eq(sector.get_troops(), 75);
  sector.subtract_troops(10);
  test::expect_eq(sector.get_troops(), 65);
  sector.subtract_troops(100);  // Clamps to 0
  test::expect_eq(sector.get_troops(), 0);

  sector.set_troops_exact(100);
  test::expect_eq(sector.get_troops(), 100);
  sector.clear_troops();
  test::expect_eq(sector.get_troops(), 0);
}

void test_sector_transfer_autoclaim() {
  Sector source(sector_struct{
      .popn = 100,
      .owner = player_t{1},
      .race = player_t{1},
  });
  Sector target(sector_struct{
      .popn = 0,
      .owner = player_t{0},
  });

  source.transfer_popn_to(target, 40);
  test::expect_eq(source.get_popn(), 60);
  test::expect_eq(target.get_popn(), 40);
  test::expect_eq(target.get_owner(), player_t{1});
  test::expect_eq(target.get_race(), player_t{1});
}

void test_sectormap_range_views() {
  Planet planet(planet_struct{
      .dimensions = Coordinates{2, 2},
  });
  SectorMap smap(planet);

  // Setup sectors:
  // (0,0): owner 1, popn 100 (populated)
  // (1,0): owner 1, popn 0 (unpopulated)
  // (0,1): owner 2, popn 50 (populated)
  // (1,1): unowned, popn 0
  smap.get(Coordinates{0, 0}).set_owner(1);
  smap.get(Coordinates{0, 0}).set_popn_exact(100);

  smap.get(Coordinates{1, 0}).set_owner(1);
  smap.get(Coordinates{1, 0}).set_popn_exact(0);

  smap.get(Coordinates{0, 1}).set_owner(2);
  smap.get(Coordinates{0, 1}).set_popn_exact(50);

  smap.get(Coordinates{1, 1}).set_owner(0);
  smap.get(Coordinates{1, 1}).set_popn_exact(0);

  // 1. Direct SectorMap iteration
  std::size_t total_count = 0;
  for (const Sector& s : smap) {
    test::expect_false(s.is_wasted());
    ++total_count;
  }
  test::expect_eq(total_count, 4UL);

  // 2. smap.owned()
  std::size_t owned_count = 0;
  for (const Sector& s : smap.owned()) {
    test::expect_true(s.is_owned());
    ++owned_count;
  }
  test::expect_eq(owned_count, 3UL);

  // 2. smap.owned_by()
  std::size_t p1_count = 0;
  for (const Sector& s : smap.owned_by(player_t{1})) {
    test::expect_eq(s.get_owner(), player_t{1});
    ++p1_count;
  }
  test::expect_eq(p1_count, 2UL);

  std::size_t p2_count = 0;
  for (const Sector& s : smap.owned_by(player_t{2})) {
    test::expect_eq(s.get_owner(), player_t{2});
    ++p2_count;
  }
  test::expect_eq(p2_count, 1UL);

  // 3. smap.populated()
  std::size_t pop_count = 0;
  for (const Sector& s : smap.populated()) {
    test::expect_true(s.is_populated());
    ++pop_count;
  }
  test::expect_eq(pop_count, 2UL);

  // 4. smap.populated_by()
  std::size_t p1_pop_count = 0;
  for (const Sector& s : smap.populated_by(player_t{1})) {
    test::expect_eq(s.get_owner(), player_t{1});
    test::expect_true(s.is_populated());
    ++p1_pop_count;
  }
  test::expect_eq(p1_pop_count, 1UL);

  // 5. Mutable iteration through view
  for (Sector& s : smap.owned_by(player_t{1})) {
    s.add_resource(50);
  }
  test::expect_eq(smap.get(Coordinates{0, 0}).get_resource(), 50);
  test::expect_eq(smap.get(Coordinates{1, 0}).get_resource(), 50);
  test::expect_eq(smap.get(Coordinates{0, 1}).get_resource(), 0);

  // 6. Const SectorMap range views
  const SectorMap& const_smap = smap;
  std::size_t const_owned_count = 0;
  for (const Sector& s : const_smap.owned_by(player_t{1})) {
    test::expect_eq(s.get_resource(), 50);
    ++const_owned_count;
  }
  test::expect_eq(const_owned_count, 2UL);
}

}  // namespace

int main() {
  test_sector_devastate();
  test_sector_apply_supernova();
  test_sector_plating_and_ownership();
  test_sector_invariants();
  test_sector_terraform();
  test_sector_colonize_and_claim();
  test_sector_troops_and_mobilization();
  test_sector_transfer_autoclaim();
  test_sectormap_range_views();
  return 0;
}
