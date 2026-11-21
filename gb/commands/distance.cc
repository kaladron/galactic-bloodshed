// SPDX-License-Identifier: Apache-2.0

/// \file distance.cc
/// \brief Calculate distance between stars, planets, or coordinates.

module;

import gb.entities;
import gb.presentation.core;
import gb.services;
import std;

module commands;

namespace GB::commands {

static std::optional<Place> resolve_distance_place(GameObj& g,
                                                   std::string_view arg) {
  const auto viewer = g.scope_context();
  auto res = Place::resolve(g.entity_manager, viewer, arg, false);
  if (res) {
    return *res;
  }
  if (res.error().kind == PlaceErrorKind::DontOwnShip) {
    if (const auto shipno = string_to_shipnum(arg)) {
      try {
        if (const auto* s = g.entity_manager.peek_ship(*shipno);
            s != nullptr && s->alive()) {
          g.out << "Nice try.\n";
          return std::nullopt;
        }
      } catch (const EntityNotFoundError&) {
      }
    }
  } else if (res.error().kind == PlaceErrorKind::StarUnexplored) {
    if (auto star_only = Place::resolve(g.entity_manager, viewer, arg, true);
        star_only && star_only->level == ScopeLevel::LEVEL_STAR) {
      return *star_only;
    }
  }
  g.out << format_place_error(res.error());
  g.out << std::format("Bad scope '{}'\n", arg);
  return std::nullopt;
}

static std::optional<UniverseCoordinates>
resolve_scope_coords(const Place& place, EntityManager& em, GameObj& g) {
  const auto viewer = g.scope_context();
  switch (place.level) {
    case ScopeLevel::LEVEL_SHIP: {
      const Ship* ship = nullptr;
      try {
        ship = em.peek_ship(place.shipno);
      } catch (const EntityNotFoundError&) {
        g.out << "Ship not found.\n";
        return std::nullopt;
      }
      if (!viewer.owns_ship(*ship)) {
        g.out << "Nice try.\n";
        return std::nullopt;
      }
      return ship->coordinates();
    }
    case ScopeLevel::LEVEL_PLAN: {
      const auto& p = *em.peek_planet(place.snum, place.pnum);
      const auto& star = *em.peek_star(place.snum);
      if (!viewer.can_see_star(star) || !viewer.can_see_planet(p)) {
        g.out << std::format("You have not explored {} yet.\n",
                             !viewer.can_see_star(star)
                                 ? star.get_name()
                                 : star.get_planet_name(place.pnum));
        return std::nullopt;
      }
      return p.absolute_coordinates(star);
    }
    case ScopeLevel::LEVEL_STAR: {
      const auto& star = *em.peek_star(place.snum);
      return star.coordinates();
    }
    default:
      g.out << "Scope has no spatial coordinates.\n";
      return std::nullopt;
  }
}

bool distance(const command_t& argv, GameObj& g) {
  if (argv.size() != 3) {
    g.out << "Syntax: 'distance <from> <to>'.\n";
    return false;
  }

  const auto from_res = resolve_distance_place(g, argv[1]);
  if (!from_res) return false;
  const auto to_res = resolve_distance_place(g, argv[2]);
  if (!to_res) return false;

  const auto from_coords = resolve_scope_coords(*from_res, g.entity_manager, g);
  if (!from_coords) return false;

  const auto to_coords = resolve_scope_coords(*to_res, g.entity_manager, g);
  if (!to_coords) return false;

  g.present(GB::presentation::make_output_message(
      std::format("Distance = {}\n", from_coords->distance_to(*to_coords))));
  return true;
}

static constexpr std::array<std::string_view, 1> distance_aliases{"dist"};

const CommandDescriptor distance_cmd{
    .name = "distance",
    .aliases = distance_aliases,
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 3,
    .syntax = "distance <from> <to>",
    .description = "Calculate distance between stars, planets, or ships",
    .handler = &distance,
};

}  // namespace GB::commands
