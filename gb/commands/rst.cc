// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*
 *  ship -- report -- stock -- tactical -- stuff on ship
 *
 *  Command "factories" programmed by varneyml@gb.erc.clarkson.edu
 */

module;

import gblib;
import std.compat;

#include "gb/buffers.h"
#include "gb/order.h"
#include "gb/races.h"
#include "gb/shootblast.h"
#include "gb/tweakables.h"

module commands;

#define PLANET 1

static const char Caliber[] = {' ', 'L', 'M', 'H'};
static char shiplist[256];

static unsigned char Status, SHip, Stock, Report, Weapons, Factories, first;

static bool Tactical;

struct reportdata {
  unsigned char type; /* ship or planet */
  Ship s;
  Planet p;
  shipnum_t n;
  starnum_t star;
  planetnum_t pnum;
  double x;
  double y;
};

using report_array = std::array<char, NUMSTYPES>;

static struct reportdata *rd;
static int enemies_only, who;

static void Free_rlist();
static int Getrship(player_t, governor_t, shipnum_t);
static int listed(int, char *);
static void plan_getrships(player_t, governor_t, starnum_t, planetnum_t);
static void ship_report(GameObj &, shipnum_t, const report_array &);
static void star_getrships(player_t, governor_t, starnum_t);

namespace GB::commands {
void rst(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  shipnum_t shipno;

  report_array Report_types;
  Report_types.fill(1);

  enemies_only = 0;
  Num_ships = 0;
  first = 1;
  if (argv[0] == "report") {
    Report = 1;
    Weapons = Status = Stock = SHip = Tactical = Factories = 0;
  } else if (argv[0] == "stock") {
    Stock = 1;
    Weapons = Status = Report = SHip = Tactical = Factories = 0;
  } else if (argv[0] == "tactical") {
    Tactical = true;
    Weapons = Status = Report = SHip = Stock = Factories = 0;
  } else if (argv[0] == "ship") {
    SHip = Report = Stock = 1;
    Tactical = false;
    Weapons = Status = Factories = 1;
  } else if (argv[0] == "stats") {
    Status = 1;
    Weapons = Report = Stock = Tactical = SHip = Factories = 0;
  } else if (argv[0] == "weapons") {
    Weapons = 1;
    Status = Report = Stock = Tactical = SHip = Factories = 0;
  } else if (argv[0] == "factories") {
    Factories = 1;
    Status = Report = Stock = Tactical = SHip = Weapons = 0;
  }
  shipnum_t n_ships = Numships();
  rd = (struct reportdata *)malloc(sizeof(struct reportdata) *
                                   (n_ships + Sdata.numstars * MAXPLANETS));
  /* (one list entry for each ship, planet in universe) */

  if (argv.size() == 3) {
    if (isdigit(argv[2][0]))
      who = std::stoi(argv[2]);
    else {
      who = 999; /* treat argv[2].c_str() as a list of ship types */
      strcpy(shiplist, argv[2].c_str());
    }
  } else
    who = 0;

  if (argv.size() >= 2) {
    if (*argv[1].c_str() == '#' || isdigit(*argv[1].c_str())) {
      /* report on a couple ships */
      int l = 1;
      while (l < MAXARGS && *argv[l].c_str() != '\0') {
        sscanf(argv[l].c_str() + (*argv[l].c_str() == '#'), "%lu", &shipno);
        if (shipno > n_ships || shipno < 1) {
          sprintf(buf, "rst: no such ship #%lu \n", shipno);
          notify(Playernum, Governor, buf);
          free(rd);
          return;
        }
        (void)Getrship(Playernum, Governor, shipno);
        if (rd[Num_ships - 1].s.whatorbits != ScopeLevel::LEVEL_UNIV) {
          star_getrships(Playernum, Governor, rd[Num_ships - 1].s.storbits);
          ship_report(g, Num_ships - 1, Report_types);
        } else
          ship_report(g, Num_ships - 1, Report_types);
        l++;
      }
      Free_rlist();
      return;
    }
    Report_types.fill(0);

    for (const auto &c : argv[1]) {
      shipnum_t i = NUMSTYPES;
      while (--i && Shipltrs[i] != c);
      if (Shipltrs[i] != c) {
        sprintf(buf, "'%c' -- no such ship letter\n", c);
        notify(Playernum, Governor, buf);
      } else
        Report_types[i] = 1;
    }
  }

  switch (g.level) {
    case ScopeLevel::LEVEL_UNIV:
      if (!(Tactical && argv.size() < 2)) {
        shipnum_t shn = Sdata.ships;
        while (shn && Getrship(Playernum, Governor, shn))
          shn = rd[Num_ships - 1].s.nextship;

        for (starnum_t i = 0; i < Sdata.numstars; i++)
          star_getrships(Playernum, Governor, i);
        for (shipnum_t i = 0; i < Num_ships; i++)
          ship_report(g, i, Report_types);
      } else {
        notify(Playernum, Governor,
               "You can't do tactical option from universe level.\n");
        free(rd); /* nothing allocated */
        return;
      }
      break;
    case ScopeLevel::LEVEL_PLAN:
      plan_getrships(Playernum, Governor, g.snum, g.pnum);
      for (shipnum_t i = 0; i < Num_ships; i++) ship_report(g, i, Report_types);
      break;
    case ScopeLevel::LEVEL_STAR:
      star_getrships(Playernum, Governor, g.snum);
      for (shipnum_t i = 0; i < Num_ships; i++) ship_report(g, i, Report_types);
      break;
    case ScopeLevel::LEVEL_SHIP:
      (void)Getrship(Playernum, Governor, g.shipno);
      ship_report(g, 0, Report_types); /* first ship report */
      shipnum_t shn = rd[0].s.ships;
      Num_ships = 0;

      while (shn && Getrship(Playernum, Governor, shn))
        shn = rd[Num_ships - 1].s.nextship;

      for (shipnum_t i = 0; i < Num_ships; i++) ship_report(g, i, Report_types);
      break;
  }
  Free_rlist();
}
}  // namespace GB::commands

static void ship_report(GameObj &g, shipnum_t indx,
                        const report_array &rep_on) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  int i;
  int sight;
  int caliber;
  char orb[PLACENAMESIZE];
  char strng[COMMANDSIZE];
  char locstrn[COMMANDSIZE];
  char tmpbuf1[10];
  char tmpbuf2[10];
  char tmpbuf3[10];
  char tmpbuf4[10];
  double Dist;

  /* last ship gotten from disk */
  auto &s = rd[indx].s;
  auto &p = rd[indx].p;
  shipnum_t shipno = rd[indx].n;

  /* launched canister, non-owned ships don't show up */
  if ((rd[indx].type == PLANET && p.info[Playernum - 1].numsectsowned) ||
      (rd[indx].type != PLANET && s.alive && s.owner == Playernum &&
       authorized(Governor, s) && rep_on[s.type] &&
       !(s.type == ShipType::OTYPE_CANIST && !s.docked) &&
       !(s.type == ShipType::OTYPE_GREEN && !s.docked))) {
    if (rd[indx].type != PLANET && Stock) {
      if (first) {
        sprintf(buf,
                "    #       name        x  hanger   res        des       "
                "  fuel      crew/mil\n");
        notify(Playernum, Governor, buf);
        if (!SHip) first = 0;
      }
      sprintf(buf,
              "%5lu %c "
              "%14.14s%3u%4u:%-3u%5lu:%-5ld%5u:%-5ld%7.1f:%-6ld%lu/%lu:%d\n",
              shipno, Shipltrs[s.type], (s.active ? s.name : "INACTIVE"),
              s.crystals, s.hanger, s.max_hanger, s.resource, max_resource(s),
              s.destruct, max_destruct(s), s.fuel, max_fuel(s), s.popn,
              s.troops, s.max_crew);
      notify(Playernum, Governor, buf);
    }

    if (rd[indx].type != PLANET && Status) {
      if (first) {
        sprintf(buf,
                "    #       name       las cew hyp    guns   arm tech "
                "spd cost  mass size\n");
        notify(Playernum, Governor, buf);
        if (!SHip) first = 0;
      }
      sprintf(buf,
              "%5lu %c %14.14s %s%s%s%3lu%c/%3lu%c%4lu%5.0f%4lu%5lu%7.1f%4u",
              shipno, Shipltrs[s.type], (s.active ? s.name : "INACTIVE"),
              s.laser ? "yes " : "    ", s.cew ? "yes " : "    ",
              s.hyper_drive.has ? "yes " : "    ", s.primary,
              Caliber[s.primtype], s.secondary, Caliber[s.sectype], armor(s),
              s.tech, max_speed(s), shipcost(s), mass(s), size(s));
      notify(Playernum, Governor, buf);
      if (s.type == ShipType::STYPE_POD) {
        sprintf(buf, " (%d)", s.special.pod.temperature);
        notify(Playernum, Governor, buf);
      }
      g.out << "\n";
    }

    if (rd[indx].type != PLANET && Weapons) {
      if (first) {
        sprintf(buf,
                "    #       name      laser   cew     safe     guns    "
                "damage   class\n");
        notify(Playernum, Governor, buf);
        if (!SHip) first = 0;
      }
      sprintf(
          buf,
          "%5lu %c %14.14s %s  %3d/%-4d  %4d  %3lu%c/%3lu%c    %3d%%  %c %s\n",
          shipno, Shipltrs[s.type], (s.active ? s.name : "INACTIVE"),
          s.laser ? "yes " : "    ", s.cew, s.cew_range,
          (int)((1.0 - .01 * s.damage) * s.tech / 4.0), s.primary,
          Caliber[s.primtype], s.secondary, Caliber[s.sectype], s.damage,
          s.type == ShipType::OTYPE_FACTORY ? Shipltrs[s.build_type] : ' ',
          ((s.type == ShipType::OTYPE_TERRA) ||
           (s.type == ShipType::OTYPE_PLOW))
              ? "Standard"
              : s.shipclass);
      notify(Playernum, Governor, buf);
    }

    if (rd[indx].type != PLANET && Factories &&
        (s.type == ShipType::OTYPE_FACTORY)) {
      if (first) {
        sprintf(buf,
                "   #    Cost Tech Mass Sz A Crw Ful Crg Hng Dst Sp "
                "Weapons Lsr CEWs Range Dmg\n");
        notify(Playernum, Governor, buf);
        if (!SHip) first = 0;
      }
      if ((s.build_type == 0) || (s.build_type == ShipType::OTYPE_FACTORY)) {
        sprintf(buf,
                "%5lu               (No ship type specified yet)           "
                "           75%% (OFF)",
                shipno);
        notify(Playernum, Governor, buf);
      } else {
        if (s.primtype)
          sprintf(tmpbuf1, "%2lu%s", s.primary,
                  s.primtype == GTYPE_LIGHT    ? "L"
                  : s.primtype == GTYPE_MEDIUM ? "M"
                  : s.primtype == GTYPE_HEAVY  ? "H"
                                               : "N");
        else
          strcpy(tmpbuf1, "---");
        if (s.sectype)
          sprintf(tmpbuf2, "%2lu%s", s.secondary,
                  s.sectype == GTYPE_LIGHT    ? "L"
                  : s.sectype == GTYPE_MEDIUM ? "M"
                  : s.sectype == GTYPE_HEAVY  ? "H"
                                              : "N");
        else
          strcpy(tmpbuf2, "---");
        if (s.cew)
          sprintf(tmpbuf3, "%4d", s.cew);
        else
          strcpy(tmpbuf3, "----");
        if (s.cew)
          sprintf(tmpbuf4, "%5d", s.cew_range);
        else
          strcpy(tmpbuf4, "-----");
        sprintf(buf,
                "%5lu %c%4d%6.1f%5.1f%3d%2d%4d%4d%4lu%4d%4d %s%1d %s/%s %s "
                "%s %s %02d%%%s\n",
                shipno, Shipltrs[s.build_type], s.build_cost, s.complexity,
                s.base_mass, ship_size(s), s.armor, s.max_crew, s.max_fuel,
                s.max_resource, s.max_hanger, s.max_destruct,
                s.hyper_drive.has ? (s.mount ? "+" : "*") : " ", s.max_speed,
                tmpbuf1, tmpbuf2, s.laser ? "yes" : " no", tmpbuf3, tmpbuf4,
                s.damage, s.damage ? (s.on ? "" : "*") : "");
        notify(Playernum, Governor, buf);
      }
    }

    if (rd[indx].type != PLANET && Report) {
      if (first) {
        sprintf(buf,
                " #      name       gov dam crew mil  des fuel sp orbits  "
                "   destination\n");
        notify(Playernum, Governor, buf);
        if (!SHip) first = 0;
      }
      if (s.docked)
        if (s.whatdest == ScopeLevel::LEVEL_SHIP)
          sprintf(locstrn, "D#%ld", s.destshipno);
        else
          sprintf(locstrn, "L%2d,%-2d", s.land_x, s.land_y);
      else if (s.navigate.on)
        sprintf(locstrn, "nav:%d (%d)", s.navigate.bearing, s.navigate.turns);
      else
        strcpy(locstrn, prin_ship_dest(s).c_str());

      if (!s.active) {
        sprintf(strng, "INACTIVE(%d)", s.rad);
        notify(Playernum, Governor, buf);
      }

      sprintf(buf,
              "%c%-5lu %12.12s %2d %3u%5lu%4lu%5u%5.0f %c%1u %-10s %-18s\n",
              Shipltrs[s.type], shipno, (s.active ? s.name : strng), s.governor,
              s.damage, s.popn, s.troops, s.destruct, s.fuel,
              s.hyper_drive.has ? (s.mounted ? '+' : '*') : ' ', s.speed,
              dispshiploc_brief(s).c_str(), locstrn);
      notify(Playernum, Governor, buf);
    }

    auto &race = races[Playernum - 1];

    if (Tactical) {
      int fev = 0;
      int fspeed = 0;
      int fdam = 0;
      double tech;

      sprintf(buf,
              "\n  #         name        tech    guns  armor size dest   "
              "fuel dam spd evad               orbits\n");
      notify(Playernum, Governor, buf);

      if (rd[indx].type == PLANET) {
        tech = race.tech;
        /* tac report from planet */
        sprintf(buf, "(planet)%15.15s%4.0f %4dM           %5u %6u\n",
                stars[rd[indx].star].pnames[rd[indx].pnum], tech,
                p.info[Playernum - 1].guns, p.info[Playernum - 1].destruct,
                p.info[Playernum - 1].fuel);
        notify(Playernum, Governor, buf);
        caliber = GTYPE_MEDIUM;
      } else {
        Place where{s.whatorbits, s.storbits, s.pnumorbits};
        tech = s.tech;
        caliber = current_caliber(&s);
        if ((s.whatdest != ScopeLevel::LEVEL_UNIV || s.navigate.on) &&
            !s.docked && s.active) {
          fspeed = s.speed;
          fev = s.protect.evade;
        }
        fdam = s.damage;
        sprintf(orb, "%30.30s", where.to_string().c_str());
        sprintf(buf,
                "%3lu %c %16.16s %4.0f%3lu%c/%3lu%c%6d%5d%5u%7.1f%3d%%  %d  "
                "%3s%21.22s",
                shipno, Shipltrs[s.type], (s.active ? s.name : "INACTIVE"),
                s.tech, s.primary, Caliber[s.primtype], s.secondary,
                Caliber[s.sectype], s.armor, s.size, s.destruct, s.fuel,
                s.damage, fspeed, (fev ? "yes" : "   "), orb);
        notify(Playernum, Governor, buf);
        if (landed(s)) {
          sprintf(buf, " (%d,%d)", s.land_x, s.land_y);
          notify(Playernum, Governor, buf);
        }
        if (!s.active) {
          sprintf(buf, " INACTIVE(%d)", s.rad);
          notify(Playernum, Governor, buf);
        }
        sprintf(buf, "\n");
        notify(Playernum, Governor, buf);
      }

      sight = 0;
      if (rd[indx].type == PLANET)
        sight = 1;
      else if (shipsight(s))
        sight = 1;

      /* tactical display */
      sprintf(buf,
              "\n  Tactical: #  own typ        name   rng   (50%%) size "
              "spd evade hit  dam  loc\n");
      notify(Playernum, Governor, buf);

      if (sight)
        for (i = 0; i < Num_ships; i++) {
          if (i != indx &&
              (Dist = sqrt(Distsq(rd[indx].x, rd[indx].y, rd[i].x, rd[i].y))) <
                  gun_range(&race, &rd[indx].s, (rd[indx].type == PLANET))) {
            if (rd[i].type == PLANET) {
              /* tac report at planet */
              sprintf(buf, " %13s(planet)          %8.0f\n",
                      stars[rd[i].star].pnames[rd[i].pnum], Dist);
              notify(Playernum, Governor, buf);
            } else if (!who || who == rd[i].s.owner ||
                       (who == 999 && listed((int)rd[i].s.type, shiplist))) {
              /* tac report at ship */
              if ((rd[i].s.owner != Playernum ||
                   !authorized(Governor, rd[i].s)) &&
                  rd[i].s.alive && rd[i].s.type != ShipType::OTYPE_CANIST &&
                  rd[i].s.type != ShipType::OTYPE_GREEN) {
                int tev = 0;
                int tspeed = 0;
                int body = 0;
                int prob = 0;
                int factor = 0;
                if ((rd[i].s.whatdest != ScopeLevel::LEVEL_UNIV ||
                     rd[i].s.navigate.on) &&
                    !rd[i].s.docked && rd[i].s.active) {
                  tspeed = rd[i].s.speed;
                  tev = rd[i].s.protect.evade;
                }
                body = size(rd[i].s);
                auto defense = getdefense(rd[i].s);
                prob = hit_odds(Dist, &factor, tech, fdam, fev, tev, fspeed,
                                tspeed, body, caliber, defense);
                if (rd[indx].type != PLANET && laser_on(rd[indx].s) &&
                    rd[indx].s.focus)
                  prob = prob * prob / 100;
                sprintf(buf,
                        "%13lu %s%2d,%1d %c%14.14s %4.0f  %4d   %4d %d  %3s  "
                        "%3d%% %3u%%%s",
                        rd[i].n,
                        (isset(races[Playernum - 1].atwar, rd[i].s.owner)) ? "-"
                        : (isset(races[Playernum - 1].allied, rd[i].s.owner))
                            ? "+"
                            : " ",
                        rd[i].s.owner, rd[i].s.governor, Shipltrs[rd[i].s.type],
                        rd[i].s.name, Dist, factor, body, tspeed,
                        (tev ? "yes" : "   "), prob, rd[i].s.damage,
                        (rd[i].s.active ? "" : " INACTIVE"));
                if ((enemies_only == 0) ||
                    ((enemies_only == 1) &&
                     (!isset(races[Playernum - 1].allied, rd[i].s.owner)))) {
                  notify(Playernum, Governor, buf);
                  if (landed(rd[i].s)) {
                    sprintf(buf, " (%d,%d)", rd[i].s.land_x, rd[i].s.land_y);
                    notify(Playernum, Governor, buf);
                  } else {
                    sprintf(buf, "     ");
                    notify(Playernum, Governor, buf);
                  }
                  sprintf(buf, "\n");
                  notify(Playernum, Governor, buf);
                }
              }
            }
          }
        }
    }
  }
}

static void plan_getrships(player_t Playernum, governor_t Governor,
                           starnum_t snum, planetnum_t pnum) {
  rd[Num_ships].p = getplanet(snum, pnum);
  const auto &p = rd[Num_ships].p;
  /* add this planet into the ship list */
  rd[Num_ships].star = snum;
  rd[Num_ships].pnum = pnum;
  rd[Num_ships].type = PLANET;
  rd[Num_ships].n = 0;
  rd[Num_ships].x = stars[snum].xpos + p.xpos;
  rd[Num_ships].y = stars[snum].ypos + p.ypos;
  Num_ships++;

  if (p.info[Playernum - 1].explored) {
    shipnum_t shn = p.ships;
    while (shn && Getrship(Playernum, Governor, shn))
      shn = rd[Num_ships - 1].s.nextship;
  }
}

static void star_getrships(player_t Playernum, governor_t Governor,
                           starnum_t snum) {
  if (isset(stars[snum].explored, Playernum)) {
    shipnum_t shn = stars[snum].ships;
    while (shn && Getrship(Playernum, Governor, shn))
      shn = rd[Num_ships - 1].s.nextship;
    for (planetnum_t i = 0; i < stars[snum].numplanets; i++)
      plan_getrships(Playernum, Governor, snum, i);
  }
}

/* get a ship from the disk and add it to the ship list we're maintaining. */
static int Getrship(player_t Playernum, governor_t Governor, shipnum_t shipno) {
  auto shiptmp = getship(shipno);
  if (shiptmp) {
    rd[Num_ships].s = *shiptmp;
    rd[Num_ships].type = 0;
    rd[Num_ships].n = shipno;
    rd[Num_ships].x = rd[Num_ships].s.xpos;
    rd[Num_ships].y = rd[Num_ships].s.ypos;
    Num_ships++;
    return 1;
  }
  sprintf(buf, "Getrship: error on ship get (%lu).\n", shipno);
  notify(Playernum, Governor, buf);
  return 0;
}

static void Free_rlist() { free(rd); }

static int listed(int type, char *string) {
  char *p;

  for (p = string; *p; p++) {
    if (Shipltrs[type] == *p) return 1;
  }
  return 0;
}
