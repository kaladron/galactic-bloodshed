// SPDX-License-Identifier: Apache-2.0

module;

import session;
import gblib;
import notification;
import scnlib;
import std;

#include <strings.h>

module commands;

namespace GB::commands {
/*! Planet vs ship */
void defend(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  ap_t APcount = 1;
  int strength;
  int retal;
  int damage;

  if (!DEFENSE) return;

  /* get the planet from the players current scope */
  if (g.level() != ScopeLevel::LEVEL_PLAN) {
    g.out << "You have to set scope to the planet first.\n";
    return;
  }

  if (argv.size() < 3) {
    g.out << "Syntax: 'defend <ship> <sector> [<strength>]'.\n";
    return;
  }
  const auto& star = *g.entity_manager.peek_star(g.snum());
  if (Governor && star.governor(Playernum - 1) != Governor) {
    g.out << "You are not authorized to do that in this system.\n";
    return;
  }
  auto toshiptmp = string_to_shipnum(argv[1]);
  if (!toshiptmp || *toshiptmp <= 0) {
    g.out << "Bad ship number.\n";
    return;
  }
  auto toship = *toshiptmp;

  if (!enufAP(Playernum, Governor, star.AP(Playernum - 1), APcount)) {
    return;
  }

  auto planet_handle = g.entity_manager.get_planet(g.snum(), g.pnum());
  if (!planet_handle.get()) {
    g.out << "Planet not found.\n";
    return;
  }
  auto& p = *planet_handle;

  if (!p.info(Playernum - 1).numsectsowned) {
    g.out << "You do not occupy any sectors here.\n";
    return;
  }

  if (p.slaved_to() && p.slaved_to() != Playernum) {
    g.out << "This planet is enslaved.\n";
    return;
  }

  auto to_handle = g.entity_manager.get_ship(toship);
  if (!to_handle.get()) {
    g.out << "Ship not found.\n";
    return;
  }
  auto* to = to_handle.get();

  if (to->whatorbits() != ScopeLevel::LEVEL_PLAN) {
    g.out << "The ship is not in planet orbit.\n";
    return;
  }

  if (to->storbits() != g.snum() || to->pnumorbits() != g.pnum()) {
    g.out << "Target is not in orbit around this planet.\n";
    return;
  }

  if (landed(*to)) {
    g.out << "Planet guns can't fire on landed ships.\n";
    return;
  }

  /* save defense strength for retaliation */
  // Calculate retaliation strength BEFORE damage is applied.
  // This pre-damage strength will be used if the ship retaliates,
  // even though the ship itself will be modified by taking damage.
  retal = check_retal_strength(*to);

  auto xy_result = scn::scan<int, int>(argv[2], "{},{}");
  if (!xy_result) {
    g.out << "Bad format for sector.\n";
    return;
  }
  auto [x, y] = xy_result->values();

  if (x < 0 || x > p.Maxx() - 1 || y < 0 || y > p.Maxy() - 1) {
    g.out << "Illegal sector.\n";
    return;
  }

  /* check to see if you own the sector */
  auto smap_handle = g.entity_manager.get_sectormap(g.snum(), g.pnum());
  if (!smap_handle.get()) {
    g.out << "Sector map not found.\n";
    return;
  }
  auto& smap = *smap_handle;
  auto& sect = smap.get(x, y);
  if (sect.get_owner() != Playernum) {
    g.out << "Nice try.\n";
    return;
  }

  if (argv.size() >= 4)
    strength = std::stoi(argv[3]);
  else
    strength = p.info(Playernum - 1).guns;

  strength = MIN(strength, p.info(Playernum - 1).destruct);
  strength = MIN(strength, p.info(Playernum - 1).guns);

  if (strength <= 0) {
    g.out << std::format("No attack - {} guns, {}d\n",
                         p.info(Playernum - 1).guns,
                         p.info(Playernum - 1).destruct);
    return;
  }

  // Need mutable race for shoot_planet_to_ship
  auto race_handle = g.entity_manager.get_race(Playernum);
  if (!race_handle.get()) {
    g.out << "Race not found.\n";
    return;
  }
  auto& race = *race_handle;

  char long_buf[1024], short_buf[256];
  damage = shoot_planet_to_ship(g.entity_manager, race, *to, strength, long_buf,
                                short_buf);

  if (damage < 0) {
    g.out << std::format("Target out of range  {}!\n", SYSTEMSIZE);
    return;
  }

  p.info(Playernum - 1).destruct -= strength;
  if (!to->alive()) post(g.entity_manager, short_buf, NewsType::COMBAT);
  notify_star(get_session_registry(g), g.entity_manager, Playernum, Governor,
              to->storbits(), short_buf);
  warn_player(get_session_registry(g), to->owner(), to->governor(), long_buf);
  g.out << long_buf;

  /* defending ship retaliates */

  strength = 0;
  if (retal && damage && to->protect().self) {
    // Use pre-damage retaliation strength (saved in 'retal' above).
    // shoot_ship_to_planet() uses the explicit strength parameter,
    // not the ship's current damage state, so this correctly applies
    // the ship's original (pre-damage) attack capability.
    strength = retal;
    if (laser_on(*to)) check_overload(g.entity_manager, *to, 0, &strength);

    auto result = shoot_ship_to_planet(g.entity_manager, *to, p, strength, x, y,
                                       smap, 0, 0, long_buf, short_buf);
    if (result.numdest < 0) {
      if (laser_on(*to))
        use_fuel(*to, 2.0 * (double)strength);
      else
        use_destruct(*to, strength);

      post(g.entity_manager, short_buf, NewsType::COMBAT);
      notify_star(get_session_registry(g), g.entity_manager, Playernum,
                  Governor, to->storbits(), short_buf);
      g.out << long_buf;
      warn_player(get_session_registry(g), to->owner(), to->governor(),
                  long_buf);
    }
  }

  /* protecting ships retaliate individually if damage was inflicted */
  if (damage) {
    const ShipList shiplist(g.entity_manager, p.ships());
    for (const Ship* ship : shiplist) {
      if (ship->protect().on && (ship->protect().ship == toship) &&
          ship->number() != toship && ship->alive() && ship->active()) {
        strength = check_retal_strength(*ship);
        if (laser_on(*ship))
          check_overload(g.entity_manager, const_cast<Ship&>(*ship), 0,
                         &strength);

        auto result2 =
            shoot_ship_to_planet(g.entity_manager, *ship, p, strength, x, y,
                                 smap, 0, 0, long_buf, short_buf);
        if (result2.numdest >= 0) {
          auto ship_mut_handle = g.entity_manager.get_ship(ship->number());
          if (!ship_mut_handle.get()) {
            continue;
          }
          auto& ship_mut = *ship_mut_handle;
          if (laser_on(*ship))
            use_fuel(ship_mut, 2.0 * (double)strength);
          else
            use_destruct(ship_mut, strength);
          post(g.entity_manager, short_buf, NewsType::COMBAT);
          notify_star(get_session_registry(g), g.entity_manager, Playernum,
                      Governor, ship->storbits(), short_buf);
          g.out << long_buf;
          warn_player(get_session_registry(g), ship->owner(), ship->governor(),
                      long_buf);
        }
      }
    }
  }

  deductAPs(g, APcount, g.snum());
}
}  // namespace GB::commands
