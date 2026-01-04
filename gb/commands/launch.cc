// SPDX-License-Identifier: Apache-2.0

module;

import session;
import gblib;
import notification;
import std;

module commands;

namespace GB::commands {
void launch(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  ap_t APcount = 1;

  if (argv.size() < 2) {
    g.out << "Launch what?\n";
    return;
  }

  ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);

  for (auto ship_handle : ships) {
    Ship& s = *ship_handle;  // Get mutable access upfront

    if (!ship_matches_filter(argv[1], s)) continue;
    if (!authorized(Governor, s)) continue;

    if (!speed_rating(s) && landed(s)) {
      g.out << "That ship is not designed to be launched.\n";
      continue;
    }

    if (!s.docked() && s.whatorbits() != ScopeLevel::LEVEL_SHIP) {
      g.out << std::format("{} is not landed or docked.\n", ship_to_string(s));
      continue;
    }
    if (!landed(s)) APcount = 0;
    if (landed(s) && s.resource() > max_resource(s)) {
      g.out << std::format("{} is too overloaded to launch.\n",
                           ship_to_string(s));
      continue;
    }
    if (s.whatorbits() == ScopeLevel::LEVEL_SHIP) {
      /* Factories cannot be launched once turned on. Maarten */
      if (s.type() == ShipType::OTYPE_FACTORY && s.on()) {
        g.out << "Factories cannot be launched once turned on.\n";
        g.out << "Consider using 'scrap'.\n";
        continue;
      }
      auto s2_handle = g.entity_manager.get_ship(s.destshipno());
      if (!s2_handle.get()) {
        g.out << "Destination ship not found.\n";
        continue;
      }
      auto& s2 = *s2_handle;
      if (landed(s2)) {
        remove_sh_ship(g.entity_manager, s, s2);
        auto planet_handle =
            g.entity_manager.get_planet(s2.storbits(), s2.pnumorbits());
        auto& p = *planet_handle;
        const auto& star = *g.entity_manager.peek_star(s2.storbits());
        insert_sh_plan(p, &s);
        s.storbits() = s2.storbits();
        s.pnumorbits() = s2.pnumorbits();
        s.destpnum() = s2.pnumorbits();
        s.deststar() = s2.deststar();
        s.xpos() = s2.xpos();
        s.ypos() = s2.ypos();
        s.land_x() = s2.land_x();
        s.land_y() = s2.land_y();
        s.docked() = 1;
        s.whatdest() = ScopeLevel::LEVEL_PLAN;
        s2.mass() -= s.mass();
        s2.hanger() -= size(s);
        g.out << std::format("Landed on {}/{}.\n", star.get_name(),
                             star.get_planet_name(s.pnumorbits()));
      } else if (s2.whatorbits() == ScopeLevel::LEVEL_PLAN) {
        remove_sh_ship(g.entity_manager, s, s2);
        g.out << std::format("{} launched from {}.\n", ship_to_string(s),
                             ship_to_string(s2));
        s.xpos() = s2.xpos();
        s.ypos() = s2.ypos();
        s.docked() = 0;
        s.whatdest() = ScopeLevel::LEVEL_UNIV;
        s2.mass() -= s.mass();
        s2.hanger() -= size(s);
        auto planet_handle =
            g.entity_manager.get_planet(s2.storbits(), s2.pnumorbits());
        auto& p = *planet_handle;
        const auto* star_ptr = g.entity_manager.peek_star(s2.storbits());
        const auto& star = *star_ptr;
        insert_sh_plan(p, &s);
        s.storbits() = s2.storbits();
        s.pnumorbits() = s2.pnumorbits();
        g.out << std::format("Orbiting {}/{}.\n", star.get_name(),
                             star.get_planet_name(s.pnumorbits()));
      } else if (s2.whatorbits() == ScopeLevel::LEVEL_STAR) {
        remove_sh_ship(g.entity_manager, s, s2);
        g.out << std::format("{} launched from {}.\n", ship_to_string(s),
                             ship_to_string(s2));
        s.xpos() = s2.xpos();
        s.ypos() = s2.ypos();
        s.docked() = 0;
        s.whatdest() = ScopeLevel::LEVEL_UNIV;
        s2.mass() -= s.mass();
        s2.hanger() -= size(s);
        auto star_handle = g.entity_manager.get_star(s2.storbits());
        auto& star = *star_handle;
        insert_sh_star(star, &s);
        s.storbits() = s2.storbits();
        g.out << std::format("Orbiting {}.\n", star.get_name());
      } else if (s2.whatorbits() == ScopeLevel::LEVEL_UNIV) {
        remove_sh_ship(g.entity_manager, s, s2);
        g.out << std::format("{} launched from {}.\n", ship_to_string(s),
                             ship_to_string(s2));
        s.xpos() = s2.xpos();
        s.ypos() = s2.ypos();
        s.docked() = 0;
        s.whatdest() = ScopeLevel::LEVEL_UNIV;
        s2.mass() -= s.mass();
        s2.hanger() -= size(s);
        auto univ_handle = g.entity_manager.get_universe();
        auto& univ_data = *univ_handle;
        insert_sh_univ(&univ_data, &s);
        g.out << "Universe level.\n";
      } else {
        g.out << "You can't launch that ship.\n";
        continue;
      }
    } else if (s.whatdest() == ScopeLevel::LEVEL_SHIP) {
      auto s2_handle = g.entity_manager.get_ship(s.destshipno());
      if (!s2_handle.get()) {
        g.out << "Destination ship not found.\n";
        continue;
      }
      auto& s2 = *s2_handle;
      if (s2.whatorbits() == ScopeLevel::LEVEL_UNIV) {
        const universe_struct* univ_data = g.entity_manager.peek_universe();
        if (!enufAP(Playernum, Governor, univ_data->AP[Playernum - 1],
                    APcount)) {
          continue;
        }
        deductAPs(g, APcount, ScopeLevel::LEVEL_UNIV);
      } else {
        const auto& star = *g.entity_manager.peek_star(s.storbits());
        if (!enufAP(Playernum, Governor, star.AP(Playernum - 1), APcount)) {
          continue;
        }
        deductAPs(g, APcount, s.storbits());
      }
      s.docked() = 0;
      s.whatdest() = ScopeLevel::LEVEL_UNIV;
      s.destshipno() = 0;
      s2.docked() = 0;
      s2.whatdest() = ScopeLevel::LEVEL_UNIV;
      s2.destshipno() = 0;
      g.out << std::format("{} undocked from {}.\n", ship_to_string(s),
                           ship_to_string(s2));
    } else {
      const auto* star_ptr = g.entity_manager.peek_star(s.storbits());
      const auto& star = *star_ptr;
      if (!enufAP(Playernum, Governor, star.AP(Playernum - 1), APcount)) {
        return;
      }
      deductAPs(g, APcount, s.storbits());

      /* adjust x,ypos to absolute coords */
      auto planet_handle =
          g.entity_manager.get_planet((int)s.storbits(), (int)s.pnumorbits());
      auto& p = *planet_handle;
      g.out << std::format("Planet /{}/{} has gravity field of {:.2f}\n",
                           star.get_name(),
                           star.get_planet_name(s.pnumorbits()), p.gravity());
      s.xpos() =
          star.xpos() + p.xpos() +
          (double)int_rand((int)(-DIST_TO_LAND / 4), (int)(DIST_TO_LAND / 4));
      s.ypos() =
          star.ypos() + p.ypos() +
          (double)int_rand((int)(-DIST_TO_LAND / 4), (int)(DIST_TO_LAND / 4));

      /* subtract fuel from ship */
      auto fuel = p.gravity() * s.mass() * LAUNCH_GRAV_MASS_FACTOR;
      if (s.fuel() < fuel) {
        g.out << std::format("{} does not have enough fuel! ({:.1f})\n",
                             ship_to_string(s), fuel);
        return;
      }
      use_fuel(s, fuel);
      s.docked() = 0;
      s.whatdest() = ScopeLevel::LEVEL_UNIV; /* no destination */
      switch (s.type()) {
        case ShipType::OTYPE_CANIST:
        case ShipType::OTYPE_GREEN:
          s.special() = TimerData{.count = 0};
          break;
        default:
          break;
      }
      s.notified() = 0;
      if (!p.explored()) {
        /* not yet explored by owner; space exploration causes the
           player to see a whole map */
        p.explored() = 1;
      }
      std::string observed = std::format(
          "{} observed launching from planet /{}/{}.\n", ship_to_string(s),
          star.get_name(), star.get_planet_name(s.pnumorbits()));
      for (player_t i = 1; i <= g.entity_manager.num_races(); i++)
        if (p.info(i - 1).numsectsowned && i != Playernum) {
          get_session_registry(g).notify_player(i, star.governor(i - 1),
                                                observed);
        }

      g.out << std::format("{} launched from planet,", ship_to_string(s));
      g.out << std::format(" using {:.1f} fuel.\n", fuel);

      switch (s.type()) {
        case ShipType::OTYPE_CANIST:
          g.out << "A cloud of dust envelopes your planet.\n";
          break;
        case ShipType::OTYPE_GREEN:
          g.out << "Greenhouse gases surround the planet.\n";
          break;
        default:
          break;
      }
    }
  }
}
}  // namespace GB::commands
