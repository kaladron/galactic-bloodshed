// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import scnlib;
import std;

#include "gb/GB_server.h"

module commands;

namespace {

/**
 * @brief Land a friendly ship onto another ship or planet.
 *
 * This function allows a friendly ship to land onto another ship or planet.
 * It performs various checks to ensure the legality of the landing operation.
 * If the landing is successful, the ship is loaded onto the target ship and
 * the necessary updates are made to the game state.
 *
 * @param argv The command arguments.
 * @param g The GameObj representing the game state.
 * @param s A Ship object representing the ship to be landed.
 */
void land_friendly(const command_t& argv, GameObj& g, Ship& s) {
  double fuel;
  double Dist;

  auto ship2tmp = string_to_shipnum(argv[2]);
  if (!ship2tmp) {
    g.out << std::format("Ship {} wasn't found.\n", argv[2]);
    return;
  }

  const auto* s2_check = g.entity_manager.peek_ship(*ship2tmp);
  if (!s2_check) {
    g.out << std::format("Ship #{} wasn't found.\n", *ship2tmp);
    return;
  }

  auto ship2no = *ship2tmp;
  if (testship(*s2_check, g)) {
    g.out << "Illegal format.\n";
    return;
  }
  if (s2_check->type() == ShipType::OTYPE_FACTORY) {
    g.out << "Can't land on factories.\n";
    return;
  }
  if (landed(s)) {
    if (!landed(*s2_check)) {
      g.out << std::format("{} is not landed on a planet.\n",
                           ship_to_string(*s2_check));
      return;
    }
    if (s2_check->storbits() != s.storbits()) {
      g.out << "These ships are not in the same star system.\n";
      return;
    }
    if (s2_check->pnumorbits() != s.pnumorbits()) {
      g.out << "These ships are not landed on the same planet.\n";
      return;
    }
    if ((s2_check->land_x() != s.land_x()) ||
        (s2_check->land_y() != s.land_y())) {
      g.out << "These ships are not in the same sector.\n";
      return;
    }
    if (s.on()) {
      g.out << std::format("{} must be turned off before loading.\n",
                           ship_to_string(s));
      return;
    }
    if (size(s) > hanger(*s2_check)) {
      g.out << std::format(
          "Mothership does not have {} hanger space available to load ship.\n",
          size(s));
      return;
    }
    /* ok, load 'em up */
    remove_sh_plan(g.entity_manager, s);
    /* get the target ship with write access for modification */
    auto s2_handle = g.entity_manager.get_ship(ship2no);
    if (!s2_handle.get()) {
      g.out << "This shouldn't happen: Target ship no longer exists.\n";
      return;
    }
    auto& s2 = *s2_handle;
    insert_sh_ship(&s, &s2);
    /* increase mass of mothership */
    s2.mass() += s.mass();
    s2.hanger() += size(s);
    fuel = 0.0;
    g.out << std::format("{} loaded onto {} using {} fuel.\n",
                         ship_to_string(s), ship_to_string(s2), fuel);
    s.docked() = 1;
  } else if (s.docked()) {
    g.out << std::format("{} is already docked or landed.\n",
                         ship_to_string(s));
    return;
  } else {
    /* Check if the ships are in the same scope level. Maarten */
    if (s.whatorbits() != s2_check->whatorbits()) {
      g.out << "Those ships are not in the same scope.\n";
      return;
    }

    /* check to see if close enough to land */
    Dist = std::hypot(s2_check->xpos() - s.xpos(), s2_check->ypos() - s.ypos());
    if (Dist > DIST_TO_DOCK) {
      g.out << std::format("{} must be {} or closer to {}.\n",
                           ship_to_string(s), DIST_TO_DOCK,
                           ship_to_string(*s2_check));
      return;
    }
    fuel = 0.05 + Dist * 0.025 * std::sqrt(s.mass());
    if (s.fuel() < fuel) {
      g.out << "Not enough fuel.\n";
      return;
    }
    if (size(s) > hanger(*s2_check)) {
      g.out << std::format(
          "Mothership does not have {} hanger space available to load ship.\n",
          size(s));
      return;
    }
    use_fuel(s, fuel);

    /* remove the ship from whatever scope it is currently in */
    if (s.whatorbits() == ScopeLevel::LEVEL_PLAN)
      remove_sh_plan(g.entity_manager, s);
    else if (s.whatorbits() == ScopeLevel::LEVEL_STAR)
      remove_sh_star(g.entity_manager, s);
    else {
      g.out << "Ship is not in planet or star scope.\n";
      return;
    }
    /* get the target ship with write access for modification */
    auto s2_handle = g.entity_manager.get_ship(ship2no);
    if (!s2_handle.get()) {
      g.out << "This shouldn't happen: Target ship no longer exists.\n";
      return;
    }
    auto& s2 = *s2_handle;
    insert_sh_ship(&s, &s2);
    /* increase mass of mothership */
    s2.mass() += s.mass();
    s2.hanger() += size(s);
    g.out << std::format("{} landed on {} using {} fuel.\n", ship_to_string(s),
                         ship_to_string(s2), fuel);
    s.docked() = 1;
  }
}

/**
 * @brief Lands a ship on a planet.
 *
 * This function is responsible for landing a ship on a planet. It performs
 * various checks and calculations to determine if the landing is possible and
 * handles the landing process accordingly. The ship must meet certain
 * conditions, such as being equipped to land and being in orbit around the
 * planet it intends to land on. The function also checks for the distance
 * between the ship and the planet, ensuring it is within a specified range for
 * landing. If the ship meets all the requirements, it will be landed on the
 * specified coordinates on the planet's surface.
 *
 * @param argv The command arguments.
 * @param g The GameObj representing the game state.
 * @param s The Ship object representing the ship to be landed.
 * @param APcount The number of action points available for the player.
 */
void land_planet(const command_t& argv, GameObj& g, Ship& s, ap_t APcount) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  int numdest = 0;
  int strength;
  double fuel;
  double Dist;

  if (s.docked()) {
    g.out << std::format("{} is docked.\n", ship_to_string(s));
    return;
  }
  auto xy_result = scn::scan<int, int>(argv[2], "{},{}");
  if (!xy_result) {
    g.out << "Invalid coordinates format. Use: x,y\n";
    return;
  }
  auto [x, y] = xy_result->values();
  if (s.whatorbits() != ScopeLevel::LEVEL_PLAN) {
    g.out << std::format("{} doesn't orbit a planet.\n", ship_to_string(s));
    return;
  }
  if (!Shipdata[s.type()][ABIL_CANLAND]) {
    g.out << "This ship is not equipped to land.\n";
    return;
  }
  if ((s.storbits() != g.snum()) || (s.pnumorbits() != g.pnum())) {
    g.out << "You have to cs to the planet it orbits.\n";
    return;
  }
  if (!speed_rating(s)) {
    g.out << "This ship is not rated for maneuvering.\n";
    return;
  }

  const auto* star = g.entity_manager.peek_star(s.storbits());
  if (!star) {
    g.out << "Star system not found.\n";
    return;
  }

  if (!enufAP(Playernum, Governor, star->AP(Playernum - 1), APcount)) {
    return;
  }

  auto planet_handle =
      g.entity_manager.get_planet(s.storbits(), s.pnumorbits());
  if (!planet_handle.get()) {
    g.out << "Planet not found.\n";
    return;
  }
  auto& p = *planet_handle;

  g.out << std::format("Planet /{}/{} has gravity field of {:.2f}.\n",
                       star->get_name(), star->get_planet_name(s.pnumorbits()),
                       p.gravity());

  Dist = std::hypot((star->xpos() + p.xpos()) - s.xpos(),
                    (star->ypos() + p.ypos()) - s.ypos());
  g.out << std::format("Distance to planet: {:.2f}.\n", Dist);

  if (Dist > DIST_TO_LAND) {
    g.out << std::format(
        "{} must be {:.3g} or closer to the planet ({:.2f}).\n",
        ship_to_string(s), DIST_TO_LAND, Dist);
    return;
  }

  fuel = s.mass() * p.gravity() * LAND_GRAV_MASS_FACTOR;

  if ((x < 0) || (y < 0) || (x > p.Maxx() - 1) || (y > p.Maxy() - 1)) {
    g.out << "Illegal coordinates.\n";
    return;
  }

  if (DEFENSE) {
    /* people who have declared war on you will fire at your landing ship
     */
    for (auto alien_race_handle : RaceList(g.entity_manager)) {
      const auto i = alien_race_handle->Playernum;
      if (s.alive() && i != Playernum && p.info(i - 1).popn &&
          p.info(i - 1).guns && p.info(i - 1).destruct) {
        if (isset(alien_race_handle->atwar, s.owner())) {
          /* attack the landing ship - need mutable access for shoot function */
          auto alien_handle = g.entity_manager.get_race(i);
          if (!alien_handle.get()) continue;
          auto& alien = *alien_handle;
          /* attack the landing ship */
          strength = MIN((int)p.info(i - 1).guns, (int)p.info(i - 1).destruct);
          if (strength) {
            char long_buf[1024], short_buf[256];
            shoot_planet_to_ship(g.entity_manager, alien, s, strength, long_buf,
                                 short_buf);
            post(g.entity_manager, short_buf, NewsType::COMBAT);
            notify_star(g.entity_manager, 0, 0, s.storbits(), short_buf);
            warn(i, star->governor(i - 1), long_buf);
            notify(s.owner(), s.governor(), long_buf);
            p.info(i - 1).destruct -= strength;
          }
        }
      }
    }
    if (!s.alive()) {
      return;
    }
  }
  /* check to see if the ship crashes from lack of fuel or damage */
  if (auto [did_crash, roll] = crash(s, fuel); did_crash) {
    /* damaged ships stand of chance of crash landing */
    auto smap_handle =
        g.entity_manager.get_sectormap(s.storbits(), s.pnumorbits());
    auto& smap = *smap_handle;
    char long_buf[1024], short_buf[256];
    auto result = shoot_ship_to_planet(
        g.entity_manager, s, p, round_rand((double)(s.destruct()) / 3.), x, y,
        smap, 0, GTYPE_HEAVY, long_buf, short_buf);
    numdest = result.numdest;
    auto buf = std::format(
        "BOOM!! {} crashes on sector {},{} with blast radius of {}.\n",
        ship_to_string(s), x, y, numdest);
    for (auto race_handle : RaceList(g.entity_manager)) {
      const auto i = race_handle->Playernum;
      if (p.info(i - 1).numsectsowned || i == Playernum)
        warn(i, star->governor(i - 1), buf);
    }
    if (roll)
      g.out << std::format("Ship damage {}% (you rolled a {})\n",
                           (int)s.damage(), roll);
    else
      g.out << std::format(
          "You had {:.1f}f while the landing required {:.1f}f\n", s.fuel(),
          fuel);
    g.entity_manager.kill_ship(s.owner(), s);
  } else {
    // Normal landing
    auto smap_handle =
        g.entity_manager.get_sectormap(s.storbits(), s.pnumorbits());
    auto& smap = *smap_handle;

    s.land_x() = x;
    s.land_y() = y;
    s.xpos() = p.xpos() + star->xpos();
    s.ypos() = p.ypos() + star->ypos();
    use_fuel(s, fuel);
    s.docked() = 1;
    s.whatdest() = ScopeLevel::LEVEL_PLAN; /* no destination */
    s.deststar() = s.storbits();
    s.destpnum() = s.pnumorbits();
  }

  auto smap_handle =
      g.entity_manager.get_sectormap(s.storbits(), s.pnumorbits());
  auto& smap = *smap_handle;
  auto& sector = smap.get(x, y);

  if (sector.is_wasted()) {
    g.out << "Warning: That sector is a wasteland!\n";
  } else if (sector.get_owner() && sector.get_owner() != Playernum) {
    // Use g.race for current player (already set by process_command)
    const auto* alien = g.entity_manager.peek_race(sector.get_owner());
    if (!alien) return;  // Should never happen but be safe
    if (!(isset(g.race->allied, sector.get_owner()) &&
          isset(alien->allied, Playernum))) {
      g.out << std::format("You have landed on an alien sector ({}).\n",
                           alien->name);
    } else {
      g.out << std::format("You have landed on allied sector ({}).\n",
                           alien->name);
    }
  }

  if (s.whatorbits() == ScopeLevel::LEVEL_UNIV)
    deductAPs(g, APcount, ScopeLevel::LEVEL_UNIV);
  else
    deductAPs(g, APcount, s.storbits());

  /* send messages to anyone there */
  auto landing_msg =
      std::format("{} observed landing on sector {},{},planet /{}/{}.\n",
                  ship_to_string(s), s.land_x(), s.land_y(), star->get_name(),
                  star->get_planet_name(s.pnumorbits()));
  for (auto race_handle : RaceList(g.entity_manager)) {
    const auto i = race_handle->Playernum;
    if (p.info(i - 1).numsectsowned && i != Playernum) {
      notify(i, star->governor(i - 1), landing_msg);
    }
  }
  g.out << std::format("{} landed on planet.\n", ship_to_string(s));
}
}  // namespace

namespace GB::commands {
void land(const command_t& argv, GameObj& g) {
  governor_t Governor = g.governor();
  ap_t APcount = 1;

  if (argv.size() < 2) {
    g.out << "Land what?\n";
    return;
  }

  ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);

  for (auto ship_handle : ships) {
    Ship& s = *ship_handle;  // Get mutable access upfront

    if (!GB::ship_matches_filter(argv[1], s)) continue;
    if (!authorized(Governor, s)) continue;

    if (overloaded(s)) {
      g.out << std::format("{} is too overloaded to land.\n",
                           ship_to_string(s));
      continue;
    }
    if (s.type() == ShipType::OTYPE_QUARRY) {
      g.out << "You can't load quarries onto ship.\n";
      continue;
    }
    if (docked(s)) {
      g.out << "That ship is docked to another ship.\n";
      continue;
    }

    /* attempting to land on a friendly ship (for carriers/stations/etc) */
    if (argv[2][0] == '#') {
      land_friendly(argv, g, s);
      continue;
    } else { /* attempting to land on a planet */
      land_planet(argv, g, s, APcount);
      continue;
    }
  }
}
}  // namespace GB::commands
