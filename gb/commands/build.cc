// SPDX-License-Identifier: Apache-2.0

/// \file build.cc
/// \brief Ship construction on planets and by builder ships.

module;

import gb.entities;
import gb.mechanics;
import gb.presentation.construction;
import gb.services;
import std;

module commands;

namespace {

bool handle_build_info_query(const command_t& argv, GameObj& g) {
  if (argv.size() == 2) {
    g.present(GB::presentation::build_ship_specs_table_vm(*g.race));
    return true;
  }
  const auto ship_type = get_build_type(argv[2][0]);
  if (!ship_type) {
    g.out << "No such ship type.\n";
    return false;
  }
  const auto& tmpl = ship_template(*ship_type);
  if (!tmpl.is_programmed) {
    g.out << "This ship type has not been programmed.\n";
    return false;
  }
  const auto* exam = g.entity_manager.peek_ship_exam(*ship_type);
  const std::optional<std::string_view> desc =
      exam ? std::optional<std::string_view>{exam->description} : std::nullopt;
  g.present(
      GB::presentation::build_ship_spec_detail_vm(*ship_type, *g.race, desc));
  return true;
}

int parse_ship_build_count(const command_t& argv, bool is_factory) {
  if (is_factory) {
    if (argv.size() >= 2 &&
        (std::isdigit(static_cast<unsigned char>(argv[1][0])) ||
         argv[1][0] == '-')) {
      return getcount(argv, 1);
    }
    return getcount(argv, 2);
  }
  if (argv.size() >= 4) {
    return getcount(argv, 3);
  }
  return getcount(argv, 2);
}

struct PlanetBuildPlan {
  ShipType what;
  Coordinates coords;
  int count;
};

std::optional<PlanetBuildPlan> validate_planet_build(const command_t& argv,
                                                     GameObj& g, starnum_t snum,
                                                     planetnum_t pnum) {
  const auto& race = *g.race;
  if (argv.size() < 2) {
    g.out << "Build what?\n";
    return std::nullopt;
  }
  auto what = get_build_type(argv[1][0]);
  if (!what) {
    g.out << "No such ship type.\n";
    return std::nullopt;
  }
  auto buildresult = can_build_this(*what, race);
  if (!buildresult && !race.God) {
    g.out << buildresult.error();
    return std::nullopt;
  }
  if (!ship_template(*what).can_build_on_planet() && !race.God) {
    g.out << "This ship cannot be built by a planet.\n";
    return std::nullopt;
  }
  if (argv.size() < 3) {
    g.out << "Build where?\n";
    return std::nullopt;
  }
  const auto& planet = *g.entity_manager.peek_planet(snum, pnum);
  const auto& star = *g.entity_manager.peek_star(snum);
  if (!race.God) {
    if (const auto res =
            can_build_at_planet(g.player(), g.governor(), star, planet);
        !res) {
      g.out << GB::presentation::format_planet_build_error(res.error());
      g.out << "You can't build that here.\n";
      return std::nullopt;
    }
  }
  auto coords_opt = Coordinates::parse(argv[2]);
  if (!coords_opt) {
    g.out << "Invalid sector format. Use: x,y\n";
    return std::nullopt;
  }
  Coordinates build_coords = *coords_opt;
  if (!planet.is_valid(build_coords)) {
    g.out << "Illegal sector.\n";
    return std::nullopt;
  }
  const auto& sectormap = *g.entity_manager.peek_sectormap(snum, pnum);
  const auto& sector = sectormap.get(build_coords);
  auto result = can_build_on_sector(g.entity_manager, *what, race, planet,
                                    sector, build_coords);
  if (!result && !race.God) {
    g.out << result.error();
    return std::nullopt;
  }
  int count = getcount(argv, 3);
  if (!count) {
    g.out << "Give a positive number of builds.\n";
    return std::nullopt;
  }
  return PlanetBuildPlan{.what = *what, .coords = build_coords, .count = count};
}

bool execute_single_planet_build(GameObj& g, const PlanetBuildPlan& plan,
                                 starnum_t snum, planetnum_t pnum) {
  const player_t Playernum = g.player();
  const governor_t Governor = g.governor();
  const auto& race = *g.race;
  auto newship = getship(plan.what, race);
  bool built = false;

  g.entity_manager.mutate_planet(snum, pnum, [&](Planet& planet) {
    const resource_t shipcost = newship->build_cost();
    if (shipcost > planet.info(Playernum).resource) {
      g.out << std::format("You need {}r to construct this ship.\n", shipcost);
      return;
    }
    if (!g.deduct_ap(snum, 1)) {
      g.out << "You don't have 1 action points there.\n";
      return;
    }
    g.present(create_ship_by_planet(g.entity_manager, Playernum, Governor, race,
                                    *newship, planet, snum, pnum, plan.coords));
    std::pair<population_t, fuel_t> loaded{0, 0.0};
    if (g.current_governor().toggle.autoload &&
        plan.what != ShipType::OTYPE_TRANSDEV && !race.God) {
      g.entity_manager.mutate_sectormap(snum, pnum, [&](SectorMap& sectormap) {
        auto& sector = sectormap.get(plan.coords);
        loaded = autoload_at_planet(Playernum, *newship, planet, sector);
      });
    }
    const auto [load_crew, load_fuel] = loaded;
    g.present(
        initialize_new_ship(race, Governor, *newship, load_fuel, load_crew));
    g.entity_manager.create_ship(std::move(newship));
    built = true;
  });

  return built;
}

bool execute_planet_build_command(const command_t& argv, GameObj& g) {
  starnum_t snum = g.snum();
  planetnum_t pnum = g.pnum();
  auto plan = validate_planet_build(argv, g, snum, pnum);
  if (!plan) {
    return false;
  }
  bool any_built = false;
  for (int _ : std::views::iota(0, plan->count)) {
    if (!execute_single_planet_build(g, *plan, snum, pnum)) {
      break;
    }
    any_built = true;
  }
  return any_built;
}

struct ShipBuildPlan {
  ShipType what;
  Coordinates land_coords;
  int count;
  bool outside;
};

std::optional<ShipBuildPlan> validate_factory_ship_build(const command_t& argv,
                                                         GameObj& g,
                                                         const Ship& builder,
                                                         starnum_t snum,
                                                         planetnum_t pnum) {
  const auto& race = *g.race;
  int count = parse_ship_build_count(argv, true);
  if (!count) {
    g.out << "Give a positive number of builds.\n";
    return std::nullopt;
  }
  if (!builder.has_factory_design()) {
    g.out << "This factory has not been designated to build a ship type.\n";
    return std::nullopt;
  }
  double tech = complexity(builder);
  if (tech > race.tech && !race.God) {
    g.out << std::format(
        "You are not advanced enough to build this ship.\n"
        "{:.1f} engineering technology needed. You have {:.1f}.\n",
        tech, race.tech);
    return std::nullopt;
  }
  Coordinates bcoords = builder.land_coords();
  ShipType what = builder.build_type();
  const auto& planet = *g.entity_manager.peek_planet(snum, pnum);
  const auto& star = *g.entity_manager.peek_star(snum);
  if (const auto res =
          can_build_at_planet(g.player(), g.governor(), star, planet);
      !res) {
    g.out << GB::presentation::format_planet_build_error(res.error());
    g.out << "You can't build that here.\n";
    return std::nullopt;
  }
  const auto& sectormap = *g.entity_manager.peek_sectormap(snum, pnum);
  const auto& sector = sectormap.get(bcoords);
  auto result = can_build_on_sector(g.entity_manager, what, race, planet,
                                    sector, bcoords);
  if (!result) {
    g.out << result.error();
    return std::nullopt;
  }
  return ShipBuildPlan{
      .what = what, .land_coords = bcoords, .count = count, .outside = true};
}

std::optional<ShipBuildPlan>
validate_non_factory_ship_build(const command_t& argv, GameObj& g,
                                const Ship& builder) {
  const auto& race = *g.race;
  bool outside = (builder.type() == ShipType::STYPE_SHUTTLE ||
                  builder.type() == ShipType::STYPE_CARGO);
  if (outside && builder.is_landed()) {
    g.out << "This ships cannot build when landed.\n";
    return std::nullopt;
  }
  if (argv.size() < 2) {
    g.out << "Build what?\n";
    return std::nullopt;
  }
  auto what = get_build_type(argv[1][0]);
  if (!what) {
    g.out << "No such ship type.\n";
    return std::nullopt;
  }
  auto build_on_ship_result = can_build_on_ship(*what, race, builder);
  if (!build_on_ship_result) {
    g.out << build_on_ship_result.error();
    return std::nullopt;
  }
  int count = parse_ship_build_count(argv, false);
  if (!count) {
    g.out << "Give a positive number of builds.\n";
    return std::nullopt;
  }
  double tech = ship_template(*what).base_tech;
  if (tech > race.tech && !race.God) {
    g.out << std::format(
        "You are not advanced enough to build this ship.\n"
        "{:.1f} engineering technology needed. You have {:.1f}.\n",
        tech, race.tech);
    return std::nullopt;
  }
  return ShipBuildPlan{.what = *what,
                       .land_coords = builder.land_coords(),
                       .count = count,
                       .outside = outside};
}

bool execute_single_factory_build(GameObj& g, Ship& builder,
                                  const ShipBuildPlan& plan, starnum_t snum,
                                  planetnum_t pnum) {
  const player_t Playernum = g.player();
  const governor_t Governor = g.governor();
  const auto& race = *g.race;
  auto newship = getfactship(builder);
  std::pair<population_t, fuel_t> loaded{0, 0.0};
  bool success = false;

  g.entity_manager.mutate_planet(snum, pnum, [&](Planet& planet) {
    const resource_t shipcost = newship->build_cost();
    if (shipcost > planet.info(Playernum).resource) {
      g.out << std::format("You need {}r to construct this ship.\n", shipcost);
      return;
    }
    if (!g.deduct_ap(snum, 1)) {
      g.out << "You don't have 1 action points there.\n";
      return;
    }
    g.present(create_ship_by_planet(g.entity_manager, Playernum, Governor, race,
                                    *newship, planet, snum, pnum,
                                    plan.land_coords));
    if (g.current_governor().toggle.autoload &&
        plan.what != ShipType::OTYPE_TRANSDEV && !race.God) {
      g.entity_manager.mutate_sectormap(snum, pnum, [&](SectorMap& sectormap) {
        auto& sector = sectormap.get(plan.land_coords);
        loaded = autoload_at_planet(Playernum, *newship, planet, sector);
      });
    }
    success = true;
  });

  if (!success) {
    return false;
  }

  const auto [load_crew, load_fuel] = loaded;
  g.present(
      initialize_new_ship(race, Governor, *newship, load_fuel, load_crew));
  g.entity_manager.create_ship(std::move(newship));
  g.entity_manager.mutate_ship(builder.number(),
                               [&](Ship& b) { b = Ship(builder.to_struct()); });
  return true;
}

bool execute_single_non_factory_ship_build(GameObj& g, Ship& builder,
                                           const ShipBuildPlan& plan,
                                           starnum_t snum) {
  const player_t Playernum = g.player();
  const governor_t Governor = g.governor();
  const auto& race = *g.race;
  auto newship = getship(plan.what, race);
  const resource_t shipcost = newship->build_cost();

  if (!plan.outside && !builder.can_fit_in_hangar(newship->size())) {
    g.out << "Not enough hanger space.\n";
    return false;
  }
  if (builder.resource() < shipcost) {
    g.out << std::format("You need {}r to construct the ship.\n", shipcost);
    return false;
  }
  if (!plan.outside && builder.whatorbits() == ScopeLevel::LEVEL_UNIV) {
    if (!g.deduct_univ_ap(1)) {
      g.out << "You need 1 universe action point.\n";
      return false;
    }
  } else {
    if (!g.deduct_ap(snum, 1)) {
      g.out << "You don't have 1 action points there.\n";
      return false;
    }
  }

  g.present(create_ship_by_ship(g.entity_manager, Playernum, Governor, race,
                                plan.outside, *newship, builder));
  const auto [load_crew, load_fuel] =
      (g.current_governor().toggle.autoload &&
       plan.what != ShipType::OTYPE_TRANSDEV && !race.God)
          ? autoload_at_ship(*newship, builder, race.mass)
          : std::pair<population_t, fuel_t>{0, 0.0};

  g.present(
      initialize_new_ship(race, Governor, *newship, load_fuel, load_crew));
  if (!plan.outside) {
    builder.load_docked_craft(*newship);
  }
  g.entity_manager.create_ship(std::move(newship));
  g.entity_manager.mutate_ship(builder.number(),
                               [&](Ship& b) { b = Ship(builder.to_struct()); });
  return true;
}

bool execute_ship_build_command(const command_t& argv, GameObj& g) {
  const auto& builder_ref = *g.entity_manager.peek_ship(g.shipno());
  Ship builder(builder_ref.to_struct());
  const auto build_loc =
      build_at_ship(g.player(), g.governor(), g.god(), builder);
  if (!build_loc) {
    g.out << GB::presentation::format_ship_build_error(build_loc.error());
    g.out << "You can't build here.\n";
    return false;
  }
  const auto [build_level, snum, pnum] = *build_loc;

  std::optional<ShipBuildPlan> plan;
  if (builder.type() == ShipType::OTYPE_FACTORY) {
    plan = validate_factory_ship_build(argv, g, builder, snum, pnum);
  } else {
    plan = validate_non_factory_ship_build(argv, g, builder);
  }
  if (!plan) {
    return false;
  }

  bool any_built = false;
  for (int _ : std::views::iota(0, plan->count)) {
    bool built =
        (builder.type() == ShipType::OTYPE_FACTORY)
            ? execute_single_factory_build(g, builder, *plan, snum, pnum)
            : execute_single_non_factory_ship_build(g, builder, *plan, snum);
    if (!built) {
      break;
    }
    any_built = true;
  }
  return any_built;
}

}  // namespace

namespace GB::commands {

bool build(const command_t& argv, GameObj& g) {
  if (argv.size() > 1 && argv[1][0] == '?') {
    return handle_build_info_query(argv, g);
  }

  ScopeLevel level = g.level();
  if (level == ScopeLevel::LEVEL_PLAN) {
    return execute_planet_build_command(argv, g);
  }
  if (level == ScopeLevel::LEVEL_SHIP) {
    return execute_ship_build_command(argv, g);
  }

  g.out << "You must change scope to a ship or planet to build.\n";
  return false;
}

const CommandDescriptor build_cmd{
    .name = "build",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::dynamic(),
    .min_args = 1,
    .syntax = "build <type> <x,y> [count] | build ? [type]",
    .description = "Construct a ship on a planet or from a ship",
    .handler = &build,
};

}  // namespace GB::commands
