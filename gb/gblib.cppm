export module gblib;

import std;

export enum ScopeLevel { LEVEL_UNIV, LEVEL_STAR, LEVEL_PLAN, LEVEL_SHIP };

export using shipnum_t = uint64_t;
export using starnum_t = uint32_t;
export using planetnum_t = uint32_t;
export using player_t = uint32_t;
export using governor_t = uint32_t;

export using commodnum_t = int64_t;
export using resource_t = int64_t;
export using money_t = int64_t;
export using population_t = int64_t;

export using command_t = std::vector<std::string>;

/**
 * \brief Convert input string to a shipnum_t
 * \param s User-provided input string
 * \return If the user provided a valid number, return it.
 */
export inline std::optional<shipnum_t> string_to_shipnum(std::string_view s) {
  if (s.size() > 1 && s[0] == '#') {
    s.remove_prefix(1);
    return string_to_shipnum(s);
  }

  if (s.size() > 0 && std::isdigit(s[0])) {
    return (std::stoi(std::string(s.begin(), s.end())));
  }
  return {};
}

export class Db {
 public:
  virtual ~Db() {}
  virtual int Numcommods() = 0;
  virtual int Numraces() = 0;

 protected:
  Db() {}
};

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
  Db &db;
  GameObj(Db &db_) : db(db_) {}
  GameObj(const GameObj &) = delete;
  GameObj &operator=(const GameObj &) = delete;
};

export class Sector {
 public:
  Sector(unsigned int x_, unsigned int y_, unsigned int eff_,
         unsigned int fert_, unsigned int mobilization_, unsigned int crystals_,
         resource_t resource_, population_t popn_, population_t troops_,
         player_t owner_, player_t race_, unsigned int type_,
         unsigned int condition_)
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

export struct Commod {
  player_t owner;
  governor_t governor;
  uint8_t type;
  uint64_t amount;
  bool deliver; /* whether the lot is ready for shipping or not */
  money_t bid;
  player_t bidder;
  governor_t bidder_gov;
  starnum_t star_from; /* where the stuff originated from */
  planetnum_t planet_from;
  starnum_t star_to; /* where it goes to */
  planetnum_t planet_to;
};

export class Victory {
 public:
  std::weak_ordering operator<=>(const Victory &that) const {
    // Ensure that folks who shouldn't count are always ranked last.
    if (no_count && !that.no_count) return std::weak_ordering::greater;
    if (that.no_count && !no_count) return std::weak_ordering::less;

    if (that.rawscore > rawscore) return std::weak_ordering::greater;
    if (that.rawscore < rawscore) return std::weak_ordering::less;

    // Must be equal
    return std::weak_ordering::equivalent;
  }
  player_t racenum;
  std::string name;
  bool no_count;
  char sleep;
  double tech;
  int Thing;
  int IQ;
  long rawscore;
  long login;
};

export template <typename T, class = typename std::enable_if<
                                 std::is_unsigned<T>::value>::type>
void setbit(T &target, const unsigned int pos) {
  T bit = 1;
  target |= (bit << pos);
}

export template <typename T, class = typename std::enable_if<
                                 std::is_unsigned<T>::value>::type>
void clrbit(T &target, const unsigned int pos) {
  T bit = 1;
  target &= ~(bit << pos);
}

export template <typename T, class = typename std::enable_if<
                                 std::is_unsigned<T>::value>::type>
bool isset(const T target, const unsigned int pos) {
  T bit = 1;
  return target & (bit << pos);
}

export template <typename T, class = typename std::enable_if<
                                 std::is_unsigned<T>::value>::type>
bool isclr(const T target, const unsigned int pos) {
  return !isset(target, pos);
}
