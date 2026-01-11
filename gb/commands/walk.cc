// SPDX-License-Identifier: Apache-2.0

module;

import session;
import gblib;
import notification;
import std;

module commands;

namespace GB::commands {
void walk(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  const governor_t Governor = g.governor();
  const ap_t APcount = 1;

  char long_buf[1024], short_buf[256];

  if (argv.size() < 2) {
    g.out << "Walk what?\n";
    return;
  }
  auto shipno = string_to_shipnum(argv[1]);
  if (!shipno || *shipno <= 0) {
    g.out << "Bad ship number.\n";
    return;
  }
  auto ship_handle = g.entity_manager.get_ship(*shipno);
  if (!ship_handle.get()) {
    g.out << "No such ship.\n";
    return;
  }
  auto* ship = ship_handle.get();
  if (testship(*ship, g)) {
    g.out << "You do not control this ship.\n";
    return;
  }
  if (ship->type() != ShipType::OTYPE_AFV) {
    g.out << "This ship doesn't walk!\n";
    return;
  }
  if (!landed(*ship)) {
    g.out << "This ship is not landed on a planet.\n";
    return;
  }
  if (!ship->popn()) {
    g.out << "No crew.\n";
    return;
  }
  if (ship->fuel() < AFV_FUEL_COST) {
    g.out << std::format("You don't have {:.1f} fuel to move it.\n",
                         AFV_FUEL_COST);
    return;
  }

  // Use get_star to keep star alive for entire function
  auto star_handle = g.entity_manager.get_star(ship->storbits());
  if (!star_handle.get()) {
    g.out << "Star not found.\n";
    return;
  }
  const auto& star = star_handle.read();

  if (!enufAP(g.entity_manager, Playernum, Governor, star.AP(Playernum),
              APcount)) {
    return;
  }
  auto planet_handle =
      g.entity_manager.get_planet(ship->storbits(), ship->pnumorbits());
  if (!planet_handle.get()) {
    g.out << "Planet not found.\n";
    return;
  }
  auto& p = *planet_handle;

  auto [x, y] = get_move(p, argv[2][0], {ship->land_x(), ship->land_y()});
  if (ship->land_x() == x && ship->land_y() == y) {
    g.out << "Illegal move.\n";
    return;
  }
  if (x < 0 || y < 0 || x > p.Maxx() - 1 || y > p.Maxy() - 1) {
    g.out << std::format("Illegal coordinates {},{}.\n", x, y);
    return;
  }
  auto smap_handle =
      g.entity_manager.get_sectormap(ship->storbits(), ship->pnumorbits());
  if (!smap_handle.get()) {
    g.out << "Sector map not found.\n";
    return;
  }
  auto& smap = *smap_handle;
  /* check to see if player is permited on the sector type */
  auto& sect = smap.get(x, y);
  if (!g.race->likes[sect.get_condition()]) {
    g.out << "Your ships cannot walk into that sector type!\n";
    return;
  }
  /* if the sector is occupied by non-aligned AFVs, each one will attack */
  ShipList shiplist(g.entity_manager, p.ships());
  for (auto ship_handle : shiplist) {
    Ship& ship2 = *ship_handle;
    if (ship2.owner() != Playernum && ship2.type() == ShipType::OTYPE_AFV &&
        landed(ship2) && retal_strength(ship2) && (ship2.land_x() == x) &&
        (ship2.land_y() == y)) {
      auto alien_handle = g.entity_manager.get_race(ship2.owner());
      if (!alien_handle.get()) {
        continue;
      }
      auto& alien = *alien_handle;
      if (!isset(g.race->allied, ship2.owner()) ||
          !isset(alien.allied, Playernum)) {
        int strength;
        int strength1;
        while ((strength = retal_strength(ship2)) &&
               (strength1 = retal_strength(*ship))) {
          use_destruct(ship2, strength);
          g.out << long_buf;
          warn_player(g.session_registry, g.entity_manager, ship2.owner(),
                      ship2.governor(), long_buf);
          if (!ship2.alive())
            post(g.entity_manager, short_buf, NewsType::COMBAT);
          notify_star(g.session_registry, g.entity_manager, Playernum, Governor,
                      ship->storbits(), short_buf);
          if (strength1) {
            use_destruct(*ship, strength1);
            g.out << long_buf;
            warn_player(g.session_registry, g.entity_manager, ship2.owner(),
                        ship2.governor(), long_buf);
            if (!ship2.alive())
              post(g.entity_manager, short_buf, NewsType::COMBAT);
            notify_star(g.session_registry, g.entity_manager, Playernum,
                        Governor, ship->storbits(), short_buf);
          }
        }
      }
    }
    if (!ship->alive()) break;
  }
  /* if the sector is occupied by non-aligned player, attack them first */
  if (ship->popn() && ship->alive() && sect.get_owner() != 0 &&
      sect.get_owner() != Playernum) {
    auto oldowner = sect.get_owner();
    auto oldgov = star.governor(sect.get_owner());
    const auto* alien = g.entity_manager.peek_race(oldowner);
    if (!alien) return;
    if (!isset(g.race->allied, oldowner) || !isset(alien->allied, Playernum)) {
      if (!retal_strength(*ship)) {
        g.out << "You have nothing to attack with!\n";
        return;
      }
      while ((sect.get_popn() + sect.get_troops()) && retal_strength(*ship)) {
        auto civ = sect.get_popn();
        auto mil = sect.get_troops();
        mech_attack_people(g.entity_manager, *ship, &civ, &mil, *g.race, *alien,
                           sect, false, long_buf, short_buf);
        g.out << long_buf;
        warn_player(g.session_registry, g.entity_manager, alien->Playernum,
                    oldgov, long_buf);
        notify_star(g.session_registry, g.entity_manager, Playernum, Governor,
                    ship->storbits(), short_buf);
        post(g.entity_manager, short_buf, NewsType::COMBAT);

        people_attack_mech(g.entity_manager, *ship, sect.get_popn(),
                           sect.get_troops(), *alien, *g.race, sect, x, y,
                           long_buf, short_buf);
        g.out << long_buf;
        warn_player(g.session_registry, g.entity_manager, alien->Playernum,
                    oldgov, long_buf);
        notify_star(g.session_registry, g.entity_manager, Playernum, Governor,
                    ship->storbits(), short_buf);
        if (!ship->alive()) post(g.entity_manager, short_buf, NewsType::COMBAT);

        sect.set_popn(civ);
        sect.set_troops(mil);
        if (sect.is_empty()) {
          p.info(sect.get_owner()).mob_points -= (int)sect.get_mobilization();
          sect.set_owner(0);
        }
      }
    }
  }

  int succ = 0;
  if ((sect.get_owner() == Playernum ||
       isset(g.race->allied, sect.get_owner()) || sect.get_owner() == 0) &&
      ship->alive())
    succ = 1;

  if (ship->alive() && ship->popn() && succ) {
    std::string moving =
        std::format("{} moving from {},{} to {},{} on {}.\n",
                    ship_to_string(*ship), ship->land_x(), ship->land_y(), x, y,
                    dispshiploc(g.entity_manager, *ship));
    ship->land_x() = x;
    ship->land_y() = y;
    use_fuel(*ship, AFV_FUEL_COST);
    for (player_t i{1}; i <= g.entity_manager.num_races();
         i = player_t{i.value + 1})
      if (i != Playernum && p.info(i).numsectsowned)
        g.session_registry.notify_player(i, star.governor(i), moving);
  }
  deductAPs(g, APcount, ship->storbits());
}
}  // namespace GB::commands
