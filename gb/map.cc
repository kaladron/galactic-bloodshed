// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import commands;
import std.compat;

#include "gb/map.h"

#include "gb/buffers.h"
#include "gb/tweakables.h"

void show_map(const player_t Playernum, const governor_t Governor,
              const starnum_t snum, const planetnum_t pnum, const Planet &p) {
  player_t i;
  int iq = 0;
  char shiplocs[MAX_X][MAX_Y] = {};
  std::ostringstream output;

  const int show = 1;  // TODO(jeffbailey): This was always set to on, but this
                       // fact is output to the client, which might affect the
                       // client interface.  Can remove the conditional as soon
                       // as we know that it's not client affecting.

  auto &race = races[Playernum - 1];
  auto smap = getsmap(p);
  if (!race.governor[Governor].toggle.geography) {
    /* traverse ship list on planet; find out if we can look at
       ships here. */
    iq = !!p.info[Playernum - 1].numsectsowned;

    Shiplist shiplist{p.ships};
    for (const auto &s : shiplist) {
      if (s.owner == Playernum && authorized(Governor, s) &&
          (s.popn || (s.type == ShipType::OTYPE_PROBE)))
        iq = 1;
      if (s.alive && landed(s)) shiplocs[s.land_x][s.land_y] = Shipltrs[s.type];
    }
  }
  /* report that this is a planet map */
  output << '$';

  sprintf(buf, "%s;", stars[snum].pnames[pnum]);
  output << buf;

  sprintf(buf, "%d;%d;%d;", p.Maxx, p.Maxy, show);
  output << buf;

  /* send map data */
  for (const auto &sector : smap) {
    bool owned1 = (sector.owner == race.governor[Governor].toggle.highlight);
    if (shiplocs[sector.x][sector.y] && iq) {
      if (race.governor[Governor].toggle.color)
        sprintf(buf, "%c%c", (char)(sector.owner + '?'),
                shiplocs[sector.x][sector.y]);
      else {
        if (owned1 && race.governor[Governor].toggle.inverse)
          sprintf(buf, "1%c", shiplocs[sector.x][sector.y]);
        else
          sprintf(buf, "0%c", shiplocs[sector.x][sector.y]);
      }
    } else {
      if (race.governor[Governor].toggle.color)
        sprintf(buf, "%c%c", (char)(sector.owner + '?'),
                desshow(Playernum, Governor, race, sector));
      else {
        if (owned1 && race.governor[Governor].toggle.inverse)
          sprintf(buf, "1%c", desshow(Playernum, Governor, race, sector));
        else
          sprintf(buf, "0%c", desshow(Playernum, Governor, race, sector));
      }
    }
    output << buf;
  }
  output << '\n';
  output << std::ends;
  notify(Playernum, Governor, output.str());

  if (show) {
    char temp[2047];
    sprintf(temp, "Type: %8s   Sects %7s: %3u   Aliens:", Planet_types[p.type],
            race.Metamorph ? "covered" : "owned",
            p.info[Playernum - 1].numsectsowned);
    if (p.explored || race.tech >= TECH_EXPLORE) {
      bool f = false;
      for (i = 1; i < MAXPLAYERS; i++)
        if (p.info[i - 1].numsectsowned != 0 && i != Playernum) {
          f = true;
          sprintf(buf, "%c%d", isset(race.atwar, i) ? '*' : ' ', i);
          strcat(temp, buf);
        }
      if (!f) strcat(temp, "(none)");
    } else
      strcat(temp, R"(???)");
    strcat(temp, "\n");
    notify(Playernum, Governor, temp);
    sprintf(temp, "              Guns : %3d             Mob Points : %ld\n",
            p.info[Playernum - 1].guns, p.info[Playernum - 1].mob_points);
    notify(Playernum, Governor, temp);
    sprintf(temp, "      Mobilization : %3d (%3d)     Compatibility: %.2f%%",
            p.info[Playernum - 1].comread, p.info[Playernum - 1].mob_set,
            p.compatibility(race));
    if (p.conditions[TOXIC] > 50) {
      sprintf(buf, "    (%d%% TOXIC)", p.conditions[TOXIC]);
      strcat(temp, buf);
    }
    strcat(temp, "\n");
    notify(Playernum, Governor, temp);
    sprintf(temp, "Resource stockpile : %-9lu    Fuel stockpile: %u\n",
            p.info[Playernum - 1].resource, p.info[Playernum - 1].fuel);
    notify(Playernum, Governor, temp);
    sprintf(temp, "      Destruct cap : %-9u%18s: %-5lu (%lu/%u)\n",
            p.info[Playernum - 1].destruct,
            race.Metamorph ? "Tons of biomass" : "Total Population",
            p.info[Playernum - 1].popn, p.popn,
            round_rand(.01 * (100. - p.conditions[TOXIC]) * p.maxpopn));
    notify(Playernum, Governor, temp);
    sprintf(temp, "          Crystals : %-9u%18s: %-5lu (%lu)\n",
            p.info[Playernum - 1].crystals, "Ground forces",
            p.info[Playernum - 1].troops, p.troops);
    notify(Playernum, Governor, temp);
    sprintf(temp, "%ld Total Resource Deposits     Tax rate %u%%  New %u%%\n",
            p.total_resources, p.info[Playernum - 1].tax,
            p.info[Playernum - 1].newtax);
    notify(Playernum, Governor, temp);
    sprintf(temp, "Estimated Production Next Update : %.2f\n",
            p.info[Playernum - 1].est_production);
    notify(Playernum, Governor, temp);
    if (p.slaved_to) {
      sprintf(temp, "      ENSLAVED to player %d\n", p.slaved_to);
      notify(Playernum, Governor, temp);
    }
  }
}

char desshow(const player_t Playernum, const governor_t Governor, const Race &r,
             const Sector &s) {
  if (s.troops && !r.governor[Governor].toggle.geography) {
    if (s.owner == Playernum) return CHAR_MY_TROOPS;
    if (isset(r.allied, s.owner)) return CHAR_ALLIED_TROOPS;
    if (isset(r.atwar, s.owner)) return CHAR_ATWAR_TROOPS;

    return CHAR_NEUTRAL_TROOPS;
  }

  if (s.owner && !r.governor[Governor].toggle.geography &&
      !r.governor[Governor].toggle.color) {
    if (!r.governor[Governor].toggle.inverse ||
        s.owner != r.governor[Governor].toggle.highlight) {
      if (!r.governor[Governor].toggle.double_digits) return s.owner % 10 + '0';

      if (s.owner < 10 || s.x % 2) return s.owner % 10 + '0';

      return s.owner / 10 + '0';
    }
  }

  if (s.crystals && (r.discoveries[D_CRYSTAL] || r.God)) return CHAR_CRYSTAL;

  switch (s.condition) {
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
