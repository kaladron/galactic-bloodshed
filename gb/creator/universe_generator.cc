// SPDX-License-Identifier: Apache-2.0

/// \file universe_generator.cc
/// \brief Procedural universe generation engine.

module;

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <numbers>
#include <string>
#include <utility>
#include <vector>

import std;
import dallib;
import gb.entities;
import gb.services;
import gb.repositories;

module gb.creator;

namespace GB::creator {

namespace {
constexpr double PLANET_DIST_MAX = 1900.0;
constexpr double PLANET_DIST_MIN = 100.0;

constexpr const char* PlanetTypeNames[] = {"Earth",   "Asteroid", "Airless",
                                           "Iceball", "Gaseous",  "Water",
                                           "Forest",  "Desert",   "Unknown"};

PlanetType roll_planet_type(int temperature) {
  int roll = int_rand(1, 100);
  if ((int_rand(1, 100) <= 10) || (temperature > 400)) {
    return PlanetType::ASTEROID;
  } else if ((temperature > 100) && (temperature <= 400)) {
    return (roll <= 60) ? PlanetType::MARS : PlanetType::DESERT;
  } else if ((temperature > 30) && (temperature <= 100)) {
    if (roll <= 25) return PlanetType::EARTH;
    if (roll <= 50) return PlanetType::WATER;
    if (roll <= 80) return PlanetType::FOREST;
    if (roll <= 90) return PlanetType::DESERT;
    return PlanetType::MARS;
  } else if ((temperature > -10) && (temperature <= 30)) {
    if (roll <= 45) return PlanetType::EARTH;
    if (roll <= 70) return PlanetType::WATER;
    if (roll <= 95) return PlanetType::FOREST;
    return PlanetType::DESERT;
  } else if ((temperature > -50) && (temperature <= -10)) {
    if (roll <= 30) return PlanetType::DESERT;
    if (roll <= 60) return PlanetType::ICEBALL;
    if (roll <= 90) return PlanetType::FOREST;
    return PlanetType::MARS;
  } else if ((temperature > -100) && (temperature <= -50)) {
    if (roll <= 50) return PlanetType::GASGIANT;
    if (roll <= 80) return PlanetType::ICEBALL;
    return PlanetType::MARS;
  } else {
    return (roll <= 80) ? PlanetType::ICEBALL : PlanetType::GASGIANT;
  }
}
}  // namespace

UniverseGenerator::UniverseGenerator(UniverseConfig config)
    : config_(std::move(config)) {}

void UniverseGenerator::set_star_names(std::vector<std::string> names) {
  star_names_ = std::move(names);
  star_indices_ = shuffled_indices(star_names_.size());
  star_name_cursor_ = 0;
}

void UniverseGenerator::set_planet_names(std::vector<std::string> names) {
  planet_names_ = std::move(names);
  planet_indices_ = shuffled_indices(planet_names_.size());
  planet_name_cursor_ = 0;
}

void UniverseGenerator::load_name_lists() {
  if (star_names_.empty() && config_.auto_name_stars) {
    std::ifstream f(config_.star_names_file);
    if (f.is_open()) {
      std::string line;
      while (std::getline(f, line) && star_names_.size() < 1000) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) {
          star_names_.push_back(line.substr(0, 19));
        }
      }
    }
    if (!star_names_.empty()) {
      star_indices_ = shuffled_indices(star_names_.size());
    }
  }

  if (planet_names_.empty() && config_.auto_name_planets) {
    std::ifstream f(config_.planet_names_file);
    if (f.is_open()) {
      std::string line;
      while (std::getline(f, line) && planet_names_.size() < 1000) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) {
          planet_names_.push_back(line.substr(0, 19));
        }
      }
    }
    if (!planet_names_.empty()) {
      planet_indices_ = shuffled_indices(planet_names_.size());
    }
  }
}

std::string UniverseGenerator::next_star_name(starnum_t snum) {
  if (config_.auto_name_stars && !star_names_.empty() &&
      star_name_cursor_ < star_indices_.size()) {
    return star_names_[star_indices_[star_name_cursor_++]];
  }
  return std::format("Star {}", snum.value + 1);
}

std::string UniverseGenerator::next_planet_name(planetnum_t pnum) {
  if (config_.auto_name_planets && !planet_names_.empty() &&
      planet_name_cursor_ < planet_indices_.size()) {
    return planet_names_[planet_indices_[planet_name_cursor_++]];
  }
  return std::format("{}", pnum.value + 1);
}

void UniverseGenerator::place_star(star_struct& star) {
  constexpr auto to_grid_bin = [](double coord) noexcept -> std::size_t {
    const double normalized = (coord + UNIVSIZE) / (2.0 * UNIVSIZE);
    return static_cast<std::size_t>(std::clamp(normalized * 100.0, 0.0, 99.0));
  };

  while (true) {
    const UniverseCoordinates pos{double_rand(-UNIVSIZE, UNIVSIZE),
                                  double_rand(-UNIVSIZE, UNIVSIZE)};
    const std::size_t i = to_grid_bin(pos.x);
    const std::size_t j = to_grid_bin(pos.y);
    if (!star_grid_occupancy_[i][j]) {
      star_grid_occupancy_[i][j] = true;
      star.xpos = pos.x;
      star.ypos = pos.y;
      return;
    }
  }
}

Star UniverseGenerator::make_star_system(Database& db, starnum_t snum,
                                         UniverseGenerationResult& result) {
  star_struct star{};
  star.star_id = snum;
  star.gravity = int_rand(0, int_rand(0, 300)) + int_rand(0, 300) +
                 int_rand(100, 400) + int_rand(0, 9) / 10.0;
  star.temperature = round_rand(star.gravity / 100.0);
  star.name = next_star_name(snum);
  place_star(star);

  if (config_.print_star_info) {
    std::println(std::cout, "Star {}: gravity {:.1f}, temp {}", star.name,
                 star.gravity, static_cast<int>(star.temperature));
  }

  int num_planets =
      int_rand(config_.min_planets.value, config_.max_planets.value);
  if (config_.planetless_chance_percent > 0 &&
      int_rand(1, 100) <= config_.planetless_chance_percent) {
    num_planets = 0;
  }
  star.pnames.reserve(num_planets);

  double distmin = PLANET_DIST_MIN;
  for (int i = 0; i < num_planets; ++i) {
    double distsep =
        (PLANET_DIST_MAX - distmin) / static_cast<double>(num_planets - i);
    double distmax = distmin + distsep;
    double dist = distmin + double_rand() * (distmax - distmin);
    distmin = dist;

    int temperature =
        calculate_temperature(dist, static_cast<int>(star.temperature));
    double angle = 2.0 * std::numbers::pi * double_rand();
    double xpos = dist * std::sin(angle);
    double ypos = dist * std::cos(angle);

    star.pnames.push_back(next_planet_name(static_cast<planetnum_t>(i)));

    PlanetType type = roll_planet_type(temperature);

    std::optional<SectorMap> smap_opt;
    auto planet = makeplanet(dist, static_cast<short>(star.temperature), type,
                             snum, static_cast<planetnum_t>(i), smap_opt);
    auto& smap = *smap_opt;

    planet.xpos() = xpos;
    planet.ypos() = ypos;
    planet.total_resources() = 0;

    result.planets_by_type[std::to_underlying(type)]++;

    if (config_.print_planet_info) {
      std::println(std::cout, "Planet {}: temp {}, type {} ({})",
                   star.pnames[i], planet.conditions(RTEMP),
                   PlanetTypeNames[std::to_underlying(planet.type())],
                   static_cast<unsigned int>(planet.type()));
      std::println(
          std::cout,
          "Position is ({:.0f},{:.0f}) relative to {}; distance {:.0f}.",
          planet.xpos(), planet.ypos(), star.name, dist);
      std::println(std::cout, "sect map({}x{}):", planet.dimensions().x,
                   planet.dimensions().y);
      for (int y = 0; y < planet.dimensions().y; ++y) {
        for (int x = 0; x < planet.dimensions().x; ++x) {
          std::cout << get_sector_char(
              smap.get(Coordinates{x, y}).get_condition());
        }
        std::cout << '\n';
      }
      std::cout << '\n';
    }

    for (const auto& sect : smap) {
      planet.total_resources() += sect.get_resource();
      result.total_resources += sect.get_resource();
    }

    JsonStore store(db);
    SectorRepository(store).save_map(smap);
    PlanetRepository(store).save(planet);
  }

  return star;
}

UniverseGenerationResult UniverseGenerator::generate(Database& db) {
  load_name_lists();
  initialize_schema(db);

  UniverseGenerationResult result{};
  result.num_stars = config_.num_stars;

  universe_struct universe_data{};
  universe_data.id = 1;
  universe_data.numstars = static_cast<int>(config_.num_stars.value);

  std::vector<Star> stars;
  stars.reserve(config_.num_stars.value);

  for (starnum_t snum = 0; snum < config_.num_stars; ++snum) {
    stars.push_back(make_star_system(db, snum, result));
  }

  universe_data.planet_count =
      static_cast<planetnum_t>(db.count_non_asteroid_planets());
  result.planet_count = universe_data.planet_count;

  JsonStore store(db);
  UniverseRepository universe_repo(store);
  universe_repo.save(universe_data);

  StarRepository star_repo(store);
  for (starnum_t snum = 0; snum < config_.num_stars; ++snum) {
    star_repo.save(stars[snum.value]);
  }

  BlockRepository block_repo(store);
  PowerRepository power_repo(store);
  for (int i : std::views::iota(0, MAXPLAYERS)) {
    power p{};
    p.id = i;
    power_repo.save(p);

    block b{};
    b.Playernum = i;
    block_repo.save(b);
  }

  ShipExamRepository exam_repo(store);
  exam_repo.seed_from_file(config_.exam_file);

  return result;
}

}  // namespace GB::creator
