// SPDX-License-Identifier: Apache-2.0

/// \file rst.cc
/// \brief Ship reporting commands (report, stock, stats, weapons, factories)
///
/// Implements various reporting modes for ships including status,
/// stock levels, and factory configurations.
///
/// Note: Tactical reporting has been moved to tactical.cc

module;

import gb.entities;
import gb.presentation;
import gb.services;
import scnlib;
import std;
import tabulate;
#undef stdout

module commands;

namespace {

using ReportSet = std::unordered_set<char>;
using GB::presentation::RstFactoryRow;
using GB::presentation::RstGeneralRow;
using GB::presentation::RstReportFlags;
using GB::presentation::RstShipEntry;
using GB::presentation::RstStatusRow;
using GB::presentation::RstStockRow;
using GB::presentation::RstViewModel;
using GB::presentation::RstWeaponsRow;

constexpr RstReportFlags REPORT_MODE_FLAGS{.report = true};
constexpr RstReportFlags STOCK_MODE_FLAGS{.stock = true};
constexpr RstReportFlags SHIP_MODE_FLAGS{.status = true,
                                         .ship = true,
                                         .stock = true,
                                         .report = true,
                                         .weapons = true,
                                         .factories = true};
constexpr RstReportFlags STATS_MODE_FLAGS{.status = true};
constexpr RstReportFlags WEAPONS_MODE_FLAGS{.weapons = true};
constexpr RstReportFlags FACTORIES_MODE_FLAGS{.factories = true};

RstStockRow build_stock_row(const Ship& s) {
  return RstStockRow{
      .number = s.number(),
      .type_letter = s.type_letter(),
      .active = s.active(),
      .name = s.name(),
      .crystals = s.crystals(),
      .hanger = s.hanger(),
      .max_hanger = s.max_hanger(),
      .resource = s.resource(),
      .max_resource = s.max_resource_capacity(),
      .destruct = s.destruct(),
      .max_destruct = s.max_destruct_capacity(),
      .fuel = s.fuel(),
      .max_fuel = s.max_fuel_capacity(),
      .popn = s.popn(),
      .troops = s.troops(),
      .max_crew = s.max_crew_capacity(),
  };
}

RstStatusRow build_status_row(const Ship& s) {
  std::optional<int> pod_temp{std::nullopt};
  if (const auto* pod = s.as<SporePodShip>()) {
    pod_temp = pod->temperature();
  }
  return RstStatusRow{
      .number = s.number(),
      .type_letter = s.type_letter(),
      .active = s.active(),
      .name = s.name(),
      .has_laser = s.laser(),
      .has_cew = (s.cew() != 0),
      .has_hyperdrive = (s.hyper_drive().has != 0),
      .battery_summary = s.battery_summary(),
      .effective_armor = s.effective_armor(),
      .tech = s.tech(),
      .max_speed = s.max_speed_capacity(),
      .effective_cost = s.effective_cost(),
      .mass = s.mass(),
      .size = s.size(),
      .pod_temperature = pod_temp,
  };
}

RstWeaponsRow build_weapons_row(const Ship& s) {
  std::string ship_class = ((s.type() == ShipType::OTYPE_TERRA) ||
                            (s.type() == ShipType::OTYPE_PLOW))
                               ? "Standard"
                               : s.shipclass();
  std::string class_with_type =
      (s.type() == ShipType::OTYPE_FACTORY)
          ? std::format("{} {}", ship_template(s.build_type()).letter,
                        ship_class)
          : ship_class;
  return RstWeaponsRow{
      .number = s.number(),
      .type_letter = s.type_letter(),
      .active = s.active(),
      .name = s.name(),
      .has_laser = s.laser(),
      .cew = s.cew(),
      .cew_range = s.cew_range(),
      .safe_cew = static_cast<int>((1.0 - .01 * s.damage()) * s.tech() / 4.0),
      .battery_summary = s.battery_summary(),
      .damage = s.damage(),
      .ship_class = std::move(class_with_type),
  };
}

RstFactoryRow build_factory_row(const Ship& s) {
  const bool unconfigured = (s.build_type() == ShipType::STYPE_POD) ||
                            (s.build_type() == ShipType::OTYPE_FACTORY);
  if (unconfigured) {
    return RstFactoryRow{
        .number = s.number(),
        .unconfigured = true,
    };
  }

  return RstFactoryRow{
      .number = s.number(),
      .unconfigured = false,
      .build_type_letter = ship_template(s.build_type()).letter,
      .build_cost = s.build_cost(),
      .complexity = s.complexity(),
      .base_mass = s.base_mass(),
      .size = s.size(),
      .armor = s.armor(),
      .max_crew = s.max_crew_capacity(),
      .max_fuel = s.max_fuel_capacity(),
      .max_resource = s.max_resource_capacity(),
      .max_hanger = s.max_hanger(),
      .max_destruct = s.max_destruct_capacity(),
      .has_hyperdrive = (s.hyper_drive().has != 0),
      .has_mount = s.mount(),
      .max_speed = s.max_speed(),
      .primary_guns = s.primary_battery().has_guns()
                          ? s.primary_battery().to_string()
                          : "---",
      .secondary_guns = s.secondary_battery().has_guns()
                            ? s.secondary_battery().to_string()
                            : "---",
      .has_laser = s.laser(),
      .cew = s.cew(),
      .cew_range = s.cew_range(),
      .damage = s.damage(),
      .on = s.on(),
  };
}

RstGeneralRow build_general_row(EntityManager& em, const Ship& s) {
  return RstGeneralRow{
      .type_letter = s.type_letter(),
      .number = s.number(),
      .active = s.active(),
      .name = s.name(),
      .radiation = s.rad(),
      .governor = s.governor(),
      .damage = s.damage(),
      .popn = s.popn(),
      .troops = s.troops(),
      .destruct = s.destruct(),
      .fuel = s.fuel(),
      .has_hyperdrive = (s.hyper_drive().has != 0),
      .mounted = s.mounted(),
      .speed = s.speed(),
      .orbits = dispshiploc_brief(em, s),
      .docked = s.docked(),
      .whatdest = s.whatdest(),
      .dest_shipno = s.destshipno(),
      .land_coords = s.land_coords(),
      .navigating = s.navigate().on,
      .nav_bearing = s.navigate().bearing,
      .nav_turns = s.navigate().turns,
      .dest_path = format_ship_dest(em, s),
  };
}

void render_stock_row(GameObj& g, bool& first, bool per_ship_header,
                      const RstStockRow& row) {
  tabulate::Table table;
  table.format().hide_border().column_separator("  ");
  table.column(0).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(1).format().width(16);
  table.column(2).format().width(3).font_align(tabulate::FontAlign::center);
  table.column(3).format().width(10).font_align(tabulate::FontAlign::center);
  table.column(4).format().width(12).font_align(tabulate::FontAlign::center);
  table.column(5).format().width(12).font_align(tabulate::FontAlign::center);
  table.column(6).format().width(14).font_align(tabulate::FontAlign::center);
  table.column(7).format().width(12).font_align(tabulate::FontAlign::center);

  if (first) {
    table.add_row(
        {"#", "name", "x", "hanger", "res", "des", "fuel", "crew/mil"});
    table[0].format().font_style({tabulate::FontStyle::bold});
    if (!per_ship_header) first = false;
  }

  table.add_row(
      {std::format("{}", row.number),
       std::format("{}{} {}", row.type_letter, row.crystals ? 'x' : ' ',
                   row.active ? row.name : "INACTIVE"),
       std::format("{}", row.crystals),
       std::format("{}:{}", row.hanger, row.max_hanger),
       std::format("{}:{}", row.resource, row.max_resource),
       std::format("{}:{}", row.destruct, row.max_destruct),
       std::format("{:.1f}:{}", row.fuel, row.max_fuel),
       std::format("{}/{}:{}", row.popn, row.troops, row.max_crew)});

  g.out << table << "\n";
}

void render_status_row(GameObj& g, bool& first, bool per_ship_header,
                       const RstStatusRow& row) {
  tabulate::Table table;
  table.format().hide_border().column_separator("  ");
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

  if (first) {
    table.add_row({"#", "", "name", "las", "cew", "hyp", "guns", "arm", "tech",
                   "spd", "cost", "mass", "size"});
    table[0].format().font_style({tabulate::FontStyle::bold});
    if (!per_ship_header) first = false;
  }

  const std::string pod_suffix =
      row.pod_temperature ? std::format(" ({})", *row.pod_temperature) : "";

  table.add_row(
      {std::format("{}", row.number), std::format("{}", row.type_letter),
       row.active ? row.name : "INACTIVE", row.has_laser ? "yes" : "",
       row.has_cew ? "yes" : "", row.has_hyperdrive ? "yes" : "",
       row.battery_summary, std::format("{}", row.effective_armor),
       std::format("{:.0f}", row.tech), std::format("{}", row.max_speed),
       std::format("{}", row.effective_cost), std::format("{:.1f}", row.mass),
       std::format("{}{}", row.size, pod_suffix)});

  g.out << table << "\n";
}

void render_weapons_row(GameObj& g, bool& first, bool per_ship_header,
                        const RstWeaponsRow& row) {
  tabulate::Table table;
  table.format().hide_border().column_separator("  ");
  table.column(0).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(1).format().width(1).font_align(tabulate::FontAlign::center);
  table.column(2).format().width(14);
  table.column(3).format().width(5).font_align(tabulate::FontAlign::center);
  table.column(4).format().width(8).font_align(tabulate::FontAlign::center);
  table.column(5).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(6).format().width(8).font_align(tabulate::FontAlign::center);
  table.column(7).format().width(6).font_align(tabulate::FontAlign::center);
  table.column(8).format().width(8);

  if (first) {
    table.add_row(
        {"#", "", "name", "laser", "cew", "safe", "guns", "damage", "class"});
    table[0].format().font_style({tabulate::FontStyle::bold});
    if (!per_ship_header) first = false;
  }

  table.add_row({std::format("{}", row.number),
                 std::format("{}", row.type_letter),
                 row.active ? row.name : "INACTIVE", row.has_laser ? "yes" : "",
                 std::format("{}/{}", row.cew, row.cew_range),
                 std::format("{}", row.safe_cew), row.battery_summary,
                 std::format("{}%", row.damage), row.ship_class});

  g.out << table << "\n";
}

void render_factory_row(GameObj& g, bool& first, bool per_ship_header,
                        const RstFactoryRow& row) {
  tabulate::Table table;
  table.format().hide_border().column_separator("  ");
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

  if (first) {
    table.add_row({"#", "", "Cost", "Tech", "Mass", "Sz", "A", "Crw", "Ful",
                   "Crg", "Hng", "Dst", "Sp", "Weapons", "Lsr", "CEWs", "Range",
                   "Dmg"});
    table[0].format().font_style({tabulate::FontStyle::bold});
    if (!per_ship_header) first = false;
  }

  if (row.unconfigured) {
    table.add_row({std::format("{}", row.number), "", "", "", "", "", "", "",
                   "", "", "", "", "", "(No ship type specified yet)", "", "",
                   "", "75% (OFF)"});
    g.out << table << "\n";
    return;
  }

  const std::string speed_indicator = std::format(
      "{}{}", row.has_hyperdrive ? (row.has_mount ? "+" : "*") : " ",
      row.max_speed);

  std::string damage_status = std::format("{}%", row.damage);
  if (row.damage && !row.on) {
    damage_status += "*";
  }

  table.add_row(
      {std::format("{}", row.number), std::format("{}", row.build_type_letter),
       std::format("{}", row.build_cost), std::format("{:.1f}", row.complexity),
       std::format("{:.1f}", row.base_mass), std::format("{}", row.size),
       std::format("{}", row.armor), std::format("{}", row.max_crew),
       std::format("{}", row.max_fuel), std::format("{}", row.max_resource),
       std::format("{}", row.max_hanger), std::format("{}", row.max_destruct),
       speed_indicator,
       std::format("{}/{}", row.primary_guns, row.secondary_guns),
       row.has_laser ? "yes" : " no",
       row.cew ? std::format("{}", row.cew) : "----",
       row.cew ? std::format("{}", row.cew_range) : "-----", damage_status});

  g.out << table << "\n";
}

void render_general_row(GameObj& g, bool& first, bool per_ship_header,
                        const RstGeneralRow& row) {
  tabulate::Table table;
  table.format().hide_border().column_separator("  ");
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

  if (first) {
    table.add_row({"", "#", "name", "gov", "dam", "crew", "mil", "des", "fuel",
                   "sp", "orbits", "destination"});
    table[0].format().font_style({tabulate::FontStyle::bold});
    if (!per_ship_header) first = false;
  }

  const std::string name_str =
      row.active ? row.name : std::format("INACTIVE({})", row.radiation);
  const std::string speed_indicator = std::format(
      "{}{}", row.has_hyperdrive ? (row.mounted ? "+" : "*") : " ", row.speed);
  std::string destination;
  if (row.docked) {
    if (row.whatdest == ScopeLevel::LEVEL_SHIP) {
      destination = std::format("D#{}", row.dest_shipno.value_or(0));
    } else {
      destination =
          std::format("L{:2},{:<2}", row.land_coords.x, row.land_coords.y);
    }
  } else if (row.navigating) {
    destination = std::format("nav:{} ({})", row.nav_bearing, row.nav_turns);
  } else {
    destination = row.dest_path;
  }

  table.add_row(
      {std::format("{}", row.type_letter), std::format("{}", row.number),
       name_str, std::format("{}", row.governor), std::format("{}", row.damage),
       std::format("{}", row.popn), std::format("{}", row.troops),
       std::format("{}", row.destruct), std::format("{:.0f}", row.fuel),
       speed_indicator, row.orbits, destination});

  g.out << table << "\n";
}

void render_rst_view_model(GameObj& g, const RstViewModel& vm) {
  bool first = true;
  for (const auto& entry : vm.ships) {
    if (entry.stock) {
      render_stock_row(g, first, vm.flags.ship, *entry.stock);
    }
    if (entry.status) {
      render_status_row(g, first, vm.flags.ship, *entry.status);
    }
    if (entry.weapons) {
      render_weapons_row(g, first, vm.flags.ship, *entry.weapons);
    }
    if (entry.factory) {
      render_factory_row(g, first, vm.flags.ship, *entry.factory);
    }
    if (entry.general) {
      render_general_row(g, first, vm.flags.ship, *entry.general);
    }
  }
}

bool should_report_ship(const Ship& s, player_t player_num, governor_t governor,
                        const ReportSet& rep_on) {
  if (!s.alive()) return false;
  if (s.owner() != player_num) return false;
  if (!s.is_authorized_for(governor)) return false;
  if (!rep_on.contains(s.type_letter())) return false;
  if (s.type() == ShipType::OTYPE_CANIST && !s.docked()) return false;
  if (s.type() == ShipType::OTYPE_GREEN && !s.docked()) return false;
  return true;
}

void collect_ship_report(GameObj& g, RstViewModel& vm, const Ship& s,
                         const ReportSet& rep_on) {
  if (!should_report_ship(s, g.player(), g.governor(), rep_on)) {
    return;
  }

  RstShipEntry entry{};
  if (vm.flags.stock) {
    entry.stock = build_stock_row(s);
  }
  if (vm.flags.status) {
    entry.status = build_status_row(s);
  }
  if (vm.flags.weapons) {
    entry.weapons = build_weapons_row(s);
  }
  if (vm.flags.factories && s.type() == ShipType::OTYPE_FACTORY) {
    entry.factory = build_factory_row(s);
  }
  if (vm.flags.report) {
    entry.general = build_general_row(g.entity_manager, s);
  }
  vm.ships.push_back(std::move(entry));
}

void collect_planet_ships(GameObj& g, RstViewModel& vm, player_t player_num,
                          starnum_t snum, planetnum_t pnum,
                          const ReportSet& rep_on) {
  const auto* planet = g.entity_manager.peek_planet(snum, pnum);
  if (!planet) return;

  if (planet->info(player_num).explored) {
    for (const Ship& ship :
         ShipList::readonly_on_planet(g.entity_manager, snum, pnum)) {
      collect_ship_report(g, vm, ship, rep_on);
    }
  }
}

void collect_star_ships(GameObj& g, RstViewModel& vm, player_t player_num,
                        starnum_t snum, const ReportSet& rep_on) {
  const auto* star = g.entity_manager.peek_star(snum);
  if (!star) return;

  if (star->is_explored_by(player_num)) {
    for (const Ship& ship :
         ShipList::readonly_in_star(g.entity_manager, snum)) {
      collect_ship_report(g, vm, ship, rep_on);
    }

    for (const auto& planet :
         PlanetList::readonly(g.entity_manager, snum, star->numplanets())) {
      collect_planet_ships(g, vm, player_num, snum, planet.planet_order(),
                           rep_on);
    }
  }
}

bool run_rst(const command_t& argv, GameObj& g, const RstReportFlags& flags) {
  ReportSet report_types;
  RstViewModel vm{.flags = flags};

  const shipnum_t n_ships = g.entity_manager.num_ships();

  if (argv.size() >= 2) {
    if (*argv[1].c_str() == '#' || std::isdigit(*argv[1].c_str())) {
      for (std::size_t l = 1; l < argv.size(); ++l) {
        std::string_view arg_sv = argv[l];
        if (!arg_sv.empty() && arg_sv.front() == '#') {
          arg_sv.remove_prefix(1);
        }
        auto scan_res = scn::scan<underlying_type_t<shipnum_t>>(arg_sv, "{}");
        if (!scan_res) {
          g.out << std::format("rst: invalid ship argument {}\n", argv[l]);
          return false;
        }
        const shipnum_t shipno{scan_res->value()};
        if (shipno > n_ships || shipno < 1) {
          g.out << std::format("rst: no such ship #{} \n", shipno);
          return false;
        }

        const auto* ship = g.entity_manager.peek_ship(shipno);
        if (!ship) {
          g.out << std::format("rst: no such ship #{} \n", shipno);
          return false;
        }

        auto all_letters = get_all_ship_letters();
        std::ranges::copy(all_letters,
                          std::inserter(report_types, report_types.end()));
        collect_ship_report(g, vm, *ship, report_types);
      }
      render_rst_view_model(g, vm);
      return true;
    }

    std::ranges::copy_if(argv[1],
                         std::inserter(report_types, report_types.end()),
                         [](char c) { return is_valid_ship_letter(c); });

    if (report_types.empty()) {
      g.out << std::format("'{}' -- no valid ship letters found\n", argv[1]);
      return false;
    }
  } else {
    auto all_letters = get_all_ship_letters();
    std::ranges::copy(all_letters,
                      std::inserter(report_types, report_types.end()));
  }

  switch (g.level()) {
    case ScopeLevel::LEVEL_UNIV: {
      for (const Ship& ship :
           ShipList::readonly(g.entity_manager, ScopeLevel::LEVEL_UNIV)) {
        collect_ship_report(g, vm, ship, report_types);
      }

      for (const Star& star : StarList::readonly(g.entity_manager)) {
        collect_star_ships(g, vm, g.player(), star.star_id(), report_types);
      }
      break;
    }

    case ScopeLevel::LEVEL_PLAN:
      collect_planet_ships(g, vm, g.player(), g.snum(), g.pnum(), report_types);
      break;

    case ScopeLevel::LEVEL_STAR:
      collect_star_ships(g, vm, g.player(), g.snum(), report_types);
      break;

    case ScopeLevel::LEVEL_SHIP: {
      if (g.shipno() == 0) {
        g.out << "Error: No ship is currently scoped. Use 'cs #<shipno>' to "
                 "scope to a ship.\n";
        return false;
      }

      const auto* scoped_ship = g.entity_manager.peek_ship(g.shipno());
      if (!scoped_ship) {
        g.out << std::format("Error: Unable to retrieve ship #{} data.\n",
                             g.shipno());
        return false;
      }

      collect_ship_report(g, vm, *scoped_ship, report_types);

      for (const Ship& ship : ShipList::readonly_in_carrier(
               g.entity_manager, scoped_ship->number())) {
        collect_ship_report(g, vm, ship, report_types);
      }
      break;
    }
  }

  render_rst_view_model(g, vm);
  return true;
}

bool report_handler(const command_t& argv, GameObj& g) {
  return run_rst(argv, g, REPORT_MODE_FLAGS);
}

bool ship_handler(const command_t& argv, GameObj& g) {
  return run_rst(argv, g, SHIP_MODE_FLAGS);
}

bool stats_handler(const command_t& argv, GameObj& g) {
  return run_rst(argv, g, STATS_MODE_FLAGS);
}

bool stock_handler(const command_t& argv, GameObj& g) {
  return run_rst(argv, g, STOCK_MODE_FLAGS);
}

bool weapons_handler(const command_t& argv, GameObj& g) {
  return run_rst(argv, g, WEAPONS_MODE_FLAGS);
}

bool factories_handler(const command_t& argv, GameObj& g) {
  return run_rst(argv, g, FACTORIES_MODE_FLAGS);
}

}  // namespace

namespace GB::commands {

const CommandDescriptor report_cmd{
    .name = "report",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "report [<shiptypes> | #<shipno> ...]",
    .description = "Summary report of ships and craft",
    .handler = &report_handler,
};

const CommandDescriptor ship_cmd{
    .name = "ship",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "ship [<shiptypes> | #<shipno> ...]",
    .description =
        "Comprehensive status, stock, and capability report of ships",
    .handler = &ship_handler,
};

const CommandDescriptor stats_cmd{
    .name = "stats",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "stats [<shiptypes> | #<shipno> ...]",
    .description = "Combat and capability statistics report of ships",
    .handler = &stats_handler,
};

const CommandDescriptor stock_cmd{
    .name = "stock",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "stock [<shiptypes> | #<shipno> ...]",
    .description = "Inventory, cargo, and resource stock report of ships",
    .handler = &stock_handler,
};

const CommandDescriptor weapons_cmd{
    .name = "weapons",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "weapons [<shiptypes> | #<shipno> ...]",
    .description = "Weaponry, ordnance, and combat loadout report of ships",
    .handler = &weapons_handler,
};

const CommandDescriptor factories_cmd{
    .name = "factories",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "factories [<shiptypes> | #<shipno> ...]",
    .description =
        "Factory configuration and production capability report of ships",
    .handler = &factories_handler,
};

}  // namespace GB::commands
