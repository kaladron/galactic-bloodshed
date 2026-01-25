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
  explicit Sector(const sector_struct& s) : data_(s) {}

  Sector(unsigned int x_, unsigned int y_, unsigned int eff_,
         unsigned int fert_, unsigned int mobilization_, unsigned int crystals_,
         resource_t resource_, population_t popn_, population_t troops_,
         player_t owner_, player_t race_, unsigned int type_,
         unsigned int condition_)
      : data_{x_,        y_,        eff_,      fert_,   mobilization_,
              crystals_, resource_, popn_,     troops_, owner_,
              race_,     type_,     condition_} {}

  Sector() = default;
  ~Sector() = default;
  Sector(const Sector&) = delete;
  Sector& operator=(const Sector&) = delete;

  // Move constructor
  Sector(Sector&& other) noexcept : data_(std::move(other.data_)) {}

  // Move assignment
  Sector& operator=(Sector&& other) noexcept {
    if (this != &other) {
      data_ = std::move(other.data_);
    }
    return *this;
  }

  // Comparison operator deleted due to complex member (data_)
  auto operator<=>(const Sector&) const = delete;

  // Read accessors (const)
  [[nodiscard]] unsigned int get_x() const noexcept {
    return data_.x;
  }
  [[nodiscard]] unsigned int get_y() const noexcept {
    return data_.y;
  }
  [[nodiscard]] unsigned int get_eff() const noexcept {
    return data_.eff;
  }
  [[nodiscard]] unsigned int get_fert() const noexcept {
    return data_.fert;
  }
  [[nodiscard]] unsigned int get_mobilization() const noexcept {
    return data_.mobilization;
  }
  [[nodiscard]] unsigned int get_crystals() const noexcept {
    return data_.crystals;
  }
  [[nodiscard]] resource_t get_resource() const noexcept {
    return data_.resource;
  }
  [[nodiscard]] population_t get_popn() const noexcept {
    return data_.popn;
  }
  [[nodiscard]] population_t get_troops() const noexcept {
    return data_.troops;
  }
  [[nodiscard]] player_t get_owner() const noexcept {
    return data_.owner;
  }
  [[nodiscard]] player_t get_race() const noexcept {
    return data_.race;
  }
  [[nodiscard]] unsigned int get_type() const noexcept {
    return data_.type;
  }
  [[nodiscard]] unsigned int get_condition() const noexcept {
    return data_.condition;
  }

  // Write accessors (non-const)
  void set_x(unsigned int val) noexcept {
    data_.x = val;
  }
  void set_y(unsigned int val) noexcept {
    data_.y = val;
  }
  void set_fert(unsigned int val) noexcept {
    data_.fert = val;
  }
  void set_mobilization(unsigned int val) noexcept {
    data_.mobilization = val;
  }
  void set_crystals(unsigned int val) noexcept {
    data_.crystals = val;
  }
  void set_resource(resource_t val) noexcept {
    data_.resource = val;
  }

  /// Resource operations with invariant protection
  /// Add resources to sector (no max limit)
  void add_resource(resource_t amount) noexcept;

  /// Remove resources from sector, clamping to zero.
  /// Logs if amount > current resource (invariant violation).
  void subtract_resource(resource_t amount) noexcept;

  /// Efficiency operations with bounds (0-100)
  /// Set efficiency to exact value, clamping to 0-100 bounds.
  /// Logs if input is out of valid range.
  void set_efficiency_bounded(int eff) noexcept;

  /// Improve efficiency by delta, saturating at 100.
  /// Logs if delta is negative (use degrade_efficiency instead).
  void improve_efficiency(int delta) noexcept;

  /// Degrade efficiency by delta, bottoming at 0.
  /// Logs if attempted degradation exceeds current efficiency.
  void degrade_efficiency(int delta) noexcept;

  /// Clear efficiency to 0 (e.g., after terraforming or devastation)
  void clear_efficiency() noexcept {
    data_.eff = 0;
  }

  void set_troops(population_t val) noexcept {
    data_.troops = val;
  }
  void set_owner(player_t val) noexcept {
    data_.owner = val;
  }
  void set_race(player_t val) noexcept {
    data_.race = val;
  }
  void set_type(unsigned int val) noexcept {
    data_.type = val;
  }
  void set_condition(unsigned int val) noexcept {
    data_.condition = val;
  }

  // State predicates - commonly used checks encapsulated as methods
  [[nodiscard]] bool is_owned() const noexcept {
    return data_.owner != 0;
  }
  [[nodiscard]] bool is_empty() const noexcept {
    return data_.popn == 0 && data_.troops == 0;
  }
  [[nodiscard]] bool is_wasted() const noexcept {
    return data_.condition == SectorType::SEC_WASTED;
  }
  [[nodiscard]] bool is_plated() const noexcept {
    return data_.condition == SectorType::SEC_PLATED;
  }

  // State modification methods
  /// Plate the sector - set efficiency to 100 and condition to SEC_PLATED
  /// (unless it's a gas sector)
  void plate() noexcept {
    data_.eff = 100;
    if (data_.condition != SectorType::SEC_GAS)
      data_.condition = SectorType::SEC_PLATED;
  }

  /// Clear ownership if sector is empty (no popn or troops)
  void clear_owner_if_empty() noexcept {
    if (is_empty()) data_.owner = 0;
  }

  /// Population operations with invariant protection
  /// Add population to sector, saturating at a reasonable max
  void add_popn(population_t amount) noexcept;

  /// Remove population from sector, clamping to zero.
  /// Logs if amount > current population (invariant violation).
  void subtract_popn(population_t amount) noexcept;

  /// Atomically transfer population from this sector to another.
  /// Logs if transfer amount exceeds source population.
  void transfer_popn_to(Sector& dest, population_t amount) noexcept;

  /// Check if sector has minimum population
  [[nodiscard]] bool has_popn(population_t min) const noexcept {
    return data_.popn >= min;
  }

  /// Clear all population from sector
  void clear_popn() noexcept {
    data_.popn = 0;
  }

  /// Set population to exact value (used during initialization/loading).
  /// This is the only public population setter - used when loading state
  /// from database or initializing colonization.
  void set_popn_exact(population_t val) noexcept {
    data_.popn = val;
  }

  // Struct conversion methods - FOR SERIALIZATION USE ONLY
  // These methods expose the underlying POD struct for
  // serialization/deserialization. Regular code should use the accessor methods
  // above instead.
  [[nodiscard]] const sector_struct& to_struct() const noexcept {
    return data_;
  }
  [[nodiscard]] sector_struct& to_struct() noexcept {
    return data_;
  }

  friend std::ostream& operator<<(std::ostream&, const Sector&);
};

export class SectorMap {
public:
  SectorMap(const Planet& planet)
      : star_id_(planet.star_id()), planet_order_(planet.planet_order()),
        maxx_(planet.Maxx()), maxy_(planet.Maxy()) {
    grid_.reserve(planet.Maxx() * planet.Maxy());
  }

  //! Add an empty sector for every potential space.  Used for initialization.
  SectorMap(const Planet& planet, bool)
      : star_id_(planet.star_id()), planet_order_(planet.planet_order()),
        maxx_(planet.Maxx()), maxy_(planet.Maxy()),
        grid_(planet.Maxx() * planet.Maxy()) {}

  // Accessors for planet identity
  [[nodiscard]] starnum_t star_id() const {
    return star_id_;
  }
  [[nodiscard]] planetnum_t planet_order() const {
    return planet_order_;
  }

  auto begin() {
    return grid_.begin();
  }
  auto end() {
    return grid_.end();
  }
  auto begin() const {
    return grid_.begin();
  }
  auto end() const {
    return grid_.end();
  }

  Sector& get(const int x, const int y) {
    return grid_.at(static_cast<size_t>(x + (y * maxx_)));
  }

  [[nodiscard]] const Sector& get(const int x, const int y) const {
    return grid_.at(static_cast<size_t>(x + (y * maxx_)));
  }

  // Set from sector_struct
  void set(const int x, const int y, const sector_struct& s) {
    grid_.at(static_cast<size_t>(x + (y * maxx_))) = Sector(s);
  }

  // Set from Sector - extract struct and reconstruct
  void set(const int x, const int y, const Sector& s) {
    grid_.at(static_cast<size_t>(x + (y * maxx_))) = Sector(s.to_struct());
  }

  void put(Sector&& s) {
    grid_.emplace_back(std::move(s));
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
  std::vector<Sector> grid_;
};
