// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "map.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "GB_server.h"
#include "buffers.h"
#include "files_shl.h"
#include "fire.h"
#include "getplace.h"
#include "max.h"
#include "orbit.h"
#include "races.h"
#include "rand.h"
#include "ships.h"
#include "shlmisc.h"
#include "tweakables.h"
#include "vars.h"

static void show_map(const player_t, const governor_t, const starnum_t,
                     const planetnum_t, const Planet &);

void map(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  placetype where;

  if (argv.size() > 1) {
    where = Getplace(g, argv[1], 0);
  } else {
    where = Getplace(g, "", 0);
  }

  if (where.err) return;

  switch (where.level) {
    case ScopeLevel::LEVEL_SHIP:
      g.out << "Bad scope.\n";
      return;
    case ScopeLevel::LEVEL_PLAN: {
      const auto &p = getplanet(where.snum, where.pnum);
      show_map(Playernum, Governor, where.snum, where.pnum, p);
      if (Stars[where.snum]->stability > 50)
        notify(Playernum, Governor,
               "WARNING! This planet's primary is unstable.\n");
    } break;
    default:
      orbit(argv, g); /* make orbit map instead */
  }
}

static void show_map(const player_t Playernum, const governor_t Governor,
                     const starnum_t snum, const planetnum_t pnum,
                     const Planet &p) {
  int x, y, i, f = 0, owner, owned1;
  int iq = 0;
  int sh;
  Ship *s;
  char shiplocs[MAX_X][MAX_Y] = {};
  hugestr output;

  const int show = 1;  // TODO(jeffbailey): This was always set to on, but this
                       // fact is output to the client, which might affect the
                       // client interface.  Can remove the conditional as soon
                       // as we know that it's not client affecting.

  auto Race = races[Playernum - 1];
  auto smap = getsmap(p);
  if (!Race->governor[Governor].toggle.geography) {
    /* traverse ship list on planet; find out if we can look at
       ships here. */
    iq = !!p.info[Playernum - 1].numsectsowned;
    sh = p.ships;

    while (sh) {
      if (!getship(&s, sh)) {
        sh = 0;
        continue;
      }
      if (s->owner == Playernum && authorized(Governor, s) &&
          (s->popn || (s->type == ShipType::OTYPE_PROBE)))
        iq = 1;
      if (s->alive && landed(s))
        shiplocs[s->land_x][s->land_y] = Shipltrs[s->type];
      sh = s->nextship;
      free(s);
    }
  }
  /* report that this is a planet map */
  sprintf(output, "$");

  sprintf(buf, "%s;", Stars[snum]->pnames[pnum]);
  strcat(output, buf);

  sprintf(buf, "%d;%d;%d;", p.Maxx, p.Maxy, show);
  strcat(output, buf);

  /* send map data */
  for (y = 0; y < p.Maxy; y++)
    for (x = 0; x < p.Maxx; x++) {
      owner = smap.get(x, y).owner;
      owned1 = (owner == Race->governor[Governor].toggle.highlight);
      if (shiplocs[x][y] && iq) {
        if (Race->governor[Governor].toggle.color)
          sprintf(buf, "%c%c", (char)(owner + '?'), shiplocs[x][y]);
        else {
          if (owned1 && Race->governor[Governor].toggle.inverse)
            sprintf(buf, "1%c", shiplocs[x][y]);
          else
            sprintf(buf, "0%c", shiplocs[x][y]);
        }
      } else {
        if (Race->governor[Governor].toggle.color)
          sprintf(buf, "%c%c", (char)(owner + '?'),
                  desshow(Playernum, Governor, x, y, Race, smap));
        else {
          if (owned1 && Race->governor[Governor].toggle.inverse)
            sprintf(buf, "1%c", desshow(Playernum, Governor, x, y, Race, smap));
          else
            sprintf(buf, "0%c", desshow(Playernum, Governor, x, y, Race, smap));
        }
      }
      strcat(output, buf);
    }
  strcat(output, "\n");
  notify(Playernum, Governor, output);

  if (show) {
    sprintf(temp, "Type: %8s   Sects %7s: %3u   Aliens:", Planet_types[p.type],
            Race->Metamorph ? "covered" : "owned",
            p.info[Playernum - 1].numsectsowned);
    if (p.explored || Race->tech >= TECH_EXPLORE) {
      f = 0;
      for (i = 1; i < MAXPLAYERS; i++)
        if (p.info[i - 1].numsectsowned && i != Playernum) {
          f = 1;
          sprintf(buf, "%c%d", isset(Race->atwar, i) ? '*' : ' ', i);
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
            compatibility(p, Race));
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
            Race->Metamorph ? "Tons of biomass" : "Total Population",
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

char desshow(const player_t Playernum, const governor_t Governor, const int x,
             const int y, const racetype *r, sector_map &smap) {
  auto &s = smap.get(x, y);

  if (s.troops && !r->governor[Governor].toggle.geography) {
    if (s.owner == Playernum) return CHAR_MY_TROOPS;
    if (isset(r->allied, s.owner)) return CHAR_ALLIED_TROOPS;
    if (isset(r->atwar, s.owner)) return CHAR_ATWAR_TROOPS;

    return CHAR_NEUTRAL_TROOPS;
  }

  if (s.owner && !r->governor[Governor].toggle.geography &&
      !r->governor[Governor].toggle.color) {
    if (!r->governor[Governor].toggle.inverse ||
        s.owner != r->governor[Governor].toggle.highlight) {
      if (!r->governor[Governor].toggle.double_digits)
        return s.owner % 10 + '0';

      if (s.owner < 10 || x % 2) return s.owner % 10 + '0';

      return s.owner / 10 + '0';
    }
  }

  if (s.crystals && (r->discoveries[D_CRYSTAL] || r->God)) return CHAR_CRYSTAL;

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
