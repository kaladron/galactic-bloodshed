// SPDX-License-Identifier: Apache-2.0

/// \file planet.cppm
/// \brief Module interface partition for Planet entity, routes, and planetary
/// exploration models.

export module gb.entities:planet;

import :race;
import :rand;
import :types;
import :tweakables;
import std;

export class Star;
export class Sector;
export class SectorMap;

/// Set of commodities selected for loading or unloading on a shipping route.
export struct CommodityManifest {
  bool fuel{false};       ///< Fuel commodity
  bool destruct{false};   ///< Destructive potential commodity
  bool resources{false};  ///< Minerals / resources commodity
  bool crystals{false};   ///< Power crystals commodity

  /// Parse commodity character flags ('f', 'd', 'r', 'x') into a manifest.
  [[nodiscard]] static constexpr CommodityManifest
  parse(std::string_view flags) noexcept {
    CommodityManifest manifest{};
    for (char c : flags) {
      if (c == 'f') manifest.fuel = true;
      if (c == 'd') manifest.destruct = true;
      if (c == 'r') manifest.resources = true;
      if (c == 'x') manifest.crystals = true;
    }
    return manifest;
  }

  /// Format as a compact string containing only active commodity flags.
  [[nodiscard]] std::string format_compact() const {
    std::string flags = fuel ? "f" : "";
    if (destruct) flags += 'd';
    if (resources) flags += 'r';
    if (crystals) flags += 'x';
    return flags;
  }

  /// Returns whether any commodity is selected.
  [[nodiscard]] constexpr bool any() const noexcept {
    return fuel || destruct || resources || crystals;
  }

  [[nodiscard]] bool
  operator==(const CommodityManifest&) const noexcept = default;
};

/// Merchant shipping route parameters.
export struct plroute {
  bool set{false};                ///< Whether this merchant route is active
  starnum_t dest_star{0};         ///< Destination star system ID
  planetnum_t dest_planet{0};     ///< Destination planet number
  CommodityManifest load{};       ///< Commodities to load at destination
  CommodityManifest unload{};     ///< Commodities to unload at destination
  Coordinates dest_coords{0, 0};  ///< Landing coordinates on destination planet

  [[nodiscard]] bool operator==(const plroute&) const noexcept = default;
};

/// \brief Cohesive bundle of planetary commodity stockpiles.
export struct Stockpile {
  resource_t resources{0};
  resource_t destruct{0};
  resource_t fuel{0};
  resource_t crystals{0};

  [[nodiscard]] constexpr bool empty() const noexcept {
    return resources == 0 && destruct == 0 && fuel == 0 && crystals == 0;
  }

  constexpr Stockpile& operator+=(const Stockpile& other) noexcept {
    resources += other.resources;
    destruct += other.destruct;
    fuel += other.fuel;
    crystals += other.crystals;
    return *this;
  }

  constexpr Stockpile& operator-=(const Stockpile& other) noexcept {
    resources -= std::min(resources, other.resources);
    destruct -= std::min(destruct, other.destruct);
    fuel -= std::min(fuel, other.fuel);
    crystals -= std::min(crystals, other.crystals);
    return *this;
  }

  [[nodiscard]] constexpr Stockpile
  clamp_to(const Stockpile& limit) const noexcept {
    return Stockpile{
        .resources = std::min(resources, limit.resources),
        .destruct = std::min(destruct, limit.destruct),
        .fuel = std::min(fuel, limit.fuel),
        .crystals = std::min(crystals, limit.crystals),
    };
  }

  [[nodiscard]] Stockpile split_share(std::size_t shares) const noexcept {
    return Stockpile{
        .resources = round_rand<resource_t>(resources, shares),
        .destruct = round_rand<resource_t>(destruct, shares),
        .fuel = round_rand<resource_t>(fuel, shares),
        .crystals = round_rand<resource_t>(crystals, shares),
    };
  }

  [[nodiscard]] bool operator==(const Stockpile&) const noexcept = default;
};

export struct plinfo {      // planetary stockpiles
  resource_t fuel = 0;      // fuel for powering things
  resource_t destruct = 0;  // destructive potential
  resource_t resource = 0;  // resources in storage
  population_t popn = 0;
  population_t troops = 0;
  resource_t crystals = 0;

  resource_t prod_res = 0;  // shows last update production
  resource_t prod_fuel = 0;
  resource_t prod_dest = 0;
  resource_t prod_crystals = 0;
  money_t prod_money = 0;
  double prod_tech = 0;

  money_t tech_invest = 0;
  std::uint32_t numsectsowned = 0;

  Percentage comread{0};  // combat readiness (mobilization)
  Percentage mob_set{0};  // mobilization target
  std::optional<Percentage> tox_thresh =
      std::nullopt;  // min to build a waste can

  bool explored = false;
  std::uint32_t autorep = 0;
  Percentage tax{0};       // tax rate
  Percentage newtax{0};    // new tax rate (after update)
  std::uint32_t guns = 0;  // number of planet guns (mob/5)

  /* merchant shipping parameters */
  std::array<plroute, MAX_ROUTES> route{};

  /// \brief Access a merchant route slot by its 1-based route number
  /// (`1..MAX_ROUTES`). Throws `std::out_of_range` if out of bounds.
  [[nodiscard]] const plroute& route_at(int route_number) const {
    return route.at(static_cast<std::size_t>(route_number - 1));
  }
  [[nodiscard]] plroute& route_at(int route_number) {
    return route.at(static_cast<std::size_t>(route_number - 1));
  }

  std::uint32_t mob_points = 0;
  double est_production = 0;  // estimated production

  /// \brief Returns a snapshot of current stockpiles.
  [[nodiscard]] Stockpile stockpile() const noexcept {
    return Stockpile{
        .resources = resource,
        .destruct = destruct,
        .fuel = fuel,
        .crystals = crystals,
    };
  }

  /// \brief Atomically extracts all stored stockpiles from this colony and
  /// resets them to zero.
  Stockpile drain_stockpile() noexcept {
    const Stockpile looted = stockpile();
    resource = 0;
    destruct = 0;
    fuel = 0;
    crystals = 0;
    return looted;
  }

  /// \brief Atomically deposits a stockpile bundle into this colony.
  void deposit_stockpile(const Stockpile& bundle) noexcept {
    resource += bundle.resources;
    destruct += bundle.destruct;
    fuel += bundle.fuel;
    crystals += bundle.crystals;
  }

  /// \brief Deposits turn production outputs into planetary stockpiles.
  void deposit_production(resource_t fuel_in, resource_t res_in,
                          resource_t dest_in, resource_t crystals_in) noexcept {
    deposit_stockpile(Stockpile{
        .resources = res_in,
        .destruct = dest_in,
        .fuel = fuel_in,
        .crystals = crystals_in,
    });
  }

  /// \brief Deposits a stockpile bundle into planetary stockpiles.
  void deposit_production(const Stockpile& bundle) noexcept {
    deposit_stockpile(bundle);
  }

  /// \brief Taxes the population on this planet, generating revenue for the
  /// governor. Adjusts the active tax rate towards `newtax` (capped at +5% max
  /// increase per update). If the race has no active government center, tax
  /// collection is disabled (returns 0).
  money_t collect_tax(Race::gov& gov, const Race& race) noexcept;

  /// \brief Deducts technology investment from governor treasury, calculating
  /// and applying tech advancement points to the race. If the treasury has
  /// insufficient funds or no active government center exists, returns 0.0.
  double invest_tech(Race::gov& gov, Race& race) noexcept;

  /// \brief Updates combat readiness and planetary defense guns based on
  /// average mobilization.
  void update_combat_readiness(std::uint32_t total_mob_points) noexcept;
};

// Internal struct holding raw planet data for serialization
export struct planet_struct {
  SystemCoordinates system_coordinates{0.0, 0.0};
  Coordinates dimensions{0, 0};

  PlayerVector<plinfo, MAXPLAYERS> info;
  ConditionValues<int> conditions{};

  population_t popn = 0;
  population_t troops = 0;
  population_t maxpopn = 0;
  resource_t total_resources = 0;

  player_t slaved_to = 0;
  PlanetType type = PlanetType::EARTH;
  std::uint32_t expltimer = 0;
  bool explored = false;

  starnum_t star_id = 0;
  planetnum_t planet_order = 0;
};

export class Planet {
public:
  // Constructors
  Planet() = default;
  Planet(planet_struct in) : data_(in) {}
  Planet(PlanetType type, Coordinates dimensions) {
    data_.type = type;
    data_.dimensions = dimensions;
  }
  Planet(Planet&) = delete;
  Planet& operator=(const Planet&) = delete;
  Planet(Planet&&) = default;
  Planet& operator=(Planet&&) = default;
  ~Planet() = default;

  /// \brief Returns continuous in-system coordinates relative to the host star
  /// (+/- SYSTEMSIZE = 2,000).
  [[nodiscard]] constexpr SystemCoordinates
  system_coordinates() const noexcept {
    return data_.system_coordinates;
  }
  constexpr SystemCoordinates& system_coordinates() noexcept {
    return data_.system_coordinates;
  }

  /// \brief Sets continuous in-system coordinates relative to the host star.
  constexpr void set_system_coordinates(SystemCoordinates coords) noexcept {
    data_.system_coordinates = coords;
  }

  /// \brief Computes absolute galactic position given host star universe
  /// coordinates.
  [[nodiscard]] constexpr UniverseCoordinates
  absolute_coordinates(UniverseCoordinates star_coords) const noexcept {
    return star_coords + system_coordinates();
  }

  /// \brief Computes absolute galactic position given the host star entity.
  [[nodiscard]] UniverseCoordinates
  absolute_coordinates(const Star& star) const noexcept;

  [[nodiscard]] constexpr Coordinates dimensions() const noexcept {
    return data_.dimensions;
  }
  constexpr Coordinates& dimensions() noexcept {
    return data_.dimensions;
  }

  [[nodiscard]] constexpr bool is_valid(const Coordinates c) const noexcept {
    return c.x >= 0 && c.y >= 0 && c.x < data_.dimensions.x &&
           c.y < data_.dimensions.y;
  }

  [[nodiscard]] constexpr Coordinates wrap(const Coordinates c) const noexcept {
    if (data_.dimensions.x == 0) return c;
    int wrapped_x =
        (c.x % data_.dimensions.x + data_.dimensions.x) % data_.dimensions.x;
    return {wrapped_x, c.y};
  }

  [[nodiscard]] constexpr int num_sectors() const noexcept {
    return data_.dimensions.x * data_.dimensions.y;
  }

  [[nodiscard]] population_t popn() const {
    return data_.popn;
  }
  population_t& popn() {
    return data_.popn;
  }

  [[nodiscard]] population_t troops() const {
    return data_.troops;
  }
  population_t& troops() {
    return data_.troops;
  }

  [[nodiscard]] population_t maxpopn() const {
    return data_.maxpopn;
  }
  population_t& maxpopn() {
    return data_.maxpopn;
  }

  [[nodiscard]] resource_t total_resources() const {
    return data_.total_resources;
  }
  resource_t& total_resources() {
    return data_.total_resources;
  }

  [[nodiscard]] player_t slaved_to() const {
    return data_.slaved_to;
  }
  player_t& slaved_to() {
    return data_.slaved_to;
  }

  [[nodiscard]] PlanetType type() const {
    return data_.type;
  }
  PlanetType& type() {
    return data_.type;
  }

  /// \brief Returns whether a sector type is common (native) on this planet.
  [[nodiscard]] constexpr bool
  is_common_sector(SectorType sector) const noexcept {
    return is_common_sector(data_.type, sector);
  }

  /// \brief Returns whether a sector type is common (native) on a planet type.
  [[nodiscard]] static constexpr bool
  is_common_sector(PlanetType planet, SectorType sector) noexcept {
    switch (planet) {
      case PlanetType::EARTH:
        return sector == SectorType::SEC_SEA || sector == SectorType::SEC_LAND;
      case PlanetType::FOREST:
        return sector == SectorType::SEC_SEA ||
               sector == SectorType::SEC_FOREST;
      case PlanetType::DESERT:
        return sector == SectorType::SEC_LAND ||
               sector == SectorType::SEC_MOUNT ||
               sector == SectorType::SEC_DESERT;
      case PlanetType::WATER:
        return sector == SectorType::SEC_SEA;
      case PlanetType::MARS:
        return sector == SectorType::SEC_LAND ||
               sector == SectorType::SEC_MOUNT ||
               sector == SectorType::SEC_DESERT;
      case PlanetType::ICEBALL:
        return sector == SectorType::SEC_LAND ||
               sector == SectorType::SEC_MOUNT || sector == SectorType::SEC_ICE;
      case PlanetType::GASGIANT:
        return sector == SectorType::SEC_GAS;
      case PlanetType::ASTEROID:
        return false;
    }
    return false;
  }

  [[nodiscard]] std::uint32_t expltimer() const noexcept {
    return data_.expltimer;
  }
  std::uint32_t& expltimer() noexcept {
    return data_.expltimer;
  }

  [[nodiscard]] bool explored() const noexcept {
    return data_.explored;
  }
  bool& explored() noexcept {
    return data_.explored;
  }

  [[nodiscard]] starnum_t star_id() const {
    return data_.star_id;
  }
  starnum_t& star_id() {
    return data_.star_id;
  }

  [[nodiscard]] planetnum_t planet_order() const {
    return data_.planet_order;
  }
  planetnum_t& planet_order() {
    return data_.planet_order;
  }

  // Array accessors with bounds checking
  [[nodiscard]] const plinfo& info(player_t player) const {
    return data_.info[player];
  }
  plinfo& info(player_t player) {
    return data_.info[player];
  }
  [[nodiscard]] const plinfo& info(const Race& race) const {
    return data_.info[race.Playernum];
  }
  plinfo& info(const Race& race) {
    return data_.info[race.Playernum];
  }

  /// \brief Returns whether this planet has been explored by the given player.
  [[nodiscard]] constexpr bool is_explored_by(player_t player) const noexcept {
    return data_.info[player].explored;
  }

  /// \brief Marks this planet as explored by the given player.
  constexpr void mark_explored_by(player_t player) noexcept {
    data_.info[player].explored = true;
  }

  [[nodiscard]] Percentage toxicity() const noexcept {
    return Percentage{data_.conditions[TOXIC]};
  }

  [[nodiscard]] int conditions(Conditions cond) const {
    if (cond < 0 || cond > TOXIC) {
      throw std::runtime_error(std::format("Condition {} out of range (max {})",
                                           static_cast<int>(cond),
                                           static_cast<int>(TOXIC)));
    }
    return data_.conditions[cond];
  }
  int& conditions(Conditions cond) {
    if (cond < 0 || cond > TOXIC) {
      throw std::runtime_error(std::format("Condition {} out of range (max {})",
                                           static_cast<int>(cond),
                                           static_cast<int>(TOXIC)));
    }
    return data_.conditions[cond];
  }

  // Existing methods
  [[nodiscard]] double gravity() const;
  [[nodiscard]] double compatibility(const Race&) const;
  [[nodiscard]] ap_t get_points() const;

  /// \brief Checks whether two coordinates are topologically adjacent on this
  /// planet.
  [[nodiscard]] bool is_adjacent(const Coordinates from,
                                 const Coordinates to) const noexcept;

  /// \brief Returns all valid adjacent coordinates surrounding the given
  /// position, respecting east/west toroidal wrapping and north/south polar
  /// boundaries.
  [[nodiscard]] std::vector<Coordinates>
  adjacent_coordinates(Coordinates from) const;

  /// \brief Returns a randomly chosen valid adjacent coordinate on the
  /// planetary grid.
  [[nodiscard]] Coordinates random_adjacent_coordinates(Coordinates from) const;

  /// \brief Updates planetary atmospheric temperature by adding stellar/mirror
  /// thermal variance to baseline surface temperature.
  void update_climate(int temp_variance = 0) noexcept;

  /// \brief Returns whether this planet is currently enslaved to a player.
  [[nodiscard]] bool is_enslaved() const noexcept {
    return data_.slaved_to != 0;
  }

  /// \brief Returns whether this planet is currently enslaved to a foreign
  /// player.
  [[nodiscard]] constexpr bool
  is_enslaved_to_foreign(player_t player) const noexcept {
    return data_.slaved_to != 0 && data_.slaved_to != player;
  }

  /// \brief Checks if a slave revolt is triggered.
  ///
  /// A slave revolt occurs on an enslaved planet when the slave master's
  /// population on the planet falls to or below 0.1% (1/1000th) of the
  /// total planet population, leaving insufficient forces to suppress the
  /// revolt.
  [[nodiscard]] bool is_slave_revolt_triggered() const noexcept {
    if (data_.slaved_to == 0) return false;
    return data_.info[data_.slaved_to].popn <= (data_.popn / 1000);
  }

  /// \brief Calculates the number of random sectors devastated during a slave
  /// revolt (1 sector per 1,000 planet population, plus 1 baseline sector).
  [[nodiscard]] int calculate_revolt_devastation_count() const noexcept {
    return static_cast<int>(data_.popn / 1000) + 1;
  }

  /// \brief Enslaves the planet to a master player.
  void enslave_to(player_t master) noexcept {
    data_.slaved_to = master;
  }

  /// \brief Frees the planet from enslavement.
  void free_slaves() noexcept {
    data_.slaved_to = 0;
  }

  /// \brief Deposits purchased or delivered commodities into the player's
  /// planetary stockpile.
  /// \param type Type of commodity deposited.
  /// \param amount Quantity of commodity to deposit.
  /// \param player Recipient player owning the stockpile.
  void deposit_commodity(CommodType type, resource_t amount,
                         player_t player) noexcept {
    switch (type) {
      case CommodType::RESOURCE:
        info(player).resource += amount;
        break;
      case CommodType::FUEL:
        info(player).fuel += amount;
        break;
      case CommodType::DESTRUCT:
        info(player).destruct += amount;
        break;
      case CommodType::CRYSTAL:
        info(player).crystals += amount;
        break;
    }
  }

  /// \brief Adjusts a sector's population, automatically maintaining
  /// planet-level demographic totals, player colony statistics, and
  /// state-driven sector colonization and abandonment transitions.
  void adjust_sector_population(Sector& sect, player_t player,
                                population_t civ_delta,
                                population_t mil_delta) noexcept;

  /// \brief Transfers population between sectors on this planet, handling
  /// colonization and abandonment transitions automatically.
  void move_sector_population(Sector& from, Sector& to, player_t player,
                              population_t amount,
                              PopulationType type) noexcept;

  /// \brief Reconciles and synchronizes all planet population totals, troop
  /// garrisons, owned sector counts, and mobilization points directly from the
  /// ground-truth sector grid.
  void sync_demographics(const SectorMap& smap) noexcept;

  /// \brief Executes a planetary insurgency revolt against sectors owned by
  /// `victim_race`, transferring revolted sectors to `agent`, eliminating
  /// defending troops, and synchronizing planet demographics.
  /// \return Number of sectors that revolted.
  int revolt(SectorMap& smap, const Race& victim_race, player_t agent);

  /// \brief Updates planetary toxicity based on population overcapacity
  /// relative to maximum supportable capacity. Clamps toxicity within [0, 100].
  void update_toxicity() noexcept {
    if (data_.maxpopn > 0 && data_.conditions[TOXIC] < 100) {
      data_.conditions[TOXIC] += data_.popn / data_.maxpopn;
    }
    data_.conditions[TOXIC] =
        std::clamp<short>(data_.conditions[TOXIC], 0, 100);
  }

  /// \brief If planetary toxicity exceeds ENVIR_DAMAGE_TOX, devastates a random
  /// sector and returns the devastated coordinates, or std::nullopt if no
  /// damage occurred.
  std::optional<Coordinates>
  process_toxic_environmental_damage(SectorMap& smap) const;

  /// \brief Selects an alien colony on the planet to steal resources from.
  [[nodiscard]] std::optional<player_t>
  select_victim_to_steal_from(std::span<const player_t> race_order) const;

  // For repository serialization
  [[nodiscard]] planet_struct get_struct() const {
    return data_;
  }

private:
  planet_struct data_{};
};

export enum class PlunderError {
  NoConquerors,
  EmptyLoot,
};

export struct PlayerLootShare {
  player_t player{0};
  Stockpile share{};

  [[nodiscard]] bool
  operator==(const PlayerLootShare&) const noexcept = default;
};

export struct PlunderDistribution {
  std::vector<PlayerLootShare> shares;
  Stockpile total_loot{};

  [[nodiscard]] bool
  operator==(const PlunderDistribution&) const noexcept = default;
};

/// \brief Divvies up a looted stockpile among conquerors.
/// The first (N - 1) conquerors receive their rounded share, while the final
/// conqueror receives all exact remaining leftovers, ensuring zero loss or
/// creation of commodities. Returns PlunderDistribution on success, or
/// PlunderError if conquerors list is empty or loot is empty.
export std::expected<PlunderDistribution, PlunderError>
calculate_plunder_distribution(Stockpile total_loot,
                               std::span<const player_t> conquerors);

/// \brief Localized exploration state grid for a planet map during turn
/// processing, replacing static TurnStats.Sectinfo arrays.
export class PlanetExplorationContext {
public:
  explicit PlanetExplorationContext(Coordinates dimensions);
  explicit PlanetExplorationContext(const Planet& planet);

  [[nodiscard]] Coordinates dimensions() const noexcept {
    return dimensions_;
  }

  [[nodiscard]] bool in_bounds(Coordinates c) const noexcept;
  [[nodiscard]] bool is_explored(Coordinates c, player_t player) const;
  [[nodiscard]] bool is_explored(Coordinates c) const;

  void set_explored(Coordinates c, player_t player);
  void clear_explored(Coordinates c, player_t player);

  [[nodiscard]] bool all_explored(player_t player) const;
  [[nodiscard]] bool all_explored() const;

  /// \brief Explores sectors surrounding sectors currently explored for player
  /// `p`. If `s.coords()` is already explored by `p`, marks adjacent neighbors
  /// as explored by `p`. If `s.coords()` is not explored by `p`, but owned by
  /// `p`, marks `s.coords()` as explored by `p`.
  void explore_sector(const Planet& planet, const Sector& s, player_t p);

private:
  [[nodiscard]] std::size_t index(Coordinates c) const noexcept {
    return static_cast<std::size_t>(c.y) *
               static_cast<std::size_t>(dimensions_.x) +
           static_cast<std::size_t>(c.x);
  }

  Coordinates dimensions_{0, 0};
  std::vector<std::bitset<MAXPLAYERS + 1>> explored_;
};

/// \brief Calculates destination coordinates on a planet grid for a single
/// compass/keypad direction step with horizontal toroidal wrapping.
export Coordinates get_move(const Planet& planet, char direction,
                            Coordinates from);

//* Return gravity for the Planet
double Planet::gravity() const {
  return static_cast<double>(num_sectors()) * GRAV_FACTOR;
}

double Planet::compatibility(const Race& race) const {
  double atmosphere = 1.0;

  /* make an adjustment for planetary temperature */
  int add = 0.1 * ((double)conditions(TEMP) - race.conditions[TEMP]);
  double sum = 1.0 - ((double)std::abs(add) / 100.0);

  /* step through and report compatibility of each planetary gas */
  for (Conditions cond : all_gas_conditions) {
    add = (double)conditions(cond) - race.conditions[cond];
    atmosphere *= 1.0 - ((double)std::abs(add) / 100.0);
  }
  sum *= atmosphere;
  sum *= 100.0 - conditions(TOXIC);

  if (sum < 0.0) return 0.0;
  return sum;
}

ap_t Planet::get_points() const {
  switch (type()) {
    case PlanetType::ASTEROID:
      return ASTEROID_POINTS;
    case PlanetType::EARTH:
      return int_rand(EARTH_POINTS_LOW, EARTH_POINTS_HIGH);
    case PlanetType::MARS:
      return int_rand(MARS_POINTS_LOW, MARS_POINTS_HIGH);
    case PlanetType::ICEBALL:
      return int_rand(ICEBALL_POINTS_LOW, ICEBALL_POINTS_HIGH);
    case PlanetType::GASGIANT:
      return int_rand(GASGIANT_POINTS_LOW, GASGIANT_POINTS_HIGH);
    case PlanetType::WATER:
      return int_rand(WATER_POINTS_LOW, WATER_POINTS_HIGH);
    case PlanetType::FOREST:
      return int_rand(FOREST_POINTS_LOW, FOREST_POINTS_HIGH);
    case PlanetType::DESERT:
      return int_rand(DESERT_POINTS_LOW, DESERT_POINTS_HIGH);
  }
}

export inline double tech_prod(const money_t investment,
                               const population_t popn) noexcept {
  double scale = static_cast<double>(popn) / 10000.;
  return (TECH_INVEST *
          std::log10(static_cast<double>(investment) * scale + 1.0));
}

export constexpr gun_count_t planet_guns(resource_t points) noexcept {
  if (points < 0) return 0; /* shouldn't happen */
  return static_cast<gun_count_t>(std::min<resource_t>(20, points / 1000));
}
