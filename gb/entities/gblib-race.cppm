// SPDX-License-Identifier: Apache-2.0

/// \file gblib-race.cppm
/// \brief Module interface partition for Race entity and governance models.

export module gblib:race;

import :types;
import :tweakables;
import std;

export using toggletype = struct {
  bool invisible;
  bool standby;
  bool color; /* true if you are using a color client */
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
  unsigned int fighters{0};  /* Fight rating of this race. */
  int IQ{0};
  int IQ_limit{0}; /* Asymtotic IQ for collective IQ races. */
  unsigned int number_sexes{1};
  unsigned int fertilize{0}; /* Chance that this race will increase the
                              fertility of its sectors by 1 each update */
  double adventurism{0.0};
  double birthrate{0.0};
  double mass{0.0};
  double metabolism{0.0};
  short conditions[OTHER + 1]{}; /* Atmosphere/temperature this race likes. */
  double likes[SectorType::SEC_WASTED + 1]{}; /* Sector condition compats. */
  SectorType likesbest{
      SectorType::SEC_LAND}; /* 100% compat sector condition for this race. */

  bool dissolved{false}; /* Player has quit. */
  bool God{false};       /* Player is a God race. */
  bool Guest{false};     /* Player is a guest race. */
  bool Metamorph{false}; /* Player is a morph; (for printing). */
  bool monitor{false};
  /* God is monitering this race. */  // TODO(jeffbailey): Remove this.

  PlayerVector<int, MAXPLAYERS>
      translate{}; /* translation mod for each player */

  std::uint64_t atwar{0};
  std::uint64_t allied{0};

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

  shipnum_t Gov_ship{0}; /* Shipnumber of government ship. */
  [[nodiscard]] bool has_government_center() const noexcept {
    return Gov_ship != 0;
  }
  long morale{0}; /* race's morale level */
  PlayerVector<std::uint32_t, MAXPLAYERS>
      points{}; /* keep track of war status against another player - for short
                   reports */

  /// Adjusts morale and combat victory points following a combat victory over
  /// loser.
  void adjust_morale(Race& loser, int amount) noexcept {
    morale += amount;
    loser.morale -= amount;
    points[loser] += amount;
  }
  unsigned short controlled_planets{0}; /* Number of planets under control. */
  unsigned short victory_turns{0};
  unsigned short turn{0};

  double tech{0.0};
  TechDiscoveries discoveries{};  /* Tech discoveries. */
  unsigned long victory_score{0}; /* Number of victory points. */
  bool votes{false};
  ap_t planet_points{0}; /* For the determination of global APs */

  int governors{0};
  struct gov {
    std::string name;
    std::string password;
    bool active{false};
    ScopeLevel deflevel{ScopeLevel::LEVEL_UNIV};
    starnum_t defsystem{0};
    planetnum_t defplanetnum{0}; /* current default */
    ScopeLevel homelevel{ScopeLevel::LEVEL_UNIV};
    starnum_t homesystem{0};
    planetnum_t homeplanetnum{0}; /* home place */
    unsigned long newspos[4]{};   /* news file pointers */
    toggletype toggle{};
    money_t money{0};
    unsigned long income{0};
    money_t maintain{0};
    unsigned long cost_tech{0};
    unsigned long cost_market{0};
    unsigned long profit_market{0};
    std::time_t login{0}; /* last login for this governor */
  } governor[MAXGOVERNORS + 1];

  /// \brief Resets turn-level economic accounting ledgers, controlled planet
  /// tallies, and player update votes at the start of a turn update.
  void reset_turn_accounting() noexcept {
    controlled_planets = 0;
    planet_points = 0;
    votes = false;
    for (auto& gov : governor) {
      if (gov.active) {
        gov.maintain = 0;
        gov.cost_market = 0;
        gov.profit_market = 0;
        gov.cost_tech = 0;
        gov.income = 0;
      }
    }
  }

  /// \brief Deducts treasury funds for maintenance costs, deducting morale
  /// (clamped to [0, 100]) if treasury funds are insufficient to cover costs.
  void deduct_maintenance(governor_t gov_num, money_t amount) noexcept {
    deduct_maintenance(governor[gov_num.value], amount);
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

  /// \brief Updates race IQ based on collective population scaling if the race
  /// possesses collective intelligence traits.
  void update_collective_intelligence(population_t total_popn) noexcept {
    if (collective_iq) {
      double x = ((2.0 / std::numbers::pi) *
                  std::atan(static_cast<double>(total_popn) / MESO_POP_SCALE));
      IQ = static_cast<unsigned short>(IQ_limit * x * x);
    }
  }

  // Iterate over active governors only
  [[nodiscard]] auto active_governors() const;

  // Iterate over all governors (active or not)
  [[nodiscard]] auto all_governors() const;
};

// Entry returned when iterating over governors (const version)
export struct GovernorEntry {
  governor_t id;
  const Race::gov& data;
};

// Range class for iterating over active governors only
export class ActiveGovernorRange {
  const Race* race_;

public:
  explicit ActiveGovernorRange(const Race* r) : race_(r) {}

  class Iterator {
    const Race* race_;
    int current_;  // Use int internally for array access

    void advance_to_active() {
      while (current_ <= MAXGOVERNORS && !race_->governor[current_].active) {
        ++current_;
      }
    }

  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = GovernorEntry;
    using difference_type = std::ptrdiff_t;

    Iterator(const Race* r, int start) : race_(r), current_(start) {
      advance_to_active();
    }

    GovernorEntry operator*() const {
      return {static_cast<governor_t>(current_), race_->governor[current_]};
    }

    Iterator& operator++() {
      ++current_;
      advance_to_active();
      return *this;
    }

    bool operator==(const Iterator& other) const {
      return current_ == other.current_;
    }
    bool operator!=(const Iterator& other) const {
      return current_ != other.current_;
    }
  };

  [[nodiscard]] Iterator begin() const {
    return Iterator(race_, 0);
  }
  [[nodiscard]] Iterator end() const {
    return Iterator(race_, MAXGOVERNORS + 1);
  }
};

// Range class for iterating over ALL governors (active or not)
export class AllGovernorRange {
  const Race* race_;

public:
  explicit AllGovernorRange(const Race* r) : race_(r) {}

  class Iterator {
    const Race* race_;
    int current_;

  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = GovernorEntry;
    using difference_type = std::ptrdiff_t;

    Iterator(const Race* r, int start) : race_(r), current_(start) {}

    GovernorEntry operator*() const {
      return {static_cast<governor_t>(current_), race_->governor[current_]};
    }

    Iterator& operator++() {
      ++current_;
      return *this;
    }
    bool operator==(const Iterator& other) const {
      return current_ == other.current_;
    }
    bool operator!=(const Iterator& other) const {
      return current_ != other.current_;
    }
  };

  [[nodiscard]] Iterator begin() const {
    return Iterator(race_, 0);
  }
  [[nodiscard]] Iterator end() const {
    return Iterator(race_, MAXGOVERNORS + 1);
  }
};

inline auto Race::active_governors() const {
  return ActiveGovernorRange(this);
}

inline auto Race::all_governors() const {
  return AllGovernorRange(this);
}

export struct power {
  int id{0};           // Power entry ID for database persistence
  population_t troops; /* total troops */
  population_t popn;   /* total population */
  resource_t resource; /* total resource in stock */
  unsigned long fuel;
  unsigned long destruct;     /* total dest in stock */
  unsigned short ships_owned; /* # of ships owned */
  unsigned short planets_owned;
  unsigned long sectors_owned;
  money_t money;
  unsigned long sum_mob; /* total mobilization */
  unsigned long sum_eff; /* total efficiency */
};

export struct block {
  player_t Playernum;
  std::string name;
  std::string motto;
  std::uint64_t invite;
  std::uint64_t pledge;
  std::uint64_t atwar;
  std::uint64_t allied;
  unsigned short next;
  unsigned short systems_owned;
  unsigned long VPs;
  unsigned long money;

  /// Returns whether the given player is invited to this bloc.
  [[nodiscard]] bool is_invited(player_t p) const noexcept;

  /// Invites the given player to this bloc.
  void invite_player(player_t p) noexcept;

  /// Cancels the invitation for the given player to this bloc.
  void cancel_invite(player_t p) noexcept;

  /// Returns whether the given player is pledged to this bloc.
  [[nodiscard]] bool is_pledged(player_t p) const noexcept;

  /// Pledges the given player to this bloc.
  void pledge_player(player_t p) noexcept;

  /// Unpledges the given player from this bloc.
  void unpledge_player(player_t p) noexcept;

  /// Returns whether this bloc is allied with the given player.
  [[nodiscard]] bool is_allied_with(player_t p) const noexcept;

  /// Declares this bloc's alliance with the given player.
  void declare_alliance_with(player_t p) noexcept;

  /// Rescinds this bloc's alliance with the given player.
  void rescind_alliance_with(player_t p) noexcept;

  /// Returns whether this bloc is at war with the given player.
  [[nodiscard]] bool is_at_war_with(player_t p) const noexcept;

  /// Declares this bloc at war with the given player.
  void declare_war_on(player_t p) noexcept;

  /// Makes peace between this bloc and the given player.
  void make_peace_with(player_t p) noexcept;
};

export struct PowerBlockStats {
  std::uint32_t members{0};
  population_t troops{0};       /* total troops */
  population_t popn{0};         /* total population */
  resource_t resource{0};       /* total resource in stock */
  resource_t fuel{0};           /* total fuel in stock */
  resource_t destruct{0};       /* total dest in stock */
  std::uint32_t ships_owned{0}; /* # of ships owned */
  std::uint32_t systems_owned{0};
  std::uint32_t sectors_owned{0};
  money_t money{0};
  std::uint64_t VPs{0};
};

export struct power_blocks {
  std::time_t time{0};
  PlayerVector<PowerBlockStats, MAXPLAYERS> blocks{};
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