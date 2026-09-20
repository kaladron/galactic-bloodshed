// SPDX-License-Identifier: Apache-2.0

/// \file planet.cc
/// \brief Planet domain object implementations.

module;

import std;

module gb.entities;

/**
 * @brief Determines if two coordinates are adjacent on a planet.
 *
 * This function checks if two coordinates on a planet are adjacent to each
 * other. Adjacency is defined as having a maximum difference of 1 in both the x
 * and y coordinates. Additionally, the function handles the case where the
 * coordinates wrap around the planet's boundaries.
 *
 * @param from The starting coordinates.
 * @param to The target coordinates.
 * @return True if the coordinates are adjacent, false otherwise.
 */
bool Planet::is_adjacent(const Coordinates from,
                         const Coordinates to) const noexcept {
  if (std::abs(from.y - to.y) > 1) return false;
  const int dx = std::abs(from.x - to.x);
  if (dx <= 1) return true;
  if (dimensions().x > 0 && dx == dimensions().x - 1) return true;
  return false;
}

std::vector<Coordinates> Planet::adjacent_coordinates(Coordinates from) const {
  std::vector<Coordinates> neighbors;
  if (dimensions().x <= 0 || dimensions().y <= 0) {
    return neighbors;
  }
  neighbors.reserve(8);
  for (int dy = -1; dy <= 1; ++dy) {
    const int new_y = from.y + dy;
    if (new_y < 0 || new_y >= dimensions().y) {
      continue;
    }
    for (int dx = -1; dx <= 1; ++dx) {
      if (dx == 0 && dy == 0) {
        continue;
      }
      neighbors.push_back(wrap(Coordinates{from.x + dx, new_y}));
    }
  }
  return neighbors;
}

Coordinates Planet::random_adjacent_coordinates(Coordinates from) const {
  const auto neighbors = adjacent_coordinates(from);
  if (neighbors.empty()) {
    return from;
  }
  return neighbors[int_rand(0, neighbors.size() - 1)];
}

void Planet::update_climate(int temp_variance) noexcept {
  conditions(TEMP) = conditions(RTEMP) + temp_variance + int_rand(-5, 5);
}

money_t plinfo::collect_tax(Race::gov& gov, const Race& race) noexcept {
  if (!race.has_government_center()) {
    prod_money = 0;
    return 0;
  }
  prod_money = round_rand(INCOME_FACTOR * static_cast<double>(tax) *
                          static_cast<double>(popn));
  gov.money += prod_money;
  gov.income += prod_money;
  tax += std::min(static_cast<int>(newtax) - static_cast<int>(tax), 5);
  return prod_money;
}

double plinfo::invest_tech(Race::gov& gov, Race& race) noexcept {
  if (!race.has_government_center() || gov.money < tech_invest) {
    prod_tech = 0.0;
    return 0.0;
  }
  prod_tech = tech_prod(static_cast<int>(tech_invest), static_cast<int>(popn));
  gov.money -= tech_invest;
  gov.cost_tech += tech_invest;
  race.tech += prod_tech;
  return prod_tech;
}

void plinfo::update_combat_readiness(std::uint32_t total_mob_points) noexcept {
  mob_points = total_mob_points;
  if (numsectsowned > 0) {
    comread = total_mob_points / numsectsowned;
  } else {
    comread = 0;
  }
  guns = static_cast<std::uint32_t>(planet_guns(mob_points));
}

UniverseCoordinates
Planet::absolute_coordinates(const Star& star) const noexcept {
  return star.coordinates() + system_coordinates();
}

namespace {

void apply_sector_civ_delta(Sector& sect, planet_struct& data, player_t owner,
                            population_t civ_delta) noexcept {
  if (civ_delta > 0) {
    sect.add_popn(civ_delta);
    data.popn += civ_delta;
    if (owner != 0) {
      data.info[owner].popn += civ_delta;
    }
  } else if (civ_delta < 0) {
    const population_t loss = std::min(sect.get_popn(), -civ_delta);
    sect.subtract_popn(loss);
    data.popn = (data.popn >= loss) ? (data.popn - loss) : 0;
    if (owner != 0) {
      auto& pinfo = data.info[owner];
      pinfo.popn = (pinfo.popn >= loss) ? (pinfo.popn - loss) : 0;
    }
  }
}

void apply_sector_mil_delta(Sector& sect, planet_struct& data, player_t owner,
                            population_t mil_delta) noexcept {
  if (mil_delta > 0) {
    sect.add_troops(mil_delta);
    data.troops += mil_delta;
    if (owner != 0) {
      data.info[owner].troops += mil_delta;
    }
  } else if (mil_delta < 0) {
    const population_t loss = std::min(sect.get_troops(), -mil_delta);
    sect.subtract_troops(loss);
    data.troops = (data.troops >= loss) ? (data.troops - loss) : 0;
    if (owner != 0) {
      auto& pinfo = data.info[owner];
      pinfo.troops = (pinfo.troops >= loss) ? (pinfo.troops - loss) : 0;
    }
  }
}

void abandon_sector_if_empty(Sector& sect, planet_struct& data,
                             player_t owner) noexcept {
  if (owner == 0 || !sect.is_empty()) {
    return;
  }
  sect.set_owner(0);
  sect.set_race(0);
  auto& pinfo = data.info[owner];
  if (pinfo.numsectsowned > 0) {
    pinfo.numsectsowned -= 1;
  }
  const auto sect_mob = sect.get_mobilization();
  pinfo.mob_points =
      (pinfo.mob_points >= sect_mob) ? (pinfo.mob_points - sect_mob) : 0;
}

}  // namespace

void Planet::adjust_sector_population(Sector& sect, player_t player,
                                      population_t civ_delta,
                                      population_t mil_delta) noexcept {
  // Colonization transition: unowned sector gains population
  if (!sect.is_owned() && player != 0 && (civ_delta > 0 || mil_delta > 0)) {
    sect.set_owner(player);
    sect.set_race(player);
    data_.info[player].numsectsowned += 1;
    data_.info[player].mob_points += sect.get_mobilization();
  }

  const player_t owner = sect.get_owner();
  apply_sector_civ_delta(sect, data_, owner, civ_delta);
  apply_sector_mil_delta(sect, data_, owner, mil_delta);
  abandon_sector_if_empty(sect, data_, owner);
}

void Planet::move_sector_population(Sector& from, Sector& to, player_t player,
                                    population_t amount,
                                    PopulationType type) noexcept {
  if (amount <= 0) {
    return;
  }
  if (type == PopulationType::CIV) {
    adjust_sector_population(from, from.get_owner(), -amount, 0);
    adjust_sector_population(to, player, amount, 0);
  } else if (type == PopulationType::MIL) {
    adjust_sector_population(from, from.get_owner(), 0, -amount);
    adjust_sector_population(to, player, 0, amount);
  }
}

void Planet::sync_demographics(const SectorMap& smap) noexcept {
  data_.popn = 0;
  data_.troops = 0;
  for (plinfo& pinfo : data_.info) {
    pinfo.popn = 0;
    pinfo.troops = 0;
    pinfo.numsectsowned = 0;
    pinfo.mob_points = 0;
  }

  for (const Sector& s : smap) {
    data_.popn += s.get_popn();
    data_.troops += s.get_troops();
    if (s.is_owned()) {
      const player_t owner = s.get_owner();
      data_.info[owner].popn += s.get_popn();
      data_.info[owner].troops += s.get_troops();
      data_.info[owner].numsectsowned += 1;
      data_.info[owner].mob_points += s.get_mobilization();
    }
  }
}

int Planet::revolt(SectorMap& smap, const Race& victim_race,
                   const player_t agent) {
  const player_t victim = victim_race.Playernum;
  int revolted_sectors = 0;
  for (Sector& s : smap) {
    if (s.get_owner() != victim || s.get_popn() == 0) continue;

    // Revolt rate is a function of tax rate.
    if (!success(info(victim).tax)) continue;

    if (long_rand(1, s.get_popn()) <=
        10L * victim_race.fighters * s.get_troops()) {
      continue;
    }

    // Revolt successful: sector transfers to agent, some civilians die, and
    // defending troops are eliminated.
    s.set_owner(agent);
    s.subtract_popn(long_rand(0, s.get_popn() - 1));
    s.set_troops(0);
    revolted_sectors++;
  }
  if (revolted_sectors > 0) {
    sync_demographics(smap);
  }

  return revolted_sectors;
}

std::expected<PlunderDistribution, PlunderError>
calculate_plunder_distribution(Stockpile total_loot,
                               std::span<const player_t> conquerors) {
  if (conquerors.empty()) {
    return std::unexpected(PlunderError::NoConquerors);
  }
  if (total_loot.empty()) {
    return std::unexpected(PlunderError::EmptyLoot);
  }

  const std::size_t shares_count = conquerors.size();
  Stockpile remaining = total_loot;
  std::vector<PlayerLootShare> shares;
  shares.reserve(shares_count);

  for (const player_t conqueror : conquerors.first(shares_count - 1)) {
    const Stockpile allocated =
        total_loot.split_share(shares_count).clamp_to(remaining);
    remaining -= allocated;
    shares.push_back(PlayerLootShare{.player = conqueror, .share = allocated});
  }

  // Last conqueror gets all leftovers
  shares.push_back(
      PlayerLootShare{.player = conquerors.back(), .share = remaining});

  return PlunderDistribution{
      .shares = std::move(shares),
      .total_loot = total_loot,
  };
}

PlanetExplorationContext::PlanetExplorationContext(Coordinates dimensions)
    : dimensions_(dimensions),
      explored_(static_cast<std::size_t>(dimensions.x) *
                static_cast<std::size_t>(dimensions.y)) {}

PlanetExplorationContext::PlanetExplorationContext(const Planet& planet)
    : PlanetExplorationContext(planet.dimensions()) {}

bool PlanetExplorationContext::in_bounds(Coordinates c) const noexcept {
  return c.x >= 0 && c.y >= 0 && c.x < dimensions_.x && c.y < dimensions_.y;
}

bool PlanetExplorationContext::is_explored(Coordinates c,
                                           player_t player) const {
  return explored_[index(c)].test(player.value);
}

bool PlanetExplorationContext::is_explored(Coordinates c) const {
  return explored_[index(c)].any();
}

void PlanetExplorationContext::set_explored(Coordinates c, player_t player) {
  explored_[index(c)].set(player.value);
}

void PlanetExplorationContext::clear_explored(Coordinates c, player_t player) {
  explored_[index(c)].reset(player.value);
}

bool PlanetExplorationContext::all_explored(player_t player) const {
  return std::ranges::all_of(explored_, [player](const auto& bitset) {
    return bitset.test(player.value);
  });
}

bool PlanetExplorationContext::all_explored() const {
  return std::ranges::all_of(explored_,
                             [](const auto& bitset) { return bitset.any(); });
}

void PlanetExplorationContext::explore_sector(const Planet& planet,
                                              const Sector& s, player_t p) {
  const Coordinates c = s.coords();
  if (is_explored(c, p)) {
    for (const auto& neighbor : planet.adjacent_coordinates(c)) {
      set_explored(neighbor, p);
    }
  } else if (s.get_owner() == p) {
    set_explored(c, p);
  }
}

std::optional<Coordinates>
Planet::process_toxic_environmental_damage(SectorMap& smap) const {
  if (conditions(TOXIC) <= ENVIR_DAMAGE_TOX) {
    return std::nullopt;
  }
  auto& p = smap.get_random();
  p.devastate();
  return p.coords();
}

std::optional<player_t> Planet::select_victim_to_steal_from(
    std::span<const player_t> race_order) const {
  for (player_t candidate : race_order) {
    if (info(candidate).resource > 0) {
      return candidate;
    }
  }
  return std::nullopt;
}

namespace {

// TODO(C++26): Use std::inplace_vector when it lands in libc++ and make
// constexpr when P3372 (constexpr containers and adaptors) lands.
const std::flat_map<char, Coordinates> direction_mappings{
    {'1', {-1, 1}},  {'b', {-1, 1}},   // Southwest
    {'2', {0, 1}},   {'k', {0, 1}},    // South
    {'3', {1, 1}},   {'n', {1, 1}},    // Southeast
    {'4', {-1, 0}},  {'h', {-1, 0}},   // West
    {'6', {1, 0}},   {'l', {1, 0}},    // East
    {'7', {-1, -1}}, {'y', {-1, -1}},  // Northwest
    {'8', {0, -1}},  {'j', {0, -1}},   // North
    {'9', {1, -1}},  {'u', {1, -1}},   // Northeast
};

}  // namespace

Coordinates get_move(const Planet& planet, const char direction,
                     const Coordinates from) {
  if (const auto it = direction_mappings.find(direction);
      it != direction_mappings.end()) {
    return planet.wrap(from + it->second);
  }
  return from;
}
