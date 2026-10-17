// SPDX-License-Identifier: Apache-2.0

/// \file declare.cc
/// \brief Declare alliance, neutrality, war, and relations.

module;

import std;
import gb.entities;
import gb.presentation;
import gb.services;
import scnlib;

module commands;

namespace GB::commands {
bool declare(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  player_t n = get_player(g.entity_manager, argv[1]);
  if (n == player_t{0}) {
    g.out << "No such player.\n";
    return false;
  }

  if (n == Playernum) {
    g.out << "You cannot declare against yourself.\n";
    return false;
  }

  const auto& alien = *g.entity_manager.peek_race(n);

  GB::presentation::DeclareResult result{
      .declarer_id = Playernum,
      .declarer_name = g.race->name,
      .target_id = n,
      .target_name = alien.name,
  };

  switch (argv[2][0]) {
    case 'a':
      result.relation = GB::presentation::DiplomaticRelation::Alliance;
      result.rare_quip = success(5);
      if (argv.size() > 3) {
        if (auto parsed = scn::scan<int>(argv[3], "{}")) {
          result.translation_modifier = std::max(parsed->value(), 30);
        }
      }
      break;
    case 'n':
      result.relation = GB::presentation::DiplomaticRelation::Neutral;
      break;
    case 'w': {
      result.relation = GB::presentation::DiplomaticRelation::War;
      result.rare_quip = success(4);
      const auto& variants =
          GB::presentation::all_war_declaration_news_variants;
      result.news_variant =
          variants[int_rand(0, static_cast<int>(variants.size()) - 1)];
      break;
    }
    default:
      g.out << "I don't understand.\n";
      return false;
  }

  g.entity_manager.mutate_race(Playernum, [&](Race& race) {
    switch (result.relation) {
      case GB::presentation::DiplomaticRelation::Alliance:
        race.declare_alliance_with(n);
        race.make_peace_with(n);
        break;
      case GB::presentation::DiplomaticRelation::Neutral:
        race.rescind_alliance_with(n);
        race.make_peace_with(n);
        break;
      case GB::presentation::DiplomaticRelation::War:
        race.declare_war_on(n);
        race.rescind_alliance_with(n);
        break;
    }
  });

  g.entity_manager.mutate_race(n, [&](Race& target_race) {
    /* They, of course, learn more about you */
    target_race.increase_translation(Playernum, result.translation_modifier);
  });

  warn_race(g.session_registry, g.entity_manager, n,
            GB::presentation::render_declare_target_notification(result));
  const std::string news_msg =
      GB::presentation::render_declare_declaration_news(result);
  post(g.entity_manager, news_msg, NewsType::DECLARATION);
  warn_race(g.session_registry, g.entity_manager, Playernum, news_msg);

  g.present(result);
  return true;
}

const CommandDescriptor declare_cmd{
    .name = "declare",
    .roles = {.no_guests = true, .leader_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::fixed_univ(1),
    .min_args = 3,
    .syntax = "declare <race> <alliance|neutral|war> [<modifier>]",
    .description = "Declare alliance, neutrality, or war with another race",
    .handler = &declare,
};

}  // namespace GB::commands
