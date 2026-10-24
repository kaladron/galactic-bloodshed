// SPDX-License-Identifier: Apache-2.0

/// \file bless.cc
/// \brief Bestow divine blessings upon a player.

module;

import gb.entities;
import gb.presentation.empire_views;
import gb.services;
import scnlib;
import std;

module commands;

namespace GB::commands {

namespace {

using GB::presentation::BlessProperty;
using GB::presentation::BlessResult;
using GB::presentation::BlessValue;

void emit_blessing(GameObj& g, const BlessResult& result) {
  warn_player(g.session_registry, g.entity_manager, result.target_player,
              Race::leader_id,
              GB::presentation::render_bless_notification(result));
  g.present(result);
}

void emit_planet_blessing(GameObj& g, player_t who, BlessProperty prop,
                          BlessValue value = {}) {
  const auto& star = *g.entity_manager.peek_star(g.snum());
  emit_blessing(g, BlessResult{
                       .target_player = who,
                       .property = prop,
                       .value = std::move(value),
                       .star_name = star.get_name(),
                       .planet_name = star.get_planet_name(g.pnum()),
                   });
}

struct RaceIntProp {
  std::string_view name;
  BlessProperty property;
  void (*apply)(Race&, int);
};

constexpr std::array<RaceIntProp, 7> race_int_props{{
    {.name = "money",
     .property = BlessProperty::Money,
     .apply = [](Race& r, int amt) { r.leader().money += amt; }},
    {.name = "morale",
     .property = BlessProperty::Morale,
     .apply = [](Race& r, int amt) { r.morale += amt; }},
    {.name = "fertility",
     .property = BlessProperty::Fertility,
     .apply = [](Race& r, int amt) { r.fertilize = amt; }},
    {.name = "IQ",
     .property = BlessProperty::Iq,
     .apply = [](Race& r, int amt) { r.IQ = amt; }},
    {.name = "fight",
     .property = BlessProperty::Fight,
     .apply = [](Race& r, int amt) { r.fighters = amt; }},
    {.name = "technology",
     .property = BlessProperty::Technology,
     .apply = [](Race& r, int amt) { r.tech += static_cast<double>(amt); }},
    {.name = "maxiq",
     .property = BlessProperty::MaxIq,
     .apply = [](Race& r, int amt) { r.IQ_limit = amt; }},
}};

struct RaceFloatProp {
  std::string_view name;
  BlessProperty property;
  void (*apply)(Race&, double);
};

constexpr std::array<RaceFloatProp, 4> race_float_props{{
    {.name = "mass",
     .property = BlessProperty::Mass,
     .apply = [](Race& r, double v) { r.mass = v; }},
    {.name = "metabolism",
     .property = BlessProperty::Metabolism,
     .apply = [](Race& r, double v) { r.metabolism = v; }},
    {.name = "adventurism",
     .property = BlessProperty::Adventurism,
     .apply = [](Race& r, double v) { r.adventurism = v; }},
    {.name = "birthrate",
     .property = BlessProperty::Birthrate,
     .apply = [](Race& r, double v) { r.birthrate = v; }},
}};

struct RaceFlagProp {
  std::string_view name;
  BlessProperty property;
  void (*apply)(Race&);
};

constexpr std::array<RaceFlagProp, 7> race_flag_props{{
    {.name = "pods",
     .property = BlessProperty::Pods,
     .apply = [](Race& r) { r.pods = true; }},
    {.name = "nopods",
     .property = BlessProperty::NoPods,
     .apply = [](Race& r) { r.pods = false; }},
    {.name = "collectiveiq",
     .property = BlessProperty::CollectiveIq,
     .apply = [](Race& r) { r.collective_iq = true; }},
    {.name = "nocollectiveiq",
     .property = BlessProperty::NoCollectiveIq,
     .apply = [](Race& r) { r.collective_iq = false; }},
    {.name = "guest",
     .property = BlessProperty::Guest,
     .apply = [](Race& r) { r.Guest = true; }},
    {.name = "god",
     .property = BlessProperty::God,
     .apply = [](Race& r) { r.God = true; }},
    {.name = "mortal",
     .property = BlessProperty::Mortal,
     .apply =
         [](Race& r) {
           r.God = false;
           r.Guest = false;
         }},
}};

struct SectorPref {
  std::string_view name;
  BlessProperty property;
  SectorType type;
};

constexpr std::array<SectorPref, 8> sector_prefs{{
    {.name = "water",
     .property = BlessProperty::Water,
     .type = SectorType::SEC_SEA},
    {.name = "land",
     .property = BlessProperty::Land,
     .type = SectorType::SEC_LAND},
    {.name = "mountain",
     .property = BlessProperty::Mountain,
     .type = SectorType::SEC_MOUNT},
    {.name = "gas",
     .property = BlessProperty::Gas,
     .type = SectorType::SEC_GAS},
    {.name = "ice",
     .property = BlessProperty::Ice,
     .type = SectorType::SEC_ICE},
    {.name = "forest",
     .property = BlessProperty::Forest,
     .type = SectorType::SEC_FOREST},
    {.name = "desert",
     .property = BlessProperty::Desert,
     .type = SectorType::SEC_DESERT},
    {.name = "plated",
     .property = BlessProperty::Plated,
     .type = SectorType::SEC_PLATED},
}};

std::optional<bool> bless_race_flags(player_t who, std::string_view prop,
                                     GameObj& g) {
  for (const auto& entry : race_flag_props) {
    if (entry.name == prop) {
      g.entity_manager.mutate_race(who, [&](Race& race) { entry.apply(race); });
      emit_blessing(g, {.target_player = who, .property = entry.property});
      return true;
    }
  }
  return std::nullopt;
}

std::optional<bool> bless_race_floats(player_t who, std::string_view prop,
                                      std::string_view val_str, GameObj& g) {
  for (const auto& entry : race_float_props) {
    if (entry.name == prop) {
      auto val = scn::scan<double>(val_str, "{}");
      if (!val) {
        g.out << "Invalid numeric value.\n";
        return false;
      }
      g.entity_manager.mutate_race(
          who, [&](Race& race) { entry.apply(race, val->value()); });
      emit_blessing(g, {.target_player = who,
                        .property = entry.property,
                        .value = val->value()});
      return true;
    }
  }
  return std::nullopt;
}

std::optional<bool> bless_race_ints(player_t who, std::string_view prop,
                                    std::string_view val_str, GameObj& g) {
  for (const auto& entry : race_int_props) {
    if (entry.name == prop) {
      auto val = scn::scan<int>(val_str, "{}");
      if (!val) {
        g.out << "Invalid amount.\n";
        return false;
      }
      g.entity_manager.mutate_race(
          who, [&](Race& race) { entry.apply(race, val->value()); });
      emit_blessing(g, {.target_player = who,
                        .property = entry.property,
                        .value = val->value()});
      return true;
    }
  }
  return std::nullopt;
}

std::optional<bool> bless_race_prefs(player_t who, std::string_view prop,
                                     std::string_view val_str, GameObj& g) {
  for (const auto& entry : sector_prefs) {
    if (entry.name == prop) {
      auto val = scn::scan<int>(val_str, "{}");
      if (!val) {
        g.out << "Invalid preference percentage.\n";
        return false;
      }
      g.entity_manager.mutate_race(who, [&](Race& race) {
        race.likes[entry.type] = 0.01 * static_cast<double>(val->value());
      });
      emit_blessing(g, {.target_player = who,
                        .property = entry.property,
                        .value = val->value()});
      return true;
    }
  }
  return std::nullopt;
}

std::optional<bool> bless_race_property(player_t who, std::string_view prop,
                                        std::string_view val_str, GameObj& g) {
  if (prop == "password") {
    g.entity_manager.mutate_race(
        who, [&](Race& race) { race.password = std::string(val_str); });
    emit_blessing(g, {.target_player = who,
                      .property = BlessProperty::Password,
                      .value = std::string(val_str)});
    return true;
  }

  if (auto res = bless_race_flags(who, prop, g); res.has_value()) {
    return res;
  }
  if (auto res = bless_race_floats(who, prop, val_str, g); res.has_value()) {
    return res;
  }
  if (auto res = bless_race_ints(who, prop, val_str, g); res.has_value()) {
    return res;
  }
  return bless_race_prefs(who, prop, val_str, g);
}

std::optional<bool> bless_planet_or_star(player_t who, std::string_view prop,
                                         std::string_view val_str, GameObj& g) {
  if (prop == "explorebit") {
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
      planet.info(who).explored = 1;
    });
    g.entity_manager.mutate_star(
        g.snum(), [&](Star& star) { star.mark_explored_by(who); });
    emit_planet_blessing(g, who, BlessProperty::ExploreBit);
    return true;
  }

  if (prop == "noexplorebit") {
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
      planet.info(who).explored = 0;
    });
    emit_planet_blessing(g, who, BlessProperty::NoExploreBit);
    return true;
  }

  if (prop == "planetpopulation") {
    auto val = scn::scan<int>(val_str, "{}");
    if (!val) {
      g.out << "Invalid population count.\n";
      return false;
    }
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
      planet.info(who).popn = val->value();
    });
    emit_planet_blessing(g, who, BlessProperty::PlanetPopulation, val->value());
    return true;
  }

  if (prop == "inhabited") {
    g.entity_manager.mutate_star(
        g.snum(), [&](Star& star) { star.mark_inhabited_by(who); });
    emit_planet_blessing(g, who, BlessProperty::Inhabited);
    return true;
  }

  if (prop == "numsectsowned") {
    auto val = scn::scan<int>(val_str, "{}");
    if (!val) {
      g.out << "Invalid sector count.\n";
      return false;
    }
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
      planet.info(who).numsectsowned = val->value();
    });
    emit_planet_blessing(g, who, BlessProperty::NumSectsOwned, val->value());
    return true;
  }

  return std::nullopt;
}

enum class BlessCommodity : std::uint8_t {
  Resources,
  Destruct,
  Fuel,
  Crystals,
  ActionPoints,
};

struct CommodityAlias {
  std::string_view alias;
  BlessCommodity commodity;
  BlessProperty property;
};

constexpr std::array<CommodityAlias, 13> commodity_aliases{{
    {"r", BlessCommodity::Resources, BlessProperty::Resources},
    {"resource", BlessCommodity::Resources, BlessProperty::Resources},
    {"resources", BlessCommodity::Resources, BlessProperty::Resources},
    {"d", BlessCommodity::Destruct, BlessProperty::Destruct},
    {"destruct", BlessCommodity::Destruct, BlessProperty::Destruct},
    {"f", BlessCommodity::Fuel, BlessProperty::Fuel},
    {"fuel", BlessCommodity::Fuel, BlessProperty::Fuel},
    {"x", BlessCommodity::Crystals, BlessProperty::Crystals},
    {"crystal", BlessCommodity::Crystals, BlessProperty::Crystals},
    {"crystals", BlessCommodity::Crystals, BlessProperty::Crystals},
    {"a", BlessCommodity::ActionPoints, BlessProperty::ActionPoints},
    {"ap", BlessCommodity::ActionPoints, BlessProperty::ActionPoints},
    {"action", BlessCommodity::ActionPoints, BlessProperty::ActionPoints},
}};

std::optional<CommodityAlias> parse_commodity(std::string_view prop) {
  for (const auto& entry : commodity_aliases) {
    if (entry.alias == prop) {
      return entry;
    }
  }
  return std::nullopt;
}

bool bless_commodity(player_t who, CommodityAlias commod, int amount,
                     GameObj& g) {
  switch (commod.commodity) {
    case BlessCommodity::Resources:
      g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
        planet.info(who).resource += amount;
      });
      break;
    case BlessCommodity::Destruct:
      g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
        planet.info(who).destruct += amount;
      });
      break;
    case BlessCommodity::Fuel:
      g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
        planet.info(who).fuel += amount;
      });
      break;
    case BlessCommodity::Crystals:
      g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
        planet.info(who).crystals += amount;
      });
      break;
    case BlessCommodity::ActionPoints:
      g.entity_manager.mutate_star(g.snum(),
                                   [&](Star& s) { s.AP(who) += amount; });
      break;
  }
  emit_planet_blessing(g, who, commod.property, amount);
  return true;
}

}  // namespace

bool bless(const command_t& argv, GameObj& g) {
  if (argv.size() > 4) {
    g.out << "Syntax: bless <player> <what> <+amount>\n";
    return false;
  }

  const player_t who = get_player(g.entity_manager, argv[1]);
  if (who == 0) {
    g.out << "No such player number.\n";
    return false;
  }

  std::string_view prop = argv[2];
  std::string_view val_str = argv[3];

  if (auto res = bless_race_property(who, prop, val_str, g); res.has_value()) {
    return *res;
  }

  if (auto res = bless_planet_or_star(who, prop, val_str, g); res.has_value()) {
    return *res;
  }

  auto commod = parse_commodity(prop);
  if (!commod) {
    g.out << "No such commodity.\n";
    return false;
  }

  auto parsed_amount = scn::scan<int>(val_str, "{}");
  if (!parsed_amount) {
    g.out << "Invalid amount.\n";
    return false;
  }

  return bless_commodity(who, *commod, parsed_amount->value(), g);
}

const CommandDescriptor bless_cmd{
    .name = "bless",
    .roles = {.god_only = true},
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::free(),
    .min_args = 4,
    .syntax = "bless <player> <what> <+amount>",
    .description = "Bestow divine blessings upon a player (deity only)",
    .handler = &bless,
};

}  // namespace GB::commands
