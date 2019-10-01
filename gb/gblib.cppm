export module gblib;

import std;

export enum ScopeLevel { LEVEL_UNIV, LEVEL_STAR, LEVEL_PLAN, LEVEL_SHIP };

export using shipnum_t = uint64_t;
export using starnum_t = uint8_t;
export using planetnum_t = uint8_t;
export using player_t = uint8_t;
export using governor_t = uint8_t;
export using commodnum_t = uint64_t;
export using resource_t = unsigned long;

export using money_t = int64_t;
export using population_t = uint64_t;

export class GameObj {
 public:
  player_t player;
  governor_t governor;
  bool god;
  double lastx[2] = {0.0, 0.0};
  double lasty[2] = {0.0, 0.0};
  double zoom[2] = {1.0, 0.5};  ///< last coords for zoom
  ScopeLevel level;             ///< what directory level
  starnum_t snum;               ///< what star system obj # (level=0)
  planetnum_t pnum;             ///< number of planet
  shipnum_t shipno;             ///< # of ship
  std::stringstream out;
  GameObj() = default;
  GameObj(const GameObj &) = delete;
  GameObj &operator=(const GameObj &) = delete;
};

