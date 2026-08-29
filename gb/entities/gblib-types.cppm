// SPDX-License-Identifier: Apache-2.0

/// \file gblib-types.cppm
/// \brief Module interface partition for foundational game types, vectors,
/// coordinates, and scopes.

export module gblib:types;

import std;

// Re-export basic types from standalone types module
export import types;

export using command_t = std::vector<std::string>;

export enum ScopeLevel {
  LEVEL_UNIV,
  LEVEL_STAR,
  LEVEL_PLAN,
  LEVEL_SHIP
};

export enum PlanetType {
  EARTH = 0,
  ASTEROID = 1,
  MARS = 2,
  ICEBALL = 3,
  GASGIANT = 4,
  WATER = 5,
  FOREST = 6,
  DESERT = 7,
};

export constexpr std::array all_planet_types = {
    PlanetType::EARTH,   PlanetType::ASTEROID, PlanetType::MARS,
    PlanetType::ICEBALL, PlanetType::GASGIANT, PlanetType::WATER,
    PlanetType::FOREST,  PlanetType::DESERT,
};

export template <>
struct std::formatter<PlanetType> : std::formatter<int> {
  auto format(PlanetType type, format_context& ctx) const {
    return formatter<int>::format(static_cast<int>(type), ctx);
  }
};

export enum class NewsType {
  ANNOUNCE,
  COMBAT,
  DECLARATION,
  TRANSFER,
};

export enum SectorType {
  SEC_SEA = 0,
  SEC_LAND = 1,
  SEC_MOUNT = 2,
  SEC_GAS = 3,
  SEC_ICE = 4,
  SEC_FOREST = 5,
  SEC_DESERT = 6,
  SEC_PLATED = 7,
  SEC_WASTED = 8,
};

export constexpr std::array all_sector_types = {
    SectorType::SEC_SEA,    SectorType::SEC_LAND,   SectorType::SEC_MOUNT,
    SectorType::SEC_GAS,    SectorType::SEC_ICE,    SectorType::SEC_FOREST,
    SectorType::SEC_DESERT, SectorType::SEC_PLATED, SectorType::SEC_WASTED,
};

export template <>
struct std::formatter<SectorType> : std::formatter<int> {
  auto format(SectorType type, format_context& ctx) const {
    return formatter<int>::format(static_cast<int>(type), ctx);
  }
};

export constexpr std::optional<SectorType> to_sector_type(int val) noexcept {
  if (val >= SectorType::SEC_SEA && val <= SectorType::SEC_WASTED) {
    return static_cast<SectorType>(val);
  }
  return std::nullopt;
}

export enum class PopulationType {
  CIV,
  MIL,
};

// These values are persisted to SQL
export enum class CommodType {
  RESOURCE = 0,
  DESTRUCT = 1,
  FUEL = 2,
  CRYSTAL = 3,
};
export template <>
struct std::formatter<CommodType> : std::formatter<std::string_view> {
  auto format(CommodType type, format_context& ctx) const {
    switch (type) {
      case CommodType::RESOURCE:
        return formatter<std::string_view>::format("resources", ctx);
      case CommodType::DESTRUCT:
        return formatter<std::string_view>::format("destruct", ctx);
      case CommodType::FUEL:
        return formatter<std::string_view>::format("fuel", ctx);
      case CommodType::CRYSTAL:
        return formatter<std::string_view>::format("crystals", ctx);
    }
  }
};

export enum Conditions {
  RTEMP = 0,   /* regular temp for planet */
  TEMP = 1,    /* temperature */
  METHANE = 2, /* %age of gases for terraforming */
  OXYGEN = 3,
  CO2 = 4,
  HYDROGEN = 5,
  NITROGEN = 6,
  SULFUR = 7,
  HELIUM = 8,
  OTHER = 9,
  TOXIC = 10,
};

export constexpr std::array all_condition_types = {
    Conditions::RTEMP,    Conditions::TEMP,   Conditions::METHANE,
    Conditions::OXYGEN,   Conditions::CO2,    Conditions::HYDROGEN,
    Conditions::NITROGEN, Conditions::SULFUR, Conditions::HELIUM,
    Conditions::OTHER,    Conditions::TOXIC,
};

export constexpr std::array all_atmosphere_conditions = {
    Conditions::RTEMP,    Conditions::TEMP,   Conditions::METHANE,
    Conditions::OXYGEN,   Conditions::CO2,    Conditions::HYDROGEN,
    Conditions::NITROGEN, Conditions::SULFUR, Conditions::HELIUM,
    Conditions::OTHER,
};

/// Temporary per-planet simulation state tracking across turn update passes.
export struct Stinfo {
  int temp_add{0};  ///< Thermal adjustment applied to planet temperature
  bool thing_add{
      false};  ///< Whether a new alien Thing colony spawned on this planet
  bool inhab{
      false};  ///< Whether any race inhabits or explored this planet this turn
  bool intimidated{
      false};  ///< Whether an assault platform is suppressing slave revolts
};

export struct Vnbrain {
  unsigned short total_mad{0}; /* total # of VN's destroyed so far */
  unsigned char most_mad{0};   /* player most mad at */
};

export struct ServerState {
  int id{1};                         // Always 1 - singleton entity
  unsigned long segments{1};         // Number of movement segments
  std::time_t next_update_time{0};   // Next update timestamp
  std::time_t next_segment_time{0};  // Next segment timestamp
  int update_time_minutes{10};       // Interval between updates in minutes
  segments_t nsegments_done{0};      // Segments completed this update
  std::string welcome_message;  // Welcome message shown to connecting players
};

export struct Commod {
  int id{0};  // Commodity ID for database persistence
  player_t owner{0};
  governor_t governor{0};
  CommodType type{CommodType::RESOURCE};
  std::uint64_t amount{0};
  bool deliver{false}; /* whether the lot is ready for shipping or not */
  money_t bid{0};
  player_t bidder{0};
  governor_t bidder_gov{0};
  starnum_t star_from{0}; /* where the stuff originated from */
  planetnum_t planet_from{0};
  starnum_t star_to{0}; /* where it goes to */
  planetnum_t planet_to{0};
};

export struct Victory {
  std::weak_ordering operator<=>(const Victory& that) const {
    // Ensure that folks who shouldn't count are always ranked last.
    if (no_count && !that.no_count) return std::weak_ordering::greater;
    if (that.no_count && !no_count) return std::weak_ordering::less;

    if (that.rawscore > rawscore) return std::weak_ordering::greater;
    if (that.rawscore < rawscore) return std::weak_ordering::less;

    // Must be equal
    return std::weak_ordering::equivalent;
  }
  player_t racenum{0};
  std::string name;
  bool no_count = false;
  double tech{0.0};
  int thing{0};
  int iq{0};
  unsigned long rawscore{0};
};

export struct Coordinates {
  int x{0};
  int y{0};

  constexpr Coordinates() = default;
  constexpr Coordinates(int x_val, int y_val) noexcept : x(x_val), y(y_val) {}

  constexpr Coordinates operator+(const Coordinates& other) const noexcept {
    return {x + other.x, y + other.y};
  }
  constexpr Coordinates operator-(const Coordinates& other) const noexcept {
    return {x - other.x, y - other.y};
  }
  constexpr Coordinates& operator+=(const Coordinates& other) noexcept {
    x += other.x;
    y += other.y;
    return *this;
  }
  constexpr Coordinates& operator-=(const Coordinates& other) noexcept {
    x -= other.x;
    y -= other.y;
    return *this;
  }

  constexpr auto operator<=>(const Coordinates&) const = default;
  constexpr bool operator==(const Coordinates&) const = default;

  /**
   * \brief Parse a string in the format "x,y" into a Coordinates object.
   * \param str Input string view
   * \return Coordinates if successfully parsed, empty optional otherwise.
   */
  static std::optional<Coordinates> parse(std::string_view str) {
    int x_val = 0;
    int y_val = 0;
    auto comma_pos = str.find(',');
    if (comma_pos == std::string_view::npos) {
      return std::nullopt;
    }

    auto x_str = str.substr(0, comma_pos);
    auto y_str = str.substr(comma_pos + 1);

    while (!x_str.empty() &&
           std::isspace(static_cast<unsigned char>(x_str.front()))) {
      x_str.remove_prefix(1);
    }
    while (!x_str.empty() &&
           std::isspace(static_cast<unsigned char>(x_str.back()))) {
      x_str.remove_suffix(1);
    }
    while (!y_str.empty() &&
           std::isspace(static_cast<unsigned char>(y_str.front()))) {
      y_str.remove_prefix(1);
    }
    while (!y_str.empty() &&
           std::isspace(static_cast<unsigned char>(y_str.back()))) {
      y_str.remove_suffix(1);
    }

    if (x_str.empty() || y_str.empty()) {
      return std::nullopt;
    }

    auto [p1, ec1] =
        std::from_chars(x_str.data(), x_str.data() + x_str.size(), x_val);
    if (ec1 != std::errc{} || p1 != x_str.data() + x_str.size()) {
      return std::nullopt;
    }

    auto [p2, ec2] =
        std::from_chars(y_str.data(), y_str.data() + y_str.size(), y_val);
    if (ec2 != std::errc{} || p2 != y_str.data() + y_str.size()) {
      return std::nullopt;
    }

    return Coordinates{x_val, y_val};
  }
};

export template <typename CharT>
struct std::formatter<Coordinates, CharT> {
  constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const Coordinates& c, FormatContext& ctx) const {
    auto out = ctx.out();
    out = std::format_to(out, "{}", c.x);
    *out++ = static_cast<CharT>(',');
    return std::format_to(out, "{}", c.y);
  }
};

/// \brief 1-indexed fixed-size player array wrapper indexed by player_t (1..N).
export template <typename T, std::size_t N>
class PlayerVector {
public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;
  using iterator = typename std::array<T, N>::iterator;
  using const_iterator = typename std::array<T, N>::const_iterator;

  constexpr PlayerVector() = default;

  [[nodiscard]] constexpr size_type size() const noexcept {
    return N;
  }
  [[nodiscard]] constexpr size_type max_size() const noexcept {
    return N;
  }
  [[nodiscard]] constexpr bool empty() const noexcept {
    return N == 0;
  }

  /// \brief 1-indexed access via player_t.
  [[nodiscard]] constexpr reference operator[](player_t player) {
    if (player.value < 1 || static_cast<std::size_t>(player.value) > N) {
      throw std::out_of_range(
          std::format("Player index {} out of range (1..{})", player.value, N));
    }
    return data_[player.value - 1];
  }

  /// \brief 1-indexed const access via player_t.
  [[nodiscard]] constexpr const_reference operator[](player_t player) const {
    if (player.value < 1 || static_cast<std::size_t>(player.value) > N) {
      throw std::out_of_range(
          std::format("Player index {} out of range (1..{})", player.value, N));
    }
    return data_[player.value - 1];
  }

  /// \brief Checked 1-indexed access.
  [[nodiscard]] constexpr reference at(player_t player) {
    return (*this)[player];
  }

  /// \brief Checked 1-indexed const access.
  [[nodiscard]] constexpr const_reference at(player_t player) const {
    return (*this)[player];
  }

  /// \brief Iterators over all player slots.
  [[nodiscard]] constexpr iterator begin() noexcept {
    return data_.begin();
  }
  [[nodiscard]] constexpr iterator end() noexcept {
    return data_.end();
  }
  [[nodiscard]] constexpr const_iterator begin() const noexcept {
    return data_.begin();
  }
  [[nodiscard]] constexpr const_iterator end() const noexcept {
    return data_.end();
  }
  [[nodiscard]] constexpr const_iterator cbegin() const noexcept {
    return data_.cbegin();
  }
  [[nodiscard]] constexpr const_iterator cend() const noexcept {
    return data_.cend();
  }

  [[nodiscard]] constexpr pointer data() noexcept {
    return data_.data();
  }
  [[nodiscard]] constexpr const_pointer data() const noexcept {
    return data_.data();
  }

  /// \brief Underlying std::array reference for raw array access /
  /// serialization.
  [[nodiscard]] constexpr std::array<T, N>& raw_array() noexcept {
    return data_;
  }
  [[nodiscard]] constexpr const std::array<T, N>& raw_array() const noexcept {
    return data_;
  }

  constexpr bool operator==(const PlayerVector& other) const = default;

private:
  std::array<T, N> data_{};
};

/**
 * \brief Convert input string to a shipnum_t
 * \param s User-provided input string
 * \return If the user provided a valid number, return it.
 */
export constexpr std::optional<shipnum_t>
string_to_shipnum(std::string_view s) {
  while (s.size() > 1 && s.front() == '#') {
    s.remove_prefix(1);
  }

  if (!s.empty() && std::isdigit(s.front())) {
    return std::stoi(std::string(s.begin(), s.end()));
  }
  return {};
}
