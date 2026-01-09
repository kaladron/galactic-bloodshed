// SPDX-License-Identifier: Apache-2.0

module;

import session;
import gblib;
import notification;
import scnlib;
import std;

module commands;

namespace GB::commands {
void move_popn(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  const governor_t Governor = g.governor();
  PopulationType what =
      (argv[0] == "move") ? PopulationType::CIV : PopulationType::MIL;
  int Assault;
  int APcost; /* unfriendly movement */
  int casualties;
  int casualties2;
  int casualties3;

  int people;
  int oldpopn;
  int old2popn;
  int old3popn;
  int old2owner;
  governor_t old2gov;
  int absorbed;
  int n;
  int done;
  double astrength;
  double dstrength;

  if (g.level() != ScopeLevel::LEVEL_PLAN) {
    g.out << "Wrong scope\n";
    return;
  }
  const auto& star = *g.entity_manager.peek_star(g.snum());
  if (!star.control(Playernum, Governor)) {
    g.out << "You are not authorized to do that here.\n";
    return;
  }
  auto planet_handle = g.entity_manager.get_planet(g.snum(), g.pnum());
  if (!planet_handle.get()) {
    g.out << "Planet not found.\n";
    return;
  }
  auto& planet = *planet_handle;

  if (planet.slaved_to() > 0 && planet.slaved_to() != Playernum) {
    g.out << "That planet has been enslaved!\n";
    return;
  }
  auto xy_result = scn::scan<int, int>(argv[1], "{},{}");
  if (!xy_result) {
    g.out << "Bad format for sector.\n";
    return;
  }
  auto [x, y] = xy_result->values();
  if (x < 0 || y < 0 || x > planet.Maxx() - 1 || y > planet.Maxy() - 1) {
    g.out << "Origin coordinates illegal.\n";
    return;
  }

  auto smap_handle = g.entity_manager.get_sectormap(g.snum(), g.pnum());
  if (!smap_handle.get()) {
    g.out << "Sector map not found.\n";
    return;
  }
  auto& smap = *smap_handle;

  /* movement loop */
  done = 0;
  n = 0;
  while (!done) {
    auto& sect = smap.get(x, y);
    if (sect.get_owner() != Playernum) {
      g.out << std::format("You don't own sector {},{}!\n", x, y);
      return;
    }
    auto [x2, y2] = get_move(planet, argv[2][n++], {x, y});
    if (x == x2 && y == y2) {
      g.out << "Finished.\n";
      return;
    }

    if (x2 < 0 || y2 < 0 || x2 > planet.Maxx() - 1 || y2 > planet.Maxy() - 1) {
      g.out << std::format("Illegal coordinates {},{}.\n", x2, y2);
      return;
    }

    if (!adjacent(planet, {x, y}, {x2, y2})) {
      g.out << "Illegal move - to adjacent sectors only!\n";
      return;
    }

    /* ok, the move is legal */
    auto& sect2 = smap.get(x2, y2);
    if (argv.size() >= 4) {
      people = std::stoi(argv[3]);
      if (people < 0) {
        if (what == PopulationType::CIV)
          people = sect.get_popn() + people;
        else if (what == PopulationType::MIL)
          people = sect.get_troops() + people;
      }
    } else {
      if (what == PopulationType::CIV)
        people = sect.get_popn();
      else if (what == PopulationType::MIL)
        people = sect.get_troops();
    }

    if ((what == PopulationType::CIV && (std::abs(people) > sect.get_popn())) ||
        (what == PopulationType::MIL &&
         (std::abs(people) > sect.get_troops())) ||
        people <= 0) {
      if (what == PopulationType::CIV)
        g.out << std::format("Bad value - {} civilians in [{},{}]\n",
                             sect.get_popn(), x, y);
      else if (what == PopulationType::MIL)
        g.out << std::format("Bad value - {} troops in [{},{}]\n",
                             sect.get_troops(), x, y);
      return;
    }

    g.out << std::format("{} {} moved.\n", people,
                         what == PopulationType::CIV ? "population" : "troops");

    /* check for defending mechs */
    mech_defend(g, &people, what, planet, x2, y2, sect2);
    if (!people) {
      g.out << "Attack aborted.\n";
      return;
    }

    if (sect2.get_owner() && (sect2.get_owner() != Playernum))
      Assault = 1;
    else
      Assault = 0;

    /* action point cost depends on the size of the group being moved */
    if (what == PopulationType::CIV)
      APcost =
          MOVE_FACTOR * ((int)std::log(1.0 + (double)people) + Assault) + 1;
    else if (what == PopulationType::MIL)
      APcost =
          MOVE_FACTOR * ((int)std::log10(1.0 + (double)people) + Assault) + 1;

    if (!enufAP(g.entity_manager, Playernum, Governor, star.AP(Playernum - 1),
                APcost)) {
      return;
    }

    if (Assault) {
      ground_assaults[Playernum - 1][sect2.get_owner() - 1][g.snum()] += 1;
      auto race_handle = g.entity_manager.get_race(Playernum);
      auto alien_handle = g.entity_manager.get_race(sect2.get_owner());
      if (!race_handle.get() || !alien_handle.get()) {
        continue;
      }
      auto& race = *race_handle;
      auto& alien = *alien_handle;
      /* races find out about each other */
      alien.translate[Playernum - 1] =
          MIN(alien.translate[Playernum - 1] + 5, 100);
      race.translate[sect2.get_owner() - 1] =
          MIN(race.translate[sect2.get_owner() - 1] + 5, 100);

      old2owner = (int)(sect2.get_owner());
      old2gov = star.governor(sect2.get_owner() - 1);
      if (what == PopulationType::CIV)
        sect.set_popn(std::max(0L, sect.get_popn() - people));
      else if (what == PopulationType::MIL)
        sect.set_troops(std::max(0L, sect.get_troops() - people));

      if (what == PopulationType::CIV)
        g.out << std::format("{} civ assault {} civ/{} mil\n", people,
                             sect2.get_popn(), sect2.get_troops());
      else if (what == PopulationType::MIL)
        g.out << std::format("{} mil assault {} civ/{} mil\n", people,
                             sect2.get_popn(), sect2.get_troops());
      oldpopn = people;
      old2popn = sect2.get_popn();
      old3popn = sect2.get_troops();

      auto sect2_popn = sect2.get_popn();
      auto sect2_troops = sect2.get_troops();
      ground_attack(
          race_handle.read(), alien_handle.read(), &people, what, &sect2_popn,
          &sect2_troops, Defensedata[sect.get_condition()],
          Defensedata[sect2.get_condition()],
          race_handle.read().likes[sect.get_condition()],
          alien_handle.read().likes[sect2.get_condition()], &astrength,
          &dstrength, &casualties, &casualties2, &casualties3);

      sect2.set_popn(sect2_popn);
      sect2.set_troops(sect2_troops);

      g.out << std::format("Attack: {:.2f}   Defense: {:.2f}.\n", astrength,
                           dstrength);

      if (sect2.is_empty()) { /* we got 'em */
        sect2.set_owner(Playernum);
        /* mesomorphs absorb the bodies of their victims */
        absorbed = 0;
        if (race.absorb) {
          absorbed = int_rand(0, old2popn + old3popn);
          g.out << std::format("{} alien bodies absorbed.\n", absorbed);
          g.session_registry.notify_player(
              old2owner, old2gov,
              std::format("Metamorphs have absorbed {} bodies!!!\n", absorbed));
        }
        if (what == PopulationType::CIV)
          sect2.set_popn(people + absorbed);
        else if (what == PopulationType::MIL) {
          sect2.set_popn(absorbed);
          sect2.set_troops(people);
        }
        adjust_morale(race, alien, (int)alien.fighters);
      } else { /* retreat */
        absorbed = 0;
        if (alien.absorb) {
          absorbed = int_rand(0, oldpopn - people);
          g.session_registry.notify_player(
              old2owner, old2gov,
              std::format("{} alien bodies absorbed.\n", absorbed));
          g.out << std::format("Metamorphs have absorbed {} bodies!!!\n",
                               absorbed);
          sect2.set_popn(sect2.get_popn() + absorbed);
        }
        if (what == PopulationType::CIV)
          sect.set_popn(sect.get_popn() + people);
        else if (what == PopulationType::MIL)
          sect.set_troops(sect.get_troops() + people);
        adjust_morale(alien, race, (int)race.fighters);
      }

      std::string telegram = std::format(
          "/{}/{}: {} [{}] {}({},{}) assaults {} [{}] {}({},{}) {}\n",
          star.get_name(), star.get_planet_name(g.pnum()), race.name, Playernum,
          Dessymbols[sect.get_condition()], x, y, alien.name, alien.Playernum,
          Dessymbols[sect2.get_condition()], x2, y2,
          (sect2.get_owner() == Playernum ? "VICTORY" : "DEFEAT"));

      if (sect2.get_owner() == Playernum) {
        g.out << std::format("VICTORY! The sector is yours!\n");
        telegram += "Sector CAPTURED!\n";
        if (people) {
          g.out << std::format("{} {} move in.\n", people,
                               what == PopulationType::CIV ? "civilians"
                                                           : "troops");
        }
        planet.info(Playernum - 1).mob_points += (int)sect2.get_mobilization();
        planet.info(old2owner - 1).mob_points -= (int)sect2.get_mobilization();
      } else {
        g.out << std::format("The invasion was repulsed; try again.\n");
        telegram += "You fought them off!\n";
        done = 1; /* end loop */
      }

      if (!(sect.get_popn() + sect.get_troops() + people)) {
        telegram += "You killed all of them!\n";
        /* increase modifier */
        race.translate[old2owner - 1] =
            MIN(race.translate[old2owner - 1] + 5, 100);
      }
      if (!people) {
        g.out << std::format(
            "Oh no! They killed your party to the last man!\n");
        /* increase modifier */
        alien.translate[Playernum - 1] =
            MIN(alien.translate[Playernum - 1] + 5, 100);
      }

      telegram += std::format("Casualties: You: {} civ/{} mil, Them: {} {}\n",
                              casualties2, casualties3, casualties,
                              what == PopulationType::CIV ? "civ" : "mil");
      warn_player(g.session_registry, g.entity_manager, old2owner, old2gov,
                  telegram);
      g.out << std::format("Casualties: You: {} {}, Them: {} civ/{} mil\n",
                           casualties,
                           what == PopulationType::CIV ? "civ" : "mil",
                           casualties2, casualties3);
    } else {
      if (what == PopulationType::CIV) {
        sect.set_popn(sect.get_popn() - people);
        sect2.set_popn(sect2.get_popn() + people);
      } else if (what == PopulationType::MIL) {
        sect.set_troops(sect.get_troops() - people);
        sect2.set_troops(sect2.get_troops() + people);
      }
      if (!sect2.get_owner())
        planet.info(Playernum - 1).mob_points += (int)sect2.get_mobilization();
      sect2.set_owner(Playernum);
    }

    if (sect.is_empty()) {
      planet.info(Playernum - 1).mob_points -= (int)sect.get_mobilization();
      sect.set_owner(0);
    }

    if (sect2.is_empty()) {
      sect2.set_owner(0);
      done = 1;
    }

    deductAPs(g, APcost, g.snum());
    x = x2;
    y = y2; /* get ready for the next round */
  }
  g.out << "Finished.\n";
}
}  // namespace GB::commands
