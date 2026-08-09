// SPDX-License-Identifier: Apache-2.0

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

export struct stinfo {
  short temp_add; /* addition to temperature to each planet */
  unsigned char Thing_add;
  /* new Thing colony on this planet */
  unsigned char inhab;       /* explored by anybody */
  unsigned char intimidated; /* assault platform is here */
};

export struct vnbrain {
  unsigned short Total_mad; /* total # of VN's destroyed so far */
  unsigned char Most_mad;   /* player most mad at */
};

export struct sectinfo {
  player_t explored;  /* sector has been explored */
  unsigned char VN;   /* this sector has a VN */
  unsigned char done; /* this sector has been updated */
};

export struct ServerState {
  int id{1};                         // Always 1 - singleton entity
  unsigned long segments{1};         // Number of movement segments
  std::time_t next_update_time{0};   // Next update timestamp
  std::time_t next_segment_time{0};  // Next segment timestamp
  int update_time_minutes{10};       // Interval between updates in minutes
  segments_t nsegments_done{0};      // Segments completed this update
  std::string welcome_message{};  // Welcome message shown to connecting players
};

export struct Commod {
  int id{0};  // Commodity ID for database persistence
  player_t owner;
  governor_t governor;
  CommodType type;
  std::uint64_t amount;
  bool deliver; /* whether the lot is ready for shipping or not */
  money_t bid;
  player_t bidder;
  governor_t bidder_gov;
  starnum_t star_from; /* where the stuff originated from */
  planetnum_t planet_from;
  starnum_t star_to; /* where it goes to */
  planetnum_t planet_to;
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
  player_t racenum;
  std::string name;
  bool no_count = false;
  double tech;
  int Thing;
  int IQ;
  unsigned long rawscore;
};

export struct Coordinates {
  int x{0};
  int y{0};

  constexpr Coordinates() = default;
  constexpr Coordinates(int x_, int y_) noexcept : x(x_), y(y_) {}

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
    if (comma_pos == std::string_view::npos) return std::nullopt;

    auto x_str = str.substr(0, comma_pos);
    auto y_str = str.substr(comma_pos + 1);

    while (!x_str.empty() &&
           std::isspace(static_cast<unsigned char>(x_str.front())))
      x_str.remove_prefix(1);
    while (!x_str.empty() &&
           std::isspace(static_cast<unsigned char>(x_str.back())))
      x_str.remove_suffix(1);
    while (!y_str.empty() &&
           std::isspace(static_cast<unsigned char>(y_str.front())))
      y_str.remove_prefix(1);
    while (!y_str.empty() &&
           std::isspace(static_cast<unsigned char>(y_str.back())))
      y_str.remove_suffix(1);

    if (x_str.empty() || y_str.empty()) return std::nullopt;

    auto [p1, ec1] =
        std::from_chars(x_str.data(), x_str.data() + x_str.size(), x_val);
    if (ec1 != std::errc{} || p1 != x_str.data() + x_str.size())
      return std::nullopt;

    auto [p2, ec2] =
        std::from_chars(y_str.data(), y_str.data() + y_str.size(), y_val);
    if (ec2 != std::errc{} || p2 != y_str.data() + y_str.size())
      return std::nullopt;

    return Coordinates{x_val, y_val};
  }
};

export template <>
struct std::formatter<Coordinates> : std::formatter<std::string_view> {
  auto format(const Coordinates& c, auto& ctx) const {
    return std::format_to(ctx.out(), "{},{}", c.x, c.y);
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

  if (s.size() > 0 && std::isdigit(s.front())) {
    return std::stoi(std::string(s.begin(), s.end()));
  }
  return {};
}
