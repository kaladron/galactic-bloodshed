// SPDX-License-Identifier: Apache-2.0

/// \file visibility.cc
/// \brief Planetary surface map rendering and sector character formatting.

module;

import std;

module gb.mechanics;

namespace {

char format_troop_sector_char(player_t playernum, const Race& r,
                              const Sector& s) {
  if (s.get_owner() == playernum) return CHAR_MY_TROOPS;
  if (r.is_allied_with(s.get_owner())) return CHAR_ALLIED_TROOPS;
  if (r.is_at_war_with(s.get_owner())) return CHAR_ATWAR_TROOPS;
  return CHAR_NEUTRAL_TROOPS;
}

std::optional<char> format_owned_sector_digit(const Race::gov& gov,
                                              const Sector& s) {
  if (s.get_owner() == 0 || gov.toggle.geography) {
    return std::nullopt;
  }
  if (gov.toggle.inverse && s.get_owner() == gov.toggle.highlight) {
    return std::nullopt;
  }
  const int owner_val = s.get_owner().value;
  if (!gov.toggle.double_digits || owner_val < 10 || (s.coords().x % 2) != 0) {
    return static_cast<char>((owner_val % 10) + '0');
  }
  return static_cast<char>((owner_val / 10) + '0');
}

struct LandedShipGrid {
  bool has_visual_iq{false};
  std::flat_map<Coordinates, char> shiplocs{};

  [[nodiscard]] std::optional<char> ship_at(Coordinates c) const noexcept {
    if (const auto it = shiplocs.find(c); it != shiplocs.end()) {
      return it->second;
    }
    return std::nullopt;
  }
};

LandedShipGrid scan_planet_ships_for_map(EntityManager& em, starnum_t snum,
                                         planetnum_t pnum, const Planet& p,
                                         const SectorMap& smap,
                                         player_t playernum,
                                         governor_t governor,
                                         const Race& race) {
  LandedShipGrid grid{};
  if (race.governor(governor).toggle.geography) {
    return grid;
  }
  grid.has_visual_iq = p.info(playernum).numsectsowned > 0;
  for (const Ship& s : ShipList::readonly_on_planet(em, snum, pnum)) {
    if (s.owner() == playernum && s.is_authorized_for(governor) &&
        s.has_sight()) {
      grid.has_visual_iq = true;
    }
    if (s.is_landed() && smap.in_bounds(s.land_coords())) {
      grid.shiplocs[s.land_coords()] = s.type_letter();
    }
  }
  return grid;
}

PlanetMapCell build_map_sector_cell(player_t playernum, governor_t governor,
                                    const Race& race, const Sector& sector,
                                    std::optional<char> ship_glyph,
                                    bool has_visual_iq) {
  const auto& toggle = race.governor(governor).toggle;
  const char display_char = (ship_glyph && has_visual_iq)
                                ? *ship_glyph
                                : desshow(playernum, governor, race, sector);
  const bool inverse =
      (sector.get_owner() == toggle.highlight && toggle.inverse);
  return PlanetMapCell{
      .coords = sector.coords(),
      .owner = sector.get_owner(),
      .glyph = display_char,
      .inverse = inverse,
  };
}

std::vector<PlanetAlienPresence>
collect_planet_aliens(const Planet& p, player_t playernum, const Race& race) {
  std::vector<PlanetAlienPresence> aliens;
  if (!p.explored() && race.tech < TECH_EXPLORE) {
    return aliens;
  }
  for (const auto& [i, info] : p.info_map()) {
    if (info.numsectsowned != 0 && i != playernum) {
      aliens.push_back(PlanetAlienPresence{
          .player = i,
          .at_war = race.is_at_war_with(i),
      });
    }
  }
  return aliens;
}

}  // namespace

PlanetMapViewModel build_planet_map(EntityManager& em, const starnum_t snum,
                                    const planetnum_t pnum, const Planet& p,
                                    const player_t playernum,
                                    const governor_t governor,
                                    const Race& race) {
  const auto& smap = *em.peek_sectormap(snum, pnum);
  const LandedShipGrid grid = scan_planet_ships_for_map(
      em, snum, pnum, p, smap, playernum, governor, race);

  std::vector<PlanetMapCell> sectors;
  sectors.reserve(static_cast<std::size_t>(smap.num_sectors()));
  for (const Sector& sector : smap) {
    sectors.push_back(build_map_sector_cell(playernum, governor, race, sector,
                                            grid.ship_at(sector.coords()),
                                            grid.has_visual_iq));
  }

  const auto& star = *em.peek_star(snum);
  const auto& pinfo = p.info(playernum);
  const bool aliens_unknown = (!p.explored() && race.tech < TECH_EXPLORE);
  const population_t effective_maxpopn = static_cast<population_t>(
      std::lround(0.01 * (100.0 - p.toxic()) * p.maxpopn()));

  return PlanetMapViewModel{
      .planet_name = star.get_planet_name(pnum),
      .dimensions = p.dimensions(),
      .sectors = std::move(sectors),
      .planet_type_name = std::string(p.type_name()),
      .is_metamorph = race.Metamorph,
      .sectors_owned = pinfo.numsectsowned,
      .aliens_unknown = aliens_unknown,
      .aliens = collect_planet_aliens(p, playernum, race),
      .guns = pinfo.guns,
      .mob_points = pinfo.mob_points,
      .comread = pinfo.comread,
      .mob_set = pinfo.mob_set,
      .compatibility = p.compatibility(race),
      .toxicity = p.toxic(),
      .resource_stockpile = pinfo.resource,
      .fuel_stockpile = pinfo.fuel,
      .destruct_cap = pinfo.destruct,
      .player_popn = pinfo.popn,
      .total_popn = p.popn(),
      .effective_maxpopn = effective_maxpopn,
      .crystals = pinfo.crystals,
      .player_troops = pinfo.troops,
      .total_troops = p.troops(),
      .total_resources = p.total_resources(),
      .tax = pinfo.tax,
      .newtax = pinfo.newtax,
      .est_production = pinfo.est_production,
      .slaved_to = p.slaved_to(),
      .primary_unstable = star.stability() > 50,
  };
}

char desshow(const player_t Playernum, const governor_t Governor, const Race& r,
             const Sector& s) {
  const auto& gov = r.governor(Governor);
  if (s.get_troops() && !gov.toggle.geography) {
    return format_troop_sector_char(Playernum, r, s);
  }
  if (const auto digit = format_owned_sector_digit(gov, s)) {
    return *digit;
  }
  if (s.get_crystals() && (r.discoveries.crystal || r.God)) {
    return CHAR_CRYSTAL;
  }
  return s.condition_symbol();
}
