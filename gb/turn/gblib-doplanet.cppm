// SPDX-License-Identifier: Apache-2.0

/// \file gblib-doplanet.cppm
/// \brief Module interface partition for planetary lifecycle and turn
/// processing.

export module gblib:doplanet;

import std;
import :planet;
import :sector;
import :services;
import :ships;
import :star;
import :turnstats;
import :types;
import :misc;

export void doplanet(EntityManager&, const Star& star, Planet& planet,
                     TurnStats& stats);

export void moveplanet(EntityManager& entity_manager, const Star& star,
                       Planet& planet);

export enum class GroundMovementError {
  NotTerraformVehicle,
  Stopped,
  EmptyOrders,
  InvalidIndex,
};

export std::expected<char, GroundMovementError>
get_ground_order(const Ship& ship, std::size_t index);

export std::expected<Coordinates, GroundMovementError>
advance_ground_vehicle(Ship& ship, const Planet& planet,
                       EntityManager& entity_manager);

export enum class GroundActionError {
  NotSwitchedOn,
  NotLanded,
  NoCrew,
  InsufficientFuel,
  InsufficientResources,
  MovementFailed,
  SectorAlreadyOptimal,
  IncompatibleSector,
};

export bool moveship_onplanet(Ship& ship, const Planet& planet,
                              EntityManager& entity_manager);

/// \brief Moves terraformer vehicle and attempts to convert target sector
/// condition. Returns whether terraforming succeeded, or GroundActionError.
export std::expected<bool, GroundActionError>
execute_terraforming(Ship& ship, Planet& planet, SectorMap& smap,
                     EntityManager& entity_manager);

/// \brief Moves space plow and improves target sector fertility.
/// Returns fertility increase amount, or GroundActionError.
export std::expected<int, GroundActionError>
execute_plowing(Ship& ship, Planet& planet, SectorMap& smap,
                EntityManager& entity_manager);

/// \brief Upgrades constructor dome efficiency using ship resources.
/// Returns efficiency increase amount, or GroundActionError.
export std::expected<int, GroundActionError>
upgrade_sector_dome(EntityManager& entity_manager, Ship& ship, SectorMap& smap);

/// \brief Strip mines quarry sector, producing resources and generating
/// pollution. Returns resources produced, or GroundActionError.
export std::expected<int, GroundActionError>
strip_mine_quarry(Ship& ship, Planet& planet, SectorMap& smap,
                  EntityManager& entity_manager, TurnStats& stats);

/// \brief Executes berserker bombardment on target planet if in orbit.
/// Decrements VN hitlist on kill or selects next destination planet if no
/// targets found. Returns true if bombardment caused destruction, false
/// otherwise.
export bool execute_berserker_bombardment(EntityManager& entity_manager,
                                          Ship& ship, Planet& planet);

/// \brief Refuels ships in orbit around a gas giant planet based on ship type
/// capacity. Returns amount of fuel added (0.0 if not in orbit or not a gas
/// giant).
export double refuel_gasgiant_orbiters(const Planet& planet, Ship& ship);

/// \brief Processes all planetary ships (VN replication, berserker bombardment,
/// terraforming, plowing, dome construction, weapon plants, quarrying, and gas
/// refueling).
export void process_planetary_ships(EntityManager& entity_manager,
                                    Planet& planet, SectorMap& smap,
                                    TurnStats& stats);

/// \brief Updates planetary temperature and climate based on space mirror
/// warming/cooling trends.
export void process_planet_climate(Planet& planet, const Star& star,
                                   const TurnStats& stats);

/// \brief If planetary toxicity exceeds ENVIR_DAMAGE_TOX, devastates a random
/// sector and returns the devastated coordinates, or std::nullopt if no damage
/// occurred.
export std::optional<Coordinates>
process_toxic_environmental_damage(const Planet& planet, SectorMap& smap);

/// \brief If star is undergoing supernova, applies radiation devastation
/// across all inhabited sectors. Returns true if any inhabited sectors were
/// affected.
export bool process_supernova_sector_devastation(const Star& star,
                                                 SectorMap& smap);

/// \brief If automated waste canister threshold is set and conditions met,
/// builds a toxic waste canister ship, reduces planetary toxicity, and places
/// the ship on the planet. Returns the new ship number if constructed.
export std::optional<shipnum_t>
build_automated_waste_can(EntityManager& entity_manager, const Star& star,
                          Planet& planet, SectorMap& smap, const Race& race);

/// \brief Verifies that all players in the provided list are mutually allied.
/// Returns true if the list contains 0 or 1 player, or if every distinct pair
/// of players has mutual alliance bits set. Returns false if any race cannot be
/// loaded or if any pair is not mutually allied.
export bool check_mutual_alliances(EntityManager& entity_manager,
                                   std::span<const player_t> players);

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

export struct RecoveryReport {
  starnum_t star_id{0};
  std::string star_name;
  std::string planet_name;
  planetnum_t planet_num{0};
  std::vector<player_t> recipients;
  std::vector<PlayerLootShare> allocated_shares;
  Stockpile total_stolen{};

  [[nodiscard]] bool empty() const noexcept {
    return recipients.empty() || allocated_shares.empty() ||
           total_stolen.empty();
  }

  [[nodiscard]] bool operator==(const RecoveryReport&) const noexcept = default;
};

/// \brief Scans planet colonies for conqueror and defeated races. If conquerors
/// are mutually allied and defeated races have stockpiles, distributes the
/// plunder to conquerors, drains defeated races' stockpiles, and returns a
/// RecoveryReport. Returns std::nullopt if no conquerors, no loot, or
/// conquerors are unallied.
export std::optional<RecoveryReport>
recover_conquered_stockpiles(EntityManager& entity_manager, const Star& star,
                             Planet& planet);

/// \brief Formats ASCII recovery report using tabulate from a RecoveryReport.
export std::string format_recovery_report(const RecoveryReport& report,
                                          EntityManager& entity_manager);

/// \brief Formats and sends recovery report telegrams to all recipient
/// conquerors.
export void dispatch_recovery_telegrams(EntityManager& entity_manager,
                                        const Star& star,
                                        const RecoveryReport& report);

export void do_recover(EntityManager& entity_manager, const Star& star,
                       Planet& planet);

/// \brief Localized exploration state grid for a planet map during turn
/// processing, replacing static TurnStats.Sectinfo arrays.
export class PlanetExplorationContext {
public:
  explicit PlanetExplorationContext(Coordinates dimensions)
      : dimensions_(dimensions),
        explored_(static_cast<std::size_t>(dimensions.x) *
                  static_cast<std::size_t>(dimensions.y)) {}

  explicit PlanetExplorationContext(const Planet& planet)
      : PlanetExplorationContext(planet.dimensions()) {}

  [[nodiscard]] Coordinates dimensions() const noexcept {
    return dimensions_;
  }
  [[nodiscard]] int maxx() const noexcept {
    return dimensions_.x;
  }
  [[nodiscard]] int maxy() const noexcept {
    return dimensions_.y;
  }

  [[nodiscard]] bool in_bounds(Coordinates c) const noexcept {
    return c.x >= 0 && c.y >= 0 && c.x < dimensions_.x && c.y < dimensions_.y;
  }

  [[nodiscard]] bool is_explored(Coordinates c, player_t player) const {
    return explored_[index(c)].test(player.value);
  }

  [[nodiscard]] bool is_explored(Coordinates c) const {
    return explored_[index(c)].any();
  }

  void set_explored(Coordinates c, player_t player) {
    explored_[index(c)].set(player.value);
  }

  void clear_explored(Coordinates c, player_t player) {
    explored_[index(c)].reset(player.value);
  }

  [[nodiscard]] bool all_explored(player_t player) const {
    return std::ranges::all_of(explored_, [player](const auto& bitset) {
      return bitset.test(player.value);
    });
  }

  [[nodiscard]] bool all_explored() const {
    return std::ranges::all_of(explored_,
                               [](const auto& bitset) { return bitset.any(); });
  }

  /// \brief Explores sectors surrounding sectors currently explored for player
  /// `p`. If `s.coords()` is already explored by `p`, marks adjacent 4-way
  /// neighbors as explored by `p`. If `s.coords()` is not explored by `p`, but
  /// owned by `p`, marks `s.coords()` as explored by `p`.
  void explore_sector(const Sector& s, player_t p) {
    const Coordinates c = s.coords();
    if (is_explored(c, p)) {
      const int left_x = (c.x > 0) ? (c.x - 1) : (dimensions_.x - 1);
      const int right_x = (c.x + 1 < dimensions_.x) ? (c.x + 1) : 0;
      set_explored(Coordinates{left_x, c.y}, p);
      set_explored(Coordinates{right_x, c.y}, p);
      if (c.y == 0) {
        if (dimensions_.y > 1) {
          set_explored(Coordinates{c.x, 1}, p);
        }
      } else if (c.y == dimensions_.y - 1) {
        set_explored(Coordinates{c.x, c.y - 1}, p);
      } else {
        set_explored(Coordinates{c.x, c.y - 1}, p);
        set_explored(Coordinates{c.x, c.y + 1}, p);
      }
    } else if (s.get_owner() == p) {
      set_explored(c, p);
    }
  }

private:
  [[nodiscard]] std::size_t index(Coordinates c) const noexcept {
    return static_cast<std::size_t>(c.y) *
               static_cast<std::size_t>(dimensions_.x) +
           static_cast<std::size_t>(c.x);
  }

  Coordinates dimensions_{0, 0};
  std::vector<std::bitset<MAXPLAYERS + 1>> explored_;
};

/// \brief Represents an island sector discovered and colonized during
/// exploration.
export struct IslandDiscovery {
  Coordinates coords{0, 0};
  player_t player{0};

  [[nodiscard]] bool
  operator==(const IslandDiscovery&) const noexcept = default;
};

/// \brief Executes planetary sea/island exploration for inhabited races on the
/// planet. If the exploration timer reaches 0, explores adjacent sectors for
/// each race. If a candidate island sector matching race preferences is
/// discovered, colonizes it and returns the discovery details.
export std::optional<IslandDiscovery>
process_island_exploration(EntityManager& entity_manager, const Star& star,
                           Planet& planet, SectorMap& smap, TurnStats& stats);

/// \brief Outcome of turn-based enslavement processing.
export enum class EnslavementOutcome {
  None,
  ProductionDiverted,
  SlaveRevolt,
};

/// \brief Summary of enslavement and revolt processing for a planet.
export struct EnslavementResult {
  EnslavementOutcome outcome{EnslavementOutcome::None};
  player_t master{0};
  int collateral_devastated_count{0};
  int master_devastated_count{0};

  [[nodiscard]] bool
  operator==(const EnslavementResult&) const noexcept = default;
};

/// \brief Diverts planetary production yields (resources, fuel, destruct) from
/// enslaved colonies to the master race's stockpile.
export void divert_slave_tribute(EntityManager& entity_manager, Planet& planet,
                                 TurnStats& stats, player_t master);

/// \brief Sends telegram notifications to all colonies on a planet informing
/// them of a successful slave revolt.
export void notify_slave_revolt(EntityManager& entity_manager, const Star& star,
                                const Planet& planet, player_t former_master);

/// \brief Executes a violent slave revolt on a planet, devastating collateral
/// sectors and former master holdings, sending notifications, and freeing
/// slaves.
export EnslavementResult execute_slave_revolt(EntityManager& entity_manager,
                                              const Star& star, Planet& planet,
                                              SectorMap& smap,
                                              bool intimidated);

/// \brief Resolves planetary enslavement, production tribute diversion, and
/// slave revolts. If the planet is enslaved and master population is sufficient
/// to suppress revolt, diverts production yields to the master. If master
/// population falls below suppression thresholds, triggers a violent slave
/// revolt, devastates sectors, notifies colonies, and frees the planet from
/// enslavement.
export EnslavementResult
process_enslavement_and_revolts(EntityManager& entity_manager, const Star& star,
                                Planet& planet, SectorMap& smap,
                                TurnStats& stats);

/// \brief Recalculates planetary population, troop counts, maximum supportable
/// population, and total mineral resources across all sectors. Updates
/// per-player colony info and empire-wide power statistics in TurnStats.
export void recalculate_census(EntityManager& entity_manager, const Star& star,
                               Planet& planet, const SectorMap& smap,
                               TurnStats& stats);

/// \brief Updates planetary toxicity based on population overcapacity relative
/// to maximum supportable capacity. Clamps toxicity within [0, 100].
export void update_planet_toxicity(Planet& planet);

/// \brief Executes the post-production planetary economy pass: deposits
/// production into stockpiles, collects taxes, invests in technology,
/// constructs automated waste canisters, updates environmental toxicity, and
/// updates empire-wide power metrics and combat readiness.
export void process_planet_economy(EntityManager& entity_manager,
                                   const Star& star, Planet& planet,
                                   SectorMap& smap, TurnStats& stats);

/// \brief Resets transient planetary turn statistics and colony info for all
/// races before turn production starts.
export void reset_planet_turn_state(EntityManager& entity_manager,
                                    Planet& planet, TurnStats& stats);

/// \brief Executes sector production and population spread across all sectors
/// on a planet, or processes supernova devastation if the star is in nova.
export void process_planet_production(EntityManager& entity_manager,
                                      const Star& star, Planet& planet,
                                      SectorMap& smap, TurnStats& stats);

/// \brief Sends autoreport and emergency nova bulletin telegrams to planetary
/// colonies.
export void send_planet_turn_telegrams(
    EntityManager& entity_manager, const Star& star, Planet& planet,
    const std::optional<Coordinates>& envir_damage, const TurnStats& stats);
