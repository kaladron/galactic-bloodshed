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

export class Sector {
 public:
  Sector(int x_, int y_, int eff_, int fert_, int mobilization_, int crystals_,
         int resource_, int popn_, int troops_, int owner_, int race_,
         int type_, int condition_)
      : x(x_),
        y(y_),
        eff(eff_),
        fert(fert_),
        mobilization(mobilization_),
        crystals(crystals_),
        resource(resource_),
        popn(popn_),
        troops(troops_),
        owner(owner_),
        race(race_),
        type(type_),
        condition(condition_) {}

  Sector() = default;
  Sector(Sector &) = delete;
  void operator=(const Sector &) = delete;
  Sector(Sector &&) = default;
  Sector &operator=(Sector &&) = default;

  unsigned int x{0};
  unsigned int y{0};
  unsigned int eff{0};          /* efficiency (0-100) */
  unsigned int fert{0};         /* max popn is proportional to this */
  unsigned int mobilization{0}; /* percent popn is mobilized for war */
  unsigned int crystals{0};
  resource_t resource{0};

  population_t popn{0};
  population_t troops{0}; /* troops (additional combat value) */

  player_t owner{0};         /* owner of place */
  player_t race{0};          /* race type occupying sector
                 (usually==owner) - makes things more
                 realistic when alien races revolt and
                 you gain control of them! */
  unsigned int type{0};      /* underlying sector geology */
  unsigned int condition{0}; /* environmental effects */
  friend std::ostream &operator<<(std::ostream &, const Sector &);
};

export struct vic {
  unsigned char racenum;
  std::string name;
  bool no_count;
  char sleep;
  double tech;
  int Thing;
  int IQ;
  long rawscore;
  long login;
};
