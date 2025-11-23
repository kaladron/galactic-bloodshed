// SPDX-License-Identifier: Apache-2.0

export module gblib:types;

import std.compat;

export using shipnum_t = std::uint64_t;
export using starnum_t = std::uint32_t;
export using planetnum_t = std::uint32_t;
export using player_t = std::uint32_t;
export using governor_t = std::uint32_t;
export using segments_t = std::uint32_t;

export using ap_t = std::uint32_t;
export using commodnum_t = std::int64_t;
export using resource_t = std::int64_t;
export using money_t = std::int64_t;
export using population_t = std::int64_t;

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
  char explored;      /* sector has been explored */
  unsigned char VN;   /* this sector has a VN */
  unsigned char done; /* this sector has been updated */
};

export struct Commod {
  int id{0};  // Commodity ID for database persistence
  player_t owner;
  governor_t governor;
  CommodType type;
  uint64_t amount;
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
  int x;
  int y;

  Coordinates(int x_, int y_) : x(x_), y(y_) {}
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

