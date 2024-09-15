// SPDX-License-Identifier: Apache-2.0

export module gblib:sector;

import :types;
import :planet;

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
  ~Sector() = default;
  Sector(Sector &) = delete;
  void operator=(const Sector &) = delete;
  Sector(Sector &&) = default;
  Sector &operator=(Sector &&) = default;
  auto operator<=>(const Sector &) const = default;

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

export class SectorMap {
 public:
  SectorMap(const Planet &planet) : maxx_(planet.Maxx), maxy_(planet.Maxy) {
    vec_.reserve(planet.Maxx * planet.Maxy);
  }

  //! Add an empty sector for every potential space.  Used for initialization.
  SectorMap(const Planet &planet, bool)
      : maxx_(planet.Maxx),
        maxy_(planet.Maxy),
        vec_(planet.Maxx * planet.Maxy) {}

  // TODO(jeffbailey): Should wrap this in a subclass so the underlying
  // vector isn't exposed to callers.
  auto begin() { return vec_.begin(); }
  auto end() { return vec_.end(); }

  Sector &get(const int x, const int y) {
    return vec_.at(static_cast<size_t>(x + (y * maxx_)));
  }

  [[nodiscard]] const Sector &get(const int x, const int y) const {
    return vec_.at(static_cast<size_t>(x + (y * maxx_)));
  }
  void put(Sector &&s) { vec_.emplace_back(std::move(s)); }
  int get_maxx() { return maxx_; }
  int get_maxy() { return maxy_; }
  Sector &get_random();
  // TODO(jeffbailey): Don't expose the underlying vector.
  std::vector<std::reference_wrapper<Sector>>
  shuffle();  /// Randomizes the order of the SectorMap.

  SectorMap(SectorMap &) = delete;
  ~SectorMap() = default;
  void operator=(const SectorMap &) = delete;
  SectorMap(SectorMap &&) = default;
  SectorMap &operator=(SectorMap &&) = default;

 private:
  SectorMap(const int maxx, const int maxy) : maxx_(maxx), maxy_(maxy) {}
  int maxx_;
  int maxy_;
  std::vector<Sector> vec_;
};
