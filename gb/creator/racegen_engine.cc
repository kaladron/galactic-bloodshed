// SPDX-License-Identifier: Apache-2.0

/// \file racegen_engine.cc
/// \brief Implementation of the RacegenEngine point calculator and validator.

module;

import dallib;
import gb.entities;
import gb.services;
import gb.repositories;
import std;

module gb.creator;

namespace GB::creator {

namespace {

enum class AttributeIndex : std::size_t {
  Adventurism = 0,
  Absorb = 1,
  Birthrate = 2,
  CollectiveIQ = 3,
  Fertilize = 4,
  IQ = 5,
  Fight = 6,
  Pods = 7,
  Mass = 8,
  Sexes = 9,
  Metabolism = 10,
};

constexpr std::string_view to_string(AttributeIndex idx) noexcept {
  switch (idx) {
    case AttributeIndex::Adventurism:
      return "Adventurism";
    case AttributeIndex::Absorb:
      return "Absorb";
    case AttributeIndex::Birthrate:
      return "Birthrate";
    case AttributeIndex::CollectiveIQ:
      return "Collective IQ";
    case AttributeIndex::Fertilize:
      return "Fertilize";
    case AttributeIndex::IQ:
      return "IQ";
    case AttributeIndex::Fight:
      return "Fight";
    case AttributeIndex::Pods:
      return "Pods";
    case AttributeIndex::Mass:
      return "Mass";
    case AttributeIndex::Sexes:
      return "Sexes";
    case AttributeIndex::Metabolism:
      return "Metabolism";
  }
}

std::string attribute_name(const RaceEnrollmentSpec& spec, AttributeIndex idx) {
  if (idx == AttributeIndex::IQ && spec.collective_iq) {
    return "IQ Limit";
  }
  return std::string(to_string(idx));
}

double get_attribute_value(const RaceEnrollmentSpec& spec,
                           AttributeIndex idx) noexcept {
  switch (idx) {
    case AttributeIndex::Adventurism:
      return spec.adventurism;
    case AttributeIndex::Absorb:
      return spec.absorb ? 1.0 : 0.0;
    case AttributeIndex::Birthrate:
      return spec.birthrate;
    case AttributeIndex::CollectiveIQ:
      return spec.collective_iq ? 1.0 : 0.0;
    case AttributeIndex::Fertilize:
      return static_cast<double>(spec.fertilize) / 100.0;
    case AttributeIndex::IQ:
      return static_cast<double>(std::max(spec.iq, spec.iq_limit));
    case AttributeIndex::Fight:
      return static_cast<double>(spec.fighters);
    case AttributeIndex::Pods:
      return spec.pods ? 1.0 : 0.0;
    case AttributeIndex::Mass:
      return spec.mass;
    case AttributeIndex::Sexes:
      return static_cast<double>(spec.number_sexes);
    case AttributeIndex::Metabolism:
      return spec.metabolism;
  }
}

constexpr std::optional<int> get_planet_cost(PlanetType type) noexcept {
  switch (type) {
    case PlanetType::EARTH:
      return 75;
    case PlanetType::FOREST:
      return 50;
    case PlanetType::DESERT:
      return 50;
    case PlanetType::WATER:
      return 50;
    case PlanetType::MARS:
      return -25;
    case PlanetType::ICEBALL:
      return -25;
    case PlanetType::GASGIANT:
      return 600;
    default:
      return std::nullopt;
  }
}

constexpr bool is_habitable_home_planet(PlanetType type) noexcept {
  return get_planet_cost(type).has_value();
}

constexpr std::size_t num_settleable_sectors = 8;

constexpr std::array<int, 9> sector_count_costs = {
    -1, 0, 50, 100, 200, 300, 400, 500, 600,
};

constexpr std::array<std::array<double, num_settleable_sectors>,
                     num_settleable_sectors>
    sector_compat_cov = {{
        /* . Water (0) */ {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
        /* * Land (1) */ {0.001, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
        /* ^ Mount (2) */ {0.002, -0.0005, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
        /* ~ Gas (3) */ {999.0, 999.0, 999.0, 0.0, 0.0, 0.0, 0.0, 0.0},
        /* # Ice (4) */ {0.001, 0.0, -0.002, 999.0, 0.0, 0.0, 0.0, 0.0},
        /* ) Forest (5) */ {0.0, -0.001, 0.0, 999.0, 0.001, 0.0, 0.0, 0.0},
        /* - Desert (6) */ {0.003, -0.0005, 0.0, 999.0, 0.0, 0.001, 0.0, 0.0},
        /* o Plated (7) */ {0.0, 0.0, 0.0, 999.0, 0.0, 0.0, 0.0, 0.0},
    }};

constexpr double planet_compat_cov_multiplier(PlanetType planet,
                                              SectorType sector) noexcept {
  if (sector < SectorType::SEC_SEA || sector > SectorType::SEC_PLATED) {
    return 1.0;
  }
  // Sector order: SEC_SEA(0), SEC_LAND(1), SEC_MOUNT(2), SEC_GAS(3),
  // SEC_ICE(4), SEC_FOREST(5), SEC_DESERT(6), SEC_PLATED(7)
  switch (planet) {
    case PlanetType::EARTH: {
      constexpr std::array<double, num_settleable_sectors> cov = {
          1.00, 1.00, 2.00, 99.00, 1.01, 1.50, 3.00, 1.01};
      return cov[sector];
    }
    case PlanetType::FOREST: {
      constexpr std::array<double, num_settleable_sectors> cov = {
          1.01, 1.50, 2.00, 99.00, 1.01, 1.00, 3.00, 1.01};
      return cov[sector];
    }
    case PlanetType::DESERT: {
      constexpr std::array<double, num_settleable_sectors> cov = {
          3.00, 1.01, 1.01, 99.00, 1.50, 3.00, 1.00, 1.01};
      return cov[sector];
    }
    case PlanetType::WATER: {
      constexpr std::array<double, num_settleable_sectors> cov = {
          1.00, 1.50, 3.00, 99.00, 1.01, 1.01, 3.00, 1.01};
      return cov[sector];
    }
    case PlanetType::MARS: {  // Airless
      constexpr std::array<double, num_settleable_sectors> cov = {
          1.01, 1.00, 1.00, 99.00, 1.01, 1.01, 1.00, 1.01};
      return cov[sector];
    }
    case PlanetType::ICEBALL: {
      constexpr std::array<double, num_settleable_sectors> cov = {
          3.00, 1.01, 1.00, 99.00, 1.00, 1.50, 2.00, 1.01};
      return cov[sector];
    }
    case PlanetType::GASGIANT: {  // Jovian
      constexpr std::array<double, num_settleable_sectors> cov = {
          99.00, 99.00, 99.00, 1.00, 99.00, 99.00, 99.00, 99.00};
      return cov[sector];
    }
    default:
      return 1.0;
  }
}

}  // namespace

RacegenEngine::RacegenEngine() {
  // 0: Adventurism
  base_attr_[0] = {.e_factor = 0.0,
                   .e_fudge = 0.0,
                   .e_hinge = 0.0,
                   .l_factor = 300.0,
                   .minimum = 0.05,
                   .init = 0.4,
                   .maximum = 0.99,
                   .is_integral = 0};
  // 1: Absorb
  base_attr_[1] = {.e_factor = 0.0,
                   .e_fudge = 0.0,
                   .e_hinge = 0.0,
                   .l_factor = 200.0,
                   .minimum = 0.0,
                   .init = 0.0,
                   .maximum = 1.0,
                   .is_integral = 2};
  // 2: Birthrate
  base_attr_[2] = {.e_factor = 0.0,
                   .e_fudge = 0.0,
                   .e_hinge = 0.0,
                   .l_factor = 500.0,
                   .minimum = 0.2,
                   .init = 0.6,
                   .maximum = 1.0,
                   .is_integral = 0};
  // 3: Collective IQ
  base_attr_[3] = {.e_factor = 0.0,
                   .e_fudge = 0.0,
                   .e_hinge = 0.0,
                   .l_factor = -350.5,
                   .minimum = 0.0,
                   .init = 0.0,
                   .maximum = 1.0,
                   .is_integral = 2};
  // 4: Fertilize
  base_attr_[4] = {.e_factor = 200.0,
                   .e_fudge = 1.0,
                   .e_hinge = 1.0,
                   .l_factor = 300.0,
                   .minimum = 0.0,
                   .init = 0.0,
                   .maximum = 1.0,
                   .is_integral = 0};
  // 5: IQ
  base_attr_[5] = {.e_factor = 100.0,
                   .e_fudge = 0.03,
                   .e_hinge = 140.0,
                   .l_factor = 6.0,
                   .minimum = 50.0,
                   .init = 150.0,
                   .maximum = 220.0,
                   .is_integral = 1};
  // 6: Fight
  base_attr_[6] = {.e_factor = 10.0,
                   .e_fudge = 0.4,
                   .e_hinge = 6.0,
                   .l_factor = 65.0,
                   .minimum = 1.0,
                   .init = 4.0,
                   .maximum = 20.0,
                   .is_integral = 1};
  // 7: Pods
  base_attr_[7] = {.e_factor = 0.0,
                   .e_fudge = 0.0,
                   .e_hinge = 0.0,
                   .l_factor = 200.0,
                   .minimum = 0.0,
                   .init = 0.0,
                   .maximum = 1.0,
                   .is_integral = 2};
  // 8: Mass
  base_attr_[8] = {.e_factor = 100.0,
                   .e_fudge = 1.0,
                   .e_hinge = 3.1,
                   .l_factor = -100.0,
                   .minimum = 0.1,
                   .init = 1.0,
                   .maximum = 3.0,
                   .is_integral = 0};
  // 9: Sexes
  base_attr_[9] = {.e_factor = 2.2,
                   .e_fudge = -0.5,
                   .e_hinge = 9.0,
                   .l_factor = -3.0,
                   .minimum = 1.0,
                   .init = 2.0,
                   .maximum = 53.0,
                   .is_integral = 1};
  // 10: Metabolism
  base_attr_[10] = {.e_factor = 300.0,
                    .e_fudge = 1.0,
                    .e_hinge = 1.3,
                    .l_factor = 700.0,
                    .minimum = 0.1,
                    .init = 1.0,
                    .maximum = 4.0,
                    .is_integral = 0};

  auto attr_range = [this](AttributeIndex idx) {
    auto i = std::to_underlying(idx);
    return base_attr_[i].maximum - base_attr_[i].minimum;
  };

  const auto adv_idx = std::to_underlying(AttributeIndex::Adventurism);
  const auto brt_idx = std::to_underlying(AttributeIndex::Birthrate);
  const auto col_idx = std::to_underlying(AttributeIndex::CollectiveIQ);
  const auto frt_idx = std::to_underlying(AttributeIndex::Fertilize);
  const auto iq_idx = std::to_underlying(AttributeIndex::IQ);
  const auto fgt_idx = std::to_underlying(AttributeIndex::Fight);
  const auto mas_idx = std::to_underlying(AttributeIndex::Mass);
  const auto sex_idx = std::to_underlying(AttributeIndex::Sexes);
  const auto met_idx = std::to_underlying(AttributeIndex::Metabolism);

  // Normal race covariances
  normal_cov_[adv_idx][iq_idx] = -0.40 / attr_range(AttributeIndex::IQ);
  normal_cov_[brt_idx][iq_idx] = 0.20 / attr_range(AttributeIndex::IQ);
  normal_cov_[brt_idx][mas_idx] = 0.40 / attr_range(AttributeIndex::Mass);
  normal_cov_[brt_idx][sex_idx] = 0.90 / attr_range(AttributeIndex::Sexes);
  normal_cov_[brt_idx][met_idx] =
      -0.20 / attr_range(AttributeIndex::Metabolism);
  normal_cov_[frt_idx][met_idx] =
      -0.20 / attr_range(AttributeIndex::Metabolism);
  normal_cov_[fgt_idx][iq_idx] = -0.20 / attr_range(AttributeIndex::IQ);
  normal_cov_[fgt_idx][adv_idx] =
      -0.05 / attr_range(AttributeIndex::Adventurism);
  normal_cov_[fgt_idx][mas_idx] = -0.20 / attr_range(AttributeIndex::Mass);
  normal_cov_[fgt_idx][met_idx] =
      -0.05 / attr_range(AttributeIndex::Metabolism);
  normal_cov_[met_idx][mas_idx] = 0.15 / attr_range(AttributeIndex::Mass);
  normal_cov_[met_idx][iq_idx] = -0.10 / attr_range(AttributeIndex::IQ);
  normal_cov_[iq_idx][mas_idx] = -0.25 / attr_range(AttributeIndex::Mass);

  // Metamorph race covariances
  morph_cov_[adv_idx][iq_idx] = 0.0;
  morph_cov_[brt_idx][mas_idx] = 0.10 / attr_range(AttributeIndex::Mass);
  morph_cov_[brt_idx][sex_idx] = 0.50 / attr_range(AttributeIndex::Sexes);
  morph_cov_[brt_idx][met_idx] = -0.10 / attr_range(AttributeIndex::Metabolism);
  morph_cov_[frt_idx][met_idx] = -0.30 / attr_range(AttributeIndex::Metabolism);
  morph_cov_[fgt_idx][adv_idx] =
      -0.10 / attr_range(AttributeIndex::Adventurism);
  morph_cov_[fgt_idx][mas_idx] = -0.20 / attr_range(AttributeIndex::Mass);
  morph_cov_[fgt_idx][met_idx] = -0.15 / attr_range(AttributeIndex::Metabolism);
  morph_cov_[met_idx][mas_idx] = 0.05 / attr_range(AttributeIndex::Mass);
  morph_cov_[iq_idx][mas_idx] = 0.0;
  morph_cov_[iq_idx][col_idx] = 0.0;

  // Compute l_fudge values so that baseline normal attributes cost 0
  for (std::size_t i = 0; i < num_race_attributes; ++i) {
    const auto& p = base_attr_[i];
    double cost = std::exp(p.e_fudge * (p.init - p.e_hinge)) * p.e_factor +
                  p.init * p.l_factor;
    for (std::size_t j = 0; j < num_race_attributes; ++j) {
      if (normal_cov_[i][j] != 0.0) {
        cost *= (1.0 + normal_cov_[i][j] * base_attr_[j].init);
      }
    }
    base_attr_[i].l_fudge = -cost;
  }
}

RaceEnrollmentSpec
RacegenEngine::create_default_spec(bool metamorph) const noexcept {
  RaceEnrollmentSpec spec{};
  spec.name = "Unknown";
  spec.password = "XXXX";
  spec.address = "Unknown";
  spec.home_planet_type = PlanetType::EARTH;
  spec.is_god = false;
  spec.is_guest = false;

  spec.metamorph = metamorph;
  spec.absorb = metamorph;
  spec.collective_iq = metamorph;
  spec.pods = metamorph;

  spec.adventurism = 0.4;
  spec.birthrate = 0.6;
  spec.fertilize = 0;
  if (metamorph) {
    spec.iq = 0;
    spec.iq_limit = 150;
  } else {
    spec.iq = 150;
    spec.iq_limit = 0;
  }
  spec.fighters = 4;
  spec.mass = 1.0;
  spec.number_sexes = 2;
  spec.metabolism = 1.0;

  spec.sector_compatibilities[SectorType::SEC_PLATED] = 1.0;
  spec.likesbest = SectorType::SEC_PLATED;
  spec.preferred_sector = SectorType::SEC_PLATED;

  return spec;
}

RaceCostBreakdown
RacegenEngine::calculate_cost(const RaceEnrollmentSpec& spec) const noexcept {
  RaceCostBreakdown breakdown{};
  const auto& cov = spec.metamorph ? morph_cov_ : normal_cov_;
  int sum = 0;

  // 1. Calculate attribute costs
  for (std::size_t i = 0; i < num_race_attributes; ++i) {
    const auto& p = base_attr_[i];
    const auto attr_idx = static_cast<AttributeIndex>(i);
    const double val = get_attribute_value(spec, attr_idx);
    double cost =
        std::exp(p.e_fudge * (val - p.e_hinge)) * p.e_factor + val * p.l_factor;
    for (std::size_t j = 0; j < num_race_attributes; ++j) {
      if (cov[i][j] != 0.0) {
        cost *= (1.0 + cov[i][j] * get_attribute_value(
                                       spec, static_cast<AttributeIndex>(j)));
      }
    }
    cost += p.l_fudge;
    breakdown.attribute_costs[i] = std::round(cost);
    sum += static_cast<int>(breakdown.attribute_costs[i]);
  }

  // 2. Planet and race type costs
  breakdown.planet_cost = get_planet_cost(spec.home_planet_type).value_or(0);
  sum += breakdown.planet_cost;
  breakdown.race_type_cost = 0;
  sum += breakdown.race_type_cost;

  // 3. Sector compatibility costs
  // Settleable sector types: SEC_SEA (0) through SEC_PLATED (7).
  // Compatibilities in spec are 0.0 to 1.0, scaled to 0.0 to 100.0 for classic
  // formula.
  std::size_t sector_types_count = 0;
  for (std::size_t i = 0; i < num_settleable_sectors; ++i) {
    const auto st = static_cast<SectorType>(i);
    const double compat_pct = spec.sector_compatibilities[st] * 100.0;
    if (compat_pct > 0.0) {
      ++sector_types_count;
    }
    breakdown.sector_costs[st] =
        compat_pct * 0.5 + 10.8 * std::log(1.0 + compat_pct);
  }

  for (std::size_t i = 0; i < num_settleable_sectors; ++i) {
    const auto st_i = static_cast<SectorType>(i);
    const double compat_i = spec.sector_compatibilities[st_i] * 100.0;
    for (std::size_t j = i + 1; j < num_settleable_sectors; ++j) {
      const auto st_j = static_cast<SectorType>(j);
      const double compat_j = spec.sector_compatibilities[st_j] * 100.0;
      if (sector_compat_cov[j][i] != 0.0) {
        breakdown.sector_costs[st_i] *=
            (1.0 + sector_compat_cov[j][i] * compat_j);
        breakdown.sector_costs[st_j] *=
            (1.0 + sector_compat_cov[j][i] * compat_i);
      }
    }
  }

  for (std::size_t i = 0; i < num_settleable_sectors; ++i) {
    const auto st = static_cast<SectorType>(i);
    const double multiplier =
        planet_compat_cov_multiplier(spec.home_planet_type, st);
    if (multiplier > 1.01) {
      breakdown.sector_costs[st] *= multiplier;
    }
  }

  for (std::size_t i = 0; i < num_settleable_sectors; ++i) {
    const auto st = static_cast<SectorType>(i);
    breakdown.sector_costs[st] = std::round(breakdown.sector_costs[st]);
    sum += static_cast<int>(breakdown.sector_costs[st]);
  }
  breakdown.sector_costs[SectorType::SEC_WASTED] = 0.0;

  breakdown.sector_count_cost = sector_count_costs[std::min(
      sector_types_count, sector_count_costs.size() - 1)];
  sum += breakdown.sector_count_cost;

  breakdown.total_cost = sum;
  breakdown.points_remaining = STARTING_POINTS - sum;
  return breakdown;
}

std::vector<std::string> RacegenEngine::validate(const RaceEnrollmentSpec& spec,
                                                 bool is_player,
                                                 bool rigorous) const {
  std::vector<std::string> errors;

  // 1. Attribute bounds and validity
  for (std::size_t i = 0; i < num_race_attributes; ++i) {
    const auto attr_idx = static_cast<AttributeIndex>(i);
    const auto& p = base_attr_[i];
    const double val = get_attribute_value(spec, attr_idx);
    const auto name = attribute_name(spec, attr_idx);

    if (p.is_integral == 2 && val != 0.0 && val != 1.0) {
      errors.push_back(
          std::format("{} is boolean valued. Use \"yes\" or \"no\".", name));
    }
    if (val < p.minimum) {
      errors.push_back(
          std::format("{} must be at least {:.2f}.", name, p.minimum));
    }
    if (val > p.maximum) {
      errors.push_back(
          std::format("{} may be at most {:.2f}.", name, p.maximum));
    }
  }

  // 2. Normal race characteristics
  if (!spec.metamorph) {
    if (spec.absorb) {
      errors.push_back("Normal races do not absorb their enemies in combat.");
    }
    if (spec.pods) {
      errors.push_back("Normal races do not make pods.");
    }
  }

  // 3. Name validation
  if (spec.name.empty()) {
    errors.push_back("Use a non-empty name.");
  }

  // 4. Privileges
  if (is_player && (spec.is_god || spec.is_guest)) {
    errors.push_back("Players may not create non-normal races.");
  }

  // 5. Home planet validity
  if (!is_habitable_home_planet(spec.home_planet_type)) {
    errors.push_back("Home planet type out of valid range.");
  }

  // 6. Sector compatibilities
  const bool is_jovian = (spec.home_planet_type == PlanetType::GASGIANT);

  if (!is_jovian &&
      spec.sector_compatibilities[SectorType::SEC_PLATED] != 1.0) {
    errors.push_back("Non-jovian races must have 100% plated compat.");
  }

  for (std::size_t i = 0; i < num_settleable_sectors; ++i) {
    const auto st = static_cast<SectorType>(i);
    const double compat = spec.sector_compatibilities[st];
    if (compat < 0.0) {
      errors.push_back("Sector compatibility is at minimum 0%.");
    }
    if (compat > 1.0) {
      errors.push_back("Sector compatibility may be at most 100%.");
    }
    if (st == SectorType::SEC_GAS && compat != 0.0 && !is_jovian) {
      errors.push_back("Non-jovian races may never have gas compatibility!");
    }
    if (st != SectorType::SEC_GAS && compat != 0.0 && is_jovian) {
      errors.push_back(
          "Jovian races may have no compatibility other than gas!");
    }
  }

  // 7. Rigorous checking
  if (rigorous) {
    if (spec.password.length() < MIN_PASSWORD_LENGTH) {
      errors.push_back(std::format(
          "Passwords are required to be at least {} characters long.",
          MIN_PASSWORD_LENGTH));
    } else if (spec.password == "XXXX") {
      errors.push_back("You must change your password from the default.");
    }

    if (spec.address == "Unknown" || spec.address.empty()) {
      errors.push_back("You must change your email address.");
    }

    const auto breakdown = calculate_cost(spec);
    if (breakdown.points_remaining < 0) {
      errors.push_back("You can't have negative points left!");
    }

    std::size_t count = 0;
    for (std::size_t i = 0; i < num_settleable_sectors; ++i) {
      const auto st = static_cast<SectorType>(i);
      if (spec.sector_compatibilities[st] > 0.0) {
        ++count;
      }
    }

    if (!is_jovian && count == 1) {
      errors.push_back(
          "Non-jovian races must be compat with at least one sector type "
          "besides plated.");
    }

    bool has_common_sector = false;
    for (std::size_t i = 0; i < num_settleable_sectors; ++i) {
      const auto st = static_cast<SectorType>(i);
      if (Planet::is_common_sector(spec.home_planet_type, st) &&
          spec.sector_compatibilities[st] == 1.0) {
        has_common_sector = true;
        break;
      }
    }
    if (!has_common_sector) {
      errors.push_back(
          "You must have 100% compat with at least one sector type that is "
          "common on your home planet type.");
    }
  }

  return errors;
}

}  // namespace GB::creator
