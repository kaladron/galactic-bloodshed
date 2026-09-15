// SPDX-License-Identifier: Apache-2.0

/// \file racegen_test.cc
/// \brief Unit tests for RacegenEngine point calculations, covariances, and
/// validation.

import std;
import gb.creator;
import gb.entities;
import test;

namespace {

void test_default_race_costs() {
  GB::creator::RacegenEngine engine;
  auto default_spec = engine.create_default_spec(false);
  auto breakdown = engine.calculate_cost(default_spec);

  // All baseline attributes must cost exactly 0
  for (std::size_t i = 0; i < GB::creator::num_race_attributes; ++i) {
    test::expect_eq(breakdown.attribute_costs[i], 0.0,
                    "default attribute cost must be 0");
  }

  // Planet cost: Earth = 75
  test::expect_eq(breakdown.planet_cost, 75, "Earth planet cost must be 75");

  // Sector costs: Plated (100%) = 100
  test::expect_eq(breakdown.sector_costs.plated, 100.0,
                  "100% plated sector cost must be 100");

  // Sector count penalty: 1 type = 0
  test::expect_eq(breakdown.sector_count_cost, 0,
                  "1 sector type count cost must be 0");

  // Total cost: 75 (Earth) + 100 (Plated) = 175
  test::expect_eq(breakdown.total_cost, 175,
                  "default race total cost must be 175");
  test::expect_eq(breakdown.points_remaining, 1400 - 175,
                  "default race points remaining must be 1225");
}

void test_planet_costs() {
  GB::creator::RacegenEngine engine;
  auto spec = engine.create_default_spec(false);

  spec.home_planet_type = PlanetType::EARTH;
  test::expect_eq(engine.calculate_cost(spec).planet_cost, 75);

  spec.home_planet_type = PlanetType::FOREST;
  test::expect_eq(engine.calculate_cost(spec).planet_cost, 50);

  spec.home_planet_type = PlanetType::DESERT;
  test::expect_eq(engine.calculate_cost(spec).planet_cost, 50);

  spec.home_planet_type = PlanetType::WATER;
  test::expect_eq(engine.calculate_cost(spec).planet_cost, 50);

  spec.home_planet_type = PlanetType::MARS;
  test::expect_eq(engine.calculate_cost(spec).planet_cost, -25);

  spec.home_planet_type = PlanetType::ICEBALL;
  test::expect_eq(engine.calculate_cost(spec).planet_cost, -25);

  spec.home_planet_type = PlanetType::GASGIANT;
  test::expect_eq(engine.calculate_cost(spec).planet_cost, 600);
}

void test_normal_vs_metamorph_covariances() {
  GB::creator::RacegenEngine engine;

  auto normal_spec = engine.create_default_spec(false);
  auto morph_spec = engine.create_default_spec(true);

  // Baseline default attributes cost 0 in normal race
  auto normal_cost = engine.calculate_cost(normal_spec);
  test::expect_eq(normal_cost.iq(), 0.0);

  // In Metamorphs, pod, absorb, and collective IQ are enabled
  test::expect_true(morph_spec.metamorph,
                    "morph spec must be marked metamorph");
  test::expect_true(morph_spec.absorb);
  test::expect_true(morph_spec.pods);
  test::expect_true(morph_spec.collective_iq);

  // Increase mass and IQ together
  normal_spec.mass = 2.5;
  normal_spec.iq = 200;
  morph_spec.mass = 2.5;
  morph_spec.iq_limit = 200;

  auto normal_high = engine.calculate_cost(normal_spec);
  auto morph_high = engine.calculate_cost(morph_spec);

  // In normal races, high mass reduces IQ cost via negative covariance (-0.25 /
  // range). In metamorphs, IQ on mass covariance is 0.0.
  test::expect_true(normal_high.iq() < morph_high.iq(),
                    "normal high mass should yield lower IQ cost than "
                    "metamorph due to covariance");
}

void test_sector_compatibility_costs() {
  GB::creator::RacegenEngine engine;
  auto spec = engine.create_default_spec(false);
  spec.home_planet_type = PlanetType::WATER;

  // Add Water (SectorType::SEC_SEA = 0) at 100%
  spec.sector_compatibilities.sea = 1.0;

  auto breakdown = engine.calculate_cost(spec);
  // Sector count jumps from 1 (cost 0) to 2 (cost 50)
  test::expect_eq(breakdown.sector_count_cost, 50,
                  "2 sector types cost penalty must be 50");

  // Water on Water planet has planet_compat_cov == 1.00 (not > 1.01), so no
  // penalty multiplier
  test::expect_eq(breakdown.sector_costs.sea, 100.0,
                  "100% sea cost on water planet must be 100");

  // Desert on Water planet has planet_compat_cov == 3.00, costing 3x as much!
  spec.sector_compatibilities.desert = 0.5;
  auto desert_breakdown = engine.calculate_cost(spec);
  test::expect_true(desert_breakdown.sector_costs.desert > 100.0,
                    "desert on water world must be heavily penalized by planet "
                    "compatibility covariance");
}

void test_validation_rules() {
  GB::creator::RacegenEngine engine;
  auto spec = engine.create_default_spec(false);
  spec.name = "Terrans";
  spec.password = "secretpass";
  spec.address = "admin@earth.gov";
  // Add 100% Land compatibility (common on Earth)
  spec.sector_compatibilities.land = 1.0;

  // Standard non-rigorous validation should pass cleanly
  auto errors = engine.validate(spec, /*is_player=*/true, /*rigorous=*/false);
  test::expect_true(errors.empty(), "valid race should produce no errors");

  // Rigorous validation should also pass
  auto rigorous_errors =
      engine.validate(spec, /*is_player=*/true, /*rigorous=*/true);
  test::expect_true(rigorous_errors.empty(),
                    "valid complete race should pass rigorous validation");

  // 1. Attribute bounds check
  spec.adventurism = 0.01;  // Below minimum 0.05
  errors = engine.validate(spec, true, false);
  test::expect_true(!errors.empty() &&
                        errors[0].contains("Adventurism must be at least 0.05"),
                    "should reject adventurism below minimum");
  spec.adventurism = 0.4;

  // 2. Metamorph traits on normal race
  spec.absorb = true;
  errors = engine.validate(spec, true, false);
  test::expect_true(!errors.empty() &&
                        errors[0].contains("Normal races do not absorb"),
                    "should reject absorb on normal race");
  spec.absorb = false;

  // 3. Name validation
  spec.name = "";
  errors = engine.validate(spec, true, false);
  test::expect_true(!errors.empty() &&
                        errors[0].contains("Use a non-empty name"),
                    "should reject empty name");
  spec.name = "Terrans";

  // 4. Non-habitable planet (Asteroid)
  spec.home_planet_type = PlanetType::ASTEROID;
  errors = engine.validate(spec, true, false);
  test::expect_true(
      !errors.empty() &&
          errors[0].contains("Home planet type out of valid range"),
      "should reject asteroid as home planet");
  spec.home_planet_type = PlanetType::EARTH;

  // 5. Jovian sector restrictions
  spec.sector_compatibilities.gas = 0.5;
  errors = engine.validate(spec, true, false);
  test::expect_true(
      !errors.empty() &&
          errors[0].contains(
              "Non-jovian races may never have gas compatibility"),
      "should reject gas compatibility on non-Jovian planet");
  spec.sector_compatibilities.gas = 0.0;

  // 6. Rigorous: default password
  spec.password = "XXXX";
  errors = engine.validate(spec, true, true);
  test::expect_true(
      !errors.empty() &&
          errors[0].contains("change your password from the default"),
      "should reject default password XXXX in rigorous mode");
  spec.password = "validpass";

  // 7. Rigorous: default address
  spec.address = "Unknown";
  errors = engine.validate(spec, true, true);
  test::expect_true(!errors.empty() &&
                        errors[0].contains("change your email address"),
                    "should reject default address Unknown in rigorous mode");
  spec.address = "player@game.org";

  // 8. Rigorous: negative points remaining
  spec.iq = 220;
  spec.fighters = 20;
  spec.birthrate = 1.0;
  spec.metabolism = 4.0;
  spec.mass = 3.0;
  spec.sector_compatibilities = {.sea = 1.0,
                                 .land = 1.0,
                                 .mount = 1.0,
                                 .ice = 1.0,
                                 .forest = 1.0,
                                 .desert = 1.0,
                                 .plated = 1.0};
  errors = engine.validate(spec, true, true);
  test::expect_true(
      !errors.empty() &&
          errors[0].contains("You can't have negative points left"),
      "should reject negative remaining points in rigorous mode");
}

void test_archetypes_valid_and_within_budget() {
  GB::creator::RacegenEngine engine;

  for (std::size_t i = 0; i < GB::creator::race_archetypes.size(); ++i) {
    const auto& arch = GB::creator::race_archetypes[i];

    for (PlanetType ptype : habitable_planet_types) {
      // 1. Base archetype stats (randomize = false)
      auto base_spec = arch.to_enrollment_spec(ptype, /*randomize=*/false);
      base_spec.password = "validpass";
      base_spec.address = "player@galaxy.org";

      auto errors =
          engine.validate(base_spec, /*is_player=*/true, /*rigorous=*/true);
      test::expect_true(errors.empty(),
                        std::format("Archetype {} ({}) base spec on {} must "
                                    "pass rigorous validation",
                                    i + 1, arch.name, to_string(ptype)));

      auto cost = engine.calculate_cost(base_spec);
      test::expect_ge(
          cost.points_remaining, 0,
          std::format("Archetype {} ({}) base spec on {} must not exceed 1400 "
                      "points (remaining: {})",
                      i + 1, arch.name, to_string(ptype),
                      cost.points_remaining));

      // 2. Worst-case maximum random roll (+max_rand on every attribute)
      auto max_spec = base_spec;
      max_spec.mass = std::clamp(arch.base_mass + 0.025, 0.10, 3.00);
      max_spec.birthrate = std::clamp(arch.base_birthrate + 0.10, 0.20, 1.00);
      max_spec.fighters = static_cast<fighters_t>(
          std::clamp(static_cast<int>(arch.base_fighters) + 1, 1, 20));
      if (arch.is_metamorphic) {
        max_spec.iq_limit = static_cast<iq_t>(
            std::clamp(static_cast<int>(arch.base_iq_limit) + 10, 50, 220));
      } else {
        max_spec.iq = static_cast<iq_t>(
            std::clamp(static_cast<int>(arch.base_iq) + 10, 50, 220));
      }
      max_spec.adventurism =
          std::clamp(arch.base_adventurism + 0.10, 0.05, 0.99);
      max_spec.metabolism = std::clamp(arch.base_metabolism + 0.15, 0.10, 4.00);

      auto max_errors =
          engine.validate(max_spec, /*is_player=*/true, /*rigorous=*/true);
      test::expect_true(max_errors.empty(),
                        std::format("Archetype {} ({}) max roll on {} must "
                                    "pass rigorous validation",
                                    i + 1, arch.name, to_string(ptype)));

      auto max_cost = engine.calculate_cost(max_spec);
      test::expect_ge(
          max_cost.points_remaining, 0,
          std::format("Archetype {} ({}) max roll on {} must not exceed 1400 "
                      "points (remaining: {})",
                      i + 1, arch.name, to_string(ptype),
                      max_cost.points_remaining));
    }

    // 3. Random sample iterations on default planet
    for (int sample = 0; sample < 20; ++sample) {
      auto rand_spec =
          arch.to_enrollment_spec(std::nullopt, /*randomize=*/true);
      rand_spec.password = "validpass";
      rand_spec.address = "player@galaxy.org";

      auto rand_errors =
          engine.validate(rand_spec, /*is_player=*/true, /*rigorous=*/true);
      test::expect_true(rand_errors.empty(),
                        std::format("Archetype {} ({}) random roll must pass "
                                    "rigorous validation",
                                    i + 1, arch.name));
      auto rand_cost = engine.calculate_cost(rand_spec);
      test::expect_ge(rand_cost.points_remaining, 0,
                      std::format("Archetype {} ({}) random roll must not "
                                  "exceed 1400 points",
                                  i + 1, arch.name));
    }
  }
}

}  // namespace

int main() {
  test_default_race_costs();
  test_planet_costs();
  test_normal_vs_metamorph_covariances();
  test_sector_compatibility_costs();
  test_validation_rules();
  test_archetypes_valid_and_within_budget();

  std::println(std::cout, "✅ All RacegenEngine tests passed!");
  return 0;
}
