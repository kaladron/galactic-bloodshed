// SPDX-License-Identifier: Apache-2.0

/// \file ship_template_test.cc
/// \brief Unit tests for ShipTemplate specifications and Ship capability
/// accessors.

import gb.entities;
import std;
import test;

int main() {
  std::println(std::cout, "Testing ShipTemplate invariants...");
  test::expect_eq(ship_templates.size(), static_cast<std::size_t>(NUMSTYPES));
  test::expect_eq(NUMSTYPES, 47);

  std::set<char> seen_letters;
  for (int t = 0; t < NUMSTYPES; ++t) {
    const auto ship_type = static_cast<ShipType>(t);
    const auto& tmpl = ship_template(ship_type);

    // Metadata invariants
    test::expect_eq(static_cast<int>(tmpl.type), t);
    test::expect_false(tmpl.name.empty(), "Ship template name cannot be empty");
    test::expect_true(is_valid_ship_letter(tmpl.letter),
                      "Ship letter must be valid");
    test::expect_false(seen_letters.contains(tmpl.letter),
                       "Ship letters must be distinct");
    seen_letters.insert(tmpl.letter);

    // Consistency of domain predicates with raw values
    test::expect_eq(tmpl.can_build_on_planet(),
                    (static_cast<int>(tmpl.build_time) & 1) != 0);
    test::expect_eq(tmpl.can_construct_ships(),
                    static_cast<int>(tmpl.construction_cost) != 0);
    test::expect_true(tmpl.max_resource >= 0,
                      "Template max_resource must be non-negative");
  }
  test::expect_eq(seen_letters.size(), static_cast<std::size_t>(NUMSTYPES));
  std::println(std::cout,
               "  ✓ All 47 ship templates have valid metadata and unique "
               "letters");

  std::println(std::cout, "Testing get_all_ship_letters helper...");
  {
    auto letters = get_all_ship_letters();
    test::expect_eq(letters.size(), static_cast<std::size_t>(NUMSTYPES));
    for (char c : letters) {
      test::expect_true(is_valid_ship_letter(c));
    }
    test::expect_false(is_valid_ship_letter('?'));
    test::expect_false(is_valid_ship_letter('$'));
  }

  std::println(std::cout, "Testing specific ship template properties...");
  {
    const auto& pod_tmpl = ship_template(ShipType::STYPE_POD);
    test::expect_eq(pod_tmpl.name, "Spore pod");
    test::expect_eq(pod_tmpl.letter, 'p');
    test::expect_true(pod_tmpl.can_repair);
    test::expect_false(pod_tmpl.requires_maintenance);
    test::expect_true(pod_tmpl.can_land);
    test::expect_false(pod_tmpl.can_hyperjump);
    test::expect_false(pod_tmpl.can_mount);
    test::expect_false(pod_tmpl.can_mount_laser);
    test::expect_false(pod_tmpl.can_modify);
    test::expect_false(pod_tmpl.is_starport);

    const auto& battle_tmpl = ship_template(ShipType::STYPE_BATTLE);
    test::expect_eq(battle_tmpl.name, "Battleship");
    test::expect_eq(battle_tmpl.letter, 'B');
    test::expect_eq(battle_tmpl.base_armor, 7);
    test::expect_eq(battle_tmpl.max_crew, 30);
    test::expect_eq(battle_tmpl.max_resource, 235);
    test::expect_eq(battle_tmpl.max_fuel, 200);
    test::expect_true(battle_tmpl.can_modify);
    test::expect_true(battle_tmpl.can_mount);
    test::expect_true(battle_tmpl.can_mount_laser);
    test::expect_true(battle_tmpl.can_hyperjump);
    test::expect_true(battle_tmpl.can_land);

    const auto& factory_tmpl = ship_template(ShipType::OTYPE_FACTORY);
    test::expect_eq(factory_tmpl.name, "Factory");
    test::expect_eq(factory_tmpl.letter, 'F');
    test::expect_eq(factory_tmpl.max_resource, 50);
    test::expect_true(factory_tmpl.can_construct_ships());
    test::expect_true(factory_tmpl.has_switch);

    const auto& hab_tmpl = ship_template(ShipType::STYPE_HABITAT);
    test::expect_eq(hab_tmpl.name, "Habitat");
    test::expect_eq(hab_tmpl.letter, 'H');
    test::expect_eq(hab_tmpl.max_resource, 5000);
    test::expect_true(hab_tmpl.is_starport);
    test::expect_true(hab_tmpl.can_repair);
    test::expect_true(hab_tmpl.requires_maintenance);
    test::expect_false(hab_tmpl.can_land);

    const auto& cargo_tmpl = ship_template(ShipType::STYPE_CARGO);
    test::expect_eq(cargo_tmpl.name, "Cargo Ship");
    test::expect_eq(cargo_tmpl.letter, 'c');
    test::expect_eq(cargo_tmpl.max_crew, 100);
    test::expect_eq(cargo_tmpl.max_resource, 1000);
    test::expect_eq(cargo_tmpl.max_fuel, 1000);
    test::expect_true(cargo_tmpl.can_modify);
  }

  std::println(std::cout, "Testing Ship entity helper accessors...");
  {
    Ship carrier;
    carrier.type() = ShipType::STYPE_CARRIER;
    test::expect_eq(carrier.type_name(), "Carrier");
    test::expect_eq(carrier.type_letter(), 'X');
    test::expect_true(carrier.can_repair());
    test::expect_true(carrier.requires_maintenance());
    test::expect_false(carrier.can_land());
    test::expect_true(carrier.can_hyperjump());
    test::expect_true(carrier.can_mount());
    test::expect_true(carrier.can_mount_laser());
    test::expect_true(carrier.can_modify());
    test::expect_false(carrier.is_starport());

    Ship pod;
    pod.type() = ShipType::STYPE_POD;
    test::expect_eq(pod.type_name(), "Spore pod");
    test::expect_eq(pod.type_letter(), 'p');
    test::expect_true(pod.can_repair());
    test::expect_false(pod.requires_maintenance());
    test::expect_true(pod.can_land());
    test::expect_false(pod.can_hyperjump());
    test::expect_false(pod.can_mount());
    test::expect_false(pod.can_mount_laser());
    test::expect_false(pod.can_modify());

    Ship hab;
    hab.type() = ShipType::STYPE_HABITAT;
    test::expect_eq(hab.type_name(), "Habitat");
    test::expect_eq(hab.type_letter(), 'H');
    test::expect_true(hab.can_repair());
    test::expect_true(hab.requires_maintenance());
    test::expect_false(hab.can_land());
    test::expect_false(hab.can_hyperjump());
    test::expect_false(hab.can_mount());
    test::expect_false(hab.can_mount_laser());
    test::expect_true(hab.is_starport());
    std::println(std::cout, "  ✓ Ship capability accessors work correctly");
  }

  std::println(std::cout,
               "Testing ShipTemplate battery caliber specifications...");
  {
    for (int t = 0; t < NUMSTYPES; ++t) {
      const auto ship_type = static_cast<ShipType>(t);
      const auto& tmpl = ship_template(ship_type);
      test::expect_eq(tmpl.has_primary(),
                      tmpl.max_primary_caliber != guntype_t::NONE);
      test::expect_eq(tmpl.has_secondary(),
                      tmpl.max_secondary_caliber != guntype_t::NONE);
      test::expect_eq(shipdata_primary(ship_type), tmpl.max_primary_caliber);
      test::expect_eq(shipdata_secondary(ship_type),
                      tmpl.max_secondary_caliber);

      if (tmpl.has_secondary()) {
        test::expect_true(
            tmpl.has_primary(),
            "Ship with secondary battery should also support primary");
      }
    }

    const auto& battle = ship_template(ShipType::STYPE_BATTLE);
    test::expect_eq(battle.max_primary_caliber, guntype_t::HEAVY);
    test::expect_eq(battle.max_secondary_caliber, guntype_t::MEDIUM);
    test::expect_true(battle.has_primary());
    test::expect_true(battle.has_secondary());

    const auto& shuttle = ship_template(ShipType::STYPE_SHUTTLE);
    test::expect_eq(shuttle.max_primary_caliber, guntype_t::LIGHT);
    test::expect_eq(shuttle.max_secondary_caliber, guntype_t::NONE);
    test::expect_true(shuttle.has_primary());
    test::expect_false(shuttle.has_secondary());

    const auto& pod = ship_template(ShipType::STYPE_POD);
    test::expect_eq(pod.max_primary_caliber, guntype_t::NONE);
    test::expect_eq(pod.max_secondary_caliber, guntype_t::NONE);
    test::expect_false(pod.has_primary());
    test::expect_false(pod.has_secondary());

    std::println(std::cout,
                 "  ✓ ShipTemplate battery caliber specifications valid");
  }

  return 0;
}
