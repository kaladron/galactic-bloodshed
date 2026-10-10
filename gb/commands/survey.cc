// SPDX-License-Identifier: Apache-2.0

/// \file survey.cc
/// \brief Detailed survey of planet, sector, or star.

module;

import gb.entities;
import gb.presentation.planet_views;
import gb.services;
import std;
#undef stdout

module commands;

namespace {
constexpr int MAX_SHIPS_PER_SECTOR = 10;

constexpr std::array<std::string_view, 12> kToxDescriptions = {
    "Stage 0, mild",
    "Stage 1, mild",
    "Stage 2, semi-mild",
    "Stage 3, semi-semi mild",
    "Stage 4, ecologically unsound",
    "Stage 5: ecologically unsound",
    "Stage 6: below birth threshold",
    "Stage 7: ecologically unstable--below birth threshold",
    "Stage 8: ecologically poisonous --below birth threshold",
    "Stage 9: WARNING: nearing 100% toxicity",
    "Stage 10: WARNING: COMPLETELY TOXIC!!!",
    "???"};

std::string_view get_tox(int toxicity) {
  int index = toxicity / 10;
  if (index < 0) {
    index = 0;
  } else if (index > 10) {
    index = 11;  // "???" for toxicity > 100%
  }
  return kToxDescriptions[index];
}

std::string_view stability_label(int pct) {
  if (pct < 20) return "stable";
  if (pct < 40) return "unstable";
  if (pct < 60) return "dangerous";
  if (pct < 100) return "WARNING! Nova iminent!";
  return "undergoing nova";
}

// Helper: Parse survey arguments and determine location
std::optional<Place> parse_survey_location(const command_t& argv, GameObj& g,
                                           bool& all, std::string& range_arg) {
  all = false;
  range_arg.clear();

  if (argv.size() == 1) {
    // No args - use current scope
    return Place(g.level(), g.snum(), g.pnum());
  }

  // Parse argument to determine survey type
  if (argv[1].empty()) {
    return Place(g.level(), g.snum(), g.pnum());
  }

  if ((std::isdigit(argv[1][0]) && argv[1].contains(',')) ||
      ((argv[1][0] == '-') && (all = true))) {
    // Sector range or full survey
    if (g.level() != ScopeLevel::LEVEL_PLAN) {
      g.out << "There are no sectors here.\n";
      return std::nullopt;
    }
    if (!all) {
      range_arg = argv[1];
    }
    return Place(ScopeLevel::LEVEL_PLAN, g.snum(), g.pnum());
  }

  // Survey a named location
  auto where = Place::resolve(g.entity_manager, g.scope_context(), argv[1]);
  if (!where) {
    g.out << format_place_error(where.error());
    return std::nullopt;
  }
  if (where->level == ScopeLevel::LEVEL_SHIP) {
    return std::nullopt;
  }
  return *where;
}

bool has_planetary_survey_presence(GameObj& g, const Place& where,
                                   const Planet& p) {
  if (p.info(g.player()).numsectsowned > 0) return true;
  for (const Ship& shipa :
       ShipList::readonly_on_planet(g.entity_manager, where.snum, where.pnum)) {
    if (shipa.owner() == g.player() && shipa.is_authorized_for(g.governor()) &&
        shipa.has_sight()) {
      return true;
    }
  }
  return false;
}

std::flat_map<Coordinates, std::vector<GB::presentation::SurveySectorShip>>
collect_landed_sector_ships(GameObj& g, const Place& where, const Planet& p) {
  std::flat_map<Coordinates, std::vector<GB::presentation::SurveySectorShip>>
      shiplocs;
  if (!has_planetary_survey_presence(g, where, p)) {
    return shiplocs;
  }
  for (const Ship& shipa :
       ShipList::readonly_on_planet(g.entity_manager, where.snum, where.pnum)) {
    if (!shipa.alive() || !shipa.is_landed()) continue;
    auto& loc = shiplocs[shipa.land_coords()];
    if (std::ssize(loc) < MAX_SHIPS_PER_SECTOR) {
      loc.push_back({.shipno = shipa.number(),
                     .ltr = shipa.type_letter(),
                     .owner = shipa.owner()});
    }
  }
  return shiplocs;
}

// Helper: Survey planet sectors (detailed sector-by-sector view)
void survey_planet_sectors(GameObj& g, const Place& where,
                           const std::string& range_arg, bool all) {
  const auto& race = *g.race;
  const auto& star = *g.entity_manager.peek_star(where.snum);
  const auto& p = *g.entity_manager.peek_planet(where.snum, where.pnum);
  const auto* smap = g.entity_manager.peek_sectormap(where.snum, where.pnum);

  // Determine sector range
  std::pair<Coordinates, Coordinates> bounds{
      Coordinates{0, 0}, p.dimensions() - Coordinates{1, 1}};

  if (!all) {
    auto coords = get4args(range_arg);
    if (!coords) {
      g.out << "Invalid coordinate format. Use: x,y or xl:xh,yl:yh\n";
      return;
    }
    bounds = *coords;
  }
  const auto [low, high] = bounds;

  auto shiplocs = collect_landed_sector_ships(g, where, p);
  const double compat = p.compatibility(race);
  const auto& pinfo = p.info(g.player());

  GB::presentation::SurveySectorViewModel vm{
      .dimensions = p.dimensions(),
      .star_name = star.get_name(),
      .planet_name = star.get_planet_name(where.pnum),
      .resource_stock = pinfo.resource,
      .fuel_stock = pinfo.fuel,
      .destruct_stock = pinfo.destruct,
      .popn = p.popn(),
      .maxpopn = p.maxpopn(),
      .toxicity = p.toxic(),
      .compatibility = compat,
      .enslaved_to = p.slaved_to().value_or(0),
  };

  for (auto [c, s] : smap->indexed_sectors()) {
    if (!c.within_bounds(low, high)) continue;

    std::vector<GB::presentation::SurveySectorShip> ships;
    if (auto it = shiplocs.find(c); it != shiplocs.end()) {
      ships = it->second;
    }
    vm.sectors.push_back({
        .coords = c,
        .sect_char = s.condition_symbol(),
        .type_char = s.type_symbol(),
        .desshow_char = desshow(g.player(), g.governor(), race, s),
        .wasted = s.is_wasted(),
        .owner = s.get_owner(),
        .eff = s.get_eff(),
        .fert = s.get_fert(),
        .mobilization = s.get_mobilization(),
        .crystals =
            s.get_crystals() > 0 && (race.discoveries.crystal || race.God),
        .resource = s.get_resource(),
        .popn = s.get_popn(),
        .troops = s.get_troops(),
        .maxpopn = maxsupport(race, s, compat, p.toxic()),
        .ships = std::move(ships),
    });
  }

  g.present(vm);
}

// Helper: Survey planet overview (conditions, stats, etc.)
void survey_planet_overview(GameObj& g, const Place& where) {
  const auto& race = *g.race;

  const auto& star = *g.entity_manager.peek_star(where.snum);

  const auto& p = *g.entity_manager.peek_planet(where.snum, where.pnum);

  g.out << std::format("{}:\n", star.get_planet_name(where.pnum));
  g.out << std::format("gravity   x,y absolute     x,y relative to {}\n",
                       star.get_name());
  const auto abs_coords = p.absolute_coordinates(star);
  const auto sys_coords = p.system_coordinates();
  g.out << std::format("{:7.2f}   {:7.1f},{:7.1f}   {:8.1f},{:8.1f}\n",
                       p.gravity(), abs_coords.x, abs_coords.y, sys_coords.x,
                       sys_coords.y);
  g.out << "======== Planetary conditions: ========\n";
  g.out << "atmosphere concentrations:\n";
  g.out << std::format(
      "     methane {:02d}%({:02d}%)     oxygen {:02d}%({:02d}%)\n",
      p.conditions(METHANE), race.conditions[METHANE], p.conditions(OXYGEN),
      race.conditions[OXYGEN]);
  g.out << std::format("         CO2 {:02d}%({:02d}%)   hydrogen "
                       "{:02d}%({:02d}%)      temperature: {:3d} ({:3d})\n",
                       p.conditions(CO2), race.conditions[CO2],
                       p.conditions(HYDROGEN), race.conditions[HYDROGEN],
                       p.temp(), race.temp);
  g.out << std::format("    nitrogen {:02d}%({:02d}%)     sulfur "
                       "{:02d}%({:02d}%)           normal: {:3d}\n",
                       p.conditions(NITROGEN), race.conditions[NITROGEN],
                       p.conditions(SULFUR), race.conditions[SULFUR],
                       p.rtemp());
  g.out << std::format(
      "      helium {:02d}%({:02d}%)      other {:02d}%({:02d}%)\n",
      p.conditions(HELIUM), race.conditions[HELIUM], p.conditions(OTHER),
      race.conditions[OTHER]);

  g.out << std::format("                     Toxicity: {}% ({})\n", p.toxic(),
                       get_tox(p.toxic()));
  g.out << std::format("Total planetary compatibility: {:.2f}%\n",
                       p.compatibility(race));

  const auto* smap = g.entity_manager.peek_sectormap(where.snum, where.pnum);

  int crystal_count = 0;
  int avg_fert = 0;
  int avg_resource = 0;
  for (const auto& s : *smap) {
    avg_fert += s.get_fert();
    avg_resource += s.get_resource();
    if (race.discoveries.crystal || race.God) {
      crystal_count += !!s.get_crystals();
    }
  }

  g.out << std::format("{:>29}: {}\n{:>29}: {}\n{:>29}: {}\n",
                       "Average fertility", avg_fert / p.num_sectors(),
                       "Average resource", avg_resource / p.num_sectors(),
                       "Crystal sectors", crystal_count);
  g.out << std::format("{:>29}: {}\n", "Total resource deposits",
                       p.total_resources());
  g.out << std::format("fuel_stock  resource_stock dest_pot.   {}    ^{}\n",
                       race.Metamorph ? "biomass" : "popltn",
                       race.Metamorph ? "biomass" : "popltn");
  g.out << std::format("{:10}  {:14} {:9}  {:7}{:11}\n",
                       p.info(g.player()).fuel, p.info(g.player()).resource,
                       p.info(g.player()).destruct, p.popn(), p.maxpopn());
  if (p.slaved_to()) {
    g.out << std::format("This planet ENSLAVED to player {}!\n",
                         *p.slaved_to());
  }
}

// Helper: Survey star system
void survey_star(GameObj& g, const Place& where) {
  const auto& race = *g.race;

  const auto& star = *g.entity_manager.peek_star(where.snum);

  g.out << std::format("Star {}\n", star.get_name());
  g.out << std::format("locn: {}\n", star.coordinates());

  if (race.God) {
    for (const auto& name : star.planet_names()) {
      g.out << std::format(" \"{}\"\n", name);
    }
  }

  g.out << std::format("Gravity: {:.2f}\tInstability: ", star.gravity());

  if (race.tech >= TECH_SEE_STABILITY || race.God) {
    g.out << std::format("{}% ({})\n", star.stability(),
                         stability_label(star.stability()));
  } else {
    g.out << "(cannot determine)\n";
  }

  g.out << std::format("temperature class (1->10) {}\n", star.temperature());
  g.out << std::format("{} planets are ", star.numplanets());
  for (const auto& name : star.planet_names()) {
    g.out << std::format("{} ", name);
  }
  g.out << "\n";
}

}  // namespace

namespace GB::commands {

bool survey(const command_t& argv, GameObj& g) {
  // Parse arguments and determine what to survey
  bool all = false;
  std::string range_arg;
  auto where_opt = parse_survey_location(argv, g, all, range_arg);
  if (!where_opt) return false;

  const auto& where = *where_opt;

  // Dispatch based on scope level
  switch (where.level) {
    case ScopeLevel::LEVEL_PLAN:
      // Check if this is a sector survey or planet overview
      if ((argv.size() > 1 && std::isdigit(argv[1][0]) &&
           argv[1].contains(',')) ||
          all) {
        survey_planet_sectors(g, where, range_arg, all);
      } else {
        survey_planet_overview(g, where);
      }
      return true;

    case ScopeLevel::LEVEL_STAR:
      survey_star(g, where);
      return true;

    case ScopeLevel::LEVEL_UNIV:
      g.out << "It's just _there_, you know?\n";
      return true;

    default:
      g.out << "Illegal scope.\n";
      return false;
  }
}

const CommandDescriptor survey_cmd{
    .name = "survey",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "survey [<coords>|<range>|<place>]",
    .description = "Detailed survey of planet, sector, or star",
    .handler = &survey,
};

}  // namespace GB::commands