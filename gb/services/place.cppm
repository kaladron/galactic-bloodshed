// SPDX-License-Identifier: Apache-2.0

/// \file place.cppm
/// \brief Module interface partition for Place coordinate resolution and entity
/// referencing.

export module gb.services:place;

import gb.entities;
import :entitylists;
import :gameobj;
import :services;
import :tele;

export class Place { /* used in function return for finding place */
public:
  Place(ScopeLevel level_, starnum_t snum_, planetnum_t pnum_,
        shipnum_t shipno_)
      : level(level_), snum(snum_), pnum(pnum_), shipno(shipno_),
        entity_manager(nullptr) {}

  Place(ScopeLevel level_, starnum_t snum_, planetnum_t pnum_);

  Place(GameObj&, std::string_view, bool ignore_explore = false);
  ScopeLevel level{ScopeLevel::LEVEL_UNIV};
  starnum_t snum{0};
  planetnum_t pnum{0};
  shipnum_t shipno{0};
  bool err = false;
  std::string to_string();

private:
  EntityManager* entity_manager =
      nullptr;  // For accessing star/planet names in to_string()
  void resolve_ship_place(GameObj& g, std::string_view string,
                          bool ignore_explore);
  void ascend_parent_scope(GameObj& g, std::string_view string,
                           bool ignore_explore);
  void descend_from_universe(GameObj& g, std::string_view star_name,
                             std::string_view remaining, bool ignore_explore);
  void descend_from_star(GameObj& g, std::string_view planet_name,
                         std::string_view remaining, bool ignore_explore);
  void getplace2(GameObj& g, std::string_view string, bool ignoreexpl);
};

void Place::ascend_parent_scope(GameObj& g, std::string_view string,
                                const bool ignore_explore) {
  switch (level) {
    case ScopeLevel::LEVEL_UNIV:
      g.out << "Can't go higher.\n";
      err = true;
      return;
    case ScopeLevel::LEVEL_SHIP: {
      const Ship* ship = nullptr;
      try {
        ship = g.entity_manager.peek_ship(shipno);
      } catch (const EntityNotFoundError&) {
        g.out << "Ship not found.\n";
        err = true;
        return;
      }
      level = ship->whatorbits();
      if (level == ScopeLevel::LEVEL_SHIP) {
        shipno = ship->destshipno().value_or(0);
        const auto* parent = g.entity_manager.peek_ship(shipno);
        snum = parent->storbits();
        pnum = parent->pnumorbits();
      } else {
        snum = ship->storbits();
        pnum = ship->pnumorbits();
        shipno = 0;
      }
      break;
    }
    case ScopeLevel::LEVEL_STAR:
      level = ScopeLevel::LEVEL_UNIV;
      break;
    case ScopeLevel::LEVEL_PLAN:
      level = ScopeLevel::LEVEL_STAR;
      break;
  }
  while (string.starts_with('.')) {
    string.remove_prefix(1);
  }
  while (string.starts_with('/')) {
    string.remove_prefix(1);
  }
  getplace2(g, string, ignore_explore);
}

void Place::descend_from_universe(GameObj& g, std::string_view star_name,
                                  std::string_view remaining,
                                  const bool ignore_explore) {
  for (const Star& star : StarList::readonly(g.entity_manager)) {
    if (star_name != star.get_name()) continue;
    level = ScopeLevel::LEVEL_STAR;
    snum = star.star_id();
    if (ignore_explore || star.is_explored_by(g.player()) || g.god()) {
      if (remaining.starts_with('/')) remaining.remove_prefix(1);
      getplace2(g, remaining, ignore_explore);
      return;
    }
    g.out << std::format("You have not explored {} yet.\n", star.get_name());
    err = true;
    return;
  }
  g.out << std::format("No such star {}.\n", star_name);
  err = true;
}

void Place::descend_from_star(GameObj& g, std::string_view planet_name,
                              std::string_view remaining,
                              const bool ignore_explore) {
  const auto& star = *g.entity_manager.peek_star(snum);
  for (const Planet& planet :
       PlanetList::readonly(g.entity_manager, snum, star)) {
    const planetnum_t i = planet.planet_order();
    if (planet_name != star.get_planet_name(i)) continue;
    level = ScopeLevel::LEVEL_PLAN;
    pnum = i;
    if (ignore_explore || planet.info(g.player()).explored || g.god()) {
      if (remaining.starts_with('/')) remaining.remove_prefix(1);
      getplace2(g, remaining, ignore_explore);
      return;
    }
    g.out << std::format("You have not explored {} yet.\n",
                         star.get_planet_name(i));
    err = true;
    return;
  }
  g.out << std::format("No such planet {}.\n", planet_name);
  err = true;
}

void Place::getplace2(GameObj& g, std::string_view string,
                      const bool ignoreexpl) {
  if (err || string.empty()) return;

  if (string.front() == '.') {
    ascend_parent_scope(g, string, ignoreexpl);
    return;
  }

  // Extract path component up to the next '/'
  const auto slash_pos = string.find_first_of('/');
  const std::string_view substr = string.substr(0, slash_pos);
  std::string_view remaining =
      (slash_pos == std::string_view::npos) ? "" : string.substr(slash_pos);

  switch (level) {
    case ScopeLevel::LEVEL_UNIV:
      descend_from_universe(g, substr, remaining, ignoreexpl);
      return;
    case ScopeLevel::LEVEL_STAR:
      descend_from_star(g, substr, remaining, ignoreexpl);
      return;
    default:
      g.out << std::format("Can't descend to {}.\n", substr);
      err = true;
      return;
  }
}

Place::Place(ScopeLevel level_, starnum_t snum_, planetnum_t pnum_)
    : level(level_), snum(snum_), pnum(pnum_), shipno(0) {
  if (level_ == ScopeLevel::LEVEL_SHIP) err = true;
}

std::string Place::to_string() {
  std::ostringstream out;
  switch (level) {
    case ScopeLevel::LEVEL_STAR:
      if (entity_manager) {
        const auto& star = *entity_manager->peek_star(snum);
        out << "/" << star.get_name();
      }
      return out.str();
    case ScopeLevel::LEVEL_PLAN:
      if (entity_manager) {
        const auto& star = *entity_manager->peek_star(snum);
        out << "/" << star.get_name() << "/" << star.get_planet_name(pnum);
      }
      return out.str();
    case ScopeLevel::LEVEL_SHIP:
      out << "#" << shipno;
      return out.str();
    case ScopeLevel::LEVEL_UNIV:
      out << "/";
      return out.str();
  }
}

void Place::resolve_ship_place(GameObj& g, std::string_view string,
                               const bool ignore_explore) {
  const auto shipnum = string_to_shipnum(string);
  if (!shipnum) {
    notify_dont_own_ship(g, shipno);
    err = true;
    return;
  }
  const Ship* ship = nullptr;
  try {
    ship = g.entity_manager.peek_ship(*shipnum);
  } catch (const EntityNotFoundError&) {
    notify_dont_own_ship(g, *shipnum);
    err = true;
    return;
  }
  if ((ship->owner() == g.player() || ignore_explore || g.god()) &&
      (ship->alive() || g.god())) {
    level = ScopeLevel::LEVEL_SHIP;
    shipno = ship->number();
    snum = ship->storbits();
    pnum = ship->pnumorbits();
    return;
  }
  notify_dont_own_ship(g, *shipnum);
  err = true;
}

Place::Place(GameObj& g, std::string_view string, const bool ignoreexpl)
    : level(g.level()), snum(g.snum()), pnum(g.pnum()),
      entity_manager(&g.entity_manager) {
  if (level == ScopeLevel::LEVEL_SHIP) shipno = g.shipno();

  if (string.empty()) {
    return;
  }

  switch (string.front()) {
    case ':':
      return;
    case '/':
      level = ScopeLevel::LEVEL_UNIV; /* scope = root (universe) */
      snum = 0;
      pnum = 0;
      shipno = 0;
      string.remove_prefix(1);
      getplace2(g, string, ignoreexpl);
      return;
    case '#':
      resolve_ship_place(g, string, ignoreexpl);
      return;
    case '-':
      /* no destination */
      level = ScopeLevel::LEVEL_UNIV;
      snum = 0;
      pnum = 0;
      shipno = 0;
      return;
    default:
      getplace2(g, string, ignoreexpl);
      return;
  }
}
