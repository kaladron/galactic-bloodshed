// SPDX-License-Identifier: Apache-2.0

/// \file domain_types.cppm
/// \brief Module interface partition for foundational game types, vectors,
/// coordinates, and scopes.

export module gb.entities:types;

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

export constexpr std::array habitable_planet_types = {
    PlanetType::EARTH,    PlanetType::MARS,  PlanetType::ICEBALL,
    PlanetType::GASGIANT, PlanetType::WATER, PlanetType::FOREST,
    PlanetType::DESERT,
};

/// Named values indexed by PlanetType.
export template <typename T>
struct PlanetValues {
  T earth{};
  T asteroid{};
  T mars{};
  T iceball{};
  T gasgiant{};
  T water{};
  T forest{};
  T desert{};

  [[nodiscard]] constexpr T& operator[](PlanetType type) {
    switch (type) {
      case PlanetType::EARTH:
        return earth;
      case PlanetType::ASTEROID:
        return asteroid;
      case PlanetType::MARS:
        return mars;
      case PlanetType::ICEBALL:
        return iceball;
      case PlanetType::GASGIANT:
        return gasgiant;
      case PlanetType::WATER:
        return water;
      case PlanetType::FOREST:
        return forest;
      case PlanetType::DESERT:
        return desert;
    }
    throw std::out_of_range("Invalid PlanetType");
  }

  [[nodiscard]] constexpr const T& operator[](PlanetType type) const {
    switch (type) {
      case PlanetType::EARTH:
        return earth;
      case PlanetType::ASTEROID:
        return asteroid;
      case PlanetType::MARS:
        return mars;
      case PlanetType::ICEBALL:
        return iceball;
      case PlanetType::GASGIANT:
        return gasgiant;
      case PlanetType::WATER:
        return water;
      case PlanetType::FOREST:
        return forest;
      case PlanetType::DESERT:
        return desert;
    }
    throw std::out_of_range("Invalid PlanetType");
  }

  template <typename U>
    requires(!std::same_as<U, PlanetType>)
  constexpr T& operator[](U) = delete;

  template <typename U>
    requires(!std::same_as<U, PlanetType>)
  constexpr const T& operator[](U) const = delete;

  constexpr bool operator==(const PlanetValues&) const noexcept = default;
};

/// Returns the display string for a PlanetType.
export constexpr std::string_view to_string(PlanetType type) noexcept {
  switch (type) {
    case PlanetType::EARTH:
      return "Class M";
    case PlanetType::ASTEROID:
      return "Asteroid";
    case PlanetType::MARS:
      return "Airless";
    case PlanetType::ICEBALL:
      return "Iceball";
    case PlanetType::GASGIANT:
      return "Jovian";
    case PlanetType::WATER:
      return "Waterball";
    case PlanetType::FOREST:
      return "Forest";
    case PlanetType::DESERT:
      return "Desert";
  }
  return "Unknown";
}

export template <>
struct std::formatter<PlanetType> {
  enum class Mode {
    String,
    Int
  };
  Mode mode{Mode::String};
  std::formatter<std::string_view> str_fmt;
  std::formatter<int> int_fmt;

  constexpr auto parse(std::format_parse_context& ctx) {
    auto it = ctx.begin();
    const auto end = ctx.end();
    while (it != end && *it != '}') {
      if (*it == 'd') {
        mode = Mode::Int;
        break;
      }
      ++it;
    }
    if (mode == Mode::Int) {
      return int_fmt.parse(ctx);
    }
    return str_fmt.parse(ctx);
  }

  auto format(PlanetType type, format_context& ctx) const {
    if (mode == Mode::Int) {
      return int_fmt.format(std::to_underlying(type), ctx);
    }
    return str_fmt.format(to_string(type), ctx);
  }
};

export enum class NewsType {
  ANNOUNCE,
  COMBAT,
  DECLARATION,
  TRANSFER,
};

/// Named values indexed by NewsType (e.g., last-read SQLite news item IDs).
export template <typename T = int>
struct NewsValues {
  T announce{};
  T combat{};
  T declaration{};
  T transfer{};

  [[nodiscard]] constexpr T& operator[](NewsType type) {
    switch (type) {
      case NewsType::ANNOUNCE:
        return announce;
      case NewsType::COMBAT:
        return combat;
      case NewsType::DECLARATION:
        return declaration;
      case NewsType::TRANSFER:
        return transfer;
    }
    throw std::out_of_range("Invalid NewsType");
  }

  [[nodiscard]] constexpr const T& operator[](NewsType type) const {
    switch (type) {
      case NewsType::ANNOUNCE:
        return announce;
      case NewsType::COMBAT:
        return combat;
      case NewsType::DECLARATION:
        return declaration;
      case NewsType::TRANSFER:
        return transfer;
    }
    throw std::out_of_range("Invalid NewsType");
  }

  template <typename U>
    requires(!std::same_as<U, NewsType>)
  constexpr T& operator[](U) = delete;

  template <typename U>
    requires(!std::same_as<U, NewsType>)
  constexpr const T& operator[](U) const = delete;

  constexpr bool operator==(const NewsValues&) const noexcept = default;
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

export constexpr std::array settleable_sector_types = {
    SectorType::SEC_SEA,    SectorType::SEC_LAND,   SectorType::SEC_MOUNT,
    SectorType::SEC_GAS,    SectorType::SEC_ICE,    SectorType::SEC_FOREST,
    SectorType::SEC_DESERT, SectorType::SEC_PLATED,
};

/// Named values indexed by SectorType.
export template <typename T = double, T DefaultPlated = T{}>
struct SectorValues {
  T sea{};
  T land{};
  T mount{};
  T gas{};
  T ice{};
  T forest{};
  T desert{};
  T plated{DefaultPlated};
  T wasted{};

  [[nodiscard]] constexpr T& operator[](SectorType type) {
    switch (type) {
      case SectorType::SEC_SEA:
        return sea;
      case SectorType::SEC_LAND:
        return land;
      case SectorType::SEC_MOUNT:
        return mount;
      case SectorType::SEC_GAS:
        return gas;
      case SectorType::SEC_ICE:
        return ice;
      case SectorType::SEC_FOREST:
        return forest;
      case SectorType::SEC_DESERT:
        return desert;
      case SectorType::SEC_PLATED:
        return plated;
      case SectorType::SEC_WASTED:
        return wasted;
    }
    throw std::out_of_range("Invalid SectorType");
  }

  [[nodiscard]] constexpr const T& operator[](SectorType type) const {
    switch (type) {
      case SectorType::SEC_SEA:
        return sea;
      case SectorType::SEC_LAND:
        return land;
      case SectorType::SEC_MOUNT:
        return mount;
      case SectorType::SEC_GAS:
        return gas;
      case SectorType::SEC_ICE:
        return ice;
      case SectorType::SEC_FOREST:
        return forest;
      case SectorType::SEC_DESERT:
        return desert;
      case SectorType::SEC_PLATED:
        return plated;
      case SectorType::SEC_WASTED:
        return wasted;
    }
    throw std::out_of_range("Invalid SectorType");
  }

  template <typename U>
    requires(!std::same_as<U, SectorType>)
  constexpr T& operator[](U) = delete;

  template <typename U>
    requires(!std::same_as<U, SectorType>)
  constexpr const T& operator[](U) const = delete;

  [[nodiscard]] constexpr std::array<std::pair<SectorType, T>, 8>
  settleable() const noexcept {
    return {{
        {SectorType::SEC_SEA, sea},
        {SectorType::SEC_LAND, land},
        {SectorType::SEC_MOUNT, mount},
        {SectorType::SEC_GAS, gas},
        {SectorType::SEC_ICE, ice},
        {SectorType::SEC_FOREST, forest},
        {SectorType::SEC_DESERT, desert},
        {SectorType::SEC_PLATED, plated},
    }};
  }

  constexpr bool operator==(const SectorValues&) const noexcept = default;
};

/// Named sector compatibility ratings (0.0 to 1.0) for each terrain type.
export using SectorCompatibilities = SectorValues<double, 1.0>;

export constexpr std::optional<SectorType> to_sector_type(int val) noexcept {
  if (val >= SectorType::SEC_SEA && val <= SectorType::SEC_WASTED) {
    return static_cast<SectorType>(val);
  }
  return std::nullopt;
}

/// Returns the display string for a SectorType.
export constexpr std::string_view to_string(SectorType type) noexcept {
  switch (type) {
    case SectorType::SEC_SEA:
      return "ocean";
    case SectorType::SEC_LAND:
      return "land";
    case SectorType::SEC_MOUNT:
      return "mountainous";
    case SectorType::SEC_GAS:
      return "gaseous";
    case SectorType::SEC_ICE:
      return "ice";
    case SectorType::SEC_FOREST:
      return "forest";
    case SectorType::SEC_DESERT:
      return "desert";
    case SectorType::SEC_PLATED:
      return "plated";
    case SectorType::SEC_WASTED:
      return "wasted";
  }
  return "unknown";
}

export template <>
struct std::formatter<SectorType> {
  enum class Mode {
    String,
    Int
  };
  Mode mode{Mode::String};
  std::formatter<std::string_view> str_fmt;
  std::formatter<int> int_fmt;

  constexpr auto parse(std::format_parse_context& ctx) {
    auto it = ctx.begin();
    const auto end = ctx.end();
    while (it != end && *it != '}') {
      if (*it == 'd') {
        mode = Mode::Int;
        break;
      }
      ++it;
    }
    if (mode == Mode::Int) {
      return int_fmt.parse(ctx);
    }
    return str_fmt.parse(ctx);
  }

  auto format(SectorType type, format_context& ctx) const {
    if (mode == Mode::Int) {
      return int_fmt.format(std::to_underlying(type), ctx);
    }
    return str_fmt.format(to_string(type), ctx);
  }
};

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

export enum AtmosphereConditions {
  METHANE = 0, /* %age of gases for terraforming */
  OXYGEN = 1,
  CO2 = 2,
  HYDROGEN = 3,
  NITROGEN = 4,
  SULFUR = 5,
  HELIUM = 6,
  OTHER = 7,
};

export constexpr std::array all_atmosphere_conditions = {
    AtmosphereConditions::METHANE,  AtmosphereConditions::OXYGEN,
    AtmosphereConditions::CO2,      AtmosphereConditions::HYDROGEN,
    AtmosphereConditions::NITROGEN, AtmosphereConditions::SULFUR,
    AtmosphereConditions::HELIUM,   AtmosphereConditions::OTHER,
};

// Diagnostic logging for invariant violations
export constexpr bool DEBUG_INVARIANTS = true;

export template <typename T, typename U>
void log_invariant_violation(
    std::string_view entity, std::string_view field, T attempted, U clamped_to,
    std::source_location loc = std::source_location::current()) {
  if constexpr (DEBUG_INVARIANTS) {
    std::print(std::cerr,
               "[INVARIANT] {}::{}: attempted {}, clamped to {} (at {}:{})\n",
               entity, field, attempted, clamped_to, loc.file_name(),
               loc.line());
  }
}

/// \brief Strongly-typed integer percentage in `[0, 100]`.
///
/// Logs an invariant violation via `log_invariant_violation` and clamps to
/// `[0, 100]` if constructed or assigned with a value outside `[0, 100]`.
/// Meaningless binary arithmetic between two `Percentage` instances
/// (`Percentage + Percentage`, `Percentage * Percentage`, etc.) is deleted,
/// while conversions and comparisons with primitive integers/floats preserve
/// exact game formulas.
export class Percentage {
public:
  constexpr Percentage() noexcept = default;

  template <std::integral T>
  constexpr Percentage(T v, std::source_location loc =
                                std::source_location::current()) noexcept {
    if constexpr (std::signed_integral<T>) {
      const int clamped = static_cast<int>(std::clamp<T>(v, 0, 100));
      if !consteval {
        if (v < 0 || v > 100) {
          log_invariant_violation("Percentage", "value", v, clamped, loc);
        }
      }
      value_ = clamped;
    } else {
      const int clamped = static_cast<int>(std::min<T>(v, 100));
      if !consteval {
        if (v > 100) {
          log_invariant_violation("Percentage", "value", v, clamped, loc);
        }
      }
      value_ = clamped;
    }
  }

  template <std::floating_point T>
  explicit constexpr Percentage(
      T v,
      std::source_location loc = std::source_location::current()) noexcept {
    const int rounded = static_cast<int>(std::round(v));
    const int clamped = std::clamp(rounded, 0, 100);
    if !consteval {
      if (rounded < 0 || rounded > 100) {
        log_invariant_violation("Percentage", "value", v, clamped, loc);
      }
    }
    value_ = clamped;
  }

  [[nodiscard]] constexpr int value() const noexcept {
    return value_;
  }

  [[nodiscard]] constexpr operator int() const noexcept {
    return value_;
  }

  /// \brief Returns the percentage as a unit fraction in `[0.0, 1.0]`.
  [[nodiscard]] constexpr double as_fraction() const noexcept {
    return static_cast<double>(value_) / 100.0;
  }

  /// \brief Returns the complementary percentage `100 - value()`.
  [[nodiscard]] constexpr Percentage complement() const noexcept {
    return Percentage{100 - value_};
  }

  /// \brief Adjusts the percentage by a signed delta, saturating smoothly at
  /// `[0, 100]` without logging an invariant violation.
  constexpr Percentage& adjust(int delta) noexcept {
    value_ = std::clamp(value_ + delta, 0, 100);
    return *this;
  }

  constexpr Percentage& operator+=(int delta) noexcept {
    return adjust(delta);
  }

  constexpr Percentage& operator-=(int delta) noexcept {
    return adjust(-delta);
  }

  constexpr Percentage& operator++() noexcept {
    return adjust(1);
  }
  constexpr Percentage operator++(int) noexcept {
    Percentage tmp = *this;
    adjust(1);
    return tmp;
  }
  constexpr Percentage& operator--() noexcept {
    return adjust(-1);
  }
  constexpr Percentage operator--(int) noexcept {
    Percentage tmp = *this;
    adjust(-1);
    return tmp;
  }

  template <std::same_as<Percentage> P1, std::same_as<Percentage> P2>
  friend constexpr void operator+(P1, P2) = delete;
  template <std::same_as<Percentage> P1, std::same_as<Percentage> P2>
  friend constexpr void operator-(P1, P2) = delete;
  template <std::same_as<Percentage> P1, std::same_as<Percentage> P2>
  friend constexpr void operator*(P1, P2) = delete;
  template <std::same_as<Percentage> P1, std::same_as<Percentage> P2>
  friend constexpr void operator/(P1, P2) = delete;

  [[nodiscard]] friend constexpr bool operator==(Percentage lhs,
                                                 Percentage rhs) noexcept {
    return lhs.value_ == rhs.value_;
  }
  [[nodiscard]] friend constexpr auto operator<=>(Percentage lhs,
                                                  Percentage rhs) noexcept {
    return lhs.value_ <=> rhs.value_;
  }

  template <typename T>
    requires(std::integral<T> || std::floating_point<T>)
  [[nodiscard]] friend constexpr bool operator==(Percentage lhs,
                                                 T rhs) noexcept {
    return static_cast<T>(lhs.value_) == rhs;
  }
  template <typename T>
    requires(std::integral<T> || std::floating_point<T>)
  [[nodiscard]] friend constexpr auto operator<=>(Percentage lhs,
                                                  T rhs) noexcept {
    return static_cast<T>(lhs.value_) <=> rhs;
  }

  friend std::ostream& operator<<(std::ostream& os, Percentage p) {
    return os << p.value_;
  }

private:
  int value_{0};
};

export using percent_t = Percentage;
export using fertilize_t = Percentage;

export template <>
struct std::formatter<Percentage, char> : std::formatter<int, char> {
  auto format(Percentage p, auto& ctx) const {
    return std::formatter<int, char>::format(p.value(), ctx);
  }
};

/// \brief Planetary and racial temperature in degrees Celsius (`>= -273` °C).
///
/// Models temperature as a 1D affine space over `temp_delta_t` (`int`):
/// - Direct construction or assignment clamps at absolute zero (`-273` °C) and
///   logs an invariant violation if a sub-absolute-zero value is supplied.
/// - Relative thermal adjustments (`adjust(delta)`, `+=`, `-=`) saturate
///   smoothly at `-273` °C without logging an invariant violation (e.g., during
///   nuclear winter bombardment).
/// - Subtracting two `Temperature` values yields a `temp_delta_t` (`int`),
///   while adding or multiplying two `Temperature` values is deleted.
export class Temperature {
public:
  static constexpr int ABSOLUTE_ZERO_CELSIUS = -273;

  constexpr Temperature() noexcept = default;

  template <std::signed_integral T>
  constexpr Temperature(T v, std::source_location loc =
                                 std::source_location::current()) noexcept {
    const int clamped = static_cast<int>(std::max<T>(v, ABSOLUTE_ZERO_CELSIUS));
    if !consteval {
      if (v < ABSOLUTE_ZERO_CELSIUS) {
        log_invariant_violation("Temperature", "celsius", v, clamped, loc);
      }
    }
    value_ = clamped;
  }

  template <std::floating_point T>
  explicit constexpr Temperature(
      T v,
      std::source_location loc = std::source_location::current()) noexcept {
    const int rounded = static_cast<int>(std::round(v));
    const int clamped = std::max(rounded, ABSOLUTE_ZERO_CELSIUS);
    if !consteval {
      if (rounded < ABSOLUTE_ZERO_CELSIUS) {
        log_invariant_violation("Temperature", "celsius", v, clamped, loc);
      }
    }
    value_ = clamped;
  }

  [[nodiscard]] constexpr int value() const noexcept {
    return value_;
  }

  [[nodiscard]] constexpr operator int() const noexcept {
    return value_;
  }

  /// \brief Adjusts the temperature by a signed thermal delta, saturating
  /// smoothly at absolute zero (`-273` °C) without logging an invariant
  /// violation.
  constexpr Temperature& adjust(temp_delta_t delta) noexcept {
    value_ = std::max(value_ + delta, ABSOLUTE_ZERO_CELSIUS);
    return *this;
  }

  constexpr Temperature& operator+=(temp_delta_t delta) noexcept {
    return adjust(delta);
  }

  constexpr Temperature& operator-=(temp_delta_t delta) noexcept {
    return adjust(-delta);
  }

  [[nodiscard]] friend constexpr Temperature
  operator+(Temperature lhs, temp_delta_t delta) noexcept {
    lhs += delta;
    return lhs;
  }

  [[nodiscard]] friend constexpr Temperature
  operator-(Temperature lhs, temp_delta_t delta) noexcept {
    lhs -= delta;
    return lhs;
  }

  [[nodiscard]] friend constexpr temp_delta_t
  operator-(Temperature lhs, Temperature rhs) noexcept {
    return lhs.value_ - rhs.value_;
  }

  template <std::same_as<Temperature> T1, std::same_as<Temperature> T2>
  friend constexpr void operator+(T1, T2) = delete;
  template <std::same_as<Temperature> T1, std::same_as<Temperature> T2>
  friend constexpr void operator*(T1, T2) = delete;
  template <std::same_as<Temperature> T1, std::same_as<Temperature> T2>
  friend constexpr void operator/(T1, T2) = delete;

  [[nodiscard]] friend constexpr bool operator==(Temperature lhs,
                                                 Temperature rhs) noexcept {
    return lhs.value_ == rhs.value_;
  }
  [[nodiscard]] friend constexpr auto operator<=>(Temperature lhs,
                                                  Temperature rhs) noexcept {
    return lhs.value_ <=> rhs.value_;
  }

  template <typename T>
    requires(std::integral<T> || std::floating_point<T>)
  [[nodiscard]] friend constexpr bool operator==(Temperature lhs,
                                                 T rhs) noexcept {
    return static_cast<T>(lhs.value_) == rhs;
  }
  template <typename T>
    requires(std::integral<T> || std::floating_point<T>)
  [[nodiscard]] friend constexpr auto operator<=>(Temperature lhs,
                                                  T rhs) noexcept {
    return static_cast<T>(lhs.value_) <=> rhs;
  }

  friend std::ostream& operator<<(std::ostream& os, Temperature t) {
    return os << t.value_;
  }

private:
  int value_{0};
};

export using temperature_t = Temperature;

export template <>
struct std::formatter<Temperature, char> : std::formatter<int, char> {
  auto format(Temperature t, std::format_context& ctx) const {
    return std::formatter<int, char>::format(t.value(), ctx);
  }
};

/// Planetary and racial atmospheric gas composition (`[0, 100]%` percentages).
export struct ConditionValues {
  Percentage methane{0};
  Percentage oxygen{0};
  Percentage co2{0};
  Percentage hydrogen{0};
  Percentage nitrogen{0};
  Percentage sulfur{0};
  Percentage helium{0};
  Percentage other{0};

  [[nodiscard]] constexpr Percentage&
  operator[](AtmosphereConditions cond) noexcept {
    switch (cond) {
      case AtmosphereConditions::METHANE:
        return methane;
      case AtmosphereConditions::OXYGEN:
        return oxygen;
      case AtmosphereConditions::CO2:
        return co2;
      case AtmosphereConditions::HYDROGEN:
        return hydrogen;
      case AtmosphereConditions::NITROGEN:
        return nitrogen;
      case AtmosphereConditions::SULFUR:
        return sulfur;
      case AtmosphereConditions::HELIUM:
        return helium;
      case AtmosphereConditions::OTHER:
        return other;
    }
    std::unreachable();
  }

  [[nodiscard]] constexpr const Percentage&
  operator[](AtmosphereConditions cond) const noexcept {
    switch (cond) {
      case AtmosphereConditions::METHANE:
        return methane;
      case AtmosphereConditions::OXYGEN:
        return oxygen;
      case AtmosphereConditions::CO2:
        return co2;
      case AtmosphereConditions::HYDROGEN:
        return hydrogen;
      case AtmosphereConditions::NITROGEN:
        return nitrogen;
      case AtmosphereConditions::SULFUR:
        return sulfur;
      case AtmosphereConditions::HELIUM:
        return helium;
      case AtmosphereConditions::OTHER:
        return other;
    }
    std::unreachable();
  }

  template <typename U>
    requires(!std::same_as<U, AtmosphereConditions>)
  constexpr Percentage& operator[](U) = delete;

  template <typename U>
    requires(!std::same_as<U, AtmosphereConditions>)
  constexpr const Percentage& operator[](U) const = delete;

  constexpr bool operator==(const ConditionValues&) const noexcept = default;
};

export constexpr std::string_view
to_string(AtmosphereConditions cond) noexcept {
  switch (cond) {
    case AtmosphereConditions::METHANE:
      return "methane";
    case AtmosphereConditions::OXYGEN:
      return "oxygen";
    case AtmosphereConditions::CO2:
      return "co2";
    case AtmosphereConditions::HYDROGEN:
      return "hydrogen";
    case AtmosphereConditions::NITROGEN:
      return "nitrogen";
    case AtmosphereConditions::SULFUR:
      return "sulfur";
    case AtmosphereConditions::HELIUM:
      return "helium";
    case AtmosphereConditions::OTHER:
      return "other";
  }
  std::unreachable();
}

export constexpr std::optional<AtmosphereConditions>
parse_condition(std::string_view name) noexcept {
  for (AtmosphereConditions cond : all_atmosphere_conditions) {
    if (to_string(cond) == name) {
      return cond;
    }
  }
  return std::nullopt;
}

export template <>
struct std::formatter<AtmosphereConditions> : std::formatter<std::string_view> {
  auto format(AtmosphereConditions cond, format_context& ctx) const {
    return formatter<std::string_view>::format(to_string(cond), ctx);
  }
};

export struct Vnbrain {
  std::uint32_t total_mad{0}; /* total # of VN's destroyed so far */
  std::optional<player_t> most_mad{std::nullopt}; /* player most mad at */
};

/// \brief Formats a UNIX epoch timestamp in standard 24-character
/// "Day Mon DD HH:MM:SS YYYY" format using C++26 std::chrono formatting
/// (without a trailing newline, thread-safe, no static C buffers).
export [[nodiscard]] inline std::string format_timestamp(std::time_t t) {
  const auto tp = std::chrono::floor<std::chrono::seconds>(
      std::chrono::system_clock::from_time_t(t));
  return std::format("{:%a %b %e %H:%M:%S %Y}", tp);
}

export struct ServerState {
  int id{1};                         // Always 1 - singleton entity
  segments_t segments{1};            // Number of movement segments
  std::time_t next_update_time{0};   // Next update timestamp
  std::time_t next_segment_time{0};  // Next segment timestamp
  int update_time_minutes{10};       // Interval between updates in minutes
  segments_t nsegments_done{0};      // Segments completed this update
  turn_t nupdates_done{0};           // Total updates completed
  std::time_t server_start_time{0};  // Timestamp when server started
  std::time_t last_update_time{0};   // Timestamp of most recent update
  std::time_t last_segment_time{0};  // Timestamp of most recent segment
  std::string welcome_message;  // Welcome message shown to connecting players

  void record_server_start(std::time_t clk) noexcept {
    server_start_time = clk;
  }

  void record_update_completed(std::time_t clk,
                               bool increment_updates) noexcept {
    if (increment_updates) {
      ++nupdates_done;
    }
    last_update_time = clk;
    record_segment_completed(clk);
  }

  void record_segment_completed(std::time_t clk) noexcept {
    last_segment_time = clk;
  }

  [[nodiscard]] std::string start_line() const {
    if (server_start_time == 0) return {};
    return std::format("Server started  : {}\n",
                       format_timestamp(server_start_time));
  }

  [[nodiscard]] std::string update_line() const {
    if (last_update_time == 0) return {};
    return std::format("Last Update {:3d} : {}\n", nupdates_done,
                       format_timestamp(last_update_time));
  }

  [[nodiscard]] std::string segment_line() const {
    if (last_segment_time == 0) return {};
    return std::format("Last Segment {:2d} : {}\n", nsegments_done,
                       format_timestamp(last_segment_time));
  }
};

export struct Commod {
  int id{0};  // Commodity ID for database persistence
  player_t owner{0};
  governor_t governor{0};
  CommodType type{CommodType::RESOURCE};
  std::uint64_t amount{0};
  bool deliver{false}; /* whether the lot is ready for shipping or not */
  money_t bid{0};
  std::optional<player_t> bidder{std::nullopt};
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
  iq_t iq{0};
  victory_score_t rawscore{0};
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

  /// \brief Returns true if this coordinate lies within the inclusive bounding
  /// box [low, high].
  [[nodiscard]] constexpr bool within_bounds(Coordinates low,
                                             Coordinates high) const noexcept {
    return x >= low.x && x <= high.x && y >= low.y && y <= high.y;
  }

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

/// \brief Continuous 2D position within a star system relative to the host star
/// (+/- SYSTEMSIZE = 2,000).
export struct SystemCoordinates {
  double x{0.0};
  double y{0.0};

  constexpr SystemCoordinates() = default;
  constexpr SystemCoordinates(double x_val, double y_val) noexcept
      : x(x_val), y(y_val) {}

  constexpr SystemCoordinates
  operator+(SystemCoordinates other) const noexcept {
    return {x + other.x, y + other.y};
  }
  constexpr SystemCoordinates
  operator-(SystemCoordinates other) const noexcept {
    return {x - other.x, y - other.y};
  }
  constexpr SystemCoordinates& operator+=(SystemCoordinates other) noexcept {
    x += other.x;
    y += other.y;
    return *this;
  }
  constexpr SystemCoordinates& operator-=(SystemCoordinates other) noexcept {
    x -= other.x;
    y -= other.y;
    return *this;
  }

  constexpr SystemCoordinates operator-() const noexcept {
    return {-x, -y};
  }

  constexpr SystemCoordinates operator*(double scalar) const noexcept {
    return {x * scalar, y * scalar};
  }
  friend constexpr SystemCoordinates operator*(double scalar,
                                               SystemCoordinates c) noexcept {
    return {c.x * scalar, c.y * scalar};
  }
  constexpr SystemCoordinates operator/(double scalar) const noexcept {
    return {x / scalar, y / scalar};
  }
  constexpr SystemCoordinates& operator*=(double scalar) noexcept {
    x *= scalar;
    y *= scalar;
    return *this;
  }
  constexpr SystemCoordinates& operator/=(double scalar) noexcept {
    x /= scalar;
    y /= scalar;
    return *this;
  }

  [[nodiscard]] double distance_to(SystemCoordinates other) const noexcept {
    return std::hypot(x - other.x, y - other.y);
  }
  [[nodiscard]] double bearing_to(SystemCoordinates other) const noexcept {
    return std::atan2(other.y - y, other.x - x);
  }

  constexpr auto operator<=>(const SystemCoordinates&) const = default;
  constexpr bool operator==(const SystemCoordinates&) const = default;
};

export template <typename CharT>
struct std::formatter<SystemCoordinates, CharT> {
  constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const SystemCoordinates& c, FormatContext& ctx) const {
    auto out = ctx.out();
    out = std::format_to(out, "{}", c.x);
    *out++ = static_cast<CharT>(',');
    return std::format_to(out, "{}", c.y);
  }
};

/// \brief Continuous 2D position in the universe (+/- UNIVSIZE = 150,000).
export struct UniverseCoordinates {
  double x{0.0};
  double y{0.0};

  constexpr UniverseCoordinates() = default;
  constexpr UniverseCoordinates(double x_val, double y_val) noexcept
      : x(x_val), y(y_val) {}

  [[nodiscard]] double distance_to(UniverseCoordinates other) const noexcept {
    return std::hypot(x - other.x, y - other.y);
  }
  [[nodiscard]] double bearing_to(UniverseCoordinates other) const noexcept {
    return std::atan2(other.y - y, other.x - x);
  }

  // Cross-frame operations with SystemCoordinates
  constexpr UniverseCoordinates
  operator+(SystemCoordinates offset) const noexcept {
    return {x + offset.x, y + offset.y};
  }
  friend constexpr UniverseCoordinates
  operator+(SystemCoordinates offset, UniverseCoordinates base) noexcept {
    return base + offset;
  }
  constexpr UniverseCoordinates
  operator-(SystemCoordinates offset) const noexcept {
    return {x - offset.x, y - offset.y};
  }
  constexpr UniverseCoordinates& operator+=(SystemCoordinates offset) noexcept {
    x += offset.x;
    y += offset.y;
    return *this;
  }
  constexpr UniverseCoordinates& operator-=(SystemCoordinates offset) noexcept {
    x -= offset.x;
    y -= offset.y;
    return *this;
  }

  // Difference between two UniverseCoordinates is a displacement vector
  // (SystemCoordinates)
  constexpr SystemCoordinates
  operator-(UniverseCoordinates other) const noexcept {
    return {x - other.x, y - other.y};
  }

  constexpr auto operator<=>(const UniverseCoordinates&) const = default;
  constexpr bool operator==(const UniverseCoordinates&) const = default;
};

export template <typename CharT>
struct std::formatter<UniverseCoordinates, CharT> {
  constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const UniverseCoordinates& c, FormatContext& ctx) const {
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

  /// \brief Assigns the given value to all elements.
  constexpr void fill(const T& value) {
    data_.fill(value);
  }

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

  /// \brief 1-indexed access via race-like object with .Playernum.
  template <typename RaceLike>
    requires requires(const RaceLike& r) {
      { r.Playernum } -> std::convertible_to<player_t>;
    }
  [[nodiscard]] constexpr reference operator[](const RaceLike& race) {
    return (*this)[race.Playernum];
  }

  /// \brief 1-indexed const access via race-like object with .Playernum.
  template <typename RaceLike>
    requires requires(const RaceLike& r) {
      { r.Playernum } -> std::convertible_to<player_t>;
    }
  [[nodiscard]] constexpr const_reference
  operator[](const RaceLike& race) const {
    return (*this)[race.Playernum];
  }

  /// \brief Checked 1-indexed access via race-like object with .Playernum.
  template <typename RaceLike>
    requires requires(const RaceLike& r) {
      { r.Playernum } -> std::convertible_to<player_t>;
    }
  [[nodiscard]] constexpr reference at(const RaceLike& race) {
    return (*this)[race.Playernum];
  }

  /// \brief Checked 1-indexed const access via race-like object with
  /// .Playernum.
  template <typename RaceLike>
    requires requires(const RaceLike& r) {
      { r.Playernum } -> std::convertible_to<player_t>;
    }
  [[nodiscard]] constexpr const_reference at(const RaceLike& race) const {
    return (*this)[race.Playernum];
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

/// \brief 1-indexed fixed-size player bitset wrapper indexed by player_t
/// (1..N).
export template <std::size_t N = 64>
class PlayerBitset {
public:
  using size_type = std::size_t;
  using reference = typename std::bitset<N>::reference;

  constexpr PlayerBitset() noexcept = default;

  /// \brief Construct from an unsigned integer bitmask where bit 0 is player 1.
  explicit constexpr PlayerBitset(unsigned long long val) noexcept
      : bits_(val) {}

  /// \brief Returns a bitset with only the specified player set.
  [[nodiscard]] static constexpr PlayerBitset singleton(player_t player) {
    PlayerBitset b;
    b.set(player);
    return b;
  }

  /// \brief 1-indexed checked test via player_t.
  [[nodiscard]] constexpr bool test(player_t player) const {
    check_bounds(player);
    return bits_.test(player.value - 1);
  }

  /// \brief 1-indexed checked set via player_t.
  constexpr PlayerBitset& set(player_t player, bool value = true) {
    check_bounds(player);
    bits_.set(player.value - 1, value);
    return *this;
  }

  /// \brief Sets all bits to true.
  constexpr PlayerBitset& set() noexcept {
    bits_.set();
    return *this;
  }

  /// \brief 1-indexed checked reset via player_t.
  constexpr PlayerBitset& reset(player_t player) {
    check_bounds(player);
    bits_.reset(player.value - 1);
    return *this;
  }

  /// \brief Resets all bits to false.
  constexpr PlayerBitset& reset() noexcept {
    bits_.reset();
    return *this;
  }

  /// \brief 1-indexed checked flip via player_t.
  constexpr PlayerBitset& flip(player_t player) {
    check_bounds(player);
    bits_.flip(player.value - 1);
    return *this;
  }

  /// \brief Flips all bits.
  constexpr PlayerBitset& flip() noexcept {
    bits_.flip();
    return *this;
  }

  /// \brief 1-indexed subscript access via player_t.
  [[nodiscard]] constexpr reference operator[](player_t player) {
    check_bounds(player);
    return bits_[player.value - 1];
  }

  /// \brief 1-indexed const subscript access via player_t.
  [[nodiscard]] constexpr bool operator[](player_t player) const {
    check_bounds(player);
    return bits_[player.value - 1];
  }

  /// \brief Access via race-like object with .Playernum.
  template <typename RaceLike>
    requires requires(const RaceLike& r) {
      { r.Playernum } -> std::convertible_to<player_t>;
    }
  [[nodiscard]] constexpr reference operator[](const RaceLike& race) {
    return (*this)[race.Playernum];
  }

  template <typename RaceLike>
    requires requires(const RaceLike& r) {
      { r.Playernum } -> std::convertible_to<player_t>;
    }
  [[nodiscard]] constexpr bool operator[](const RaceLike& race) const {
    return (*this)[race.Playernum];
  }

  template <typename RaceLike>
    requires requires(const RaceLike& r) {
      { r.Playernum } -> std::convertible_to<player_t>;
    }
  [[nodiscard]] constexpr bool test(const RaceLike& race) const {
    return test(race.Playernum);
  }

  template <typename RaceLike>
    requires requires(const RaceLike& r) {
      { r.Playernum } -> std::convertible_to<player_t>;
    }
  constexpr PlayerBitset& set(const RaceLike& race, bool value = true) {
    return set(race.Playernum, value);
  }

  template <typename RaceLike>
    requires requires(const RaceLike& r) {
      { r.Playernum } -> std::convertible_to<player_t>;
    }
  constexpr PlayerBitset& reset(const RaceLike& race) {
    return reset(race.Playernum);
  }

  template <typename RaceLike>
    requires requires(const RaceLike& r) {
      { r.Playernum } -> std::convertible_to<player_t>;
    }
  constexpr PlayerBitset& flip(const RaceLike& race) {
    return flip(race.Playernum);
  }

  [[nodiscard]] constexpr bool all() const noexcept {
    return bits_.all();
  }
  [[nodiscard]] constexpr bool any() const noexcept {
    return bits_.any();
  }
  [[nodiscard]] constexpr bool none() const noexcept {
    return bits_.none();
  }
  [[nodiscard]] constexpr size_type count() const noexcept {
    return bits_.count();
  }
  [[nodiscard]] constexpr size_type size() const noexcept {
    return N;
  }
  [[nodiscard]] constexpr size_type max_size() const noexcept {
    return N;
  }
  [[nodiscard]] constexpr bool empty() const noexcept {
    return N == 0;
  }

  [[nodiscard]] unsigned long to_ulong() const {
    return bits_.to_ulong();
  }

  [[nodiscard]] unsigned long long to_ullong() const {
    return bits_.to_ullong();
  }

  [[nodiscard]] std::string to_string(char zero = '0', char one = '1') const {
    return bits_.to_string(zero, one);
  }

  /// \brief Range view over all player IDs set in this bitset.
  [[nodiscard]] auto players() const {
    return std::views::iota(std::size_t{1}, N + 1) |
           std::views::filter(
               [this](std::size_t i) { return bits_.test(i - 1); }) |
           std::views::transform([](std::size_t i) {
             return player_t{static_cast<player_t::value_type>(i)};
           });
  }

  /// \brief Underlying std::bitset reference for direct bitwise operations /
  /// serialization.
  [[nodiscard]] constexpr std::bitset<N>& raw_bitset() noexcept {
    return bits_;
  }
  [[nodiscard]] constexpr const std::bitset<N>& raw_bitset() const noexcept {
    return bits_;
  }

  // Bitwise operators
  [[nodiscard]] constexpr PlayerBitset operator~() const noexcept {
    PlayerBitset result;
    result.bits_ = ~bits_;
    return result;
  }

  constexpr PlayerBitset& operator&=(const PlayerBitset& other) noexcept {
    bits_ &= other.bits_;
    return *this;
  }

  constexpr PlayerBitset& operator|=(const PlayerBitset& other) noexcept {
    bits_ |= other.bits_;
    return *this;
  }

  constexpr PlayerBitset& operator^=(const PlayerBitset& other) noexcept {
    bits_ ^= other.bits_;
    return *this;
  }

  [[nodiscard]] friend constexpr PlayerBitset
  operator&(const PlayerBitset& lhs, const PlayerBitset& rhs) noexcept {
    PlayerBitset result = lhs;
    result &= rhs;
    return result;
  }

  [[nodiscard]] friend constexpr PlayerBitset
  operator|(const PlayerBitset& lhs, const PlayerBitset& rhs) noexcept {
    PlayerBitset result = lhs;
    result |= rhs;
    return result;
  }

  [[nodiscard]] friend constexpr PlayerBitset
  operator^(const PlayerBitset& lhs, const PlayerBitset& rhs) noexcept {
    PlayerBitset result = lhs;
    result ^= rhs;
    return result;
  }

  constexpr bool operator==(const PlayerBitset& other) const noexcept = default;

private:
  constexpr void check_bounds(player_t player) const {
    if (player.value < 1 || static_cast<std::size_t>(player.value) > N) {
      throw std::out_of_range(
          std::format("Player index {} out of range (1..{})", player.value, N));
    }
  }

  std::bitset<N> bits_{};
};

export template <std::size_t N, typename CharT>
struct std::formatter<PlayerBitset<N>, CharT> {
  constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const PlayerBitset<N>& b, FormatContext& ctx) const {
    return std::format_to(ctx.out(), "{}", b.to_string());
  }
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
    int val = std::stoi(std::string(s.begin(), s.end()));
    if (val > 0) {
      return shipnum_t{static_cast<shipnum_t::value_type>(val)};
    }
  }
  return {};
}

/// \brief Parses a 2D sector coordinate range string ("x,y" or "xl:xh,yl:yh")
/// into a pair of bounding Coordinates (low, high).
export [[nodiscard]] inline std::optional<std::pair<Coordinates, Coordinates>>
get4args(std::string_view s) {
  if (s.empty()) return std::nullopt;

  auto comma_pos = s.find(',');
  if (comma_pos == std::string_view::npos) return std::nullopt;

  std::string_view x_part = s.substr(0, comma_pos);
  std::string_view y_part = s.substr(comma_pos + 1);

  int xl = 0;
  int xh = 0;
  int yl = 0;
  int yh = 0;

  auto x_colon = x_part.find(':');
  if (x_colon != std::string_view::npos) {
    auto xl_result =
        std::from_chars(x_part.data(), x_part.data() + x_colon, xl);
    if (xl_result.ec != std::errc{} ||
        xl_result.ptr != x_part.data() + x_colon) {
      return std::nullopt;
    }

    auto xh_part = x_part.substr(x_colon + 1);
    auto xh_result =
        std::from_chars(xh_part.data(), xh_part.data() + xh_part.size(), xh);
    if (xh_result.ec != std::errc{} ||
        xh_result.ptr != xh_part.data() + xh_part.size()) {
      return std::nullopt;
    }
  } else {
    auto x_result =
        std::from_chars(x_part.data(), x_part.data() + x_part.size(), xl);
    if (x_result.ec != std::errc{} ||
        x_result.ptr != x_part.data() + x_part.size()) {
      return std::nullopt;
    }
    xh = xl;
  }

  auto y_colon = y_part.find(':');
  if (y_colon != std::string_view::npos) {
    auto yl_result =
        std::from_chars(y_part.data(), y_part.data() + y_colon, yl);
    if (yl_result.ec != std::errc{} ||
        yl_result.ptr != y_part.data() + y_colon) {
      return std::nullopt;
    }

    auto yh_part = y_part.substr(y_colon + 1);
    auto yh_result =
        std::from_chars(yh_part.data(), yh_part.data() + yh_part.size(), yh);
    if (yh_result.ec != std::errc{} ||
        yh_result.ptr != yh_part.data() + yh_part.size()) {
      return std::nullopt;
    }
  } else {
    auto y_result =
        std::from_chars(y_part.data(), y_part.data() + y_part.size(), yl);
    if (y_result.ec != std::errc{} ||
        y_result.ptr != y_part.data() + y_part.size()) {
      return std::nullopt;
    }
    yh = yl;
  }

  return std::make_pair(Coordinates{xl, yl}, Coordinates{xh, yh});
}
