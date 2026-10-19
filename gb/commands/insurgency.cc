// SPDX-License-Identifier: Apache-2.0

/// \file insurgency.cc
/// \brief Finance a planetary insurgency against an occupying player.

module;

import gb.entities;
import gb.presentation;
import gb.services;
import scnlib;
import std;

module commands;

namespace GB::commands {

namespace {

/// Scales star-system civilian population differential onto the morale_factor
/// curve (500,000 popn difference -> 10,000 morale-equivalent).
constexpr double POPULATION_DIFF_SCALE = 50.0;

/// Scales planetary effective troop strength differential (fighters * troops)
/// onto the morale_factor curve (1,000 effective strength -> 10,000).
constexpr double TROOP_STRENGTH_SCALE = 10.0;

/// Divisor normalizing raw insurgency strength x before arctangent conversion.
constexpr double INSURGENCY_ODDS_DIVISOR = 50.0;

/// Converts atan(x) in [0, pi/2) to a percentage in [0, 100): 100 / (pi / 2) =
/// 200 / pi.
constexpr double ATAN_TO_PERCENT_SCALE = 200.0;

}  // namespace

bool insurgency(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  player_t who{0};
  population_t eligible = 0;
  population_t them = 0;

  if (g.level() != ScopeLevel::LEVEL_PLAN) {
    g.out << "You must 'cs' to the planet you wish to try it on.\n";
    return false;
  }
  const auto& star = *g.entity_manager.peek_star(g.snum());
  if (!star.control(Playernum, Governor)) {
    g.out << "You are not authorized to do that here.\n";
    return false;
  }

  who = get_player(g.entity_manager, argv[1]);
  if (who == player_t{0}) {
    g.out << "No such player.\n";
    return false;
  }
  const auto* alien = g.entity_manager.peek_race(who);
  if (alien->Guest) {
    g.out << "Don't be such a dickweed.\n";
    return false;
  }
  if (who == Playernum) {
    g.out << "You can't revolt against yourself!\n";
    return false;
  }
  PlanetList planets(g.entity_manager, g.snum(), star);
  for (auto planet_handle : planets) {
    eligible += planet_handle->info(Playernum).popn;
    them += planet_handle->info(who).popn;
  }
  if (!eligible) {
    g.out << "You must have population in the star system to attempt "
             "insurgency.\n";
    return false;
  }
  bool ok = false;
  g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
    if (!p.info(who).popn) {
      g.out << "This player does not occupy this planet.\n";
      return;
    }

    auto parsed_amount = scn::scan<money_t>(argv[2], "{}");
    if (!parsed_amount || parsed_amount->value() <= 0) {
      g.out << "You have to use a positive amount of money.\n";
      return;
    }
    money_t amount = parsed_amount->value();
    if (g.current_governor().money < amount) {
      g.out << "Nice try.\n";
      return;
    }

    const population_t target_planet_pop = p.info(who).popn;
    const int target_tax_rate = p.info(who).tax;
    double x = INSURG_FACTOR * static_cast<double>(amount) *
               static_cast<double>(target_tax_rate) /
               static_cast<double>(target_planet_pop);
    x *= morale_factor(g.race->morale - alien->morale);
    x *= morale_factor(static_cast<double>(eligible - them) /
                       POPULATION_DIFF_SCALE);
    x *= morale_factor(
             TROOP_STRENGTH_SCALE *
             static_cast<double>(g.race->fighters * p.info(Playernum).troops -
                                 alien->fighters * p.info(who).troops)) /
         INSURGENCY_ODDS_DIVISOR;
    const int chance =
        round_rand(ATAN_TO_PERCENT_SCALE * std::atan(x) / std::numbers::pi);
    const bool succeeded = success(chance);
    int changed_hands = 0;
    if (succeeded) {
      g.entity_manager.mutate_sectormap(
          g.snum(), g.pnum(), [&](SectorMap& smap) {
            changed_hands = p.revolt(smap, *alien, Playernum);
          });
      p.info(Playernum).tax = p.info(who).tax;
      /* you inherit their tax rate (insurgency wars he he ) */
    }

    const presentation::InsurgencyResult result{
        .star_name = star.get_name(),
        .planet_name = star.get_planet_name(g.pnum()),
        .instigator_id = Playernum,
        .instigator_name = g.race->name,
        .target_id = who,
        .target_name = alien->name,
        .instigator_system_pop = eligible,
        .target_system_pop = them,
        .instigator_morale = g.race->morale,
        .target_morale = alien->morale,
        .money = amount,
        .target_planet_pop = target_planet_pop,
        .target_tax_rate = target_tax_rate,
        .raw_factor = x,
        .success_chance = chance,
        .succeeded = succeeded,
        .sectors_changed = changed_hands,
    };
    g.present(result);
    warn_player(g.session_registry, g.entity_manager, who, star.governor(who),
                presentation::render_insurgency_telegram(result));
    post(g.entity_manager, presentation::render_insurgency_news(result),
         NewsType::DECLARATION);

    g.entity_manager.mutate_race(Playernum, [&](Race& race) {
      race.governor(Governor).money -= amount;
    });
    ok = true;
  });
  return ok;
}

const CommandDescriptor insurgency_cmd{
    .name = "insurgency",
    .roles =
        {
            .star_control = true,
        },
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::fixed_star(10),
    .min_args = 3,
    .syntax = "insurgency <race> <money>",
    .description = "Finance a planetary insurgency against an occupying player",
    .handler = &insurgency,
};

}  // namespace GB::commands
