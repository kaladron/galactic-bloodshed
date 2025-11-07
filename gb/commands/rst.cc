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

module commands;

enum ReportType {
  SHIP = 0,
  PLANET = 1,
};

static const char Caliber[] = {' ', 'L', 'M', 'H'};
static char shiplist[256];

static bool Status, SHip, Stock, Report, Weapons, Factories, first;

static bool Tactical;

struct reportdata {
  ReportType type; /* ship or planet */
  Ship s;
  Planet p;
  shipnum_t n;
  starnum_t star;
  planetnum_t pnum;
  double x;
  double y;
};

using report_array = std::array<bool, NUMSTYPES>;

static std::vector<reportdata> rd;
static bool enemies_only;
static int who;
static bool Getrship(GameObj&, shipnum_t);
static bool listed(int, char*);
static void plan_getrships(GameObj&, player_t, starnum_t, planetnum_t);
static void ship_report(GameObj &, shipnum_t, const report_array &);
static void star_getrships(GameObj&, player_t, starnum_t);

namespace GB::commands {
void rst(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  shipnum_t shipno;

  report_array Report_types;
  Report_types.fill(true);

  enemies_only = false;
  Num_ships = 0;
  first = true;
  if (argv[0] == "report") {
    Report = true;
    Weapons = Status = Stock = SHip = Factories = false;
    Tactical = false;
  } else if (argv[0] == "stock") {
    Stock = true;
    Weapons = Status = Report = SHip = Factories = false;
    Tactical = false;
  } else if (argv[0] == "tactical") {
    Tactical = true;
    Weapons = Status = Report = SHip = Stock = Factories = false;
  } else if (argv[0] == "ship") {
    SHip = Report = Stock = true;
    Tactical = false;
    Weapons = Status = Factories = true;
  } else if (argv[0] == "stats") {
    Status = true;
    Weapons = Report = Stock = SHip = Factories = false;
    Tactical = false;
  } else if (argv[0] == "weapons") {
    Weapons = true;
    Status = Report = Stock = SHip = Factories = false;
    Tactical = false;
  } else if (argv[0] == "factories") {
    Factories = true;
    Status = Report = Stock = SHip = Weapons = false;
    Tactical = false;
  }
  shipnum_t n_ships = Numships();
  rd.clear();
  rd.reserve(n_ships + Sdata.numstars * MAXPLANETS);
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
          g.out << std::format("rst: no such ship #{} \n", shipno);
          return;
        }
        (void)Getrship(g, shipno);
        if (rd[Num_ships - 1].s.whatorbits != ScopeLevel::LEVEL_UNIV) {
          star_getrships(g, Playernum, rd[Num_ships - 1].s.storbits);
          ship_report(g, Num_ships - 1, Report_types);
        } else
          ship_report(g, Num_ships - 1, Report_types);
        l++;
      }
      return;
    }
    Report_types.fill(false);

    for (const auto &c : argv[1]) {
      shipnum_t i = NUMSTYPES;
      while (--i && Shipltrs[i] != c);
      if (Shipltrs[i] != c) {
        g.out << std::format("'{}' -- no such ship letter\n", c);
      } else
        Report_types[i] = true;
    }
  }

  switch (g.level) {
    case ScopeLevel::LEVEL_UNIV:
      if (!(Tactical && argv.size() < 2)) {
        shipnum_t shn = Sdata.ships;
        while (shn && Getrship(g, shn))
          shn = rd[Num_ships - 1].s.nextship;

        for (starnum_t i = 0; i < Sdata.numstars; i++)
          star_getrships(g, Playernum, i);
        for (shipnum_t i = 0; i < Num_ships; i++)
          ship_report(g, i, Report_types);
      } else {
        g.out << "You can't do tactical option from universe level.\n";
        return;
      }
      break;
    case ScopeLevel::LEVEL_PLAN:
      plan_getrships(g, Playernum, g.snum, g.pnum);
      for (shipnum_t i = 0; i < Num_ships; i++) ship_report(g, i, Report_types);
      break;
    case ScopeLevel::LEVEL_STAR:
      star_getrships(g, Playernum, g.snum);
      for (shipnum_t i = 0; i < Num_ships; i++) ship_report(g, i, Report_types);
      break;
    case ScopeLevel::LEVEL_SHIP:
      (void)Getrship(g, g.shipno);
      ship_report(g, 0, Report_types); /* first ship report */
      shipnum_t shn = rd[0].s.ships;
      Num_ships = 0;

      while (shn && Getrship(g, shn))
        shn = rd[Num_ships - 1].s.nextship;

      for (shipnum_t i = 0; i < Num_ships; i++) ship_report(g, i, Report_types);
      break;
  }
}
}  // namespace GB::commands

static void ship_report(GameObj &g, shipnum_t indx,
                        const report_array &rep_on) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  int i;
  int sight;
  guntype_t caliber;
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
        g.out << "    #       name        x  hanger   res        des       "
                 "  fuel      crew/mil\n";
        if (!SHip) first = false;
      }
      g.out << std::format(
          "{:5} {:c} "
          "{:14.14}{:3}{:4}:{:<3}{:5}:{:<5}{:5}:{:<5}{:7.1f}:{:<6}{}/{}:{}\n",
          shipno, Shipltrs[s.type], (s.active ? s.name : "INACTIVE"),
          s.crystals, s.hanger, s.max_hanger, s.resource, max_resource(s),
          s.destruct, max_destruct(s), s.fuel, max_fuel(s), s.popn,
          s.troops, s.max_crew);
    }

    if (rd[indx].type != PLANET && Status) {
      if (first) {
        g.out << "    #       name       las cew hyp    guns   arm tech "
                 "spd cost  mass size\n";
        if (!SHip) first = false;
      }
      g.out << std::format(
          "{:5} {:c} {:14.14} {}{}{}{:3}{:c}/{:3}{:c}{:4}{:5.0f}{:4}{:5}{:7.1f}{:4}",
          shipno, Shipltrs[s.type], (s.active ? s.name : "INACTIVE"),
          s.laser ? "yes " : "    ", s.cew ? "yes " : "    ",
          s.hyper_drive.has ? "yes " : "    ", s.primary,
          Caliber[s.primtype], s.secondary, Caliber[s.sectype], armor(s),
          s.tech, max_speed(s), shipcost(s), mass(s), size(s));
      if (s.type == ShipType::STYPE_POD) {
        if (std::holds_alternative<PodData>(s.special)) {
          auto pod = std::get<PodData>(s.special);
          g.out << std::format(" ({})", pod.temperature);
        }
      }
      g.out << "\n";
    }

    if (rd[indx].type != PLANET && Weapons) {
      if (first) {
        g.out << "    #       name      laser   cew     safe     guns    "
                 "damage   class\n";
        if (!SHip) first = false;
      }
      g.out << std::format(
          "{:5} {:c} {:14.14} {}  {:3}/{:<4}  {:4}  {:3}{:c}/{:3}{:c}    {:3}%  {:c} {}\n",
          shipno, Shipltrs[s.type], (s.active ? s.name : "INACTIVE"),
          s.laser ? "yes " : "    ", s.cew, s.cew_range,
          (int)((1.0 - .01 * s.damage) * s.tech / 4.0), s.primary,
          Caliber[s.primtype], s.secondary, Caliber[s.sectype], s.damage,
          s.type == ShipType::OTYPE_FACTORY ? Shipltrs[s.build_type] : ' ',
          ((s.type == ShipType::OTYPE_TERRA) ||
           (s.type == ShipType::OTYPE_PLOW))
              ? "Standard"
              : s.shipclass);
    }

    if (rd[indx].type != PLANET && Factories &&
        (s.type == ShipType::OTYPE_FACTORY)) {
      if (first) {
        g.out << "   #    Cost Tech Mass Sz A Crw Ful Crg Hng Dst Sp "
                 "Weapons Lsr CEWs Range Dmg\n";
        if (!SHip) first = false;
      }
      if ((s.build_type == 0) || (s.build_type == ShipType::OTYPE_FACTORY)) {
        g.out << std::format("{:5}               (No ship type specified yet)           "
                            "           75% (OFF)",
                            shipno);
      } else {
        std::string tmpbuf1;
        if (s.primtype) {
          auto caliber = s.primtype == GTYPE_LIGHT ? "L"
                       : s.primtype == GTYPE_MEDIUM ? "M"
                       : s.primtype == GTYPE_HEAVY  ? "H"
                                                    : "N";
          tmpbuf1 = std::format("{:2}{}", s.primary, caliber);
        } else {
          tmpbuf1 = "---";
        }
        
        std::string tmpbuf2;
        if (s.sectype) {
          auto caliber = s.sectype == GTYPE_LIGHT ? "L"
                       : s.sectype == GTYPE_MEDIUM ? "M"
                       : s.sectype == GTYPE_HEAVY  ? "H"
                                                   : "N";
          tmpbuf2 = std::format("{:2}{}", s.secondary, caliber);
        } else {
          tmpbuf2 = "---";
        }
        
        std::string tmpbuf3 = s.cew ? std::format("{:4}", s.cew) : "----";
        std::string tmpbuf4 = s.cew ? std::format("{:5}", s.cew_range) : "-----";
        
        g.out << std::format(
            "{:5} {:c}{:4}{:6.1f}{:5.1f}{:3}{:2}{:4}{:4}{:4}{:4}{:4} {}{:1} {}/{} {} "
            "{} {} {:02}%{}\n",
            shipno, Shipltrs[s.build_type], s.build_cost, s.complexity,
            s.base_mass, ship_size(s), s.armor, s.max_crew, s.max_fuel,
            s.max_resource, s.max_hanger, s.max_destruct,
            s.hyper_drive.has ? (s.mount ? "+" : "*") : " ", s.max_speed,
            tmpbuf1, tmpbuf2, s.laser ? "yes" : " no", tmpbuf3, tmpbuf4,
            s.damage, s.damage ? (s.on ? "" : "*") : "");
      }
    }

    if (rd[indx].type != PLANET && Report) {
      if (first) {
        g.out << " #      name       gov dam crew mil  des fuel sp orbits  "
                 "   destination\n";
        if (!SHip) first = false;
      }
      std::string locstrn;
      if (s.docked) {
        if (s.whatdest == ScopeLevel::LEVEL_SHIP)
          locstrn = std::format("D#{}", s.destshipno);
        else
          locstrn = std::format("L{:2},{:<2}", s.land_x, s.land_y);
      } else if (s.navigate.on) {
        locstrn = std::format("nav:{} ({})", s.navigate.bearing, s.navigate.turns);
      } else {
        locstrn = prin_ship_dest(s);
      }

      std::string strng;
      if (!s.active) {
        strng = std::format("INACTIVE({})", s.rad);
      }

      g.out << std::format("{:c}{:<5} {:12.12} {:2} {:3}{:5}{:4}{:5}{:5.0f} {:c}{:1} {:<10} {:<18}\n",
                          Shipltrs[s.type], shipno, (s.active ? s.name : strng), s.governor,
                          s.damage, s.popn, s.troops, s.destruct, s.fuel,
                          s.hyper_drive.has ? (s.mounted ? '+' : '*') : ' ', s.speed,
                          dispshiploc_brief(s), locstrn);
    }

    auto &race = races[Playernum - 1];

    if (Tactical) {
      int fev = 0;
      int fspeed = 0;
      int fdam = 0;
      double tech;

      g.out << "\n  #         name        tech    guns  armor size dest   "
               "fuel dam spd evad               orbits\n";

      if (rd[indx].type == PLANET) {
        tech = race.tech;
        /* tac report from planet */
        g.out << std::format("(planet){:15.15}{:4.0f} {:4}M           {:5} {:6}\n",
                            stars[rd[indx].star].get_planet_name(rd[indx].pnum),
                            tech, p.info[Playernum - 1].guns,
                            p.info[Playernum - 1].destruct, p.info[Playernum - 1].fuel);
        caliber = GTYPE_MEDIUM;
      } else {
        Place where{s.whatorbits, s.storbits, s.pnumorbits};
        tech = s.tech;
        caliber = current_caliber(s);
        if ((s.whatdest != ScopeLevel::LEVEL_UNIV || s.navigate.on) &&
            !s.docked && s.active) {
          fspeed = s.speed;
          fev = s.protect.evade;
        }
        fdam = s.damage;
        auto orb = std::format("{:30.30}", where.to_string());
        g.out << std::format(
            "{:3} {:c} {:16.16} {:4.0f}{:3}{:c}/{:3}{:c}{:6}{:5}{:5}{:7.1f}{:3}%  {}  "
            "{:3}{:21.22}",
            shipno, Shipltrs[s.type], (s.active ? s.name : "INACTIVE"),
            s.tech, s.primary, Caliber[s.primtype], s.secondary,
            Caliber[s.sectype], s.armor, s.size, s.destruct, s.fuel,
            s.damage, fspeed, (fev ? "yes" : "   "), orb);
        if (landed(s)) {
          g.out << std::format(" ({},{})", s.land_x, s.land_y);
        }
        if (!s.active) {
          g.out << std::format(" INACTIVE({})", s.rad);
        }
        g.out << "\n";
      }

      sight = 0;
      if (rd[indx].type == PLANET)
        sight = 1;
      else if (shipsight(s))
        sight = 1;

      /* tactical display */
      g.out << "\n  Tactical: #  own typ        name   rng   (50%) size "
               "spd evade hit  dam  loc\n";

      if (sight)
        for (i = 0; i < Num_ships; i++) {
          double range =
              rd[indx].type == PLANET ? gun_range(race) : gun_range(rd[indx].s);
          if (i != indx && (Dist = sqrt(Distsq(rd[indx].x, rd[indx].y, rd[i].x,
                                               rd[i].y))) < range) {
            if (rd[i].type == PLANET) {
              /* tac report at planet */
              g.out << std::format(" {:13}(planet)          {:8.0f}\n",
                                  stars[rd[i].star].get_planet_name(rd[i].pnum),
                                  Dist);
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

                if ((rd[i].s.whatdest != ScopeLevel::LEVEL_UNIV ||
                     rd[i].s.navigate.on) &&
                    !rd[i].s.docked && rd[i].s.active) {
                  tspeed = rd[i].s.speed;
                  tev = rd[i].s.protect.evade;
                }
                body = size(rd[i].s);
                auto defense = getdefense(rd[i].s);
                auto [prob, factor] =
                    hit_odds(Dist, tech, fdam, fev, tev, fspeed, tspeed, body,
                             caliber, defense);
                if (rd[indx].type != PLANET && laser_on(rd[indx].s) &&
                    rd[indx].s.focus)
                  prob = prob * prob / 100;
                auto war_status = isset(races[Playernum - 1].atwar, rd[i].s.owner) ? "-"
                                : isset(races[Playernum - 1].allied, rd[i].s.owner) ? "+"
                                                                                     : " ";
                if (!enemies_only ||
                    (enemies_only &&
                     (!isset(races[Playernum - 1].allied, rd[i].s.owner)))) {
                  g.out << std::format(
                      "{:13} {}{:2},{:1} {:c}{:14.14} {:4.0f}  {:4}   {:4} {}  {:3}  "
                      "{:3}% {:3}%{}",
                      rd[i].n, war_status,
                      rd[i].s.owner, rd[i].s.governor, Shipltrs[rd[i].s.type],
                      rd[i].s.name, Dist, factor, body, tspeed,
                      (tev ? "yes" : "   "), prob, rd[i].s.damage,
                      (rd[i].s.active ? "" : " INACTIVE"));
                  if (landed(rd[i].s)) {
                    g.out << std::format(" ({},{})", rd[i].s.land_x, rd[i].s.land_y);
                  } else {
                    g.out << "     ";
                  }
                  g.out << "\n";
                }
              }
            }
          }
        }
    }
  }
}

static void plan_getrships(GameObj& g, player_t Playernum,
                           starnum_t snum, planetnum_t pnum) {
  rd.resize(Num_ships + 1);
  rd[Num_ships].p = getplanet(snum, pnum);
  const auto &p = rd[Num_ships].p;
  /* add this planet into the ship list */
  rd[Num_ships].star = snum;
  rd[Num_ships].pnum = pnum;
  rd[Num_ships].type = PLANET;
  rd[Num_ships].n = 0;
  rd[Num_ships].x = stars[snum].xpos() + p.xpos;
  rd[Num_ships].y = stars[snum].ypos() + p.ypos;
  Num_ships++;

  if (p.info[Playernum - 1].explored) {
    shipnum_t shn = p.ships;
    while (shn && Getrship(g, shn))
      shn = rd[Num_ships - 1].s.nextship;
  }
}

static void star_getrships(GameObj& g, player_t Playernum,
                           starnum_t snum) {
  if (isset(stars[snum].explored(), Playernum)) {
    shipnum_t shn = stars[snum].ships();
    while (shn && Getrship(g, shn))
      shn = rd[Num_ships - 1].s.nextship;
    for (planetnum_t i = 0; i < stars[snum].numplanets(); i++)
      plan_getrships(g, Playernum, snum, i);
  }
}

/* get a ship from the disk and add it to the ship list we're maintaining. */
static bool Getrship(GameObj& g, shipnum_t shipno) {
  auto shiptmp = getship(shipno);
  if (shiptmp) {
    rd.resize(Num_ships + 1);
    rd[Num_ships].s = *shiptmp;
    rd[Num_ships].type = SHIP;
    rd[Num_ships].n = shipno;
    rd[Num_ships].x = rd[Num_ships].s.xpos;
    rd[Num_ships].y = rd[Num_ships].s.ypos;
    Num_ships++;
    return true;
  }
  g.out << std::format("Getrship: error on ship get ({}).\n", shipno);
  return false;
}

static bool listed(int type, char* string) {
  char *p;

  for (p = string; *p; p++) {
    if (Shipltrs[type] == *p) return true;
  }
  return false;
}
