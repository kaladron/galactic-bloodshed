// SPDX-License-Identifier: Apache-2.0

/// \file distance.cc
/// \brief Calculate distance between stars, planets, or coordinates.

module;

import std;
import gb.entities;
import gb.services;

module commands;

namespace GB::commands {

static std::optional<UniverseCoordinates>
resolve_scope_coords(const Place& place, player_t player, EntityManager& em,
                     GameObj& g) {
  switch (place.level) {
    case ScopeLevel::LEVEL_SHIP: {
      const Ship* ship = nullptr;
      try {
        ship = em.peek_ship(place.shipno);
      } catch (const EntityNotFoundError&) {
        g.out << "Ship not found.\n";
        return std::nullopt;
      }
      if (ship->owner() != player) {
        g.out << "Nice try.\n";
        return std::nullopt;
      }
      return ship->coordinates();
    }
    case ScopeLevel::LEVEL_PLAN: {
      const auto* p = em.peek_planet(place.snum, place.pnum);
      if (!p) {
        g.out << "Planet not found.\n";
        return std::nullopt;
      }
      const auto* star = em.peek_star(place.snum);
      if (!star) {
        g.out << "Star not found.\n";
        return std::nullopt;
      }
      return p->absolute_coordinates(*star);
    }
    case ScopeLevel::LEVEL_STAR: {
      const auto* star = em.peek_star(place.snum);
      if (!star) {
        g.out << "Star not found.\n";
        return std::nullopt;
      }
      return star->coordinates();
    }
    default:
      return std::nullopt;
  }
}

bool distance(const command_t& argv, GameObj& g) {
  if (argv.size() < 3) {
    g.out << "Syntax: 'distance <from> <to>'.\n";
    return false;
  }

  Place from{g, argv[1], true};
  if (from.err) {
    g.out << std::format("Bad scope '{}'\n", argv[1]);
    return false;
  }
  Place to{g, argv[2], true};
  if (to.err) {
    g.out << std::format("Bad scope '{}'\n", argv[2]);
    return false;
  }

  const auto from_coords =
      resolve_scope_coords(from, g.player(), g.entity_manager, g);
  if (!from_coords) return false;

  const auto to_coords =
      resolve_scope_coords(to, g.player(), g.entity_manager, g);
  if (!to_coords) return false;

  const double dist = from_coords->distance_to(*to_coords);
  g.out << std::format("Distance = {}\n", dist);
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
