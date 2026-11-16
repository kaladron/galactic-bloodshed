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

constexpr std::string_view to_string(RaceAttribute idx) noexcept {
  switch (idx) {
    case RaceAttribute::Adventurism:
      return "Adventurism";
    case RaceAttribute::Absorb:
      return "Absorb";
    case RaceAttribute::Birthrate:
      return "Birthrate";
    case RaceAttribute::CollectiveIQ:
      return "Collective IQ";
    case RaceAttribute::Fertilize:
      return "Fertilize";
    case RaceAttribute::IQ:
      return "IQ";
    case RaceAttribute::Fight:
      return "Fight";
    case RaceAttribute::Pods:
      return "Pods";
    case RaceAttribute::Mass:
      return "Mass";
    case RaceAttribute::Sexes:
      return "Sexes";
    case RaceAttribute::Metabolism:
      return "Metabolism";
  }
}

std::string attribute_name(const RaceEnrollmentSpec& spec, RaceAttribute idx) {
  if (idx == RaceAttribute::IQ && spec.collective_iq) {
    return "IQ Limit";
  }
  return std::string(to_string(idx));
}

double get_attribute_value(const RaceEnrollmentSpec& spec,
                           RaceAttribute idx) noexcept {
  switch (idx) {
    case RaceAttribute::Adventurism:
      return spec.adventurism;
    case RaceAttribute::Absorb:
      return spec.absorb ? 1.0 : 0.0;
    case RaceAttribute::Birthrate:
      return spec.birthrate;
    case RaceAttribute::CollectiveIQ:
      return spec.collective_iq ? 1.0 : 0.0;
    case RaceAttribute::Fertilize:
      return spec.fertilize.as_fraction();
    case RaceAttribute::IQ:
      return static_cast<double>(std::max(spec.iq, spec.iq_limit));
    case RaceAttribute::Fight:
      return static_cast<double>(spec.fighters);
    case RaceAttribute::Pods:
      return spec.pods ? 1.0 : 0.0;
    case RaceAttribute::Mass:
      return spec.mass;
    case RaceAttribute::Sexes:
      return static_cast<double>(spec.number_sexes);
    case RaceAttribute::Metabolism:
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

constexpr std::array<int, 9> sector_count_costs = {
    -1, 0, 50, 100, 200, 300, 400, 500, 600,
};

double sector_compat_covariance(SectorType a, SectorType b) {
  if (a > b) {
    std::swap(a, b);
  }
  // TODO(C++26): Use std::inplace_vector when it lands in libc++ and make
  // constexpr when P3372 (constexpr containers and adaptors) lands.
  static const std::flat_map<SectorType, SectorCompatibilities> cov_map = {
      {SectorType::SEC_LAND, {.sea = 0.001, .plated = 0.0}},
      {SectorType::SEC_MOUNT, {.sea = 0.002, .land = -0.0005, .plated = 0.0}},
      {SectorType::SEC_GAS,
       {.sea = 999.0, .land = 999.0, .mount = 999.0, .plated = 0.0}},
      {SectorType::SEC_ICE,
       {.sea = 0.001, .mount = -0.002, .gas = 999.0, .plated = 0.0}},
      {SectorType::SEC_FOREST,
       {.land = -0.001, .gas = 999.0, .ice = 0.001, .plated = 0.0}},
      {SectorType::SEC_DESERT,
       {.sea = 0.003,
        .land = -0.0005,
        .gas = 999.0,
        .forest = 0.001,
        .plated = 0.0}},
      {SectorType::SEC_PLATED, {.gas = 999.0, .plated = 0.0}},
  };
  if (auto it = cov_map.find(b); it != cov_map.end()) {
    return it->second[a];
  }
  return 0.0;
}

}  // namespace

RacegenEngine::RacegenEngine() {
  base_attr_[std::to_underlying(RaceAttribute::Adventurism)] = {
      .e_factor = 0.0,
      .e_fudge = 0.0,
      .e_hinge = 0.0,
      .l_factor = 300.0,
      .minimum = 0.05,
      .init = 0.4,
      .maximum = 0.99,
      .is_integral = 0};
  base_attr_[std::to_underlying(RaceAttribute::Absorb)] = {.e_factor = 0.0,
                                                           .e_fudge = 0.0,
                                                           .e_hinge = 0.0,
                                                           .l_factor = 200.0,
                                                           .minimum = 0.0,
                                                           .init = 0.0,
                                                           .maximum = 1.0,
                                                           .is_integral = 2};
  base_attr_[std::to_underlying(RaceAttribute::Birthrate)] = {.e_factor = 0.0,
                                                              .e_fudge = 0.0,
                                                              .e_hinge = 0.0,
                                                              .l_factor = 500.0,
                                                              .minimum = 0.2,
                                                              .init = 0.6,
                                                              .maximum = 1.0,
                                                              .is_integral = 0};
  base_attr_[std::to_underlying(RaceAttribute::CollectiveIQ)] = {
      .e_factor = 0.0,
      .e_fudge = 0.0,
      .e_hinge = 0.0,
      .l_factor = -350.5,
      .minimum = 0.0,
      .init = 0.0,
      .maximum = 1.0,
      .is_integral = 2};
  base_attr_[std::to_underlying(RaceAttribute::Fertilize)] = {.e_factor = 200.0,
                                                              .e_fudge = 1.0,
                                                              .e_hinge = 1.0,
                                                              .l_factor = 300.0,
                                                              .minimum = 0.0,
                                                              .init = 0.0,
                                                              .maximum = 1.0,
                                                              .is_integral = 0};
  base_attr_[std::to_underlying(RaceAttribute::IQ)] = {.e_factor = 100.0,
                                                       .e_fudge = 0.03,
                                                       .e_hinge = 140.0,
                                                       .l_factor = 6.0,
                                                       .minimum = 50.0,
                                                       .init = 150.0,
                                                       .maximum = 220.0,
                                                       .is_integral = 1};
  base_attr_[std::to_underlying(RaceAttribute::Fight)] = {.e_factor = 10.0,
                                                          .e_fudge = 0.4,
                                                          .e_hinge = 6.0,
                                                          .l_factor = 65.0,
                                                          .minimum = 1.0,
                                                          .init = 4.0,
                                                          .maximum = 20.0,
                                                          .is_integral = 1};
  base_attr_[std::to_underlying(RaceAttribute::Pods)] = {.e_factor = 0.0,
                                                         .e_fudge = 0.0,
                                                         .e_hinge = 0.0,
                                                         .l_factor = 200.0,
                                                         .minimum = 0.0,
                                                         .init = 0.0,
                                                         .maximum = 1.0,
                                                         .is_integral = 2};
  base_attr_[std::to_underlying(RaceAttribute::Mass)] = {.e_factor = 100.0,
                                                         .e_fudge = 1.0,
                                                         .e_hinge = 3.1,
                                                         .l_factor = -100.0,
                                                         .minimum = 0.1,
                                                         .init = 1.0,
                                                         .maximum = 3.0,
                                                         .is_integral = 0};
  base_attr_[std::to_underlying(RaceAttribute::Sexes)] = {.e_factor = 2.2,
                                                          .e_fudge = -0.5,
                                                          .e_hinge = 9.0,
                                                          .l_factor = -3.0,
                                                          .minimum = 1.0,
                                                          .init = 2.0,
                                                          .maximum = 53.0,
                                                          .is_integral = 1};
  base_attr_[std::to_underlying(RaceAttribute::Metabolism)] = {
      .e_factor = 300.0,
      .e_fudge = 1.0,
      .e_hinge = 1.3,
      .l_factor = 700.0,
      .minimum = 0.1,
      .init = 1.0,
      .maximum = 4.0,
      .is_integral = 0};

  auto attr_range = [this](RaceAttribute idx) {
    auto i = std::to_underlying(idx);
    return base_attr_[i].maximum - base_attr_[i].minimum;
  };

  const auto adv_idx = std::to_underlying(RaceAttribute::Adventurism);
  const auto brt_idx = std::to_underlying(RaceAttribute::Birthrate);
  const auto col_idx = std::to_underlying(RaceAttribute::CollectiveIQ);
  const auto frt_idx = std::to_underlying(RaceAttribute::Fertilize);
  const auto iq_idx = std::to_underlying(RaceAttribute::IQ);
  const auto fgt_idx = std::to_underlying(RaceAttribute::Fight);
  const auto mas_idx = std::to_underlying(RaceAttribute::Mass);
  const auto sex_idx = std::to_underlying(RaceAttribute::Sexes);
  const auto met_idx = std::to_underlying(RaceAttribute::Metabolism);

  // Normal race covariances
  normal_cov_[adv_idx][iq_idx] = -0.40 / attr_range(RaceAttribute::IQ);
  normal_cov_[brt_idx][iq_idx] = 0.20 / attr_range(RaceAttribute::IQ);
  normal_cov_[brt_idx][mas_idx] = 0.40 / attr_range(RaceAttribute::Mass);
  normal_cov_[brt_idx][sex_idx] = 0.90 / attr_range(RaceAttribute::Sexes);
  normal_cov_[brt_idx][met_idx] = -0.20 / attr_range(RaceAttribute::Metabolism);
  normal_cov_[frt_idx][met_idx] = -0.20 / attr_range(RaceAttribute::Metabolism);
  normal_cov_[fgt_idx][iq_idx] = -0.20 / attr_range(RaceAttribute::IQ);
  normal_cov_[fgt_idx][adv_idx] =
      -0.05 / attr_range(RaceAttribute::Adventurism);
  normal_cov_[fgt_idx][mas_idx] = -0.20 / attr_range(RaceAttribute::Mass);
  normal_cov_[fgt_idx][met_idx] = -0.05 / attr_range(RaceAttribute::Metabolism);
  normal_cov_[met_idx][mas_idx] = 0.15 / attr_range(RaceAttribute::Mass);
  normal_cov_[met_idx][iq_idx] = -0.10 / attr_range(RaceAttribute::IQ);
  normal_cov_[iq_idx][mas_idx] = -0.25 / attr_range(RaceAttribute::Mass);

  // Metamorph race covariances
  morph_cov_[adv_idx][iq_idx] = 0.0;
  morph_cov_[brt_idx][mas_idx] = 0.10 / attr_range(RaceAttribute::Mass);
  morph_cov_[brt_idx][sex_idx] = 0.50 / attr_range(RaceAttribute::Sexes);
  morph_cov_[brt_idx][met_idx] = -0.10 / attr_range(RaceAttribute::Metabolism);
  morph_cov_[frt_idx][met_idx] = -0.30 / attr_range(RaceAttribute::Metabolism);
  morph_cov_[fgt_idx][adv_idx] = -0.10 / attr_range(RaceAttribute::Adventurism);
  morph_cov_[fgt_idx][mas_idx] = -0.20 / attr_range(RaceAttribute::Mass);
  morph_cov_[fgt_idx][met_idx] = -0.15 / attr_range(RaceAttribute::Metabolism);
  morph_cov_[met_idx][mas_idx] = 0.05 / attr_range(RaceAttribute::Mass);
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
  spec.home_planet_type = PlanetType::EARTH;
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
  spec.number_sexes = metamorph ? 1 : 2;
  spec.metabolism = 1.0;

  spec.sector_compatibilities = {.plated = 1.0};
  spec.preferred_sector = std::nullopt;

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
    const auto attr_idx = static_cast<RaceAttribute>(i);
    const double val = get_attribute_value(spec, attr_idx);
    double cost =
        std::exp(p.e_fudge * (val - p.e_hinge)) * p.e_factor + val * p.l_factor;
    for (std::size_t j = 0; j < num_race_attributes; ++j) {
      if (cov[i][j] != 0.0) {
        cost *= (1.0 + cov[i][j] * get_attribute_value(
                                       spec, static_cast<RaceAttribute>(j)));
      }
    }
    cost += p.l_fudge;
    breakdown.attribute_costs[i] = static_cast<int>(std::lround(cost));
    sum += breakdown.attribute_costs[i];
  }

  // 2. Planet and race type costs
  breakdown.planet_cost = get_planet_cost(spec.home_planet_type).value_or(0);
  sum += breakdown.planet_cost;
  breakdown.race_type_cost = 0;
  sum += breakdown.race_type_cost;

  // 3. Sector compatibility costs
  SectorCompatibilities raw_sector_costs{.plated = 0.0};
  std::size_t sector_types_count = 0;
  for (SectorType st : settleable_sector_types) {
    const double compat_pct = spec.sector_compatibilities[st] * 100.0;
    if (compat_pct > 0.0) {
      ++sector_types_count;
    }
    raw_sector_costs[st] = compat_pct * 0.5 + 10.8 * std::log(1.0 + compat_pct);
  }

  for (SectorType st_i : settleable_sector_types) {
    for (SectorType st_j : settleable_sector_types) {
      if (st_j <= st_i) continue;
      const double cov = sector_compat_covariance(st_i, st_j);
      if (cov != 0.0) {
        const double compat_i = spec.sector_compatibilities[st_i] * 100.0;
        const double compat_j = spec.sector_compatibilities[st_j] * 100.0;
        raw_sector_costs[st_i] *= (1.0 + cov * compat_j);
        raw_sector_costs[st_j] *= (1.0 + cov * compat_i);
      }
    }
  }

  for (SectorType st : settleable_sector_types) {
    const double multiplier =
        Planet::compat_cost_multiplier(spec.home_planet_type, st);
    if (multiplier > 1.01) {
      raw_sector_costs[st] *= multiplier;
    }
  }

  for (SectorType st : settleable_sector_types) {
    breakdown.sector_costs[st] =
        static_cast<int>(std::lround(raw_sector_costs[st]));
    sum += breakdown.sector_costs[st];
  }
  breakdown.sector_costs.wasted = 0;

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
    const auto& p = base_attr_[i];
    const auto attr_idx = static_cast<RaceAttribute>(i);
    const double val = get_attribute_value(spec, attr_idx);
    const std::string name = attribute_name(spec, attr_idx);

    if (val < p.minimum) {
      errors.push_back(
          std::format("{} must be at least {:.2f}.", name, p.minimum));
    }
    if (val > p.maximum) {
      errors.push_back(
          std::format("{} may be at most {:.2f}.", name, p.maximum));
    }
  }

  // 2. Normal vs. Metamorph race characteristics
  if (!spec.metamorph) {
    if (spec.absorb) {
      errors.push_back("Normal races do not absorb their enemies in combat.");
    }
    if (spec.pods) {
      errors.push_back("Normal races do not make pods.");
    }
  } else if (spec.number_sexes != 1) {
    errors.push_back("Metamorphs are required to have one sex.");
  }

  // 3. Privileges
  if (is_player && spec.is_guest) {
    errors.push_back("Players may not create non-normal races.");
  }

  // 4. Home planet validity
  if (!is_habitable_home_planet(spec.home_planet_type)) {
    errors.push_back("Home planet type out of valid range.");
  }

  // 5. Sector compatibilities
  const bool is_jovian = (spec.home_planet_type == PlanetType::GASGIANT);

  if (!is_jovian && spec.sector_compatibilities.plated != 1.0) {
    errors.push_back("Non-jovian races must have 100% plated compat.");
  }

  for (auto [st, compat] : spec.sector_compatibilities.settleable()) {
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

  if (spec.preferred_sector == SectorType::SEC_PLATED ||
      spec.preferred_sector == SectorType::SEC_WASTED) {
    errors.push_back(
        "Preferred sector must be a natural habitable terrain type.");
  }

  if ((!spec.password.empty() && !Race::is_valid_password(spec.password)) ||
      (!spec.governor_password.empty() &&
       !Race::is_valid_password(spec.governor_password))) {
    errors.push_back("Passwords cannot contain whitespace.");
  }

  // 6. Rigorous checking
  if (rigorous) {
    if (spec.name.empty()) {
      errors.push_back("Use a non-empty name.");
    }

    if (!Race::is_valid_password(spec.password) ||
        spec.password.length() < MIN_PASSWORD_LENGTH) {
      errors.push_back(std::format(
          "Passwords are required to be at least {} characters long.",
          MIN_PASSWORD_LENGTH));
    }

    if (!Race::is_valid_password(spec.governor_password)) {
      errors.push_back(
          "Governor password must be non-empty and contain no whitespace.");
    }

    if (spec.address.empty()) {
      errors.push_back("You must change your email address.");
    }

    const auto breakdown = calculate_cost(spec);
    if (breakdown.points_remaining < 0) {
      errors.push_back(std::format("You can't have negative points left! "
                                   "(exceeds point budget by {} points)",
                                   -breakdown.points_remaining));
    }

    std::size_t count = 0;
    for (auto [st, compat] : spec.sector_compatibilities.settleable()) {
      if (compat > 0.0) {
        ++count;
      }
    }

    if (!is_jovian && count == 1) {
      errors.push_back(
          "Non-jovian races must be compat with at least one sector type "
          "besides plated.");
    }

    bool has_common_sector = false;
    for (auto [st, compat] : spec.sector_compatibilities.settleable()) {
      if (Planet::is_common_sector(spec.home_planet_type, st) &&
          compat == 1.0) {
        has_common_sector = true;
        break;
      }
    }
    if (!has_common_sector) {
      errors.push_back(
          "You must have 100% compat with at least one sector type that is "
          "common on your home planet type.");
    }

    if (spec.preferred_sector.has_value() &&
        spec.preferred_sector != SectorType::SEC_PLATED &&
        spec.preferred_sector != SectorType::SEC_WASTED &&
        spec.sector_compatibilities[*spec.preferred_sector] != 1.0) {
      errors.push_back("Preferred sector must have 100% compatibility.");
    }
  }

  return errors;
}

}  // namespace GB::creator
