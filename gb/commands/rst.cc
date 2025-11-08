// SPDX-License-Identifier: Apache-2.0

/// \file rst.cc
/// \brief Ship and planet reporting commands (report, stock, tactical, stats,
///        weapons, factories)
///
/// Implements various reporting modes for ships and planets including status,
/// stock levels, tactical combat information, and factory configurations.

module;

import gblib;
import std.compat;

module commands;

namespace {
enum class ReportType {
  Ship,
  Planet,
};

constexpr char kCaliber[] = {' ', 'L', 'M', 'H'};

struct ReportData {
  ReportType type; /* ship or planet */
  Ship s;
  Planet p;
  shipnum_t n;
  starnum_t star;
  planetnum_t pnum;
  double x;
  double y;
};

using ReportArray = std::array<bool, NUMSTYPES>;

// Report mode flags - all default to false
struct ReportFlags {
  bool status = false;
  bool ship = false;
  bool stock = false;
  bool report = false;
  bool weapons = false;
  bool factories = false;
  bool tactical = false;
};

// Context structure holding all rst command state
struct RstContext {
  std::vector<ReportData> rd;
  std::string shiplist;
  ReportFlags flags;
  bool first;
  bool enemies_only;
  int who;
  shipnum_t num_ships = 0;  // Local counter instead of global Num_ships
};

// Map of command names to their report flag configurations
// TODO(jeffbailey): Replace with std::unordered_map when available in C++20
// modules
constexpr auto kCommandReportModes = std::array{
    std::pair{"report", ReportFlags{.report = true}},
    std::pair{"stock", ReportFlags{.stock = true}},
    std::pair{"tactical", ReportFlags{.tactical = true}},
    std::pair{"ship", ReportFlags{.status = true,
                                  .ship = true,
                                  .stock = true,
                                  .report = true,
                                  .weapons = true,
                                  .factories = true}},
    std::pair{"stats", ReportFlags{.status = true}},
    std::pair{"weapons", ReportFlags{.weapons = true}},
    std::pair{"factories", ReportFlags{.factories = true}},
};

bool listed(int type, const std::string& string) {
  return std::ranges::any_of(string,
                             [type](char c) { return Shipltrs[type] == c; });
}

/* get a ship from the disk and add it to the ship list we're maintaining. */
bool get_report_ship(GameObj& g, RstContext& ctx, shipnum_t shipno) {
  const auto* ship = g.entity_manager.peek_ship(shipno);
  if (ship) {
    ctx.rd.resize(ctx.num_ships + 1);
    ctx.rd[ctx.num_ships].s = *ship;
    ctx.rd[ctx.num_ships].type = ReportType::Ship;
    ctx.rd[ctx.num_ships].n = shipno;
    ctx.rd[ctx.num_ships].x = ctx.rd[ctx.num_ships].s.xpos;
    ctx.rd[ctx.num_ships].y = ctx.rd[ctx.num_ships].s.ypos;
    ctx.num_ships++;
    return true;
  }
  g.out << std::format("get_report_ship: error on ship get ({}).\n", shipno);
  return false;
}

void plan_get_report_ships(GameObj& g, RstContext& ctx, player_t player_num,
                           starnum_t snum, planetnum_t pnum) {
  const auto* star = g.entity_manager.peek_star(snum);
  if (!star) return;

  ctx.rd.resize(ctx.num_ships + 1);
  ctx.rd[ctx.num_ships].p = getplanet(snum, pnum);
  const auto& p = ctx.rd[ctx.num_ships].p;
  /* add this planet into the ship list */
  ctx.rd[ctx.num_ships].star = snum;
  ctx.rd[ctx.num_ships].pnum = pnum;
  ctx.rd[ctx.num_ships].type = ReportType::Planet;
  ctx.rd[ctx.num_ships].n = 0;
  ctx.rd[ctx.num_ships].x = star->xpos + p.xpos;
  ctx.rd[ctx.num_ships].y = star->ypos + p.ypos;
  ctx.num_ships++;

  if (p.info[player_num - 1].explored) {
    shipnum_t shn = p.ships;
    while (shn && get_report_ship(g, ctx, shn))
      shn = ctx.rd[ctx.num_ships - 1].s.nextship;
  }
}

void star_get_report_ships(GameObj& g, RstContext& ctx, player_t player_num,
                           starnum_t snum) {
  const auto* star = g.entity_manager.peek_star(snum);
  if (!star) return;

  if (isset(star->explored, player_num)) {
    shipnum_t shn = star->ships;
    while (shn && get_report_ship(g, ctx, shn))
      shn = ctx.rd[ctx.num_ships - 1].s.nextship;
    for (planetnum_t i = 0; i < star->numplanets; i++)
      plan_get_report_ships(g, ctx, player_num, snum, i);
  }
}

void report_stock(GameObj& g, RstContext& ctx, shipnum_t indx, const Ship& s,
                  shipnum_t shipno) {
  // Only report for ships with Stock flag enabled
  if (ctx.rd[indx].type != ReportType::Ship || !ctx.flags.stock) return;

  if (ctx.first) {
    g.out << "    #       name        x  hanger   res        des       "
             "  fuel      crew/mil\n";
    if (!ctx.flags.ship) ctx.first = false;
  }
  g.out << std::format(
      "{:5} {:c} "
      "{:14.14}{:3}{:4}:{:<3}{:5}:{:<5}{:5}:{:<5}{:7.1f}:{:<6}{}/{}:{}\n",
      shipno, Shipltrs[s.type], (s.active ? s.name : "INACTIVE"), s.crystals,
      s.hanger, s.max_hanger, s.resource, max_resource(s), s.destruct,
      max_destruct(s), s.fuel, max_fuel(s), s.popn, s.troops, s.max_crew);
}

void report_status(GameObj& g, RstContext& ctx, shipnum_t indx, const Ship& s,
                   shipnum_t shipno) {
  // Only report for ships with Status flag enabled
  if (ctx.rd[indx].type != ReportType::Ship || !ctx.flags.status) return;

  if (ctx.first) {
    g.out << "    #       name       las cew hyp    guns   arm tech "
             "spd cost  mass size\n";
    if (!ctx.flags.ship) ctx.first = false;
  }
  g.out << std::format(
      "{:5} {:c} {:14.14} "
      "{}{}{}{:3}{:c}/{:3}{:c}{:4}{:5.0f}{:4}{:5}{:7.1f}{:4}",
      shipno, Shipltrs[s.type], (s.active ? s.name : "INACTIVE"),
      s.laser ? "yes " : "    ", s.cew ? "yes " : "    ",
      s.hyper_drive.has ? "yes " : "    ", s.primary, kCaliber[s.primtype],
      s.secondary, kCaliber[s.sectype], armor(s), s.tech, max_speed(s),
      shipcost(s), mass(s), size(s));
  if (s.type == ShipType::STYPE_POD) {
    if (std::holds_alternative<PodData>(s.special)) {
      auto pod = std::get<PodData>(s.special);
      g.out << std::format(" ({})", pod.temperature);
    }
  }
  g.out << "\n";
}

void report_weapons(GameObj& g, RstContext& ctx, shipnum_t indx, const Ship& s,
                    shipnum_t shipno) {
  // Only report for ships with Weapons flag enabled
  if (ctx.rd[indx].type != ReportType::Ship || !ctx.flags.weapons) return;

  if (ctx.first) {
    g.out << "    #       name      laser   cew     safe     guns    "
             "damage   class\n";
    if (!ctx.flags.ship) ctx.first = false;
  }
  g.out << std::format(
      "{:5} {:c} {:14.14} {}  {:3}/{:<4}  {:4}  {:3}{:c}/{:3}{:c}    {:3}% "
      " {:c} {}\n",
      shipno, Shipltrs[s.type], (s.active ? s.name : "INACTIVE"),
      s.laser ? "yes " : "    ", s.cew, s.cew_range,
      (int)((1.0 - .01 * s.damage) * s.tech / 4.0), s.primary,
      kCaliber[s.primtype], s.secondary, kCaliber[s.sectype], s.damage,
      s.type == ShipType::OTYPE_FACTORY ? Shipltrs[s.build_type] : ' ',
      ((s.type == ShipType::OTYPE_TERRA) || (s.type == ShipType::OTYPE_PLOW))
          ? "Standard"
          : s.shipclass);
}

void report_factories(GameObj& g, RstContext& ctx, shipnum_t indx,
                      const Ship& s, shipnum_t shipno) {
  // Only report for ships with Factories flag enabled and factory type
  if (ctx.rd[indx].type != ReportType::Ship || !ctx.flags.factories ||
      s.type != ShipType::OTYPE_FACTORY)
    return;

  if (ctx.first) {
    g.out << "   #    Cost Tech Mass Sz A Crw Ful Crg Hng Dst Sp "
             "Weapons Lsr CEWs Range Dmg\n";
    if (!ctx.flags.ship) ctx.first = false;
  }

  if ((s.build_type == 0) || (s.build_type == ShipType::OTYPE_FACTORY)) {
    g.out << std::format(
        "{:5}               (No ship type specified yet)           "
        "           75% (OFF)",
        shipno);
    return;
  }

  std::string tmpbuf1;
  if (s.primtype) {
    const char* caliber;
    switch (s.primtype) {
      case GTYPE_LIGHT:
        caliber = "L";
        break;
      case GTYPE_MEDIUM:
        caliber = "M";
        break;
      case GTYPE_HEAVY:
        caliber = "H";
        break;
      default:
        caliber = "N";
        break;
    }
    tmpbuf1 = std::format("{:2}{}", s.primary, caliber);
  } else {
    tmpbuf1 = "---";
  }

  std::string tmpbuf2;
  if (s.sectype) {
    const char* caliber;
    switch (s.sectype) {
      case GTYPE_LIGHT:
        caliber = "L";
        break;
      case GTYPE_MEDIUM:
        caliber = "M";
        break;
      case GTYPE_HEAVY:
        caliber = "H";
        break;
      default:
        caliber = "N";
        break;
    }
    tmpbuf2 = std::format("{:2}{}", s.secondary, caliber);
  } else {
    tmpbuf2 = "---";
  }

  std::string tmpbuf3 = s.cew ? std::format("{:4}", s.cew) : "----";
  std::string tmpbuf4 = s.cew ? std::format("{:5}", s.cew_range) : "-----";

  g.out << std::format(
      "{:5} {:c}{:4}{:6.1f}{:5.1f}{:3}{:2}{:4}{:4}{:4}{:4}{:4} {}{:1} "
      "{}/{} {} "
      "{} {} {:02}%{}\n",
      shipno, Shipltrs[s.build_type], s.build_cost, s.complexity, s.base_mass,
      ship_size(s), s.armor, s.max_crew, s.max_fuel, s.max_resource,
      s.max_hanger, s.max_destruct,
      s.hyper_drive.has ? (s.mount ? "+" : "*") : " ", s.max_speed, tmpbuf1,
      tmpbuf2, s.laser ? "yes" : " no", tmpbuf3, tmpbuf4, s.damage,
      s.damage ? (s.on ? "" : "*") : "");
}

void report_general(GameObj& g, RstContext& ctx, shipnum_t indx, const Ship& s,
                    shipnum_t shipno) {
  // Only report for ships with Report flag enabled
  if (ctx.rd[indx].type != ReportType::Ship || !ctx.flags.report) return;

  if (ctx.first) {
    g.out << " #      name       gov dam crew mil  des fuel sp orbits  "
             "   destination\n";
    if (!ctx.flags.ship) ctx.first = false;
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

  g.out << std::format(
      "{:c}{:<5} {:12.12} {:2} {:3}{:5}{:4}{:5}{:5.0f} {:c}{:1} {:<10} "
      "{:<18}\n",
      Shipltrs[s.type], shipno, (s.active ? s.name : strng), s.governor,
      s.damage, s.popn, s.troops, s.destruct, s.fuel,
      s.hyper_drive.has ? (s.mounted ? '+' : '*') : ' ', s.speed,
      dispshiploc_brief(s), locstrn);
}

void print_tactical_target_ship(GameObj& g, RstContext& ctx, const Race& race,
                                shipnum_t indx, int i, double dist, double tech,
                                int fdam, bool fev, int fspeed,
                                guntype_t caliber) {
  const player_t player_num = g.player;
  const governor_t governor = g.governor;

  if (!ctx.who || ctx.who == ctx.rd[i].s.owner ||
      (ctx.who == 999 && listed((int)ctx.rd[i].s.type, ctx.shiplist))) {
    /* tac report at ship */
    if ((ctx.rd[i].s.owner != player_num ||
         !authorized(governor, ctx.rd[i].s)) &&
        ctx.rd[i].s.alive && ctx.rd[i].s.type != ShipType::OTYPE_CANIST &&
        ctx.rd[i].s.type != ShipType::OTYPE_GREEN) {
      bool tev = false;
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
      auto [prob, factor] = hit_odds(dist, tech, fdam, fev, tev, fspeed, tspeed,
                                     body, caliber, defense);
      if (ctx.rd[indx].type == ReportType::Ship && laser_on(ctx.rd[indx].s) &&
          ctx.rd[indx].s.focus)
        prob = prob * prob / 100;
      const auto* war_status = isset(race.atwar, ctx.rd[i].s.owner)    ? "-"
                               : isset(race.allied, ctx.rd[i].s.owner) ? "+"
                                                                       : " ";
      if (!ctx.enemies_only ||
          (ctx.enemies_only && (!isset(race.allied, ctx.rd[i].s.owner)))) {
        g.out << std::format(
            "{:13} {}{:2},{:1} {:c}{:14.14} {:4.0f}  {:4}   {:4} {}  "
            "{:3}  "
            "{:3}% {:3}%{}",
            ctx.rd[i].n, war_status, ctx.rd[i].s.owner, ctx.rd[i].s.governor,
            Shipltrs[ctx.rd[i].s.type], ctx.rd[i].s.name, dist, factor, body,
            tspeed, (tev ? "yes" : "   "), prob, ctx.rd[i].s.damage,
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

struct TacticalParams {
  double tech = 0.0;
  bool fev = false;
  int fspeed = 0;
};

void print_tactical_header_summary(GameObj& g, RstContext& ctx, shipnum_t indx,
                                   const Ship& s, const Planet& p,
                                   shipnum_t shipno,
                                   const TacticalParams& params) {
  const player_t player_num = g.player;

  g.out << "\n  #         name        tech    guns  armor size dest   "
           "fuel dam spd evad               orbits\n";

  if (ctx.rd[indx].type == ReportType::Planet) {
    const auto* star = g.entity_manager.peek_star(ctx.rd[indx].star);
    /* tac report from planet */
    g.out << std::format("(planet){:15.15}{:4.0f} {:4}M           {:5} {:6}\n",
                         star ? star->pnames[ctx.rd[indx].pnum] : "Unknown",
                         params.tech, p.info[player_num - 1].guns,
                         p.info[player_num - 1].destruct,
                         p.info[player_num - 1].fuel);
  } else {
    Place where{s.whatorbits, s.storbits, s.pnumorbits};
    auto orb = std::format("{:30.30}", where.to_string());
    g.out << std::format(
        "{:3} {:c} {:16.16} "
        "{:4.0f}{:3}{:c}/{:3}{:c}{:6}{:5}{:5}{:7.1f}{:3}%  {}  "
        "{:3}{:21.22}",
        shipno, Shipltrs[s.type], (s.active ? s.name : "INACTIVE"), s.tech,
        s.primary, kCaliber[s.primtype], s.secondary, kCaliber[s.sectype],
        s.armor, s.size, s.destruct, s.fuel, s.damage, params.fspeed,
        (params.fev ? "yes" : "   "), orb);
    if (landed(s)) {
      g.out << std::format(" ({},{})", s.land_x, s.land_y);
    }
    if (!s.active) {
      g.out << std::format(" INACTIVE({})", s.rad);
    }
    g.out << "\n";
  }
}

void print_tactical_target_planet(GameObj& g, RstContext& ctx, int i,
                                  double dist) {
  const auto* star = g.entity_manager.peek_star(ctx.rd[i].star);
  g.out << std::format(" {:13}(planet)          {:8.0f}\n",
                       star ? star->pnames[ctx.rd[i].pnum] : "Unknown", dist);
}

void report_tactical(GameObj& g, RstContext& ctx, shipnum_t indx, const Ship& s,
                     const Planet& p, shipnum_t shipno) {
  // Only report if Tactical flag is enabled
  if (!ctx.flags.tactical) return;

  const player_t player_num = g.player;

  const auto* race = g.entity_manager.peek_race(player_num);
  if (!race) return;

  // Calculate tactical parameters
  TacticalParams params{};
  params.tech = ctx.rd[indx].type == ReportType::Planet ? race->tech : s.tech;

  if (ctx.rd[indx].type == ReportType::Ship &&
      (s.whatdest != ScopeLevel::LEVEL_UNIV || s.navigate.on) && !s.docked &&
      s.active) {
    params.fspeed = s.speed;
    params.fev = s.protect.evade;
  }

  print_tactical_header_summary(g, ctx, indx, s, p, shipno, params);

  bool sight = ctx.rd[indx].type == ReportType::Planet || shipsight(s);

  /* tactical display */
  g.out << "\n  Tactical: #  own typ        name   rng   (50%) size "
           "spd evade hit  dam  loc\n";

  if (!sight) return;

  for (int i = 0; i < ctx.num_ships; i++) {
    if (i == indx) continue;

    double range = ctx.rd[indx].type == ReportType::Planet
                       ? gun_range(*race)
                       : gun_range(ctx.rd[indx].s);
    double dist =
        sqrt(Distsq(ctx.rd[indx].x, ctx.rd[indx].y, ctx.rd[i].x, ctx.rd[i].y));
    if (dist >= range) continue;

    if (ctx.rd[i].type == ReportType::Planet) {
      print_tactical_target_planet(g, ctx, i, dist);
      continue;
    }

    // Calculate ship-specific combat parameters
    int fdam = ctx.rd[indx].type == ReportType::Planet ? 0 : s.damage;
    guntype_t caliber = ctx.rd[indx].type == ReportType::Planet
                            ? GTYPE_MEDIUM
                            : current_caliber(s);
    print_tactical_target_ship(g, ctx, *race, indx, i, dist, params.tech, fdam,
                               params.fev, params.fspeed, caliber);
  }
}

void ship_report(GameObj& g, RstContext& ctx, shipnum_t indx,
                 const ReportArray& rep_on) {
  player_t player_num = g.player;
  governor_t governor = g.governor;

  // Bounds check - ensure indx is within the vector
  if (indx >= ctx.rd.size()) {
    return;
  }

  // Get race from EntityManager
  const auto* race = g.entity_manager.peek_race(player_num);
  if (!race) {
    g.out << "Race not found.\n";
    return;
  }

  /* last ship gotten from disk */
  auto& s = ctx.rd[indx].s;
  auto& p = ctx.rd[indx].p;
  shipnum_t shipno = ctx.rd[indx].n;

  // Don't report on planets where we don't own any sectors
  if (ctx.rd[indx].type == ReportType::Planet &&
      !p.info[player_num - 1].numsectsowned) {
    return;
  }

  // Don't report on ships that are dead
  if (ctx.rd[indx].type == ReportType::Ship && !s.alive) {
    return;
  }

  // Don't report on ships not owned by this player
  if (ctx.rd[indx].type == ReportType::Ship && s.owner != player_num) {
    return;
  }

  // Don't report on ships this governor is not authorized for
  if (ctx.rd[indx].type == ReportType::Ship && !authorized(governor, s)) {
    return;
  }

  // Don't report on ships whose type isn't in the requested report filter
  if (ctx.rd[indx].type == ReportType::Ship && !rep_on[s.type]) {
    return;
  }

  // Don't report on undocked canisters (launched canisters don't show up)
  if (ctx.rd[indx].type == ReportType::Ship &&
      s.type == ShipType::OTYPE_CANIST && !s.docked) {
    return;
  }

  // Don't report on undocked greens (launched greens don't show up)
  if (ctx.rd[indx].type == ReportType::Ship &&
      s.type == ShipType::OTYPE_GREEN && !s.docked) {
    return;
  }

  // All early-return filters passed - proceed with reporting
  report_stock(g, ctx, indx, s, shipno);
  report_status(g, ctx, indx, s, shipno);
  report_weapons(g, ctx, indx, s, shipno);
  report_factories(g, ctx, indx, s, shipno);
  report_general(g, ctx, indx, s, shipno);
  report_tactical(g, ctx, indx, s, p, shipno);
}
}  // namespace

namespace GB::commands {
void rst(const command_t& argv, GameObj& g) {
  const player_t player_num = g.player;

  ReportArray report_types;
  report_types.fill(true);

  RstContext ctx;
  ctx.enemies_only = false;
  ctx.first = true;

  // Set report flags based on command name
  for (const auto& [cmd, flags] : kCommandReportModes) {
    if (argv[0] == cmd) {
      ctx.flags = flags;
      break;
    }
  }

  shipnum_t n_ships = Numships();
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
        shipnum_t shipno;
        sscanf(argv[l].c_str() + (*argv[l].c_str() == '#'), "%lu", &shipno);
        if (shipno > n_ships || shipno < 1) {
          g.out << std::format("rst: no such ship #{} \n", shipno);
          return;
        }
        get_report_ship(g, ctx, shipno);
        if (ctx.rd[ctx.num_ships - 1].s.whatorbits != ScopeLevel::LEVEL_UNIV) {
          star_get_report_ships(g, ctx, player_num,
                                ctx.rd[ctx.num_ships - 1].s.storbits);
          ship_report(g, ctx, ctx.num_ships - 1, report_types);
        } else
          ship_report(g, ctx, ctx.num_ships - 1, report_types);
        l++;
      }
      return;
    }
    report_types.fill(false);

    for (const auto& c : argv[1]) {
      shipnum_t i = NUMSTYPES;
      while (--i && Shipltrs[i] != c);
      if (Shipltrs[i] != c) {
        g.out << std::format("'{}' -- no such ship letter\n", c);
      } else
        report_types[i] = true;
    }
  }

  switch (g.level) {
    case ScopeLevel::LEVEL_UNIV:
      if (!ctx.flags.tactical || argv.size() >= 2) {
        shipnum_t shn = Sdata.ships;
        while (shn && get_report_ship(g, ctx, shn))
          shn = ctx.rd[ctx.num_ships - 1].s.nextship;

        for (starnum_t i = 0; i < Sdata.numstars; i++)
          star_get_report_ships(g, ctx, player_num, i);
        for (shipnum_t i = 0; i < ctx.rd.size(); i++)
          ship_report(g, ctx, i, report_types);
      } else {
        g.out << "You can't do tactical option from universe level.\n";
        return;
      }
      break;
    case ScopeLevel::LEVEL_PLAN:
      plan_get_report_ships(g, ctx, player_num, g.snum, g.pnum);
      for (shipnum_t i = 0; i < ctx.rd.size(); i++)
        ship_report(g, ctx, i, report_types);
      break;
    case ScopeLevel::LEVEL_STAR:
      star_get_report_ships(g, ctx, player_num, g.snum);
      for (shipnum_t i = 0; i < ctx.rd.size(); i++)
        ship_report(g, ctx, i, report_types);
      break;
    case ScopeLevel::LEVEL_SHIP:
      if (g.shipno == 0) {
        g.out << "Error: No ship is currently scoped. Use 'cs #<shipno>' to "
                 "scope to a ship.\n";
        return;
      }
      get_report_ship(g, ctx, g.shipno);
      if (ctx.rd.empty()) {
        g.out << std::format("Error: Unable to retrieve ship #{} data.\n",
                             g.shipno);
        return;
      }
      ship_report(g, ctx, 0, report_types); /* first ship report */
      shipnum_t shn = ctx.rd[0].s.ships;
      shipnum_t first_child = ctx.num_ships;

      while (shn && get_report_ship(g, ctx, shn))
        shn = ctx.rd[ctx.num_ships - 1].s.nextship;

      for (shipnum_t i = first_child; i < ctx.rd.size(); i++)
        ship_report(g, ctx, i, report_types);
      break;
  }
}
}  // namespace GB::commands