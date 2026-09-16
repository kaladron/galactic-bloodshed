// SPDX-License-Identifier: Apache-2.0

/// \file make_mod.cc
/// \brief Make and modify command implementations.

module;

import gb.entities;
import gb.services;
import scnlib;
import std;
import tabulate;
#undef stdout

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

std::optional<guntype_t> parse_caliber_name(std::string_view name) {
  if (name == "light") return guntype_t::LIGHT;
  if (name == "medium") return guntype_t::MEDIUM;
  if (name == "heavy") return guntype_t::HEAVY;
  return std::nullopt;
}

std::string format_factory_guns(const ShipTemplate& btmpl,
                                const Ship& dirship) {
  std::string guns = "Guns:";
  if (btmpl.has_primary() && dirship.primary_battery().has_guns()) {
    guns += std::format("{:3}{:c}", dirship.primary_battery().count,
                        caliber_char(dirship.primary_battery().caliber));
  }
  if (btmpl.has_secondary() && dirship.secondary_battery().has_guns()) {
    guns += std::format("/{:}{:c}", dirship.secondary_battery().count,
                        caliber_char(dirship.secondary_battery().caliber));
  }
  return guns;
}

bool print_factory_design_specs(GameObj& g, const Ship& dirship,
                                const Race& race) {
  if (!dirship.has_factory_design()) {
    g.out << "No ship type specified.\n";
    return false;
  }
  g.out << "  --- Current Production Specifications ---\n";
  const auto& btmpl = ship_template(dirship.build_type());

  tabulate::Table table;
  table.format().hide_border().column_separator("    ");

  table.add_row({dirship.on() ? "Online" : "Offline",
                 std::format("Armor:    {:4}", dirship.armor()),
                 format_factory_guns(btmpl, dirship)});
  table.add_row({std::format("Ship:  {}", btmpl.name),
                 std::format("Crew:     {:4}", dirship.max_crew()),
                 btmpl.can_mount ? std::format("Xtal Mount: {}",
                                               dirship.mount() ? "yes" : "no")
                                 : ""});
  table.add_row({std::format("Class: {}", dirship.shipclass()),
                 std::format("Fuel:     {:4}", dirship.max_fuel()),
                 btmpl.can_hyperjump
                     ? std::format("Hyperdrive: {}",
                                   dirship.hyper_drive().has ? "yes" : "no")
                     : ""});
  table.add_row(
      {std::format("Cost:  {} r", dirship.build_cost()),
       std::format("Cargo:    {:4}", dirship.max_resource()),
       btmpl.can_mount_laser
           ? std::format("Combat Lasers: {}", dirship.laser() ? "yes" : "no")
           : ""});
  table.add_row({std::format("Mass:  {:.1f}", dirship.base_mass()),
                 std::format("Hanger:   {:4}", dirship.max_hanger()),
                 btmpl.has_cew
                     ? std::format("CEW: {}", dirship.cew() ? "yes" : "no")
                     : ""});
  table.add_row({std::format("Size:  {}", dirship.size()),
                 std::format("Destruct: {:4}", dirship.max_destruct()),
                 (btmpl.has_cew && dirship.cew())
                     ? std::format("   Opt Range: {:4}", dirship.cew_range())
                     : ""});
  table.add_row(
      {std::format("Tech:  {:.1f} ({:.1f})", dirship.complexity(), race.tech),
       std::format("Speed:    {:4}", dirship.max_speed()),
       (btmpl.has_cew && dirship.cew())
           ? std::format("   Energy:    {:4d}", dirship.cew())
           : ""});

  g.out << table << "\n";

  if (race.tech < dirship.complexity()) {
    g.out << "Your engineering capability is not "
             "advanced enough to produce this "
             "design.\n";
  }
  return true;
}

bool designate_factory_ship_type(GameObj& g, Ship& dirship, const Race& race,
                                 char shipc) {
  auto i = get_build_type(shipc);
  if ((!i) || ((*i == ShipType::STYPE_POD) && (!race.pods))) {
    g.out << "Illegal ship letter.\n";
    return false;
  }
  const auto& itmpl = ship_template(*i);
  if (itmpl.is_god_only && !race.God) {
    g.out << "Nice try!\n";
    return false;
  }
  if (!itmpl.can_be_built_by(ship_template(ShipType::OTYPE_FACTORY))) {
    g.out << "This kind of ship does not require a factory to construct.\n";
    return false;
  }

  dirship.set_factory_blueprint(*i, &race);
  dirship.shipclass() = std::format("mod {}", g.shipno());

  g.out << std::format("Factory designated to produce {}s.\n", itmpl.name);
  g.out << std::format("Design complexity {:.1f} ({:.1f}).\n",
                       dirship.complexity(), race.tech);
  if (dirship.complexity() > race.tech) {
    g.out << "You can't produce this design yet!\n";
  }
  return true;
}

bool modify_battery(GameObj& g, Ship& dirship, const command_t& argv,
                    bool is_primary) {
  if (argv.size() < 4) {
    g.out << "No such gun characteristic.\n";
    return false;
  }
  if (argv[2] == "strength") {
    auto strength = parse_non_negative_int(argv[3]);
    if (!strength) {
      g.out << "That's a ridiculous setting.\n";
      return false;
    }
    if (is_primary) {
      dirship.set_primary_battery(*strength, dirship.primary_battery().caliber);
    } else {
      dirship.set_secondary_battery(*strength,
                                    dirship.secondary_battery().caliber);
    }
    return true;
  }
  if (argv[2] == "caliber") {
    auto new_caliber = parse_caliber_name(argv[3]);
    if (!new_caliber) {
      g.out << "No such caliber.\n";
      return false;
    }
    if (is_primary) {
      dirship.set_primary_battery(
          dirship.primary_battery().count,
          std::min(shipdata_primary(dirship.build_type()), *new_caliber));
    } else {
      dirship.set_secondary_battery(
          dirship.secondary_battery().count,
          std::min(shipdata_secondary(dirship.build_type()), *new_caliber));
    }
    return true;
  }
  g.out << "No such gun characteristic.\n";
  return false;
}

bool modify_cew(GameObj& g, Ship& dirship, const Race& race,
                const command_t& argv) {
  if (!race.discoveries.cew) {
    g.out << "Your race does not understand confined energy weapons.\n";
    return false;
  }
  if (argv.size() < 4) {
    g.out << "No such option for CEWs.\n";
    return false;
  }
  auto value = parse_non_negative_int(argv[3]);
  if (!value) {
    g.out << "That's a ridiculous setting.\n";
    return false;
  }
  if (argv[2] == "strength") {
    dirship.cew() = *value;
    return true;
  }
  if (argv[2] == "range") {
    dirship.cew_range() = *value;
    return true;
  }
  g.out << "No such option for CEWs.\n";
  return false;
}

bool modify_simple_attribute(GameObj& g, Ship& dirship, const Race& race,
                             const ShipTemplate& btmpl, const command_t& argv) {
  std::string_view attr = argv[1];
  if (attr == "mount" && btmpl.can_mount && race.discoveries.crystal) {
    dirship.mount() = !dirship.mount();
    return true;
  }
  if (attr == "hyperdrive" && btmpl.can_hyperjump &&
      race.discoveries.hyperdrive) {
    dirship.hyper_drive().has = !dirship.hyper_drive().has;
    return true;
  }
  if (attr == "laser" && btmpl.can_mount_laser) {
    if (!race.discoveries.laser) {
      g.out << "Your race does not understand lasers yet.\n";
      return false;
    }
    dirship.laser() = !dirship.laser();
    return true;
  }

  int value = 0;
  if (argv.size() >= 3) {
    auto parsed = parse_non_negative_int(argv[2]);
    if (!parsed) {
      g.out << "That's a ridiculous setting.\n";
      return false;
    }
    value = *parsed;
  }

  if (attr == "armor") {
    dirship.armor() = std::min<armor_t>(value, 100);
    return true;
  }
  if (attr == "crew" && btmpl.max_crew) {
    dirship.max_crew() = std::min<population_t>(value, 10000);
    return true;
  }
  if ((attr == "cargo" || attr == "resource") && btmpl.max_resource) {
    dirship.max_resource() = std::min<resource_t>(value, 10000);
    return true;
  }
  if (attr == "hanger" && btmpl.max_hangar) {
    dirship.max_hanger() = std::min<hangar_t>(value, 10000);
    return true;
  }
  if (attr == "fuel" && btmpl.max_fuel) {
    dirship.max_fuel() = std::min<fuel_t>(value, 10000);
    return true;
  }
  if (attr == "destruct" && btmpl.max_destruct) {
    dirship.max_destruct() = std::min<resource_t>(value, 10000);
    return true;
  }
  if (attr == "speed" && btmpl.base_speed) {
    dirship.max_speed() = std::clamp<speed_t>(value, 1, 9);
    return true;
  }

  g.out << "That characteristic either doesn't exist or can't be modified.\n";
  return false;
}

bool apply_factory_modification(GameObj& g, Ship& dirship, const Race& race,
                                const command_t& argv) {
  if (!dirship.has_factory_design()) {
    g.out << "No ship design specified. Use 'make <ship type>' first.\n";
    return false;
  }
  if (argv.size() < 2) {
    g.out << "You have to specify the characteristic you wish to modify.\n";
    return false;
  }

  const auto& btmpl = ship_template(dirship.build_type());
  if (!btmpl.can_modify) {
    if (race.discoveries.hyperdrive) {
      if (argv[1] == "hyperdrive") {
        dirship.hyper_drive().has = !dirship.hyper_drive().has;
        return true;
      }
      g.out << "You may only modify hyperdrive installation on this kind of "
               "ship.\n";
      return false;
    }
    g.out << "Sorry, but you can't modify this ship right now.\n";
    return false;
  }

  if (argv[1] == "primary" && btmpl.has_primary()) {
    return modify_battery(g, dirship, argv, true);
  }
  if (argv[1] == "secondary" && btmpl.has_secondary()) {
    return modify_battery(g, dirship, argv, false);
  }
  if (argv[1] == "cew" && btmpl.has_cew) {
    return modify_cew(g, dirship, race, argv);
  }
  return modify_simple_attribute(g, dirship, race, btmpl, argv);
}

bool finalize_factory_design_stats(GameObj& g, Ship& dirship, const Race& race,
                                   ship_size_t original_size) {
  double cost0 = cost(dirship);
  if (cost0 > 65535.0) {
    g.out << "Woah!! YOU CHEATER!!!  The max cost allowed "
             "is 65535!!! I'm Telllllllling!!!\n";
    dirship.size() = original_size;
    return false;
  }

  dirship.build_cost() = race.God ? 0 : static_cast<resource_t>(cost0);
  g.out << std::format("The current cost of the ship is {} resources.\n",
                       dirship.build_cost());
  dirship.size() = dirship.calculate_size();
  g.out << std::format(
      "The current base mass of the ship is {:.1f} - size is {}.\n",
      dirship.base_mass(), dirship.size());
  dirship.complexity() = complexity(dirship);
  g.out << std::format("Ship complexity is {:.1f} (you have {:.1f} engineering "
                       "technology).\n",
                       dirship.complexity(), race.tech);

  /* Restore size to what it was before.  Maarten */
  dirship.size() = original_size;
  return true;
}

}  // namespace

bool make_mod(const command_t& argv, GameObj& g) {
  const bool is_make = (argv[0] == "make");
  bool ok = false;

  g.entity_manager.mutate_ship(g.shipno(), [&](Ship& dirship) {
    if (!dirship.check_commandable(g)) {
      return;
    }
    if (dirship.type() != ShipType::OTYPE_FACTORY) {
      g.out << "That is not a factory.\n";
      return;
    }
    if (dirship.on() && argv.size() > 1) {
      g.out << "This factory is already online.\n";
      return;
    }
    const auto& race = *g.race;

    /* Save size of the factory, and set it to the
       correct values for the design.  Maarten */
    const ship_size_t original_size = dirship.size();
    dirship.size() = dirship.calculate_size();

    if (is_make) {
      if (argv.size() < 2) {
        ok = print_factory_design_specs(g, dirship, race);
        dirship.size() = original_size;
        return;
      }
      if (!designate_factory_ship_type(g, dirship, race, argv[1][0])) {
        dirship.size() = original_size;
        return;
      }
    } else {
      if (!apply_factory_modification(g, dirship, race, argv)) {
        dirship.size() = original_size;
        return;
      }
    }

    ok = finalize_factory_design_stats(g, dirship, race, original_size);
  });
  return ok;
}

const CommandDescriptor make_cmd{
    .name = "make",
    .roles = {.no_guests = true},
    .scopes = AllowedScopes::ship_only(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "make [<shiptype>]",
    .description = "Configure ship type to build at a factory installation",
    .handler = &make_mod,
};

const CommandDescriptor modify_cmd{
    .name = "modify",
    .roles = {.no_guests = true},
    .scopes = AllowedScopes::ship_only(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "modify <characteristic> [<value>] [<extra>]",
    .description = "Modify ship specifications at a factory installation",
    .handler = &make_mod,
};

}  // namespace GB::commands
