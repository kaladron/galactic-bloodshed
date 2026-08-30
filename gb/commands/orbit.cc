// SPDX-License-Identifier: Apache-2.0

/// \file orbit.cc
/// \brief Display orbits of planets (graphic representation).

module;

import std;
import gb.entities;
import gb.services;
import scnlib;

module commands;

static double Lastx, Lasty, Zoom;
static const int SCALE = 100;

static std::string DispStar(const GameObj&, const ScopeLevel, const Star&, int,
                            const Race&);
static std::string DispPlanet(const GameObj&, const ScopeLevel, const Planet&,
                              std::string_view, int, const Race&);
static std::string DispShip(const GameObj&, EntityManager&, const Place&,
                            const Ship&, const Race&);
static std::string DispShip(const GameObj&, EntityManager&, const Place&,
                            const Ship&, const Race&, const Planet&);

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
bool orbit(const command_t& argv, GameObj& g) {
  int DontDispNum = -1;
  int DontDispPlanets;
  int DontDispShips;
  int DontDispStars;
  std::string system_map_text;

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
          default: {
            auto scan_res =
                scn::scan<int>(std::string_view(argv[flag]).substr(1), "{}");
            if (scan_res) {
              DontDispNum = scan_res->value();
            } else {
              g.out << std::format("Bad number {}.\n",
                                   std::string_view(argv[flag]).substr(1));
              return false;
            }
            if (DontDispNum > 0) DontDispNum--; /* make a '1' into a '0' */
            break;
          }
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
    return false;
  }

  /* orbit type of map */
  system_map_text = "#";

  const auto* race_ptr = g.entity_manager.peek_race(g.player());
  if (!race_ptr) {
    g.out << "Race not found.\n";
    return false;
  }
  const Race& Race = *race_ptr;

  switch (where->level) {
    case ScopeLevel::LEVEL_UNIV: {
      const auto* universe = g.entity_manager.peek_universe();
      if (!universe) {
        g.out << "Universe data not available.\n";
        return false;
      }
      for (const Star& star_ref : StarList::readonly(g.entity_manager)) {
        if (DontDispNum != star_ref.star_id()) {
          std::string star = DispStar(g, ScopeLevel::LEVEL_UNIV, star_ref,
                                      DontDispStars, Race);
          system_map_text += star;
        }
      }
      if (!DontDispShips) {
        for (const Ship& s :
             ShipList::readonly(g.entity_manager, ScopeLevel::LEVEL_UNIV)) {
          if (DontDispNum != s.number()) {
            system_map_text += DispShip(g, g.entity_manager, *where, s, Race);
          }
        }
      }
      break;
    }
    case ScopeLevel::LEVEL_STAR: {
      const auto* star_ptr = g.entity_manager.peek_star(where->snum);
      if (!star_ptr) {
        g.out << "Star not found.\n";
        return false;
      }
      std::string star =
          DispStar(g, ScopeLevel::LEVEL_STAR, *star_ptr, DontDispStars, Race);
      system_map_text += star;

      for (planetnum_t i = 0; i < star_ptr->numplanets(); i++)
        if (DontDispNum != i) {
          const auto* p = g.entity_manager.peek_planet(where->snum, i);
          if (!p) continue;
          std::string planet =
              DispPlanet(g, ScopeLevel::LEVEL_STAR, *p,
                         star_ptr->get_planet_name(i), DontDispPlanets, Race);
          system_map_text += planet;
        }
      /* check to see if you have ships at orbiting the star, if so you can
         see enemy ships */
      bool iq = false;
      if (g.god())
        iq = true;
      else {
        for (const Ship& s :
             ShipList::readonly(g.entity_manager, where->snum)) {
          if (s.owner() == g.player() && s.has_sight()) {
            iq = true; /* you are there to sight, need a crew */
            break;
          }
        }
      }
      if (!DontDispShips) {
        for (const Ship& s :
             ShipList::readonly(g.entity_manager, where->snum)) {
          if (DontDispNum != s.number() &&
              !(s.owner() != g.player() && s.type() == ShipType::STYPE_MINE)) {
            if ((s.owner() == g.player()) || iq) {
              system_map_text += DispShip(g, g.entity_manager, *where, s, Race);
            }
          }
        }
      }
    } break;
    case ScopeLevel::LEVEL_PLAN: {
      const auto* plan_star = g.entity_manager.peek_star(where->snum);
      if (!plan_star) {
        g.out << "Star not found.\n";
        return false;
      }
      const auto* p = g.entity_manager.peek_planet(where->snum, where->pnum);
      if (!p) {
        g.out << "Planet not found.\n";
        return false;
      }
      std::string planet = DispPlanet(g, ScopeLevel::LEVEL_PLAN, *p,
                                      plan_star->get_planet_name(where->pnum),
                                      DontDispPlanets, Race);
      system_map_text += planet;

      /* check to see if you have ships at landed or
         orbiting the planet, if so you can see orbiting enemy ships */
      bool iq = false;
      for (const Ship& s :
           ShipList::readonly(g.entity_manager, where->snum, where->pnum)) {
        if (s.owner() == g.player() && s.has_sight()) {
          iq = true; /* you are there to sight, need a crew */
          break;
        }
      }
      /* end check */
      if (!DontDispShips) {
        for (const Ship& s :
             ShipList::readonly(g.entity_manager, where->snum, where->pnum)) {
          if (DontDispNum != s.number()) {
            if (!s.is_landed()) {
              if ((s.owner() == g.player()) || iq) {
                system_map_text +=
                    DispShip(g, g.entity_manager, *where, s, Race, *p);
              }
            }
          }
        }
      }
    } break;
    default:
      g.out << "Bad scope.\n";
      return false;
  }
  system_map_text += '\n';
  g.out << system_map_text;
  return true;
}

const CommandDescriptor orbit_cmd{
    .name = "orbit",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "orbit [-p] [-S] [-s] [-<num>] [<path>]",
    .description = "Graphic representation of objects in current scope or path",
    .handler = &orbit,
};

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
    char stand = (star.is_explored_by(g.player()) ? g.player().value : 0) + '?';
    ss << std::format("{} {} {} 0 * ", stand, x, y);
    stand = (star.is_inhabited_by(g.player()) ? g.player().value : 0) + '?';
    ss << std::format("{} {};", stand, star.get_name());
  } else {
    int stand = (star.is_explored_by(g.player()) ? 1 : 0);
    ss << std::format("{} {} {} 0 * ", stand, x, y);
    stand = (star.is_inhabited_by(g.player()) ? 1 : 0);
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
    char stand = (p.info(g.player()).explored ? g.player().value : 0) + '?';
    ss << std::format("{} {} {} 0 {} ", stand, x, y,
                      (stand > '0' ? Psymbol[p.type()] : '?'));
    stand = (p.info(g.player()).numsectsowned ? g.player().value : 0) + '?';
    ss << std::format("{} {}", stand, name);
  } else {
    int stand = p.info(g.player()).explored ? 1 : 0;
    ss << std::format("{} {} {} 0 {} ", stand, x, y,
                      (stand ? Psymbol[p.type()] : '?'));
    stand = p.info(g.player()).numsectsowned ? 1 : 0;
    ss << std::format("{} {}", stand, name);
  }
  if (r.governor[g.governor().value].toggle.compat &&
      p.info(g.player()).explored) {
    ss << std::format("({})", (int)p.compatibility(r));
  }
  ss << ";";

  return ss.str();
}

static std::string DispShip(const GameObj& g, EntityManager& em,
                            const Place& where, const Ship& ship, const Race& r,
                            const Planet& pl) {
  if (!ship.alive()) return "";

  // Get star position for coordinate calculations
  const auto* where_star = (where.level != ScopeLevel::LEVEL_UNIV)
                               ? em.peek_star(where.snum)
                               : nullptr;

  int x = 0;
  int y = 0;

  switch (where.level) {
    case ScopeLevel::LEVEL_PLAN:
      if (!where_star) return "";
      x = (int)(SCALE + (SCALE * (ship.xpos() -
                                  (where_star->xpos() + pl.xpos()) - Lastx)) /
                            (PLORBITSIZE * Zoom));
      y = (int)(SCALE + (SCALE * (ship.ypos() -
                                  (where_star->ypos() + pl.ypos()) - Lasty)) /
                            (PLORBITSIZE * Zoom));
      break;
    case ScopeLevel::LEVEL_STAR:
      if (!where_star) return "";
      x = (int)(SCALE + (SCALE * (ship.xpos() - where_star->xpos() - Lastx)) /
                            (SYSTEMSIZE * Zoom));
      y = (int)(SCALE + (SCALE * (ship.ypos() - where_star->ypos() - Lasty)) /
                            (SYSTEMSIZE * Zoom));
      break;
    case ScopeLevel::LEVEL_UNIV:
      x = (int)(SCALE + (SCALE * (ship.xpos() - Lastx)) / (UNIVSIZE * Zoom));
      y = (int)(SCALE + (SCALE * (ship.ypos() - Lasty)) / (UNIVSIZE * Zoom));
      break;
    case ScopeLevel::LEVEL_SHIP:
      // Ships can't orbit other ships; this case should never be reached.
      return "";
  }

  // The 4th field in graphical orbit display represents mirror compass heading
  // (0..7). For standard vessels, mirror_heading is 0.
  int mirror_heading = 0;
  switch (ship.type()) {
    case ShipType::STYPE_MIRROR: {
      const auto* mirror = ship.as<SpaceMirrorShip>();
      mirror_heading = mirror ? mirror->aim_direction(em) : 0;
      break;
    }

    case ShipType::OTYPE_CANIST:
    case ShipType::OTYPE_GREEN:
      return "";

    default:
      /* other ships can only be seen when in system */
      if (ship.whatorbits() == ScopeLevel::LEVEL_UNIV &&
          ship.owner() != g.player() && !g.god()) {
        return "";
      }
      break;
  }

  if (x >= 0 && y >= 0) {
    if (r.governor[g.governor().value].toggle.color) {
      return std::format("{} {} {} {} {} {} {};",
                         (char)(ship.owner().value + '?'), x, y, mirror_heading,
                         Shipltrs[ship.type()],
                         (char)(ship.owner().value + '?'), ship.number().value);
    }
    const bool stand =
        (ship.owner() == r.governor[g.governor().value].toggle.highlight);
    return std::format("{} {} {} {} {} {} {};", stand, x, y, mirror_heading,
                       Shipltrs[ship.type()], stand, ship.number().value);
  }
  return "";
}

static std::string DispShip(const GameObj& g, EntityManager& em,
                            const Place& where, const Ship& ship,
                            const Race& r) {
  static const Planet dummy_planet{};
  return DispShip(g, em, where, ship, r, dummy_planet);
}
