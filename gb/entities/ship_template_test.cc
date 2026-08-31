// SPDX-License-Identifier: Apache-2.0

/// \file ship_template_test.cc
/// \brief Unit tests for ShipTemplate specifications and Ship capability
/// accessors.

import gb.entities;
import std;
import test;

int main() {
  std::println(std::cout, "Testing ShipTemplate parity against Shipdata...");
  for (int t = 0; t < NUMSTYPES; ++t) {
    const auto ship_type = static_cast<ShipType>(t);
    const auto& tmpl = ship_template(ship_type);

    // Metadata parity
    test::expect_eq(static_cast<int>(tmpl.type), t);
    test::expect_eq(tmpl.name, std::string_view(Shipnames[t]));
    test::expect_eq(tmpl.letter, Shipltrs[t]);

    // Numerical baseline capacity parity
    test::expect_eq(static_cast<long>(tmpl.base_tech), Shipdata[t][ABIL_TECH]);
    test::expect_eq(tmpl.max_cargo,
                    static_cast<resource_t>(Shipdata[t][ABIL_CARGO]));
    test::expect_eq(tmpl.max_hangar,
                    static_cast<hangar_t>(Shipdata[t][ABIL_HANGER]));
    test::expect_eq(tmpl.max_destruct,
                    static_cast<resource_t>(Shipdata[t][ABIL_DESTCAP]));
    test::expect_eq(tmpl.max_guns,
                    static_cast<gun_count_t>(Shipdata[t][ABIL_GUNS]));
    test::expect_eq(tmpl.primary_power,
                    static_cast<weapon_power_t>(Shipdata[t][ABIL_PRIMARY]));
    test::expect_eq(tmpl.secondary_power,
                    static_cast<weapon_power_t>(Shipdata[t][ABIL_SECONDARY]));
    test::expect_eq(static_cast<long>(tmpl.max_fuel),
                    Shipdata[t][ABIL_FUELCAP]);
    test::expect_eq(tmpl.max_crew,
                    static_cast<population_t>(Shipdata[t][ABIL_MAXCREW]));
    test::expect_eq(tmpl.base_armor,
                    static_cast<armor_t>(Shipdata[t][ABIL_ARMOR]));
    test::expect_eq(tmpl.build_cost,
                    static_cast<money_t>(Shipdata[t][ABIL_COST]));
    test::expect_eq(tmpl.base_speed,
                    static_cast<speed_t>(Shipdata[t][ABIL_SPEED]));
    test::expect_eq(tmpl.base_damage,
                    static_cast<damage_t>(Shipdata[t][ABIL_DAMAGE]));
    test::expect_eq(static_cast<long>(tmpl.build_time),
                    Shipdata[t][ABIL_BUILD]);
    test::expect_eq(static_cast<long>(tmpl.construction_cost),
                    Shipdata[t][ABIL_CONSTRUCT]);
    test::expect_eq(tmpl.can_modify, Shipdata[t][ABIL_MOD] != 0);
    test::expect_eq(tmpl.max_lasers,
                    static_cast<gun_count_t>(Shipdata[t][ABIL_LASER]));

    // Boolean capabilities parity
    test::expect_eq(tmpl.can_mount, Shipdata[t][ABIL_MOUNT] != 0);
    test::expect_eq(tmpl.can_hyperjump, Shipdata[t][ABIL_JUMP] != 0);
    test::expect_eq(tmpl.can_land, Shipdata[t][ABIL_CANLAND] != 0);
    test::expect_eq(tmpl.has_switch, Shipdata[t][ABIL_HASSWITCH] != 0);
    test::expect_eq(tmpl.has_cew, Shipdata[t][ABIL_CEW] != 0);
    test::expect_eq(tmpl.can_cloak, Shipdata[t][ABIL_CLOAK] != 0);
    test::expect_eq(tmpl.is_god_only, Shipdata[t][ABIL_GOD] != 0);
    test::expect_eq(tmpl.is_programmed, Shipdata[t][ABIL_PROGRAMMED] != 0);
    test::expect_eq(tmpl.is_starport, Shipdata[t][ABIL_PORT] != 0);
    test::expect_eq(tmpl.can_repair, Shipdata[t][ABIL_REPAIR] != 0);
    test::expect_eq(tmpl.requires_maintenance, Shipdata[t][ABIL_MAINTAIN] != 0);
  }
  std::println(std::cout, "  ✓ All 47 ship templates match Shipdata exactly");

  std::println(std::cout, "Testing Ship entity helper accessors...");
  {
    Ship carrier;
    carrier.type() = ShipType::STYPE_CARRIER;
    test::expect_eq(carrier.get_template().name, "Carrier");
    test::expect_true(carrier.can_repair());
    test::expect_true(carrier.requires_maintenance());
    test::expect_false(carrier.can_land());
    test::expect_true(carrier.can_hyperjump());
    test::expect_true(carrier.can_mount());
    test::expect_true(carrier.can_modify());

    Ship pod;
    pod.type() = ShipType::STYPE_POD;
    test::expect_eq(pod.get_template().name, "Spore pod");
    test::expect_true(pod.can_repair());
    test::expect_false(pod.requires_maintenance());
    test::expect_true(pod.can_land());
    test::expect_false(pod.can_hyperjump());
    test::expect_false(pod.can_mount());
    test::expect_false(pod.can_modify());

    Ship hab;
    hab.type() = ShipType::STYPE_HABITAT;
    test::expect_eq(hab.get_template().name, "Habitat");
    test::expect_true(hab.can_repair());
    test::expect_true(hab.requires_maintenance());
    test::expect_false(hab.can_land());
    test::expect_false(hab.can_hyperjump());
    test::expect_false(hab.can_mount());
    test::expect_true(hab.get_template().is_starport);
    std::println(std::cout, "  ✓ Ship capability accessors work correctly");
  }

  return 0;
}
