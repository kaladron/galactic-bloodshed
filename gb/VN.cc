// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* VN.c -- assorted Von Neumann machine code */

module;

import std.compat;

#include <cstdlib>

module gblib;

namespace {
void order_berserker(Ship& ship) {
  /* give berserkers a mission - send to planet of offending player and bombard
   * it */
  ship.bombard() = 1;
  MindData mind{}; /* who to attack */
  mind.target = VN_brain.Most_mad;
  ship.whatdest() = ScopeLevel::LEVEL_PLAN;
  if (random() & 01)
    ship.deststar() = Sdata.VN_index1[mind.target - 1];
  else
    ship.deststar() = Sdata.VN_index2[mind.target - 1];
  ship.destpnum() = int_rand(0, stars[ship.deststar()].numplanets() - 1);
  if (ship.hyper_drive().has && ship.mounted()) {
    ship.hyper_drive().on = 1;
    ship.hyper_drive().ready = 1;
    mind.busy = 1;
  }
  ship.special() = mind;
}

void order_VN(Ship& ship) {
  int min = 0;
  int min2 = 0;

  /* find closest star */
  for (auto s = 0; s < Sdata.numstars; s++)
    if (s != ship.storbits() &&
        Distsq(stars[s].xpos(), stars[s].ypos(), ship.xpos(), ship.ypos()) <
            Distsq(stars[min].xpos(), stars[min].ypos(), ship.xpos(),
                   ship.ypos())) {
      min2 = min;
      min = s;
    }

  /* don't go there if we have a choice,
     and we have VN's there already */
  if (isset(stars[min].inhabited(), 1U))
    if (isset(stars[min2].inhabited(), 1U))
      ship.deststar() = int_rand(0, (int)Sdata.numstars - 1);
    else
      ship.deststar() = min2; /* 2nd closest star */
  else
    ship.deststar() = min;

  if (stars[ship.deststar()].numplanets()) {
    ship.destpnum() = int_rand(0, stars[ship.deststar()].numplanets() - 1);
    ship.whatdest() = ScopeLevel::LEVEL_PLAN;
    if (std::holds_alternative<MindData>(ship.special())) {
      auto mind = std::get<MindData>(ship.special());
      mind.busy = 1;
      ship.special() = mind;
    }
  } else {
    /* no good; find someplace else. */
    if (std::holds_alternative<MindData>(ship.special())) {
      auto mind = std::get<MindData>(ship.special());
      mind.busy = 0;
      ship.special() = mind;
    }
  }
  ship.speed() = Shipdata[ShipType::OTYPE_VN][ABIL_SPEED];
}
}  // namespace

/*  do_VN() -- called by doship() */
void do_VN(EntityManager& em, Ship& ship, TurnStats& stats) {
  if (!landed(ship)) {
    // Doing other things
    if (!std::holds_alternative<MindData>(ship.special()) ||
        !std::get<MindData>(ship.special()).busy)
      return;

    // we were just built & launched
    if (ship.type() == ShipType::OTYPE_BERS)
      order_berserker(ship);
    else
      order_VN(ship);
    return;
  }

  stats.Stinfo[ship.storbits()][ship.pnumorbits()].inhab = 1;

  /* launch if no assignment */
  if (!std::holds_alternative<MindData>(ship.special()) ||
      !std::get<MindData>(ship.special()).busy) {
    if (ship.fuel() >= (double)ship.max_fuel()) {
      const auto* star = em.peek_star(ship.storbits());
      const auto* planet = em.peek_planet(ship.storbits(), ship.pnumorbits());
      ship.xpos() = star->xpos() + planet->xpos() + int_rand(-10, 10);
      ship.ypos() = star->ypos() + planet->ypos() + int_rand(-10, 10);
      ship.docked() = 0;
      ship.whatdest() = ScopeLevel::LEVEL_UNIV;
    }
    return;
  }

  /* we have an assignment.  Since we are landed, this means
     we are engaged in building up resources/fuel. */
  /* steal resources from other players */
  /* permute list of people to steal from */
  std::array<int, MAXPLAYERS + 1> nums;
  for (int i = 1; i <= Num_races; i++)
    nums[i] = i;
  for (int i = 1; i <= Num_races; i++) {
    int f = int_rand(1, Num_races);
    std::swap(nums[i], nums[f]);
  }

  auto planet_handle = em.get_planet(ship.storbits(), ship.pnumorbits());

  // Loop through permuted vector until someone has resources on
  // this planet to steal

  player_t f = 0;
  for (player_t i = 1; i <= Num_races; i++)
    if (planet_handle->info(nums[i] - 1).resource) f = nums[i];

  // No resources to steal
  if (f == 0) return;

  // Steal the resources

  auto prod = std::min(planet_handle->info(f - 1).resource,
                       Shipdata[ShipType::OTYPE_VN][ABIL_COST]);
  planet_handle->info(f - 1).resource -= prod;

  std::string buf;

  if (ship.type() == ShipType::OTYPE_VN) {
    rcv_resource(ship, prod);
    buf = std::format("{0} resources stolen from [{1}] by {2}{3} at {4}.", prod,
                      f, Shipltrs[ShipType::OTYPE_VN], ship.number(),
                      prin_ship_orbits(em, ship).c_str());
  } else if (ship.type() == ShipType::OTYPE_BERS) {
    rcv_destruct(ship, prod);
    buf = std::format("{0} resources stolen from [{1}] by {2}{3} at {4}.", prod,
                      f, Shipltrs[ShipType::OTYPE_BERS], ship.number(),
                      prin_ship_orbits(em, ship));
  }

  push_telegram_race(em, f, buf);
  if (f != ship.owner()) push_telegram(ship.owner(), ship.governor(), buf);
}

/*  planet_doVN() -- called by doplanet() */
void planet_doVN(Ship& ship, Planet& planet, SectorMap& smap,
                 EntityManager& entity_manager) {
  int j;
  int oldres;
  int xa;
  int ya;
  int prod;

  if (landed(ship)) {
    if (ship.type() == ShipType::OTYPE_VN &&
        std::holds_alternative<MindData>(ship.special()) &&
        std::get<MindData>(ship.special()).busy) {
      /* first try and make some resources(VNs) by ourselves.
         more might be stolen in doship */
      auto& s = smap.get(ship.land_x(), ship.land_y());
      if (!(oldres = s.get_resource())) {
        /* move to another sector */
        xa = int_rand(-1, 1);
        ship.land_x() = mod(ship.land_x() + xa, planet.Maxx());
        ya = (ship.land_y() == 0)
                 ? 1
                 : ((ship.land_y() == (planet.Maxy() - 1)) ? -1
                                                           : int_rand(-1, 1));
        ship.land_y() += ya;
      } else {
        /* mine the sector */
        s.set_resource(s.get_resource() * VN_RES_TAKE);
        prod = oldres -
               s.get_resource(); /* poor way for a player to mine resources */
        if (ship.type() == ShipType::OTYPE_VN)
          rcv_resource(ship, prod);
        else if (ship.type() == ShipType::OTYPE_BERS)
          rcv_destruct(ship, 5 * prod);
        rcv_fuel(ship, (double)prod);
      }
      /* now try to construct another machine */
      ShipType shipbuild = (VN_brain.Total_mad > 100 && random() & 01)
                               ? ShipType::OTYPE_BERS
                               : ShipType::OTYPE_VN;
      if (ship.resource() >= Shipdata[shipbuild][ABIL_COST]) {
        int numVNs;
        /* construct as many VNs as possible */
        numVNs = ship.resource() / Shipdata[shipbuild][ABIL_COST];
        for (j = 1; j <= numVNs; j++) {
          use_resource(ship, Shipdata[shipbuild][ABIL_COST]);

          // Create new ship via EntityManager with designated initializers
          ship_struct s2_data{
              .xpos = ship.xpos(),
              .ypos = ship.ypos(),
              .land_x = ship.land_x(),
              .land_y = ship.land_y(),
              .nextship = planet.ships(),
              .armor = static_cast<unsigned char>(ship.armor() + 1),
              .max_crew = static_cast<unsigned short>(
                  Shipdata[shipbuild][ABIL_MAXCREW]),
              .max_resource =
                  static_cast<resource_t>(Shipdata[shipbuild][ABIL_CARGO]),
              .max_destruct = static_cast<unsigned short>(
                  Shipdata[shipbuild][ABIL_DESTCAP]),
              .max_fuel = static_cast<unsigned short>(
                  Shipdata[shipbuild][ABIL_FUELCAP]),
              .max_speed =
                  static_cast<unsigned short>(Shipdata[shipbuild][ABIL_SPEED]),
              .storbits = ship.storbits(),
              .deststar = ship.deststar(),
              .destpnum = ship.destpnum(),
              .pnumorbits = ship.pnumorbits(),
              .whatdest = ship.whatdest(),
              .whatorbits = ScopeLevel::LEVEL_PLAN,
              .type = shipbuild,
              .alive = 1,
              .mode = 0,
              .docked = 1,
              .guns = static_cast<unsigned char>(
                  Shipdata[shipbuild][ABIL_PRIMARY] ? PRIMARY : GTYPE_NONE),
              .primary =
                  static_cast<unsigned long>(Shipdata[shipbuild][ABIL_GUNS]),
              .primtype = shipdata_primary(shipbuild),
              .secondary = 0,
              .sectype = shipdata_secondary(shipbuild),
          };
          auto ship_handle = entity_manager.create_ship(s2_data);
          Ship& s2 = *ship_handle;
          s2.size() = ship_size(s2);
          s2.base_mass() = getmass(s2);
          s2.mass() = s2.base_mass();

          planet.ships() = s2.number();
          if (shipbuild == ShipType::OTYPE_BERS) {
            /* target = person killed the most VN's */
            auto ship_mind = std::holds_alternative<MindData>(ship.special())
                                 ? std::get<MindData>(ship.special())
                                 : MindData{};
            s2.special() = MindData{.progenitor = ship_mind.progenitor,
                                    .target = VN_brain.Most_mad,
                                    .generation = ship_mind.generation,
                                    .busy = 0,
                                    .tampered = ship_mind.tampered,
                                    .who_killed = ship_mind.who_killed};
            s2.speed() = Shipdata[ShipType::OTYPE_BERS][ABIL_SPEED];
            s2.tech() = ship.tech() + 100.0;
            s2.bombard() = 1;
            s2.protect().self = 1;
            s2.protect().planet = 1;
            s2.armor() += 10; /* give 'em some armor */
            s2.active() = 1;
            s2.owner() = 1;
            s2.governor() = 0;
            s2.fuel() = 5 * ship.fuel(); /* give 'em some fuel */
            s2.retaliate() = s2.primary();
            s2.destruct() = 500;
            ship.fuel() *= 0.5; /* lose some fuel */
            s2.hyper_drive().has = 1;
            s2.hyper_drive().on = 1;
            s2.hyper_drive().ready = 1;
            s2.hyper_drive().charge = 0;
            s2.mounted() = 1;
            auto buf = std::format("{0} constructed {1}.", ship_to_string(ship),
                                   ship_to_string(s2));
            push_telegram(ship.owner(), ship.governor(), buf);
            if (std::holds_alternative<MindData>(s2.special())) {
              auto mind = std::get<MindData>(s2.special());
              mind.tampered = 0;
              s2.special() = mind;
            }
          } else {
            s2.tech() = ship.tech() + 20.0;
            int n = int_rand(3, std::min(10, SHIP_NAMESIZE)); /* for name */
            s2.name()[n] = '\0';
            while (n--)
              s2.name()[n] = (random() & 01) + '0';
            s2.owner() = 1;
            s2.governor() = 0;
            s2.active() = 1;
            s2.speed() = Shipdata[ShipType::OTYPE_VN][ABIL_SPEED];
            s2.bombard() = 0;
            s2.fuel() = 0.5 * ship.fuel();
            ship.fuel() *= 0.5;
          }
          // Handle mind data for new ship and current ship
          if (std::holds_alternative<MindData>(ship.special())) {
            auto ship_mind = std::get<MindData>(ship.special());
            s2.special() = MindData{.progenitor = ship_mind.progenitor,
                                    .target = ship_mind.target,
                                    .generation = static_cast<unsigned char>(
                                        ship_mind.generation + 1),
                                    .busy = 0,
                                    .tampered = ship_mind.tampered,
                                    .who_killed = ship_mind.who_killed};
            ship.special() =
                MindData{.progenitor = ship_mind.progenitor,
                         .target = ship_mind.target,
                         .generation = ship_mind.generation,
                         .busy = static_cast<unsigned char>(random() & 01),
                         .tampered = ship_mind.tampered,
                         .who_killed = ship_mind.who_killed};
          }
        }
      }
    } else { /* orbiting a planet */
      if (std::holds_alternative<MindData>(ship.special()) &&
          std::get<MindData>(ship.special()).busy) {
        if (ship.whatdest() == ScopeLevel::LEVEL_PLAN &&
            ship.deststar() == ship.storbits() &&
            ship.destpnum() == ship.pnumorbits()) {
          if (planet.type() == PlanetType::GASGIANT) {
            if (std::holds_alternative<MindData>(ship.special())) {
              auto mind = std::get<MindData>(ship.special());
              mind.busy = 0;
              ship.special() = mind;
            }
          } else {
            /* find a place on the planet to land */
            bool found = false;
            for (auto shuffled = smap.shuffle(); auto& sector_wrap : shuffled) {
              Sector& sect = sector_wrap;
              if (sect.get_resource() == 0) continue;
              found = true;
              ship.docked() = 1;
              ship.whatdest() = ScopeLevel::LEVEL_PLAN;
              ship.deststar() = ship.storbits();
              ship.destpnum() = ship.pnumorbits();
              ship.xpos() = stars[ship.storbits()].xpos() + planet.xpos();
              ship.ypos() = stars[ship.storbits()].ypos() + planet.ypos();
              ship.land_x() = sect.get_x();
              ship.land_y() = sect.get_y();
              if (std::holds_alternative<MindData>(ship.special())) {
                auto mind = std::get<MindData>(ship.special());
                mind.busy = 1;
                ship.special() = mind;
              }
            }
            if (!found && std::holds_alternative<MindData>(ship.special())) {
              auto mind = std::get<MindData>(ship.special());
              mind.busy = 0;
              ship.special() = mind;
            }
          }
        }
      }
    }
  }
}
