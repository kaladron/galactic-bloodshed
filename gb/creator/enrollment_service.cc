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

namespace GB::creator {

EnrollmentService::EnrollmentService(EntityManager& em) : entity_manager_(em) {}

std::optional<std::pair<starnum_t, planetnum_t>>
EnrollmentService::find_suitable_planet(PlanetType ppref,
                                        std::span<const starnum_t> star_order) {
  auto check_star = [&](starnum_t star, const Star& star_data)
      -> std::optional<std::pair<starnum_t, planetnum_t>> {
    /* skip over inhabited stars - or stars with just one planet! */
    if (star_data.inhabited() != 0 || star_data.numplanets() < 2) {
      return std::nullopt;
    }

    for (const Planet& planet :
         PlanetList::readonly(entity_manager_, star, star_data)) {
      if (planet.type() == ppref) {
        bool vacant = true;
        for (player_t p : all_players()) {
          if (planet.info(p).numsectsowned > 0) {
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

  const auto* univ = entity_manager_.peek_universe();
  int numstars = univ->numstars;

  for (auto star : star_order) {
    if (star < 0 || star >= numstars) continue;
    const auto* star_ptr = entity_manager_.peek_star(star);
    if (!star_ptr) continue;
    auto res = check_star(star, *star_ptr);
    if (res) return res;
  }
  return std::nullopt;
}

EnrollmentResult
EnrollmentService::enroll_player(const RaceEnrollmentSpec& spec) {
  // 1. Check player count limit
  player_t playernum{entity_manager_.num_races().value + 1};
  if (playernum >= MAXPLAYERS) {
    return EnrollmentResult{
        .success = false,
        .message = std::format("There are already {} players; No more allowed.",
                               MAXPLAYERS - 1),
    };
  }

  // 2. Check God requirement for player 1
  if (playernum == 1 && !spec.is_god) {
    return EnrollmentResult{
        .success = false,
        .message = "The first race enrolled must have God privileges.",
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
              Planet_types[spec.home_planet_type]),
      };
    }
    star = found_loc->first;
    pnum = found_loc->second;
  }

  // 4. Determine preferred sector and capital coordinates
  SectorType pref = spec.likesbest.value_or(
      spec.preferred_sector.value_or(SectorType::SEC_LAND));
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

  // 5. Build race entity
  Race race{};
  race.Playernum = playernum;
  race.God = spec.is_god;
  race.Guest = spec.is_guest;
  race.name = spec.name;
  race.password = spec.password;
  race.info = spec.address;

  // Governor 0 is designated as the race Leader.
  // Note: Governors 1 through MAXGOVERNORS are value-initialized to inactive
  // (active = false) by Race::gov in-class member initializers on `Race
  // race{};`.
  race.governor[0].name = "Leader";
  race.governor[0].password = spec.governor_password;
  race.governor[0].homelevel = race.governor[0].deflevel =
      ScopeLevel::LEVEL_PLAN;
  race.governor[0].homesystem = race.governor[0].defsystem = star;
  race.governor[0].homeplanetnum = race.governor[0].defplanetnum = pnum;
  race.governor[0].toggle.highlight = playernum;
  race.governor[0].toggle.inverse = true;
  race.governor[0].toggle.color = false;
  race.governor[0].active = true;

  // Conditions copied from home planet
  entity_manager_.with_planet(star, pnum, [&](const Planet& p) {
    for (Conditions c : all_atmosphere_conditions) {
      race.conditions[c] = p.conditions(c);
    }
  });

  // Translation matrix
  for (player_t p : all_players()) {
    if (p == playernum || playernum == 1 || race.God) {
      race.translate[p] = 100;
    } else {
      race.translate[p] = 1;
    }
  }

  // Racial characteristics
  race.mass = spec.mass;
  race.birthrate = spec.birthrate;
  race.fighters = spec.fighters;
  race.IQ = spec.iq;
  race.IQ_limit = spec.iq_limit;
  race.Metamorph = spec.metamorph;
  race.absorb = spec.absorb;
  race.collective_iq = spec.collective_iq;
  race.pods = spec.pods;
  race.adventurism = spec.adventurism;
  race.number_sexes = spec.number_sexes;
  race.metabolism = spec.metabolism;
  race.fertilize = spec.fertilize;

  // Sector preferences
  for (SectorType st : all_sector_types) {
    race.likes[st] = spec.sector_compatibilities[st];
  }
  race.likesbest = pref;

  race.discoveries = {};
  race.tech = 0.0;
  race.morale = 0;
  race.turn = 0;
  race.allied = 0;
  race.atwar = 0;
  race.points.fill(0);

  // 6. Build and dock capital government ship
  auto ship_handle =
      entity_manager_.create_ship(ShipType::OTYPE_GOV, playernum);
  Ship& ss = *ship_handle;
  entity_manager_.with_star(star, [&](const Star& s) {
    entity_manager_.with_planet(star, pnum, [&](const Planet& p) {
      ss.set_coordinates(p.absolute_coordinates(s));
    });
  });
  ss.set_land_coords(capital_coords);
  ss.race() = playernum;
  ss.tech() = 100.0;

  const auto& gov_tmpl = ship_template(ShipType::OTYPE_GOV);
  ss.set_secondary_battery(gov_tmpl.max_guns,
                           shipdata_secondary(ShipType::OTYPE_GOV));
  ss.shipclass() = "Standard";
  ss.popn() = gov_tmpl.max_crew;
  ss.set_mass(ss.base_mass() + gov_tmpl.max_crew * race.mass);
  ss.protect().self = 1;
  ss.land_on_planet();
  ss.deststar() = star;
  ss.destpnum() = pnum;
  ss.storbits() = star;
  ss.pnumorbits() = pnum;
  ss.on() = 1;

  shipnum_t shipno = ss.number();
  race.Gov_ship = shipno;

  // 7. Mutate Sector and Planet
  entity_manager_.mutate_sectormap(star, pnum, [&](SectorMap& smap) {
    entity_manager_.mutate_planet(star, pnum, [&](Planet& planet) {
      auto& sect = smap.get(capital_coords);
      sect.colonize(playernum, race.number_sexes, playernum);
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

  // 8. Save race and auto-seed baseline alliance block and power records
  entity_manager_.create_race(race);

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
