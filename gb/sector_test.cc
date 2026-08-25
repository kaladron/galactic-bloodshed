// SPDX-License-Identifier: Apache-2.0

/// \file sector_test.cc
/// \brief Unit tests for Sector domain methods, devastation, supernova
/// radiation, and invariant handling.

import gblib;
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

}  // namespace

int main() {
  test_sector_devastate();
  test_sector_apply_supernova();
  test_sector_plating_and_ownership();
  test_sector_invariants();
  return 0;
}
