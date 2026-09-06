// SPDX-License-Identifier: Apache-2.0

/// \file GB_racegen.cc
/// \brief Helper functions for race generation and enrollment.

import std;
import dallib;
import gb.entities;
import gb.services;
import gb.creator;

#include "gb/creator/racegen.h"

namespace {
constexpr std::array<PlanetType, N_HOME_PLANET_TYPES> planet_translate = {
    PlanetType::EARTH,   PlanetType::FOREST, PlanetType::DESERT,
    PlanetType::WATER,   PlanetType::MARS,   PlanetType::ICEBALL,
    PlanetType::GASGIANT};

std::string racegen_db_path = PKGSTATEDIR "gb.db";
}  // namespace

void set_racegen_db_path(std::string_view path) {
  racegen_db_path = path;
}

const std::string& get_racegen_db_path() {
  return racegen_db_path;
}

int enroll_valid_race(Database& database);

/*
 * Returns 0 if successfully enrolled, or 1 if failure. */
int enroll_valid_race() {
  Database database{racegen_db_path};
  return enroll_valid_race(database);
}

/*
 * Returns 0 if successfully enrolled, or 1 if failure. */
int enroll_valid_race(Database& database) {
  EntityManager entity_manager{database};
  GB::creator::EnrollmentService service{entity_manager, database};

  std::cout << std::format("Looking for {}..",
                           planet_print_name[race_info.home_planet_type]);

  GB::creator::RaceEnrollmentSpec spec{};
  spec.name = race_info.name;
  spec.password = race_info.password;
  spec.is_god = (race_info.priv_type == P_GOD);
  spec.is_guest = (race_info.priv_type == P_GUEST);
  spec.home_planet_type = planet_translate[race_info.home_planet_type];

  // Assign racial characteristics
  spec.absorb = (race_info.attr[ABSORB] != 0.0);
  spec.collective_iq = (race_info.attr[COL_IQ] != 0.0);
  spec.metamorph = (race_info.race_type == R_METAMORPH);
  spec.pods = (race_info.attr[PODS] != 0.0);

  spec.fighters = static_cast<fighters_t>(race_info.attr[FIGHT]);
  if (race_info.attr[COL_IQ] == 1.0) {
    spec.iq_limit = static_cast<iq_t>(race_info.attr[A_IQ]);
    spec.iq = 0;
  } else {
    spec.iq = static_cast<iq_t>(race_info.attr[A_IQ]);
    spec.iq_limit = 0;
  }
  spec.number_sexes = static_cast<sexes_t>(race_info.attr[SEXES]);
  spec.fertilize = static_cast<fertilize_t>(race_info.attr[FERT] * 100);
  spec.adventurism = race_info.attr[ADVENT];
  spec.birthrate = race_info.attr[BIRTH];
  spec.mass = race_info.attr[MASS];
  spec.metabolism = race_info.attr[METAB];

  // Assign sector compats and determine a primary sector type.
  for (SectorType st : all_sector_types) {
    if (st == SectorType::SEC_WASTED) {
      spec.sector_compatibilities[st] = 0.0;
      continue;
    }
    spec.sector_compatibilities[st] = race_info.compat[st] / 100.0;
    if ((100 == race_info.compat[st]) &&
        (1.0 == planet_compat_cov[race_info.home_planet_type][st])) {
      spec.likesbest = st;
      spec.preferred_sector = st;
    }
  }

  auto result = service.enroll_player(spec);
  if (!result.success) {
    std::cout << " failed!\n";
    if (result.message.contains("Didn't find any free")) {
      race_info.rejection = std::format(
          "Didn't find any free {}; choose another home planet type.\n",
          planet_print_name[race_info.home_planet_type]);
    } else {
      race_info.rejection = result.message + "\n";
    }
    if (result.message.find("God privileges") == std::string::npos) {
      race_info.status = EnrollmentStatus::UNENROLLABLE;
    }
    return 1;
  }

  std::cout << " found!\n";

  entity_manager.with_star(result.star, [&](const Star& star_data) {
    std::cout << std::format(
        "Player {} ({}) created on sector {},{} on {}/{}.\n", result.player_num,
        race_info.name, result.capital_coords.x, result.capital_coords.y,
        star_data.get_name(), star_data.get_planet_name(result.pnum));
  });

  race_info.status = EnrollmentStatus::ENROLLED;
  return 0;
}
