// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include "gb/files.h"

module commands;

namespace {
void finish_build_plan(const Sector& sector, int x, int y, const Planet& planet,
                       starnum_t snum, planetnum_t pnum) {
  putsector(sector, planet, x, y);
  putplanet(planet, stars[snum], pnum);
}

void finish_build_ship(const Sector& sector, int x, int y, const Planet& planet,
                       starnum_t snum, planetnum_t pnum, bool outside,
                       ScopeLevel build_level,
                       const std::optional<Ship>& builder) {
  if (outside) switch (build_level) {
      case ScopeLevel::LEVEL_PLAN:
        putplanet(planet, stars[snum], pnum);
        if (landed(*builder)) {
          putsector(sector, planet, x, y);
        }
        break;
      case ScopeLevel::LEVEL_STAR:
        putstar(stars[snum], snum);
        break;
      case ScopeLevel::LEVEL_UNIV:
        putsdata(&Sdata);
        break;
      default:
        break;
    }
  putship(*builder);
}
}  // namespace

namespace GB::commands {
void build(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  // TODO(jeffbailey): Fix unused ap_t APcount = 1;
  Planet planet;
  char c;
  int j;
  int m;
  int n;
  int x;
  int y;
  int count;
  bool outside;
  ScopeLevel level;
  ScopeLevel build_level;
  int shipcost;
  int load_crew;
  double load_fuel;
  double tech;

  FILE* fd;
  Sector sector;
  std::optional<Ship> builder;
  Ship newship;

  if (argv.size() > 1 && argv[1][0] == '?') {
    /* information request */
    if (argv.size() == 2) {
      /* Ship parameter list */
      g.out << "     - Default ship parameters -\n";
      g.out << std::format(
          "{} {:<15} {:>5} {:>5} {:>3} {:>4} {:>3} {:>3} {:>3} {:>4} {:>4} "
          "{:>2} {:>4} {:>4}\n",
          "?", "name", "cargo", "hang", "arm", "dest", "gun", "pri", "sec",
          "fuel", "crew", "sp", "tech", "cost");
      const auto& race = *g.race;
      for (j = 0; j < NUMSTYPES; j++) {
        ShipType i{ShipVector[j]};
        if ((!Shipdata[i][ABIL_GOD]) || race.God) {
          if (race.pods || (i != ShipType::STYPE_POD)) {
            if (Shipdata[i][ABIL_PROGRAMMED]) {
              g.out << std::format(
                  "{} {:<15.15} {:>5} {:>5} {:>3} {:>4} {:>3} {:>3} {:>3} "
                  "{:>4} {:>4} {:>2} {:.0f} {:>4}\n",
                  Shipltrs[i], Shipnames[i], Shipdata[i][ABIL_CARGO],
                  Shipdata[i][ABIL_HANGER], Shipdata[i][ABIL_ARMOR],
                  Shipdata[i][ABIL_DESTCAP], Shipdata[i][ABIL_GUNS],
                  Shipdata[i][ABIL_PRIMARY], Shipdata[i][ABIL_SECONDARY],
                  Shipdata[i][ABIL_FUELCAP], Shipdata[i][ABIL_MAXCREW],
                  Shipdata[i][ABIL_SPEED], (double)Shipdata[i][ABIL_TECH],
                  Shipcost(i, race));
            }
          }
        }
      }
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
      g.out << std::format(
          "{} {:<15} {:>5} {:>5} {:>3} {:>4} {:>3} {:>3} {:>3} {:>4} {:>4} "
          "{:>2} {:>4} {:>4}\n",
          "?", "name", "cargo", "hang", "arm", "dest", "gun", "pri", "sec",
          "fuel", "crew", "sp", "tech", "cost");
      const auto& race = *g.race;
      g.out << std::format(
          "{} {:<15.15} {:>5} {:>5} {:>3} {:>4} {:>3} {:>3} {:>3} {:>4} {:>4} "
          "{:>2} {:.0f} {:>4}\n",
          Shipltrs[*i], Shipnames[*i], Shipdata[*i][ABIL_CARGO],
          Shipdata[*i][ABIL_HANGER], Shipdata[*i][ABIL_ARMOR],
          Shipdata[*i][ABIL_DESTCAP], Shipdata[*i][ABIL_GUNS],
          Shipdata[*i][ABIL_PRIMARY], Shipdata[*i][ABIL_SECONDARY],
          Shipdata[*i][ABIL_FUELCAP], Shipdata[*i][ABIL_MAXCREW],
          Shipdata[*i][ABIL_SPEED], (double)Shipdata[*i][ABIL_TECH],
          Shipcost(*i, race));
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
  do {
    switch (level) {
      case ScopeLevel::LEVEL_PLAN:
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
          planet = getplanet(snum, pnum);
          if (!can_build_at_planet(g, stars[snum], planet) && !race.God) {
            g.out << "You can't build that here.\n";
            return;
          }
          sscanf(argv[2].c_str(), "%d,%d", &x, &y);
          if (x < 0 || x >= planet.Maxx() || y < 0 || y >= planet.Maxy()) {
            g.out << "Illegal sector.\n";
            return;
          }
          sector = getsector(planet, x, y);
          auto result = can_build_on_sector(g.entity_manager, *what, race,
                                            planet, sector, {x, y});
          if (!result && !race.God) {
            g.out << result.error();
            return;
          }
          if (!(count = getcount(argv, 4))) {
            g.out << "Give a positive number of builds.\n";
            return;
          }
          Getship(&newship, *what, race);
        }
        if ((shipcost = newship.build_cost()) >
            planet.info(Playernum - 1).resource) {
          g.out << std::format("You need {}r to construct this ship.\n",
                               shipcost);
          finish_build_plan(sector, x, y, planet, snum, pnum);
          return;
        }
        create_ship_by_planet(Playernum, Governor, race, newship, planet, snum,
                              pnum, x, y);
        if (race.governor[Governor].toggle.autoload &&
            what != ShipType::OTYPE_TRANSDEV && !race.God)
          autoload_at_planet(Playernum, &newship, &planet, sector, &load_crew,
                             &load_fuel);
        else {
          load_crew = 0;
          load_fuel = 0.0;
        }
        initialize_new_ship(g, race, &newship, load_fuel, load_crew);
        putship(newship);
        break;
      case ScopeLevel::LEVEL_SHIP:
        if (!count) { /* initialize loop variables */
          builder = getship(g.shipno);
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
            planet = getplanet(snum, pnum);
            if (builder->type() == ShipType::OTYPE_FACTORY) {
              if (!can_build_at_planet(g, stars[snum], planet)) {
                g.out << "You can't build that here.\n";
                return;
              }
              x = builder->land_x();
              y = builder->land_y();
              what = builder->build_type();
              sector = getsector(planet, x, y);
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
          case ShipType::OTYPE_FACTORY:
            if ((shipcost = newship.build_cost()) >
                planet.info(Playernum - 1).resource) {
              g.out << std::format("You need {}r to construct this ship.\n",
                                   shipcost);
              finish_build_ship(sector, x, y, planet, snum, pnum, outside,
                                build_level, builder);
              return;
            }
            create_ship_by_planet(Playernum, Governor, race, newship, planet,
                                  snum, pnum, x, y);
            if (race.governor[Governor].toggle.autoload &&
                what != ShipType::OTYPE_TRANSDEV && !race.God) {
              autoload_at_planet(Playernum, &newship, &planet, sector,
                                 &load_crew, &load_fuel);
            } else {
              load_crew = 0;
              load_fuel = 0.0;
            }
            break;
          case ShipType::STYPE_SHUTTLE:
          case ShipType::STYPE_CARGO:
            if (builder->resource() < (shipcost = newship.build_cost())) {
              g.out << std::format("You need {}r to construct the ship.\n",
                                   shipcost);
              finish_build_ship(sector, x, y, planet, snum, pnum, outside,
                                build_level, builder);
              return;
            }
            create_ship_by_ship(Playernum, Governor, race, true, &planet,
                                &newship, &*builder);
            if (race.governor[Governor].toggle.autoload &&
                what != ShipType::OTYPE_TRANSDEV && !race.God)
              autoload_at_ship(&newship, &*builder, &load_crew, &load_fuel);
            else {
              load_crew = 0;
              load_fuel = 0.0;
            }
            break;
          default:
            if (builder->hanger() + ship_size(newship) >
                builder->max_hanger()) {
              g.out << "Not enough hanger space.\n";
              finish_build_ship(sector, x, y, planet, snum, pnum, outside,
                                build_level, builder);
              return;
            }
            if (builder->resource() < (shipcost = newship.build_cost())) {
              g.out << std::format("You need {}r to construct the ship.\n",
                                   shipcost);
              finish_build_ship(sector, x, y, planet, snum, pnum, outside,
                                build_level, builder);
              return;
            }
            create_ship_by_ship(Playernum, Governor, race, false, nullptr,
                                &newship, &*builder);
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
        putship(newship);
        break;
      default:
        // Shouldn't be possible.
        break;
    }
    count--;
  } while (count);
  /* free stuff */
  switch (level) {
    case ScopeLevel::LEVEL_PLAN:
      finish_build_plan(sector, x, y, planet, snum, pnum);
      break;
    case ScopeLevel::LEVEL_SHIP:
      finish_build_ship(sector, x, y, planet, snum, pnum, outside, build_level,
                        builder);
      break;
    default:
      break;
  }
}
}  // namespace GB::commands
