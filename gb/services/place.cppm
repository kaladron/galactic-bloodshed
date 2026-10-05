// SPDX-License-Identifier: Apache-2.0

/// \file place.cppm
/// \brief Module interface partition for Place coordinate resolution and entity
/// referencing.

export module gb.services:place;

import gb.entities;
import :entitylists;
import :gameobj;
import :services;

export struct ScopeContext {
  player_t player{0};
  governor_t governor{Race::leader_id};
  bool god{false};
  ScopeLevel level{ScopeLevel::LEVEL_UNIV};
  starnum_t snum{0};
  planetnum_t pnum{0};
  shipnum_t shipno{0};

  [[nodiscard]] static ScopeContext from(const GameObj& g) noexcept {
    return ScopeContext{
        .player = g.player(),
        .governor = g.governor(),
        .god = g.god(),
        .level = g.level(),
        .snum = g.snum(),
        .pnum = g.pnum(),
        .shipno =
            (g.level() == ScopeLevel::LEVEL_SHIP) ? g.shipno() : shipnum_t{0},
    };
  }
};

export enum class PlaceErrorKind {
  CantGoHigher,
  ShipNotFound,
  StarUnexplored,
  NoSuchStar,
  PlanetUnexplored,
  NoSuchPlanet,
  CantDescend,
  DontOwnShip,
};

export struct PlaceError {
  PlaceErrorKind kind;
  std::string target{};
};

export [[nodiscard]] std::string format_place_error(const PlaceError& error) {
  std::string result;
  switch (error.kind) {
    case PlaceErrorKind::CantGoHigher:
      result = "Can't go higher.\n";
      break;
    case PlaceErrorKind::ShipNotFound:
      result = "Ship not found.\n";
      break;
    case PlaceErrorKind::StarUnexplored:
    case PlaceErrorKind::PlanetUnexplored:
      result = std::format("You have not explored {} yet.\n", error.target);
      break;
    case PlaceErrorKind::NoSuchStar:
      result = std::format("No such star {}.\n", error.target);
      break;
    case PlaceErrorKind::NoSuchPlanet:
      result = std::format("No such planet {}.\n", error.target);
      break;
    case PlaceErrorKind::CantDescend:
      result = std::format("Can't descend to {}.\n", error.target);
      break;
    case PlaceErrorKind::DontOwnShip:
      result = std::format("You don't own ship #{}.\n", error.target);
      break;
  }
  return result;
}

export class Place { /* used in function return for finding place */
public:
  Place(ScopeLevel level_, starnum_t snum_, planetnum_t pnum_,
        shipnum_t shipno_)
      : level(level_), snum(snum_), pnum(pnum_), shipno(shipno_),
        entity_manager(nullptr) {}

  Place(ScopeLevel level_, starnum_t snum_, planetnum_t pnum_);

  Place(GameObj&, std::string_view, bool ignore_explore = false);

  [[nodiscard]] static std::expected<Place, PlaceError>
  resolve(EntityManager& em, const ScopeContext& ctx, std::string_view string,
          bool ignore_explore = false);

  ScopeLevel level{ScopeLevel::LEVEL_UNIV};
  starnum_t snum{0};
  planetnum_t pnum{0};
  shipnum_t shipno{0};
  bool err = false;
  std::string to_string();

private:
  EntityManager* entity_manager =
      nullptr;  // For accessing star/planet names in to_string()
  std::expected<void, PlaceError> resolve_ship_place(EntityManager& em,
                                                     const ScopeContext& ctx,
                                                     std::string_view string,
                                                     bool ignore_explore);
  std::expected<void, PlaceError> ascend_parent_scope(EntityManager& em,
                                                      const ScopeContext& ctx,
                                                      std::string_view string,
                                                      bool ignore_explore);
  std::expected<void, PlaceError>
  descend_from_universe(EntityManager& em, const ScopeContext& ctx,
                        std::string_view star_name, std::string_view remaining,
                        bool ignore_explore);
  std::expected<void, PlaceError>
  descend_from_star(EntityManager& em, const ScopeContext& ctx,
                    std::string_view planet_name, std::string_view remaining,
                    bool ignore_explore);
  std::expected<void, PlaceError> getplace2(EntityManager& em,
                                            const ScopeContext& ctx,
                                            std::string_view string,
                                            bool ignoreexpl);
};

std::expected<void, PlaceError>
Place::ascend_parent_scope(EntityManager& em, const ScopeContext& ctx,
                           std::string_view string, const bool ignore_explore) {
  switch (level) {
    case ScopeLevel::LEVEL_UNIV:
      return std::unexpected(PlaceError{PlaceErrorKind::CantGoHigher, {}});
    case ScopeLevel::LEVEL_SHIP: {
      try {
        const Ship* ship = em.peek_ship(shipno);
        level = ship->whatorbits();
        if (level == ScopeLevel::LEVEL_SHIP) {
          shipno = ship->destshipno().value_or(0);
          const auto* parent = em.peek_ship(shipno);
          snum = parent->storbits();
          pnum = parent->pnumorbits();
        } else {
          snum = ship->storbits();
          pnum = ship->pnumorbits();
          shipno = 0;
        }
      } catch (const EntityNotFoundError&) {
        return std::unexpected(PlaceError{PlaceErrorKind::ShipNotFound, {}});
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
  return getplace2(em, ctx, string, ignore_explore);
}

std::expected<void, PlaceError> Place::descend_from_universe(
    EntityManager& em, const ScopeContext& ctx, std::string_view star_name,
    std::string_view remaining, const bool ignore_explore) {
  for (const Star& star : StarList::readonly(em)) {
    if (star_name != star.get_name()) continue;
    level = ScopeLevel::LEVEL_STAR;
    snum = star.star_id();
    if (ignore_explore || star.is_explored_by(ctx.player) || ctx.god) {
      if (remaining.starts_with('/')) remaining.remove_prefix(1);
      return getplace2(em, ctx, remaining, ignore_explore);
    }
    return std::unexpected(
        PlaceError{PlaceErrorKind::StarUnexplored, star.get_name()});
  }
  return std::unexpected(
      PlaceError{PlaceErrorKind::NoSuchStar, std::string(star_name)});
}

std::expected<void, PlaceError> Place::descend_from_star(
    EntityManager& em, const ScopeContext& ctx, std::string_view planet_name,
    std::string_view remaining, const bool ignore_explore) {
  const auto& star = *em.peek_star(snum);
  for (const Planet& planet : PlanetList::readonly(em, snum, star)) {
    const planetnum_t i = planet.planet_order();
    if (planet_name != star.get_planet_name(i)) continue;
    level = ScopeLevel::LEVEL_PLAN;
    pnum = i;
    if (ignore_explore || planet.info(ctx.player).explored || ctx.god) {
      if (remaining.starts_with('/')) remaining.remove_prefix(1);
      return getplace2(em, ctx, remaining, ignore_explore);
    }
    return std::unexpected(
        PlaceError{PlaceErrorKind::PlanetUnexplored, star.get_planet_name(i)});
  }
  return std::unexpected(
      PlaceError{PlaceErrorKind::NoSuchPlanet, std::string(planet_name)});
}

std::expected<void, PlaceError> Place::getplace2(EntityManager& em,
                                                 const ScopeContext& ctx,
                                                 std::string_view string,
                                                 const bool ignoreexpl) {
  if (string.empty()) return {};

  if (string.front() == '.') {
    return ascend_parent_scope(em, ctx, string, ignoreexpl);
  }

  // Extract path component up to the next '/'
  const auto slash_pos = string.find_first_of('/');
  const std::string_view substr = string.substr(0, slash_pos);
  std::string_view remaining =
      (slash_pos == std::string_view::npos) ? "" : string.substr(slash_pos);

  switch (level) {
    case ScopeLevel::LEVEL_UNIV:
      return descend_from_universe(em, ctx, substr, remaining, ignoreexpl);
    case ScopeLevel::LEVEL_STAR:
      return descend_from_star(em, ctx, substr, remaining, ignoreexpl);
    default:
      return std::unexpected(
          PlaceError{PlaceErrorKind::CantDescend, std::string(substr)});
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

std::expected<void, PlaceError>
Place::resolve_ship_place(EntityManager& em, const ScopeContext& ctx,
                          std::string_view string, const bool ignore_explore) {
  const auto shipnum = string_to_shipnum(string);
  if (!shipnum) {
    const std::string_view raw_target =
        string.starts_with('#') ? string.substr(1) : string;
    return std::unexpected(
        PlaceError{PlaceErrorKind::DontOwnShip, std::string(raw_target)});
  }
  const Ship* ship = nullptr;
  try {
    ship = em.peek_ship(*shipnum);
  } catch (const EntityNotFoundError&) {
    return std::unexpected(
        PlaceError{PlaceErrorKind::DontOwnShip, std::format("{}", *shipnum)});
  }
  if (!ctx.god) {
    if (!ship->alive()) {
      return std::unexpected(
          PlaceError{PlaceErrorKind::DontOwnShip, std::format("{}", *shipnum)});
    }
    if (!ignore_explore && ship->owner() != ctx.player) {
      return std::unexpected(
          PlaceError{PlaceErrorKind::DontOwnShip, std::format("{}", *shipnum)});
    }
  }
  level = ScopeLevel::LEVEL_SHIP;
  shipno = ship->number();
  snum = ship->storbits();
  pnum = ship->pnumorbits();
  return {};
}

std::expected<Place, PlaceError> Place::resolve(EntityManager& em,
                                                const ScopeContext& ctx,
                                                std::string_view string,
                                                const bool ignore_explore) {
  Place place(ctx.level, ctx.snum, ctx.pnum,
              (ctx.level == ScopeLevel::LEVEL_SHIP) ? ctx.shipno
                                                    : shipnum_t{0});
  place.entity_manager = &em;

  if (string.empty()) {
    return place;
  }

  std::expected<void, PlaceError> step;
  switch (string.front()) {
    case ':':
      break;
    case '/':
      place.level = ScopeLevel::LEVEL_UNIV;
      place.snum = 0;
      place.pnum = 0;
      place.shipno = 0;
      string.remove_prefix(1);
      step = place.getplace2(em, ctx, string, ignore_explore);
      break;
    case '#':
      step = place.resolve_ship_place(em, ctx, string, ignore_explore);
      break;
    case '-':
      place.level = ScopeLevel::LEVEL_UNIV;
      place.snum = 0;
      place.pnum = 0;
      place.shipno = 0;
      break;
    default:
      step = place.getplace2(em, ctx, string, ignore_explore);
      break;
  }

  if (!step) {
    return std::unexpected(step.error());
  }
  return place;
}

Place::Place(GameObj& g, std::string_view string, const bool ignoreexpl)
    : level(g.level()), snum(g.snum()), pnum(g.pnum()),
      entity_manager(&g.entity_manager) {
  if (level == ScopeLevel::LEVEL_SHIP) shipno = g.shipno();
  auto resolved = Place::resolve(g.entity_manager, ScopeContext::from(g),
                                 string, ignoreexpl);
  if (!resolved) {
    g.out << format_place_error(resolved.error());
    err = true;
    return;
  }
  *this = *resolved;
}
