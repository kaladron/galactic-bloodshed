// SPDX-License-Identifier: Apache-2.0

/// \file race.cppm
/// \brief Module interface partition for Race entity and governance models.

export module gb.entities:race;

import :types;
import :tweakables;
import std;

export using toggletype = struct {
  bool invisible;
  bool gag;
  bool double_digits;
  bool inverse;
  bool geography;
  bool autoload;
  player_t highlight; /* which race to highlight */
  bool compat;
};

/// Technology discoveries and breakthrough unlocks achieved by a race.
export struct TechDiscoveries {
  bool hyperdrive{
      false};         ///< Capable of constructing faster-than-light hyperdrives
  bool laser{false};  ///< Capable of constructing combat laser weaponry
  bool cew{
      false};  ///< Capable of constructing Concentrated Energy Weapons (CEWs)
  bool vn{
      false};  ///< Capable of building self-replicating Von Neumann machines
  bool tractor_beam{
      false};  ///< Capable of constructing long-range tractor/repulsor beams
  bool transporter{
      false};  ///< Capable of operating planetary matter transporters
  bool avpm{
      false};  ///< Capable of building Anti-Vehicle Planetary Missiles (AVPM)
  bool cloak{false};  ///< Capable of constructing starship cloaking devices
  bool wormhole{
      false};  ///< Capable of detecting and traversing artificial wormholes
  bool crystal{false};  ///< Capable of synthesizing alien power crystals

  [[nodiscard]] bool
  operator==(const TechDiscoveries&) const noexcept = default;
};

export class Race {
public:
  player_t Playernum{0};
  std::string name; /* Racial name. */
  std::string password;
  std::string info;          /* personal information */
  std::string motto;         /* for a cute message */
  bool absorb{false};        /* Does this race absorb enemies in combat? */
  bool collective_iq{false}; /* Does this race have collective IQ? */
  bool pods{false};          /* Can this race use pods? */
  fighters_t fighters{0};    /* Fight rating of this race. */
  iq_t IQ{0};
  iq_t IQ_limit{0}; /* Asymtotic IQ for collective IQ races. */
  sexes_t number_sexes{1};
  fertilize_t fertilize{0}; /* Chance that this race will increase the
                             fertility of its sectors by 1 each update */
  adventurism_t adventurism{0.0};
  birthrate_t birthrate{0.0};
  mass_t mass{0.0};
  metabolism_t metabolism{0.0};
  temperature_t temp{0}; /* Temperature (Celsius) this race likes. */
  ConditionValues
      conditions{}; /* Atmospheric gas percentages this race likes. */
  SectorCompatibilities likes{}; /* Sector condition compats. */
  SectorType likesbest{
      SectorType::SEC_LAND}; /* 100% compat sector condition for this race. */

  /// Returns this race's compatibility [0.0, 1.0] with the given sector
  /// condition. Throws `std::out_of_range` if `condition` is out of bounds.
  [[nodiscard]] constexpr double
  sector_compatibility(const SectorType condition) const {
    return likes[condition];
  }

  /// Returns this race's compatibility [0.0, 1.0] with the given sector's
  /// current surface condition.
  [[nodiscard]] constexpr double
  sector_compatibility(const class Sector& sect) const;

  /// Returns whether this race can inhabit or traverse a sector with the given
  /// surface condition (`sector_compatibility(condition) > 0.0`).
  [[nodiscard]] constexpr bool
  tolerates_sector(const SectorType condition) const {
    return sector_compatibility(condition) > 0.0;
  }

  /// Returns whether this race can inhabit or traverse the given sector
  /// (`sector_compatibility(sect) > 0.0`).
  [[nodiscard]] constexpr bool tolerates_sector(const class Sector& sect) const;

  /// Returns this race's combat effectiveness multiplier [1.0, 2.0] on the
  /// given sector (`1.0 + sector_compatibility(sect)`).
  [[nodiscard]] constexpr double
  sector_combat_factor(const class Sector& sect) const;

  bool dissolved{false}; /* Player has quit. */
  bool God{false};       /* Player is a God race. */
  bool Guest{false};     /* Player is a guest race. */
  bool Metamorph{false}; /* Player is a morph; (for printing). */

  PlayerVector<int, MAXPLAYERS> translate; /* translation mod for each player */

  /// Increases this race's translation knowledge of `other` by `amount`,
  /// clamped to [0, 100].
  void increase_translation(player_t other, int amount = 5) noexcept {
    translate[other] = std::clamp(translate[other] + amount, 0, 100);
  }

  PlayerBitset<MAXPLAYERS> atwar;
  PlayerBitset<MAXPLAYERS> allied;

  /// Returns whether this race is allied with the given player.
  [[nodiscard]] bool is_allied_with(player_t p) const noexcept;

  /// Declares a diplomatic alliance with the given player.
  void declare_alliance_with(player_t p) noexcept;

  /// Rescinds a diplomatic alliance with the given player.
  void rescind_alliance_with(player_t p) noexcept;

  /// Returns whether this race is at war with the given player.
  [[nodiscard]] bool is_at_war_with(player_t p) const noexcept;

  /// Declares war on the given player.
  void declare_war_on(player_t p) noexcept;

  /// Makes peace with the given player, clearing the at-war state.
  void make_peace_with(player_t p) noexcept;

  std::optional<shipnum_t> Gov_ship{
      std::nullopt}; /* Shipnumber of government ship. */
  [[nodiscard]] constexpr bool has_government_center() const noexcept {
    return Gov_ship.has_value();
  }
  long morale{0}; /* race's morale level */
  PlayerVector<std::uint32_t, MAXPLAYERS>
      points; /* keep track of war status against another player - for short
                 reports */

  /// Adjusts morale and combat victory points following a combat victory over
  /// loser.
  void adjust_morale(Race& loser, int amount) noexcept {
    morale += amount;
    loser.morale -= amount;
    points[loser] += amount;
  }
  planet_count_t controlled_planets{0}; /* Number of planets under control. */
  turn_t victory_turns{0};
  turn_t turn{0};

  double tech{0.0};
  TechDiscoveries discoveries{}; /* Tech discoveries. */

  /// \brief Returns the maximum effective range of this race's planetary
  /// defense guns.
  [[nodiscard]] constexpr double gun_range() const noexcept {
    return ::gun_range(tech);
  }

  victory_score_t victory_score{0}; /* Number of victory points. */
  bool votes{false};
  ap_t planet_points{0}; /* For the determination of global APs */

  struct gov {
    std::string name;
    std::string password;
    ScopeLevel deflevel{ScopeLevel::LEVEL_UNIV};
    starnum_t defsystem{1};
    planetnum_t defplanetnum{1}; /* current default */
    starnum_t homesystem{1};
    planetnum_t homeplanetnum{1}; /* home place */
    NewsValues<int> newspos{};    /* last-read news database IDs per NewsType */
    toggletype toggle{};
    money_t money{0};
    unsigned long income{0};
    money_t maintain{0};
    unsigned long cost_tech{0};
    unsigned long cost_market{0};
    unsigned long profit_market{0};
    std::time_t login{0}; /* last login for this governor */
  };
  /// Canonical governor ID of the race leader.
  static constexpr governor_t leader_id{1};

  /// Returns true if the given governor ID is the race leader.
  [[nodiscard]] static constexpr bool is_leader(governor_t id) noexcept {
    return id == leader_id;
  }

  /// Returns a mutable reference to the race leader's governor entry.
  [[nodiscard]] gov& leader() {
    return governors_.at(leader_id);
  }

  /// Returns a read-only reference to the race leader's governor entry.
  [[nodiscard]] const gov& leader() const {
    return governors_.at(leader_id);
  }

  /// Returns whether the given governor ID is appointed for this race.
  [[nodiscard]] bool has_governor(governor_t id) const noexcept {
    return governors_.contains(id);
  }

  /// Returns a mutable reference to the governor entry for `id`.
  /// Throws `std::out_of_range` if `id` is not appointed for this race.
  [[nodiscard]] gov& governor(governor_t id) {
    auto it = governors_.find(id);
    if (it == governors_.end()) {
      throw std::out_of_range(
          std::format("Governor ID {} out of range", id.value));
    }
    return it->second;
  }

  /// Returns a read-only reference to the governor entry for `id`.
  /// Throws `std::out_of_range` if `id` is not appointed for this race.
  [[nodiscard]] const gov& governor(governor_t id) const {
    auto it = governors_.find(id);
    if (it == governors_.end()) {
      throw std::out_of_range(
          std::format("Governor ID {} out of range", id.value));
    }
    return it->second;
  }

  /// Specification for appointing or configuring a governor via designated
  /// initializers.
  struct GovernorSpec {
    std::string name{};
    std::string password{};
    money_t money{0};
    std::optional<ScopeLevel> deflevel{std::nullopt};
    std::optional<starnum_t> homesystem{std::nullopt};
    std::optional<planetnum_t> homeplanetnum{std::nullopt};
    std::optional<toggletype> toggle{std::nullopt};
  };

  std::flat_map<governor_t, gov> governors_{};

  /// \brief Appoints a new governor at the next available ID (>= 2) with
  /// default settings, returning the assigned governor ID.
  governor_t appoint_governor() {
    return appoint_governor(GovernorSpec{});
  }

  /// \brief Appoints a new governor at the next available ID (>= 2),
  /// inheriting default home scope from the race leader unless overridden in
  /// `spec`, and returns the assigned governor ID.
  governor_t appoint_governor(GovernorSpec spec) {
    const int max_id =
        governors_.empty() ? leader_id.value : governors_.rbegin()->first.value;
    const governor_t next_id{std::max(leader_id.value + 1, max_id + 1)};
    appoint_governor(next_id, std::move(spec));
    return next_id;
  }

  /// \brief Appoints and activates a governor slot with default settings,
  /// inheriting default home scope from the race leader.
  /// Throws `std::out_of_range` if `id < 1` or `std::invalid_argument` if
  /// `is_leader(id)`.
  gov& appoint_governor(governor_t id) {
    return appoint_governor(id, GovernorSpec{});
  }

  /// \brief Appoints and activates a governor slot, inheriting default home
  /// scope from the race leader unless overridden in `spec`.
  /// Throws `std::out_of_range` if `id < 1` or `std::invalid_argument` if
  /// `is_leader(id)`.
  gov& appoint_governor(governor_t id, GovernorSpec spec) {
    if (id.value < 1) {
      throw std::out_of_range(
          std::format("Governor ID {} out of range", id.value));
    }
    if (is_leader(id)) {
      throw std::invalid_argument("Cannot appoint the race leader slot");
    }
    const auto ldr = leader();
    auto& target = governors_[id];
    target = gov{};
    target.name = std::move(spec.name);
    target.password = std::move(spec.password);
    target.money = spec.money;
    target.deflevel = spec.deflevel.value_or(ldr.deflevel);
    target.homesystem = target.defsystem =
        spec.homesystem.value_or(ldr.defsystem);
    target.homeplanetnum = target.defplanetnum =
        spec.homeplanetnum.value_or(ldr.defplanetnum);
    if (spec.toggle) {
      target.toggle = *spec.toggle;
    } else {
      target.toggle.highlight = Playernum;
      target.toggle.inverse = true;
    }
    return target;
  }

  /// \brief Revokes a governor slot, transferring its treasury to `tgt_id` and
  /// removing the revoked governor entry.
  /// Throws `std::invalid_argument` if `is_leader(src_id)` or `src_id ==
  /// tgt_id`, or `std::out_of_range` if `src_id` or `tgt_id` is not an active
  /// governor.
  /// \return The amount of money transferred to `tgt_id`.
  money_t revoke_governor(governor_t src_id, governor_t tgt_id = leader_id) {
    if (is_leader(src_id)) {
      throw std::invalid_argument("Cannot revoke the race leader");
    }
    if (src_id == tgt_id) {
      throw std::invalid_argument(
          "Cannot transfer revoked governor treasury to itself");
    }
    auto& src = governor(src_id);
    auto& tgt = governor(tgt_id);
    const money_t transferred = src.money;
    tgt.money += transferred;
    governors_.erase(src_id);
    return transferred;
  }

  /// \brief Initializes the Race Leader with 1-based home/default coordinates.
  void init_leader(starnum_t home_star = 1, planetnum_t home_planet = 1,
                   std::string gov_password = "",
                   ScopeLevel level = ScopeLevel::LEVEL_PLAN) {
    auto& ldr = governors_[leader_id];
    ldr.name = "Leader";
    ldr.password = std::move(gov_password);
    ldr.deflevel = level;
    ldr.homesystem = ldr.defsystem = home_star;
    ldr.homeplanetnum = ldr.defplanetnum = home_planet;
    ldr.toggle.highlight = Playernum;
    ldr.toggle.inverse = true;
  }

  Race() {
    init_leader();
  }

  /// \brief Resets turn-level economic accounting ledgers, controlled planet
  /// tallies, and player update votes at the start of a turn update.
  void reset_turn_accounting() noexcept {
    controlled_planets = 0;
    planet_points = 0;
    votes = false;
    for (auto [_, gov] : governors_) {
      gov.maintain = 0;
      gov.cost_market = 0;
      gov.profit_market = 0;
      gov.cost_tech = 0;
      gov.income = 0;
    }
  }

  /// \brief Deducts treasury funds for maintenance costs, deducting morale
  /// (clamped to [0, 100]) if treasury funds are insufficient to cover costs.
  void deduct_maintenance(governor_t gov_num, money_t amount) {
    deduct_maintenance(governor(gov_num), amount);
  }

  /// \brief Deducts treasury funds for maintenance costs, deducting morale
  /// (clamped to [0, 100]) if treasury funds are insufficient to cover costs.
  void deduct_maintenance(gov& gov_ref, money_t amount) noexcept {
    if (gov_ref.money >= amount) {
      gov_ref.money -= amount;
    } else {
      const money_t deficit = amount - gov_ref.money;
      const int morale_penalty = static_cast<int>(deficit / 10);
      morale = std::clamp(static_cast<long>(morale - morale_penalty), 0L, 100L);
      gov_ref.money = 0;
    }
  }

  /// \brief Deducts accumulated maintenance costs for all active governors.
  void deduct_all_maintenance() noexcept {
    for (auto [_, gov] : governors_) {
      deduct_maintenance(gov, gov.maintain);
    }
  }

  /// \brief Updates race IQ based on collective population scaling if the race
  /// possesses collective intelligence traits.
  void update_collective_intelligence(population_t total_popn) noexcept {
    if (collective_iq) {
      const double x =
          (2.0 / std::numbers::pi) *
          std::atan(static_cast<double>(total_popn) / MESO_POP_SCALE);
      IQ = static_cast<iq_t>(static_cast<double>(IQ_limit) * x * x);
    }
  }

  /// \brief Returns a read-only view of all active governors.
  [[nodiscard]] const std::flat_map<governor_t, gov>&
  active_governors() const noexcept {
    return governors_;
  }

  /**
   * Provides translated estimates of numeric values based on this race's
   * translation capability toward `target`. Values are rounded based on
   * translation level and formatted with K (thousands) or M (millions)
   * suffixes for readability.
   */
  template <typename T>
    requires(std::is_arithmetic_v<T> || std::convertible_to<T, int>)
  [[nodiscard]] std::string estimate(const T data,
                                     const player_t target) const {
    if (translate[target] > 10) {
      int k = 101 - std::min(translate[target], 100);
      int est = (std::abs(static_cast<int>(data)) / k) * k;
      if (est < 1000) return std::format("{}", est);
      if (est < 10000) {
        return std::format("{:.1f}K", static_cast<double>(est) / 1000.);
      }
      if (est < 1000000) {
        return std::format("{:.0f}K", static_cast<double>(est) / 1000.);
      }

      return std::format("{:.1f}M", static_cast<double>(est) / 1000000.);
    }
    return "?";
  }

  template <typename T>
    requires(std::is_arithmetic_v<T> || std::convertible_to<T, int>)
  [[nodiscard]] std::string estimate(const T data, const Race& target) const {
    return estimate(data, target.Playernum);
  }
};

export struct power {
  int id{0};                   // Power entry ID for database persistence
  population_t troops{0};      /* total troops */
  population_t popn{0};        /* total population */
  resource_t resource{0};      /* total resource in stock */
  resource_t fuel{0};          /* total fuel in stock */
  resource_t destruct{0};      /* total dest in stock */
  ship_count_t ships_owned{0}; /* # of ships owned */
  planet_count_t planets_owned{0};
  money_t money{0};
};

export struct block {
  player_t Playernum{0};
  std::string name;
  std::string motto;
  PlayerBitset<MAXPLAYERS> invited;
  PlayerBitset<MAXPLAYERS> pledged;
  std::uint32_t members{0};
  population_t popn{0};        /* total population */
  resource_t resource{0};      /* total resource in stock */
  resource_t fuel{0};          /* total fuel in stock */
  resource_t destruct{0};      /* total dest in stock */
  ship_count_t ships_owned{0}; /* # of ships owned */
  planet_count_t systems_owned{0};
  victory_score_t VPs{0};
  money_t money{0};

  /// Resets aggregated member power statistics prior to recomputation.
  void clear_power_stats() noexcept {
    members = 0;
    popn = 0;
    resource = 0;
    fuel = 0;
    destruct = 0;
    ships_owned = 0;
    money = 0;
  }

  /// Accumulates a member race's power report into this bloc's totals.
  void accumulate_member_power(const power& p) noexcept {
    members += 1;
    popn += p.popn;
    resource += p.resource;
    fuel += p.fuel;
    destruct += p.destruct;
    ships_owned += p.ships_owned;
    money += p.money;
  }

  /// Adds a player as a full member (both invited and pledged).
  void add_member(player_t p) noexcept {
    invite(p);
    pledge(p);
  }

  /// Returns whether the given player is a member of this bloc (both invited
  /// and pledged).
  [[nodiscard]] bool is_member(player_t p) const noexcept;

  /// Returns the bitset of all members (players that are both invited and
  /// pledged).
  [[nodiscard]] PlayerBitset<MAXPLAYERS> member_mask() const noexcept {
    return invited & pledged;
  }

  /// Returns whether the given player is invited to this bloc.
  [[nodiscard]] bool is_invited(player_t p) const noexcept;

  /// Invites the given player to this bloc.
  void invite(player_t p) noexcept;

  /// Uninvites the given player from this bloc.
  void uninvite(player_t p) noexcept;

  /// Returns whether the given player is pledged to this bloc.
  [[nodiscard]] bool is_pledged(player_t p) const noexcept;

  /// Pledges the given player to this bloc.
  void pledge(player_t p) noexcept;

  /// Unpledges the given player from this bloc.
  void unpledge(player_t p) noexcept;
};

export constexpr double TECH_HYPER_DRIVE = 50.0;
export constexpr double TECH_LASER = 100.0;
export constexpr double TECH_CEW = 150.0;
export constexpr double TECH_VN = 100.0;
export constexpr double TECH_TRACTOR_BEAM = 999.0;
export constexpr double TECH_TRANSPORTER = 999.0;
export constexpr double TECH_AVPM = 250.0;
export constexpr double TECH_CLOAK = 999.0;
export constexpr double TECH_WORMHOLE = 999.0;
export constexpr double TECH_CRYSTAL = 50.0;