// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  orbit.c -- display orbits of planets (graphic representation) */

module;

import gblib;
import std.compat;

module commands;

static double Lastx, Lasty, Zoom;
static const int SCALE = 100;

static std::string DispStar(const GameObj&, const ScopeLevel, const Star&, int,
                            const Race&);
static std::string DispPlanet(const GameObj&, const ScopeLevel, const Planet&,
                              std::string_view, int, const Race&);
static void DispShip(const GameObj&, EntityManager&, const Place&, const Ship*,
                     const Race&, char*, const Planet& = Planet());

namespace GB::commands {
/* OPTIONS
 *  -p : If this option is set, ``orbit'' will not display planet names.
 *
 *  -S : Do not display star names.
 *
 *  -s : Do not display ships.
 *
 *  -(number) : Do not display that #'d ship or planet (in case it obstructs
 * 		the view of another object)
 */
void orbit(const command_t& argv, GameObj& g) {
  int DontDispNum = -1;
  int DontDispPlanets;
  int DontDispShips;
  int DontDispStars;
  char output[100000];

  DontDispPlanets = DontDispShips = DontDispStars = 0;

  /* find options, set flags accordingly */
  for (int flag = 1; flag <= argv.size() - 1; flag++)
    if (*argv[flag].c_str() == '-') {
      for (int i = 1; argv[flag][i] != '\0'; i++)
        switch (argv[flag][i]) {
          case 's':
            DontDispShips = 1;
            break;
          case 'S':
            DontDispStars = 1;
            break;
          case 'p':
            DontDispPlanets = 1;
            break;
          default:
            if (sscanf(argv[flag].c_str() + 1, "%d", &DontDispNum) != 1) {
              g.out << std::format("Bad number {}.\n",
                                   std::string_view(argv[flag]).substr(1));
              DontDispNum = -1;
            }
            if (DontDispNum) DontDispNum--; /* make a '1' into a '0' */
            break;
        }
    }

  std::unique_ptr<Place> where;
  if (argv.size() == 1) {
    where = std::make_unique<Place>(g, ":");
    int i = (g.level() == ScopeLevel::LEVEL_UNIV);
    Lastx = g.lastx[i];
    Lasty = g.lasty[i];
    Zoom = g.zoom[i];
  } else {
    where = std::make_unique<Place>(g, argv[argv.size() - 1]);
    Lastx = Lasty = 0.0;
    Zoom = 1.1;
  }

  if (where->err) {
    g.out << "orbit: error in args.\n";
    return;
  }

  /* orbit type of map */
  sprintf(output, "#");

  const auto* race_ptr = g.entity_manager.peek_race(g.player());
  if (!race_ptr) {
    g.out << "Race not found.\n";
    return;
  }
  const Race& Race = *race_ptr;

  switch (where->level) {
    case ScopeLevel::LEVEL_UNIV: {
      const auto* universe = g.entity_manager.peek_universe();
      if (!universe) {
        g.out << "Universe data not available.\n";
        return;
      }
      for (auto star_handle : StarList(g.entity_manager)) {
        const auto& star_ref = *star_handle;
        if (DontDispNum != star_ref.star_id()) {
          std::string star = DispStar(g, ScopeLevel::LEVEL_UNIV, star_ref,
                                      DontDispStars, Race);
          strcat(output, star.c_str());
        }
      }
      if (!DontDispShips) {
        ShipList ships(g.entity_manager, universe->ships);
        char shipbuf[256];
        for (auto ship_handle : ships) {
          const Ship& s = ship_handle.peek();  // Read-only access
          if (DontDispNum != s.number()) {
            shipbuf[0] = '\0';
            DispShip(g, g.entity_manager, *where, &s, Race, shipbuf);
            strcat(output, shipbuf);
          }
        }
      }
      break;
    }
    case ScopeLevel::LEVEL_STAR: {
      const auto* star_ptr = g.entity_manager.peek_star(where->snum);
      if (!star_ptr) {
        g.out << "Star not found.\n";
        return;
      }
      std::string star =
          DispStar(g, ScopeLevel::LEVEL_STAR, *star_ptr, DontDispStars, Race);
      strcat(output, star.c_str());

      for (planetnum_t i = 0; i < star_ptr->numplanets(); i++)
        if (DontDispNum != i) {
          const auto* p = g.entity_manager.peek_planet(where->snum, i);
          if (!p) continue;
          std::string planet =
              DispPlanet(g, ScopeLevel::LEVEL_STAR, *p,
                         star_ptr->get_planet_name(i), DontDispPlanets, Race);
          strcat(output, planet.c_str());
        }
      /* check to see if you have ships at orbiting the star, if so you can
         see enemy ships */
      bool iq = false;
      if (g.god())
        iq = true;
      else {
        ShipList ships(g.entity_manager, star_ptr->ships());
        for (auto ship_handle : ships) {
          const Ship& s = ship_handle.peek();  // Read-only access
          if (s.owner() == g.player() && shipsight(s)) {
            iq = true; /* you are there to sight, need a crew */
            break;
          }
        }
      }
      if (!DontDispShips) {
        ShipList ships(g.entity_manager, star_ptr->ships());
        char shipbuf[256];
        for (auto ship_handle : ships) {
          const Ship& s = ship_handle.peek();  // Read-only access
          if (DontDispNum != s.number() &&
              !(s.owner() != g.player() && s.type() == ShipType::STYPE_MINE)) {
            if ((s.owner() == g.player()) || iq) {
              shipbuf[0] = '\0';
              DispShip(g, g.entity_manager, *where, &s, Race, shipbuf);
              strcat(output, shipbuf);
            }
          }
        }
      }
    } break;
    case ScopeLevel::LEVEL_PLAN: {
      const auto* plan_star = g.entity_manager.peek_star(where->snum);
      if (!plan_star) {
        g.out << "Star not found.\n";
        return;
      }
      const auto* p = g.entity_manager.peek_planet(where->snum, where->pnum);
      if (!p) {
        g.out << "Planet not found.\n";
        return;
      }
      std::string planet = DispPlanet(g, ScopeLevel::LEVEL_PLAN, *p,
                                      plan_star->get_planet_name(where->pnum),
                                      DontDispPlanets, Race);
      strcat(output, planet.c_str());

      /* check to see if you have ships at landed or
         orbiting the planet, if so you can see orbiting enemy ships */
      bool iq = false;
      ShipList ships(g.entity_manager, p->ships());
      for (auto ship_handle : ships) {
        const Ship& s = ship_handle.peek();  // Read-only access
        if (s.owner() == g.player() && shipsight(s)) {
          iq = true; /* you are there to sight, need a crew */
          break;
        }
      }
      /* end check */
      if (!DontDispShips) {
        char shipbuf[256];
        for (auto ship_handle : ships) {
          const Ship& s = ship_handle.peek();  // Read-only access
          if (DontDispNum != s.number()) {
            if (!landed(s)) {
              if ((s.owner() == g.player()) || iq) {
                shipbuf[0] = '\0';
                DispShip(g, g.entity_manager, *where, &s, Race, shipbuf, *p);
                strcat(output, shipbuf);
              }
            }
          }
        }
      }
    } break;
    default:
      g.out << "Bad scope.\n";
      return;
  }
  strcat(output, "\n");
  g.out << output;
}
}  // namespace GB::commands

// TODO(jeffbailey) Remove DontDispStar parameter as unused, but it really looks
// like we should be doing something here.
static std::string DispStar(const GameObj& g, const ScopeLevel level,
                            const Star& star, int /* DontDispStars */,
                            const Race& r) {
  int x;
  int y;

  switch (level) {
    case (ScopeLevel::LEVEL_UNIV):
      x = (int)(SCALE + ((SCALE * (star.xpos() - Lastx)) / (UNIVSIZE * Zoom)));
      y = (int)(SCALE + ((SCALE * (star.ypos() - Lasty)) / (UNIVSIZE * Zoom)));
      break;
    case (ScopeLevel::LEVEL_STAR):
      x = (int)(SCALE + (SCALE * (-Lastx)) / (SYSTEMSIZE * Zoom));
      y = (int)(SCALE + (SCALE * (-Lasty)) / (SYSTEMSIZE * Zoom));
      break;
    default:
      return "";
  }

  std::stringstream ss;
  if (r.governor[g.governor().value].toggle.color) {
    char stand = (isset(star.explored(), g.player()) ? g.player() : 0) + '?';
    ss << std::format("{} {} {} 0 * ", stand, x, y);
    stand = (isset(star.inhabited(), g.player()) ? g.player() : 0) + '?';
    ss << std::format("{} {};", stand, star.get_name());
  } else {
    int stand = (isset(star.explored(), g.player()) ? 1 : 0);
    ss << std::format("{} {} {} 0 * ", stand, x, y);
    stand = (isset(star.inhabited(), g.player()) ? 1 : 0);
    ss << std::format("{} {};", stand, star.get_name());
  }

  return ss.str();
}

// TODO(jeffbailey): We remove DontDispPlanets as unused, but it really seems
// like we should be doing something here!
static std::string DispPlanet(const GameObj& g, const ScopeLevel level,
                              const Planet& p, std::string_view name,
                              int /* DontDispPlanets */, const Race& r) {
  int x = 0;  // TODO(jeffbailey): Check if init to 0 is right.
  int y = 0;

  switch (level) {
    case ScopeLevel::LEVEL_STAR:
      y = (int)(SCALE + (SCALE * (p.ypos() - Lasty)) / (SYSTEMSIZE * Zoom));
      x = (int)(SCALE + (SCALE * (p.xpos() - Lastx)) / (SYSTEMSIZE * Zoom));
      break;
    case ScopeLevel::LEVEL_PLAN:
      y = (int)(SCALE + (SCALE * (-Lasty)) / (PLORBITSIZE * Zoom));
      x = (int)(SCALE + (SCALE * (-Lastx)) / (PLORBITSIZE * Zoom));
      break;
    default:
      return "";
  }
  std::stringstream ss;

  if (r.governor[g.governor().value].toggle.color) {
    char stand = (p.info(g.player() - 1).explored ? g.player() : 0) + '?';
    ss << std::format("{} {} {} 0 {} ", stand, x, y,
                      (stand > '0' ? Psymbol[p.type()] : '?'));
    stand = (p.info(g.player() - 1).numsectsowned ? g.player() : 0) + '?';
    ss << std::format("{} {}", stand, name);
  } else {
    int stand = p.info(g.player() - 1).explored ? 1 : 0;
    ss << std::format("{} {} {} 0 {} ", stand, x, y,
                      (stand ? Psymbol[p.type()] : '?'));
    stand = p.info(g.player() - 1).numsectsowned ? 1 : 0;
    ss << std::format("{} {}", stand, name);
  }
  if (r.governor[g.governor().value].toggle.compat &&
      p.info(g.player() - 1).explored) {
    ss << std::format("({})", (int)p.compatibility(r));
  }
  ss << ";";

  return ss.str();
}

static void DispShip(const GameObj& g, EntityManager& em, const Place& where,
                     const Ship* ship, const Race& r, char* string,
                     const Planet& pl) {
  int x;
  int y;
  int wm;
  int stand;
  double xt;
  double yt;
  double slope;

  if (!ship->alive()) return;

  *string = '\0';

  // Get star position for coordinate calculations
  const auto* where_star = (where.level != ScopeLevel::LEVEL_UNIV)
                               ? em.peek_star(where.snum)
                               : nullptr;

  switch (where.level) {
    case ScopeLevel::LEVEL_PLAN:
      if (!where_star) return;
      x = (int)(SCALE + (SCALE * (ship->xpos() -
                                  (where_star->xpos() + pl.xpos()) - Lastx)) /
                            (PLORBITSIZE * Zoom));
      y = (int)(SCALE + (SCALE * (ship->ypos() -
                                  (where_star->ypos() + pl.ypos()) - Lasty)) /
                            (PLORBITSIZE * Zoom));
      break;
    case ScopeLevel::LEVEL_STAR:
      if (!where_star) return;
      x = (int)(SCALE + (SCALE * (ship->xpos() - where_star->xpos() - Lastx)) /
                            (SYSTEMSIZE * Zoom));
      y = (int)(SCALE + (SCALE * (ship->ypos() - where_star->ypos() - Lasty)) /
                            (SYSTEMSIZE * Zoom));
      break;
    case ScopeLevel::LEVEL_UNIV:
      x = (int)(SCALE + (SCALE * (ship->xpos() - Lastx)) / (UNIVSIZE * Zoom));
      y = (int)(SCALE + (SCALE * (ship->ypos() - Lasty)) / (UNIVSIZE * Zoom));
      break;
    case ScopeLevel::LEVEL_SHIP:
      // Ships can't orbit other ships; this case should never be reached.
      return;
  }

  switch (ship->type()) {
    case ShipType::STYPE_MIRROR: {
      if (std::holds_alternative<AimedAtData>(ship->special())) {
        auto aimed_at = std::get<AimedAtData>(ship->special());
        if (aimed_at.level == ScopeLevel::LEVEL_STAR) {
          const auto* aimed_star = em.peek_star(aimed_at.snum);
          if (aimed_star) {
            xt = aimed_star->xpos();
            yt = aimed_star->ypos();
          } else {
            xt = yt = 0.0;
          }
        } else if (aimed_at.level == ScopeLevel::LEVEL_PLAN) {
          const auto* aimed_star = em.peek_star(aimed_at.snum);
          if (!aimed_star) {
            xt = yt = 0.0;
          } else if (where.level == ScopeLevel::LEVEL_PLAN &&
                     aimed_at.pnum == where.pnum) {
            /* same planet */
            xt = aimed_star->xpos() + pl.xpos();
            yt = aimed_star->ypos() + pl.ypos();
          } else { /* different planet */
            const auto* apl = em.peek_planet(where.snum, where.pnum);
            if (apl) {
              xt = aimed_star->xpos() + apl->xpos();
              yt = aimed_star->ypos() + apl->ypos();
            } else {
              xt = yt = 0.0;
            }
          }
        } else if (aimed_at.level == ScopeLevel::LEVEL_SHIP) {
          const auto* aship = em.peek_ship(aimed_at.shipno);
          if (aship) {
            xt = aship->xpos();
            yt = aship->ypos();
          } else {
            xt = yt = 0.0;
          }
        } else {
          xt = yt = 0.0;
        }
      } else {
        xt = yt = 0.0;
      }
      wm = 0;

      if (xt == ship->xpos()) {
        if (yt > ship->ypos())
          wm = 4;
        else
          wm = 0;
      } else {
        slope = (yt - ship->ypos()) / (xt - ship->xpos());
        if (yt == ship->ypos()) {
          if (xt > ship->xpos())
            wm = 2;
          else
            wm = 6;
        } else if (yt > ship->ypos()) {
          if (slope < -2.414) wm = 4;
          if (slope > -2.414) wm = 5;
          if (slope > -0.414) wm = 6;
          if (slope > 0.000) wm = 2;
          if (slope > 0.414) wm = 3;
          if (slope > 2.414) wm = 4;
        } else if (yt < ship->ypos()) {
          if (slope < -2.414) wm = 0;
          if (slope > -2.414) wm = 1;
          if (slope > -0.414) wm = 2;
          if (slope > 0.000) wm = 6;
          if (slope > 0.414) wm = 7;
          if (slope > 2.414) wm = 0;
        }
      }

      /* (magnification) */
      if (x >= 0 && y >= 0) {
        if (r.governor[g.governor().value].toggle.color) {
          sprintf(string, "%c %d %d %d %c %c %lu;", (char)(ship->owner() + '?'),
                  x, y, wm, Shipltrs[ship->type()], (char)(ship->owner() + '?'),
                  ship->number());
        } else {
          stand = (ship->owner() ==
                   r.governor[g.governor().value].toggle.highlight);
          sprintf(string, "%d %d %d %d %c %d %lu;", stand, x, y, wm,
                  Shipltrs[ship->type()], stand, ship->number());
        }
      }
      break;
    }

    case ShipType::OTYPE_CANIST:
    case ShipType::OTYPE_GREEN:
      break;

    default:
      /* other ships can only be seen when in system */
      wm = 0;
      if (ship->whatorbits() != ScopeLevel::LEVEL_UNIV ||
          ((ship->owner() == g.player()) || g.god()))
        if (x >= 0 && y >= 0) {
          if (r.governor[g.governor().value].toggle.color) {
            sprintf(string, "%c %d %d %d %c %c %lu;",
                    (char)(ship->owner() + '?'), x, y, wm,
                    Shipltrs[ship->type()], (char)(ship->owner() + '?'),
                    ship->number());
          } else {
            stand = (ship->owner() ==
                     r.governor[g.governor().value].toggle.highlight);
            sprintf(string, "%d %d %d %d %c %d %lu;", stand, x, y, wm,
                    Shipltrs[ship->type()], stand, ship->number());
          }
        }
      break;
  }
}
