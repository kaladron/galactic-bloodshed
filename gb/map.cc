// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module gblib;

namespace {
// Converts a sector's owner ID to a printable ASCII character.
// Owner 0 → '?', Owner 1 → '@', Owner 2 → 'A', etc.
// This allows player numbers to be displayed as readable characters in map
// output.
char get_owner_char(const Sector& sector) {
  return (char)(sector.get_owner().value + '?');
}
}  // namespace

void show_map(GameObj& g, const starnum_t snum, const planetnum_t pnum,
              const Planet& p) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  int iq = 0;
  char shiplocs[MAX_X][MAX_Y] = {};

  const int show = 1;  // TODO(jeffbailey): This was always set to on, but this
                       // fact is output to the client, which might affect the
                       // client interface.  Can remove the conditional as soon
                       // as we know that it's not client affecting.

  auto& race = *g.race;
  const auto* smap = g.entity_manager.peek_sectormap(snum, pnum);
  if (!race.governor[Governor.value].toggle.geography) {
    /* traverse ship list on planet; find out if we can look at
       ships here. */
    iq = !!p.info(Playernum).numsectsowned;

    const ShipList shiplist(g.entity_manager, p.ships());
    for (const Ship* s : shiplist) {
      if (s->owner() == Playernum && authorized(Governor, *s) &&
          (s->popn() || (s->type() == ShipType::OTYPE_PROBE)))
        iq = 1;
      if (s->alive() && landed(*s))
        shiplocs[s->land_x()][s->land_y()] = Shipltrs[s->type()];
    }
  }
  /* report that this is a planet map */
  g.out << '$';
  const auto* star = g.entity_manager.peek_star(snum);
  g.out << std::format("{};", star->get_planet_name(pnum));
  g.out << std::format("{};{};{};", p.Maxx(), p.Maxy(), show);

  /* send map data */
  for (const auto& sector : *smap) {
    bool owned1 =
        (sector.get_owner() == race.governor[Governor.value].toggle.highlight);
    if (shiplocs[sector.get_x()][sector.get_y()] && iq) {
      if (race.governor[Governor.value].toggle.color)
        g.out << std::format("{}{}", get_owner_char(sector),
                             shiplocs[sector.get_x()][sector.get_y()]);
      else {
        if (owned1 && race.governor[Governor.value].toggle.inverse)
          g.out << std::format("1{}{}", get_owner_char(sector),
                               shiplocs[sector.get_x()][sector.get_y()]);

        else
          g.out << std::format("0{}{}", get_owner_char(sector),
                               shiplocs[sector.get_x()][sector.get_y()]);
      }
    } else {
      if (race.governor[Governor.value].toggle.color) {
        g.out << std::format("{}{}", get_owner_char(sector),
                             desshow(Playernum, Governor, race, sector));
      } else {
        if (owned1 && race.governor[Governor.value].toggle.inverse) {
          g.out << std::format("1{}{}", get_owner_char(sector),
                               desshow(Playernum, Governor, race, sector));
        } else {
          g.out << std::format("0{}{}", get_owner_char(sector),
                               desshow(Playernum, Governor, race, sector));
        }
      }
    }
  }
  g.out << '\n';

  if (!show) return;

  g.out << std::format(
      "Type: {:<8}   Sects {:<7}: {:<3}   Aliens:", Planet_types[p.type()],
      race.Metamorph ? "covered" : "owned", p.info(Playernum).numsectsowned);
  if (p.explored() || race.tech >= TECH_EXPLORE) {
    bool f = false;
    for (auto i = 1U; i < MAXPLAYERS; i++) {
      if (p.info(i).numsectsowned != 0 && i != Playernum.value) {
        f = true;
        g.out << std::format("{}{}", isset(race.atwar, i) ? '*' : ' ', i);
      }
    }
    if (!f) g.out << "(none)\n";
  } else {
    g.out << R"(???)";
  }
  g.out << "\n";
  g.out << std::format(
      "              Guns : {:<3}             Mob Points : {}\n",
      p.info(Playernum).guns, p.info(Playernum).mob_points);
  g.out << std::format(
      "      Mobilization : {:<3} ({:<3})     Compatibility: {:.2f}%",
      p.info(Playernum).comread, p.info(Playernum).mob_set,
      p.compatibility(race));
  if (p.conditions(TOXIC) > 50) {
    g.out << std::format("    ({}% TOXIC)\n", p.conditions(TOXIC));
  }
  g.out << "\n";
  g.out << std::format("Resource stockpile : {:<9}    Fuel stockpile: {}\n",
                       p.info(Playernum).resource, p.info(Playernum).fuel);
  g.out << std::format(
      "      Destruct cap : {:<9} {:>18}: {:<5} ({:<5}/{:<})\n",
      p.info(Playernum).destruct,
      race.Metamorph ? "Tons of biomass" : "Total Population",
      p.info(Playernum).popn, p.popn(),
      round_rand(.01 * (100. - p.conditions(TOXIC)) * p.maxpopn()));
  g.out << std::format("          Crystals : {:<9} {:>18}: {:<5} ({:<5})\n",
                       p.info(Playernum).crystals, "Ground forces",
                       p.info(Playernum).troops, p.troops());
  g.out << std::format("{} Total Resource Deposits     Tax rate {}%  New {}%\n",
                       p.total_resources(), p.info(Playernum).tax,
                       p.info(Playernum).newtax);
  g.out << std::format("Estimated Production Next Update : {:.2f}\n",
                       p.info(Playernum).est_production);
  if (p.slaved_to() != 0) {
    g.out << std::format("      ENSLAVED to player {};\n", p.slaved_to());
  }
}

char get_sector_char(unsigned int condition) {
  switch (condition) {
    case SectorType::SEC_WASTED:
      return CHAR_WASTED;
    case SectorType::SEC_SEA:
      return CHAR_SEA;
    case SectorType::SEC_LAND:
      return CHAR_LAND;
    case SectorType::SEC_MOUNT:
      return CHAR_MOUNT;
    case SectorType::SEC_GAS:
      return CHAR_GAS;
    case SectorType::SEC_PLATED:
      return CHAR_PLATED;
    case SectorType::SEC_ICE:
      return CHAR_ICE;
    case SectorType::SEC_DESERT:
      return CHAR_DESERT;
    case SectorType::SEC_FOREST:
      return CHAR_FOREST;
    default:
      return ('?');
  }
}

char desshow(const player_t Playernum, const governor_t Governor, const Race& r,
             const Sector& s) {
  if (s.get_troops() && !r.governor[Governor.value].toggle.geography) {
    if (s.get_owner() == Playernum) return CHAR_MY_TROOPS;
    if (isset(r.allied, s.get_owner())) return CHAR_ALLIED_TROOPS;
    if (isset(r.atwar, s.get_owner())) return CHAR_ATWAR_TROOPS;

    return CHAR_NEUTRAL_TROOPS;
  }

  if (s.get_owner() != 0 && !r.governor[Governor.value].toggle.geography &&
      !r.governor[Governor.value].toggle.color) {
    if (!r.governor[Governor.value].toggle.inverse ||
        s.get_owner() != r.governor[Governor.value].toggle.highlight) {
      if (!r.governor[Governor.value].toggle.double_digits)
        return (s.get_owner().value % 10) + '0';

      if (s.get_owner().value < 10 || s.get_x() % 2)
        return (s.get_owner().value % 10) + '0';
      return (s.get_owner().value / 10) + '0';
    }
  }

  if (s.get_crystals() && (r.discoveries[D_CRYSTAL] || r.God))
    return CHAR_CRYSTAL;

  return get_sector_char(s.get_condition());
}
