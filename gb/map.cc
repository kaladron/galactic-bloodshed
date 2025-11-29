// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module gblib;

void show_map(GameObj& g, const starnum_t snum, const planetnum_t pnum,
              const Planet& p) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  int iq = 0;
  char shiplocs[MAX_X][MAX_Y] = {};

  const int show = 1;  // TODO(jeffbailey): This was always set to on, but this
                       // fact is output to the client, which might affect the
                       // client interface.  Can remove the conditional as soon
                       // as we know that it's not client affecting.

  auto& race = races[Playernum - 1];
  auto smap = getsmap(p);
  if (!race.governor[Governor].toggle.geography) {
    /* traverse ship list on planet; find out if we can look at
       ships here. */
    iq = !!p.info(Playernum - 1).numsectsowned;

    const ShipList shiplist(g.entity_manager, p.ships());
    for (const Ship* s : shiplist) {
      if (s->owner == Playernum && authorized(Governor, *s) &&
          (s->popn || (s->type == ShipType::OTYPE_PROBE)))
        iq = 1;
      if (s->alive && landed(*s))
        shiplocs[s->land_x][s->land_y] = Shipltrs[s->type];
    }
  }
  /* report that this is a planet map */
  g.out << '$';
  g.out << std::format("{};", stars[snum].get_planet_name(pnum));
  g.out << std::format("{};{};{};", p.Maxx(), p.Maxy(), show);

  /* send map data */
  for (const auto& sector : smap) {
    bool owned1 =
        (sector.get_owner() == race.governor[Governor].toggle.highlight);
    if (shiplocs[sector.get_x()][sector.get_y()] && iq) {
      if (race.governor[Governor].toggle.color)
        g.out << std::format("{}{}", (char)(sector.get_owner() + '?'),
                             shiplocs[sector.get_x()][sector.get_y()]);
      else {
        if (owned1 && race.governor[Governor].toggle.inverse)
          g.out << std::format("1{}{}", (char)(sector.get_owner() + '?'),
                               shiplocs[sector.get_x()][sector.get_y()]);

        else
          g.out << std::format("0{}{}", (char)(sector.get_owner() + '?'),
                               shiplocs[sector.get_x()][sector.get_y()]);
      }
    } else {
      if (race.governor[Governor].toggle.color) {
        g.out << std::format("{}{}", (char)(sector.get_owner() + '?'),
                             desshow(Playernum, Governor, race, sector));
      } else {
        if (owned1 && race.governor[Governor].toggle.inverse) {
          g.out << std::format("1{}{}", (char)(sector.get_owner() + '?'),
                               desshow(Playernum, Governor, race, sector));
        } else {
          g.out << std::format("0{}{}", (char)(sector.get_owner() + '?'),
                               desshow(Playernum, Governor, race, sector));
        }
      }
    }
  }
  g.out << '\n';

  if (!show) return;

  g.out << std::format(
      "Type: {:<8}   Sects {:<7}: {:<3}   Aliens:", Planet_types[p.type()],
      race.Metamorph ? "covered" : "owned",
      p.info(Playernum - 1).numsectsowned);
  if (p.explored() || race.tech >= TECH_EXPLORE) {
    bool f = false;
    for (auto i = 1U; i < MAXPLAYERS; i++) {
      if (p.info(i - 1).numsectsowned != 0 && i != Playernum) {
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
      p.info(Playernum - 1).guns, p.info(Playernum - 1).mob_points);
  g.out << std::format(
      "      Mobilization : {:<3} ({:<3})     Compatibility: {:.2f}%",
      p.info(Playernum - 1).comread, p.info(Playernum - 1).mob_set,
      p.compatibility(race));
  if (p.conditions(TOXIC) > 50) {
    g.out << std::format("    ({}% TOXIC)\n", p.conditions(TOXIC));
  }
  g.out << "\n";
  g.out << std::format("Resource stockpile : {:<9}    Fuel stockpile: {}\n",
                       p.info(Playernum - 1).resource,
                       p.info(Playernum - 1).fuel);
  g.out << std::format(
      "      Destruct cap : {:<9} {:>18}: {:<5} ({:<5}/{:<})\n",
      p.info(Playernum - 1).destruct,
      race.Metamorph ? "Tons of biomass" : "Total Population",
      p.info(Playernum - 1).popn, p.popn(),
      round_rand(.01 * (100. - p.conditions(TOXIC)) * p.maxpopn()));
  g.out << std::format("          Crystals : {:<9} {:>18}: {:<5} ({:<5})\n",
                       p.info(Playernum - 1).crystals, "Ground forces",
                       p.info(Playernum - 1).troops, p.troops());
  g.out << std::format("{} Total Resource Deposits     Tax rate {}%  New {}%\n",
                       p.total_resources(), p.info(Playernum - 1).tax,
                       p.info(Playernum - 1).newtax);
  g.out << std::format("Estimated Production Next Update : {:.2f}\n",
                       p.info(Playernum - 1).est_production);
  if (p.slaved_to()) {
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
  if (s.get_troops() && !r.governor[Governor].toggle.geography) {
    if (s.get_owner() == Playernum) return CHAR_MY_TROOPS;
    if (isset(r.allied, s.get_owner())) return CHAR_ALLIED_TROOPS;
    if (isset(r.atwar, s.get_owner())) return CHAR_ATWAR_TROOPS;

    return CHAR_NEUTRAL_TROOPS;
  }

  if (s.get_owner() && !r.governor[Governor].toggle.geography &&
      !r.governor[Governor].toggle.color) {
    if (!r.governor[Governor].toggle.inverse ||
        s.get_owner() != r.governor[Governor].toggle.highlight) {
      if (!r.governor[Governor].toggle.double_digits)
        return (s.get_owner() % 10) + '0';

      if (s.get_owner() < 10 || s.get_x() % 2)
        return (s.get_owner() % 10) + '0';
      return (s.get_owner() / 10) + '0';
    }
  }

  if (s.get_crystals() && (r.discoveries[D_CRYSTAL] || r.God))
    return CHAR_CRYSTAL;

  return get_sector_char(s.get_condition());
}
