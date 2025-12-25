// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import scnlib;
import std;
import tabulate;

#include "gb/files.h"
#include <cstdio>

module commands;

namespace {
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
  table.add_row(
      {std::string(1, Shipltrs[i]), std::string(Shipnames[i]),
       std::format("{}", Shipdata[i][ABIL_CARGO]),
       std::format("{}", Shipdata[i][ABIL_HANGER]),
       std::format("{}", Shipdata[i][ABIL_ARMOR]),
       std::format("{}", Shipdata[i][ABIL_DESTCAP]),
       std::format("{}", Shipdata[i][ABIL_GUNS]),
       std::format("{}", Shipdata[i][ABIL_PRIMARY]),
       std::format("{}", Shipdata[i][ABIL_SECONDARY]),
       std::format("{}", Shipdata[i][ABIL_FUELCAP]),
       std::format("{}", Shipdata[i][ABIL_MAXCREW]),
       std::format("{}", Shipdata[i][ABIL_SPEED]),
       std::format("{:.0f}", (double)Shipdata[i][ABIL_TECH]),
       std::format("{}", Shipcost(i, race))});
}
}  // namespace

namespace GB::commands {
void build(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  // TODO(jeffbailey): Fix unused ap_t APcount = 1;
  char c;
  int j;
  int m;
  int n;
  int count;
  bool outside = false;
  ScopeLevel level;
  ScopeLevel build_level = ScopeLevel::LEVEL_UNIV;
  int shipcost;
  int load_crew;
  double load_fuel;
  double tech;

  FILE* fd;
  std::optional<Ship> builder;
  Ship newship;

  // Entity handles that persist across the loop
  std::optional<EntityHandle<Planet>> planet_handle;
  std::optional<EntityHandle<SectorMap>> sectormap_handle;

  if (argv.size() > 1 && argv[1][0] == '?') {
    /* information request */
    if (argv.size() == 2) {
      /* Ship parameter list */
      g.out << "     - Default ship parameters -\n";
      auto table = create_ship_spec_table();
      const auto& race = *g.race;
      for (j = 0; j < NUMSTYPES; j++) {
        ShipType i{ShipVector[j]};
        if ((!Shipdata[i][ABIL_GOD]) || race.God) {
          if (race.pods || (i != ShipType::STYPE_POD)) {
            if (Shipdata[i][ABIL_PROGRAMMED]) {
              add_ship_spec_row(table, i, race);
            }
          }
        }
      }
      g.out << table << "\n";
      return;
    }
    /* Description of specific ship type */
    auto i = get_build_type(argv[2][0]);
    if (!i)
      g.out << "No such ship type.\n";
    else if (!Shipdata[*i][ABIL_PROGRAMMED])
      g.out << "This ship type has not been programmed.\n";
    else {
      if ((fd = fopen(EXAM_FL, "r")) == nullptr) {
        perror(EXAM_FL);
        return;
      }
      /* look through ship description file */
      g.out << "\n";
      for (j = 0; j <= i; j++)
        while (fgetc(fd) != '~')
          ;
      /* Give description */
      std::stringstream ss;
      while ((c = fgetc(fd)) != '~') {
        ss << c;
      }
      g.out << ss.str();
      fclose(fd);
      /* Built where? */
      if (Shipdata[*i][ABIL_BUILD] & 1) {
        g.out << "\nCan be constructed on planet.";
      }
      n = 0;
      std::string header = "\nCan be built by ";
      for (j = 0; j < NUMSTYPES; j++)
        if (Shipdata[*i][ABIL_BUILD] & Shipdata[j][ABIL_CONSTRUCT]) n++;
      if (n) {
        m = 0;
        g.out << header;
        for (j = 0; j < NUMSTYPES; j++) {
          if (Shipdata[*i][ABIL_BUILD] & Shipdata[j][ABIL_CONSTRUCT]) {
            m++;
            if (n - m > 1)
              g.out << std::format("{}, ", Shipltrs[j]);
            else if (n - m > 0)
              g.out << std::format("{} and ", Shipltrs[j]);
            else
              g.out << std::format("{} ", Shipltrs[j]);
          }
        }
        g.out << "type ships.\n";
      }
      /* default parameters */
      auto table = create_ship_spec_table();
      const auto& race = *g.race;
      add_ship_spec_row(table, *i, race);
      g.out << table << "\n";
    }

    return;
  }

  level = g.level;
  if (level != ScopeLevel::LEVEL_SHIP && level != ScopeLevel::LEVEL_PLAN) {
    g.out << "You must change scope to a ship or planet to build.\n";
    return;
  }
  starnum_t snum = g.snum;
  planetnum_t pnum = g.pnum;
  const auto& race = *g.race;
  count = 0; /* this used used to reset count in the loop */
  std::optional<ShipType> what;
  int x, y;  // Coordinates for sector access
  do {
    switch (level) {
      case ScopeLevel::LEVEL_PLAN: {
        if (!count) { /* initialize loop variables */
          if (argv.size() < 2) {
            g.out << "Build what?\n";
            return;
          }
          what = get_build_type(argv[1][0]);
          if (!what) {
            g.out << "No such ship type.\n";
            return;
          }
          auto buildresult = can_build_this(*what, race);
          if (!buildresult && !race.God) {
            g.out << buildresult.error();
            return;
          }
          if (!(Shipdata[*what][ABIL_BUILD] & 1) && !race.God) {
            g.out << "This ship cannot be built by a planet.\n";
            return;
          }
          if (argv.size() < 3) {
            g.out << "Build where?\n";
            return;
          }
          planet_handle = g.entity_manager.get_planet(snum, pnum);
          Planet& planet = **planet_handle;
          const auto& star = *g.entity_manager.peek_star(snum);
          if (!can_build_at_planet(g, star, planet) && !race.God) {
            g.out << "You can't build that here.\n";
            return;
          }
          auto xy_result = scn::scan<int, int>(argv[2], "{},{}");
          if (!xy_result) {
            g.out << "Invalid sector format. Use: x,y\n";
            return;
          }
          std::tie(x, y) = xy_result->values();
          if (x < 0 || x >= planet.Maxx() || y < 0 || y >= planet.Maxy()) {
            g.out << "Illegal sector.\n";
            return;
          }
          sectormap_handle = g.entity_manager.get_sectormap(snum, pnum);
          auto& sectormap = **sectormap_handle;
          auto& sector = sectormap.get(x, y);
          auto result = can_build_on_sector(g.entity_manager, *what, race,
                                            planet, sector, {x, y});
          if (!result && !race.God) {
            g.out << result.error();
            return;
          }
          if (!(count = getcount(argv, 3))) {
            g.out << "Give a positive number of builds.\n";
            return;
          }
          Getship(&newship, *what, race);
        }
        Planet& planet = **planet_handle;
        auto& sectormap = **sectormap_handle;
        auto& sector = sectormap.get(x, y);
        if ((shipcost = newship.build_cost()) >
            planet.info(Playernum - 1).resource) {
          g.out << std::format("You need {}r to construct this ship.\n",
                               shipcost);
          return;
        }
        create_ship_by_planet(g.entity_manager, Playernum, Governor, race,
                              newship, planet, snum, pnum, x, y);
        if (race.governor[Governor].toggle.autoload &&
            what != ShipType::OTYPE_TRANSDEV && !race.God)
          autoload_at_planet(Playernum, &newship, &planet, sector, &load_crew,
                             &load_fuel);
        else {
          load_crew = 0;
          load_fuel = 0.0;
        }
        initialize_new_ship(g, race, &newship, load_fuel, load_crew);
        {
          auto ship_handle = g.entity_manager.create_ship(newship.to_struct());
          // Ship is now created in database with its data
        }
        break;
      }
      case ScopeLevel::LEVEL_SHIP: {
        if (!count) { /* initialize loop variables */
          const auto* builder_ptr = g.entity_manager.peek_ship(g.shipno);
          if (!builder_ptr) {
            g.out << "Ship not found.\n";
            return;
          }
          builder =
              Ship(builder_ptr->to_struct());  // Copy ship data to local Ship
          outside = false;
          auto test_build_level = build_at_ship(g, &*builder, &snum, &pnum);
          if (!test_build_level) {
            g.out << "You can't build here.\n";
            return;
          }
          build_level = test_build_level.value();
          switch (builder->type()) {
            case ShipType::OTYPE_FACTORY:
              if (!(count = getcount(argv, 2))) {
                g.out << "Give a positive number of builds.\n";
                return;
              }
              if (!landed(*builder)) {
                g.out << "Factories can only build when landed on a planet.\n";
                return;
              }
              newship = Getfactship(*builder);
              outside = true;
              break;
            case ShipType::STYPE_SHUTTLE:
            case ShipType::STYPE_CARGO:
              if (landed(*builder)) {
                g.out << "This ships cannot build when landed.\n";
                return;
              }
              outside = true;
              [[clang::fallthrough]];  // TODO(jeffbailey): Added this to
                                       // silence
                                       // warning, check it.
            default:
              if (argv.size() < 2) {
                g.out << "Build what?\n";
                return;
              }
              if ((what = get_build_type(argv[1][0])) < 0) {
                g.out << "No such ship type.\n";
                return;
              }
              auto build_on_ship_result =
                  can_build_on_ship(*what, race, *builder);
              if (!build_on_ship_result) {
                g.out << build_on_ship_result.error();
                return;
              }
              if (!(count = getcount(argv, 3))) {
                g.out << "Give a positive number of builds.\n";
                return;
              }
              Getship(&newship, *what, race);
              break;
          }
          if ((tech = builder->type() == ShipType::OTYPE_FACTORY
                          ? complexity(*builder)
                          : Shipdata[*what][ABIL_TECH]) > race.tech &&
              !race.God) {
            g.out << std::format(
                "You are not advanced enough to build this ship.\n"
                "{:.1f} engineering technology needed. You have {:.1f}.\n",
                tech, race.tech);
            return;
          }
          if (outside && build_level == ScopeLevel::LEVEL_PLAN) {
            planet_handle = g.entity_manager.get_planet(snum, pnum);
            Planet& planet = **planet_handle;
            if (builder->type() == ShipType::OTYPE_FACTORY) {
              const auto& star = *g.entity_manager.peek_star(snum);
              if (!can_build_at_planet(g, star, planet)) {
                g.out << "You can't build that here.\n";
                return;
              }
              x = builder->land_x();
              y = builder->land_y();
              what = builder->build_type();
              sectormap_handle = g.entity_manager.get_sectormap(snum, pnum);
              auto& sectormap = **sectormap_handle;
              auto& sector = sectormap.get(x, y);
              auto result = can_build_on_sector(g.entity_manager, *what, race,
                                                planet, sector, {x, y});
              if (!result) {
                g.out << result.error();
                return;
              }
            }
          }
        }
        /* build 'em */
        switch (builder->type()) {
          case ShipType::OTYPE_FACTORY: {
            Planet& planet = **planet_handle;
            auto& sectormap = **sectormap_handle;
            auto& sector = sectormap.get(x, y);
            if ((shipcost = newship.build_cost()) >
                planet.info(Playernum - 1).resource) {
              g.out << std::format("You need {}r to construct this ship.\n",
                                   shipcost);
              return;
            }
            create_ship_by_planet(g.entity_manager, Playernum, Governor, race,
                                  newship, planet, snum, pnum, x, y);
            if (race.governor[Governor].toggle.autoload &&
                what != ShipType::OTYPE_TRANSDEV && !race.God) {
              autoload_at_planet(Playernum, &newship, &planet, sector,
                                 &load_crew, &load_fuel);
            } else {
              load_crew = 0;
              load_fuel = 0.0;
            }
            break;
          }
          case ShipType::STYPE_SHUTTLE:
          case ShipType::STYPE_CARGO: {
            Planet& planet = **planet_handle;
            if (builder->resource() < (shipcost = newship.build_cost())) {
              g.out << std::format("You need {}r to construct the ship.\n",
                                   shipcost);
              return;
            }
            create_ship_by_ship(g.entity_manager, Playernum, Governor, race,
                                true, &planet, &newship, &*builder);
            if (race.governor[Governor].toggle.autoload &&
                what != ShipType::OTYPE_TRANSDEV && !race.God)
              autoload_at_ship(&newship, &*builder, &load_crew, &load_fuel);
            else {
              load_crew = 0;
              load_fuel = 0.0;
            }
            break;
          }
          default:
            if (builder->hanger() + ship_size(newship) >
                builder->max_hanger()) {
              g.out << "Not enough hanger space.\n";
              return;
            }
            if (builder->resource() < (shipcost = newship.build_cost())) {
              g.out << std::format("You need {}r to construct the ship.\n",
                                   shipcost);
              return;
            }
            create_ship_by_ship(g.entity_manager, Playernum, Governor, race,
                                false, nullptr, &newship, &*builder);
            if (race.governor[Governor].toggle.autoload &&
                what != ShipType::OTYPE_TRANSDEV && !race.God)
              autoload_at_ship(&newship, &*builder, &load_crew, &load_fuel);
            else {
              load_crew = 0;
              load_fuel = 0.0;
            }
            break;
        }
        initialize_new_ship(g, race, &newship, load_fuel, load_crew);
        {
          auto ship_handle = g.entity_manager.create_ship(newship.to_struct());
          // Ship is now created in database with its data
        }
        {
          auto builder_handle = g.entity_manager.get_ship(builder->number());
          *builder_handle = std::move(*builder);
        }
        break;
      }
      default:
        // Shouldn't be possible.
        break;
    }
    count--;
  } while (count);
}
}  // namespace GB::commands
