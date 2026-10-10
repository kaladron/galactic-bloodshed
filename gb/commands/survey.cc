// SPDX-License-Identifier: Apache-2.0

/// \file survey.cc
/// \brief Detailed survey of planet, sector, or star.

module;

import gb.entities;
import gb.presentation.planet_views;
import gb.services;
import std;

module commands;

namespace {
constexpr int MAX_SHIPS_PER_SECTOR = 10;

struct SurveyRequest {
  Place where;
  bool all_sectors{false};
  std::string range_arg{};
};

// Helper: Parse survey arguments and determine location
std::optional<SurveyRequest> parse_survey_location(const command_t& argv,
                                                   GameObj& g) {
  if (argv.size() == 1 || argv[1].empty()) {
    // No args - use current scope
    return SurveyRequest{
        .where = Place(g.level(), g.snum(), g.pnum()),
    };
  }

  const bool is_range = std::isdigit(argv[1][0]) && argv[1].contains(',');
  const bool is_all = argv[1][0] == '-';
  if (is_range || is_all) {
    // Sector range or full survey
    if (g.level() != ScopeLevel::LEVEL_PLAN) {
      g.out << "There are no sectors here.\n";
      return std::nullopt;
    }
    return SurveyRequest{
        .where = Place(ScopeLevel::LEVEL_PLAN, g.snum(), g.pnum()),
        .all_sectors = is_all,
        .range_arg = is_range ? argv[1] : std::string{},
    };
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
  return SurveyRequest{.where = *where};
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
    if (!shipa.is_landed()) continue;
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
  const auto* smap = g.entity_manager.peek_sectormap(where.snum, where.pnum);

  sector_count_t crystal_count = 0;
  int total_fert = 0;
  resource_t total_resource = 0;
  for (const auto& s : *smap) {
    total_fert += s.get_fert();
    total_resource += s.get_resource();
    if ((race.discoveries.crystal || race.God) && s.get_crystals() > 0) {
      ++crystal_count;
    }
  }

  const auto& pinfo = p.info(g.player());
  const int num_sects = std::max(1, p.num_sectors());
  const GB::presentation::SurveyPlanetOverviewViewModel vm{
      .planet_name = star.get_planet_name(where.pnum),
      .star_name = star.get_name(),
      .gravity = p.gravity(),
      .absolute_coords = p.absolute_coordinates(star),
      .system_coords = p.system_coordinates(),
      .planet_conditions = p.conditions(),
      .race_conditions = race.conditions,
      .planet_temp = p.temp(),
      .race_temp = race.temp,
      .normal_temp = p.rtemp(),
      .toxicity = p.toxic(),
      .compatibility = p.compatibility(race),
      .avg_fertility = Percentage{total_fert / num_sects},
      .avg_resource = total_resource / num_sects,
      .crystal_sectors = crystal_count,
      .total_resources = p.total_resources(),
      .is_metamorph = race.Metamorph,
      .fuel_stock = pinfo.fuel,
      .resource_stock = pinfo.resource,
      .destruct_stock = pinfo.destruct,
      .popn = p.popn(),
      .maxpopn = p.maxpopn(),
      .slaved_to = p.slaved_to(),
  };
  g.present(vm);
}

// Helper: Survey star system
void survey_star(GameObj& g, const Place& where) {
  const auto& race = *g.race;
  const auto& star = *g.entity_manager.peek_star(where.snum);

  std::optional<int> stability = std::nullopt;
  if (race.tech >= TECH_SEE_STABILITY || race.God) {
    stability = star.stability();
  }

  const GB::presentation::SurveyStarViewModel vm{
      .star_name = star.get_name(),
      .coordinates = star.coordinates(),
      .is_god = race.God,
      .gravity = star.gravity(),
      .stability = stability,
      .temperature_class = star.temperature(),
      .planet_names = star.planet_names(),
  };
  g.present(vm);
}

}  // namespace

namespace GB::commands {

bool survey(const command_t& argv, GameObj& g) {
  auto req = parse_survey_location(argv, g);
  if (!req) return false;

  // Dispatch based on scope level
  switch (req->where.level) {
    case ScopeLevel::LEVEL_PLAN:
      if (!req->range_arg.empty() || req->all_sectors) {
        survey_planet_sectors(g, req->where, req->range_arg, req->all_sectors);
      } else {
        survey_planet_overview(g, req->where);
      }
      return true;

    case ScopeLevel::LEVEL_STAR:
      survey_star(g, req->where);
      return true;

    case ScopeLevel::LEVEL_UNIV:
      g.present(GB::presentation::SurveyUniverseViewModel{});
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