// SPDX-License-Identifier: Apache-2.0

/// \file visibility.cc
/// \brief Planetary surface map rendering and sector character formatting.

module;

import std;

module gb.mechanics;

namespace {

// Base ASCII character ('?', 0x3F) used to pack a numeric player_t (0..64)
// into a single printable byte in the '$' planet map client-server wire
// protocol. Unowned (0) encodes as '?', Player 1 as '@', Player 2 as 'A', etc.,
// and clients decode the owner ID via (byte - '?').
constexpr char map_protocol_owner_base_char = '?';

char encode_map_protocol_owner(const Sector& sector) {
  return static_cast<char>(map_protocol_owner_base_char +
                           sector.get_owner().value);
}

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
  std::array<std::array<char, MAX_Y>, MAX_X> shiplocs{};
};

LandedShipGrid scan_planet_ships_for_map(EntityManager& em, starnum_t snum,
                                         planetnum_t pnum, const Planet& p,
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
        (s.popn() > 0 || s.type() == ShipType::OTYPE_PROBE)) {
      grid.has_visual_iq = true;
    }
    if (s.alive() && s.is_landed()) {
      const Coordinates land = s.land_coords();
      grid.shiplocs[land.x][land.y] = s.type_letter();
    }
  }
  return grid;
}

void output_map_sector_cell(GameObj& g, player_t playernum, governor_t governor,
                            const Race& race, const Sector& sector,
                            char ship_char, bool has_visual_iq) {
  const auto& toggle = race.governor(governor).toggle;
  const char display_char = (ship_char != '\0' && has_visual_iq)
                                ? ship_char
                                : desshow(playernum, governor, race, sector);
  const char highlight_prefix =
      (sector.get_owner() == toggle.highlight && toggle.inverse) ? '1' : '0';
  g.out << std::format("{}{}{}", highlight_prefix,
                       encode_map_protocol_owner(sector), display_char);
}

void show_planet_aliens(GameObj& g, const Planet& p, player_t playernum,
                        const Race& race) {
  if (!p.explored() && race.tech < TECH_EXPLORE) {
    g.out << "???";
    return;
  }
  bool found_alien = false;
  for (player_t i : all_players()) {
    if (p.info(i).numsectsowned != 0 && i != playernum) {
      found_alien = true;
      g.out << std::format("{}{}", race.is_at_war_with(i) ? '*' : ' ', i);
    }
  }
  if (!found_alien) {
    g.out << "(none)\n";
  }
}

void show_planet_stats(GameObj& g, const Planet& p, player_t playernum,
                       const Race& race) {
  const auto& pinfo = p.info(playernum);
  g.out << std::format(
      "Type: {:<8}   Sects {:<7}: {:<3}   Aliens:", p.type_name(),
      race.Metamorph ? "covered" : "owned", pinfo.numsectsowned);
  show_planet_aliens(g, p, playernum, race);
  g.out << "\n";
  g.out << std::format(
      "              Guns : {:<3}             Mob Points : {}\n", pinfo.guns,
      pinfo.mob_points);
  g.out << std::format(
      "      Mobilization : {:<3} ({:<3})     Compatibility: {:.2f}%",
      pinfo.comread, pinfo.mob_set, p.compatibility(race));
  if (p.toxic() > 50) {
    g.out << std::format("    ({}% TOXIC)\n", p.toxic());
  }
  g.out << "\n";
  g.out << std::format("Resource stockpile : {:<9}    Fuel stockpile: {}\n",
                       pinfo.resource, pinfo.fuel);
  g.out << std::format(
      "      Destruct cap : {:<9} {:>18}: {:<5} ({:<5}/{:<})\n", pinfo.destruct,
      race.Metamorph ? "Tons of biomass" : "Total Population", pinfo.popn,
      p.popn(), round_rand(.01 * (100. - p.toxic()) * p.maxpopn()));
  g.out << std::format("          Crystals : {:<9} {:>18}: {:<5} ({:<5})\n",
                       pinfo.crystals, "Ground forces", pinfo.troops,
                       p.troops());
  g.out << std::format("{} Total Resource Deposits     Tax rate {}%  New {}%\n",
                       p.total_resources(), pinfo.tax, pinfo.newtax);
  g.out << std::format("Estimated Production Next Update : {:.2f}\n",
                       pinfo.est_production);
  if (p.slaved_to()) {
    g.out << std::format("      ENSLAVED to player {};\n", *p.slaved_to());
  }
}

}  // namespace

void show_map(GameObj& g, const starnum_t snum, const planetnum_t pnum,
              const Planet& p) {
  const player_t playernum = g.player();
  const governor_t governor = g.governor();
  const int show = 1;  // TODO(jeffbailey): This was always set to on, but this
                       // fact is output to the client, which might affect the
                       // client interface. Can remove the conditional as soon
                       // as we know that it's not client affecting.

  const auto& race = *g.race;
  const auto* smap = g.entity_manager.peek_sectormap(snum, pnum);
  const LandedShipGrid grid = scan_planet_ships_for_map(
      g.entity_manager, snum, pnum, p, playernum, governor, race);

  /* report that this is a planet map */
  const auto* star = g.entity_manager.peek_star(snum);
  g.out << std::format("${};{};{};{};", star->get_planet_name(pnum),
                       p.dimensions().x, p.dimensions().y, show);

  /* send map data */
  for (auto [c, sector] : smap->indexed_sectors()) {
    output_map_sector_cell(g, playernum, governor, race, sector,
                           grid.shiplocs[c.x][c.y], grid.has_visual_iq);
  }
  g.out << '\n';

  if (show) {
    show_planet_stats(g, p, playernum, race);
  }
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
