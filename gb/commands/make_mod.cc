// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module commands;

namespace GB::commands {
void make_mod(const command_t& argv, GameObj& g) {
  int mode;
  if (argv[0] == "make")
    mode = 0;
  else
    mode = 1 /* modify */;
  int value;
  unsigned short size;
  char shipc;
  double cost0;

  if (g.level() != ScopeLevel::LEVEL_SHIP) {
    g.out << "You have to change scope to an installation.\n";
    return;
  }

  auto dirship_handle = g.entity_manager.get_ship(g.shipno());
  if (!dirship_handle.get()) {
    g.out << "Illegal dir value.\n";
    return;
  }
  Ship& dirship = *dirship_handle;
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
      g.out << std::format("{}\t\t\tArmor:    {:4d}\t\tGuns:",
                           (dirship.on() ? "Online" : "Offline"),
                           dirship.armor());
      if (Shipdata[dirship.build_type()][ABIL_PRIMARY] &&
          dirship.primtype() != GTYPE_NONE) {
        g.out << std::format("{:3}{:c}", dirship.primary(),
                             (dirship.primtype() == GTYPE_LIGHT    ? 'L'
                              : dirship.primtype() == GTYPE_MEDIUM ? 'M'
                              : dirship.primtype() == GTYPE_HEAVY  ? 'H'
                                                                   : 'N'));
      }
      if (Shipdata[dirship.build_type()][ABIL_SECONDARY] &&
          dirship.sectype() != GTYPE_NONE) {
        g.out << std::format("/{:}{:c}", dirship.secondary(),
                             (dirship.sectype() == GTYPE_LIGHT    ? 'L'
                              : dirship.sectype() == GTYPE_MEDIUM ? 'M'
                              : dirship.sectype() == GTYPE_HEAVY  ? 'H'
                                                                  : 'N'));
      }
      g.out << "\n";
      g.out << std::format("Ship:  {:<16.16s}\tCrew:     {:4d}",
                           Shipnames[dirship.build_type()], dirship.max_crew());
      if (Shipdata[dirship.build_type()][ABIL_MOUNT]) {
        g.out << std::format("\t\tXtal Mount: {}\n",
                             (dirship.mount() ? "yes" : "no"));
      } else {
        g.out << "\n";
      }
      g.out << std::format("Class: {}\t\tFuel:     {:4d}", dirship.shipclass(),
                           dirship.max_fuel());
      if (Shipdata[dirship.build_type()][ABIL_JUMP]) {
        g.out << std::format("\t\tHyperdrive: {}\n",
                             (dirship.hyper_drive().has ? "yes" : "no"));
      } else {
        g.out << "\n";
      }
      g.out << std::format("Cost:  {} r\t\tCargo:    {:4}",
                           dirship.build_cost(), dirship.max_resource());
      if (Shipdata[dirship.build_type()][ABIL_LASER]) {
        g.out << std::format("\t\tCombat Lasers: {}\n",
                             (dirship.laser() ? "yes" : "no"));
      } else {
        g.out << "\n";
      }
      g.out << std::format("Mass:  {:.1f}\t\tHanger:   {:4}",
                           dirship.base_mass(), dirship.max_hanger());
      if (Shipdata[dirship.build_type()][ABIL_CEW]) {
        g.out << std::format("\t\tCEW: {}\n", (dirship.cew() ? "yes" : "no"));
      } else {
        g.out << "\n";
      }
      g.out << std::format("Size:  {:<6d}\t\tDestruct: {:4d}", dirship.size(),
                           dirship.max_destruct());
      if (Shipdata[dirship.build_type()][ABIL_CEW] && dirship.cew()) {
        g.out << std::format("\t\t   Opt Range: {:4d}\n", dirship.cew_range());
      } else {
        g.out << "\n";
      }
      g.out << std::format("Tech:  {:.1f} ({:.1f})\tSpeed:    {:4d}",
                           dirship.complexity(), race.tech,
                           dirship.max_speed());
      if (Shipdata[dirship.build_type()][ABIL_CEW] && dirship.cew()) {
        g.out << std::format("\t\t   Energy:    {:4d}\n", dirship.cew());
      } else {
        g.out << "\n";
      }

      if (race.tech < dirship.complexity()) {
        g.out << "Your engineering capability is not "
                 "advanced enough to produce this "
                 "design.\n";
      }
      return;
    }

    shipc = argv[1][0];

    auto i = get_build_type(shipc);

    if ((!i) || ((*i == ShipType::STYPE_POD) && (!race.pods))) {
      g.out << "Illegal ship letter.\n";
      return;
    }
    if (Shipdata[*i][ABIL_GOD] && !race.God) {
      g.out << "Nice try!\n";
      return;
    }
    if (!(Shipdata[*i][ABIL_BUILD] &
          Shipdata[ShipType::OTYPE_FACTORY][ABIL_CONSTRUCT])) {
      g.out << "This kind of ship does not require a factory to construct.\n";
      return;
    }

    dirship.build_type() = *i;
    dirship.armor() = Shipdata[*i][ABIL_ARMOR];
    dirship.guns() = GTYPE_NONE; /* this keeps track of the factory status! */
    dirship.primary() = Shipdata[*i][ABIL_GUNS];
    dirship.primtype() = shipdata_primary(*i);
    dirship.primary() = Shipdata[*i][ABIL_GUNS];
    dirship.sectype() = shipdata_secondary(*i);
    dirship.max_crew() = Shipdata[*i][ABIL_MAXCREW];
    dirship.max_resource() = Shipdata[*i][ABIL_CARGO];
    dirship.max_hanger() = Shipdata[*i][ABIL_HANGER];
    dirship.max_fuel() = Shipdata[*i][ABIL_FUELCAP];
    dirship.max_destruct() = Shipdata[*i][ABIL_DESTCAP];
    dirship.max_speed() = Shipdata[*i][ABIL_SPEED];

    dirship.mount() = Shipdata[*i][ABIL_MOUNT] * Crystal(race);
    dirship.hyper_drive().has = Shipdata[*i][ABIL_JUMP] * Hyper_drive(race);
    dirship.cloak() = Shipdata[*i][ABIL_CLOAK] * Cloak(race);
    dirship.laser() = Shipdata[*i][ABIL_LASER] * Laser(race);
    dirship.cew() = 0;
    dirship.mode() = 0;

    dirship.size() = ship_size(dirship);
    dirship.complexity() = complexity(dirship);

    dirship.shipclass() = std::format("mod {}", g.shipno());

    g.out << std::format("Factory designated to produce {}s.\n", Shipnames[*i]);
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

    if (Shipdata[dirship.build_type()][ABIL_MOD]) {
      if (argv[1] == "armor") {
        dirship.armor() = MIN(value, 100);
      } else if (argv[1] == "crew" &&
                 Shipdata[dirship.build_type()][ABIL_MAXCREW]) {
        dirship.max_crew() = MIN(value, 10000);
      } else if (argv[1] == "cargo" &&
                 Shipdata[dirship.build_type()][ABIL_CARGO]) {
        dirship.max_resource() = MIN(value, 10000);
      } else if (argv[1] == "hanger" &&
                 Shipdata[dirship.build_type()][ABIL_HANGER]) {
        dirship.max_hanger() = MIN(value, 10000);
      } else if (argv[1] == "fuel" &&
                 Shipdata[dirship.build_type()][ABIL_FUELCAP]) {
        dirship.max_fuel() = MIN(value, 10000);
      } else if (argv[1] == "destruct" &&
                 Shipdata[dirship.build_type()][ABIL_DESTCAP]) {
        dirship.max_destruct() = MIN(value, 10000);
      } else if (argv[1] == "speed" &&
                 Shipdata[dirship.build_type()][ABIL_SPEED]) {
        dirship.max_speed() = MAX(1, MIN(value, 9));
      } else if (argv[1] == "mount" &&
                 Shipdata[dirship.build_type()][ABIL_MOUNT] && Crystal(race)) {
        dirship.mount() = !dirship.mount();
      } else if (argv[1] == "hyperdrive" &&
                 Shipdata[dirship.build_type()][ABIL_JUMP] &&
                 Hyper_drive(race)) {
        dirship.hyper_drive().has = !dirship.hyper_drive().has;
      } else if (argv[1] == "primary" &&
                 Shipdata[dirship.build_type()][ABIL_PRIMARY]) {
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
      } else if (argv[1] == "secondary" &&
                 Shipdata[dirship.build_type()][ABIL_SECONDARY]) {
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
          dirship.sectype() =
              MIN(shipdata_secondary(dirship.build_type()), dirship.sectype());
        } else {
          g.out << "No such gun characteristic.\n";
          return;
        }
      } else if (argv[1] == "cew" && Shipdata[dirship.build_type()][ABIL_CEW]) {
        if (!Cew(race)) {
          g.out << "Your race does not understand confined energy weapons.\n";
          return;
        }
        if (!Shipdata[dirship.build_type()][ABIL_CEW]) {
          g.out << "This kind of ship cannot mount confined energy weapons.\n";
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
      } else if (argv[1] == "laser" &&
                 Shipdata[dirship.build_type()][ABIL_LASER]) {
        if (!Laser(race)) {
          g.out << "Your race does not understand lasers yet.\n";
          return;
        }
        if (Shipdata[dirship.build_type()][ABIL_LASER])
          dirship.laser() = !dirship.laser();
        else {
          g.out << "That ship cannot be fitted with combat lasers.\n";
          return;
        }
      } else {
        g.out << "That characteristic either doesn't exist or can't be "
                 "modified.\n";
        return;
      }
    } else if (Hyper_drive(race)) {
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
  g.out << std::format("Ship complexity is {:.1f} (you have {:.1f} engineering "
                       "technology).\n",
                       dirship.complexity(), race.tech);

  /* Restore size to what it was before.  Maarten */
  dirship.size() = size;
}
}  // namespace GB::commands
