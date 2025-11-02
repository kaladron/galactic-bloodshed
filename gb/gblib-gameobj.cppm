// SPDX-License-Identifier: Apache-2.0

export module gblib:gameobj;

import :types;
import std.compat;

// Forward declaration for EntityManager (defined in gblib:services)
export class EntityManager;

export class GameObj {
 public:
  player_t player;
  governor_t governor;
  bool god = false;
  double lastx[2] = {0.0, 0.0};
  double lasty[2] = {0.0, 0.0};
  double zoom[2] = {1.0, 0.5};                ///< last coords for zoom
  ScopeLevel level = ScopeLevel::LEVEL_PLAN;  ///< what directory level
  starnum_t snum;    ///< what star system obj # (level=0)
  planetnum_t pnum;  ///< number of planet
  shipnum_t shipno;  ///< # of ship
  std::stringstream out;
  EntityManager& entity_manager;  ///< Entity lifecycle manager

  explicit GameObj(EntityManager& em) : entity_manager(em) {}
  GameObj(const GameObj &) = delete;
  GameObj &operator=(const GameObj &) = delete;
};
