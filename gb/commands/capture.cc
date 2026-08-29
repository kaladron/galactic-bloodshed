// SPDX-License-Identifier: Apache-2.0

/// \file capture.cc
/// \brief Capture ship command.

module;

import std;
import gb.entities;
import gb.services;
import notification;
import session;

module commands;

namespace GB::commands {
bool capture(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  const governor_t Governor = g.governor();
  const ap_t APcount = 1;
  player_t oldowner;
  governor_t oldgov;
  int shipdam = 0;
  int booby = 0;
  PopulationType what;
  population_t olddpopn;
  population_t olddtroops;
  population_t casualties = 0;
  population_t casualties1 = 0;
  population_t casualties2 = 0;
  population_t casualty_scale = 0;
  double astrength;
  double dstrength;
  bool any_captured = false;

  if (argv.size() < 2) {
    g.out << "Capture what?\n";
    return false;
  }
  if (Governor != 0 &&
      g.entity_manager.peek_star(g.snum())->governor(Playernum) != Governor) {
    g.out << "You are not authorized in this system.\n";
    return false;
  }

  ShipList shiplist(g.entity_manager, g);
  for (auto ship_handle : shiplist) {
    Ship& ship = *ship_handle;
    shipnum_t shipno = ship.number();
    if (ship.owner() != Playernum) {
      if (!landed(ship)) {
        g.out << std::format("{} #{} is not landed on a planet.\n",
                             Shipnames[ship.type()], shipno);

        continue;
      }
      if (ship.type() == ShipType::OTYPE_VN) {
        g.out << "You can't capture Von Neumann machines.\n";

        continue;
      }
      if (!g.deduct_ap(ship.storbits(), APcount)) {
        g.out << std::format("You don't have {} action points there.\n",
                             APcount);
        continue;
      }

      bool owns_sector = g.entity_manager.with_sectormap(
          ship.storbits(), ship.pnumorbits(), [&](const SectorMap& smap) {
            return smap.get(ship.land_coords()).get_owner() == Playernum;
          });
      if (!owns_sector) {
        g.entity_manager.with_sectormap(
            ship.storbits(), ship.pnumorbits(), [&](const SectorMap& smap) {
              g.out << std::format(
                  "You don't own the sector where the ship is landed [{}].\n",
                  smap.get(ship.land_coords()).get_owner());
            });
        continue;
      }

      if (argv.size() < 4)
        what = PopulationType::CIV;
      else if (argv[3] == "civilians")
        what = PopulationType::CIV;
      else if (argv[3] == "military")
        what = PopulationType::MIL;
      else {
        g.out << "Capture with what?\n";
        continue;
      }

      population_t available_boarders = 0;
      g.entity_manager.with_sectormap(
          ship.storbits(), ship.pnumorbits(), [&](const SectorMap& smap) {
            const auto& sect = smap.get(ship.land_coords());
            available_boarders = (what == PopulationType::CIV)
                                     ? sect.get_popn()
                                     : sect.get_troops();
          });

      population_t boarders;
      if (argv.size() < 3) {
        boarders = available_boarders;
      } else
        boarders = std::stoul(argv[2]);

      if (boarders == 0) {
        g.out << "Illegal number of boarders.\n";
        continue;
      }

      boarders = std::min(boarders, available_boarders);

      const auto* race_ptr =
          g.race ? g.race : g.entity_manager.peek_race(Playernum);
      const auto& race = *race_ptr;
      const auto* alien = g.entity_manager.peek_race(ship.owner());
      if (!alien) {
        g.out << "Ship owner race not found.\n";
        continue;
      }

      if (isset(race.allied, (ship.owner()))) {
        g.session_registry.notify_player(
            Playernum, Governor,
            std::format("Boarding the ship of your ally, {}\n", alien->name));
      }

      olddpopn = ship.popn();
      olddtroops = ship.troops();
      oldowner = ship.owner();
      oldgov = ship.governor();

      shipdam = 0;
      casualties = 0;
      casualties1 = 0;
      casualties2 = 0;

      player_t sect_owner_display = 0;
      g.entity_manager.mutate_sectormap(
          ship.storbits(), ship.pnumorbits(), [&](SectorMap& smap) {
            auto& sect = smap.get(ship.land_coords());
            if (what == PopulationType::CIV)
              sect.subtract_popn(boarders);
            else if (what == PopulationType::MIL)
              sect.set_troops(sect.get_troops() - boarders);

            if (olddpopn + olddtroops) {
              g.session_registry.notify_player(
                  Playernum, Governor,
                  std::format(
                      "Attack strength: {:.2f}     Defense strength: {:.2f}\n",
                      astrength =
                          (double)boarders *
                          (what == PopulationType::MIL
                               ? (double)race.fighters * 10.0
                               : 1.0) *
                          .01 * race.tech *
                          (race.likes[sect.get_condition()] + 0.01) *
                          ((double)Defensedata[sect.get_condition()] + 1.0) *
                          morale_factor((double)(race.morale - alien->morale)),
                      dstrength =
                          ((double)ship.popn() + (double)ship.troops() * 10.0 *
                                                     (double)alien->fighters) *
                          .01 * alien->tech * ((double)(armor(ship)) + 0.01) *
                          .01 * (100.0 - (double)ship.damage()) *
                          morale_factor(
                              (double)(alien->morale - race.morale))));
              casualty_scale = std::min(boarders, ship.popn() + ship.troops());
              if (astrength > 0.0)
                casualties = int_rand(
                    0, round_rand((double)casualty_scale * (dstrength + 1.0) /
                                  (astrength + 1.0)));

              if (dstrength > 0.0) {
                casualties1 = int_rand(
                    0, round_rand((double)casualty_scale * (astrength + 1.0) /
                                  (dstrength + 1.0)));
                casualties2 = int_rand(
                    0, round_rand((double)casualty_scale * (astrength + 1.0) /
                                  (dstrength + 1.0)));
                shipdam = int_rand(
                    0, round_rand(25. * (astrength + 1.0) / (dstrength + 1.0)));
                ship.damage() = std::min(100U, ship.damage() + shipdam);
              }

              casualties = std::min(boarders, casualties);
              boarders -= casualties;

              casualties1 = std::min(olddpopn, casualties1);
              ship.popn() -= casualties1;
              ship.mass() -= casualties1 * alien->mass;

              casualties2 = std::min(olddtroops, casualties2);
              ship.troops() -= casualties2;
              ship.mass() -= casualties2 * alien->mass;

            } else if (ship.destruct()) { /* booby trapped robot ships */
              booby = int_rand(0, 10 * ship.destruct());
              booby = std::min(100, booby);
              casualties = casualties2 = 0;
              for (unsigned long i = 0; i < boarders; i++)
                casualties += (int_rand(1, 100) < booby);
              boarders -= casualties;
              shipdam += booby;
              ship.damage() += booby;
            }
            shipdam = std::min(100, shipdam);
            if (ship.damage() >= 100)
              g.entity_manager.kill_ship(Playernum, ship);

            if (!(ship.popn() + ship.troops()) && ship.alive()) {
              /* we got 'em */
              ship.owner() = Playernum;
              ship.governor() = Governor;
              if (what == PopulationType::CIV) {
                ship.popn() = std::min(boarders, max_crew(ship));
                sect.add_popn(boarders -
                              ship.popn());  // Return excess boarders
                ship.mass() += ship.popn() * race.mass;
              } else if (what == PopulationType::MIL) {
                ship.troops() = std::min(boarders, max_mil(ship));
                sect.set_troops(sect.get_troops() + boarders - ship.troops());
                ship.mass() += ship.troops() * race.mass;
              }
              if (olddpopn + olddtroops &&
                  ship.type() != ShipType::OTYPE_FACTORY) {
                g.entity_manager.mutate_race(Playernum, [&](Race& r) {
                  g.entity_manager.mutate_race(oldowner, [&](Race& a) {
                    adjust_morale(r, a, ship.build_cost());
                  });
                });
              }
              /* unoccupied ships and factories don't count */
            } else { /* retreat */
              if (what == PopulationType::CIV)
                sect.add_popn(boarders);
              else if (what == PopulationType::MIL)
                sect.set_troops(sect.get_troops() + boarders);
            }

            sect.clear_owner_if_empty();
            sect_owner_display = sect.get_owner();
          });

      const auto& star = *g.entity_manager.peek_star(ship.storbits());
      std::string telegram =
          std::format("BULLETIN from {}/{}!!\n", star.get_name(),
                      star.get_planet_name(ship.pnumorbits()));
      telegram += std::format(
          "You are being attacked by{} Player #{} ({})!!!\n",
          (isset(alien->allied, Playernum)
               ? " your ally"
               : (isset(alien->atwar, Playernum) ? " your enemy" : " neutral")),
          Playernum, race.name);
      telegram += std::format("{} at sector {} [owner {}] !\n", ship,
                              ship.land_coords(), sect_owner_display);

      if (booby) {
        telegram +=
            std::format("Booby trap triggered causing {}% damage.\n", booby);
        g.session_registry.notify_player(
            Playernum, Governor,
            std::format("Booby trap triggered causing {}% damage.\n", booby));
      }

      if (shipdam) {
        telegram += std::format("Total damage: {}% (now {}%)\n", shipdam,
                                ship.damage());
        g.out << std::format("Damage inflicted:  Them: {}% (now {}%)\n",
                             shipdam, ship.damage());
      }

      if (!ship.alive()) {
        telegram += "              YOUR SHIP WAS DESTROYED!!!\n";
        g.out << "              Their ship DESTROYED!!!\n";
        auto short_buf = std::format("{}: {} [{}] DESTROYED {}\n",
                                     dispshiploc(g.entity_manager, ship),
                                     race.name, Playernum, ship);
      }

      if (ship.owner() == Playernum) {
        g.session_registry.notify_player(oldowner, oldgov,
                                         std::format("{} CAPTURED!\n", ship));
        g.out << "VICTORY! The ship is yours!\n";
        if (what == PopulationType::CIV)
          g.out << std::format("{} boarders move in.\n",
                               std::min(boarders, ship.popn()));
        else if (what == PopulationType::MIL)
          g.out << std::format("{} troops move in.\n",
                               std::min(boarders, ship.troops()));
        capture_stuff(ship, g);
        auto short_buf = std::format("{}: {} [{}] CAPTURED {}\n",
                                     dispshiploc(g.entity_manager, ship),
                                     race.name, Playernum, ship);
      } else if (ship.popn() + ship.troops()) {
        g.session_registry.notify_player(oldowner, oldgov,
                                         "You fought them off!\n");
        g.out << "The boarding was repulsed; try again.\n";
        auto short_buf = std::format("{}: {} [{}] assaults {}\n",
                                     dispshiploc(g.entity_manager, ship),
                                     race.name, Playernum, ship);
      }
      if (ship.alive()) {
        if (!boarders) {
          g.out << "Oh no! They killed your party to the last man!\n";
        }
      } else {
        telegram += "Your ship was weakened too much!\n";
        g.out << "The assault weakened their ship too much!\n";
      }

      if (casualties || casualties1 || casualties2) {
        telegram +=
            std::format("Casualties: Yours: {} civ/{} mil, Theirs: {} {}\n",
                        casualties1, casualties2, casualties,
                        what == PopulationType::CIV ? "civ" : "mil");
        g.out << std::format(
            "Casualties: Yours: {} {}, Theirs: {} civ/{} mil\n", casualties,
            what == PopulationType::CIV ? "civ" : "mil", casualties1,
            casualties2);
      }
      warn_player(g.session_registry, g.entity_manager, oldowner, oldgov,
                  telegram);
      if (ship.owner() != oldowner || !ship.alive()) {
        auto short_msg = std::format(
            "{}: {} [{}] {} {}\n", dispshiploc(g.entity_manager, ship),
            race.name, Playernum, (ship.alive() ? "assaults" : "DESTROYED"),
            ship);
        post(g.entity_manager, short_msg, NewsType::COMBAT);
        notify_star(g.session_registry, g.entity_manager, Playernum, Governor,
                    ship.storbits(), short_msg);
      }
      any_captured = true;
    }
  }
  return any_captured;
}

const CommandDescriptor capture_cmd{
    .name = "capture",
    .roles =
        {
            .star_control = true,
        },
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::dynamic(),
    .min_args = 2,
    .syntax = "capture <ship> [<number>] [civilians|military]",
    .description = "Attempt to capture a landed enemy ship with ground forces",
    .handler = &capture,
};

}  // namespace GB::commands
