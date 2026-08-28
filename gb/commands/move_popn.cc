// SPDX-License-Identifier: Apache-2.0

/// \file move_popn.cc
/// \brief Move civilian population or deploy military troops across sectors.

module;

import session;
import gblib;
import notification;
import scnlib;
import std;

module commands;

namespace GB::commands {

bool move_popn(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  const PopulationType what =
      (argv[0] == "move") ? PopulationType::CIV : PopulationType::MIL;
  int Assault;
  int APcost; /* unfriendly movement */
  population_t casualties;
  population_t casualties2;
  population_t casualties3;

  population_t people;
  population_t oldpopn;
  population_t old2popn;
  population_t old3popn;
  player_t old2owner;
  governor_t old2gov;
  int absorbed;
  int n;
  int done;
  double astrength;
  double dstrength;
  bool any_moved = false;

  const auto& star = *g.entity_manager.peek_star(g.snum());
  const auto& planet_peek = *g.entity_manager.peek_planet(g.snum(), g.pnum());

  if (planet_peek.slaved_to() > 0 && planet_peek.slaved_to() != Playernum) {
    g.out << "That planet has been enslaved!\n";
    return false;
  }
  auto from_opt = Coordinates::parse(argv[1]);
  if (!from_opt) {
    g.out << "Bad format for sector.\n";
    return false;
  }
  Coordinates curr_coords = *from_opt;
  if (!planet_peek.is_valid(curr_coords)) {
    g.out << "Origin coordinates illegal.\n";
    return false;
  }

  /* movement loop */
  done = 0;
  n = 0;
  while (!done) {
    const auto& smap_peek =
        *g.entity_manager.peek_sectormap(g.snum(), g.pnum());
    const auto& sect_curr = smap_peek.get(curr_coords);
    if (sect_curr.get_owner() != Playernum) {
      g.out << std::format("You don't own sector {}!\n", curr_coords);
      return any_moved;
    }
    Coordinates next_coords = get_move(planet_peek, argv[2][n++], curr_coords);
    if (curr_coords == next_coords) {
      g.out << "Finished.\n";
      return any_moved;
    }

    if (!planet_peek.is_valid(next_coords)) {
      g.out << std::format("Illegal coordinates {}.\n", next_coords);
      return any_moved;
    }

    if (!adjacent(planet_peek, curr_coords, next_coords)) {
      g.out << "Illegal move - to adjacent sectors only!\n";
      return any_moved;
    }

    /* ok, the move is legal */
    const auto& sect2_peek = smap_peek.get(next_coords);
    if (argv.size() >= 4) {
      auto count_res = scn::scan<population_t>(argv[3], "{}");
      if (count_res) {
        people = count_res->value();
        if (people < 0) {
          if (what == PopulationType::CIV)
            people = sect_curr.get_popn() + people;
          else if (what == PopulationType::MIL)
            people = sect_curr.get_troops() + people;
        }
      } else {
        people = 0;
      }
    } else {
      if (what == PopulationType::CIV)
        people = sect_curr.get_popn();
      else if (what == PopulationType::MIL)
        people = sect_curr.get_troops();
    }

    if ((what == PopulationType::CIV &&
         (std::abs(people) > sect_curr.get_popn())) ||
        (what == PopulationType::MIL &&
         (std::abs(people) > sect_curr.get_troops())) ||
        people <= 0) {
      if (what == PopulationType::CIV)
        g.out << std::format("Bad value - {} civilians in [{}]\n",
                             sect_curr.get_popn(), curr_coords);
      else if (what == PopulationType::MIL)
        g.out << std::format("Bad value - {} troops in [{}]\n",
                             sect_curr.get_troops(), curr_coords);
      return any_moved;
    }

    g.out << std::format("{} {} moved.\n", people,
                         what == PopulationType::CIV ? "population" : "troops");

    /* check for defending mechs */
    g.entity_manager.mutate_sectormap(
        g.snum(), g.pnum(), [&](SectorMap& smap_mut) {
          auto& sect2_mut = smap_mut.get(next_coords);
          mech_defend(g, &people, what, planet_peek, next_coords, sect2_mut);
        });
    if (!people) {
      g.out << "Attack aborted.\n";
      return any_moved;
    }

    if ((sect2_peek.get_owner() != 0) && (sect2_peek.get_owner() != Playernum))
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

    if (!g.deduct_ap(g.snum(), APcost)) {
      g.out << std::format("You don't have {} action points there.\n", APcost);
      return any_moved;
    }

    if (Assault) {
      ground_assaults[Playernum.value - 1][sect2_peek.get_owner().value - 1]
                     [g.snum().value] += 1;
      old2owner = sect2_peek.get_owner();
      old2gov = star.governor(old2owner);

      const auto* alien_peek = g.entity_manager.peek_race(old2owner);
      if (!alien_peek) {
        continue;
      }
      Race alien = *alien_peek;
      Race race = *g.race;

      /* races find out about each other */
      alien.translate[Playernum.value - 1] =
          MIN(alien.translate[Playernum.value - 1] + 5, 100);
      race.translate[old2owner.value - 1] =
          MIN(race.translate[old2owner.value - 1] + 5, 100);

      g.entity_manager.mutate_sectormap(
          g.snum(), g.pnum(), [&](SectorMap& smap) {
            auto& sect = smap.get(curr_coords);
            auto& sect2 = smap.get(next_coords);

            if (what == PopulationType::CIV)
              sect.subtract_popn(people);
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
            ground_attack(race, alien, &people, what, &sect2_popn,
                          &sect2_troops, Defensedata[sect.get_condition()],
                          Defensedata[sect2.get_condition()],
                          race.likes[sect.get_condition()],
                          alien.likes[sect2.get_condition()], &astrength,
                          &dstrength, &casualties, &casualties2, &casualties3);

            sect2.set_popn_exact(sect2_popn);
            sect2.set_troops(sect2_troops);

            g.out << std::format("Attack: {:.2f}   Defense: {:.2f}.\n",
                                 astrength, dstrength);

            if (sect2.is_empty()) { /* we got 'em */
              sect2.set_owner(Playernum);
              /* mesomorphs absorb the bodies of their victims */
              absorbed = 0;
              if (race.absorb) {
                absorbed = int_rand(0, old2popn + old3popn);
                g.out << std::format("{} alien bodies absorbed.\n", absorbed);
                g.session_registry.notify_player(
                    old2owner, old2gov,
                    std::format("Metamorphs have absorbed {} bodies!!!\n",
                                absorbed));
              }
              if (what == PopulationType::CIV)
                sect2.set_popn_exact(people + absorbed);
              else if (what == PopulationType::MIL) {
                sect2.set_popn_exact(absorbed);
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
                sect2.add_popn(absorbed);
              }
              if (what == PopulationType::CIV)
                sect.add_popn(people);
              else if (what == PopulationType::MIL)
                sect.set_troops(sect.get_troops() + people);
              adjust_morale(alien, race, (int)race.fighters);
            }

            std::string telegram = std::format(
                "/{}/{}: {} [{}] {}{} assaults {} [{}] {} {}\n",
                star.get_name(), star.get_planet_name(g.pnum()), race.name,
                Playernum, Dessymbols[sect.get_condition()], curr_coords,
                alien.name, alien.Playernum, Dessymbols[sect2.get_condition()],
                next_coords,
                (sect2.get_owner() == Playernum ? "VICTORY" : "DEFEAT"));

            if (sect2.get_owner() == Playernum) {
              g.out << std::format("VICTORY! The sector is yours!\n");
              telegram += "Sector CAPTURED!\n";
              if (people) {
                g.out << std::format("{} {} move in.\n", people,
                                     what == PopulationType::CIV ? "civilians"
                                                                 : "troops");
              }
              g.entity_manager.mutate_planet(
                  g.snum(), g.pnum(), [&](Planet& planet) {
                    planet.info(Playernum).mob_points +=
                        (int)sect2.get_mobilization();
                    planet.info(old2owner).mob_points -=
                        (int)sect2.get_mobilization();
                  });
            } else {
              g.out << std::format("The invasion was repulsed; try again.\n");
              telegram += "You fought them off!\n";
              done = 1; /* end loop */
            }

            if (!(sect.get_popn() + sect.get_troops() + people)) {
              telegram += "You killed all of them!\n";
              /* increase modifier */
              race.translate[old2owner.value - 1] =
                  MIN(race.translate[old2owner.value - 1] + 5, 100);
            }
            if (!people) {
              g.out << std::format(
                  "Oh no! They killed your party to the last man!\n");
              /* increase modifier */
              alien.translate[Playernum.value - 1] =
                  MIN(alien.translate[Playernum.value - 1] + 5, 100);
            }

            telegram +=
                std::format("Casualties: You: {} civ/{} mil, Them: {} {}\n",
                            casualties2, casualties3, casualties,
                            what == PopulationType::CIV ? "civ" : "mil");
            warn_player(g.session_registry, g.entity_manager, old2owner,
                        old2gov, telegram);
            g.out << std::format(
                "Casualties: You: {} {}, Them: {} civ/{} mil\n", casualties,
                what == PopulationType::CIV ? "civ" : "mil", casualties2,
                casualties3);

            if (sect.is_empty()) {
              g.entity_manager.mutate_planet(
                  g.snum(), g.pnum(), [&](Planet& planet) {
                    planet.info(Playernum).mob_points -=
                        (int)sect.get_mobilization();
                  });
              sect.set_owner(0);
            }

            if (sect2.is_empty()) {
              sect2.set_owner(0);
              done = 1;
            }
          });

      g.entity_manager.mutate_race(Playernum, [&](Race& r) { r = race; });
      g.entity_manager.mutate_race(old2owner, [&](Race& a) { a = alien; });
    } else {
      g.entity_manager.mutate_sectormap(
          g.snum(), g.pnum(), [&](SectorMap& smap) {
            auto& sect = smap.get(curr_coords);
            auto& sect2 = smap.get(next_coords);
            if (what == PopulationType::CIV) {
              sect.subtract_popn(people);
              sect2.add_popn(people);
            } else if (what == PopulationType::MIL) {
              sect.set_troops(sect.get_troops() - people);
              sect2.set_troops(sect2.get_troops() + people);
            }
            if (sect2.get_owner() == player_t{0}) {
              g.entity_manager.mutate_planet(
                  g.snum(), g.pnum(), [&](Planet& planet) {
                    planet.info(Playernum).mob_points +=
                        (int)sect2.get_mobilization();
                  });
            }
            sect2.set_owner(Playernum);

            if (sect.is_empty()) {
              g.entity_manager.mutate_planet(
                  g.snum(), g.pnum(), [&](Planet& planet) {
                    planet.info(Playernum).mob_points -=
                        (int)sect.get_mobilization();
                  });
              sect.set_owner(0);
            }

            if (sect2.is_empty()) {
              sect2.set_owner(0);
              done = 1;
            }
          });
    }

    any_moved = true;
    curr_coords = next_coords; /* get ready for the next round */
  }
  g.out << "Finished.\n";
  return any_moved;
}

bool deploy(const command_t& argv, GameObj& g) {
  return move_popn(argv, g);
}

const CommandDescriptor move_cmd{
    .name = "move",
    .roles =
        {
            .star_control = true,
        },
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::dynamic(),
    .min_args = 3,
    .syntax = "move <from_sector> <path> [<amount>]",
    .description = "Move civilian population across planetary sectors",
    .handler = &move_popn,
};

const CommandDescriptor deploy_cmd{
    .name = "deploy",
    .roles =
        {
            .star_control = true,
        },
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::dynamic(),
    .min_args = 3,
    .syntax = "deploy <from_sector> <path> [<amount>]",
    .description = "Deploy military troops across planetary sectors",
    .handler = &deploy,
};

}  // namespace GB::commands
