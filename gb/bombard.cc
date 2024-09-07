// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import std.compat;

#include "gb/bombard.h"

#include <strings.h>

#include "gb/buffers.h"

/**
 * Performs a bombardment action by a berserker ship on a planet.
 *
 * This function checks if there are any Point Defense Networks (PDNs) present
 * on the planet. If PDNs are present, the bombardment is cancelled and a
 * warning message is sent to the ship's owner. Otherwise, the function searches
 * for a sector to bombard. It looks for sectors owned by other races that are
 * at war with the ship's race or are the target of the berserker ship. If no
 * suitable sector is found, a notification is sent to the ship's owner
 * indicating that there are no sectors worth bombing.
 *
 * If a suitable sector is found, the function calculates the strength of the
 * bombardment based on the ship's guns and damage. It then proceeds to destroy
 * sectors on the planet using the shoot_ship_to_planet function. The number of
 * destroyed sectors is returned. The ship's owner is notified of the
 * bombardment results, and an alert is sent to the other players. If the ship
 * has no weapons, a notification is sent to the ship's owner indicating the
 * lack of weapons.
 *
 * @param ship A pointer to the berserker ship performing the bombardment.
 * @param planet The planet being bombarded.
 * @param r The race to which the ship belongs.
 * @return The number of sectors destroyed during the bombardment.
 */
int berserker_bombard(Ship *ship, Planet &planet, Race &r) {
  int x;
  int y;
  int x2 = -1;
  int y2;

  /* for telegramming */
  Nuked.fill(0);

  /* check to see if PDNs are present */
  Shiplist shiplist(planet.ships);
  for (auto s : shiplist) {
    if (s.alive && s.type == ShipType::OTYPE_PLANDEF &&
        s.owner != ship->owner) {
      sprintf(buf, "Bombardment of %s cancelled, PDNs are present.\n",
              prin_ship_orbits(*ship).c_str());
      warn(ship->owner, ship->governor, buf);
      return 0;
    }
  }

  auto smap = getsmap(planet);

  /* look for someone to bombard-check for war */
  bool found = false;
  for (auto shuffled = smap.shuffle(); auto &sector_wrap : shuffled) {
    Sector &sect = sector_wrap;
    if (sect.owner && sect.owner != ship->owner &&
        (sect.condition != SectorType::SEC_WASTED)) {
      if (isset(r.atwar, sect.owner) ||
          (ship->type == ShipType::OTYPE_BERS &&
           sect.owner == ship->special.mind.target)) {
        found = true;
        break;
      } else {
        x = x2 = sect.x;
        y = y2 = sect.y;
      }
    }
  }
  if (x2 != -1) {
    x = x2; /* no one we're at war with; bomb someone else. */
    y = y2;
    found = true;
  }

  if (!found) {
    /* there were no sectors worth bombing. */
    if (!ship->notified) {
      ship->notified = 1;
      sprintf(telegram_buf, "Report from %c%lu %s\n\n", Shipltrs[ship->type],
              ship->number, ship->name);
      sprintf(buf, "Planet /%s/%s has been saturation bombed.\n",
              stars[ship->storbits].name,
              stars[ship->storbits].pnames[ship->pnumorbits]);
      strcat(telegram_buf, buf);
      notify(ship->owner, ship->governor, telegram_buf);
    }
    return 0;
  }

  int str = MIN(Shipdata[ship->type][ABIL_GUNS] * (100 - ship->damage) / 100.,
                ship->destruct);
  if (!str) {
    /* no weapons! */
    if (!ship->notified) {
      ship->notified = 1;
      sprintf(telegram_buf,
              "Bulletin\n\n %c%lu %s has no weapons to bombard with.\n",
              Shipltrs[ship->type], ship->number, ship->name);
      warn(ship->owner, ship->governor, telegram_buf);
    }
    return 0;
  }

  // Enemy planet retaliates along with defending forces

  Nuked.fill(0);
  // save owner of destroyed sector
  auto oldown = smap.get(x, y).owner;
  ship->destruct -= str;
  ship->mass -= str * MASS_DESTRUCT;

  auto numdest = shoot_ship_to_planet(*ship, planet, str, x, y, smap, 0, 0,
                                      long_buf, short_buf);
  /* (0=dont get smap) */
  if (numdest < 0) numdest = 0;

  /* tell the bombarding player about it.. */
  std::stringstream telegram_report;
  telegram_report << std::format("REPORT from ship #{}\n\n", ship->number);
  telegram_report << short_buf;
  telegram_report << std::format(
      "sector {},{} (owner {}). {} sectors destroyed.\n", x, y, oldown,
      numdest);
  notify(ship->owner, ship->governor, telegram_report.str());

  /* notify other player. */
  sprintf(telegram_buf, "ALERT from planet /%s/%s\n",
          stars[ship->storbits].name,
          stars[ship->storbits].pnames[ship->pnumorbits]);
  sprintf(buf, "%c%lu%s bombarded sector %d,%d; %d sectors destroyed.\n",
          Shipltrs[ship->type], ship->number, ship->name, x, y, numdest);
  strcat(telegram_buf, buf);
  sprintf(buf, "%c%lu %s [%d] bombards %s/%s\n", Shipltrs[ship->type],
          ship->number, ship->name, ship->owner, stars[ship->storbits].name,
          stars[ship->storbits].pnames[ship->pnumorbits]);
  for (player_t i = 1; i <= Num_races; i++)
    if (Nuked[i - 1] && i != ship->owner)
      warn(i, stars[ship->storbits].governor[i - 1], telegram_buf);
  post(buf, NewsType::COMBAT);

  putsmap(smap, planet);

  return numdest;
}
