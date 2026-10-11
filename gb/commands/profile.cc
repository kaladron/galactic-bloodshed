// SPDX-License-Identifier: Apache-2.0

/// \file profile.cc
/// \brief Display racial profile, stats, morale, and planetary conditions.

module;

import gb.entities;
import gb.presentation;
import gb.services;
import std;

module commands;

namespace GB::commands {
bool profile(const command_t& argv, GameObj& g) {
  const auto& race = *g.race;

  // Get information about ourselves
  if (argv.size() == 1) {
    const auto& homestar =
        *g.entity_manager.peek_star(g.current_governor().homesystem);
    const GB::presentation::ProfileSelfViewModel vm{
        .race_name = race.name,
        .player = race.Playernum,
        .is_god = race.God,
        .personal_info = race.info,
        .home_star_name = homestar.get_name(),
        .home_planet_name =
            homestar.get_planet_name(g.current_governor().homeplanetnum),
        .gov_ship = race.Gov_ship,
        .morale = race.morale,
        .updates_active = race.turn,
        .gun_range = race.gun_range(),
        .space_tele_range = tele_range(ShipType::OTYPE_STELE, race.tech),
        .ground_tele_range = tele_range(ShipType::OTYPE_GTELE, race.tech),
        .is_metamorph = race.Metamorph,
        .fertilize = race.fertilize,
        .birthrate = race.birthrate,
        .mass = race.mass,
        .fighters = race.fighters,
        .metabolism = race.metabolism,
        .number_sexes = race.number_sexes,
        .adventurism = race.adventurism,
        .iq = race.IQ,
        .tech = race.tech,
        .temp = race.temp,
        .conditions = race.conditions,
        .sector_compat = race.likes,
        .discoveries = race.discoveries,
    };
    g.out << GB::presentation::render_for_mode(g.ui_mode(), vm);
    return true;
  }

  // Get information about another player.
  const player_t p = get_player(g.entity_manager, argv[1]);
  if (p == player_t{0}) {
    g.out << "Player does not exist.\n";
    return false;
  }
  try {
    g.entity_manager.with_race(p, [&](const Race& r) {
      const Percentage know = race.translation_for(p);
      const GB::presentation::ProfileOtherViewModel vm{
          .race_name = r.name,
          .player = p,
          .show_deity_status = (race.God && r.God),
          .personal_info = r.info,
          .know = know,
          .is_metamorph =
              (know > 50) ? std::optional<bool>{r.Metamorph} : std::nullopt,
          .fertilize = race.estimate(r.fertilize, p),
          .temp = race.estimate(r.temp, p),
          .birthrate =
              (know > 50)
                  ? std::format("{}%%", race.estimate(r.birthrate * 100.0, p))
                  : race.estimate(r.birthrate, p),
          .methane = race.estimate(r.conditions[METHANE], p),
          .mass = race.estimate(r.mass, p),
          .oxygen = race.estimate(r.conditions[OXYGEN], p),
          .gun_range = race.estimate(r.gun_range(), p),
          .fighters = race.estimate(r.fighters, p),
          .helium = race.estimate(r.conditions[HELIUM], p),
          .space_tele_range =
              race.estimate(tele_range(ShipType::OTYPE_STELE, r.tech), p),
          .metabolism = race.estimate(r.metabolism, p),
          .nitrogen = race.estimate(r.conditions[NITROGEN], p),
          .ground_tele_range =
              race.estimate(tele_range(ShipType::OTYPE_GTELE, r.tech), p),
          .number_sexes = race.estimate(r.number_sexes, p),
          .co2 = race.estimate(r.conditions[CO2], p),
          .adventurism = race.estimate(r.adventurism * 100.0, p),
          .hydrogen = race.estimate(r.conditions[HYDROGEN], p),
          .iq = race.estimate(r.IQ, p),
          .sulfur = race.estimate(r.conditions[SULFUR], p),
          .tech = race.estimate(r.tech, p),
          .other = race.estimate(r.conditions[OTHER], p),
          .morale = race.estimate(r.morale, p),
          .best_sector =
              (know > 80) ? std::string{sector_type_name(r.likesbest)} : " ? ",
      };
      g.out << GB::presentation::render_for_mode(g.ui_mode(), vm);
    });
  } catch (const EntityNotFoundError&) {
    g.out << "Race not found.\n";
    return false;
  }
  return true;
}

const CommandDescriptor profile_cmd{
    .name = "profile",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "profile [<race name>]",
    .description =
        "Display racial profile, stats, morale, and planetary conditions",
    .handler = &profile,
};

}  // namespace GB::commands
