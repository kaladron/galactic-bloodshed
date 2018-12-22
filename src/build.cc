// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* build -- build a ship */

#include "build.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctgmath>
#include <optional>

#include "GB_server.h"
#include "buffers.h"
#include "config.h"
#include "files.h"
#include "files_shl.h"
#include "fire.h"
#include "getplace.h"
#include "land.h"
#include "races.h"
#include "shipdata.h"
#include "ships.h"
#include "shlmisc.h"
#include "shootblast.h"
#include "tele.h"
#include "tweakables.h"
#include "vars.h"

static void autoload_at_planet(int, Ship *, Planet *, sector &, int *,
                               double *);
static void autoload_at_ship(Ship *, Ship *, int *, double *);
static std::optional<ScopeLevel> build_at_ship(GameObj &, Ship *, int *, int *);
static int can_build_at_planet(GameObj &, startype *, const Planet &);
static int can_build_this(int, racetype *, char *);
static int can_build_on_ship(int, racetype *, Ship *, char *);
static int can_build_on_sector(int, racetype *, const Planet &, const sector &,
                               int, int, char *);
static void create_ship_by_planet(int, int, racetype *, Ship *, Planet *, int,
                                  int, int, int);
static void create_ship_by_ship(int, int, racetype *, int, Planet *, Ship *,
                                Ship *);
static int get_build_type(const char *);
static int getcount(const command_t &, const size_t);
static void Getfactship(Ship *, Ship *);
static void Getship(Ship *, int, racetype *);
static void initialize_new_ship(GameObj &, racetype *, Ship *, double, int);
static void system_cost(double *, double *, int, int);

/* upgrade ship characteristics */
void upgrade(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  // TODO(jeffbailey): Fix unused int APcount = 1;
  int value, oldcost, newcost, netcost;
  Ship ship, *dirship, *s2;
  double complex;
  racetype *Race;

  if (g.level != ScopeLevel::LEVEL_SHIP) {
    notify(Playernum, Governor,
           "You have to change scope to the ship you wish to upgrade.\n");
    return;
  }
  if (!getship(&dirship, g.shipno)) {
    sprintf(buf, "Illegal dir value.\n");
    notify(Playernum, Governor, buf);
    return;
  }
  if (testship(Playernum, Governor, dirship)) {
    free(dirship);
    return;
  }
  if (dirship->damage) {
    g.out << "You cannot upgrade damaged ships.\n";
    free(dirship);
    return;
  }
  if (dirship->type == OTYPE_FACTORY) {
    g.out << "You can't upgrade factories.\n";
    free(dirship);
    return;
  }

  Race = races[Playernum - 1];
  bcopy(dirship, &ship, sizeof(Ship));

  if (argv.size() == 3)
    value = std::stoi(argv[2]);
  else
    value = 0;

  if (value < 0) {
    g.out << "That's a ridiculous setting.\n";
    free(dirship);
    return;
  }

  if (Shipdata[dirship->build_type][ABIL_MOD]) {
    if (argv[1] == "armor") {
      ship.armor = MAX(dirship->armor, MIN(value, 100));
    } else if (argv[1] == "crew" &&
               Shipdata[dirship->build_type][ABIL_MAXCREW]) {
      ship.max_crew = MAX(dirship->max_crew, MIN(value, 10000));
    } else if (argv[1] == "cargo" &&
               Shipdata[dirship->build_type][ABIL_CARGO]) {
      ship.max_resource = MAX(dirship->max_resource, MIN(value, 10000));
    } else if (argv[1] == "hanger" &&
               Shipdata[dirship->build_type][ABIL_HANGER]) {
      ship.max_hanger = MAX(dirship->max_hanger, MIN(value, 10000));
    } else if (argv[1] == "fuel" &&
               Shipdata[dirship->build_type][ABIL_FUELCAP]) {
      ship.max_fuel = MAX(dirship->max_fuel, MIN(value, 10000));
    } else if (argv[1] == "mount" &&
               Shipdata[dirship->build_type][ABIL_MOUNT] && !dirship->mount) {
      if (!Crystal(Race)) {
        notify(Playernum, Governor,
               "Your race does not now how to utilize crystal power yet.\n");
        free(dirship);
        return;
      }
      ship.mount = !ship.mount;
    } else if (argv[1] == "destruct" &&
               Shipdata[dirship->build_type][ABIL_DESTCAP]) {
      ship.max_destruct = MAX(dirship->max_destruct, MIN(value, 10000));
    } else if (argv[1] == "speed" &&
               Shipdata[dirship->build_type][ABIL_SPEED]) {
      ship.max_speed = MAX(dirship->max_speed, MAX(1, MIN(value, 9)));
    } else if (argv[1] == "hyperdrive" &&
               Shipdata[dirship->build_type][ABIL_JUMP] &&
               !dirship->hyper_drive.has && Hyper_drive(Race)) {
      ship.hyper_drive.has = 1;
    } else if (argv[1] == "primary" &&
               Shipdata[dirship->build_type][ABIL_PRIMARY]) {
      if (argv[2] == "strength") {
        if (ship.primtype == GTYPE_NONE) {
          g.out << "No caliber defined.\n";
          free(dirship);
          return;
        }
        ship.primary = std::stoi(argv[3]);
        ship.primary = MAX(ship.primary, dirship->primary);
      } else if (argv[2] == "caliber") {
        if (argv[3] == "light")
          ship.primtype = MAX(GTYPE_LIGHT, dirship->primtype);
        else if (argv[3] == "medium")
          ship.primtype = MAX(GTYPE_MEDIUM, dirship->primtype);
        else if (argv[3] == "heavy")
          ship.primtype = MAX(GTYPE_HEAVY, dirship->primtype);
        else {
          g.out << "No such caliber.\n";
          free(dirship);
          return;
        }
        ship.primtype =
            MIN(Shipdata[dirship->build_type][ABIL_PRIMARY], ship.primtype);
      } else {
        g.out << "No such gun characteristic.\n";
        free(dirship);
        return;
      }
    } else if (argv[1] == "secondary" &&
               Shipdata[dirship->build_type][ABIL_SECONDARY]) {
      if (argv[2] == "strength") {
        if (ship.sectype == GTYPE_NONE) {
          g.out << "No caliber defined.\n";
          free(dirship);
          return;
        }
        ship.secondary = std::stoi(argv[3]);
        ship.secondary = MAX(ship.secondary, dirship->secondary);
      } else if (argv[2] == "caliber") {
        if (argv[3] == "light")
          ship.sectype = MAX(GTYPE_LIGHT, dirship->sectype);
        else if (argv[3] == "medium")
          ship.sectype = MAX(GTYPE_MEDIUM, dirship->sectype);
        else if (argv[3] == "heavy")
          ship.sectype = MAX(GTYPE_HEAVY, dirship->sectype);
        else {
          g.out << "No such caliber.\n";
          free(dirship);
          return;
        }
        ship.sectype =
            MIN(Shipdata[dirship->build_type][ABIL_SECONDARY], ship.sectype);
      } else {
        g.out << "No such gun characteristic.\n";
        free(dirship);
        return;
      }
    } else if (argv[1] == "cew" && Shipdata[dirship->build_type][ABIL_CEW]) {
      if (!Cew(Race)) {
        sprintf(buf, "Your race cannot build confined energy weapons.\n");
        notify(Playernum, Governor, buf);
        free(dirship);
        return;
      }
      if (!Shipdata[dirship->build_type][ABIL_CEW]) {
        notify(Playernum, Governor,
               "This kind of ship cannot mount confined energy weapons.\n");
        free(dirship);
        return;
      }
      value = std::stoi(argv[3]);
      if (argv[2] == "strength") {
        ship.cew = value;
      } else if (argv[2] == "range") {
        ship.cew_range = value;
      } else {
        g.out << "No such option for CEWs.\n";
        free(dirship);
        return;
      }
    } else if (argv[1] == "laser" &&
               Shipdata[dirship->build_type][ABIL_LASER]) {
      if (!Laser(Race)) {
        sprintf(buf, "Your race cannot build lasers.\n");
        notify(Playernum, Governor, buf);
        free(dirship);
        return;
      }
      if (Shipdata[dirship->build_type][ABIL_LASER])
        ship.laser = 1;
      else {
        notify(Playernum, Governor,
               "That ship cannot be fitted with combat lasers.\n");
        free(dirship);
        return;
      }
    } else {
      notify(
          Playernum, Governor,
          "That characteristic either doesn't exist or can't be modified.\n");
      free(dirship);
      return;
    }
  } else {
    g.out << "This ship cannot be upgraded.\n";
    free(dirship);
    return;
  }

  /* check to see whether this ship can actually be built by this player */
  if ((complex = complexity(&ship)) > Race->tech) {
    sprintf(buf, "This upgrade requires an engineering technology of %.1f.\n",
            complex);
    notify(Playernum, Governor, buf);
    free(dirship);
    return;
  }

  /* check to see if the new ship will actually fit inside the hanger if it is
     on another ship. Maarten */
  if (dirship->whatorbits == ScopeLevel::LEVEL_SHIP) {
    (void)getship(&s2, dirship->destshipno);
    if (s2->max_hanger - (s2->hanger - dirship->size) < ship_size(&ship)) {
      sprintf(buf, "Not enough free hanger space on %c%ld.\n",
              Shipltrs[s2->type], dirship->destshipno);
      notify(Playernum, Governor, buf);
      sprintf(
          buf, "%d more needed.\n",
          ship_size(&ship) - (s2->max_hanger - (s2->hanger - dirship->size)));
      notify(Playernum, Governor, buf);
      free(s2);
      free(dirship);
      return;
    }
  }

  /* compute new ship costs and see if the player can afford it */
  newcost = Race->God ? 0 : (int)cost(&ship);
  oldcost = Race->God ? 0 : dirship->build_cost;
  netcost = Race->God ? 0 : 2 * (newcost - oldcost); /* upgrade is expensive */
  if (newcost < oldcost) {
    g.out << "You cannot downgrade ships!\n";
    free(dirship);
    return;
  }
  if (!Race->God) netcost += !netcost;

  if (netcost > dirship->resource) {
    sprintf(buf, "Old value %dr   New value %dr\n", oldcost, newcost);
    notify(Playernum, Governor, buf);
    sprintf(buf, "You need %d resources on board to make this modification.\n",
            netcost);
    notify(Playernum, Governor, buf);
  } else if (netcost || Race->God) {
    sprintf(buf, "Old value %dr   New value %dr\n", oldcost, newcost);
    notify(Playernum, Governor, buf);
    sprintf(buf, "Characteristic modified at a cost of %d resources.\n",
            netcost);
    notify(Playernum, Governor, buf);
    bcopy(&ship, dirship, sizeof(Ship));
    dirship->resource -= netcost;
    if (dirship->whatorbits == ScopeLevel::LEVEL_SHIP) {
      s2->hanger -= dirship->size;
      dirship->size = ship_size(dirship);
      s2->hanger += dirship->size;
      putship(s2);
    }
    dirship->size = ship_size(dirship);
    dirship->base_mass = getmass(dirship);
    dirship->build_cost = Race->God ? 0 : cost(dirship);
    dirship->complexity = complexity(dirship);

    putship(dirship);
  } else
    g.out << "You can not make this modification.\n";
  free(dirship);
}

void make_mod(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int mode;
  if (argv[0] == "make")
    mode = 0;
  else
    mode = 1 /* modify */;
  int i, value;
  unsigned short size;
  char shipc;
  Ship *dirship;
  racetype *Race;
  double cost0;

  if (g.level != ScopeLevel::LEVEL_SHIP) {
    notify(Playernum, Governor,
           "You have to change scope to an installation.\n");
    return;
  }

  if (!getship(&dirship, g.shipno)) {
    sprintf(buf, "Illegal dir value.\n");
    notify(Playernum, Governor, buf);
    return;
  }
  if (testship(Playernum, Governor, dirship)) {
    free(dirship);
    return;
  }
  if (dirship->type != OTYPE_FACTORY) {
    g.out << "That is not a factory.\n";
    free(dirship);
    return;
  }
  if (dirship->on && argv.size() > 1) {
    g.out << "This factory is already online.\n";
    free(dirship);
    return;
  }
  Race = races[Playernum - 1];

  /* Save  size of the factory, and set it to the
     correct values for the design.  Maarten */
  size = dirship->size;
  dirship->size = ship_size(dirship);

  if (mode == 0) {
    if (argv.size() < 2) { /* list the current settings for the factory */
      if (!dirship->build_type) {
        g.out << "No ship type specified.\n";
        free(dirship);
        return;
      }
      notify(Playernum, Governor,
             "  --- Current Production Specifications ---\n");
      sprintf(buf, "%s\t\t\tArmor:    %4d\t\tGuns:",
              (dirship->on ? "Online" : "Offline"), dirship->armor);
      notify(Playernum, Governor, buf);
      if (Shipdata[dirship->build_type][ABIL_PRIMARY] &&
          dirship->primtype != GTYPE_NONE) {
        sprintf(buf, "%3lu%c", dirship->primary,
                (dirship->primtype == GTYPE_LIGHT
                     ? 'L'
                     : dirship->primtype == GTYPE_MEDIUM
                           ? 'M'
                           : dirship->primtype == GTYPE_HEAVY ? 'H' : 'N'));
        notify(Playernum, Governor, buf);
      }
      if (Shipdata[dirship->build_type][ABIL_SECONDARY] &&
          dirship->sectype != GTYPE_NONE) {
        sprintf(buf, "/%lu%c", dirship->secondary,
                (dirship->sectype == GTYPE_LIGHT
                     ? 'L'
                     : dirship->sectype == GTYPE_MEDIUM
                           ? 'M'
                           : dirship->sectype == GTYPE_HEAVY ? 'H' : 'N'));
        notify(Playernum, Governor, buf);
      }
      g.out << "\n";
      sprintf(buf, "Ship:  %-16.16s\tCrew:     %4d",
              Shipnames[dirship->build_type], dirship->max_crew);
      notify(Playernum, Governor, buf);
      if (Shipdata[dirship->build_type][ABIL_MOUNT]) {
        sprintf(buf, "\t\tXtal Mount: %s\n", (dirship->mount ? "yes" : "no"));
        notify(Playernum, Governor, buf);
      } else
        g.out << "\n";
      sprintf(buf, "Class: %s\t\tFuel:     %4d", dirship->shipclass,
              dirship->max_fuel);
      notify(Playernum, Governor, buf);
      if (Shipdata[dirship->build_type][ABIL_JUMP]) {
        sprintf(buf, "\t\tHyperdrive: %s\n",
                (dirship->hyper_drive.has ? "yes" : "no"));
        notify(Playernum, Governor, buf);
      } else
        g.out << "\n";
      sprintf(buf, "Cost:  %d r\t\tCargo:    %4d", dirship->build_cost,
              dirship->max_resource);
      notify(Playernum, Governor, buf);
      if (Shipdata[dirship->build_type][ABIL_LASER]) {
        sprintf(buf, "\t\tCombat Lasers: %s\n",
                (dirship->laser ? "yes" : "no"));
        notify(Playernum, Governor, buf);
      } else
        g.out << "\n";
      sprintf(buf, "Mass:  %.1f\t\tHanger:   %4u", dirship->base_mass,
              dirship->max_hanger);
      notify(Playernum, Governor, buf);
      if (Shipdata[dirship->build_type][ABIL_CEW]) {
        sprintf(buf, "\t\tCEW: %s\n", (dirship->cew ? "yes" : "no"));
        notify(Playernum, Governor, buf);
      } else
        g.out << "\n";
      sprintf(buf, "Size:  %-6d\t\tDestruct: %4d", dirship->size,
              dirship->max_destruct);
      notify(Playernum, Governor, buf);
      if (Shipdata[dirship->build_type][ABIL_CEW] && dirship->cew) {
        sprintf(buf, "\t\t   Opt Range: %4d\n", dirship->cew_range);
        notify(Playernum, Governor, buf);
      } else
        g.out << "\n";
      sprintf(buf, "Tech:  %.1f (%.1f)\tSpeed:    %4d", dirship->complexity,
              Race->tech, dirship->max_speed);
      notify(Playernum, Governor, buf);
      if (Shipdata[dirship->build_type][ABIL_CEW] && dirship->cew) {
        sprintf(buf, "\t\t   Energy:    %4d\n", dirship->cew);
        notify(Playernum, Governor, buf);
      } else
        g.out << "\n";

      if (Race->tech < dirship->complexity)
        notify(Playernum, Governor,
               "Your engineering capability is not "
               "advanced enough to produce this "
               "design.\n");
      free(dirship);
      return;
    }

    shipc = argv[1][0];

    i = 0;
    while ((Shipltrs[i] != shipc) && (i < NUMSTYPES)) i++;

    if ((i >= NUMSTYPES) || ((i == STYPE_POD) && (!Race->pods))) {
      sprintf(buf, "Illegal ship letter.\n");
      notify(Playernum, Governor, buf);
      free(dirship);
      return;
    }
    if (Shipdata[i][ABIL_GOD] && !Race->God) {
      g.out << "Nice try!\n";
      free(dirship);
      return;
    }
    if (!(Shipdata[i][ABIL_BUILD] & Shipdata[OTYPE_FACTORY][ABIL_CONSTRUCT])) {
      notify(Playernum, Governor,
             "This kind of ship does not require a factory to construct.\n");
      free(dirship);
      return;
    }

    dirship->build_type = i;
    dirship->armor = Shipdata[i][ABIL_ARMOR];
    dirship->guns = GTYPE_NONE; /* this keeps track of the factory status! */
    dirship->primary = Shipdata[i][ABIL_GUNS];
    dirship->primtype = Shipdata[i][ABIL_PRIMARY];
    dirship->secondary = Shipdata[i][ABIL_GUNS];
    dirship->sectype = Shipdata[i][ABIL_SECONDARY];
    dirship->max_crew = Shipdata[i][ABIL_MAXCREW];
    dirship->max_resource = Shipdata[i][ABIL_CARGO];
    dirship->max_hanger = Shipdata[i][ABIL_HANGER];
    dirship->max_fuel = Shipdata[i][ABIL_FUELCAP];
    dirship->max_destruct = Shipdata[i][ABIL_DESTCAP];
    dirship->max_speed = Shipdata[i][ABIL_SPEED];

    dirship->mount = Shipdata[i][ABIL_MOUNT] * Crystal(Race);
    dirship->hyper_drive.has = Shipdata[i][ABIL_JUMP] * Hyper_drive(Race);
    dirship->cloak = Shipdata[i][ABIL_CLOAK] * Cloak(Race);
    dirship->laser = Shipdata[i][ABIL_LASER] * Laser(Race);
    dirship->cew = 0;
    dirship->mode = 0;

    dirship->size = ship_size(dirship);
    dirship->complexity = complexity(dirship);

    sprintf(dirship->shipclass, "mod %ld", g.shipno);

    sprintf(buf, "Factory designated to produce %ss.\n", Shipnames[i]);
    notify(Playernum, Governor, buf);
    sprintf(buf, "Design complexity %.1f (%.1f).\n", dirship->complexity,
            Race->tech);
    notify(Playernum, Governor, buf);
    if (dirship->complexity > Race->tech)
      g.out << "You can't produce this design yet!\n";

  } else if (mode == 1) {
    if (!dirship->build_type) {
      notify(Playernum, Governor,
             "No ship design specified. Use 'make <ship type>' first.\n");
      free(dirship);
      return;
    }

    if (argv.size() < 2) {
      notify(Playernum, Governor,
             "You have to specify the characteristic you wish to modify.\n");
      free(dirship);
      return;
    }

    if (argv.size() == 3)
      value = std::stoi(argv[2]);
    else
      value = 0;

    if (value < 0) {
      g.out << "That's a ridiculous setting.\n";
      free(dirship);
      return;
    }

    if (Shipdata[dirship->build_type][ABIL_MOD]) {
      if (argv[1] == "armor") {
        dirship->armor = MIN(value, 100);
      } else if (argv[1] == "crew" &&
                 Shipdata[dirship->build_type][ABIL_MAXCREW]) {
        dirship->max_crew = MIN(value, 10000);
      } else if (argv[1] == "cargo" &&
                 Shipdata[dirship->build_type][ABIL_CARGO]) {
        dirship->max_resource = MIN(value, 10000);
      } else if (argv[1] == "hanger" &&
                 Shipdata[dirship->build_type][ABIL_HANGER]) {
        dirship->max_hanger = MIN(value, 10000);
      } else if (argv[1] == "fuel" &&
                 Shipdata[dirship->build_type][ABIL_FUELCAP]) {
        dirship->max_fuel = MIN(value, 10000);
      } else if (argv[1] == "destruct" &&
                 Shipdata[dirship->build_type][ABIL_DESTCAP]) {
        dirship->max_destruct = MIN(value, 10000);
      } else if (argv[1] == "speed" &&
                 Shipdata[dirship->build_type][ABIL_SPEED]) {
        dirship->max_speed = MAX(1, MIN(value, 9));
      } else if (argv[1] == "mount" &&
                 Shipdata[dirship->build_type][ABIL_MOUNT] && Crystal(Race)) {
        dirship->mount = !dirship->mount;
      } else if (argv[1] == "hyperdrive" &&
                 Shipdata[dirship->build_type][ABIL_JUMP] &&
                 Hyper_drive(Race)) {
        dirship->hyper_drive.has = !dirship->hyper_drive.has;
      } else if (argv[1] == "primary" &&
                 Shipdata[dirship->build_type][ABIL_PRIMARY]) {
        if (argv[2] == "strength") {
          dirship->primary = std::stoi(argv[3]);
        } else if (argv[2] == "caliber") {
          if (argv[3] == "light")
            dirship->primtype = GTYPE_LIGHT;
          else if (argv[3] == "medium")
            dirship->primtype = GTYPE_MEDIUM;
          else if (argv[3] == "heavy")
            dirship->primtype = GTYPE_HEAVY;
          else {
            g.out << "No such caliber.\n";
            free(dirship);
            return;
          }
          dirship->primtype = MIN(Shipdata[dirship->build_type][ABIL_PRIMARY],
                                  dirship->primtype);
        } else {
          g.out << "No such gun characteristic.\n";
          free(dirship);
          return;
        }
      } else if (argv[1] == "secondary" &&
                 Shipdata[dirship->build_type][ABIL_SECONDARY]) {
        if (argv[2] == "strength") {
          dirship->secondary = std::stoi(argv[3]);
        } else if (argv[2] == "caliber") {
          if (argv[3] == "light")
            dirship->sectype = GTYPE_LIGHT;
          else if (argv[3] == "medium")
            dirship->sectype = GTYPE_MEDIUM;
          else if (argv[3] == "heavy")
            dirship->sectype = GTYPE_HEAVY;
          else {
            g.out << "No such caliber.\n";
            free(dirship);
            return;
          }
          dirship->sectype = MIN(Shipdata[dirship->build_type][ABIL_SECONDARY],
                                 dirship->sectype);
        } else {
          g.out << "No such gun characteristic.\n";
          free(dirship);
          return;
        }
      } else if (argv[1] == "cew" && Shipdata[dirship->build_type][ABIL_CEW]) {
        if (!Cew(Race)) {
          sprintf(buf,
                  "Your race does not understand confined energy weapons.\n");
          notify(Playernum, Governor, buf);
          free(dirship);
          return;
        }
        if (!Shipdata[dirship->build_type][ABIL_CEW]) {
          notify(Playernum, Governor,
                 "This kind of ship cannot mount confined energy weapons.\n");
          free(dirship);
          return;
        }
        value = std::stoi(argv[3]);
        if (argv[2] == "strength") {
          dirship->cew = value;
        } else if (argv[2] == "range") {
          dirship->cew_range = value;
        } else {
          g.out << "No such option for CEWs.\n";
          free(dirship);
          return;
        }
      } else if (argv[1] == "laser" &&
                 Shipdata[dirship->build_type][ABIL_LASER]) {
        if (!Laser(Race)) {
          sprintf(buf, "Your race does not understand lasers yet.\n");
          notify(Playernum, Governor, buf);
          free(dirship);
          return;
        }
        if (Shipdata[dirship->build_type][ABIL_LASER])
          dirship->laser = !dirship->laser;
        else {
          notify(Playernum, Governor,
                 "That ship cannot be fitted with combat lasers.\n");
          free(dirship);
          return;
        }
      } else {
        notify(
            Playernum, Governor,
            "That characteristic either doesn't exist or can't be modified.\n");
        free(dirship);
        return;
      }
    } else if (Hyper_drive(Race)) {
      if (argv[1] == "hyperdrive") {
        dirship->hyper_drive.has = !dirship->hyper_drive.has;
      } else {
        notify(Playernum, Governor,
               "You may only modify hyperdrive "
               "installation on this kind of ship.\n");
        free(dirship);
        return;
      }
    } else {
      notify(Playernum, Governor,
             "Sorry, but you can't modify this ship right now.\n");
      free(dirship);
      return;
    }
  } else {
    g.out << "Weird error.\n";
    free(dirship);
    return;
  }
  /* compute how much it's going to cost to build the ship */

  if ((cost0 = cost(dirship)) > 65535.0) {
    notify(Playernum, Governor,
           "Woah!! YOU CHEATER!!!  The max cost allowed "
           "is 65535!!! I'm Telllllllling!!!\n");
    free(dirship);
    return;
  }

  dirship->build_cost = Race->God ? 0 : (int)cost0;
  sprintf(buf, "The current cost of the ship is %d resources.\n",
          dirship->build_cost);
  notify(Playernum, Governor, buf);
  dirship->size = ship_size(dirship);
  dirship->base_mass = getmass(dirship);
  sprintf(buf, "The current base mass of the ship is %.1f - size is %d.\n",
          dirship->base_mass, dirship->size);
  notify(Playernum, Governor, buf);
  dirship->complexity = complexity(dirship);
  sprintf(buf,
          "Ship complexity is %.1f (you have %.1f engineering technology).\n",
          dirship->complexity, Race->tech);
  notify(Playernum, Governor, buf);

  /* Restore size to what it was before.  Maarten */
  dirship->size = size;

  putship(dirship);
  free(dirship);
}

void build(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  // TODO(jeffbailey): Fix unused int APcount = 1;
  racetype *Race;
  Planet planet;
  char c;
  int i, j, m, n, x, y, count, what, outside;
  ScopeLevel level, build_level;
  int shipcost, load_crew;
  int snum, pnum;
  double load_fuel, tech;

  FILE *fd;
  sector sector;
  Ship *builder;
  Ship newship;

  if (argv.size() > 1 && argv[1][0] == '?') {
    /* information request */
    if (argv.size() == 2) {
      /* Ship parameter list */
      g.out << "     - Default ship parameters -\n";
      sprintf(buf,
              "%1s %-15s %5s %5s %3s %4s %3s %3s %3s %4s %4s %2s %4s %4s\n",
              "?", "name", "cargo", "hang", "arm", "dest", "gun", "pri", "sec",
              "fuel", "crew", "sp", "tech", "cost");
      notify(Playernum, Governor, buf);
      Race = races[Playernum - 1];
      for (j = 0; j < NUMSTYPES; j++) {
        i = ShipVector[j];
        if ((!Shipdata[i][ABIL_GOD]) || Race->God) {
          if (Race->pods || (i != STYPE_POD)) {
            if (Shipdata[i][ABIL_PROGRAMMED]) {
              sprintf(buf,
                      "%1c %-15.15s %5ld %5ld %3ld %4ld %3ld %3ld %3ld "
                      "%4ld %4ld %2ld %4.0f %4d\n",
                      Shipltrs[i], Shipnames[i], Shipdata[i][ABIL_CARGO],
                      Shipdata[i][ABIL_HANGER], Shipdata[i][ABIL_ARMOR],
                      Shipdata[i][ABIL_DESTCAP], Shipdata[i][ABIL_GUNS],
                      Shipdata[i][ABIL_PRIMARY], Shipdata[i][ABIL_SECONDARY],
                      Shipdata[i][ABIL_FUELCAP], Shipdata[i][ABIL_MAXCREW],
                      Shipdata[i][ABIL_SPEED], (double)Shipdata[i][ABIL_TECH],
                      Shipcost(i, Race));
              notify(Playernum, Governor, buf);
            }
          }
        }
      }
      return;
    } else {
      /* Description of specific ship type */
      i = 0;
      while (Shipltrs[i] != argv[2][0] && i < NUMSTYPES) i++;
      if (i < 0 || i >= NUMSTYPES)
        g.out << "No such ship type.\n";
      else if (!Shipdata[i][ABIL_PROGRAMMED])
        notify(Playernum, Governor,
               "This ship type has not been programmed.\n");
      else {
        if ((fd = fopen(EXAM_FL, "r")) == nullptr) {
          perror(EXAM_FL);
          return;
        } else {
          /* look through ship description file */
          sprintf(buf, "\n");
          for (j = 0; j <= i; j++)
            while (fgetc(fd) != '~')
              ;
          /* Give description */
          while ((c = fgetc(fd)) != '~') {
            sprintf(temp, "%c", c);
            strcat(buf, temp);
          }
          fclose(fd);
          /* Built where? */
          if (Shipdata[i][ABIL_BUILD] & 1) {
            sprintf(temp, "\nCan be constructed on planet.");
            strcat(buf, temp);
          }
          n = 0;
          sprintf(temp, "\nCan be built by ");
          for (j = 0; j < NUMSTYPES; j++)
            if (Shipdata[i][ABIL_BUILD] & Shipdata[j][ABIL_CONSTRUCT]) n++;
          if (n) {
            m = 0;
            strcat(buf, temp);
            for (j = 0; j < NUMSTYPES; j++) {
              if (Shipdata[i][ABIL_BUILD] & Shipdata[j][ABIL_CONSTRUCT]) {
                m++;
                if (n - m > 1)
                  sprintf(temp, "%c, ", Shipltrs[j]);
                else if (n - m > 0)
                  sprintf(temp, "%c and ", Shipltrs[j]);
                else
                  sprintf(temp, "%c ", Shipltrs[j]);
                strcat(buf, temp);
              }
            }
            sprintf(temp, "type ships.\n");
            strcat(buf, temp);
          }
          /* default parameters */
          sprintf(
              temp,
              "\n%1s %-15s %5s %5s %3s %4s %3s %3s %3s %4s %4s %2s %4s %4s\n",
              "?", "name", "cargo", "hang", "arm", "dest", "gun", "pri", "sec",
              "fuel", "crew", "sp", "tech", "cost");
          strcat(buf, temp);
          Race = races[Playernum - 1];
          sprintf(temp,
                  "%1c %-15.15s %5ld %5ld %3ld %4ld %3ld %3ld %3ld %4ld "
                  "%4ld %2ld %4.0f %4d\n",
                  Shipltrs[i], Shipnames[i], Shipdata[i][ABIL_CARGO],
                  Shipdata[i][ABIL_HANGER], Shipdata[i][ABIL_ARMOR],
                  Shipdata[i][ABIL_DESTCAP], Shipdata[i][ABIL_GUNS],
                  Shipdata[i][ABIL_PRIMARY], Shipdata[i][ABIL_SECONDARY],
                  Shipdata[i][ABIL_FUELCAP], Shipdata[i][ABIL_MAXCREW],
                  Shipdata[i][ABIL_SPEED], (double)Shipdata[i][ABIL_TECH],
                  Shipcost(i, Race));
          strcat(buf, temp);
          notify(Playernum, Governor, buf);
        }
      }
    }
    return;
  }

  level = g.level;
  if (level != ScopeLevel::LEVEL_SHIP && level != ScopeLevel::LEVEL_PLAN) {
    notify(Playernum, Governor,
           "You must change scope to a ship or planet to build.\n");
    return;
  }
  snum = g.snum;
  pnum = g.pnum;
  Race = races[Playernum - 1];
  count = 0; /* this used used to reset count in the loop */
  do {
    switch (level) {
      case ScopeLevel::LEVEL_PLAN:
        if (!count) { /* initialize loop variables */
          if (argv.size() < 2) {
            g.out << "Build what?\n";
            return;
          }
          if ((what = get_build_type(argv[1].c_str())) < 0) {
            g.out << "No such ship type.\n";
            return;
          }
          if (!can_build_this(what, Race, buf) && !Race->God) {
            notify(Playernum, Governor, buf);
            return;
          }
          if (!(Shipdata[what][ABIL_BUILD] & 1) && !Race->God) {
            notify(Playernum, Governor,
                   "This ship cannot be built by a planet.\n");
            return;
          }
          if (argv.size() < 3) {
            g.out << "Build where?\n";
            return;
          }
          planet = getplanet(snum, pnum);
          if (!can_build_at_planet(g, Stars[snum], planet) && !Race->God) {
            g.out << "You can't build that here.\n";
            return;
          }
          sscanf(argv[2].c_str(), "%d,%d", &x, &y);
          if (x < 0 || x >= planet.Maxx || y < 0 || y >= planet.Maxy) {
            g.out << "Illegal sector.\n";
            return;
          }
          sector = getsector(planet, x, y);
          if (!can_build_on_sector(what, Race, planet, sector, x, y, buf) &&
              !Race->God) {
            notify(Playernum, Governor, buf);
            return;
          }
          if (!(count = getcount(argv, 4))) {
            g.out << "Give a positive number of builds.\n";
            return;
          }
          Getship(&newship, what, Race);
        }
        if ((shipcost = newship.build_cost) >
            planet.info[Playernum - 1].resource) {
          sprintf(buf, "You need %dr to construct this ship.\n", shipcost);
          notify(Playernum, Governor, buf);
          goto finish;
        }
        create_ship_by_planet(Playernum, Governor, Race, &newship, &planet,
                              snum, pnum, x, y);
        if (Race->governor[Governor].toggle.autoload &&
            what != OTYPE_TRANSDEV && !Race->God)
          autoload_at_planet(Playernum, &newship, &planet, sector, &load_crew,
                             &load_fuel);
        else {
          load_crew = 0;
          load_fuel = 0.0;
        }
        initialize_new_ship(g, Race, &newship, load_fuel, load_crew);
        putship(&newship);
        break;
      case ScopeLevel::LEVEL_SHIP:
        if (!count) { /* initialize loop variables */
          (void)getship(&builder, g.shipno);
          outside = 0;
          auto test_build_level = build_at_ship(g, builder, &snum, &pnum);
          if (!test_build_level) {
            g.out << "You can't build here.\n";
            free(builder);
            return;
          }
          build_level = test_build_level.value();
          switch (builder->type) {
            case OTYPE_FACTORY:
              if (!(count = getcount(argv, 2))) {
                notify(Playernum, Governor,
                       "Give a positive number of builds.\n");
                free(builder);
                return;
              }
              if (!landed(builder)) {
                notify(Playernum, Governor,
                       "Factories can only build when landed on a planet.\n");
                free(builder);
                return;
              }
              Getfactship(&newship, builder);
              outside = 1;
              break;
            case STYPE_SHUTTLE:
            case STYPE_CARGO:
              if (landed(builder)) {
                notify(Playernum, Governor,
                       "This ships cannot build when landed.\n");
                free(builder);
                return;
              }
              outside = 1;
              [[clang::fallthrough]];  // TODO(jeffbailey): Added this to
                                       // silence
                                       // warning, check it.
            default:
              if (argv.size() < 2) {
                g.out << "Build what?\n";
                free(builder);
                return;
              }
              if ((what = get_build_type(argv[1].c_str())) < 0) {
                g.out << "No such ship type.\n";
                free(builder);
                return;
              }
              if (!can_build_on_ship(what, Race, builder, buf)) {
                notify(Playernum, Governor, buf);
                free(builder);
                return;
              }
              if (!(count = getcount(argv, 3))) {
                notify(Playernum, Governor,
                       "Give a positive number of builds.\n");
                free(builder);
                return;
              }
              Getship(&newship, what, Race);
              break;
          }
          if ((tech = builder->type == OTYPE_FACTORY
                          ? complexity(builder)
                          : Shipdata[what][ABIL_TECH]) > Race->tech &&
              !Race->God) {
            sprintf(buf,
                    "You are not advanced enough to build this ship.\n%.1f "
                    "enginering technology needed. You have %.1f.\n",
                    tech, Race->tech);
            notify(Playernum, Governor, buf);
            free(builder);
            return;
          }
          if (outside && build_level == ScopeLevel::LEVEL_PLAN) {
            planet = getplanet(snum, pnum);
            if (builder->type == OTYPE_FACTORY) {
              if (!can_build_at_planet(g, Stars[snum], planet)) {
                g.out << "You can't build that here.\n";
                free(builder);
                return;
              }
              x = builder->land_x;
              y = builder->land_y;
              what = builder->build_type;
              sector = getsector(planet, x, y);
              if (!can_build_on_sector(what, Race, planet, sector, x, y, buf)) {
                notify(Playernum, Governor, buf);
                free(builder);
                return;
              }
            }
          }
        }
        /* build 'em */
        switch (builder->type) {
          case OTYPE_FACTORY:
            if ((shipcost = newship.build_cost) >
                planet.info[Playernum - 1].resource) {
              sprintf(buf, "You need %dr to construct this ship.\n", shipcost);
              notify(Playernum, Governor, buf);
              goto finish;
            }
            create_ship_by_planet(Playernum, Governor, Race, &newship, &planet,
                                  snum, pnum, x, y);
            if (Race->governor[Governor].toggle.autoload &&
                what != OTYPE_TRANSDEV && !Race->God) {
              autoload_at_planet(Playernum, &newship, &planet, sector,
                                 &load_crew, &load_fuel);
            } else {
              load_crew = 0;
              load_fuel = 0.0;
            }
            break;
          case STYPE_SHUTTLE:
          case STYPE_CARGO:
            if (builder->resource < (shipcost = newship.build_cost)) {
              sprintf(buf, "You need %dr to construct the ship.\n", shipcost);
              notify(Playernum, Governor, buf);
              goto finish;
            }
            create_ship_by_ship(Playernum, Governor, Race, 1, &planet, &newship,
                                builder);
            if (Race->governor[Governor].toggle.autoload &&
                what != OTYPE_TRANSDEV && !Race->God)
              autoload_at_ship(&newship, builder, &load_crew, &load_fuel);
            else {
              load_crew = 0;
              load_fuel = 0.0;
            }
            break;
          default:
            if (builder->hanger + ship_size(&newship) > builder->max_hanger) {
              g.out << "Not enough hanger space.\n";
              goto finish;
            }
            if (builder->resource < (shipcost = newship.build_cost)) {
              sprintf(buf, "You need %dr to construct the ship.\n", shipcost);
              notify(Playernum, Governor, buf);
              goto finish;
            }
            create_ship_by_ship(Playernum, Governor, Race, 0, nullptr, &newship,
                                builder);
            if (Race->governor[Governor].toggle.autoload &&
                what != OTYPE_TRANSDEV && !Race->God)
              autoload_at_ship(&newship, builder, &load_crew, &load_fuel);
            else {
              load_crew = 0;
              load_fuel = 0.0;
            }
            break;
        }
        initialize_new_ship(g, Race, &newship, load_fuel, load_crew);
        putship(&newship);
        break;
    }
    count--;
  } while (count);
/* free stuff */
finish:
  switch (level) {
    case ScopeLevel::LEVEL_PLAN:
      putsector(sector, planet, x, y);
      putplanet(planet, Stars[snum], pnum);
      break;
    case ScopeLevel::LEVEL_SHIP:
      if (outside) switch (build_level) {
          case ScopeLevel::LEVEL_PLAN:
            putplanet(planet, Stars[snum], pnum);
            if (landed(builder)) {
              putsector(sector, planet, x, y);
            }
            break;
          case ScopeLevel::LEVEL_STAR:
            putstar(Stars[snum], snum);
            break;
          case ScopeLevel::LEVEL_UNIV:
            putsdata(&Sdata);
            break;
        }
      putship(builder);
      free(builder);
      break;
  }
}

// Used for optional parameters.  If the element requested exists, use
// it.  If the number is negative, return zero instead.
static int getcount(const command_t &argv, const size_t elem) {
  int count = argv.size() > elem ? std::stoi(argv[elem]) : 1;
  if (count <= 0) count = 0;
  return (count);
}

static int can_build_at_planet(GameObj &g, startype *star,
                               const Planet &planet) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  if (planet.slaved_to && planet.slaved_to != Playernum) {
    sprintf(buf, "This planet is enslaved by player %d.\n", planet.slaved_to);
    notify(Playernum, Governor, buf);
    return (0);
  }
  if (Governor && star->governor[Playernum - 1] != Governor) {
    g.out << "You are not authorized in this system.\n";
    return (0);
  }
  return (1);
}

static int get_build_type(const char *string) {
  char shipc;
  int i;

  shipc = string[0];
  i = -1;
  while (Shipltrs[i] != shipc && i < NUMSTYPES) i++;
  if (i < 0 || i >= NUMSTYPES) return (-1);
  return i;
}

static int can_build_this(int what, racetype *Race, char *string) {
  if (what == STYPE_POD && !Race->pods) {
    sprintf(string, "Only Metamorphic races can build Spore Pods.\n");
    return (0);
  }
  if (!Shipdata[what][ABIL_PROGRAMMED]) {
    sprintf(string, "This ship type has not been programmed.\n");
    return (0);
  }
  if (Shipdata[what][ABIL_GOD] && !Race->God) {
    sprintf(string, "Only Gods can build this type of ship.\n");
    return (0);
  }
  if (what == OTYPE_VN && !Vn(Race)) {
    sprintf(string, "You have not discovered VN technology.\n");
    return (0);
  }
  if (what == OTYPE_TRANSDEV && !Avpm(Race)) {
    sprintf(string, "You have not discovered AVPM technology.\n");
    return (0);
  }
  if (Shipdata[what][ABIL_TECH] > Race->tech && !Race->God) {
    sprintf(string,
            "You are not advanced enough to build this ship.\n%.1f "
            "enginering technology needed. You have %.1f.\n",
            (double)Shipdata[what][ABIL_TECH], Race->tech);
    return (0);
  }
  return 1;
}

static int can_build_on_ship(int what, racetype *Race, Ship *builder,
                             char *string) {
  if (!(Shipdata[what][ABIL_BUILD] & Shipdata[builder->type][ABIL_CONSTRUCT]) &&
      !Race->God) {
    sprintf(string, "This ship type cannot be built by a %s.\n",
            Shipnames[builder->type]);
    sprintf(temp, "Use 'build ? %c' to find out where it can be built.\n",
            Shipltrs[what]);
    strcat(string, temp);
    return (0);
  }
  return (1);
}

static int can_build_on_sector(int what, racetype *Race, const Planet &planet,
                               const sector &sector, int x, int y,
                               char *string) {
  Ship *s;
  char shipc;

  shipc = Shipltrs[what];
  if (!sector.popn) {
    sprintf(string, "You have no more civs in the sector!\n");
    return (0);
  }
  if (sector.condition == SectorType::SEC_WASTED) {
    sprintf(string, "You can't build on wasted sectors.\n");
    return (0);
  }
  if (sector.owner != Race->Playernum && !Race->God) {
    sprintf(string, "You don't own that sector.\n");
    return (0);
  }
  if ((!Shipdata[what][ABIL_BUILD] & 1) && !Race->God) {
    sprintf(string, "This ship type cannot be built on a planet.\n");
    sprintf(temp, "Use 'build ? %c' to find out where it can be built.\n",
            shipc);
    strcat(string, temp);
    return (0);
  }
  if (what == OTYPE_QUARRY) {
    int sh;
    sh = planet.ships;
    while (sh) {
      (void)getship(&s, sh);
      if (s->alive && s->type == OTYPE_QUARRY && s->land_x == x &&
          s->land_y == y) {
        sprintf(string, "There already is a quarry here.\n");
        free(s);
        return (0);
      }
      sh = s->nextship;
      free(s);
    }
  }
  return (1);
}

static std::optional<ScopeLevel> build_at_ship(GameObj &g, Ship *builder,
                                               int *snum, int *pnum) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  if (testship(Playernum, Governor, builder)) return {};
  if (!Shipdata[builder->type][ABIL_CONSTRUCT]) {
    g.out << "This ship cannot construct other ships.\n";
    return {};
  }
  if (!builder->popn) {
    g.out << "This ship has no crew.\n";
    return {};
  }
  if (docked(builder)) {
    g.out << "Undock this ship first.\n";
    return {};
  }
  if (builder->damage) {
    g.out << "This ship is damaged and cannot build.\n";
    return {};
  }
  if (builder->type == OTYPE_FACTORY && !builder->on) {
    g.out << "This factory is not online.\n";
    return {};
  }
  if (builder->type == OTYPE_FACTORY && !landed(builder)) {
    g.out << "Factories must be landed on a planet.\n";
    return {};
  }
  *snum = builder->storbits;
  *pnum = builder->pnumorbits;
  return (builder->whatorbits);
}

static void autoload_at_planet(int Playernum, Ship *s, Planet *planet,
                               sector &sector, int *crew, double *fuel) {
  *crew = MIN(s->max_crew, sector.popn);
  *fuel = MIN((double)s->max_fuel, (double)planet->info[Playernum - 1].fuel);
  sector.popn -= *crew;
  if (!sector.popn && !sector.troops) sector.owner = 0;
  planet->info[Playernum - 1].fuel -= (int)(*fuel);
}

static void autoload_at_ship(Ship *s, Ship *b, int *crew, double *fuel) {
  *crew = MIN(s->max_crew, b->popn);
  *fuel = MIN((double)s->max_fuel, (double)b->fuel);
  b->popn -= *crew;
  b->fuel -= *fuel;
}

static void initialize_new_ship(GameObj &g, racetype *Race, Ship *newship,
                                double load_fuel, int load_crew) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  newship->speed = newship->max_speed;
  newship->owner = Playernum;
  newship->governor = Governor;
  newship->fuel = Race->God ? newship->max_fuel : load_fuel;
  newship->popn = Race->God ? newship->max_crew : load_crew;
  newship->troops = 0;
  newship->resource = Race->God ? newship->max_resource : 0;
  newship->destruct = Race->God ? newship->max_destruct : 0;
  newship->crystals = 0;
  newship->hanger = 0;
  newship->mass = newship->base_mass + (double)newship->popn * Race->mass +
                  (double)newship->fuel * MASS_FUEL +
                  (double)newship->resource * MASS_RESOURCE +
                  (double)newship->destruct * MASS_DESTRUCT;
  newship->alive = 1;
  newship->active = 1;
  newship->protect.self = newship->guns ? 1 : 0;
  newship->hyper_drive.on = 0;
  newship->hyper_drive.ready = 0;
  newship->hyper_drive.charge = 0;
  newship->mounted = Race->God ? newship->mount : 0;
  newship->cloak = 0;
  newship->cloaked = 0;
  newship->fire_laser = 0;
  newship->mode = 0;
  newship->rad = 0;
  newship->damage = Race->God ? 0 : Shipdata[newship->type][ABIL_DAMAGE];
  newship->retaliate = newship->primary;
  newship->ships = 0;
  newship->on = 0;
  switch (newship->type) {
    case OTYPE_VN:
      newship->special.mind.busy = 1;
      newship->special.mind.progenitor = Playernum;
      newship->special.mind.generation = 1;
      newship->special.mind.target = 0;
      newship->special.mind.tampered = 0;
      break;
    case STYPE_MINE:
      newship->special.trigger.radius = 100; /* trigger radius */
      notify(Playernum, Governor,
             "Mine disarmed.\nTrigger radius set at 100.\n");
      break;
    case OTYPE_TRANSDEV:
      newship->special.transport.target = 0;
      newship->on = 0;
      g.out << "Receive OFF.  Change with order.\n";
      break;
    case OTYPE_AP:
      g.out << "Processor OFF.\n";
      break;
    case OTYPE_STELE:
    case OTYPE_GTELE:
      sprintf(buf, "Telescope range is %.2f.\n",
              tele_range(newship->type, newship->tech));
      notify(Playernum, Governor, buf);
      break;
    default:
      break;
  }
  if (newship->damage) {
    sprintf(buf,
            "Warning: This ship is constructed with a %d%% damage level.\n",
            newship->damage);
    notify(Playernum, Governor, buf);
    if (!Shipdata[newship->type][ABIL_REPAIR] && newship->max_crew)
      notify(Playernum, Governor,
             "It will need resources to become fully operational.\n");
  }
  if (Shipdata[newship->type][ABIL_REPAIR] && newship->max_crew)
    notify(Playernum, Governor,
           "This ship does not need resources to repair.\n");
  if (newship->type == OTYPE_FACTORY)
    notify(Playernum, Governor,
           "This factory may not begin repairs until it has been activated.\n");
  if (!newship->max_crew)
    notify(Playernum, Governor,
           "This ship is robotic, and may not repair itself.\n");
  sprintf(buf, "Loaded with %d crew and %.1f fuel.\n", load_crew, load_fuel);
  notify(Playernum, Governor, buf);
}

static void create_ship_by_planet(int Playernum, int Governor, racetype *Race,
                                  Ship *newship, Planet *planet, int snum,
                                  int pnum, int x, int y) {
  int shipno;

  newship->tech = Race->tech;
  newship->xpos = Stars[snum]->xpos + planet->xpos;
  newship->ypos = Stars[snum]->ypos + planet->ypos;
  newship->land_x = x;
  newship->land_y = y;
  sprintf(newship->shipclass,
          (((newship->type == OTYPE_TERRA) || (newship->type == OTYPE_PLOW))
               ? "5"
               : "Standard"));
  newship->whatorbits = ScopeLevel::LEVEL_PLAN;
  newship->whatdest = ScopeLevel::LEVEL_PLAN;
  newship->deststar = snum;
  newship->destpnum = pnum;
  newship->storbits = snum;
  newship->pnumorbits = pnum;
  newship->docked = 1;
  planet->info[Playernum - 1].resource -= newship->build_cost;
  while ((shipno = getdeadship()) == 0)
    ;
  if (shipno == -1) shipno = Numships() + 1;
  newship->number = shipno;
  newship->owner = Playernum;
  newship->governor = Governor;
  newship->ships = 0;
  insert_sh_plan(planet, newship);
  if (newship->type == OTYPE_TOXWC) {
    sprintf(buf, "Toxin concentration on planet was %d%%,",
            planet->conditions[TOXIC]);
    notify(Playernum, Governor, buf);
    if (planet->conditions[TOXIC] > TOXMAX)
      newship->special.waste.toxic = TOXMAX;
    else
      newship->special.waste.toxic = planet->conditions[TOXIC];
    planet->conditions[TOXIC] -= newship->special.waste.toxic;
    sprintf(buf, " now %d%%.\n", planet->conditions[TOXIC]);
    notify(Playernum, Governor, buf);
  }
  sprintf(buf, "%s built at a cost of %d resources.\n",
          ship_to_string(*newship).c_str(), newship->build_cost);
  notify(Playernum, Governor, buf);
  sprintf(buf, "Technology %.1f.\n", newship->tech);
  notify(Playernum, Governor, buf);
  sprintf(buf, "%s is on sector %d,%d.\n", ship_to_string(*newship).c_str(),
          newship->land_x, newship->land_y);
  notify(Playernum, Governor, buf);
}

static void create_ship_by_ship(int Playernum, int Governor, racetype *Race,
                                int outside, Planet *planet, Ship *newship,
                                Ship *builder) {
  int shipno;

  while ((shipno = getdeadship()) == 0)
    ;
  if (shipno == -1) shipno = Numships() + 1;
  newship->number = shipno;
  newship->owner = Playernum;
  newship->governor = Governor;
  if (outside) {
    newship->whatorbits = builder->whatorbits;
    newship->whatdest = ScopeLevel::LEVEL_UNIV;
    newship->deststar = builder->deststar;
    newship->destpnum = builder->destpnum;
    newship->storbits = builder->storbits;
    newship->pnumorbits = builder->pnumorbits;
    newship->docked = 0;
    switch (builder->whatorbits) {
      case ScopeLevel::LEVEL_PLAN:
        insert_sh_plan(planet, newship);
        break;
      case ScopeLevel::LEVEL_STAR:
        insert_sh_star(Stars[builder->storbits], newship);
        break;
      case ScopeLevel::LEVEL_UNIV:
        insert_sh_univ(&Sdata, newship);
        break;
      case ScopeLevel::LEVEL_SHIP:
        // TODO(jeffbailey): The compiler can't see that this is impossible.
        break;
    }
  } else {
    newship->whatorbits = ScopeLevel::LEVEL_SHIP;
    newship->whatdest = ScopeLevel::LEVEL_SHIP;
    newship->deststar = builder->deststar;
    newship->destpnum = builder->destpnum;
    newship->destshipno = builder->number;
    newship->storbits = builder->storbits;
    newship->pnumorbits = builder->pnumorbits;
    newship->docked = 1;
    insert_sh_ship(newship, builder);
  }
  newship->tech = Race->tech;
  newship->xpos = builder->xpos;
  newship->ypos = builder->ypos;
  newship->land_x = builder->land_x;
  newship->land_y = builder->land_y;
  sprintf(newship->shipclass,
          (((newship->type == OTYPE_TERRA) || (newship->type == OTYPE_PLOW))
               ? "5"
               : "Standard"));
  builder->resource -= newship->build_cost;

  sprintf(buf, "%s built at a cost of %d resources.\n",
          ship_to_string(*newship).c_str(), newship->build_cost);
  notify(Playernum, Governor, buf);
  sprintf(buf, "Technology %.1f.\n", newship->tech);
  notify(Playernum, Governor, buf);
}

double getmass(Ship *s) {
  return (1.0 + MASS_ARMOR * s->armor + MASS_SIZE * (s->size - s->max_hanger) +
          MASS_HANGER * s->max_hanger + MASS_GUNS * s->primary * s->primtype +
          MASS_GUNS * s->secondary * s->sectype);
}

unsigned int ship_size(Ship *s) {
  double size;
  size = 1.0 + SIZE_GUNS * s->primary + SIZE_GUNS * s->secondary +
         SIZE_CREW * s->max_crew + SIZE_RESOURCE * s->max_resource +
         SIZE_FUEL * s->max_fuel + SIZE_DESTRUCT * s->max_destruct +
         s->max_hanger;
  return (std::floor(size));
}

double cost(Ship *s) {
  int i;
  double factor = 0.0, advantage = 0.0;

  i = s->build_type;
  /* compute how much it costs to build this ship */
  factor += (double)Shipdata[i][ABIL_COST];
  factor += GUN_COST * (double)s->primary;
  factor += GUN_COST * (double)s->secondary;
  factor += CREW_COST * (double)s->max_crew;
  factor += CARGO_COST * (double)s->max_resource;
  factor += FUEL_COST * (double)s->max_fuel;
  factor += AMMO_COST * (double)s->max_destruct;
  factor +=
      SPEED_COST * (double)s->max_speed * (double)sqrt((double)s->max_speed);
  factor += HANGER_COST * (double)s->max_hanger;
  factor += ARMOR_COST * (double)s->armor * (double)sqrt((double)s->armor);
  factor += CEW_COST * (double)(s->cew * s->cew_range);
  /* additional advantages/disadvantages */

  advantage += 0.5 * !!s->hyper_drive.has;
  advantage += 0.5 * !!s->laser;
  advantage += 0.5 * !!s->cloak;
  advantage += 0.5 * !!s->mount;

  factor *= sqrt(1.0 + advantage);
  return (factor);
}

static void system_cost(double *advantage, double *disadvantage, int value,
                        int base) {
  double factor;

  factor = (((double)value + 1.0) / (base + 1.0)) - 1.0;
  if (factor >= 0.0)
    *advantage += factor;
  else
    *disadvantage -= factor;
}

double complexity(Ship *s) {
  int i;
  double advantage, disadvantage, factor, tmp;

  i = s->build_type;

  advantage = 0.;
  disadvantage = 0.;

  system_cost(&advantage, &disadvantage, (int)(s->primary),
              Shipdata[i][ABIL_GUNS]);
  system_cost(&advantage, &disadvantage, (int)(s->secondary),
              Shipdata[i][ABIL_GUNS]);
  system_cost(&advantage, &disadvantage, (int)(s->max_crew),
              Shipdata[i][ABIL_MAXCREW]);
  system_cost(&advantage, &disadvantage, (int)(s->max_resource),
              Shipdata[i][ABIL_CARGO]);
  system_cost(&advantage, &disadvantage, (int)(s->max_fuel),
              Shipdata[i][ABIL_FUELCAP]);
  system_cost(&advantage, &disadvantage, (int)(s->max_destruct),
              Shipdata[i][ABIL_DESTCAP]);
  system_cost(&advantage, &disadvantage, (int)(s->max_speed),
              Shipdata[i][ABIL_SPEED]);
  system_cost(&advantage, &disadvantage, (int)(s->max_hanger),
              Shipdata[i][ABIL_HANGER]);
  system_cost(&advantage, &disadvantage, (int)(s->armor),
              Shipdata[i][ABIL_ARMOR]);
  /* additional advantages/disadvantages */

  // TODO(jeffbailey): document this function in English.
  factor = sqrt((1.0 + advantage) * exp(-(double)disadvantage / 10.0));
  tmp = COMPLEXITY_FACTOR * (factor - 1.0) /
            sqrt((double)(Shipdata[i][ABIL_TECH] + 1)) +
        1.0;
  factor = tmp * tmp;
  return (factor * (double)Shipdata[i][ABIL_TECH]);
}

static void Getship(Ship *s, int i, racetype *r) {
  bzero((char *)s, sizeof(Ship));
  s->type = i;
  s->armor = Shipdata[i][ABIL_ARMOR];
  s->guns = Shipdata[i][ABIL_PRIMARY] ? PRIMARY : GTYPE_NONE;
  s->primary = Shipdata[i][ABIL_GUNS];
  s->primtype = Shipdata[i][ABIL_PRIMARY];
  s->secondary = Shipdata[i][ABIL_GUNS];
  s->sectype = Shipdata[i][ABIL_SECONDARY];
  s->max_crew = Shipdata[i][ABIL_MAXCREW];
  s->max_resource = Shipdata[i][ABIL_CARGO];
  s->max_hanger = Shipdata[i][ABIL_HANGER];
  s->max_destruct = Shipdata[i][ABIL_DESTCAP];
  s->max_fuel = Shipdata[i][ABIL_FUELCAP];
  s->max_speed = Shipdata[i][ABIL_SPEED];
  s->build_type = i;
  s->mount = r->God ? Shipdata[i][ABIL_MOUNT] : 0;
  s->hyper_drive.has = r->God ? Shipdata[i][ABIL_JUMP] : 0;
  s->cloak = 0;
  s->laser = r->God ? Shipdata[i][ABIL_LASER] : 0;
  s->cew = 0;
  s->cew_range = 0;
  s->size = ship_size(s);
  s->base_mass = getmass(s);
  s->mass = getmass(s);
  s->build_cost = r->God ? 0 : (int)cost(s);
  if (s->type == OTYPE_VN || s->type == OTYPE_BERS)
    s->special.mind.progenitor = r->Playernum;
}

static void Getfactship(Ship *s, Ship *b) {
  bzero((char *)s, sizeof(Ship));
  s->type = b->build_type;
  s->armor = b->armor;
  s->primary = b->primary;
  s->primtype = b->primtype;
  s->secondary = b->secondary;
  s->sectype = b->sectype;
  s->guns = s->primary ? PRIMARY : GTYPE_NONE;
  s->max_crew = b->max_crew;
  s->max_resource = b->max_resource;
  s->max_hanger = b->max_hanger;
  s->max_destruct = b->max_destruct;
  s->max_fuel = b->max_fuel;
  s->max_speed = b->max_speed;
  s->build_type = b->build_type;
  s->build_cost = b->build_cost;
  s->mount = b->mount;
  s->hyper_drive.has = b->hyper_drive.has;
  s->cloak = 0;
  s->laser = b->laser;
  s->cew = b->cew;
  s->cew_range = b->cew_range;
  s->size = ship_size(s);
  s->base_mass = getmass(s);
  s->mass = getmass(s);
}

int Shipcost(int i, racetype *r) {
  Ship s;

  Getship(&s, i, r);
  return ((int)cost(&s));
}

#ifdef MARKET
void sell(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int APcount = 20;
  racetype *Race;
  Ship *s;
  commodtype c;
  int commodno, amount, item, ok = 0, sh;
  char commod;
  int snum, pnum;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    g.out << "You have to be in a planet scope to sell.\n";
    return;
  }
  snum = g.snum;
  pnum = g.pnum;
  if (argv.size() < 3) {
    g.out << "Syntax: sell <commodity> <amount>\n";
    return;
  }
  if (Governor && Stars[snum]->governor[Playernum - 1] != Governor) {
    g.out << "You are not authorized in this system.\n";
    return;
  }
  Race = races[Playernum - 1];
  if (Race->Guest) {
    g.out << "Guest races can't sell anything.\n";
    return;
  }
  /* get information on sale */
  commod = argv[1][0];
  amount = std::stoi(argv[2]);
  if (amount <= 0) {
    g.out << "Try using positive values.\n";
    return;
  }
  APcount = MIN(APcount, amount);
  if (!enufAP(Playernum, Governor, Stars[snum]->AP[Playernum - 1], APcount))
    return;
  auto p = getplanet(snum, pnum);

  if (p.slaved_to && p.slaved_to != Playernum) {
    sprintf(buf, "This planet is enslaved to player %d.\n", p.slaved_to);
    notify(Playernum, Governor, buf);
    return;
  }
  /* check to see if there is an undamage gov center or space port here */
  sh = p.ships;
  while (sh && !ok) {
    (void)getship(&s, sh);
    if (s->alive && (s->owner == Playernum) && !s->damage &&
        Shipdata[s->type][ABIL_PORT])
      ok = 1;
    sh = s->nextship;
    free(s);
  }
  if (!ok) {
    notify(
        Playernum, Governor,
        "You don't have an undamaged space port or government center here.\n");
    return;
  }
  switch (commod) {
    case 'r':
      if (!p.info[Playernum - 1].resource) {
        notify(Playernum, Governor,
               "You don't have any resources here to sell!\n");
        return;
      }
      amount = MIN(amount, p.info[Playernum - 1].resource);
      p.info[Playernum - 1].resource -= amount;
      item = RESOURCE;
      break;
    case 'd':
      if (!p.info[Playernum - 1].destruct) {
        notify(Playernum, Governor,
               "You don't have any destruct here to sell!\n");
        return;
      }
      amount = MIN(amount, p.info[Playernum - 1].destruct);
      p.info[Playernum - 1].destruct -= amount;
      item = DESTRUCT;
      break;
    case 'f':
      if (!p.info[Playernum - 1].fuel) {
        g.out << "You don't have any fuel here to sell!\n";
        return;
      }
      amount = MIN(amount, p.info[Playernum - 1].fuel);
      p.info[Playernum - 1].fuel -= amount;
      item = FUEL;
      break;
    case 'x':
      if (!p.info[Playernum - 1].crystals) {
        notify(Playernum, Governor,
               "You don't have any crystals here to sell!\n");
        return;
      }
      amount = MIN(amount, p.info[Playernum - 1].crystals);
      p.info[Playernum - 1].crystals -= amount;
      item = CRYSTAL;
      break;
    default:
      notify(Playernum, Governor,
             "Permitted commodities are r, d, f, and x.\n");
      return;
  }

  c.owner = Playernum;
  c.governor = Governor;
  c.type = item;
  c.amount = amount;
  c.deliver = 0;
  c.bid = 0;
  c.bidder = 0;
  c.star_from = snum;
  c.planet_from = pnum;

  while ((commodno = getdeadcommod()) == 0)
    ;

  if (commodno == -1) commodno = Numcommods() + 1;
  sprintf(buf, "Lot #%d - %d units of %s.\n", commodno, amount, Commod[item]);
  notify(Playernum, Governor, buf);
  sprintf(buf, "Lot #%d - %d units of %s for sale by %s [%d].\n", commodno,
          amount, Commod[item], races[Playernum - 1]->name, Playernum);
  post(buf, TRANSFER);
  for (player_t i = 1; i <= Num_races; i++) notify_race(i, buf);
  putcommod(&c, commodno);
  putplanet(p, Stars[snum], pnum);
  deductAPs(Playernum, Governor, APcount, snum, 0);
}

void bid(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  racetype *Race;
  Planet p;
  commodtype *c;
  Ship *s;
  char commod;
  int i, item, lot, shipping, ok = 0, sh;
  double dist, rate;
  int snum, pnum;

  if (argv.size() == 1) {
    /* list all market blocks for sale */
    notify(Playernum, Governor,
           "+++ Galactic Bloodshed Commodities Market +++\n\n");
    notify(Playernum, Governor,
           "  Lot Stock      Type  Owner  Bidder  Amount "
           "Cost/Unit    Ship  Dest\n");
    for (i = 1; i <= Numcommods(); i++) {
      getcommod(&c, i);
      if (c->owner && c->amount) {
        rate = (double)c->bid / (double)c->amount;
        if (c->bidder == Playernum)
          sprintf(temp, "%4.4s/%-4.4s", Stars[c->star_to]->name,
                  Stars[c->star_to]->pnames[c->planet_to]);
        else
          temp[0] = '\0';
        sprintf(
            buf, " %4d%c%5lu%10s%7d%8d%8ld%10.2f%8d %10s\n", i,
            c->deliver ? '*' : ' ', c->amount, Commod[c->type], c->owner,
            c->bidder, c->bid, rate,
            shipping_cost((int)c->star_from, (int)g.snum, &dist, (int)c->bid),
            temp);
        notify(Playernum, Governor, buf);
      }
      free(c);
    }
  } else if (argv.size() == 2) {
    /* list all market blocks for sale of the requested type */
    commod = argv[1][0];
    switch (commod) {
      case 'r':
        item = RESOURCE;
        break;
      case 'd':
        item = DESTRUCT;
        break;
      case 'f':
        item = FUEL;
        break;
      case 'x':
        item = CRYSTAL;
        break;
      default:
        g.out << "No such type of commodity.\n";
        return;
    }
    notify(Playernum, Governor,
           "+++ Galactic Bloodshed Commodities Market +++\n\n");
    notify(Playernum, Governor,
           "  Lot Stock      Type  Owner  Bidder  Amount "
           "Cost/Unit    Ship  Dest\n");
    for (i = 1; i <= Numcommods(); i++) {
      getcommod(&c, i);
      if (c->owner && c->amount && (c->type == item)) {
        rate = (double)c->bid / (double)c->amount;
        if (c->bidder == Playernum)
          sprintf(temp, "%4.4s/%-4.4s", Stars[c->star_to]->name,
                  Stars[c->star_to]->pnames[c->planet_to]);
        else
          temp[0] = '\0';
        sprintf(
            buf, " %4d%c%5lu%10s%7d%8d%8ld%10.2f%8d %10s\n", i,
            c->deliver ? '*' : ' ', c->amount, Commod[c->type], c->owner,
            c->bidder, c->bid, rate,
            shipping_cost((int)c->star_from, (int)g.snum, &dist, (int)c->bid),
            temp);
        notify(Playernum, Governor, buf);
      }
      free(c);
    }
  } else {
    if (g.level != ScopeLevel::LEVEL_PLAN) {
      g.out << "You have to be in a planet scope to buy.\n";
      return;
    }
    snum = g.snum;
    pnum = g.pnum;
    if (Governor && Stars[snum]->governor[Playernum - 1] != Governor) {
      g.out << "You are not authorized in this system.\n";
      return;
    }
    p = getplanet(snum, pnum);

    if (p.slaved_to && p.slaved_to != Playernum) {
      sprintf(buf, "This planet is enslaved to player %d.\n", p.slaved_to);
      notify(Playernum, Governor, buf);
      return;
    }
    /* check to see if there is an undamaged gov center or space port here */
    sh = p.ships;
    while (sh && !ok) {
      (void)getship(&s, sh);
      if (s->alive && (s->owner == Playernum) && !s->damage &&
          Shipdata[s->type][ABIL_PORT])
        ok = 1;
      sh = s->nextship;
      free(s);
    }
    if (!ok) {
      notify(Playernum, Governor,
             "You don't have an undamaged space port or "
             "government center here.\n");
      return;
    }

    lot = std::stoi(argv[1]);
    money_t bid0 = std::stoi(argv[2]);
    if ((lot <= 0) || lot > Numcommods()) {
      g.out << "Illegal lot number.\n";
      return;
    }
    getcommod(&c, lot);
    if (!c->owner) {
      g.out << "No such lot for sale.\n";
      free(c);
      return;
    }
    if (c->owner == g.player &&
        (c->star_from != g.snum || c->planet_from != g.pnum)) {
      notify(Playernum, Governor,
             "You can only set a minimum price for your "
             "lot from the location it was sold.\n");
      free(c);
      return;
    }
    money_t minbid = (int)((double)c->bid * (1.0 + UP_BID));
    if (bid0 < minbid) {
      sprintf(buf, "You have to bid more than %ld.\n", minbid);
      notify(Playernum, Governor, buf);
      free(c);
      return;
    }
    Race = races[Playernum - 1];
    if (Race->Guest) {
      g.out << "Guest races cannot bid.\n";
      free(c);
      return;
    }
    if (bid0 > Race->governor[Governor].money) {
      g.out << "Sorry, no buying on credit allowed.\n";
      free(c);
      return;
    }
    /* notify the previous bidder that he was just outbidded */
    if (c->bidder) {
      sprintf(buf,
              "The bid on lot #%d (%lu %s) has been upped to %ld by %s [%d].\n",
              lot, c->amount, Commod[c->type], bid0, Race->name, Playernum);
      notify((int)c->bidder, (int)c->bidder_gov, buf);
    }
    c->bid = bid0;
    c->bidder = Playernum;
    c->bidder_gov = Governor;
    c->star_to = snum;
    c->planet_to = pnum;
    shipping =
        shipping_cost((int)c->star_to, (int)c->star_from, &dist, (int)c->bid);

    sprintf(
        buf,
        "There will be an additional %d charged to you for shipping costs.\n",
        shipping);
    notify(Playernum, Governor, buf);
    putcommod(c, lot);
    g.out << "Bid accepted.\n";
    free(c);
  }
}

int shipping_cost(int to, int from, double *dist, int value) {
  double factor, fcost;
  int junk;

  *dist = sqrt(Distsq(Stars[to]->xpos, Stars[to]->ypos, Stars[from]->xpos,
                      Stars[from]->ypos));

  junk = (int)(*dist / 10000.0);
  junk *= 10000;

  factor = 1.0 - exp(-(double)junk / MERCHANT_LENGTH);

  fcost = factor * (double)value;
  return (int)fcost;
}
#endif
