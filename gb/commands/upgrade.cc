// SPDX-License-Identifier: Apache-2.0

/// \file upgrade.cc
/// \brief Upgrade ship characteristics and systems.

module;

import gb.entities;
import gb.services;
import scnlib;
import std;

module commands;

namespace GB::commands {

namespace {

std::optional<int> parse_non_negative_int(std::string_view arg) {
  auto res = scn::scan<int>(arg, "{}");
  if (!res || res->value() < 0) {
    return std::nullopt;
  }
  return res->value();
}

bool upgrade_battery(GameObj& g, Ship& ship, const Ship& dirship,
                     const command_t& argv, bool is_primary) {
  if (argv.size() < 4) {
    g.out << "No such gun characteristic.\n";
    return false;
  }
  const auto current_battery =
      is_primary ? dirship.primary_battery() : dirship.secondary_battery();

  if (argv[2] == "strength") {
    if (current_battery.caliber == guntype_t::NONE) {
      g.out << "No caliber defined.\n";
      return false;
    }
    auto parsed = parse_non_negative_int(argv[3]);
    if (!parsed) {
      g.out << "That's a ridiculous setting.\n";
      return false;
    }
    const auto count =
        std::max(static_cast<gun_count_t>(*parsed), current_battery.count);
    if (is_primary) {
      ship.set_primary_battery(count, current_battery.caliber);
    } else {
      ship.set_secondary_battery(count, current_battery.caliber);
    }
    return true;
  }

  if (argv[2] == "caliber") {
    auto requested = parse_caliber_name(argv[3]);
    if (!requested) {
      g.out << "No such caliber.\n";
      return false;
    }
    const auto max_caliber = is_primary
                                 ? shipdata_primary(dirship.build_type())
                                 : shipdata_secondary(dirship.build_type());
    const auto new_caliber =
        std::min(max_caliber, std::max(*requested, current_battery.caliber));
    if (is_primary) {
      ship.set_primary_battery(current_battery.count, new_caliber);
    } else {
      ship.set_secondary_battery(current_battery.count, new_caliber);
    }
    return true;
  }

  g.out << "No such gun characteristic.\n";
  return false;
}

bool upgrade_cew(GameObj& g, Ship& ship, const Ship& dirship, const Race& race,
                 const command_t& argv) {
  if (!race.discoveries.cew) {
    g.out << "Your race cannot build confined energy weapons.\n";
    return false;
  }
  if (argv.size() < 4) {
    g.out << "No such option for CEWs.\n";
    return false;
  }
  auto parsed = parse_non_negative_int(argv[3]);
  if (!parsed) {
    g.out << "That's a ridiculous setting.\n";
    return false;
  }
  if (argv[2] == "strength") {
    ship.cew() = std::max(dirship.cew(), static_cast<weapon_power_t>(*parsed));
    return true;
  }
  if (argv[2] == "range") {
    ship.cew_range() =
        std::max(dirship.cew_range(), static_cast<weapon_range_t>(*parsed));
    return true;
  }
  g.out << "No such option for CEWs.\n";
  return false;
}

bool upgrade_boolean_system(GameObj& g, Ship& ship, const Ship& dirship,
                            const Race& race, const ShipTemplate& btmpl,
                            std::string_view attr) {
  if (attr == "mount" && btmpl.can_mount && !dirship.mount()) {
    if (!race.discoveries.crystal) {
      g.out << "Your race does not now how to utilize crystal power yet.\n";
      return false;
    }
    ship.mount() = true;
    return true;
  }
  if (attr == "hyperdrive" && btmpl.can_hyperjump &&
      !dirship.hyper_drive().has && race.discoveries.hyperdrive) {
    ship.hyper_drive().has = 1;
    return true;
  }
  if (attr == "laser" && btmpl.can_mount_laser) {
    if (!race.discoveries.laser) {
      g.out << "Your race cannot build lasers.\n";
      return false;
    }
    ship.laser() = true;
    return true;
  }
  return false;
}

bool upgrade_numeric_attribute(GameObj& g, Ship& ship, const Ship& dirship,
                               const ShipTemplate& btmpl,
                               const command_t& argv) {
  int value = 0;
  if (argv.size() == 3) {
    auto parsed = parse_non_negative_int(argv[2]);
    if (!parsed) {
      g.out << "That's a ridiculous setting.\n";
      return false;
    }
    value = *parsed;
  }

  const std::string_view attr = argv[1];
  if (attr == "armor") {
    ship.armor() = std::max(dirship.armor(), std::min<armor_t>(value, 100));
    return true;
  }
  if (attr == "crew" && btmpl.max_crew) {
    ship.max_crew() =
        std::max(dirship.max_crew(), std::min<population_t>(value, 10000));
    return true;
  }
  if ((attr == "cargo" || attr == "resource") && btmpl.max_resource) {
    ship.max_resource() =
        std::max(dirship.max_resource(), std::min<resource_t>(value, 10000));
    return true;
  }
  if (attr == "hanger" && btmpl.max_hangar) {
    ship.max_hanger() =
        std::max(dirship.max_hanger(), std::min<hangar_t>(value, 10000));
    return true;
  }
  if (attr == "fuel" && btmpl.max_fuel) {
    ship.max_fuel() =
        std::max(dirship.max_fuel(), std::min<fuel_t>(value, 10000));
    return true;
  }
  if (attr == "destruct" && btmpl.max_destruct) {
    ship.max_destruct() =
        std::max(dirship.max_destruct(), std::min<resource_t>(value, 10000));
    return true;
  }
  if (attr == "speed" && btmpl.base_speed) {
    ship.max_speed() =
        std::max(dirship.max_speed(), std::clamp<speed_t>(value, 1, 9));
    return true;
  }

  g.out << "That characteristic either doesn't exist or can't be modified.\n";
  return false;
}

bool apply_upgrade_characteristic(GameObj& g, Ship& ship, const Ship& dirship,
                                  const Race& race, const ShipTemplate& btmpl,
                                  const command_t& argv) {
  const std::string_view attr = argv[1];
  if (attr == "primary" && btmpl.has_primary()) {
    return upgrade_battery(g, ship, dirship, argv, true);
  }
  if (attr == "secondary" && btmpl.has_secondary()) {
    return upgrade_battery(g, ship, dirship, argv, false);
  }
  if (attr == "cew" && btmpl.has_cew) {
    return upgrade_cew(g, ship, dirship, race, argv);
  }
  if ((attr == "mount" && btmpl.can_mount && !dirship.mount()) ||
      (attr == "hyperdrive" && btmpl.can_hyperjump &&
       !dirship.hyper_drive().has && race.discoveries.hyperdrive) ||
      (attr == "laser" && btmpl.can_mount_laser)) {
    return upgrade_boolean_system(g, ship, dirship, race, btmpl, attr);
  }
  return upgrade_numeric_attribute(g, ship, dirship, btmpl, argv);
}

bool check_carrier_hangar_capacity(GameObj& g, const Ship& dirship,
                                   const Ship& candidate) {
  if (dirship.whatorbits() != ScopeLevel::LEVEL_SHIP) {
    return true;
  }
  bool fits = true;
  g.entity_manager.with_ship(dirship.destshipno(), [&](const Ship& carrier) {
    const long available_space = static_cast<long>(carrier.max_hanger()) -
                                 (static_cast<long>(carrier.hanger()) -
                                  static_cast<long>(dirship.size()));
    const long needed_size = static_cast<long>(candidate.calculate_size());
    if (available_space < needed_size) {
      g.out << std::format("Not enough free hanger space on {}{}.\n",
                           carrier.type_letter(), dirship.destshipno());
      g.out << std::format("{} more needed.\n", needed_size - available_space);
      fits = false;
    }
  });
  return fits;
}

void commit_ship_upgrade(GameObj& g, Ship& dirship, const Ship& candidate,
                         const Race& race, resource_t oldcost,
                         resource_t newcost, resource_t netcost) {
  g.out << std::format("Old value {}r   New value {}r\n", oldcost, newcost);
  g.out << std::format("Characteristic modified at a cost of {} resources.\n",
                       netcost);

  const ship_size_t old_size = dirship.size();
  const double old_mass = dirship.mass();
  const double old_base_mass = dirship.base_mass();

  dirship.armor() = candidate.armor();
  dirship.max_crew() = candidate.max_crew();
  dirship.max_resource() = candidate.max_resource();
  dirship.max_hanger() = candidate.max_hanger();
  dirship.max_fuel() = candidate.max_fuel();
  dirship.mount() = candidate.mount();
  dirship.max_destruct() = candidate.max_destruct();
  dirship.max_speed() = candidate.max_speed();
  dirship.hyper_drive() = candidate.hyper_drive();
  dirship.set_primary_battery(candidate.primary_battery());
  dirship.set_secondary_battery(candidate.secondary_battery());
  dirship.cew() = candidate.cew();
  dirship.cew_range() = candidate.cew_range();
  dirship.laser() = candidate.laser();

  dirship.size() = dirship.calculate_size();
  dirship.set_mass(dirship.mass() + (dirship.base_mass() - old_base_mass));
  dirship.consume_resource(netcost);
  dirship.build_cost() = race.God ? 0 : static_cast<resource_t>(cost(dirship));
  dirship.complexity() = complexity(dirship);

  if (dirship.whatorbits() == ScopeLevel::LEVEL_SHIP) {
    g.entity_manager.mutate_ship(dirship.destshipno(), [&](Ship& carrier) {
      carrier.unload_docked_craft(old_size, old_mass);
      carrier.load_docked_craft(dirship);
    });
  }
}

}  // namespace

/* upgrade ship characteristics */
bool upgrade(const command_t& argv, GameObj& g) {
  if (g.level() != ScopeLevel::LEVEL_SHIP) {
    g.out << "You have to change scope to the ship you wish to upgrade.\n";
    return false;
  }
  bool ok = false;
  g.entity_manager.mutate_ship(g.shipno(), [&](Ship& dirship) {
    if (!g.check_commandable(dirship)) {
      return;
    }
    if (dirship.damage()) {
      g.out << "You cannot upgrade damaged ships.\n";
      return;
    }
    if (dirship.type() == ShipType::OTYPE_FACTORY) {
      g.out << "You can't upgrade factories.\n";
      return;
    }

    const auto& btmpl = ship_template(dirship.build_type());
    if (!btmpl.can_modify) {
      g.out << "This ship cannot be upgraded.\n";
      return;
    }

    const auto& race = *g.race;
    Ship candidate(dirship.get_struct());
    if (!apply_upgrade_characteristic(g, candidate, dirship, race, btmpl,
                                      argv)) {
      return;
    }

    /* check to see whether this ship can actually be built by this player */
    const double complex = complexity(candidate);
    if (complex > race.tech) {
      g.out << std::format(
          "This upgrade requires an engineering technology of {:.1f}.\n",
          complex);
      return;
    }

    /* check to see if the new ship will actually fit inside the hanger if it is
       on another ship. Maarten */
    if (!check_carrier_hangar_capacity(g, dirship, candidate)) {
      return;
    }

    /* compute new ship costs and see if the player can afford it */
    const resource_t newcost =
        race.God ? 0 : static_cast<resource_t>(cost(candidate));
    const resource_t oldcost = race.God ? 0 : dirship.build_cost();
    resource_t netcost =
        race.God ? 0 : 2 * (newcost - oldcost); /* upgrade is expensive */
    if (newcost < oldcost) {
      g.out << "You cannot downgrade ships!\n";
      return;
    }
    if (!race.God && netcost == 0) {
      netcost = 1;
    }

    if (netcost > dirship.resource()) {
      g.out << std::format("Old value {}r   New value {}r\n", oldcost, newcost);
      g.out << std::format(
          "You need {} resources on board to make this modification.\n",
          netcost);
      return;
    }
    if (netcost || race.God) {
      commit_ship_upgrade(g, dirship, candidate, race, oldcost, newcost,
                          netcost);
      ok = true;
    } else {
      g.out << "You can not make this modification.\n";
    }
  });
  return ok;
}

const CommandDescriptor upgrade_cmd{
    .name = "upgrade",
    .roles = {.no_guests = true},
    .scopes = AllowedScopes::ship_only(),
    .ap = APCost::fixed_star(1),
    .min_args = 2,
    .syntax = "upgrade <characteristic> [<value>]",
    .description = "Upgrade ship characteristics and systems",
    .handler = &upgrade,
};

}  // namespace GB::commands