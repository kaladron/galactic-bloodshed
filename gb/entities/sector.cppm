// SPDX-License-Identifier: Apache-2.0

/// \file sector.cppm
/// \brief Module interface partition for Sector entity and SectorMap grid
/// models.

export module gb.entities:sector;

import :types;
import :tweakables;
import :planet;
import :race;

/// Returns the single-character map symbol for a SectorType.
export constexpr char get_sector_char(SectorType condition) {
  switch (condition) {
    case SectorType::SEC_SEA:
      return CHAR_SEA;
    case SectorType::SEC_LAND:
      return CHAR_LAND;
    case SectorType::SEC_MOUNT:
      return CHAR_MOUNT;
    case SectorType::SEC_GAS:
      return CHAR_GAS;
    case SectorType::SEC_ICE:
      return CHAR_ICE;
    case SectorType::SEC_FOREST:
      return CHAR_FOREST;
    case SectorType::SEC_DESERT:
      return CHAR_DESERT;
    case SectorType::SEC_PLATED:
      return CHAR_PLATED;
    case SectorType::SEC_WASTED:
      return CHAR_WASTED;
  }
  throw std::domain_error("Invalid SectorType in get_sector_char");
}

export template <typename T>
  requires(!std::same_as<T, SectorType>)
constexpr char get_sector_char(T) = delete;

// POD struct containing all Sector data fields
export struct sector_struct {
  Coordinates coords;
  Percentage eff{0};          /* efficiency (0-100) */
  Percentage fert{0};         /* max popn is proportional to this */
  Percentage mobilization{0}; /* percent popn is mobilized for war */
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

  Sector(Coordinates coords_, Percentage eff_, Percentage fert_,
         Percentage mobilization_, unsigned int crystals_, resource_t resource_,
         population_t popn_, population_t troops_, player_t owner_,
         player_t race_, SectorType type_, SectorType condition_)
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
  [[nodiscard]] Percentage get_eff() const noexcept {
    return data_.eff;
  }
  [[nodiscard]] Percentage get_fert() const noexcept {
    return data_.fert;
  }
  [[nodiscard]] Percentage get_mobilization() const noexcept {
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
  [[nodiscard]] constexpr SectorType get_condition() const noexcept {
    return data_.condition;
  }
  [[nodiscard]] constexpr char type_symbol() const {
    return get_sector_char(data_.type);
  }
  [[nodiscard]] constexpr char condition_symbol() const {
    return get_sector_char(data_.condition);
  }

  /// Returns the natural terrain defense bonus for this sector's current
  /// condition (`sector_defense_bonus[condition]`).
  [[nodiscard]] constexpr int defense_bonus() const {
    return sector_defense_bonus[data_.condition];
  }

  /// Returns the terrain combat defense multiplier (`1.0 + defense_bonus()`).
  [[nodiscard]] constexpr double combat_defense_factor() const {
    return static_cast<double>(defense_bonus()) + 1.0;
  }

  // Write accessors (non-const)
  void set_coords(Coordinates val) noexcept {
    data_.coords = val;
  }
  void set_fert(int val) noexcept;
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
  /// Logs if amount > current resource (invariant violation) or if amount < 0.
  void subtract_resource(resource_t amount) noexcept;

  /// Deplete resources from combat, bombardment, or environmental damage.
  /// Clamps smoothly to zero without logging an invariant violation.
  /// Returns the actual quantity depleted.
  resource_t deplete_resource(resource_t amount) noexcept;

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
  void set_mobilization(int val) noexcept {
    set_mobilization_bounded(val);
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

  /// \brief Simulates racial fertilization and natural recovery of wasted
  /// sectors.
  void recover_fertility(const Race& race) noexcept;

  /// \brief Updates sector efficiency and converts fully developed sectors to
  /// plated condition.
  void update_efficiency(const Race& race, const Planet& planet) noexcept;

  /// \brief Extracts raw resources or fuel from a populated sector based on
  /// metabolism and efficiency.
  /// \return Extracted commodity bundle (`Stockpile`).
  [[nodiscard]] Stockpile produce_resources(const Race& race) noexcept;

  /// \brief Mines crystal deposits from a sector if race has crystal discovery.
  /// \return True if a crystal deposit was successfully mined.
  [[nodiscard]] bool mine_crystals(const Race& race) noexcept;

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

constexpr double Race::sector_compatibility(const Sector& sect) const {
  return sector_compatibility(sect.get_condition());
}

constexpr bool Race::tolerates_sector(const Sector& sect) const {
  return sector_compatibility(sect) > 0.0;
}

constexpr double Race::sector_combat_factor(const Sector& sect) const {
  return 1.0 + sector_compatibility(sect);
}

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

  class SectorIterator {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using iterator_concept = std::random_access_iterator_tag;
    using value_type = Sector;
    using difference_type = std::ptrdiff_t;
    using pointer = Sector*;
    using reference = Sector&;

    SectorIterator() = default;
    SectorIterator(SectorMap* map, std::size_t idx) : map_(map), idx_(idx) {}

    reference operator*() const noexcept {
      map_->dirty_[idx_] = true;
      return map_->grid_[idx_];
    }
    pointer operator->() const noexcept {
      map_->dirty_[idx_] = true;
      return &map_->grid_[idx_];
    }
    reference operator[](difference_type n) const noexcept {
      auto i = static_cast<std::size_t>(static_cast<difference_type>(idx_) + n);
      map_->dirty_[i] = true;
      return map_->grid_[i];
    }

    SectorIterator& operator++() noexcept {
      ++idx_;
      return *this;
    }
    SectorIterator operator++(int) noexcept {
      auto tmp = *this;
      ++idx_;
      return tmp;
    }
    SectorIterator& operator--() noexcept {
      --idx_;
      return *this;
    }
    SectorIterator operator--(int) noexcept {
      auto tmp = *this;
      --idx_;
      return tmp;
    }

    SectorIterator& operator+=(difference_type n) noexcept {
      idx_ = static_cast<std::size_t>(static_cast<difference_type>(idx_) + n);
      return *this;
    }
    SectorIterator& operator-=(difference_type n) noexcept {
      idx_ = static_cast<std::size_t>(static_cast<difference_type>(idx_) - n);
      return *this;
    }

    friend SectorIterator operator+(SectorIterator it,
                                    difference_type n) noexcept {
      return it += n;
    }
    friend SectorIterator operator+(difference_type n,
                                    SectorIterator it) noexcept {
      return it += n;
    }
    friend SectorIterator operator-(SectorIterator it,
                                    difference_type n) noexcept {
      return it -= n;
    }
    friend difference_type operator-(const SectorIterator& a,
                                     const SectorIterator& b) noexcept {
      return static_cast<difference_type>(a.idx_) -
             static_cast<difference_type>(b.idx_);
    }

    friend auto operator<=>(const SectorIterator&,
                            const SectorIterator&) noexcept = default;

  private:
    SectorMap* map_{nullptr};
    std::size_t idx_{0};
  };

  [[nodiscard]] SectorIterator begin() noexcept {
    return SectorIterator(this, 0);
  }
  [[nodiscard]] SectorIterator end() noexcept {
    return SectorIterator(this, grid_.size());
  }
  [[nodiscard]] auto begin() const noexcept {
    return grid_.cbegin();
  }
  [[nodiscard]] auto end() const noexcept {
    return grid_.cend();
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

  template <typename Pred>
  class FilteredSectorsView {
  public:
    class Iterator {
    public:
      using iterator_category = std::forward_iterator_tag;
      using iterator_concept = std::forward_iterator_tag;
      using value_type = Sector;
      using difference_type = std::ptrdiff_t;
      using pointer = Sector*;
      using reference = Sector&;

      Iterator() = default;
      Iterator(SectorMap* map, std::size_t idx, Pred pred)
          : map_(map), idx_(idx), pred_(pred) {
        advance_to_match();
      }

      reference operator*() const noexcept {
        map_->dirty_[idx_] = true;
        return map_->grid_[idx_];
      }
      pointer operator->() const noexcept {
        map_->dirty_[idx_] = true;
        return &map_->grid_[idx_];
      }

      Iterator& operator++() noexcept {
        ++idx_;
        advance_to_match();
        return *this;
      }
      Iterator operator++(int) noexcept {
        auto tmp = *this;
        ++(*this);
        return tmp;
      }

      friend bool operator==(const Iterator& a, const Iterator& b) noexcept {
        return a.idx_ == b.idx_;
      }

    private:
      void advance_to_match() noexcept {
        while (map_ && idx_ < map_->grid_.size()) {
          if (pred_(map_->grid_[idx_])) {
            return;
          }
          ++idx_;
        }
      }

      SectorMap* map_{nullptr};
      std::size_t idx_{0};
      Pred pred_{};
    };

    FilteredSectorsView(SectorMap& map, Pred pred) : map_(&map), pred_(pred) {}

    [[nodiscard]] Iterator begin() const noexcept {
      return Iterator(map_, 0, pred_);
    }
    [[nodiscard]] Iterator end() const noexcept {
      return Iterator(map_, map_->grid_.size(), pred_);
    }

  private:
    SectorMap* map_{nullptr};
    Pred pred_{};
  };

  class ShuffledSectorsView {
  public:
    class Iterator {
    public:
      using iterator_category = std::forward_iterator_tag;
      using iterator_concept = std::forward_iterator_tag;
      using value_type = Sector;
      using difference_type = std::ptrdiff_t;
      using pointer = Sector*;
      using reference = Sector&;

      Iterator() = default;
      Iterator(SectorMap* map, std::vector<std::size_t>::const_iterator it)
          : map_(map), it_(it) {}

      reference operator*() const noexcept {
        map_->dirty_[*it_] = true;
        return map_->grid_[*it_];
      }
      pointer operator->() const noexcept {
        map_->dirty_[*it_] = true;
        return &map_->grid_[*it_];
      }

      Iterator& operator++() noexcept {
        ++it_;
        return *this;
      }
      Iterator operator++(int) noexcept {
        auto tmp = *this;
        ++it_;
        return tmp;
      }

      friend bool operator==(const Iterator& a, const Iterator& b) noexcept {
        return a.it_ == b.it_;
      }

    private:
      SectorMap* map_{nullptr};
      std::vector<std::size_t>::const_iterator it_;
    };

    ShuffledSectorsView(SectorMap& map, std::vector<std::size_t> indices)
        : map_(&map), indices_(std::move(indices)) {}

    [[nodiscard]] Iterator begin() const noexcept {
      return Iterator(map_, indices_.cbegin());
    }
    [[nodiscard]] Iterator end() const noexcept {
      return Iterator(map_, indices_.cend());
    }

  private:
    SectorMap* map_{nullptr};
    std::vector<std::size_t> indices_;
  };

  struct IsOwnedPred {
    bool operator()(const Sector& s) const noexcept {
      return s.is_owned();
    }
  };
  struct IsOwnedByPred {
    player_t player;
    bool operator()(const Sector& s) const noexcept {
      return s.get_owner() == player;
    }
  };
  struct IsPopulatedPred {
    bool operator()(const Sector& s) const noexcept {
      return s.is_populated();
    }
  };
  struct IsPopulatedByPred {
    player_t player;
    bool operator()(const Sector& s) const noexcept {
      return s.get_owner() == player && s.is_populated();
    }
  };
  struct IsOccupiedPred {
    bool operator()(const Sector& s) const noexcept {
      return s.is_occupied();
    }
  };

  class ConstShuffledSectorsView {
  public:
    class Iterator {
    public:
      using iterator_category = std::forward_iterator_tag;
      using iterator_concept = std::forward_iterator_tag;
      using value_type = const Sector;
      using difference_type = std::ptrdiff_t;
      using pointer = const Sector*;
      using reference = const Sector&;

      Iterator() = default;
      Iterator(const SectorMap* map,
               std::vector<std::size_t>::const_iterator it)
          : map_(map), it_(it) {}

      reference operator*() const noexcept {
        return map_->grid_[*it_];
      }
      pointer operator->() const noexcept {
        return &map_->grid_[*it_];
      }

      Iterator& operator++() noexcept {
        ++it_;
        return *this;
      }
      Iterator operator++(int) noexcept {
        auto tmp = *this;
        ++it_;
        return tmp;
      }

      friend bool operator==(const Iterator& a, const Iterator& b) noexcept {
        return a.it_ == b.it_;
      }

    private:
      const SectorMap* map_{nullptr};
      std::vector<std::size_t>::const_iterator it_;
    };

    ConstShuffledSectorsView(const SectorMap& map,
                             std::vector<std::size_t> indices)
        : map_(&map), indices_(std::move(indices)) {}

    [[nodiscard]] Iterator begin() const noexcept {
      return Iterator(map_, indices_.cbegin());
    }
    [[nodiscard]] Iterator end() const noexcept {
      return Iterator(map_, indices_.cend());
    }

  private:
    const SectorMap* map_{nullptr};
    std::vector<std::size_t> indices_;
  };

  /// \brief Returns a non-allocating lazy view of all owned sectors.
  [[nodiscard]] auto owned() noexcept {
    return FilteredSectorsView(*this, IsOwnedPred{});
  }
  [[nodiscard]] auto owned() const noexcept {
    return grid_ | std::views::filter(IsOwnedPred{});
  }

  /// \brief Returns a non-allocating lazy view of sectors owned by a specific
  /// player.
  [[nodiscard]] auto owned_by(player_t player) noexcept {
    return FilteredSectorsView(*this, IsOwnedByPred{player});
  }
  [[nodiscard]] auto owned_by(player_t player) const noexcept {
    return grid_ | std::views::filter(IsOwnedByPred{player});
  }

  /// \brief Returns a non-allocating lazy view of all populated sectors.
  [[nodiscard]] auto populated() noexcept {
    return FilteredSectorsView(*this, IsPopulatedPred{});
  }
  [[nodiscard]] auto populated() const noexcept {
    return grid_ | std::views::filter(IsPopulatedPred{});
  }

  /// \brief Returns a non-allocating lazy view of populated sectors owned by a
  /// specific player.
  [[nodiscard]] auto populated_by(player_t player) noexcept {
    return FilteredSectorsView(*this, IsPopulatedByPred{player});
  }
  [[nodiscard]] auto populated_by(player_t player) const noexcept {
    return grid_ | std::views::filter(IsPopulatedByPred{player});
  }

  /// \brief Returns a non-allocating lazy view of all occupied (owned and
  /// populated) sectors.
  [[nodiscard]] auto occupied() noexcept {
    return FilteredSectorsView(*this, IsOccupiedPred{});
  }
  [[nodiscard]] auto occupied() const noexcept {
    return grid_ | std::views::filter(IsOccupiedPred{});
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
  [[nodiscard]] ShuffledSectorsView shuffle(URBG& g) {
    std::vector<std::size_t> indices(grid_.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::ranges::shuffle(indices, g);
    return ShuffledSectorsView(*this, std::move(indices));
  }
  [[nodiscard]] ShuffledSectorsView shuffle() {
    return shuffle(game_rng());
  }  /// Randomizes the order of the SectorMap.

  template <typename URBG>
  [[nodiscard]] ConstShuffledSectorsView shuffle(URBG& g) const {
    std::vector<std::size_t> indices(grid_.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::ranges::shuffle(indices, g);
    return ConstShuffledSectorsView(*this, std::move(indices));
  }
  [[nodiscard]] ConstShuffledSectorsView shuffle() const {
    return shuffle(game_rng());
  }  /// Randomizes the order of the SectorMap (const).

  SectorMap(SectorMap&) = delete;
  ~SectorMap() = default;
  void operator=(const SectorMap&) = delete;
  SectorMap(SectorMap&&) = default;
  SectorMap& operator=(SectorMap&&) = default;

  /// \brief If star is undergoing supernova, applies radiation devastation
  /// across all inhabited sectors. Returns true if any inhabited sectors were
  /// affected.
  bool process_supernova_devastation(const Star& star);

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

/// Scaling divisor for raw morale in the arctangent normalization curve.
/// At +/-10,000 morale, atan(x / MORALE_ATAN_SCALE) reaches +/-pi/4
/// (yielding morale_factor = 0.75 or 0.25).
export constexpr double MORALE_ATAN_SCALE = 10000.0;

/// Baseline morale factor when raw morale is 0 (atan(0) / pi + 0.5 = 0.5).
export constexpr double MORALE_FACTOR_MIDPOINT = 0.5;

/// Maps an empire's raw morale score (-infinity, +infinity) onto a normalized
/// multiplier in the open interval (0.0, 1.0), centered at 0.5 when morale = 0:
///   morale_factor(x) = atan(x / 10000) / pi + 0.5
export constexpr double morale_factor(const double x) {
  return (std::atan(x / MORALE_ATAN_SCALE) / std::numbers::pi +
          MORALE_FACTOR_MIDPOINT);
}

/**
 * @brief Calculate the maximum population a sector can support for a given
 * race.
 *
 * Determines the carrying capacity of a sector based on multiple factors
 * including the race's preference for the sector type, sector productivity
 * (efficiency and fertility), race-planet compatibility, and environmental
 * toxicity.
 *
 * @param r The race that owns or would own the sector
 * @param s The sector being evaluated
 * @param c Compatibility factor (0.0-100.0) representing how well the race
 * adapts to the planet's overall conditions
 * @param toxic Toxicity level (0-100) of the planet - higher values reduce
 * capacity
 *
 * @return Maximum population the sector can support. Returns 0 if the race
 * cannot inhabit this sector type (likes value is 0).
 *
 * @note The calculation incorporates:
 *       - Race preference: r.likes[sector_type] must be non-zero
 *       - Sector productivity: (efficiency + 1) * fertility
 *       - Compatibility: Scaled by race's adaptation to planet conditions
 *       - Toxicity penalty: Reduces capacity as (100 - toxic)%
 */
export constexpr auto maxsupport(const Race& r, const Sector& s, const double c,
                                 const int toxic) {
  if (!r.tolerates_sector(s)) return 0L;
  double a = ((double)s.get_eff() + 1.0) * (double)s.get_fert();
  double b = (.01 * c);

  auto val = std::lround(a * b * .01 * (100.0 - (double)toxic));

  return val;
}

/// \brief Computes how many colonists migrate to an unowned adjacent target
/// sector.
export population_t
calculate_migrating_colonists(const Race& race, double compatibility,
                              const Sector& target,
                              population_t available_migrants);

/// \brief Computes population change for a sector during turn simulation.
export population_t calculate_population_change(const Race& race,
                                                const Sector& s,
                                                population_t maxsup);
