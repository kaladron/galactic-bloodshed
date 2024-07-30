// SPDX-License-Identifier: Apache-2.0

export module gblib;
export import :planet;
export import :types;
export import :sector;
export import :race;
export import :rand;
export import :tweakables;
export import :globals;

/**
 * \brief Convert input string to a shipnum_t
 * \param s User-provided input string
 * \return If the user provided a valid number, return it.
 */
export inline std::optional<shipnum_t> string_to_shipnum(std::string_view s) {
  while (s.size() > 1 && s.front() == '#') {
    s.remove_prefix(1);
  }

  if (s.size() > 0 && std::isdigit(s.front())) {
    return std::stoi(std::string(s.begin(), s.end()));
  }
  return {};
}

/**
 * \brief Scales used in production efficiency etc.
 * \param x Integer from 0-100
 * \return Float 0.0 - 1.0 (logscaleOB 0.5 - .95)
 */
export inline double logscale(const int x) {
  return log10((double)x + 1.0) / 2.0;
}

export inline double morale_factor(const double x) {
  return (atan((double)x / 10000.) / 3.14159565 + .5);
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

export struct Victory {
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
  bool no_count = false;
  double tech;
  int Thing;
  int IQ;
  unsigned long rawscore;
};

export template <typename T>
concept Unsigned = std::is_unsigned<T>::value;

export template <typename T>
void setbit(T &target, const Unsigned auto pos)
  requires Unsigned<T>
{
  T bit = 1;
  target |= (bit << pos);
}

export template <typename T>
void clrbit(T &target, const Unsigned auto pos)
  requires Unsigned<T>
{
  T bit = 1;
  target &= ~(bit << pos);
}

export template <typename T>
bool isset(const T target, const Unsigned auto pos)
  requires Unsigned<T>
{
  T bit = 1;
  return target & (bit << pos);
}

export template <typename T>
bool isclr(const T target, const Unsigned auto pos)
  requires Unsigned<T>
{
  return !isset(target, pos);
}

export std::vector<Victory> create_victory_list();
