// SPDX-License-Identifier: Apache-2.0

/// \file gblib-sector.cppm
/// \brief Module interface partition for Sector entity and SectorMap grid
/// models.

export module gblib:sector;

import :types;
import :planet;
import :race;

// POD struct containing all Sector data fields
export struct sector_struct {
  Coordinates coords;
  unsigned int eff{0};          /* efficiency (0-100) */
  unsigned int fert{0};         /* max popn is proportional to this */
  unsigned int mobilization{0}; /* percent popn is mobilized for war */
  unsigned int crystals{0};
  resource_t resource{0};

  population_t popn{0};
  population_t troops{0}; /* troops (additional combat value) */

  player_t owner{0};                         /* owner of place */
  player_t race{0};                          /* race type occupying sector
                                 (usually==owner) - makes things more
                                 realistic when alien races revolt and
                                 you gain control of them! */
  SectorType type{SectorType::SEC_SEA};      /* underlying sector geology */
  SectorType condition{SectorType::SEC_SEA}; /* environmental effects */
};

export class Sector {
private:
  sector_struct data_;  // Private data member for encapsulation

public:
  // Constructor from sector_struct (for new pattern)
  explicit Sector(const sector_struct& s) : data_(s) {}

  Sector(Coordinates coords_, unsigned int eff_, unsigned int fert_,
         unsigned int mobilization_, unsigned int crystals_,
         resource_t resource_, population_t popn_, population_t troops_,
         player_t owner_, player_t race_, SectorType type_,
         SectorType condition_)
      : data_{coords_, eff_,    fert_,  mobilization_, crystals_, resource_,
              popn_,   troops_, owner_, race_,         type_,     condition_} {}

  Sector() = default;
  ~Sector() = default;
  Sector(const Sector&) = delete;
  Sector& operator=(const Sector&) = delete;

  // Move constructor
  Sector(Sector&& other) noexcept : data_(other.data_) {}

  // Move assignment
  Sector& operator=(Sector&& other) noexcept {
    if (this != &other) {
      data_ = other.data_;
    }
    return *this;
  }

  // Comparison operator deleted due to complex member (data_)
  auto operator<=>(const Sector&) const = delete;

  // Read accessors (const)
  [[nodiscard]] Coordinates coords() const noexcept {
    return data_.coords;
  }
  [[nodiscard]] unsigned int get_x() const noexcept {
    return static_cast<unsigned int>(data_.coords.x);
  }
  [[nodiscard]] unsigned int get_y() const noexcept {
    return static_cast<unsigned int>(data_.coords.y);
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
  [[nodiscard]] SectorType get_type() const noexcept {
    return data_.type;
  }
  [[nodiscard]] SectorType get_condition() const noexcept {
    return data_.condition;
  }

  // Write accessors (non-const)
  void set_coords(Coordinates val) noexcept {
    data_.coords = val;
  }
  void set_x(unsigned int val) noexcept {
    data_.coords.x = static_cast<int>(val);
  }
  void set_y(unsigned int val) noexcept {
    data_.coords.y = static_cast<int>(val);
  }
  void set_fert(unsigned int val) noexcept {
    data_.fert = val;
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

  /// Mobilization operations with bounds (0-100)
  void adjust_mobilization(int delta) noexcept;
  void set_mobilization_bounded(int val) noexcept;
  void set_mobilization(unsigned int val) noexcept {
    data_.mobilization = val;
  }

  /// Troops operations with invariant protection
  void add_troops(population_t amount) noexcept;
  void subtract_troops(population_t amount) noexcept;
  void clear_troops() noexcept {
    data_.troops = 0;
  }
  void set_troops_exact(population_t val) noexcept {
    data_.troops = val;
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
  void set_type(SectorType val) noexcept {
    data_.type = val;
  }
  void set_condition(SectorType val) noexcept {
    data_.condition = val;
  }

  // State predicates - commonly used checks encapsulated as methods
  [[nodiscard]] constexpr bool is_owned() const noexcept {
    return data_.owner != 0;
  }
  [[nodiscard]] constexpr bool is_owned_by(player_t player) const noexcept {
    return data_.owner == player;
  }
  [[nodiscard]] constexpr bool is_empty() const noexcept {
    return data_.popn == 0 && data_.troops == 0;
  }
  [[nodiscard]] constexpr bool is_populated() const noexcept {
    return data_.popn > 0 || data_.troops > 0;
  }
  [[nodiscard]] constexpr bool is_occupied() const noexcept {
    return is_owned() && is_populated();
  }
  [[nodiscard]] constexpr bool is_wasted() const noexcept {
    return data_.condition == SectorType::SEC_WASTED;
  }
  [[nodiscard]] constexpr bool is_plated() const noexcept {
    return data_.condition == SectorType::SEC_PLATED;
  }
  [[nodiscard]] constexpr bool has_resource() const noexcept {
    return data_.resource > 0;
  }
  [[nodiscard]] constexpr bool has_crystals() const noexcept {
    return data_.crystals > 0;
  }
  [[nodiscard]] bool is_colonizable_by(const Race& race) const noexcept {
    return !is_owned() && !is_wasted() && data_.condition == race.likesbest;
  }
  [[nodiscard]] constexpr bool
  is_colonizable_by(SectorType likesbest) const noexcept {
    return !is_owned() && !is_wasted() && data_.condition == likesbest;
  }
  [[nodiscard]] constexpr bool
  is_bombardable_by(player_t attacker_owner) const noexcept {
    return is_owned() && data_.owner != attacker_owner && !is_wasted();
  }

  // State modification methods
  /// Plate the sector - set efficiency to 100 and condition to SEC_PLATED
  /// (unless it's a gas sector)
  void plate() noexcept {
    data_.eff = 100;
    if (data_.condition != SectorType::SEC_GAS) {
      data_.condition = SectorType::SEC_PLATED;
    }
  }

  /// \brief Devastates a sector: resets condition to SEC_WASTED, and clears
  /// owner, population, troops, mobilization, and efficiency.
  void devastate() noexcept {
    data_.condition = SectorType::SEC_WASTED;
    data_.owner = 0;
    data_.popn = 0;
    data_.troops = 0;
    data_.mobilization = 0;
    data_.eff = 0;
  }

  /// \brief Terraforms sector to new condition, clearing efficiency,
  /// mobilization, population, troops, and owner.
  void terraform(SectorType new_condition) noexcept {
    data_.condition = new_condition;
    data_.eff = 0;
    data_.mobilization = 0;
    data_.popn = 0;
    data_.troops = 0;
    data_.owner = 0;
  }

  /// \brief Colonizes an unowned sector with an initial population and owner.
  void colonize(player_t new_owner, population_t initial_popn,
                player_t race_id = player_t{0}) noexcept {
    data_.owner = new_owner;
    data_.race = (race_id != 0) ? race_id : new_owner;
    data_.popn = initial_popn;
    data_.troops = 0;
  }

  /// \brief Sets sector owner and race.
  void claim(player_t new_owner, player_t race_id = player_t{0}) noexcept {
    data_.owner = new_owner;
    data_.race = (race_id != 0) ? race_id : new_owner;
  }

  /// \brief Applies supernova radiation damage to the sector based on the
  /// star's nova stage. Increments resource by 1, reduces fertility by 20%,
  /// and either kills ~50% of the population or sterilizes at stage 14.
  void apply_supernova(int stage) noexcept;

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
  explicit SectorMap(const Planet& planet)
      : star_id_(planet.star_id()), planet_order_(planet.planet_order()),
        dimensions_(planet.dimensions()),
        grid_(static_cast<std::size_t>(dimensions_.x) *
              static_cast<std::size_t>(dimensions_.y)),
        dirty_(static_cast<std::size_t>(dimensions_.x) *
                   static_cast<std::size_t>(dimensions_.y),
               true) {
    for (int y = 0; y < dimensions_.y; ++y) {
      for (int x = 0; x < dimensions_.x; ++x) {
        grid_[coord_to_idx(Coordinates{x, y})].set_coords(Coordinates{x, y});
      }
    }
  }

  // Accessors for planet identity
  [[nodiscard]] starnum_t star_id() const noexcept {
    return star_id_;
  }
  [[nodiscard]] planetnum_t planet_order() const noexcept {
    return planet_order_;
  }
  [[nodiscard]] constexpr Coordinates dimensions() const noexcept {
    return dimensions_;
  }
  [[nodiscard]] constexpr int num_sectors() const noexcept {
    return dimensions_.x * dimensions_.y;
  }

  // Dirty tracking operations
  [[nodiscard]] bool is_dirty(const Coordinates c) const noexcept {
    if (!in_bounds(c)) return false;
    return dirty_[coord_to_idx(c)];
  }

  [[nodiscard]] bool is_any_dirty() const noexcept {
    return std::ranges::any_of(dirty_, [](bool b) { return b; });
  }

  [[nodiscard]] std::size_t dirty_count() const noexcept {
    return std::ranges::count(dirty_, true);
  }

  void mark_dirty(const Coordinates c) noexcept {
    if (in_bounds(c)) {
      dirty_[coord_to_idx(c)] = true;
    }
  }

  void mark_all_dirty() noexcept {
    std::ranges::fill(dirty_, true);
  }

  void clear_dirty() noexcept {
    std::ranges::fill(dirty_, false);
  }

  auto begin() {
    return grid_.begin();
  }
  auto end() {
    return grid_.end();
  }
  [[nodiscard]] auto begin() const {
    return grid_.begin();
  }
  [[nodiscard]] auto end() const {
    return grid_.end();
  }

  [[nodiscard]] bool in_bounds(const Coordinates c) const noexcept {
    return c.x >= 0 && c.y >= 0 && c.x < dimensions_.x && c.y < dimensions_.y;
  }

  Sector& get(const Coordinates c) {
    if (!in_bounds(c)) {
      throw std::out_of_range(std::format(
          "SectorMap::get({}, {}) out of bounds for dimensions ({}, {})", c.x,
          c.y, dimensions_.x, dimensions_.y));
    }
    dirty_[coord_to_idx(c)] = true;
    return grid_[coord_to_idx(c)];
  }

  [[nodiscard]] const Sector& get(const Coordinates c) const {
    if (!in_bounds(c)) {
      throw std::out_of_range(std::format(
          "SectorMap::get({}, {}) out of bounds for dimensions ({}, {})", c.x,
          c.y, dimensions_.x, dimensions_.y));
    }
    return grid_[coord_to_idx(c)];
  }

  [[nodiscard]] const Sector&
  get_const_ref(const Coordinates c) const noexcept {
    return grid_[coord_to_idx(c)];
  }

  // Set from sector_struct
  void set(const Coordinates c, const sector_struct& s) {
    if (!in_bounds(c)) {
      throw std::out_of_range(std::format(
          "SectorMap::set({}, {}) out of bounds for dimensions ({}, {})", c.x,
          c.y, dimensions_.x, dimensions_.y));
    }
    auto idx = coord_to_idx(c);
    grid_[idx] = Sector(s);
    grid_[idx].set_coords(c);
    dirty_[idx] = true;
  }

  // Set from Sector by moving
  void set(const Coordinates c, Sector&& s) {
    if (!in_bounds(c)) {
      throw std::out_of_range(std::format(
          "SectorMap::set({}, {}) out of bounds for dimensions ({}, {})", c.x,
          c.y, dimensions_.x, dimensions_.y));
    }
    auto idx = coord_to_idx(c);
    grid_[idx] = std::move(s);
    grid_[idx].set_coords(c);
    dirty_[idx] = true;
  }

  // TODO(jeffbailey): Migrate to std::views::cartesian_product once supported
  // by libc++
  class CoordinatesView {
  public:
    class Iterator {
    public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = Coordinates;
      using difference_type = std::ptrdiff_t;

      Iterator(int x, int y, int maxx) : x_(x), y_(y), maxx_(maxx) {}

      value_type operator*() const {
        return Coordinates{x_, y_};
      }
      Iterator& operator++() {
        ++x_;
        if (x_ >= maxx_) {
          x_ = 0;
          ++y_;
        }
        return *this;
      }
      bool operator==(const Iterator& other) const {
        return x_ == other.x_ && y_ == other.y_;
      }

    private:
      int x_{0};
      int y_{0};
      int maxx_{0};
    };

    explicit CoordinatesView(Coordinates dims) : dims_(dims) {}
    [[nodiscard]] Iterator begin() const {
      return Iterator(0, 0, dims_.x);
    }
    [[nodiscard]] Iterator end() const {
      return Iterator(0, dims_.y, dims_.x);
    }

  private:
    Coordinates dims_{0, 0};
  };

  [[nodiscard]] CoordinatesView coordinates() const {
    return CoordinatesView(dimensions_);
  }

  // TODO(jeffbailey): Migrate to std::views::enumerate / cartesian_product once
  // supported by libc++
  template <typename MapType, typename SectorRefType>
  class IndexedSectorsViewImpl {
  public:
    class Iterator {
    public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = std::pair<Coordinates, SectorRefType>;
      using difference_type = std::ptrdiff_t;

      Iterator(MapType* map, int x, int y) : map_(map), x_(x), y_(y) {}

      value_type operator*() const {
        return {Coordinates{x_, y_}, map_->get(Coordinates{x_, y_})};
      }
      Iterator& operator++() {
        ++x_;
        if (x_ >= map_->dimensions().x) {
          x_ = 0;
          ++y_;
        }
        return *this;
      }
      bool operator==(const Iterator& other) const {
        return x_ == other.x_ && y_ == other.y_;
      }

    private:
      MapType* map_{nullptr};
      int x_{0};
      int y_{0};
    };

    IndexedSectorsViewImpl(MapType& map) : map_(&map) {}
    [[nodiscard]] Iterator begin() const {
      return Iterator(map_, 0, 0);
    }
    [[nodiscard]] Iterator end() const {
      return Iterator(map_, 0, map_->dimensions().y);
    }

  private:
    MapType* map_{nullptr};
  };

  template <typename MapType, typename SectorRefType>
  class IndexedDirtySectorsViewImpl {
  public:
    class Iterator {
    public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = std::pair<Coordinates, SectorRefType>;
      using difference_type = std::ptrdiff_t;

      Iterator(MapType* map, int x, int y) : map_(map), x_(x), y_(y) {
        advance_to_dirty();
      }

      value_type operator*() const {
        return {Coordinates{x_, y_}, map_->get_const_ref(Coordinates{x_, y_})};
      }
      Iterator& operator++() {
        advance_next();
        advance_to_dirty();
        return *this;
      }
      bool operator==(const Iterator& other) const {
        return x_ == other.x_ && y_ == other.y_;
      }

    private:
      void advance_next() {
        ++x_;
        if (x_ >= map_->dimensions().x) {
          x_ = 0;
          ++y_;
        }
      }
      void advance_to_dirty() {
        while (y_ < map_->dimensions().y) {
          if (map_->is_dirty(Coordinates{x_, y_})) {
            return;
          }
          advance_next();
        }
      }

      MapType* map_{nullptr};
      int x_{0};
      int y_{0};
    };

    explicit IndexedDirtySectorsViewImpl(MapType& map) : map_(&map) {}
    [[nodiscard]] Iterator begin() const {
      return Iterator(map_, 0, 0);
    }
    [[nodiscard]] Iterator end() const {
      return Iterator(map_, 0, map_->dimensions().y);
    }

  private:
    MapType* map_{nullptr};
  };

  auto indexed_sectors() {
    return IndexedSectorsViewImpl<SectorMap, Sector&>(*this);
  }

  [[nodiscard]] auto indexed_sectors() const {
    return IndexedSectorsViewImpl<const SectorMap, const Sector&>(*this);
  }

  [[nodiscard]] auto indexed_dirty_sectors() const {
    return IndexedDirtySectorsViewImpl<const SectorMap, const Sector&>(*this);
  }

  /// \brief Returns a non-allocating lazy view of all owned sectors.
  [[nodiscard]] auto owned() noexcept {
    return grid_ | std::views::filter(
                       [](const Sector& s) noexcept { return s.is_owned(); });
  }
  [[nodiscard]] auto owned() const noexcept {
    return grid_ | std::views::filter(
                       [](const Sector& s) noexcept { return s.is_owned(); });
  }

  /// \brief Returns a non-allocating lazy view of sectors owned by a specific
  /// player.
  [[nodiscard]] auto owned_by(player_t player) noexcept {
    return grid_ | std::views::filter([player](const Sector& s) noexcept {
             return s.get_owner() == player;
           });
  }
  [[nodiscard]] auto owned_by(player_t player) const noexcept {
    return grid_ | std::views::filter([player](const Sector& s) noexcept {
             return s.get_owner() == player;
           });
  }

  /// \brief Returns a non-allocating lazy view of all populated sectors.
  [[nodiscard]] auto populated() noexcept {
    return grid_ | std::views::filter([](const Sector& s) noexcept {
             return s.is_populated();
           });
  }
  [[nodiscard]] auto populated() const noexcept {
    return grid_ | std::views::filter([](const Sector& s) noexcept {
             return s.is_populated();
           });
  }

  /// \brief Returns a non-allocating lazy view of populated sectors owned by a
  /// specific player.
  [[nodiscard]] auto populated_by(player_t player) noexcept {
    return grid_ | std::views::filter([player](const Sector& s) noexcept {
             return s.get_owner() == player && s.is_populated();
           });
  }
  [[nodiscard]] auto populated_by(player_t player) const noexcept {
    return grid_ | std::views::filter([player](const Sector& s) noexcept {
             return s.get_owner() == player && s.is_populated();
           });
  }

  /// \brief Returns a non-allocating lazy view of all occupied (owned and
  /// populated) sectors.
  [[nodiscard]] auto occupied() noexcept {
    return grid_ | std::views::filter([](const Sector& s) noexcept {
             return s.is_occupied();
           });
  }
  [[nodiscard]] auto occupied() const noexcept {
    return grid_ | std::views::filter([](const Sector& s) noexcept {
             return s.is_occupied();
           });
  }

  template <typename URBG>
  Sector& get_random(URBG& g) {
    std::uniform_int_distribution<int> dis_x(0, dimensions_.x - 1);
    std::uniform_int_distribution<int> dis_y(0, dimensions_.y - 1);
    return get(Coordinates{dis_x(g), dis_y(g)});
  }
  Sector& get_random();

  template <typename URBG>
  const Sector& get_random(URBG& g) const {
    std::uniform_int_distribution<int> dis_x(0, dimensions_.x - 1);
    std::uniform_int_distribution<int> dis_y(0, dimensions_.y - 1);
    return get(Coordinates{dis_x(g), dis_y(g)});
  }
  const Sector& get_random() const;

  template <typename URBG>
  [[nodiscard]] auto shuffle(URBG& g) {
    std::vector<std::size_t> indices(grid_.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::ranges::shuffle(indices, g);

    return std::views::all(std::move(indices)) |
           std::views::transform(
               [this](std::size_t idx) -> Sector& { return grid_[idx]; });
  }
  [[nodiscard]] auto shuffle() {
    return shuffle(game_rng());
  }  /// Randomizes the order of the SectorMap.

  template <typename URBG>
  [[nodiscard]] auto shuffle(URBG& g) const {
    std::vector<std::size_t> indices(grid_.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::ranges::shuffle(indices, g);

    return std::views::all(std::move(indices)) |
           std::views::transform(
               [this](std::size_t idx) -> const Sector& { return grid_[idx]; });
  }
  [[nodiscard]] auto shuffle() const {
    return shuffle(game_rng());
  }  /// Randomizes the order of the SectorMap (const).

  SectorMap(SectorMap&) = delete;
  ~SectorMap() = default;
  void operator=(const SectorMap&) = delete;
  SectorMap(SectorMap&&) = default;
  SectorMap& operator=(SectorMap&&) = default;

private:
  [[nodiscard]] constexpr std::size_t
  coord_to_idx(const Coordinates c) const noexcept {
    return static_cast<std::size_t>(c.x) +
           (static_cast<std::size_t>(c.y) *
            static_cast<std::size_t>(dimensions_.x));
  }

  starnum_t star_id_{0};
  planetnum_t planet_order_{0};
  Coordinates dimensions_{0, 0};
  std::vector<Sector> grid_;
  std::vector<bool> dirty_;
};
