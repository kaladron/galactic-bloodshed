// SPDX-License-Identifier: Apache-2.0

/// \file move_popn.cc
/// \brief Move civilian population or deploy military troops across sectors.

module;

import gb.entities;
import gb.presentation.combat;
import gb.services;
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
  population_t people;
  int n;
  int done;
  bool any_moved = false;

  const auto& planet_peek = *g.entity_manager.peek_planet(g.snum(), g.pnum());

  if (planet_peek.is_enslaved_to_foreign(Playernum)) {
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
    if (n >= argv[2].size()) {
      break;
    }
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

    if (!planet_peek.is_adjacent(curr_coords, next_coords)) {
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
    MechDefendResult defend_res{};
    g.entity_manager.mutate_sectormap(
        g.snum(), g.pnum(), [&](SectorMap& smap_mut) {
          auto& sect2_mut = smap_mut.get(next_coords);
          defend_res = mech_defend(g.entity_manager, *g.race, &people, what,
                                   planet_peek, next_coords, sect2_mut);
        });
    for (const auto& round : defend_res.rounds) {
      const auto attack_long =
          GB::presentation::render_mech_attack_people_long(round.mech_attack);
      push_telegram(g.entity_manager, Playernum, g.governor(), attack_long);
      push_telegram(g.entity_manager, round.defender_player,
                    round.defender_governor, attack_long);
      if (round.people_counterattack) {
        const auto counter_long =
            GB::presentation::render_people_attack_mech_long(
                *round.people_counterattack);
        push_telegram(g.entity_manager, Playernum, g.governor(), counter_long);
        push_telegram(g.entity_manager, round.defender_player,
                      round.defender_governor, counter_long);
      }
    }
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
      const auto assault = execute_sector_ground_assault(
          g.entity_manager, g.snum(), g.pnum(), Playernum, curr_coords,
          next_coords, people, what);
      people = assault.surviving_attackers;
      if (auto absorb_warn =
              GB::presentation::render_sector_ground_assault_absorb_warning(
                  assault)) {
        warn_player(g.session_registry, g.entity_manager,
                    assault.defender_player, assault.defender_governor,
                    *absorb_warn);
      }
      warn_player(
          g.session_registry, g.entity_manager, assault.defender_player,
          assault.defender_governor,
          GB::presentation::render_sector_ground_assault_telegram(assault));
      g.out << GB::presentation::render_sector_ground_assault(assault);
      if (assault.stop_movement) {
        done = 1;
      }
    } else {
      g.entity_manager.mutate_planet_and_sectors(
          g.snum(), g.pnum(), [&](Planet& planet, SectorMap& smap) {
            auto& sect = smap.get(curr_coords);
            auto& sect2 = smap.get(next_coords);
            planet.move_sector_population(sect, sect2, Playernum, people, what);
            if (sect2.is_empty()) {
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
