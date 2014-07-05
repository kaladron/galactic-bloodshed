/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky,
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h
 * build -- build a ship
 * Mon Apr 15 02:07:08 MDT 1991
 * 	Reformatted the 'make' command when at Factory scope.
 *	Evan Koffler
 */

#include "GB_copyright.h"

#include <math.h>
#include <curses.h>
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#define EXTERN extern
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "shipdata.h"
#include "power.h"
#include "buffers.h"
#include "shootblast.p"

extern int ShipVector[];

void upgrade(int, int, int);
void make_mod(int, int, int, int);
void build(int, int, int);
int getcount(int, char *);
int can_build_at_planet(int, int, startype *, planettype *);
int get_build_type(char *);
int can_build_this(int, racetype *, char *);
int can_build_on_ship(int, racetype *, shiptype *, char *);
int can_build_on_sector(int, racetype *, planettype *, sectortype *, int, int,
                        char *);
int build_at_ship(int, int, racetype *, shiptype *, int *, int *);
void autoload_at_planet(int, shiptype *, planettype *, sectortype *, int *,
                        double *);
void autoload_at_ship(int, shiptype *, shiptype *, int *, double *);
void initialize_new_ship(int, int, racetype *, shiptype *, double, int);
void create_ship_by_planet(int, int, racetype *, shiptype *, planettype *, int,
                           int, int, int);
void create_ship_by_ship(int, int, racetype *, int, startype *, planettype *,
                         shiptype *, shiptype *);
double getmass(shiptype *);
int ship_size(shiptype *);
double cost(shiptype *);
void system_cost(double *, double *, int, int);
double complexity(shiptype *);
void Getship(shiptype *, int, racetype *);
void Getfactship(shiptype *, shiptype *);
int Shipcost(int, racetype *);
void sell(int, int, int);
void bid(int, int, int);
int shipping_cost(int, int, double *, int);
#include "GB_server.p"
#include "files_shl.p"
#include "getplace.p"
#include "shlmisc.p"
#include "fire.p"
#include "land.p"
#include "shootblast.p"
#include "teleg_send.p"

/* upgrade ship characteristics */
void upgrade(int Playernum, int Governor, int APcount) {
  int value, oldcost, newcost, netcost;
  shiptype ship, *dirship, *s2;
  double complex;
  racetype *Race;

  if (Dir[Playernum - 1][Governor].level != LEVEL_SHIP) {
    notify(Playernum, Governor,
           "You have to change scope to the ship you wish to upgrade.\n");
    return;
  }
  if (!getship(&dirship, Dir[Playernum - 1][Governor].shipno)) {
    sprintf(buf, "Illegal dir value.\n");
    notify(Playernum, Governor, buf);
    return;
  }
  if (testship(Playernum, Governor, dirship)) {
    free(dirship);
    return;
  }
  if (dirship->damage) {
    notify(Playernum, Governor, "You cannot upgrade damaged ships.\n");
    free(dirship);
    return;
  }
  if (dirship->type == OTYPE_FACTORY) {
    notify(Playernum, Governor, "You can't upgrade factories.\n");
    free(dirship);
    return;
  }

  Race = races[Playernum - 1];
  bcopy(dirship, &ship, sizeof(shiptype));

  if (argn == 3)
    sscanf(args[2], "%d", &value);
  else
    value = 0;

  if (value < 0) {
    notify(Playernum, Governor, "That's a ridiculous setting.\n");
    free(dirship);
    return;
  }

  if (Shipdata[dirship->build_type][ABIL_MOD]) {
    if (match(args[1], "armor")) {
      ship.armor = MAX(dirship->armor, MIN(value, 100));
    } else if (match(args[1], "crew") &&
               Shipdata[dirship->build_type][ABIL_MAXCREW]) {
      ship.max_crew = MAX(dirship->max_crew, MIN(value, 10000));
    } else if (match(args[1], "cargo") &&
               Shipdata[dirship->build_type][ABIL_CARGO]) {
      ship.max_resource = MAX(dirship->max_resource, MIN(value, 10000));
    } else if (match(args[1], "hanger") &&
               Shipdata[dirship->build_type][ABIL_HANGER]) {
      ship.max_hanger = MAX(dirship->max_hanger, MIN(value, 10000));
    } else if (match(args[1], "fuel") &&
               Shipdata[dirship->build_type][ABIL_FUELCAP]) {
      ship.max_fuel = MAX(dirship->max_fuel, MIN(value, 10000));
    } else if (match(args[1], "mount") &&
               Shipdata[dirship->build_type][ABIL_MOUNT] && !dirship->mount) {
      if (!Crystal(Race)) {
        notify(Playernum, Governor,
               "Your race does not now how to utilize crystal power yet.\n");
        free(dirship);
        return;
      }
      ship.mount = !ship.mount;
    } else if (match(args[1], "destruct") &&
               Shipdata[dirship->build_type][ABIL_DESTCAP]) {
      ship.max_destruct = MAX(dirship->max_destruct, MIN(value, 10000));
    } else if (match(args[1], "speed") &&
               Shipdata[dirship->build_type][ABIL_SPEED]) {
      ship.max_speed = MAX(dirship->max_speed, MAX(1, MIN(value, 9)));
    } else if (match(args[1], "hyperdrive") &&
               Shipdata[dirship->build_type][ABIL_JUMP] &&
               !dirship->hyper_drive.has && Hyper_drive(Race)) {
      ship.hyper_drive.has = 1;
    } else if (match(args[1], "primary") &&
               Shipdata[dirship->build_type][ABIL_PRIMARY]) {
      if (match(args[2], "strength")) {
        if (ship.primtype == NONE) {
          notify(Playernum, Governor, "No caliber defined.\n");
          free(dirship);
          return;
        }
        ship.primary = atoi(args[3]);
        ship.primary = MAX(ship.primary, dirship->primary);
      } else if (match(args[2], "caliber")) {
        if (match(args[3], "light"))
          ship.primtype = MAX(LIGHT, dirship->primtype);
        else if (match(args[3], "medium"))
          ship.primtype = MAX(MEDIUM, dirship->primtype);
        else if (match(args[3], "heavy"))
          ship.primtype = MAX(HEAVY, dirship->primtype);
        else {
          notify(Playernum, Governor, "No such caliber.\n");
          free(dirship);
          return;
        }
        ship.primtype =
            MIN(Shipdata[dirship->build_type][ABIL_PRIMARY], ship.primtype);
      } else {
        notify(Playernum, Governor, "No such gun characteristic.\n");
        free(dirship);
        return;
      }
    } else if (match(args[1], "secondary") &&
               Shipdata[dirship->build_type][ABIL_SECONDARY]) {
      if (match(args[2], "strength")) {
        if (ship.sectype == NONE) {
          notify(Playernum, Governor, "No caliber defined.\n");
          free(dirship);
          return;
        }
        ship.secondary = atoi(args[3]);
        ship.secondary = MAX(ship.secondary, dirship->secondary);
      } else if (match(args[2], "caliber")) {
        if (match(args[3], "light"))
          ship.sectype = MAX(LIGHT, dirship->sectype);
        else if (match(args[3], "medium"))
          ship.sectype = MAX(MEDIUM, dirship->sectype);
        else if (match(args[3], "heavy"))
          ship.sectype = MAX(HEAVY, dirship->sectype);
        else {
          notify(Playernum, Governor, "No such caliber.\n");
          free(dirship);
          return;
        }
        ship.sectype =
            MIN(Shipdata[dirship->build_type][ABIL_SECONDARY], ship.sectype);
      } else {
        notify(Playernum, Governor, "No such gun characteristic.\n");
        free(dirship);
        return;
      }
    } else if (match(args[1], "cew") &&
               Shipdata[dirship->build_type][ABIL_CEW]) {
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
      value = atoi(args[3]);
      if (match(args[2], "strength")) {
        ship.cew = value;
      } else if (match(args[2], "range")) {
        ship.cew_range = value;
      } else {
        notify(Playernum, Governor, "No such option for CEWs.\n");
        free(dirship);
        return;
      }
    } else if (match(args[1], "laser") &&
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
    notify(Playernum, Governor, "This ship cannot be upgraded.\n");
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
  if (dirship->whatorbits == LEVEL_SHIP) {
    (void)getship(&s2, dirship->destshipno);
    if (s2->max_hanger - (s2->hanger - dirship->size) < ship_size(&ship)) {
      sprintf(buf, "Not enough free hanger space on %c%d.\n",
              Shipltrs[s2->type], dirship->destshipno);
      notify(Playernum, Governor, buf);
      sprintf(buf, "%d more needed.\n",
              ship_size(&ship) -
                  (s2->max_hanger - (s2->hanger - dirship->size)));
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
    notify(Playernum, Governor, "You cannot downgrade ships!\n");
    free(dirship);
    return;
  }
  if (!Race->God)
    netcost += !netcost;

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
    bcopy(&ship, dirship, sizeof(shiptype));
    dirship->resource -= netcost;
    if (dirship->whatorbits == LEVEL_SHIP) {
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
    notify(Playernum, Governor, "You can not make this modification.\n");
  free(dirship);
}

void make_mod(int Playernum, int Governor, int APcount, int mode) {
  int i, value;
  unsigned short size;
  char shipc;
  shiptype *dirship;
  racetype *Race;
  double cost0;

  if (Dir[Playernum - 1][Governor].level != LEVEL_SHIP) {
    notify(Playernum, Governor,
           "You have to change scope to an installation.\n");
    return;
  }

  if (!getship(&dirship, Dir[Playernum - 1][Governor].shipno)) {
    sprintf(buf, "Illegal dir value.\n");
    notify(Playernum, Governor, buf);
    return;
  }
  if (testship(Playernum, Governor, dirship)) {
    free(dirship);
    return;
  }
  if (dirship->type != OTYPE_FACTORY) {
    notify(Playernum, Governor, "That is not a factory.\n");
    free(dirship);
    return;
  }
  if (dirship->on && argn > 1) {
    notify(Playernum, Governor, "This factory is already online.\n");
    free(dirship);
    return;
  }
  Race = races[Playernum - 1];

  /* Save  size of the factory, and set it to the
     correct values for the design.  Maarten */
  size = dirship->size;
  dirship->size = ship_size(dirship);

  if (mode == 0) {
    if (argn < 2) { /* list the current settings for the factory */
      if (!dirship->build_type) {
        notify(Playernum, Governor, "No ship type specified.\n");
        free(dirship);
        return;
      }
      notify(Playernum, Governor,
             "  --- Current Production Specifications ---\n");
      sprintf(buf, "%s\t\t\tArmor:    %4d\t\tGuns:",
              (dirship->on ? "Online" : "Offline"), dirship->armor);
      notify(Playernum, Governor, buf);
      if (Shipdata[dirship->build_type][ABIL_PRIMARY] &&
          dirship->primtype != NONE) {
        sprintf(buf, "%3d%c", dirship->primary,
                (dirship->primtype == LIGHT
                     ? 'L'
                     : dirship->primtype == MEDIUM
                           ? 'M'
                           : dirship->primtype == HEAVY ? 'H' : 'N'));
        notify(Playernum, Governor, buf);
      }
      if (Shipdata[dirship->build_type][ABIL_SECONDARY] &&
          dirship->sectype != NONE) {
        sprintf(buf, "/%d%c", dirship->secondary,
                (dirship->sectype == LIGHT
                     ? 'L'
                     : dirship->sectype == MEDIUM
                           ? 'M'
                           : dirship->sectype == HEAVY ? 'H' : 'N'));
        notify(Playernum, Governor, buf);
      }
      notify(Playernum, Governor, "\n");
      sprintf(buf, "Ship:  %-16.16s\tCrew:     %4d",
              Shipnames[dirship->build_type], dirship->max_crew);
      notify(Playernum, Governor, buf);
      if (Shipdata[dirship->build_type][ABIL_MOUNT]) {
        sprintf(buf, "\t\tXtal Mount: %s\n", (dirship->mount ? "yes" : "no"));
        notify(Playernum, Governor, buf);
      } else
        notify(Playernum, Governor, "\n");
      sprintf(buf, "Class: %s\t\tFuel:     %4d", dirship->class,
              dirship->max_fuel);
      notify(Playernum, Governor, buf);
      if (Shipdata[dirship->build_type][ABIL_JUMP]) {
        sprintf(buf, "\t\tHyperdrive: %s\n",
                (dirship->hyper_drive.has ? "yes" : "no"));
        notify(Playernum, Governor, buf);
      } else
        notify(Playernum, Governor, "\n");
      sprintf(buf, "Cost:  %d r\t\tCargo:    %4d", dirship->build_cost,
              dirship->max_resource);
      notify(Playernum, Governor, buf);
      if (Shipdata[dirship->build_type][ABIL_LASER]) {
        sprintf(buf, "\t\tCombat Lasers: %s\n",
                (dirship->laser ? "yes" : "no"));
        notify(Playernum, Governor, buf);
      } else
        notify(Playernum, Governor, "\n");
      sprintf(buf, "Mass:  %.1f\t\tHanger:   %4u", dirship->base_mass,
              dirship->max_hanger);
      notify(Playernum, Governor, buf);
      if (Shipdata[dirship->build_type][ABIL_CEW]) {
        sprintf(buf, "\t\tCEW: %s\n", (dirship->cew ? "yes" : "no"));
        notify(Playernum, Governor, buf);
      } else
        notify(Playernum, Governor, "\n");
      sprintf(buf, "Size:  %-6d\t\tDestruct: %4d", dirship->size,
              dirship->max_destruct);
      notify(Playernum, Governor, buf);
      if (Shipdata[dirship->build_type][ABIL_CEW] && dirship->cew) {
        sprintf(buf, "\t\t   Opt Range: %4d\n", dirship->cew_range);
        notify(Playernum, Governor, buf);
      } else
        notify(Playernum, Governor, "\n");
      sprintf(buf, "Tech:  %.1f (%.1f)\tSpeed:    %4d", dirship->complexity,
              Race->tech, dirship->max_speed);
      notify(Playernum, Governor, buf);
      if (Shipdata[dirship->build_type][ABIL_CEW] && dirship->cew) {
        sprintf(buf, "\t\t   Energy:    %4d\n", dirship->cew);
        notify(Playernum, Governor, buf);
      } else
        notify(Playernum, Governor, "\n");

      if (Race->tech < dirship->complexity)
        notify(Playernum, Governor, "Your engineering capability is not "
                                    "advanced enough to produce this "
                                    "design.\n");
      free(dirship);
      return;
    }

    shipc = args[1][0];

    i = 0;
    while ((Shipltrs[i] != shipc) && (i < NUMSTYPES))
      i++;

    if ((i >= NUMSTYPES) || ((i == STYPE_POD) && (!Race->pods))) {
      sprintf(buf, "Illegal ship letter.\n");
      notify(Playernum, Governor, buf);
      free(dirship);
      return;
    }
    if (Shipdata[i][ABIL_GOD] && !Race->God) {
      notify(Playernum, Governor, "Nice try!\n");
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
    dirship->guns = NONE; /* this keeps track of the factory status! */
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

    sprintf(dirship->class, "mod %d", Dir[Playernum - 1][Governor].shipno);

    sprintf(buf, "Factory designated to produce %ss.\n", Shipnames[i]);
    notify(Playernum, Governor, buf);
    sprintf(buf, "Design complexity %.1f (%.1f).\n", dirship->complexity,
            Race->tech);
    notify(Playernum, Governor, buf);
    if (dirship->complexity > Race->tech)
      notify(Playernum, Governor, "You can't produce this design yet!\n");

  } else if (mode == 1) {

    if (!dirship->build_type) {
      notify(Playernum, Governor,
             "No ship design specified. Use 'make <ship type>' first.\n");
      free(dirship);
      return;
    }

    if (argn < 2) {
      notify(Playernum, Governor,
             "You have to specify the characteristic you wish to modify.\n");
      free(dirship);
      return;
    }

    if (argn == 3)
      sscanf(args[2], "%d", &value);
    else
      value = 0;

    if (value < 0) {
      notify(Playernum, Governor, "That's a ridiculous setting.\n");
      free(dirship);
      return;
    }

    if (Shipdata[dirship->build_type][ABIL_MOD]) {

      if (match(args[1], "armor")) {
        dirship->armor = MIN(value, 100);
      } else if (match(args[1], "crew") &&
                 Shipdata[dirship->build_type][ABIL_MAXCREW]) {
        dirship->max_crew = MIN(value, 10000);
      } else if (match(args[1], "cargo") &&
                 Shipdata[dirship->build_type][ABIL_CARGO]) {
        dirship->max_resource = MIN(value, 10000);
      } else if (match(args[1], "hanger") &&
                 Shipdata[dirship->build_type][ABIL_HANGER]) {
        dirship->max_hanger = MIN(value, 10000);
      } else if (match(args[1], "fuel") &&
                 Shipdata[dirship->build_type][ABIL_FUELCAP]) {
        dirship->max_fuel = MIN(value, 10000);
      } else if (match(args[1], "destruct") &&
                 Shipdata[dirship->build_type][ABIL_DESTCAP]) {
        dirship->max_destruct = MIN(value, 10000);
      } else if (match(args[1], "speed") &&
                 Shipdata[dirship->build_type][ABIL_SPEED]) {
        dirship->max_speed = MAX(1, MIN(value, 9));
      } else if (match(args[1], "mount") &&
                 Shipdata[dirship->build_type][ABIL_MOUNT] && Crystal(Race)) {
        dirship->mount = !dirship->mount;
      } else if (match(args[1], "hyperdrive") &&
                 Shipdata[dirship->build_type][ABIL_JUMP] &&
                 Hyper_drive(Race)) {
        dirship->hyper_drive.has = !dirship->hyper_drive.has;
      } else if (match(args[1], "primary") &&
                 Shipdata[dirship->build_type][ABIL_PRIMARY]) {
        if (match(args[2], "strength")) {
          dirship->primary = atoi(args[3]);
        } else if (match(args[2], "caliber")) {
          if (match(args[3], "light"))
            dirship->primtype = LIGHT;
          else if (match(args[3], "medium"))
            dirship->primtype = MEDIUM;
          else if (match(args[3], "heavy"))
            dirship->primtype = HEAVY;
          else {
            notify(Playernum, Governor, "No such caliber.\n");
            free(dirship);
            return;
          }
          dirship->primtype = MIN(Shipdata[dirship->build_type][ABIL_PRIMARY],
                                  dirship->primtype);
        } else {
          notify(Playernum, Governor, "No such gun characteristic.\n");
          free(dirship);
          return;
        }
      } else if (match(args[1], "secondary") &&
                 Shipdata[dirship->build_type][ABIL_SECONDARY]) {
        if (match(args[2], "strength")) {
          dirship->secondary = atoi(args[3]);
        } else if (match(args[2], "caliber")) {
          if (match(args[3], "light"))
            dirship->sectype = LIGHT;
          else if (match(args[3], "medium"))
            dirship->sectype = MEDIUM;
          else if (match(args[3], "heavy"))
            dirship->sectype = HEAVY;
          else {
            notify(Playernum, Governor, "No such caliber.\n");
            free(dirship);
            return;
          }
          dirship->sectype = MIN(Shipdata[dirship->build_type][ABIL_SECONDARY],
                                 dirship->sectype);
        } else {
          notify(Playernum, Governor, "No such gun characteristic.\n");
          free(dirship);
          return;
        }
      } else if (match(args[1], "cew") &&
                 Shipdata[dirship->build_type][ABIL_CEW]) {
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
        value = atoi(args[3]);
        if (match(args[2], "strength")) {
          dirship->cew = value;
        } else if (match(args[2], "range")) {
          dirship->cew_range = value;
        } else {
          notify(Playernum, Governor, "No such option for CEWs.\n");
          free(dirship);
          return;
        }
      } else if (match(args[1], "laser") &&
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
      if (match(args[1], "hyperdrive")) {
        dirship->hyper_drive.has = !dirship->hyper_drive.has;
      } else {
        notify(Playernum, Governor, "You may only modify hyperdrive "
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
    notify(Playernum, Governor, "Weird error.\n");
    free(dirship);
    return;
  }
  /* compute how much it's going to cost to build the ship */

  if ((cost0 = cost(dirship)) > 65535.0) {
    notify(Playernum, Governor, "Woah!! YOU CHEATER!!!  The max cost allowed "
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

void build(int Playernum, int Governor, int APcount) {
  racetype *Race;
  char c;
  int i, j, m, n, x, y, count, level, what, outside;
  int shipcost, load_crew;
  int snum, pnum, build_level;
  double load_fuel, tech;

  FILE *fd;
  planettype *planet;
  sectortype *sector;
  shiptype *builder;
  shiptype newship;

  if (argn > 1 && args[1][0] == '?') {
    /* information request */
    if (argn == 2) {
      /* Ship parameter list */
      notify(Playernum, Governor, "     - Default ship parameters -\n");
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
              sprintf(buf, "%1c %-15.15s %5ld %5ld %3ld %4ld %3ld %3ld %3ld "
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
      while (Shipltrs[i] != args[2][0] && i < NUMSTYPES)
        i++;
      if (i < 0 || i >= NUMSTYPES)
        notify(Playernum, Governor, "No such ship type.\n");
      else if (!Shipdata[i][ABIL_PROGRAMMED])
        notify(Playernum, Governor,
               "This ship type has not been programmed.\n");
      else {
        if ((fd = fopen(EXAM_FL, "r")) == NULL) {
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
            if (Shipdata[i][ABIL_BUILD] & Shipdata[j][ABIL_CONSTRUCT])
              n++;
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
          sprintf(temp, "%1c %-15.15s %5ld %5ld %3ld %4ld %3ld %3ld %3ld %4ld "
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

  level = Dir[Playernum - 1][Governor].level;
  if (level != LEVEL_SHIP && level != LEVEL_PLAN) {
    notify(Playernum, Governor,
           "You must change scope to a ship or planet to build.\n");
    return;
  }
  snum = Dir[Playernum - 1][Governor].snum;
  pnum = Dir[Playernum - 1][Governor].pnum;
  Race = races[Playernum - 1];
  count = 0; /* this used used to reset count in the loop */
  do {
    switch (level) {
    case LEVEL_PLAN:
      if (!count) { /* initialize loop variables */
        if (argn < 2) {
          notify(Playernum, Governor, "Build what?\n");
          return;
        }
        if ((what = get_build_type(args[1])) < 0) {
          notify(Playernum, Governor, "No such ship type.\n");
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
        if (argn < 3) {
          notify(Playernum, Governor, "Build where?\n");
          return;
        }
        getplanet(&planet, snum, pnum);
        if (!can_build_at_planet(Playernum, Governor, Stars[snum], planet) &&
            !Race->God) {
          notify(Playernum, Governor, "You can't build that here.\n");
          free(planet);
          return;
        }
        sscanf(args[2], "%d,%d", &x, &y);
        if (x < 0 || x >= planet->Maxx || y < 0 || y >= planet->Maxy) {
          notify(Playernum, Governor, "Illegal sector.\n");
          free(planet);
          return;
        }
        getsector(&sector, planet, x, y);
        if (!can_build_on_sector(what, Race, planet, sector, x, y, buf) &&
            !Race->God) {
          notify(Playernum, Governor, buf);
          free(planet);
          free(sector);
          return;
        }
        if (!(count = getcount(argn < 4, args[3]))) {
          notify(Playernum, Governor, "Give a positive number of builds.\n");
          free(planet);
          free(sector);
          return;
        }
        Getship(&newship, what, Race);
      }
      if ((shipcost = newship.build_cost) >
          planet->info[Playernum - 1].resource) {
        sprintf(buf, "You need %dr to construct this ship.\n", shipcost);
        notify(Playernum, Governor, buf);
        goto finish;
      }
      create_ship_by_planet(Playernum, Governor, Race, &newship, planet, snum,
                            pnum, x, y);
      if (Race->governor[Governor].toggle.autoload && what != OTYPE_TRANSDEV &&
          !Race->God)
        autoload_at_planet(Playernum, &newship, planet, sector, &load_crew,
                           &load_fuel);
      else {
        load_crew = 0;
        load_fuel = 0.0;
      }
      initialize_new_ship(Playernum, Governor, Race, &newship, load_fuel,
                          load_crew);
      putship(&newship);
      break;
    case LEVEL_SHIP:
      if (!count) { /* initialize loop variables */
        (void)getship(&builder, Dir[Playernum - 1][Governor].shipno);
        outside = 0;
        if ((build_level = build_at_ship(Playernum, Governor, Race, builder,
                                         &snum, &pnum)) < 0) {
          notify(Playernum, Governor, "You can't build here.\n");
          free(builder);
          return;
        }
        switch (builder->type) {
        case OTYPE_FACTORY:
          if (!(count = getcount(argn < 2, args[1]))) {
            notify(Playernum, Governor, "Give a positive number of builds.\n");
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
        default:
          if (argn < 2) {
            notify(Playernum, Governor, "Build what?\n");
            free(builder);
            return;
          }
          if ((what = get_build_type(args[1])) < 0) {
            notify(Playernum, Governor, "No such ship type.\n");
            free(builder);
            return;
          }
          if (!can_build_on_ship(what, Race, builder, buf)) {
            notify(Playernum, Governor, buf);
            free(builder);
            return;
          }
          if (!(count = getcount(argn < 3, args[2]))) {
            notify(Playernum, Governor, "Give a positive number of builds.\n");
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
          sprintf(buf, "You are not advanced enough to build this ship.\n%.1f "
                       "enginering technology needed. You have %.1f.\n",
                  tech, Race->tech);
          notify(Playernum, Governor, buf);
          free(builder);
          return;
        }
        if (outside && build_level == LEVEL_PLAN) {
          getplanet(&planet, snum, pnum);
          if (builder->type == OTYPE_FACTORY) {
            if (!can_build_at_planet(Playernum, Governor, Stars[snum],
                                     planet)) {
              notify(Playernum, Governor, "You can't build that here.\n");
              free(planet);
              free(builder);
              return;
            }
            x = builder->land_x;
            y = builder->land_y;
            what = builder->build_type;
            getsector(&sector, planet, x, y);
            if (!can_build_on_sector(what, Race, planet, sector, x, y, buf)) {
              notify(Playernum, Governor, buf);
              free(planet);
              free(sector);
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
            planet->info[Playernum - 1].resource) {
          sprintf(buf, "You need %dr to construct this ship.\n", shipcost);
          notify(Playernum, Governor, buf);
          goto finish;
        }
        create_ship_by_planet(Playernum, Governor, Race, &newship, planet, snum,
                              pnum, x, y);
        if (Race->governor[Governor].toggle.autoload &&
            what != OTYPE_TRANSDEV && !Race->God) {
          autoload_at_planet(Playernum, &newship, planet, sector, &load_crew,
                             &load_fuel);
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
        create_ship_by_ship(Playernum, Governor, Race, 1,
                            Stars[builder->storbits], planet, &newship,
                            builder);
        if (Race->governor[Governor].toggle.autoload &&
            what != OTYPE_TRANSDEV && !Race->God)
          autoload_at_ship(Playernum, &newship, builder, &load_crew,
                           &load_fuel);
        else {
          load_crew = 0;
          load_fuel = 0.0;
        }
        break;
      default:
        if (builder->hanger + ship_size(&newship) > builder->max_hanger) {
          notify(Playernum, Governor, "Not enough hanger space.\n");
          goto finish;
        }
        if (builder->resource < (shipcost = newship.build_cost)) {
          sprintf(buf, "You need %dr to construct the ship.\n", shipcost);
          notify(Playernum, Governor, buf);
          goto finish;
        }
        create_ship_by_ship(Playernum, Governor, Race, 0, NULL, NULL, &newship,
                            builder);
        if (Race->governor[Governor].toggle.autoload &&
            what != OTYPE_TRANSDEV && !Race->God)
          autoload_at_ship(Playernum, &newship, builder, &load_crew,
                           &load_fuel);
        else {
          load_crew = 0;
          load_fuel = 0.0;
        }
        break;
      }
      initialize_new_ship(Playernum, Governor, Race, &newship, load_fuel,
                          load_crew);
      putship(&newship);
      break;
    }
    count--;
  } while (count);
/* free stuff */
finish:
  switch (level) {
  case LEVEL_PLAN:
    putsector(sector, planet, x, y);
    putplanet(planet, snum, pnum);
    free(sector);
    free(planet);
    break;
  case LEVEL_SHIP:
    if (outside)
      switch (build_level) {
      case LEVEL_PLAN:
        putplanet(planet, snum, pnum);
        if (landed(builder)) {
          putsector(sector, planet, x, y);
          free(sector);
        }
        free(planet);
        break;
      case LEVEL_STAR:
        putstar(Stars[snum], snum);
        break;
      case LEVEL_UNIV:
        putsdata(&Sdata);
        break;
      }
    putship(builder);
    free(builder);
    break;
  }
}

int getcount(int mode, char *string) {
  int count;

  if (mode)
    count = 1;
  else
    count = atoi(string);
  if (count <= 0)
    count = 0;

  return (count);
}

int can_build_at_planet(int Playernum, int Governor, startype *star,
                        planettype *planet) {
  if (planet->slaved_to && planet->slaved_to != Playernum) {
    sprintf(buf, "This planet is enslaved by player %d.\n", planet->slaved_to);
    notify(Playernum, Governor, buf);
    return (0);
  }
  if (Governor && star->governor[Playernum - 1] != Governor) {
    notify(Playernum, Governor, "You are not authorized in this system.\n");
    return (0);
  }
  return (1);
}

int get_build_type(char *string) {
  char shipc;
  reg int i;

  shipc = string[0];
  i = -1;
  while (Shipltrs[i] != shipc && i < NUMSTYPES)
    i++;
  if (i < 0 || i >= NUMSTYPES)
    return (-1);
  return i;
}

int can_build_this(int what, racetype *Race, char *string) {
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
    sprintf(string, "You are not advanced enough to build this ship.\n%.1f "
                    "enginering technology needed. You have %.1f.\n",
            (double)Shipdata[what][ABIL_TECH], Race->tech);
    return (0);
  }
  return 1;
}

int can_build_on_ship(int what, racetype *Race, shiptype *builder,
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

int can_build_on_sector(int what, racetype *Race, planettype *planet,
                        sectortype *sector, int x, int y, char *string) {
  shiptype *s;
  char shipc;

  shipc = Shipltrs[what];
  if (!sector->popn) {
    sprintf(string, "You have no more civs in the sector!\n");
    return (0);
  }
  if (sector->condition == WASTED) {
    sprintf(string, "You can't build on wasted sectors.\n");
    return (0);
  }
  if (sector->owner != Race->Playernum && !Race->God) {
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
    reg int sh;
    sh = planet->ships;
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

int build_at_ship(int Playernum, int Governor, racetype *Race,
                  shiptype *builder, int *snum, int *pnum) {
  if (testship(Playernum, Governor, builder))
    return (-1);
  if (!Shipdata[builder->type][ABIL_CONSTRUCT]) {
    notify(Playernum, Governor, "This ship cannot construct other ships.\n");
    return (-1);
  }
  if (!builder->popn) {
    notify(Playernum, Governor, "This ship has no crew.\n");
    return (-1);
  }
  if (docked(builder)) {
    notify(Playernum, Governor, "Undock this ship first.\n");
    return (-1);
  }
  if (builder->damage) {
    notify(Playernum, Governor, "This ship is damaged and cannot build.\n");
    return (-1);
  }
  if (builder->type == OTYPE_FACTORY && !builder->on) {
    notify(Playernum, Governor, "This factory is not online.\n");
    return (-1);
  }
  if (builder->type == OTYPE_FACTORY && !landed(builder)) {
    notify(Playernum, Governor, "Factories must be landed on a planet.\n");
    return (-1);
  }
  *snum = builder->storbits;
  *pnum = builder->pnumorbits;
  return (builder->whatorbits);
}

void autoload_at_planet(int Playernum, shiptype *s, planettype *planet,
                        sectortype *sector, int *crew, double *fuel) {
  *crew = MIN(s->max_crew, sector->popn);
  *fuel = MIN((double)s->max_fuel, (double)planet->info[Playernum - 1].fuel);
  sector->popn -= *crew;
  if (!sector->popn && !sector->troops)
    sector->owner = 0;
  planet->info[Playernum - 1].fuel -= (int)(*fuel);
}

void autoload_at_ship(int Playernum, shiptype *s, shiptype *b, int *crew,
                      double *fuel) {
  *crew = MIN(s->max_crew, b->popn);
  *fuel = MIN((double)s->max_fuel, (double)b->fuel);
  b->popn -= *crew;
  b->fuel -= *fuel;
}

void initialize_new_ship(int Playernum, int Governor, racetype *Race,
                         shiptype *newship, double load_fuel, int load_crew) {
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
    notify(Playernum, Governor, "Mine disarmed.\nTrigger radius set at 100.\n");
    break;
  case OTYPE_TRANSDEV:
    newship->special.transport.target = 0;
    newship->on = 0;
    notify(Playernum, Governor, "Receive OFF.  Change with order.\n");
    break;
  case OTYPE_AP:
    notify(Playernum, Governor, "Processor OFF.\n");
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

void create_ship_by_planet(int Playernum, int Governor, racetype *Race,
                           shiptype *newship, planettype *planet, int snum,
                           int pnum, int x, int y) {
  int shipno;

  newship->tech = Race->tech;
  newship->xpos = Stars[snum]->xpos + planet->xpos;
  newship->ypos = Stars[snum]->ypos + planet->ypos;
  newship->land_x = x;
  newship->land_y = y;
  sprintf(newship->class,
          (((newship->type == OTYPE_TERRA) || (newship->type == OTYPE_PLOW))
               ? "5"
               : "Standard"));
  newship->whatorbits = LEVEL_PLAN;
  newship->whatdest = LEVEL_PLAN;
  newship->deststar = snum;
  newship->destpnum = pnum;
  newship->storbits = snum;
  newship->pnumorbits = pnum;
  newship->docked = 1;
  planet->info[Playernum - 1].resource -= newship->build_cost;
  while ((shipno = getdeadship()) == 0)
    ;
  if (shipno == -1)
    shipno = Numships() + 1;
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
  sprintf(buf, "%s built at a cost of %d resources.\n", Ship(newship),
          newship->build_cost);
  notify(Playernum, Governor, buf);
  sprintf(buf, "Technology %.1f.\n", newship->tech);
  notify(Playernum, Governor, buf);
  sprintf(buf, "%s is on sector %d,%d.\n", Ship(newship), newship->land_x,
          newship->land_y);
  notify(Playernum, Governor, buf);
}

void create_ship_by_ship(int Playernum, int Governor, racetype *Race,
                         int outside, startype *star, planettype *planet,
                         shiptype *newship, shiptype *builder) {
  int shipno;

  while ((shipno = getdeadship()) == 0)
    ;
  if (shipno == -1)
    shipno = Numships() + 1;
  newship->number = shipno;
  newship->owner = Playernum;
  newship->governor = Governor;
  if (outside) {
    newship->whatorbits = builder->whatorbits;
    newship->whatdest = LEVEL_UNIV;
    newship->deststar = builder->deststar;
    newship->destpnum = builder->destpnum;
    newship->storbits = builder->storbits;
    newship->pnumorbits = builder->pnumorbits;
    newship->docked = 0;
    switch (builder->whatorbits) {
    case LEVEL_PLAN:
      insert_sh_plan(planet, newship);
      break;
    case LEVEL_STAR:
      insert_sh_star(Stars[builder->storbits], newship);
      break;
    case LEVEL_UNIV:
      insert_sh_univ(&Sdata, newship);
      break;
    }
  } else {
    newship->whatorbits = LEVEL_SHIP;
    newship->whatdest = LEVEL_SHIP;
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
  sprintf(newship->class,
          (((newship->type == OTYPE_TERRA) || (newship->type == OTYPE_PLOW))
               ? "5"
               : "Standard"));
  builder->resource -= newship->build_cost;

  sprintf(buf, "%s built at a cost of %d resources.\n", Ship(newship),
          newship->build_cost);
  notify(Playernum, Governor, buf);
  sprintf(buf, "Technology %.1f.\n", newship->tech);
  notify(Playernum, Governor, buf);
}

double getmass(shiptype *s) {
  return (1.0 + MASS_ARMOR * s->armor + MASS_SIZE * (s->size - s->max_hanger) +
          MASS_HANGER * s->max_hanger + MASS_GUNS * s->primary * s->primtype +
          MASS_GUNS * s->secondary * s->sectype);
}

int ship_size(shiptype *s) {
  double size;
  size = 1.0 + SIZE_GUNS * s->primary + SIZE_GUNS * s->secondary +
         SIZE_CREW * s->max_crew + SIZE_RESOURCE * s->max_resource +
         SIZE_FUEL * s->max_fuel + SIZE_DESTRUCT * s->max_destruct +
         s->max_hanger;
  return ((int)size);
}

double cost(shiptype *s) {
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

void system_cost(double *advantage, double *disadvantage, int value, int base) {
  double factor;

  factor = (((double)value + 1.0) / (base + 1.0)) - 1.0;
  if (factor >= 0.0)
    *advantage += factor;
  else
    *disadvantage -= factor;
}

double complexity(shiptype *s) {
  int i;
  double advantage, disadvantage, factor, temp;

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

  factor = sqrt((1.0 + advantage) * exp(-(double)disadvantage / 10.0));
  temp = COMPLEXITY_FACTOR * (factor - 1.0) /
             sqrt((double)(Shipdata[i][ABIL_TECH] + 1)) +
         1.0;
  factor = temp * temp;
  return (factor * (double)Shipdata[i][ABIL_TECH]);
}

void Getship(shiptype *s, int i, racetype *r) {
  bzero((char *)s, sizeof(shiptype));
  s->type = i;
  s->armor = Shipdata[i][ABIL_ARMOR];
  s->guns = Shipdata[i][ABIL_PRIMARY] ? PRIMARY : NONE;
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

void Getfactship(shiptype *s, shiptype *b) {
  bzero((char *)s, sizeof(shiptype));
  s->type = b->build_type;
  s->armor = b->armor;
  s->primary = b->primary;
  s->primtype = b->primtype;
  s->secondary = b->secondary;
  s->sectype = b->sectype;
  s->guns = s->primary ? PRIMARY : NONE;
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
  shiptype s;

  Getship(&s, i, r);
  return ((int)cost(&s));
}

#ifdef MARKET
char *Commod[] = { "resources", "destruct", "fuel", "crystals" };

void sell(int Playernum, int Governor, int APcount) {
  racetype *Race;
  planettype *p;
  shiptype *s;
  commodtype c;
  int commodno, amount, item, ok = 0, sh;
  char commod;
  int snum, pnum;
  reg int i;

  if (Dir[Playernum - 1][Governor].level != LEVEL_PLAN) {
    notify(Playernum, Governor, "You have to be in a planet scope to sell.\n");
    return;
  }
  snum = Dir[Playernum - 1][Governor].snum;
  pnum = Dir[Playernum - 1][Governor].pnum;
  if (argn < 3) {
    notify(Playernum, Governor, "Syntax: sell <commodity> <amount>\n");
    return;
  }
  if (Governor && Stars[snum]->governor[Playernum - 1] != Governor) {
    notify(Playernum, Governor, "You are not authorized in this system.\n");
    return;
  }
  Race = races[Playernum - 1];
  if (Race->Guest) {
    notify(Playernum, Governor, "Guest races can't sell anything.\n");
    return;
  }
  /* get information on sale */
  commod = args[1][0];
  amount = atoi(args[2]);
  if (amount <= 0) {
    notify(Playernum, Governor, "Try using positive values.\n");
    return;
  }
  APcount = MIN(APcount, amount);
  if (!enufAP(Playernum, Governor, Stars[snum]->AP[Playernum - 1], APcount))
    return;
  getplanet(&p, snum, pnum);

  if (p->slaved_to && p->slaved_to != Playernum) {
    sprintf(buf, "This planet is enslaved to player %d.\n", p->slaved_to);
    notify(Playernum, Governor, buf);
    free(p);
    return;
  }
  /* check to see if there is an undamage gov center or space port here */
  sh = p->ships;
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
    free(p);
    return;
  }
  switch (commod) {
  case 'r':
    if (!p->info[Playernum - 1].resource) {
      notify(Playernum, Governor,
             "You don't have any resources here to sell!\n");
      free(p);
      return;
    }
    amount = MIN(amount, p->info[Playernum - 1].resource);
    p->info[Playernum - 1].resource -= amount;
    item = RESOURCE;
    break;
  case 'd':
    if (!p->info[Playernum - 1].destruct) {
      notify(Playernum, Governor,
             "You don't have any destruct here to sell!\n");
      free(p);
      return;
    }
    amount = MIN(amount, p->info[Playernum - 1].destruct);
    p->info[Playernum - 1].destruct -= amount;
    item = DESTRUCT;
    break;
  case 'f':
    if (!p->info[Playernum - 1].fuel) {
      notify(Playernum, Governor, "You don't have any fuel here to sell!\n");
      free(p);
      return;
    }
    amount = MIN(amount, p->info[Playernum - 1].fuel);
    p->info[Playernum - 1].fuel -= amount;
    item = FUEL;
    break;
  case 'x':
    if (!p->info[Playernum - 1].crystals) {
      notify(Playernum, Governor,
             "You don't have any crystals here to sell!\n");
      free(p);
      return;
    }
    amount = MIN(amount, p->info[Playernum - 1].crystals);
    p->info[Playernum - 1].crystals -= amount;
    item = CRYSTAL;
    break;
  default:
    notify(Playernum, Governor, "Permitted commodities are r, d, f, and x.\n");
    free(p);
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

  if (commodno == -1)
    commodno = Numcommods() + 1;
  sprintf(buf, "Lot #%d - %d units of %s.\n", commodno, amount, Commod[item]);
  notify(Playernum, Governor, buf);
  sprintf(buf, "Lot #%d - %d units of %s for sale by %s [%d].\n", commodno,
          amount, Commod[item], races[Playernum - 1]->name, Playernum);
  post(buf, TRANSFER);
  for (i = 1; i <= Num_races; i++)
    notify_race(i, buf);
  putcommod(&c, commodno);
  putplanet(p, snum, pnum);
  free(p);
  deductAPs(Playernum, Governor, APcount, snum, 0);
}

void bid(int Playernum, int Governor, int APcount) {
  racetype *Race;
  planettype *p;
  commodtype *c;
  shiptype *s;
  char commod;
  int i, item, bid0, lot, shipping, ok = 0, sh;
  int minbid;
  double dist, rate;
  int snum, pnum;

  if (argn == 1) {
    /* list all market blocks for sale */
    notify(Playernum, Governor,
           "+++ Galactic Bloodshed Commodities Market +++\n\n");
    notify(Playernum, Governor, "  Lot Stock      Type  Owner  Bidder  Amount "
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
        sprintf(buf, " %4d%c%5d%10s%7d%8d%8ld%10.2f%8d %10s\n", i,
                c->deliver ? '*' : ' ', c->amount, Commod[c->type], c->owner,
                c->bidder, c->bid, rate,
                shipping_cost((int)c->star_from,
                              (int)Dir[Playernum - 1][Governor].snum, &dist,
                              (int)c->bid),
                temp);
        notify(Playernum, Governor, buf);
      }
      free(c);
    }
  } else if (argn == 2) {
    /* list all market blocks for sale of the requested type */
    commod = args[1][0];
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
      notify(Playernum, Governor, "No such type of commodity.\n");
      return;
    }
    notify(Playernum, Governor,
           "+++ Galactic Bloodshed Commodities Market +++\n\n");
    notify(Playernum, Governor, "  Lot Stock      Type  Owner  Bidder  Amount "
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
        sprintf(buf, " %4d%c%5d%10s%7d%8d%8ld%10.2f%8d %10s\n", i,
                c->deliver ? '*' : ' ', c->amount, Commod[c->type], c->owner,
                c->bidder, c->bid, rate,
                shipping_cost((int)c->star_from,
                              (int)Dir[Playernum - 1][Governor].snum, &dist,
                              (int)c->bid),
                temp);
        notify(Playernum, Governor, buf);
      }
      free(c);
    }
  } else {
    if (Dir[Playernum - 1][Governor].level != LEVEL_PLAN) {
      notify(Playernum, Governor, "You have to be in a planet scope to buy.\n");
      return;
    }
    snum = Dir[Playernum - 1][Governor].snum;
    pnum = Dir[Playernum - 1][Governor].pnum;
    if (Governor && Stars[snum]->governor[Playernum - 1] != Governor) {
      notify(Playernum, Governor, "You are not authorized in this system.\n");
      return;
    }
    getplanet(&p, snum, pnum);

    if (p->slaved_to && p->slaved_to != Playernum) {
      sprintf(buf, "This planet is enslaved to player %d.\n", p->slaved_to);
      notify(Playernum, Governor, buf);
      free(p);
      return;
    }
    /* check to see if there is an undamaged gov center or space port here */
    sh = p->ships;
    while (sh && !ok) {
      (void)getship(&s, sh);
      if (s->alive && (s->owner == Playernum) && !s->damage &&
          Shipdata[s->type][ABIL_PORT])
        ok = 1;
      sh = s->nextship;
      free(s);
    }
    if (!ok) {
      notify(Playernum, Governor, "You don't have an undamaged space port or "
                                  "government center here.\n");
      free(p);
      return;
    }

    lot = atoi(args[1]);
    bid0 = atoi(args[2]);
    if ((lot <= 0) || lot > Numcommods()) {
      notify(Playernum, Governor, "Illegal lot number.\n");
      free(p);
      return;
    }
    getcommod(&c, lot);
    if (!c->owner) {
      notify(Playernum, Governor, "No such lot for sale.\n");
      free(p);
      free(c);
      return;
    }
    if (c->owner == Playernum &&
        (c->star_from != Dir[Playernum - 1][c->governor].snum ||
         c->planet_from != Dir[Playernum - 1][c->governor].pnum)) {
      notify(Playernum, Governor, "You can only set a minimum price for your "
                                  "lot from the location it was sold.\n");
      free(p);
      free(c);
      return;
    }
    minbid = (int)((double)c->bid * (1.0 + UP_BID));
    if (bid0 < minbid) {
      sprintf(buf, "You have to bid more than %d.\n", minbid);
      notify(Playernum, Governor, buf);
      free(p);
      free(c);
      return;
    }
    Race = races[Playernum - 1];
    if (Race->Guest) {
      notify(Playernum, Governor, "Guest races cannot bid.\n");
      free(p);
      free(c);
      return;
    }
    if (bid0 > Race->governor[Governor].money) {
      notify(Playernum, Governor, "Sorry, no buying on credit allowed.\n");
      free(p);
      free(c);
      return;
    }
    /* notify the previous bidder that he was just outbidded */
    if (c->bidder) {
      sprintf(buf,
              "The bid on lot #%d (%d %s) has been upped to %d by %s [%d].\n",
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
    notify(Playernum, Governor, "Bid accepted.\n");
    free(p);
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
