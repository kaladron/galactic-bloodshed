// SPDX-License-Identifier: Apache-2.0

/// \file build.cc
/// \brief Ship construction on planets and by builder ships.

module;

import gb.entities;
import gb.services;
import scnlib;
import std;
import tabulate;

module commands;

namespace {
// Get ship types sorted by complexity (simplest first)
std::array<ShipType, NUMSTYPES> get_sorted_ship_types() {
  std::array<ShipType, NUMSTYPES> result;
  for (int i = 0; i < NUMSTYPES; i++) {
    result[i] = static_cast<ShipType>(i);
  }
  std::ranges::sort(result, [](ShipType a, ShipType b) {
    return complexity(a) < complexity(b);
  });
  return result;
}

// Create a tabulate table for ship specifications
tabulate::Table create_ship_spec_table() {
  tabulate::Table table;
  table.format().hide_border().column_separator("  ");

  table.column(0).format().width(1);   // Letter
  table.column(1).format().width(15);  // Name
  table.column(2).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(3).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(4).format().width(3).font_align(tabulate::FontAlign::right);
  table.column(5).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(6).format().width(3).font_align(tabulate::FontAlign::right);
  table.column(7).format().width(3).font_align(tabulate::FontAlign::right);
  table.column(8).format().width(3).font_align(tabulate::FontAlign::right);
  table.column(9).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(10).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(11).format().width(2).font_align(tabulate::FontAlign::right);
  table.column(12).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(13).format().width(4).font_align(tabulate::FontAlign::right);

  table.add_row({"?", "name", "cargo", "hang", "arm", "dest", "gun", "pri",
                 "sec", "fuel", "crew", "sp", "tech", "cost"});
  table[0].format().font_style({tabulate::FontStyle::bold});

  return table;
}

void add_ship_spec_row(tabulate::Table& table, ShipType i, const Race& race) {
  const auto& tmpl = ship_template(i);
  table.add_row(
      {std::string(1, tmpl.letter), std::string(tmpl.name),
       std::format("{}", tmpl.max_resource), std::format("{}", tmpl.max_hangar),
       std::format("{}", tmpl.base_armor), std::format("{}", tmpl.max_destruct),
       std::format("{}", tmpl.max_guns),
       std::format("{}", gun_caliber(tmpl.max_primary_caliber)),
       std::format("{}", gun_caliber(tmpl.max_secondary_caliber)),
       std::format("{}", tmpl.max_fuel), std::format("{}", tmpl.max_crew),
       std::format("{}", tmpl.base_speed),
       std::format("{:.0f}", tmpl.base_tech),
       std::format("{}", Shipcost(i, race))});
}

void print_all_ship_specs(GameObj& g, const Race& race) {
  g.out << "     - Default ship parameters -\n";
  auto table = create_ship_spec_table();
  auto sorted_ships = get_sorted_ship_types();
  for (ShipType i : sorted_ships) {
    const auto& tmpl = ship_template(i);
    if ((!tmpl.is_god_only) || race.God) {
      if (race.pods || (i != ShipType::STYPE_POD)) {
        if (tmpl.is_programmed) {
          add_ship_spec_row(table, i, race);
        }
      }
    }
  }
  g.out << table << "\n";
}

void format_ship_builders(GameObj& g, const ShipTemplate& tmpl) {
  if (tmpl.can_build_on_planet()) {
    g.out << "\nCan be constructed on planet.";
  }
  int n = 0;
  for (int j = 0; j < NUMSTYPES; j++) {
    if (tmpl.can_be_built_by(ship_template(ShipType{j}))) n++;
  }
  if (n > 0) {
    int m = 0;
    g.out << "\nCan be built by ";
    for (int j = 0; j < NUMSTYPES; j++) {
      if (tmpl.can_be_built_by(ship_template(ShipType{j}))) {
        m++;
        if (n - m > 1)
          g.out << std::format("{}, ", ship_template(ShipType{j}).letter);
        else if (n - m > 0)
          g.out << std::format("{} and ", ship_template(ShipType{j}).letter);
        else
          g.out << std::format("{} ", ship_template(ShipType{j}).letter);
      }
    }
    g.out << "type ships.\n";
  }
}

bool print_specific_ship_spec(GameObj& g, char type_char, const Race& race) {
  auto ship_type = get_build_type(type_char);
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
  if (exam && !exam->description.empty()) {
    g.out << "\n" << exam->description;
    if (!exam->description.ends_with('\n')) {
      g.out << "\n";
    }
  }
  format_ship_builders(g, tmpl);
  auto table = create_ship_spec_table();
  add_ship_spec_row(table, *ship_type, race);
  g.out << table << "\n";
  return true;
}

bool handle_build_info_query(const command_t& argv, GameObj& g) {
  const auto& race = *g.race;
  if (argv.size() == 2) {
    print_all_ship_specs(g, race);
    return true;
  }
  return print_specific_ship_spec(g, argv[2][0], race);
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
  if (!can_build_at_planet(g, star, planet) && !race.God) {
    g.out << "You can't build that here.\n";
    return std::nullopt;
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
    create_ship_by_planet(g.entity_manager, Playernum, Governor, race, *newship,
                          planet, snum, pnum, plan.coords);
    int load_crew = 0;
    double load_fuel = 0.0;
    if (race.governor[Governor.value].toggle.autoload &&
        plan.what != ShipType::OTYPE_TRANSDEV && !race.God) {
      g.entity_manager.mutate_sectormap(snum, pnum, [&](SectorMap& sectormap) {
        auto& sector = sectormap.get(plan.coords);
        autoload_at_planet(Playernum, newship.get(), &planet, sector,
                           &load_crew, &load_fuel);
      });
    }
    initialize_new_ship(g, race, newship.get(), load_fuel, load_crew);
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

std::optional<ShipBuildPlan>
validate_factory_ship_build(const command_t& argv, GameObj& g,
                            const Ship& builder, starnum_t snum,
                            planetnum_t pnum, ScopeLevel build_level) {
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
  if (!builder.is_landed()) {
    g.out << "Factories can only build when landed on a planet.\n";
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
  if (build_level == ScopeLevel::LEVEL_PLAN) {
    const auto& planet = *g.entity_manager.peek_planet(snum, pnum);
    const auto& star = *g.entity_manager.peek_star(snum);
    if (!can_build_at_planet(g, star, planet)) {
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
  int load_crew = 0;
  double load_fuel = 0.0;
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
    create_ship_by_planet(g.entity_manager, Playernum, Governor, race, *newship,
                          planet, snum, pnum, plan.land_coords);
    if (race.governor[Governor.value].toggle.autoload &&
        plan.what != ShipType::OTYPE_TRANSDEV && !race.God) {
      g.entity_manager.mutate_sectormap(snum, pnum, [&](SectorMap& sectormap) {
        auto& sector = sectormap.get(plan.land_coords);
        autoload_at_planet(Playernum, newship.get(), &planet, sector,
                           &load_crew, &load_fuel);
      });
    }
    success = true;
  });

  if (!success) {
    return false;
  }

  initialize_new_ship(g, race, newship.get(), load_fuel, load_crew);
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

  create_ship_by_ship(g.entity_manager, Playernum, Governor, race, plan.outside,
                      newship.get(), &builder);
  int load_crew = 0;
  double load_fuel = 0.0;
  if (race.governor[Governor.value].toggle.autoload &&
      plan.what != ShipType::OTYPE_TRANSDEV && !race.God) {
    autoload_at_ship(newship.get(), &builder, &load_crew, &load_fuel);
  }

  initialize_new_ship(g, race, newship.get(), load_fuel, load_crew);
  g.entity_manager.create_ship(std::move(newship));
  g.entity_manager.mutate_ship(builder.number(),
                               [&](Ship& b) { b = Ship(builder.to_struct()); });
  return true;
}

bool execute_ship_build_command(const command_t& argv, GameObj& g) {
  const auto& builder_ref = *g.entity_manager.peek_ship(g.shipno());
  Ship builder(builder_ref.to_struct());
  starnum_t snum = g.snum();
  planetnum_t pnum = g.pnum();
  auto test_build_level = build_at_ship(g, &builder, &snum, &pnum);
  if (!test_build_level) {
    g.out << "You can't build here.\n";
    return false;
  }
  ScopeLevel build_level = test_build_level.value();

  std::optional<ShipBuildPlan> plan;
  if (builder.type() == ShipType::OTYPE_FACTORY) {
    plan =
        validate_factory_ship_build(argv, g, builder, snum, pnum, build_level);
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
