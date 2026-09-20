// SPDX-License-Identifier: Apache-2.0

/// \file sector.cc
/// \brief Sector domain object implementations.

module;

import std;

module gb.entities;

std::ostream& operator<<(std::ostream& os, const Sector& s) {
  os << "Efficiency: " << s.get_eff() << std::endl;
  os << "Fertility: " << s.get_fert() << std::endl;
  os << "Mobilization: " << s.get_mobilization() << std::endl;
  os << "Crystals: " << s.get_crystals() << std::endl;
  os << "Resource: " << s.get_resource() << std::endl;
  os << "Population: " << s.get_popn() << std::endl;
  os << "Troops: " << s.get_troops() << std::endl;
  os << "Owner: " << s.get_owner() << std::endl;
  os << "Race: " << s.get_race() << std::endl;
  os << "Type: " << s.get_type() << std::endl;
  os << "Condition: " << s.get_condition() << std::endl;
  return os;
}

// Population operation implementations with invariant protection
namespace {
// Reasonable maximum population per sector
constexpr population_t kMaxPopulationPerSector = 10'000'000;
}  // namespace

void Sector::add_popn(population_t amount) noexcept {
  if (amount == 0) return;

  population_t new_popn = data_.popn;

  // Saturate at max to prevent overflow
  if (data_.popn > kMaxPopulationPerSector - amount) {
    new_popn = kMaxPopulationPerSector;
    log_invariant_violation(
        "Sector", "popn", std::format("{} + {}", data_.popn, amount), new_popn);
  } else {
    new_popn = data_.popn + amount;
  }

  data_.popn = new_popn;
}

void Sector::subtract_popn(population_t amount) noexcept {
  if (amount == 0) return;

  // Log if trying to subtract more than available
  if (amount > data_.popn) {
    log_invariant_violation("Sector", "popn",
                            std::format("subtract {}", amount), "clamped to 0");
    data_.popn = 0;
  } else {
    data_.popn -= amount;
  }
}

void Sector::transfer_popn_to(Sector& dest, population_t amount) noexcept {
  if (amount == 0) return;

  // Check if transfer amount exceeds source
  if (amount > data_.popn) {
    log_invariant_violation(
        "Sector", "transfer_popn",
        std::format("transfer {} from sector with {}", amount, data_.popn),
        "clamped to available");
    amount = data_.popn;
  }

  // Perform atomic transfer
  data_.popn -= amount;
  dest.add_popn(amount);
  if (dest.data_.owner == 0 && dest.data_.popn > 0) {
    dest.data_.owner = data_.owner;
    dest.data_.race = (data_.race != 0) ? data_.race : data_.owner;
  }
}

void Sector::add_troops(population_t amount) noexcept {
  if (amount == 0) return;

  population_t new_troops = data_.troops;
  if (data_.troops > kMaxPopulationPerSector - amount) {
    new_troops = kMaxPopulationPerSector;
    log_invariant_violation("Sector", "troops",
                            std::format("{} + {}", data_.troops, amount),
                            new_troops);
  } else {
    new_troops = data_.troops + amount;
  }

  data_.troops = new_troops;
}

void Sector::subtract_troops(population_t amount) noexcept {
  if (amount == 0) return;

  if (amount > data_.troops) {
    log_invariant_violation("Sector", "troops",
                            std::format("subtract {}", amount), "clamped to 0");
    data_.troops = 0;
  } else {
    data_.troops -= amount;
  }
}

void Sector::adjust_mobilization(int delta) noexcept {
  data_.mobilization.adjust(delta);
}

void Sector::set_mobilization_bounded(int val) noexcept {
  data_.mobilization = Percentage{val};
}

// Resource operation implementations
void Sector::add_resource(resource_t amount) noexcept {
  if (amount == 0) return;
  data_.resource += amount;
}

void Sector::subtract_resource(resource_t amount) noexcept {
  if (amount == 0) return;

  if (amount < 0) {
    log_invariant_violation("Sector", "resource",
                            std::format("subtract {}", amount),
                            "negative amount ignored");
    return;
  }

  // Log if trying to subtract more than available
  if (amount > data_.resource) {
    log_invariant_violation("Sector", "resource",
                            std::format("subtract {}", amount), "clamped to 0");
    data_.resource = 0;
  } else {
    data_.resource -= amount;
  }
}

resource_t Sector::deplete_resource(resource_t amount) noexcept {
  if (amount <= 0) return 0;

  const resource_t actual = std::min(amount, data_.resource);
  data_.resource -= actual;
  return actual;
}

// Efficiency and fertility operation implementations (0-100 bounds)
void Sector::set_fert(int val) noexcept {
  data_.fert = Percentage{val};
}

void Sector::set_efficiency_bounded(int eff) noexcept {
  data_.eff = Percentage{eff};
}

void Sector::improve_efficiency(int delta) noexcept {
  if (delta == 0) return;

  if (delta < 0) {
    log_invariant_violation(
        "Sector", "eff",
        std::format("improve_efficiency with negative delta {}", delta),
        "use degrade_efficiency instead");
    return;
  }

  data_.eff.adjust(delta);
}

void Sector::degrade_efficiency(int delta) noexcept {
  if (delta == 0) return;

  if (delta < 0) {
    log_invariant_violation(
        "Sector", "eff",
        std::format("degrade_efficiency with negative delta {}", delta),
        "use improve_efficiency instead");
    return;
  }

  // Normal operation: degrade by delta, clamping to zero
  // Don't log if delta exceeds current eff - this is expected in combat
  data_.eff.adjust(-delta);
}

namespace {
// Supernova stellar radiation and environmental constants
constexpr resource_t nova_resource_deposit = 1;
constexpr Percentage fertility_loss_percent{20};   // 20% fertility loss
constexpr int terminal_nova_stage = 14;            // Final stellar explosion
constexpr Percentage radiation_casualty_rate{50};  // ~50% casualties per turn
}  // namespace

void Sector::apply_supernova(int stage) noexcept {
  // Heavy element nucleosynthesis from stellar radiation deposits mineral
  // resources.
  data_.resource += nova_resource_deposit;

  // Intense thermal and ionizing radiation degrades planetary agricultural
  // fertility.
  data_.fert -= (data_.fert.value() * fertility_loss_percent.value()) / 100;

  // Stage 14 represents the terminal stellar explosion, completely incinerating
  // all life.
  if (stage >= terminal_nova_stage) {
    clear_popn();
    data_.owner = 0;
    data_.troops = 0;
  } else {
    // Active nova radiation: kills approximately 50% of the living population
    // per turn.
    auto deaths = round_rand(static_cast<double>(data_.popn) *
                             radiation_casualty_rate.as_fraction());
    subtract_popn(deaths);
  }

  clear_owner_if_empty();
}

void Sector::recover_fertility(const Race& race) noexcept {
  if (!is_wasted() && race.fertilize > 0 && data_.fert < 100 &&
      int_rand(0, 100) < race.fertilize) {
    ++data_.fert;
  }

  if (is_wasted() && success(NATURAL_REPAIR)) {
    data_.condition = data_.type;
  }
}

void Sector::update_efficiency(const Race& race,
                               const Planet& planet) noexcept {
  if (!is_owned()) return;

  if (data_.eff < 100) {
    const int chance =
        round_rand((100.0 - static_cast<double>(planet.info(data_.owner).tax)) *
                   race.sector_compatibility(*this));
    if (success(chance)) {
      improve_efficiency(round_rand(race.metabolism));
      if (data_.eff >= 100) {
        plate();
      }
    }
  } else {
    plate();
  }
}

Stockpile Sector::produce_resources(const Race& race) noexcept {
  if (!is_owned() || data_.resource <= 0 ||
      !success(static_cast<int>(data_.eff))) {
    return {};
  }

  const double eff_scale =
      double_rand(1.0, std::max(1.0, static_cast<double>(data_.eff)));
  resource_t prod = round_rand<resource_t>(race.metabolism * eff_scale);
  prod = std::clamp(prod, resource_t{0}, data_.resource);
  if (prod == 0) return {};

  data_.resource -= prod;

  const auto pfuel = prod * (1 + (data_.condition == SectorType::SEC_GAS));

  if (success(static_cast<int>(data_.mobilization))) {
    return Stockpile{.destruct = prod, .fuel = pfuel};
  }
  return Stockpile{.resources = prod, .fuel = pfuel};
}

bool Sector::mine_crystals(const Race& race) noexcept {
  if (!is_owned() || data_.crystals == 0 || !race.discoveries.crystal ||
      !success(static_cast<int>(data_.eff))) {
    return false;
  }
  --data_.crystals;
  return true;
}

population_t calculate_migrating_colonists(const Race& race,
                                           double compatibility,
                                           const Sector& target,
                                           population_t available_migrants) {
  if (available_migrants <= 0 || target.is_owned()) {
    return 0;
  }
  const double likes_factor = race.sector_compatibility(target);
  const double move_calc = static_cast<double>(available_migrants) *
                           compatibility * likes_factor / 100.0;
  return std::clamp(std::lround(move_calc), population_t{0},
                    available_migrants);
}

population_t calculate_population_change(const Race& race, const Sector& s,
                                         population_t maxsup) {
  const population_t popn = s.get_popn();
  if (popn <= 0) {
    return 0;
  }

  const population_t diff = popn - maxsup;

  if (diff < 0) {
    if (popn >= race.number_sexes) {
      return round_rand<population_t>(-static_cast<double>(diff) *
                                      race.birthrate);
    }
    return 0;
  }

  if (diff == 0) {
    return 0;
  }

  // Overpopulation starvation die-off: range [0, min(2 * diff, popn)]
  // Saturate 2 * diff to prevent integer overflow
  const population_t max_die_off =
      (diff > std::numeric_limits<population_t>::max() / 2)
          ? popn
          : std::min(2 * diff, popn);
  return -long_rand(0, max_die_off);
}
