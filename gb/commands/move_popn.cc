// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include <strings.h>

module commands;

namespace GB::commands {
void move_popn(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
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
  int x = -1;
  int y = -1;
  int old2owner;
  int old2gov;
  int absorbed;
  int n;
  int done;
  double astrength;
  double dstrength;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    g.out << "Wrong scope\n";
    return;
  }
  if (!stars[g.snum].control(Playernum, Governor)) {
    g.out << "You are not authorized to do that here.\n";
    return;
  }
  auto planet = getplanet(g.snum, g.pnum);

  if (planet.slaved_to() > 0 && planet.slaved_to() != Playernum) {
    g.out << "That planet has been enslaved!\n";
    return;
  }
  sscanf(argv[1].c_str(), "%d,%d", &x, &y);
  if (x < 0 || y < 0 || x > planet.Maxx() - 1 || y > planet.Maxy() - 1) {
    g.out << "Origin coordinates illegal.\n";
    return;
  }

  /* movement loop */
  done = 0;
  n = 0;
  while (!done) {
    auto sect = getsector(planet, x, y);
    if (sect.owner != Playernum) {
      g.out << std::format("You don't own sector {},{}!\n", x, y);
      return;
    }
    auto [x2, y2] = get_move(planet, argv[2][n++], {x, y});
    if (x == x2 && y == y2) {
      g.out << "Finished.\n";
      putplanet(planet, stars[g.snum], g.pnum);
      return;
    }

    if (x2 < 0 || y2 < 0 || x2 > planet.Maxx() - 1 || y2 > planet.Maxy() - 1) {
      g.out << std::format("Illegal coordinates {},{}.\n", x2, y2);
      putplanet(planet, stars[g.snum], g.pnum);
      return;
    }

    if (!adjacent(planet, {x, y}, {x2, y2})) {
      g.out << "Illegal move - to adjacent sectors only!\n";
      return;
    }

    /* ok, the move is legal */
    auto sect2 = getsector(planet, x2, y2);
    if (argv.size() >= 4) {
      people = std::stoi(argv[3]);
      if (people < 0) {
        if (what == PopulationType::CIV)
          people = sect.popn + people;
        else if (what == PopulationType::MIL)
          people = sect.troops + people;
      }
    } else {
      if (what == PopulationType::CIV)
        people = sect.popn;
      else if (what == PopulationType::MIL)
        people = sect.troops;
    }

    if ((what == PopulationType::CIV && (abs(people) > sect.popn)) ||
        (what == PopulationType::MIL && (abs(people) > sect.troops)) ||
        people <= 0) {
      if (what == PopulationType::CIV)
        g.out << std::format("Bad value - {} civilians in [{},{}]\n", sect.popn,
                             x, y);
      else if (what == PopulationType::MIL)
        g.out << std::format("Bad value - {} troops in [{},{}]\n", sect.troops,
                             x, y);
      putplanet(planet, stars[g.snum], g.pnum);
      return;
    }

    g.out << std::format("{} {} moved.\n", people,
                         what == PopulationType::CIV ? "population" : "troops");

    /* check for defending mechs */
    mech_defend(g.entity_manager, Playernum, Governor, &people, what, planet,
                x2, y2, sect2);
    if (!people) {
      putsector(sect, planet, x, y);
      putsector(sect2, planet, x2, y2);
      putplanet(planet, stars[g.snum], g.pnum);
      g.out << "Attack aborted.\n";
      return;
    }

    if (sect2.owner && (sect2.owner != Playernum))
      Assault = 1;
    else
      Assault = 0;

    /* action point cost depends on the size of the group being moved */
    if (what == PopulationType::CIV)
      APcost = MOVE_FACTOR * ((int)log(1.0 + (double)people) + Assault) + 1;
    else if (what == PopulationType::MIL)
      APcost = MOVE_FACTOR * ((int)log10(1.0 + (double)people) + Assault) + 1;

    if (!enufAP(Playernum, Governor, stars[g.snum].AP(Playernum - 1), APcost)) {
      putplanet(planet, stars[g.snum], g.pnum);
      return;
    }

    if (Assault) {
      ground_assaults[Playernum - 1][sect2.owner - 1][g.snum] += 1;
      auto& race = races[Playernum - 1];
      auto& alien = races[sect2.owner - 1];
      /* races find out about each other */
      alien.translate[Playernum - 1] =
          MIN(alien.translate[Playernum - 1] + 5, 100);
      race.translate[sect2.owner - 1] =
          MIN(race.translate[sect2.owner - 1] + 5, 100);

      old2owner = (int)(sect2.owner);
      old2gov = stars[g.snum].governor(sect2.owner - 1);
      if (what == PopulationType::CIV)
        sect.popn = std::max(0L, sect.popn - people);
      else if (what == PopulationType::MIL)
        sect.troops = std::max(0L, sect.troops - people);

      if (what == PopulationType::CIV)
        g.out << std::format("{} civ assault {} civ/{} mil\n", people,
                             sect2.popn, sect2.troops);
      else if (what == PopulationType::MIL)
        g.out << std::format("{} mil assault {} civ/{} mil\n", people,
                             sect2.popn, sect2.troops);
      oldpopn = people;
      old2popn = sect2.popn;
      old3popn = sect2.troops;

      ground_attack(race, alien, &people, what, &sect2.popn, &sect2.troops,
                    Defensedata[sect.condition], Defensedata[sect2.condition],
                    race.likes[sect.condition], alien.likes[sect2.condition],
                    &astrength, &dstrength, &casualties, &casualties2,
                    &casualties3);

      g.out << std::format("Attack: {:.2f}   Defense: {:.2f}.\n", astrength,
                           dstrength);

      if (!(sect2.popn + sect2.troops)) { /* we got 'em */
        sect2.owner = Playernum;
        /* mesomorphs absorb the bodies of their victims */
        absorbed = 0;
        if (race.absorb) {
          absorbed = int_rand(0, old2popn + old3popn);
          g.out << std::format("{} alien bodies absorbed.\n", absorbed);
          notify(
              old2owner, old2gov,
              std::format("Metamorphs have absorbed {} bodies!!!\n", absorbed));
        }
        if (what == PopulationType::CIV)
          sect2.popn = people + absorbed;
        else if (what == PopulationType::MIL) {
          sect2.popn = absorbed;
          sect2.troops = people;
        }
        adjust_morale(race, alien, (int)alien.fighters);
      } else { /* retreat */
        absorbed = 0;
        if (alien.absorb) {
          absorbed = int_rand(0, oldpopn - people);
          notify(old2owner, old2gov,
                 std::format("{} alien bodies absorbed.\n", absorbed));
          notify(
              Playernum, Governor,
              std::format("Metamorphs have absorbed {} bodies!!!\n", absorbed));
          sect2.popn += absorbed;
        }
        if (what == PopulationType::CIV)
          sect.popn += people;
        else if (what == PopulationType::MIL)
          sect.troops += people;
        adjust_morale(alien, race, (int)race.fighters);
      }

      std::string telegram = std::format(
          "/{}/{}: {} [{}] {}({},{}) assaults {} [{}] {}({},{}) {}\n",
          stars[g.snum].get_name(), stars[g.snum].get_planet_name(g.pnum),
          race.name, Playernum, Dessymbols[sect.condition], x, y, alien.name,
          alien.Playernum, Dessymbols[sect2.condition], x2, y2,
          (sect2.owner == Playernum ? "VICTORY" : "DEFEAT"));

      if (sect2.owner == Playernum) {
        g.out << std::format("VICTORY! The sector is yours!\n");
        telegram += "Sector CAPTURED!\n";
        if (people) {
          g.out << std::format("{} {} move in.\n", people,
                               what == PopulationType::CIV ? "civilians"
                                                           : "troops");
        }
        planet.info(Playernum - 1).mob_points += (int)sect2.mobilization;
        planet.info(old2owner - 1).mob_points -= (int)sect2.mobilization;
      } else {
        g.out << std::format("The invasion was repulsed; try again.\n");
        telegram += "You fought them off!\n";
        done = 1; /* end loop */
      }

      if (!(sect.popn + sect.troops + people)) {
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
      putrace(alien);
      putrace(race);

      telegram += std::format("Casualties: You: {} civ/{} mil, Them: {} {}\n",
                              casualties2, casualties3, casualties,
                              what == PopulationType::CIV ? "civ" : "mil");
      warn(old2owner, old2gov, telegram);
      notify(Playernum, Governor,
             std::format("Casualties: You: {} {}, Them: {} civ/{} mil\n",
                         casualties,
                         what == PopulationType::CIV ? "civ" : "mil",
                         casualties2, casualties3));
    } else {
      if (what == PopulationType::CIV) {
        sect.popn -= people;
        sect2.popn += people;
      } else if (what == PopulationType::MIL) {
        sect.troops -= people;
        sect2.troops += people;
      }
      if (!sect2.owner)
        planet.info(Playernum - 1).mob_points += (int)sect2.mobilization;
      sect2.owner = Playernum;
    }

    if (!(sect.popn + sect.troops)) {
      planet.info(Playernum - 1).mob_points -= (int)sect.mobilization;
      sect.owner = 0;
    }

    if (!(sect2.popn + sect2.troops)) {
      sect2.owner = 0;
      done = 1;
    }

    putsector(sect, planet, x, y);
    putsector(sect2, planet, x2, y2);

    deductAPs(g, APcost, g.snum);
    x = x2;
    y = y2; /* get ready for the next round */
  }
  g.out << "Finished.\n";
}
}  // namespace GB::commands
