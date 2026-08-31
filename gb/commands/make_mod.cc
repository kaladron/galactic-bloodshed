// SPDX-License-Identifier: Apache-2.0

/// \file make_mod.cc
/// \brief Make and modify command implementations.

module;

import gb.entities;
import gb.services;
import std;
#undef stdout

module commands;

namespace GB::commands {
bool make_mod(const command_t& argv, GameObj& g) {
  int mode;
  if (argv[0] == "make")
    mode = 0;
  else
    mode = 1 /* modify */;
  int value;
  unsigned short size;
  char shipc;
  double cost0;

  bool ok = false;
  g.entity_manager.mutate_ship(g.shipno(), [&](Ship& dirship) {
    if (testship(dirship, g)) {
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

    /* Save  size of the factory, and set it to the
       correct values for the design.  Maarten */
    size = dirship.size();
    dirship.size() = ship_size(dirship);

    if (mode == 0) {
      if (argv.size() < 2) { /* list the current settings for the factory */
        if (!dirship.build_type()) {
          g.out << "No ship type specified.\n";
          return;
        }
        g.out << "  --- Current Production Specifications ---\n";
        const auto& btmpl = ship_template(dirship.build_type());
        g.out << std::format("{}\t\t\tArmor:    {:4}\t\tGuns:",
                             (dirship.on() ? "Online" : "Offline"),
                             dirship.armor());
        if (btmpl.primary_power && dirship.primtype() != GTYPE_NONE) {
          g.out << std::format("{:3}{:c}", dirship.primary(),
                               caliber_char(dirship.primtype()));
        }
        if (btmpl.secondary_power && dirship.sectype() != GTYPE_NONE) {
          g.out << std::format("/{:}{:c}", dirship.secondary(),
                               caliber_char(dirship.sectype()));
        }
        g.out << "\n";
        g.out << std::format("Ship:  {:<16.16s}\tCrew:     {:4}", btmpl.name,
                             dirship.max_crew());
        if (btmpl.can_mount) {
          g.out << std::format("\t\tXtal Mount: {}\n",
                               (dirship.mount() ? "yes" : "no"));
        } else {
          g.out << "\n";
        }
        g.out << std::format("Class: {}\t\tFuel:     {:4}", dirship.shipclass(),
                             dirship.max_fuel());
        if (btmpl.can_hyperjump) {
          g.out << std::format("\t\tHyperdrive: {}\n",
                               (dirship.hyper_drive().has ? "yes" : "no"));
        } else {
          g.out << "\n";
        }
        g.out << std::format("Cost:  {} r\t\tCargo:    {:4}",
                             dirship.build_cost(), dirship.max_resource());
        if (btmpl.can_mount_laser) {
          g.out << std::format("\t\tCombat Lasers: {}\n",
                               (dirship.laser() ? "yes" : "no"));
        } else {
          g.out << "\n";
        }
        g.out << std::format("Mass:  {:.1f}\t\tHanger:   {:4}",
                             dirship.base_mass(), dirship.max_hanger());
        if (btmpl.has_cew) {
          g.out << std::format("\t\tCEW: {}\n", (dirship.cew() ? "yes" : "no"));
        } else {
          g.out << "\n";
        }
        g.out << std::format("Size:  {:<6}\t\tDestruct: {:4}", dirship.size(),
                             dirship.max_destruct());
        if (btmpl.has_cew && dirship.cew()) {
          g.out << std::format("\t\t   Opt Range: {:4}\n", dirship.cew_range());
        } else {
          g.out << "\n";
        }
        g.out << std::format("Tech:  {:.1f} ({:.1f})\tSpeed:    {:4}",
                             dirship.complexity(), race.tech,
                             dirship.max_speed());
        if (btmpl.has_cew && dirship.cew()) {
          g.out << std::format("\t\t   Energy:    {:4d}\n", dirship.cew());
        } else {
          g.out << "\n";
        }

        if (race.tech < dirship.complexity()) {
          g.out << "Your engineering capability is not "
                   "advanced enough to produce this "
                   "design.\n";
        }
        dirship.size() = size;
        ok = true;
        return;
      }

      shipc = argv[1][0];

      auto i = get_build_type(shipc);

      if ((!i) || ((*i == ShipType::STYPE_POD) && (!race.pods))) {
        g.out << "Illegal ship letter.\n";
        return;
      }
      const auto& itmpl = ship_template(*i);
      if (itmpl.is_god_only && !race.God) {
        g.out << "Nice try!\n";
        return;
      }
      if (!itmpl.can_be_built_by(ship_template(ShipType::OTYPE_FACTORY))) {
        g.out << "This kind of ship does not require a factory to construct.\n";
        return;
      }

      dirship.build_type() = *i;
      dirship.armor() = itmpl.base_armor;
      dirship.guns() =
          ActiveBattery::NONE; /* this keeps track of the factory status! */
      dirship.primary() = itmpl.max_guns;
      dirship.primtype() = shipdata_primary(*i);
      dirship.secondary() = itmpl.max_guns;
      dirship.sectype() = shipdata_secondary(*i);
      dirship.max_crew() = itmpl.max_crew;
      dirship.max_resource() = itmpl.max_cargo;
      dirship.max_hanger() = itmpl.max_hangar;
      dirship.max_fuel() = itmpl.max_fuel;
      dirship.max_destruct() = itmpl.max_destruct;
      dirship.max_speed() = itmpl.base_speed;

      dirship.mount() = itmpl.can_mount * race.discoveries.crystal;
      dirship.hyper_drive().has =
          itmpl.can_hyperjump * race.discoveries.hyperdrive;
      dirship.cloak() = itmpl.can_cloak * race.discoveries.cloak;
      dirship.laser() = itmpl.can_mount_laser && race.discoveries.laser;
      dirship.cew() = 0;
      dirship.mode() = 0;

      dirship.size() = ship_size(dirship);
      dirship.complexity() = complexity(dirship);

      dirship.shipclass() = std::format("mod {}", g.shipno());

      g.out << std::format("Factory designated to produce {}s.\n", itmpl.name);
      g.out << std::format("Design complexity {:.1f} ({:.1f}).\n",
                           dirship.complexity(), race.tech);
      if (dirship.complexity() > race.tech)
        g.out << "You can't produce this design yet!\n";

    } else if (mode == 1) {
      if (!dirship.build_type()) {
        g.out << "No ship design specified. Use 'make <ship type>' first.\n";
        return;
      }

      if (argv.size() < 2) {
        g.out << "You have to specify the characteristic you wish to modify.\n";
        return;
      }

      if (argv.size() == 3)
        value = std::stoi(argv[2]);
      else
        value = 0;

      if (value < 0) {
        g.out << "That's a ridiculous setting.\n";
        return;
      }

      const auto& btmpl = ship_template(dirship.build_type());
      if (btmpl.can_modify) {
        if (argv[1] == "armor") {
          dirship.armor() = std::min<armor_t>(value, 100);
        } else if (argv[1] == "crew" && btmpl.max_crew) {
          dirship.max_crew() = std::min<population_t>(value, 10000);
        } else if (argv[1] == "cargo" && btmpl.max_cargo) {
          dirship.max_resource() = std::min<resource_t>(value, 10000);
        } else if (argv[1] == "hanger" && btmpl.max_hangar) {
          dirship.max_hanger() = std::min<hangar_t>(value, 10000);
        } else if (argv[1] == "fuel" && btmpl.max_fuel) {
          dirship.max_fuel() = std::min<unsigned short>(value, 10000);
        } else if (argv[1] == "destruct" && btmpl.max_destruct) {
          dirship.max_destruct() = std::min<unsigned short>(value, 10000);
        } else if (argv[1] == "speed" && btmpl.base_speed) {
          dirship.max_speed() = std::clamp<speed_t>(value, 1, 9);
        } else if (argv[1] == "mount" && btmpl.can_mount &&
                   race.discoveries.crystal) {
          dirship.mount() = !dirship.mount();
        } else if (argv[1] == "hyperdrive" && btmpl.can_hyperjump &&
                   race.discoveries.hyperdrive) {
          dirship.hyper_drive().has = !dirship.hyper_drive().has;
        } else if (argv[1] == "primary" && btmpl.primary_power) {
          if (argv[2] == "strength") {
            dirship.primary() = std::stoi(argv[3]);
          } else if (argv[2] == "caliber") {
            if (argv[3] == "light")
              dirship.primtype() = GTYPE_LIGHT;
            else if (argv[3] == "medium")
              dirship.primtype() = GTYPE_MEDIUM;
            else if (argv[3] == "heavy")
              dirship.primtype() = GTYPE_HEAVY;
            else {
              g.out << "No such caliber.\n";
              return;
            }
            dirship.primtype() =
                MIN(shipdata_primary(dirship.build_type()), dirship.primtype());
          } else {
            g.out << "No such gun characteristic.\n";
            return;
          }
        } else if (argv[1] == "secondary" && btmpl.secondary_power) {
          if (argv[2] == "strength") {
            dirship.secondary() = std::stoi(argv[3]);
          } else if (argv[2] == "caliber") {
            if (argv[3] == "light")
              dirship.sectype() = GTYPE_LIGHT;
            else if (argv[3] == "medium")
              dirship.sectype() = GTYPE_MEDIUM;
            else if (argv[3] == "heavy")
              dirship.sectype() = GTYPE_HEAVY;
            else {
              g.out << "No such caliber.\n";
              return;
            }
            dirship.sectype() = MIN(shipdata_secondary(dirship.build_type()),
                                    dirship.sectype());
          } else {
            g.out << "No such gun characteristic.\n";
            return;
          }
        } else if (argv[1] == "cew" && btmpl.has_cew) {
          if (!race.discoveries.cew) {
            g.out << "Your race does not understand confined energy weapons.\n";
            return;
          }
          value = std::stoi(argv[3]);
          if (argv[2] == "strength") {
            dirship.cew() = value;
          } else if (argv[2] == "range") {
            dirship.cew_range() = value;
          } else {
            g.out << "No such option for CEWs.\n";
            return;
          }
        } else if (argv[1] == "laser" && btmpl.can_mount_laser) {
          if (!race.discoveries.laser) {
            g.out << "Your race does not understand lasers yet.\n";
            return;
          }
          dirship.laser() = !dirship.laser();
        } else {
          g.out << "That characteristic either doesn't exist or can't be "
                   "modified.\n";
          return;
        }
      } else if (race.discoveries.hyperdrive) {
        if (argv[1] == "hyperdrive") {
          dirship.hyper_drive().has = !dirship.hyper_drive().has;
        } else {
          g.out << "You may only modify hyperdrive "
                   "installation on this kind of ship.\n";
          return;
        }
      } else {
        g.out << "Sorry, but you can't modify this ship right now.\n";
        return;
      }
    } else {
      g.out << "Weird error.\n";
      return;
    }
    /* compute how much it's going to cost to build the ship */

    if ((cost0 = cost(dirship)) > 65535.0) {
      g.out << "Woah!! YOU CHEATER!!!  The max cost allowed "
               "is 65535!!! I'm Telllllllling!!!\n";
      return;
    }

    dirship.build_cost() = race.God ? 0 : (int)cost0;
    g.out << std::format("The current cost of the ship is {} resources.\n",
                         dirship.build_cost());
    dirship.size() = ship_size(dirship);
    dirship.base_mass() = getmass(dirship);
    g.out << std::format(
        "The current base mass of the ship is {:.1f} - size is {}.\n",
        dirship.base_mass(), dirship.size());
    dirship.complexity() = complexity(dirship);
    g.out << std::format(
        "Ship complexity is {:.1f} (you have {:.1f} engineering "
        "technology).\n",
        dirship.complexity(), race.tech);

    /* Restore size to what it was before.  Maarten */
    dirship.size() = size;
    ok = true;
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
    .min_args = 3,
    .syntax = "modify <characteristic> <value> [<extra>]",
    .description = "Modify ship specifications at a factory installation",
    .handler = &make_mod,
};

}  // namespace GB::commands
