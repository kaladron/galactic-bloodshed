// SPDX-License-Identifier: Apache-2.0

export module gblib:sector;

import :types;
import :planet;

// POD struct containing all Sector data fields
export struct sector_struct {
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
};

export class Sector {
private:
  sector_struct data_;  // Private data member for encapsulation

public:
  // Constructor from sector_struct (for new pattern)
  explicit Sector(const sector_struct& s) : data_(s) {
    // Copy to public fields for backward compatibility
    x = data_.x;
    y = data_.y;
    eff = data_.eff;
    fert = data_.fert;
    mobilization = data_.mobilization;
    crystals = data_.crystals;
    resource = data_.resource;
    popn = data_.popn;
    troops = data_.troops;
    owner = data_.owner;
    race = data_.race;
    type = data_.type;
    condition = data_.condition;
  }

  Sector(unsigned int x_, unsigned int y_, unsigned int eff_,
         unsigned int fert_, unsigned int mobilization_, unsigned int crystals_,
         resource_t resource_, population_t popn_, population_t troops_,
         player_t owner_, player_t race_, unsigned int type_,
         unsigned int condition_)
      : x(x_), y(y_), eff(eff_), fert(fert_), mobilization(mobilization_),
        crystals(crystals_), resource(resource_), popn(popn_), troops(troops_),
        owner(owner_), race(race_), type(type_), condition(condition_) {
    // Also populate data_ for consistency
    data_.x = x_;
    data_.y = y_;
    data_.eff = eff_;
    data_.fert = fert_;
    data_.mobilization = mobilization_;
    data_.crystals = crystals_;
    data_.resource = resource_;
    data_.popn = popn_;
    data_.troops = troops_;
    data_.owner = owner_;
    data_.race = race_;
    data_.type = type_;
    data_.condition = condition_;
  }

  Sector() = default;
  ~Sector() = default;
  Sector(Sector&) = delete;
  void operator=(const Sector&) = delete;
  
  // Move constructor - must copy public fields for backward compatibility
  Sector(Sector&& other) noexcept
      : data_(std::move(other.data_)),
        x(other.x), y(other.y), eff(other.eff), fert(other.fert),
        mobilization(other.mobilization), crystals(other.crystals),
        resource(other.resource), popn(other.popn), troops(other.troops),
        owner(other.owner), race(other.race), type(other.type),
        condition(other.condition) {}
  
  // Move assignment - must copy public fields for backward compatibility
  Sector& operator=(Sector&& other) noexcept {
    if (this != &other) {
      data_ = std::move(other.data_);
      x = other.x;
      y = other.y;
      eff = other.eff;
      fert = other.fert;
      mobilization = other.mobilization;
      crystals = other.crystals;
      resource = other.resource;
      popn = other.popn;
      troops = other.troops;
      owner = other.owner;
      race = other.race;
      type = other.type;
      condition = other.condition;
    }
    return *this;
  }
  
  // Comparison operator deleted due to complex member (data_)
  auto operator<=>(const Sector&) const = delete;

  // Read accessors (const)
  [[nodiscard]] unsigned int get_x() const noexcept { return data_.x; }
  [[nodiscard]] unsigned int get_y() const noexcept { return data_.y; }
  [[nodiscard]] unsigned int get_eff() const noexcept { return data_.eff; }
  [[nodiscard]] unsigned int get_fert() const noexcept { return data_.fert; }
  [[nodiscard]] unsigned int get_mobilization() const noexcept { return data_.mobilization; }
  [[nodiscard]] unsigned int get_crystals() const noexcept { return data_.crystals; }
  [[nodiscard]] resource_t get_resource() const noexcept { return data_.resource; }
  [[nodiscard]] population_t get_popn() const noexcept { return data_.popn; }
  [[nodiscard]] population_t get_troops() const noexcept { return data_.troops; }
  [[nodiscard]] player_t get_owner() const noexcept { return data_.owner; }
  [[nodiscard]] player_t get_race() const noexcept { return data_.race; }
  [[nodiscard]] unsigned int get_type() const noexcept { return data_.type; }
  [[nodiscard]] unsigned int get_condition() const noexcept { return data_.condition; }

  // Write accessors (non-const)
  void set_x(unsigned int val) noexcept { data_.x = val; x = val; }
  void set_y(unsigned int val) noexcept { data_.y = val; y = val; }
  void set_eff(unsigned int val) noexcept { data_.eff = val; eff = val; }
  void set_fert(unsigned int val) noexcept { data_.fert = val; fert = val; }
  void set_mobilization(unsigned int val) noexcept { data_.mobilization = val; mobilization = val; }
  void set_crystals(unsigned int val) noexcept { data_.crystals = val; crystals = val; }
  void set_resource(resource_t val) noexcept { data_.resource = val; resource = val; }
  void set_popn(population_t val) noexcept { data_.popn = val; popn = val; }
  void set_troops(population_t val) noexcept { data_.troops = val; troops = val; }
  void set_owner(player_t val) noexcept { data_.owner = val; owner = val; }
  void set_race(player_t val) noexcept { data_.race = val; race = val; }
  void set_type(unsigned int val) noexcept { data_.type = val; type = val; }
  void set_condition(unsigned int val) noexcept { data_.condition = val; condition = val; }

  // Conversion operators
  [[nodiscard]] const sector_struct& to_struct() const noexcept { return data_; }
  [[nodiscard]] sector_struct& to_struct() noexcept { return data_; }

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
  friend std::ostream& operator<<(std::ostream&, const Sector&);
};

export class SectorMap {
public:
  SectorMap(const Planet& planet)
      : star_id_(planet.star_id()), planet_order_(planet.planet_order()),
        maxx_(planet.Maxx()), maxy_(planet.Maxy()) {
    vec_.reserve(planet.Maxx() * planet.Maxy());
  }

  //! Add an empty sector for every potential space.  Used for initialization.
  SectorMap(const Planet& planet, bool)
      : star_id_(planet.star_id()), planet_order_(planet.planet_order()),
        maxx_(planet.Maxx()), maxy_(planet.Maxy()),
        vec_(planet.Maxx() * planet.Maxy()) {}

  // Accessors for planet identity
  [[nodiscard]] starnum_t star_id() const {
    return star_id_;
  }
  [[nodiscard]] planetnum_t planet_order() const {
    return planet_order_;
  }

  // TODO(jeffbailey): Should wrap this in a subclass so the underlying
  // vector isn't exposed to callers.
  auto begin() {
    return vec_.begin();
  }
  auto end() {
    return vec_.end();
  }

  Sector& get(const int x, const int y) {
    return vec_.at(static_cast<size_t>(x + (y * maxx_)));
  }

  [[nodiscard]] const Sector& get(const int x, const int y) const {
    return vec_.at(static_cast<size_t>(x + (y * maxx_)));
  }
  void put(Sector&& s) {
    vec_.emplace_back(std::move(s));
  }
  [[nodiscard]] int get_maxx() const {
    return maxx_;
  }
  [[nodiscard]] int get_maxy() const {
    return maxy_;
  }
  Sector& get_random();
  // TODO(jeffbailey): Don't expose the underlying vector.
  std::vector<std::reference_wrapper<Sector>>
  shuffle();  /// Randomizes the order of the SectorMap.

  SectorMap(SectorMap&) = delete;
  ~SectorMap() = default;
  void operator=(const SectorMap&) = delete;
  SectorMap(SectorMap&&) = default;
  SectorMap& operator=(SectorMap&&) = default;

private:
  SectorMap(const int maxx, const int maxy) : maxx_(maxx), maxy_(maxy) {}
  starnum_t star_id_;
  planetnum_t planet_order_;
  int maxx_;
  int maxy_;
  std::vector<Sector> vec_;
};
