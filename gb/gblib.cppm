// SPDX-License-Identifier: Apache-2.0

export module gblib;

export import strong_id;  // Third-party strong type ID system

export import :bombard;
export import :build;
export import :csp;
export import :doplanet;
export import :dosector;
export import :doship;
export import :doturncmd;
export import :entitylists;
export import :fire;
export import :fuel;
export import :gameobj;
export import :map;
export import :misc;
export import :move;
export import :order;
export import :planet;
export import :place;
export import :types;
export import :sector;
export import :race;
export import :rand;
export import :repositories;
export import :services;
export import :sessionregistry;  // Cross-cutting session interface
export import :ships;
export import :shipfilter;
export import :shlmisc;
export import :shootblast;
export import :star;
export import :tele;
export import :turnstats;
export import :tweakables;
export import :universe;
export import :globals;

// Re-export string_to_shipnum from :types for convenience
export import :types;

export constexpr double morale_factor(const double x) {
  return (atan((double)x / 10000.) / 3.14159565 + .5);
}

export constexpr int M_FUEL = 0x1;
export constexpr int M_DESTRUCT = 0x2;
export constexpr int M_RESOURCES = 0x4;
export constexpr int M_CRYSTALS = 0x8;

export bool Fuel(int x) {
  return x & M_FUEL;
}
export bool Destruct(int x) {
  return x & M_DESTRUCT;
};
export bool Resources(int x) {
  return x & M_RESOURCES;
};
export bool Crystals(int x) {
  return x & M_CRYSTALS;
};

export std::vector<Victory> create_victory_list(EntityManager&);

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
  if (r.likes[s.get_condition()] == 0) return 0L;
  double a = ((double)s.get_eff() + 1.0) * (double)s.get_fert();
  double b = (.01 * c);

  auto val = std::lround(a * b * .01 * (100.0 - (double)toxic));

  return val;
}
