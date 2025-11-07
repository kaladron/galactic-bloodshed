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

// Context structure holding all rst command state
struct RstContext {
  std::vector<reportdata> rd;
  std::string shiplist;
  bool Status;
  bool SHip;
  bool Stock;
  bool Report;
  bool Weapons;
  bool Factories;
  bool first;
  bool Tactical;
  bool enemies_only;
  int who;
};

static bool Getrship(GameObj&, RstContext&, shipnum_t);
static bool listed(int, const std::string&);
static void plan_getrships(GameObj&, RstContext&, player_t, starnum_t,
                           planetnum_t);
static void ship_report(GameObj&, RstContext&, shipnum_t,
                        const report_array&);
static void star_getrships(GameObj&, RstContext&, player_t, starnum_t);

namespace GB::commands {
void rst(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;
  shipnum_t shipno;

  report_array Report_types;
  Report_types.fill(true);

  RstContext ctx;
  ctx.enemies_only = false;
  Num_ships = 0;
  ctx.first = true;
  if (argv[0] == "report") {
    ctx.Report = true;
    ctx.Weapons = ctx.Status = ctx.Stock = ctx.SHip = ctx.Factories = false;
    ctx.Tactical = false;
  } else if (argv[0] == "stock") {
    ctx.Stock = true;
    ctx.Weapons = ctx.Status = ctx.Report = ctx.SHip = ctx.Factories = false;
    ctx.Tactical = false;
  } else if (argv[0] == "tactical") {
    ctx.Tactical = true;
    ctx.Weapons = ctx.Status = ctx.Report = ctx.SHip = ctx.Stock = ctx.Factories = false;
  } else if (argv[0] == "ship") {
    ctx.SHip = ctx.Report = ctx.Stock = true;
    ctx.Tactical = false;
    ctx.Weapons = ctx.Status = ctx.Factories = true;
  } else if (argv[0] == "stats") {
    ctx.Status = true;
    ctx.Weapons = ctx.Report = ctx.Stock = ctx.SHip = ctx.Factories = false;
    ctx.Tactical = false;
  } else if (argv[0] == "weapons") {
    ctx.Weapons = true;
    ctx.Status = ctx.Report = ctx.Stock = ctx.SHip = ctx.Factories = false;
    ctx.Tactical = false;
  } else if (argv[0] == "factories") {
    ctx.Factories = true;
    ctx.Status = ctx.Report = ctx.Stock = ctx.SHip = ctx.Weapons = false;
    ctx.Tactical = false;
  }
  shipnum_t n_ships = Numships();
  ctx.rd.clear();
  ctx.rd.reserve(n_ships + Sdata.numstars * MAXPLANETS);
  /* (one list entry for each ship, planet in universe) */

  if (argv.size() == 3) {
    if (isdigit(argv[2][0]))
      ctx.who = std::stoi(argv[2]);
    else {
      ctx.who = 999; /* treat argv[2].c_str() as a list of ship types */
      ctx.shiplist = argv[2];
    }
  } else
    ctx.who = 0;

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
        (void)Getrship(g, ctx, shipno);
        if (ctx.rd[Num_ships - 1].s.whatorbits != ScopeLevel::LEVEL_UNIV) {
          star_getrships(g, ctx, Playernum, ctx.rd[Num_ships - 1].s.storbits);
          ship_report(g, ctx, Num_ships - 1, Report_types);
        } else
          ship_report(g, ctx, Num_ships - 1, Report_types);
        l++;
      }
      return;
    }
    Report_types.fill(false);

    for (const auto& c : argv[1]) {
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
      if (!(ctx.Tactical && argv.size() < 2)) {
        shipnum_t shn = Sdata.ships;
        while (shn && Getrship(g, ctx, shn))
          shn = ctx.rd[Num_ships - 1].s.nextship;

        for (starnum_t i = 0; i < Sdata.numstars; i++)
          star_getrships(g, ctx, Playernum, i);
        for (shipnum_t i = 0; i < Num_ships; i++)
          ship_report(g, ctx, i, Report_types);
      } else {
        g.out << "You can't do tactical option from universe level.\n";
        return;
      }
      break;
    case ScopeLevel::LEVEL_PLAN:
      plan_getrships(g, ctx, Playernum, g.snum, g.pnum);
      for (shipnum_t i = 0; i < Num_ships; i++)
        ship_report(g, ctx, i, Report_types);
      break;
    case ScopeLevel::LEVEL_STAR:
      star_getrships(g, ctx, Playernum, g.snum);
      for (shipnum_t i = 0; i < Num_ships; i++)
        ship_report(g, ctx, i, Report_types);
      break;
    case ScopeLevel::LEVEL_SHIP:
      (void)Getrship(g, ctx, g.shipno);
      ship_report(g, ctx, 0, Report_types); /* first ship report */
      shipnum_t shn = ctx.rd[0].s.ships;
      Num_ships = 0;

      while (shn && Getrship(g, ctx, shn))
        shn = ctx.rd[Num_ships - 1].s.nextship;

      for (shipnum_t i = 0; i < Num_ships; i++)
        ship_report(g, ctx, i, Report_types);
      break;
  }
}
}  // namespace GB::commands

static void ship_report(GameObj& g, RstContext& ctx, shipnum_t indx,
                        const report_array& rep_on) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  int i;
  int sight;
  guntype_t caliber;
  double Dist;

  // Get race from EntityManager
  const auto* race = g.entity_manager.peek_race(Playernum);
  if (!race) {
    g.out << "Race not found.\n";
    return;
  }

  /* last ship gotten from disk */
  auto& s = ctx.rd[indx].s;
  auto& p = ctx.rd[indx].p;
  shipnum_t shipno = ctx.rd[indx].n;

  /* launched canister, non-owned ships don't show up */
  if ((ctx.rd[indx].type == PLANET && p.info[Playernum - 1].numsectsowned) ||
      (ctx.rd[indx].type != PLANET && s.alive && s.owner == Playernum &&
       authorized(Governor, s) && rep_on[s.type] &&
       !(s.type == ShipType::OTYPE_CANIST && !s.docked) &&
       !(s.type == ShipType::OTYPE_GREEN && !s.docked))) {
    if (ctx.rd[indx].type != PLANET && ctx.Stock) {
      if (ctx.first) {
        g.out << "    #       name        x  hanger   res        des       "
                 "  fuel      crew/mil\n";
        if (!ctx.SHip) ctx.first = false;
      }
      g.out << std::format(
          "{:5} {:c} "
          "{:14.14}{:3}{:4}:{:<3}{:5}:{:<5}{:5}:{:<5}{:7.1f}:{:<6}{}/{}:{}\n",
          shipno, Shipltrs[s.type], (s.active ? s.name : "INACTIVE"),
          s.crystals, s.hanger, s.max_hanger, s.resource, max_resource(s),
          s.destruct, max_destruct(s), s.fuel, max_fuel(s), s.popn, s.troops,
          s.max_crew);
    }

    if (ctx.rd[indx].type != PLANET && ctx.Status) {
      if (ctx.first) {
        g.out << "    #       name       las cew hyp    guns   arm tech "
                 "spd cost  mass size\n";
        if (!ctx.SHip) ctx.first = false;
      }
      g.out << std::format(
          "{:5} {:c} {:14.14} "
          "{}{}{}{:3}{:c}/{:3}{:c}{:4}{:5.0f}{:4}{:5}{:7.1f}{:4}",
          shipno, Shipltrs[s.type], (s.active ? s.name : "INACTIVE"),
          s.laser ? "yes " : "    ", s.cew ? "yes " : "    ",
          s.hyper_drive.has ? "yes " : "    ", s.primary, Caliber[s.primtype],
          s.secondary, Caliber[s.sectype], armor(s), s.tech, max_speed(s),
          shipcost(s), mass(s), size(s));
      if (s.type == ShipType::STYPE_POD) {
        if (std::holds_alternative<PodData>(s.special)) {
          auto pod = std::get<PodData>(s.special);
          g.out << std::format(" ({})", pod.temperature);
        }
      }
      g.out << "\n";
    }

    if (ctx.rd[indx].type != PLANET && ctx.Weapons) {
      if (ctx.first) {
        g.out << "    #       name      laser   cew     safe     guns    "
                 "damage   class\n";
        if (!ctx.SHip) ctx.first = false;
      }
      g.out << std::format(
          "{:5} {:c} {:14.14} {}  {:3}/{:<4}  {:4}  {:3}{:c}/{:3}{:c}    {:3}% "
          " {:c} {}\n",
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

    if (ctx.rd[indx].type != PLANET && ctx.Factories &&
        (s.type == ShipType::OTYPE_FACTORY)) {
      if (ctx.first) {
        g.out << "   #    Cost Tech Mass Sz A Crw Ful Crg Hng Dst Sp "
                 "Weapons Lsr CEWs Range Dmg\n";
        if (!ctx.SHip) ctx.first = false;
      }
      if ((s.build_type == 0) || (s.build_type == ShipType::OTYPE_FACTORY)) {
        g.out << std::format(
            "{:5}               (No ship type specified yet)           "
            "           75% (OFF)",
            shipno);
      } else {
        std::string tmpbuf1;
        if (s.primtype) {
          auto caliber = s.primtype == GTYPE_LIGHT    ? "L"
                         : s.primtype == GTYPE_MEDIUM ? "M"
                         : s.primtype == GTYPE_HEAVY  ? "H"
                                                      : "N";
          tmpbuf1 = std::format("{:2}{}", s.primary, caliber);
        } else {
          tmpbuf1 = "---";
        }

        std::string tmpbuf2;
        if (s.sectype) {
          auto caliber = s.sectype == GTYPE_LIGHT    ? "L"
                         : s.sectype == GTYPE_MEDIUM ? "M"
                         : s.sectype == GTYPE_HEAVY  ? "H"
                                                     : "N";
          tmpbuf2 = std::format("{:2}{}", s.secondary, caliber);
        } else {
          tmpbuf2 = "---";
        }

        std::string tmpbuf3 = s.cew ? std::format("{:4}", s.cew) : "----";
        std::string tmpbuf4 =
            s.cew ? std::format("{:5}", s.cew_range) : "-----";

        g.out << std::format(
            "{:5} {:c}{:4}{:6.1f}{:5.1f}{:3}{:2}{:4}{:4}{:4}{:4}{:4} {}{:1} "
            "{}/{} {} "
            "{} {} {:02}%{}\n",
            shipno, Shipltrs[s.build_type], s.build_cost, s.complexity,
            s.base_mass, ship_size(s), s.armor, s.max_crew, s.max_fuel,
            s.max_resource, s.max_hanger, s.max_destruct,
            s.hyper_drive.has ? (s.mount ? "+" : "*") : " ", s.max_speed,
            tmpbuf1, tmpbuf2, s.laser ? "yes" : " no", tmpbuf3, tmpbuf4,
            s.damage, s.damage ? (s.on ? "" : "*") : "");
      }
    }

    if (ctx.rd[indx].type != PLANET && ctx.Report) {
      if (ctx.first) {
        g.out << " #      name       gov dam crew mil  des fuel sp orbits  "
                 "   destination\n";
        if (!ctx.SHip) ctx.first = false;
      }
      std::string locstrn;
      if (s.docked) {
        if (s.whatdest == ScopeLevel::LEVEL_SHIP)
          locstrn = std::format("D#{}", s.destshipno);
        else
          locstrn = std::format("L{:2},{:<2}", s.land_x, s.land_y);
      } else if (s.navigate.on) {
        locstrn =
            std::format("nav:{} ({})", s.navigate.bearing, s.navigate.turns);
      } else {
        locstrn = prin_ship_dest(s);
      }

      std::string strng;
      if (!s.active) {
        strng = std::format("INACTIVE({})", s.rad);
      }

      g.out << std::format(
          "{:c}{:<5} {:12.12} {:2} {:3}{:5}{:4}{:5}{:5.0f} {:c}{:1} {:<10} "
          "{:<18}\n",
          Shipltrs[s.type], shipno, (s.active ? s.name : strng), s.governor,
          s.damage, s.popn, s.troops, s.destruct, s.fuel,
          s.hyper_drive.has ? (s.mounted ? '+' : '*') : ' ', s.speed,
          dispshiploc_brief(s), locstrn);
    }

    if (ctx.Tactical) {
      int fev = 0;
      int fspeed = 0;
      int fdam = 0;
      double tech;

      g.out << "\n  #         name        tech    guns  armor size dest   "
               "fuel dam spd evad               orbits\n";

      if (ctx.rd[indx].type == PLANET) {
        const auto* star = g.entity_manager.peek_star(ctx.rd[indx].star);
        tech = race->tech;
        /* tac report from planet */
        g.out << std::format(
            "(planet){:15.15}{:4.0f} {:4}M           {:5} {:6}\n",
            star ? star->pnames[ctx.rd[indx].pnum] : "Unknown", tech,
            p.info[Playernum - 1].guns, p.info[Playernum - 1].destruct,
            p.info[Playernum - 1].fuel);
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
            "{:3} {:c} {:16.16} "
            "{:4.0f}{:3}{:c}/{:3}{:c}{:6}{:5}{:5}{:7.1f}{:3}%  {}  "
            "{:3}{:21.22}",
            shipno, Shipltrs[s.type], (s.active ? s.name : "INACTIVE"), s.tech,
            s.primary, Caliber[s.primtype], s.secondary, Caliber[s.sectype],
            s.armor, s.size, s.destruct, s.fuel, s.damage, fspeed,
            (fev ? "yes" : "   "), orb);
        if (landed(s)) {
          g.out << std::format(" ({},{})", s.land_x, s.land_y);
        }
        if (!s.active) {
          g.out << std::format(" INACTIVE({})", s.rad);
        }
        g.out << "\n";
      }

      sight = 0;
      if (ctx.rd[indx].type == PLANET)
        sight = 1;
      else if (shipsight(s))
        sight = 1;

      /* tactical display */
      g.out << "\n  Tactical: #  own typ        name   rng   (50%) size "
               "spd evade hit  dam  loc\n";

      if (sight)
        for (i = 0; i < Num_ships; i++) {
          double range = ctx.rd[indx].type == PLANET
                             ? gun_range(*race)
                             : gun_range(ctx.rd[indx].s);
          if (i != indx &&
              (Dist = sqrt(Distsq(ctx.rd[indx].x, ctx.rd[indx].y, ctx.rd[i].x,
                                  ctx.rd[i].y))) < range) {
            if (ctx.rd[i].type == PLANET) {
              /* tac report at planet */
              const auto* star = g.entity_manager.peek_star(ctx.rd[i].star);
              g.out << std::format(
                  " {:13}(planet)          {:8.0f}\n",
                  star ? star->pnames[ctx.rd[i].pnum] : "Unknown", Dist);
            } else if (!ctx.who || ctx.who == ctx.rd[i].s.owner ||
                       (ctx.who == 999 &&
                        listed((int)ctx.rd[i].s.type, ctx.shiplist))) {
              /* tac report at ship */
              if ((ctx.rd[i].s.owner != Playernum ||
                   !authorized(Governor, ctx.rd[i].s)) &&
                  ctx.rd[i].s.alive &&
                  ctx.rd[i].s.type != ShipType::OTYPE_CANIST &&
                  ctx.rd[i].s.type != ShipType::OTYPE_GREEN) {
                int tev = 0;
                int tspeed = 0;
                int body = 0;

                if ((ctx.rd[i].s.whatdest != ScopeLevel::LEVEL_UNIV ||
                     ctx.rd[i].s.navigate.on) &&
                    !ctx.rd[i].s.docked && ctx.rd[i].s.active) {
                  tspeed = ctx.rd[i].s.speed;
                  tev = ctx.rd[i].s.protect.evade;
                }
                body = size(ctx.rd[i].s);
                auto defense = getdefense(ctx.rd[i].s);
                auto [prob, factor] =
                    hit_odds(Dist, tech, fdam, fev, tev, fspeed, tspeed, body,
                             caliber, defense);
                if (ctx.rd[indx].type != PLANET && laser_on(ctx.rd[indx].s) &&
                    ctx.rd[indx].s.focus)
                  prob = prob * prob / 100;
                auto war_status = isset(race->atwar, ctx.rd[i].s.owner) ? "-"
                                  : isset(race->allied, ctx.rd[i].s.owner)
                                      ? "+"
                                      : " ";
                if (!ctx.enemies_only ||
                    (ctx.enemies_only &&
                     (!isset(race->allied, ctx.rd[i].s.owner)))) {
                  g.out << std::format(
                      "{:13} {}{:2},{:1} {:c}{:14.14} {:4.0f}  {:4}   {:4} {}  "
                      "{:3}  "
                      "{:3}% {:3}%{}",
                      ctx.rd[i].n, war_status, ctx.rd[i].s.owner,
                      ctx.rd[i].s.governor, Shipltrs[ctx.rd[i].s.type],
                      ctx.rd[i].s.name, Dist, factor, body, tspeed,
                      (tev ? "yes" : "   "), prob, ctx.rd[i].s.damage,
                      (ctx.rd[i].s.active ? "" : " INACTIVE"));
                  if (landed(ctx.rd[i].s)) {
                    g.out << std::format(" ({},{})", ctx.rd[i].s.land_x,
                                         ctx.rd[i].s.land_y);
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

static void plan_getrships(GameObj& g, RstContext& ctx, player_t Playernum,
                           starnum_t snum, planetnum_t pnum) {
  const auto* star = g.entity_manager.peek_star(snum);
  if (!star) return;

  ctx.rd.resize(Num_ships + 1);
  ctx.rd[Num_ships].p = getplanet(snum, pnum);
  const auto& p = ctx.rd[Num_ships].p;
  /* add this planet into the ship list */
  ctx.rd[Num_ships].star = snum;
  ctx.rd[Num_ships].pnum = pnum;
  ctx.rd[Num_ships].type = PLANET;
  ctx.rd[Num_ships].n = 0;
  ctx.rd[Num_ships].x = star->xpos + p.xpos;
  ctx.rd[Num_ships].y = star->ypos + p.ypos;
  Num_ships++;

  if (p.info[Playernum - 1].explored) {
    shipnum_t shn = p.ships;
    while (shn && Getrship(g, ctx, shn))
      shn = ctx.rd[Num_ships - 1].s.nextship;
  }
}

static void star_getrships(GameObj& g, RstContext& ctx, player_t Playernum,
                           starnum_t snum) {
  const auto* star = g.entity_manager.peek_star(snum);
  if (!star) return;

  if (isset(star->explored, Playernum)) {
    shipnum_t shn = star->ships;
    while (shn && Getrship(g, ctx, shn))
      shn = ctx.rd[Num_ships - 1].s.nextship;
    for (planetnum_t i = 0; i < star->numplanets; i++)
      plan_getrships(g, ctx, Playernum, snum, i);
  }
}

/* get a ship from the disk and add it to the ship list we're maintaining. */
static bool Getrship(GameObj& g, RstContext& ctx, shipnum_t shipno) {
  const auto* ship = g.entity_manager.peek_ship(shipno);
  if (ship) {
    ctx.rd.resize(Num_ships + 1);
    ctx.rd[Num_ships].s = *ship;
    ctx.rd[Num_ships].type = SHIP;
    ctx.rd[Num_ships].n = shipno;
    ctx.rd[Num_ships].x = ctx.rd[Num_ships].s.xpos;
    ctx.rd[Num_ships].y = ctx.rd[Num_ships].s.ypos;
    Num_ships++;
    return true;
  }
  g.out << std::format("Getrship: error on ship get ({}).\n", shipno);
  return false;
}

static bool listed(int type, const std::string& string) {
  for (char c : string) {
    if (Shipltrs[type] == c) return true;
  }
  return false;
}
