// SPDX-License-Identifier: Apache-2.0

/// \file rst.cc
/// \brief Ship reporting commands (report, stock, stats, weapons, factories)
///
/// Implements various reporting modes for ships including status,
/// stock levels, and factory configurations.
///
/// Note: Tactical reporting has been moved to tactical.cc

module;

import gblib;
import std.compat;
import tabulate;

module commands;

namespace {

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================

using ReportSet = std::unordered_set<char>;

struct ReportFlags {
  bool status = false;
  bool ship = false;
  bool stock = false;
  bool report = false;
  bool weapons = false;
  bool factories = false;
};

struct RstContext {
  ReportFlags flags;
  bool first = true;
};

// Map of command names to their report flag configurations
const std::unordered_map<std::string_view, ReportFlags> kCommandReportModes = {
    {"report", ReportFlags{.report = true}},
    {"stock", ReportFlags{.stock = true}},
    {"ship", ReportFlags{.status = true,
                         .ship = true,
                         .stock = true,
                         .report = true,
                         .weapons = true,
                         .factories = true}},
    {"stats", ReportFlags{.status = true}},
    {"weapons", ReportFlags{.weapons = true}},
    {"factories", ReportFlags{.factories = true}},
};

// ============================================================================
// SHIP REPORT FUNCTIONS
// ============================================================================

void report_stock(GameObj& g, RstContext& ctx, const Ship& s) {
  if (!ctx.flags.stock) return;

  // Create table
  tabulate::Table table;

  // Format table: no borders, just spacing between columns
  table.format().hide_border().column_separator("  ");

  // Set column alignments and widths
  table.column(0).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(1).format().width(16);
  table.column(2).format().width(3).font_align(tabulate::FontAlign::center);
  table.column(3).format().width(10).font_align(tabulate::FontAlign::center);
  table.column(4).format().width(12).font_align(tabulate::FontAlign::center);
  table.column(5).format().width(12).font_align(tabulate::FontAlign::center);
  table.column(6).format().width(14).font_align(tabulate::FontAlign::center);
  table.column(7).format().width(12).font_align(tabulate::FontAlign::center);

  // Add header row only on first call
  if (ctx.first) {
    table.add_row(
        {"#", "name", "x", "hanger", "res", "des", "fuel", "crew/mil"});
    // Format header row with bold
    table[0].format().font_style({tabulate::FontStyle::bold});

    if (!ctx.flags.ship) ctx.first = false;
  }

  // Add data row
  table.add_row(
      {std::format("{}", s.number),
       std::format("{}{} {}", Shipltrs[s.type], s.crystals ? 'x' : ' ',
                   s.active ? s.name : "INACTIVE"),
       std::format("{}", s.crystals),
       std::format("{}:{}", s.hanger, s.max_hanger),
       std::format("{}:{}", s.resource, max_resource(s)),
       std::format("{}:{}", s.destruct, max_destruct(s)),
       std::format("{:.1f}:{}", s.fuel, max_fuel(s)),
       std::format("{}/{}:{}", s.popn, s.troops, s.max_crew)});

  g.out << table << "\n";
}

void report_status(GameObj& g, RstContext& ctx, const Ship& s) {
  if (!ctx.flags.status) return;

  // Create table
  tabulate::Table table;

  // Format table: no borders, just spacing between columns
  table.format().hide_border().column_separator("  ");

  // Set column alignments and widths
  table.column(0).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(1).format().width(1).font_align(tabulate::FontAlign::center);
  table.column(2).format().width(14);
  table.column(3).format().width(4).font_align(tabulate::FontAlign::center);
  table.column(4).format().width(4).font_align(tabulate::FontAlign::center);
  table.column(5).format().width(4).font_align(tabulate::FontAlign::center);
  table.column(6).format().width(7).font_align(tabulate::FontAlign::center);
  table.column(7).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(8).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(9).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(10).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(11).format().width(7).font_align(tabulate::FontAlign::right);
  table.column(12).format().width(4).font_align(tabulate::FontAlign::right);

  // Add header row only on first call
  if (ctx.first) {
    table.add_row({"#", "", "name", "las", "cew", "hyp", "guns", "arm", "tech",
                   "spd", "cost", "mass", "size"});
    // Format header row with bold
    table[0].format().font_style({tabulate::FontStyle::bold});

    if (!ctx.flags.ship) ctx.first = false;
  }

  // Build special suffix for POD temperature
  std::string pod_suffix;
  if (s.type == ShipType::STYPE_POD) {
    if (std::holds_alternative<PodData>(s.special)) {
      auto pod = std::get<PodData>(s.special);
      pod_suffix = std::format(" ({})", pod.temperature);
    }
  }

  // Add data row
  table.add_row(
      {std::format("{}", s.number), std::format("{}", Shipltrs[s.type]),
       std::format("{}", s.active ? s.name : "INACTIVE"), s.laser ? "yes" : "",
       s.cew ? "yes" : "", s.hyper_drive.has ? "yes" : "",
       std::format("{}{}/{}{}", s.primary, caliber_char(s.primtype),
                   s.secondary, caliber_char(s.sectype)),
       std::format("{}", armor(s)), std::format("{:.0f}", s.tech),
       std::format("{}", max_speed(s)), std::format("{}", shipcost(s)),
       std::format("{:.1f}", mass(s)),
       std::format("{}{}", size(s), pod_suffix)});

  g.out << table << "\n";
}

void report_weapons(GameObj& g, RstContext& ctx, const Ship& s) {
  if (!ctx.flags.weapons) return;

  // Create table
  tabulate::Table table;

  // Format table: no borders, just spacing between columns
  table.format().hide_border().column_separator("  ");

  // Set column alignments and widths
  table.column(0).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(1).format().width(1).font_align(tabulate::FontAlign::center);
  table.column(2).format().width(14);
  table.column(3).format().width(5).font_align(tabulate::FontAlign::center);
  table.column(4).format().width(8).font_align(tabulate::FontAlign::center);
  table.column(5).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(6).format().width(8).font_align(tabulate::FontAlign::center);
  table.column(7).format().width(6).font_align(tabulate::FontAlign::center);
  table.column(8).format().width(8);

  // Add header row only on first call
  if (ctx.first) {
    table.add_row(
        {"#", "", "name", "laser", "cew", "safe", "guns", "damage", "class"});
    // Format header row with bold
    table[0].format().font_style({tabulate::FontStyle::bold});

    if (!ctx.flags.ship) ctx.first = false;
  }

  // Determine ship class string
  std::string ship_class;
  if ((s.type == ShipType::OTYPE_TERRA) || (s.type == ShipType::OTYPE_PLOW)) {
    ship_class = "Standard";
  } else {
    ship_class = s.shipclass;
  }

  // Add factory build type indicator if applicable
  std::string class_with_type;
  if (s.type == ShipType::OTYPE_FACTORY) {
    class_with_type = std::format("{} {}", Shipltrs[s.build_type], ship_class);
  } else {
    class_with_type = ship_class;
  }

  // Add data row
  table.add_row(
      {std::format("{}", s.number), std::format("{}", Shipltrs[s.type]),
       std::format("{}", s.active ? s.name : "INACTIVE"), s.laser ? "yes" : "",
       std::format("{}/{}", s.cew, s.cew_range),
       std::format("{}", (int)((1.0 - .01 * s.damage) * s.tech / 4.0)),
       std::format("{}{}/{}{}", s.primary, caliber_char(s.primtype),
                   s.secondary, caliber_char(s.sectype)),
       std::format("{}%", s.damage), class_with_type});

  g.out << table << "\n";
}

void report_factories(GameObj& g, RstContext& ctx, const Ship& s) {
  if (!ctx.flags.factories || s.type != ShipType::OTYPE_FACTORY) return;

  // Create table
  tabulate::Table table;

  // Format table: no borders, just spacing between columns
  table.format().hide_border().column_separator("  ");

  // Set column alignments and widths
  table.column(0).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(1).format().width(1).font_align(tabulate::FontAlign::center);
  table.column(2).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(3).format().width(6).font_align(tabulate::FontAlign::right);
  table.column(4).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(5).format().width(3).font_align(tabulate::FontAlign::right);
  table.column(6).format().width(2).font_align(tabulate::FontAlign::right);
  table.column(7).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(8).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(9).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(10).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(11).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(12).format().width(3).font_align(tabulate::FontAlign::center);
  table.column(13).format().width(7).font_align(tabulate::FontAlign::center);
  table.column(14).format().width(3).font_align(tabulate::FontAlign::center);
  table.column(15).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(16).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(17).format().width(4).font_align(tabulate::FontAlign::center);

  // Add header row only on first call
  if (ctx.first) {
    table.add_row({"#", "", "Cost", "Tech", "Mass", "Sz", "A", "Crw", "Ful",
                   "Crg", "Hng", "Dst", "Sp", "Weapons", "Lsr", "CEWs", "Range",
                   "Dmg"});
    // Format header row with bold
    table[0].format().font_style({tabulate::FontStyle::bold});

    if (!ctx.flags.ship) ctx.first = false;
  }

  // Handle special case for no ship type specified
  if ((s.build_type == 0) || (s.build_type == ShipType::OTYPE_FACTORY)) {
    table.add_row({std::format("{}", s.number), "", "", "", "", "", "", "", "",
                   "", "", "", "", "(No ship type specified yet)", "", "", "",
                   "75% (OFF)"});
    g.out << table << "\n";
    return;
  }

  // Build weapon strings
  std::string prim_guns =
      s.primtype ? std::format("{}{}", s.primary, caliber_char(s.primtype))
                 : "---";

  std::string sec_guns =
      s.sectype ? std::format("{}{}", s.secondary, caliber_char(s.sectype))
                : "---";

  std::string cew_str = s.cew ? std::format("{}", s.cew) : "----";
  std::string range_str = s.cew ? std::format("{}", s.cew_range) : "-----";

  // Build speed indicator (hyper + mount + speed)
  std::string speed_indicator;
  if (s.hyper_drive.has) {
    speed_indicator = s.mount ? "+" : "*";
  } else {
    speed_indicator = " ";
  }
  speed_indicator += std::format("{}", s.max_speed);

  // Build damage status
  std::string damage_status = std::format("{}%", s.damage);
  if (s.damage) {
    if (!s.on) damage_status += "*";
  }

  // Add data row
  table.add_row(
      {std::format("{}", s.number), std::format("{}", Shipltrs[s.build_type]),
       std::format("{}", s.build_cost), std::format("{:.1f}", s.complexity),
       std::format("{:.1f}", s.base_mass), std::format("{}", ship_size(s)),
       std::format("{}", s.armor), std::format("{}", s.max_crew),
       std::format("{}", s.max_fuel), std::format("{}", s.max_resource),
       std::format("{}", s.max_hanger), std::format("{}", s.max_destruct),
       speed_indicator, std::format("{}/{}", prim_guns, sec_guns),
       s.laser ? "yes" : " no", cew_str, range_str, damage_status});

  g.out << table << "\n";
}

void report_general(GameObj& g, RstContext& ctx, const Ship& s) {
  if (!ctx.flags.report) return;

  // Create table
  tabulate::Table table;

  // Format table: no borders, just spacing between columns
  table.format().hide_border().column_separator("  ");

  // Set column alignments and widths
  table.column(0).format().width(1).font_align(tabulate::FontAlign::center);
  table.column(1).format().width(5);
  table.column(2).format().width(12);
  table.column(3).format().width(2).font_align(tabulate::FontAlign::right);
  table.column(4).format().width(3).font_align(tabulate::FontAlign::right);
  table.column(5).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(6).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(7).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(8).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(9).format().width(2).font_align(tabulate::FontAlign::center);
  table.column(10).format().width(10);
  table.column(11).format().width(18);

  // Add header row only on first call
  if (ctx.first) {
    table.add_row({"", "#", "name", "gov", "dam", "crew", "mil", "des", "fuel",
                   "sp", "orbits", "destination"});
    // Format header row with bold
    table[0].format().font_style({tabulate::FontStyle::bold});

    if (!ctx.flags.ship) ctx.first = false;
  }

  // Build location/destination string
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

  // Build name string (may have special formatting for inactive ships)
  std::string name_str;
  if (!s.active) {
    name_str = std::format("INACTIVE({})", s.rad);
  } else {
    name_str = s.name;
  }

  // Build speed indicator
  std::string speed_indicator;
  if (s.hyper_drive.has) {
    speed_indicator = s.mounted ? "+" : "*";
  } else {
    speed_indicator = " ";
  }
  speed_indicator += std::format("{}", s.speed);

  // Add data row
  table.add_row({std::format("{}", Shipltrs[s.type]),
                 std::format("{}", s.number), name_str,
                 std::format("{}", s.governor), std::format("{}", s.damage),
                 std::format("{}", s.popn), std::format("{}", s.troops),
                 std::format("{}", s.destruct), std::format("{:.0f}", s.fuel),
                 speed_indicator, dispshiploc_brief(s), locstrn});

  g.out << table << "\n";
}

// ============================================================================
// FILTERING
// ============================================================================

bool should_report_ship(const Ship& s, player_t player_num, governor_t governor,
                        const ReportSet& rep_on) {
  // Don't report on ships that are dead
  if (!s.alive) return false;

  // Don't report on ships not owned by this player
  if (s.owner != player_num) return false;

  // Don't report on ships this governor is not authorized for
  if (!authorized(governor, s)) return false;

  // Don't report on ships whose type isn't in the requested report filter
  if (!rep_on.contains(Shipltrs[s.type])) return false;

  // Don't report on undocked canisters (launched canisters don't show up)
  if (s.type == ShipType::OTYPE_CANIST && !s.docked) return false;

  // Don't report on undocked greens (launched greens don't show up)
  if (s.type == ShipType::OTYPE_GREEN && !s.docked) return false;

  return true;
}

// ============================================================================
// MAIN REPORT DRIVER
// ============================================================================

void ship_report(GameObj& g, RstContext& ctx, const Ship& s,
                 const ReportSet& rep_on) {
  // Check if this ship should be reported
  if (!should_report_ship(s, g.player, g.governor, rep_on)) {
    return;
  }

  // Dispatch to appropriate report methods
  report_stock(g, ctx, s);
  report_status(g, ctx, s);
  report_weapons(g, ctx, s);
  report_factories(g, ctx, s);
  report_general(g, ctx, s);
}

// ============================================================================
// SHIP COLLECTION HELPERS
// ============================================================================

void report_planet_ships(GameObj& g, RstContext& ctx, player_t player_num,
                         starnum_t snum, planetnum_t pnum,
                         const ReportSet& rep_on) {
  const auto* planet = g.entity_manager.peek_planet(snum, pnum);
  if (!planet) return;

  if (planet->info(player_num - 1).explored) {
    const ShipList ships(g.entity_manager, planet->ships());
    for (const Ship* ship : ships) {
      ship_report(g, ctx, *ship, rep_on);
    }
  }
}

void report_star_ships(GameObj& g, RstContext& ctx, player_t player_num,
                       starnum_t snum, const ReportSet& rep_on) {
  const auto* star = g.entity_manager.peek_star(snum);
  if (!star) return;

  if (isset(star->explored(), player_num)) {
    const ShipList ships(g.entity_manager, star->ships());
    for (const Ship* ship : ships) {
      ship_report(g, ctx, *ship, rep_on);
    }

    for (planetnum_t i = 0; i < star->numplanets(); i++)
      report_planet_ships(g, ctx, player_num, snum, i, rep_on);
  }
}

}  // namespace

// ============================================================================
// COMMAND ENTRY POINT
// ============================================================================

namespace GB::commands {
void rst(const command_t& argv, GameObj& g) {
  ReportSet report_types;

  RstContext ctx;
  ctx.first = true;

  // Set report flags based on command name
  if (auto it = kCommandReportModes.find(argv[0]);
      it != kCommandReportModes.end()) {
    ctx.flags = it->second;
  }

  shipnum_t n_ships = Numships();

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

        const auto* ship = g.entity_manager.peek_ship(shipno);
        if (!ship) {
          g.out << std::format("rst: no such ship #{} \n", shipno);
          return;
        }

        // Report all types when reporting specific ships
        std::ranges::copy(Shipltrs,
                          std::inserter(report_types, report_types.end()));
        ship_report(g, ctx, *ship, report_types);
        l++;
      }
      return;
    }

    // Parse ship type letters and add to set - only valid ship letters
    std::ranges::copy_if(
        argv[1], std::inserter(report_types, report_types.end()),
        [](char c) { return std::ranges::contains(Shipltrs, c); });

    // Warn if no valid ship types were found
    if (report_types.empty()) {
      g.out << std::format("'{}' -- no valid ship letters found\n", argv[1]);
      return;
    }
  } else {
    // No ship type filter specified - report all types
    std::ranges::copy(Shipltrs,
                      std::inserter(report_types, report_types.end()));
  }

  switch (g.level) {
    case ScopeLevel::LEVEL_UNIV: {
      const ShipList univ_ships(g.entity_manager, Sdata.ships);
      for (const Ship* ship : univ_ships) {
        ship_report(g, ctx, *ship, report_types);
      }

      for (starnum_t i = 0; i < Sdata.numstars; i++)
        report_star_ships(g, ctx, g.player, i, report_types);
      break;
    }

    case ScopeLevel::LEVEL_PLAN:
      report_planet_ships(g, ctx, g.player, g.snum, g.pnum, report_types);
      break;

    case ScopeLevel::LEVEL_STAR:
      report_star_ships(g, ctx, g.player, g.snum, report_types);
      break;

    case ScopeLevel::LEVEL_SHIP: {
      if (g.shipno == 0) {
        g.out << "Error: No ship is currently scoped. Use 'cs #<shipno>' to "
                 "scope to a ship.\n";
        return;
      }

      const auto* scoped_ship = g.entity_manager.peek_ship(g.shipno);
      if (!scoped_ship) {
        g.out << std::format("Error: Unable to retrieve ship #{} data.\n",
                             g.shipno);
        return;
      }

      // Report on the scoped ship directly
      ship_report(g, ctx, *scoped_ship, report_types);

      // Report on ships docked in this ship
      const ShipList docked_ships(g.entity_manager, scoped_ship->ships);
      for (const Ship* ship : docked_ships) {
        ship_report(g, ctx, *ship, report_types);
      }
      break;
    }
  }
}
}  // namespace GB::commands
