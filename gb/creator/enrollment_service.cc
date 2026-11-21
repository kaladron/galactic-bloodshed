// SPDX-License-Identifier: Apache-2.0

/// \file enrollment_service.cc
/// \brief Domain service implementation for player empire enrollment.

module;

import dallib;
import gb.entities;
import gb.services;
import gb.repositories;
import std;

module gb.creator;

namespace {

SectorType
resolve_preferred_sector(const GB::creator::RaceEnrollmentSpec& spec) {
  const auto [default_pref, default_compat] =
      GB::creator::default_sector_compatibilities_for_planet(
          spec.home_planet_type);
  const SectorCompatibilities& likes =
      (spec.sector_compatibilities == SectorCompatibilities{})
          ? default_compat
          : spec.sector_compatibilities;

  if (spec.preferred_sector.has_value()) {
    return *spec.preferred_sector;
  }

  for (SectorType st : settleable_sector_types) {
    if (st == SectorType::SEC_PLATED) continue;
    if (Planet::is_common_sector(spec.home_planet_type, st) &&
        likes[st] == 1.0) {
      return st;
    }
  }
  for (SectorType st : settleable_sector_types) {
    if (st == SectorType::SEC_PLATED) continue;
    if (likes[st] == 1.0) {
      return st;
    }
  }
  return default_pref;
}

}  // namespace

namespace GB::creator {

EnrollmentService::EnrollmentService(EntityManager& em) : entity_manager_(em) {}

void EnrollmentService::seed_deity(std::string_view password,
                                   std::string_view gov_password) {
  if (!Race::is_valid_password(password)) {
    throw std::invalid_argument("seed_deity: deity race password must be "
                                "non-empty and contain no spaces");
  }
  if (!Race::is_valid_password(gov_password)) {
    throw std::invalid_argument("seed_deity: deity governor password must be "
                                "non-empty and contain no spaces");
  }

  auto [default_pref, default_compat] =
      default_sector_compatibilities_for_planet(PlanetType::EARTH);

  Race deity{};
  deity.Playernum = 1;
  deity.God = true;
  deity.Guest = false;
  deity.name = "Deity";
  deity.password = std::string(password);
  deity.turn = 0;
  deity.init_deity_leader(std::string(gov_password));
  deity.translate[player_t{1}] = 100;
  deity.mass = 1.0;
  deity.birthrate = 0.5;
  deity.fighters = 1;
  deity.IQ = 200;
  deity.number_sexes = 1;
  deity.metabolism = 1.0;
  deity.likes = default_compat;
  deity.likesbest = default_pref;
  deity.Gov_ship = std::nullopt;

  entity_manager_.create_race(deity);
}

std::optional<std::pair<starnum_t, planetnum_t>>
EnrollmentService::find_suitable_planet(PlanetType ppref,
                                        std::span<const starnum_t> star_order) {
  auto check_star = [&](starnum_t star, const Star& star_data)
      -> std::optional<std::pair<starnum_t, planetnum_t>> {
    /* skip over inhabited stars - or stars with just one planet! */
    if (star_data.is_inhabited() || star_data.numplanets() < 2) {
      return std::nullopt;
    }

    for (const Planet& planet :
         PlanetList::readonly(entity_manager_, star, star_data)) {
      if (planet.type() == ppref) {
        bool vacant = true;
        for (const auto& [_, info] : planet.info_map()) {
          if (info.numsectsowned > 0) {
            vacant = false;
            break;
          }
        }
        if (vacant) {
          return std::make_pair(star, planet.planet_order());
        }
      }
    }
    return std::nullopt;
  };

  if (star_order.empty()) {
    for (const Star& star_data : StarList::shuffle(entity_manager_)) {
      auto res = check_star(star_data.star_id(), star_data);
      if (res) return res;
    }
    return std::nullopt;
  }

  for (auto star : star_order) {
    const auto* star_ptr = entity_manager_.peek_star(star);
    auto res = check_star(star, *star_ptr);
    if (res) return res;
  }
  return std::nullopt;
}

Race EnrollmentService::build_race(player_t playernum,
                                   const RaceEnrollmentSpec& spec,
                                   starnum_t home_star,
                                   planetnum_t home_planet) {
  auto [default_pref, default_compat] =
      default_sector_compatibilities_for_planet(spec.home_planet_type);
  const SectorType pref = resolve_preferred_sector(spec);

  Race race{};
  race.Playernum = playernum;
  race.God = false;
  race.Guest = spec.is_guest;
  race.name = spec.name;
  race.password = spec.password;
  race.info = spec.address;

  // Governor 1 (Race::leader_id) is designated as the race Leader.
  race.turn = 0;
  race.init_leader(home_star, home_planet, spec.governor_password);

  // Self-translation starts at 100%; foreign translation entries are populated
  // sparsely upon contact or language discovery.
  race.translate[playernum] = 100;

  // Racial characteristics
  race.mass = spec.mass;
  race.birthrate = spec.birthrate;
  race.fighters = spec.fighters;
  const iq_t configured_iq = std::max(spec.iq, spec.iq_limit);
  if (spec.collective_iq) {
    race.IQ = 0;
    race.IQ_limit = configured_iq;
  } else {
    race.IQ = configured_iq;
    race.IQ_limit = 0;
  }
  race.Metamorph = spec.metamorph;
  race.absorb = spec.absorb;
  race.collective_iq = spec.collective_iq;
  race.pods = spec.pods;
  race.adventurism = spec.adventurism;
  race.number_sexes = spec.number_sexes;
  race.metabolism = spec.metabolism;
  race.fertilize = spec.fertilize;

  // Sector preferences
  race.likes = (spec.sector_compatibilities == SectorCompatibilities{})
                   ? default_compat
                   : spec.sector_compatibilities;
  race.likesbest = pref;

  race.discoveries = {};
  race.tech = 0.0;
  race.morale = 0;
  race.allied.clear();
  race.atwar.clear();
  race.Gov_ship = std::nullopt;
  return race;
}

EnrollmentResult
EnrollmentService::enroll_player(const RaceEnrollmentSpec& spec) {
  // 1. Assign next player ID
  player_t playernum{entity_manager_.max_race_player().value + 1};

  // 2. Validate racial specification and point budget
  RacegenEngine engine;
  auto errors = engine.validate(spec, /*is_player=*/!spec.is_guest,
                                /*rigorous=*/true);
  if (!errors.empty()) {
    std::string msg = "Cannot enroll: validation errors:";
    for (const auto& err : errors) {
      msg += "\n  - " + err;
    }
    return EnrollmentResult{
        .success = false,
        .message = std::move(msg),
    };
  }

  // 3. Find candidate planet
  starnum_t star{0};
  planetnum_t pnum{0};
  if (spec.target_planet.has_value()) {
    star = spec.target_planet->first;
    pnum = spec.target_planet->second;
  } else {
    auto found_loc =
        find_suitable_planet(spec.home_planet_type, spec.candidate_stars);
    if (!found_loc) {
      return EnrollmentResult{
          .success = false,
          .message = std::format(
              "Didn't find any free {}; choose another home planet type.",
              spec.home_planet_type),
      };
    }
    star = found_loc->first;
    pnum = found_loc->second;
  }

  // 4. Determine preferred sector and capital coordinates
  const SectorType pref = resolve_preferred_sector(spec);
  Coordinates capital_coords{0, 0};
  if (spec.capital_coords.has_value()) {
    capital_coords = *spec.capital_coords;
  } else {
    entity_manager_.with_sectormap(star, pnum, [&](const SectorMap& smap) {
      for (const Sector& sector : smap.shuffle()) {
        if (sector.is_colonizable_by(pref)) {
          capital_coords = sector.coords();
          return;
        }
      }
      for (const Sector& sector : smap.shuffle()) {
        if (sector.get_condition() == pref) {
          capital_coords = sector.coords();
          return;
        }
      }
      for (const Sector& sector : smap.shuffle()) {
        if (sector.get_condition() != SectorType::SEC_WASTED) {
          capital_coords = sector.coords();
          return;
        }
      }
      capital_coords = Coordinates{0, 0};
    });
  }

  // 5. Build and persist race first (with Gov_ship = std::nullopt) so parent
  // tbl_race(id) exists before child ship and sector foreign keys are inserted.
  Race race = build_race(playernum, spec, star, pnum);
  entity_manager_.with_planet(star, pnum, [&](const Planet& p) {
    race.temp = p.rtemp();
    for (AtmosphereConditions c : all_atmosphere_conditions) {
      race.conditions[c] = p.conditions(c);
    }
  });
  entity_manager_.create_race(race);

  // 6. Commission and land starting capital Government Center via shared Ship
  // construction lifecycle, overriding initial damage to 0 and tech to 100.
  shipnum_t shipno{0};
  {
    auto ship_handle =
        entity_manager_.create_ship(ShipType::OTYPE_GOV, playernum);
    Ship& ss = *ship_handle;
    ss.initialize_constructed_state(race, Race::leader_id, 0.0,
                                    ss.max_crew_capacity());
    ss.admin_override_damage(0);
    ss.tech() = 100.0;
    ss.shipclass() = "Standard";
    entity_manager_.with_star(star, [&](const Star& s) {
      entity_manager_.with_planet(star, pnum, [&](const Planet& p) {
        ss.set_coordinates(p.absolute_coordinates(s));
      });
    });
    ss.land_on_planet(star, pnum, capital_coords);
    ss.on() = 1;

    shipno = ss.number();
  }

  race.Gov_ship = shipno;
  entity_manager_.mutate_race(playernum, [&](Race& r) { r.Gov_ship = shipno; });

  // 8. Mutate Sector and Planet
  entity_manager_.mutate_sectormap(star, pnum, [&](SectorMap& smap) {
    entity_manager_.mutate_planet(star, pnum, [&](Planet& planet) {
      auto& sect = smap.get(capital_coords);
      if (!spec.capital_coords.has_value()) {
        sect.set_condition(race.likesbest);
      }
      sect.colonize(playernum, race.number_sexes);
      sect.set_fert(100);
      sect.set_efficiency_bounded(10);
      sect.set_troops(0);

      planet.info(playernum).numsectsowned = 1;
      planet.explored() = 0;
      planet.info(playernum).explored = 1;
      planet.popn() = race.number_sexes;
      planet.troops() = 0;
      planet.maxpopn() =
          maxsupport(race, sect, 100.0, 0) * planet.num_sectors() / 2;
    });
  });

  // 9. Mutate star
  entity_manager_.mutate_star(star, [&](Star& s) {
    s.mark_explored_by(playernum);
    s.mark_inhabited_by(playernum);
    s.AP(playernum) = 5;
  });

  return EnrollmentResult{
      .success = true,
      .player_num = playernum,
      .star = star,
      .pnum = pnum,
      .capital_coords = capital_coords,
      .gov_ship = shipno,
      .message = "Success",
  };
}

}  // namespace GB::creator
