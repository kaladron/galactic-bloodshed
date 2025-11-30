// SPDX-License-Identifier: Apache-2.0

export module gblib:place;

import :files_shl;
import :gameobj;
import :globals;
import :shlmisc;
import :star;

export class Place { /* used in function return for finding place */
public:
  Place(ScopeLevel level_, starnum_t snum_, planetnum_t pnum_,
        shipnum_t shipno_)
      : level(level_), snum(snum_), pnum(pnum_), shipno(shipno_) {}

  Place(ScopeLevel level_, starnum_t snum_, planetnum_t pnum_);

  Place(GameObj&, std::string_view, bool ignore_explore = false);
  ScopeLevel level;
  starnum_t snum;
  planetnum_t pnum;
  shipnum_t shipno;
  bool err = false;
  std::string to_string();

private:
  void getplace2(GameObj& g, std::string_view string, const bool ignoreexpl);
};

void Place::getplace2(GameObj& g, std::string_view string,
                      const bool ignoreexpl) {
  const player_t Playernum = g.player;

  if (err || string.empty()) return;

  if (string.front() == '.') {
    switch (level) {
      case ScopeLevel::LEVEL_UNIV:
        g.out << "Can't go higher.\n";
        err = true;
        return;
      case ScopeLevel::LEVEL_SHIP: {
        auto ship = getship(shipno);
        level = ship->whatorbits();
        // TODO(jeffbailey): Fix 'cs .' for ships within ships
        if (level == ScopeLevel::LEVEL_SHIP) shipno = ship->destshipno();
        break;
      }
      case ScopeLevel::LEVEL_STAR:
        level = ScopeLevel::LEVEL_UNIV;
        break;
      case ScopeLevel::LEVEL_PLAN:
        level = ScopeLevel::LEVEL_STAR;
        break;
    }
    while (string.starts_with('.'))
      string.remove_prefix(1);
    while (string.starts_with('/'))
      string.remove_prefix(1);
    getplace2(g, string, ignoreexpl);
    return;
  }

  // It's the name of something
  std::string_view substr = string.substr(0, string.find_first_of('/'));
  do {
    string.remove_prefix(1);
  } while (!string.starts_with('/') && !string.empty());

  switch (level) {
    case ScopeLevel::LEVEL_UNIV:
      for (auto i = 0; i < Sdata.numstars; i++)
        if (substr == stars[i].get_name()) {
          level = ScopeLevel::LEVEL_STAR;
          snum = i;
          if (ignoreexpl || isset(stars[snum].explored(), Playernum) || g.god) {
            if (string.starts_with('/')) string.remove_prefix(1);
            return getplace2(g, string, ignoreexpl);
          }
          g.out << std::format("You have not explored {0} yet.\n",
                               stars[snum].get_name());
          err = true;
          return;
        }
      g.out << std::format("No such star {0}.\n", substr.data());
      err = true;
      return;
    case ScopeLevel::LEVEL_STAR:
      for (auto i = 0; i < stars[snum].numplanets(); i++)
        if (substr == stars[snum].get_planet_name(i)) {
          level = ScopeLevel::LEVEL_PLAN;
          pnum = i;
          const auto p = getplanet(snum, i);
          if (ignoreexpl || p.info(Playernum - 1).explored || g.god) {
            if (string.starts_with('/')) string.remove_prefix(1);
            return getplace2(g, string, ignoreexpl);
          }
          g.out << std::format("You have not explored {0} yet.\n",
                               stars[snum].get_planet_name(i));
          err = true;
          return;
        }
      g.out << std::format("No such planet {0}.\n", substr.data());
      err = true;
      return;
    default:
      g.out << std::format("Can't descend to {0}.\n", substr.data());
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
      out << "/" << stars[snum].get_name();
      out << std::ends;
      return out.str();
    case ScopeLevel::LEVEL_PLAN:
      out << "/" << stars[snum].get_name() << "/"
          << stars[snum].get_planet_name(pnum);
      out << std::ends;
      return out.str();
    case ScopeLevel::LEVEL_SHIP:
      out << "#" << shipno;
      out << std::ends;
      return out.str();
    case ScopeLevel::LEVEL_UNIV:
      out << "/";
      out << std::ends;
      return out.str();
  }
}

Place::Place(GameObj& g, std::string_view string, const bool ignoreexpl)
    : level(g.level), snum(g.snum), pnum(g.pnum) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;

  if (level == ScopeLevel::LEVEL_SHIP) shipno = g.shipno;

  if (string.empty()) {
    return;
  }

  switch (string.front()) {
    case ':':
      return;
    case '/':
      level = ScopeLevel::LEVEL_UNIV; /* scope = root (universe) */
      snum = pnum = shipno = 0;
      string.remove_prefix(1);
      getplace2(g, string, ignoreexpl);
      return;
    case '#': {
      auto ship = getship(string);
      if (!ship) {
        DontOwnErr(Playernum, Governor, shipno);
        err = true;
        return;
      }
      if ((ship->owner() == Playernum || ignoreexpl || g.god) &&
          (ship->alive() || g.god)) {
        level = ScopeLevel::LEVEL_SHIP;
        shipno = ship->number();
        snum = ship->storbits();
        pnum = ship->pnumorbits();
        return;
      }
      err = true;
      return;
    }
    case '-':
      /* no destination */
      level = ScopeLevel::LEVEL_UNIV;
      snum = pnum = shipno = 0;
      return;
    default:
      getplace2(g, string, ignoreexpl);
      return;
  }
}
