// SPDX-License-Identifier: Apache-2.0

/// \file bless.cc
/// \brief Bestow divine blessings upon a player.

module;

import gb.entities;
import gb.services;
import notification;
import scnlib;
import session;
import std;

module commands;

namespace GB::commands {

namespace {

struct RaceIntProp {
  std::string_view name;
  void (*apply)(Race&, int);
  std::string (*message)(const Race&, int);
};

constexpr std::array<RaceIntProp, 7> kRaceIntProps{{
    {.name = "money",
     .apply = [](Race& r, int amt) { r.leader().money += amt; },
     .message =
         [](const Race&, int amt) {
           return std::format("Deity gave you {} money.\n", amt);
         }},
    {.name = "morale",
     .apply = [](Race& r, int amt) { r.morale += amt; },
     .message =
         [](const Race&, int amt) {
           return std::format("Deity gave you {} morale.\n", amt);
         }},
    {.name = "fertility",
     .apply = [](Race& r, int amt) { r.fertilize = amt; },
     .message =
         [](const Race&, int amt) {
           return std::format("Deity gave you a fetilization ability of {}.\n",
                              amt);
         }},
    {.name = "IQ",
     .apply = [](Race& r, int amt) { r.IQ = amt; },
     .message =
         [](const Race&, int amt) {
           return std::format("Deity gave you {} IQ.\n", amt);
         }},
    {.name = "fight",
     .apply = [](Race& r, int amt) { r.fighters = amt; },
     .message =
         [](const Race&, int amt) {
           return std::format("Deity set your fighting ability to {}.\n", amt);
         }},
    {.name = "technology",
     .apply = [](Race& r, int amt) { r.tech += static_cast<double>(amt); },
     .message =
         [](const Race&, int amt) {
           return std::format("Deity gave you {} technology.\n", amt);
         }},
    {.name = "maxiq",
     .apply = [](Race& r, int amt) { r.IQ_limit = amt; },
     .message =
         [](const Race& r, int) {
           return std::format("Deity gave you a maximum IQ of {}.\n",
                              r.IQ_limit);
         }},
}};

struct RaceFloatProp {
  std::string_view name;
  void (*apply)(Race&, float);
  std::string (*message)(const Race&);
};

constexpr std::array<RaceFloatProp, 4> kRaceFloatProps{{
    {.name = "mass",
     .apply = [](Race& r, float v) { r.mass = v; },
     .message =
         [](const Race& r) {
           return std::format("Deity gave you {:.2f} mass.\n", r.mass);
         }},
    {.name = "metabolism",
     .apply = [](Race& r, float v) { r.metabolism = v; },
     .message =
         [](const Race& r) {
           return std::format("Deity gave you {:.2f} metabolism.\n",
                              r.metabolism);
         }},
    {.name = "adventurism",
     .apply = [](Race& r, float v) { r.adventurism = v; },
     .message =
         [](const Race& r) {
           return std::format("Deity gave you {:<3.0f}% adventurism.\n",
                              r.adventurism * 100.0);
         }},
    {.name = "birthrate",
     .apply = [](Race& r, float v) { r.birthrate = v; },
     .message =
         [](const Race& r) {
           return std::format("Deity gave you {:.2f} birthrate.\n",
                              r.birthrate);
         }},
}};

struct RaceFlagProp {
  std::string_view name;
  void (*apply)(Race&);
  std::string_view message;
};

constexpr std::array<RaceFlagProp, 7> kRaceFlagProps{{
    {.name = "pods",
     .apply = [](Race& r) { r.pods = true; },
     .message = "Deity gave you pod ability.\n"},
    {.name = "nopods",
     .apply = [](Race& r) { r.pods = false; },
     .message = "Deity took away pod ability.\n"},
    {.name = "collectiveiq",
     .apply = [](Race& r) { r.collective_iq = true; },
     .message = "Deity gave you collective intelligence.\n"},
    {.name = "nocollectiveiq",
     .apply = [](Race& r) { r.collective_iq = false; },
     .message = "Deity took away collective intelligence.\n"},
    {.name = "guest",
     .apply = [](Race& r) { r.Guest = true; },
     .message = "Deity turned you into a guest race.\n"},
    {.name = "god",
     .apply = [](Race& r) { r.God = true; },
     .message = "Deity turned you into a deity race.\n"},
    {.name = "mortal",
     .apply =
         [](Race& r) {
           r.God = false;
           r.Guest = false;
         },
     .message = "Deity turned you into a mortal race.\n"},
}};

struct SectorPref {
  std::string_view name;
  SectorType type;
};

constexpr std::array<SectorPref, 8> kSectorPrefs{{
    {.name = "water", .type = SectorType::SEC_SEA},
    {.name = "land", .type = SectorType::SEC_LAND},
    {.name = "mountain", .type = SectorType::SEC_MOUNT},
    {.name = "gas", .type = SectorType::SEC_GAS},
    {.name = "ice", .type = SectorType::SEC_ICE},
    {.name = "forest", .type = SectorType::SEC_FOREST},
    {.name = "desert", .type = SectorType::SEC_DESERT},
    {.name = "plated", .type = SectorType::SEC_PLATED},
}};

std::optional<bool> bless_race_flags(player_t who, std::string_view prop,
                                     GameObj& g) {
  for (const auto& entry : kRaceFlagProps) {
    if (entry.name == prop) {
      g.entity_manager.mutate_race(who, [&](Race& race) {
        entry.apply(race);
        warn_player(g.session_registry, g.entity_manager, who, 0,
                    std::string(entry.message));
      });
      return true;
    }
  }
  return std::nullopt;
}

std::optional<bool> bless_race_floats(player_t who, std::string_view prop,
                                      std::string_view val_str, GameObj& g) {
  for (const auto& entry : kRaceFloatProps) {
    if (entry.name == prop) {
      auto val = scn::scan<float>(val_str, "{}");
      if (!val) {
        g.out << "Invalid numeric value.\n";
        return false;
      }
      g.entity_manager.mutate_race(who, [&](Race& race) {
        entry.apply(race, val->value());
        warn_player(g.session_registry, g.entity_manager, who, 0,
                    entry.message(race));
      });
      return true;
    }
  }
  return std::nullopt;
}

std::optional<bool> bless_race_ints(player_t who, std::string_view prop,
                                    std::string_view val_str, GameObj& g) {
  for (const auto& entry : kRaceIntProps) {
    if (entry.name == prop) {
      auto val = scn::scan<int>(val_str, "{}");
      if (!val) {
        g.out << "Invalid amount.\n";
        return false;
      }
      g.entity_manager.mutate_race(who, [&](Race& race) {
        entry.apply(race, val->value());
        warn_player(g.session_registry, g.entity_manager, who, 0,
                    entry.message(race, val->value()));
      });
      return true;
    }
  }
  return std::nullopt;
}

std::optional<bool> bless_race_prefs(player_t who, std::string_view prop,
                                     std::string_view val_str, GameObj& g) {
  for (const auto& entry : kSectorPrefs) {
    if (entry.name == prop) {
      auto val = scn::scan<int>(val_str, "{}");
      if (!val) {
        g.out << "Invalid preference percentage.\n";
        return false;
      }
      g.entity_manager.mutate_race(who, [&](Race& race) {
        race.likes[entry.type] = 0.01 * static_cast<double>(val->value());
        warn_player(g.session_registry, g.entity_manager, who, 0,
                    std::format("Deity set your {} preference to {}%\n",
                                entry.name, val->value()));
      });
      return true;
    }
  }
  return std::nullopt;
}

std::optional<bool> bless_race_property(player_t who, std::string_view prop,
                                        std::string_view val_str, GameObj& g) {
  if (prop == "password") {
    g.entity_manager.mutate_race(who, [&](Race& race) {
      race.password = std::string(val_str);
      warn_player(
          g.session_registry, g.entity_manager, who, 0,
          std::format("Deity changed your race password to `{}`\n", val_str));
    });
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
    g.entity_manager.mutate_star(g.snum(), [&](Star& star) {
      star.mark_explored_by(who);
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  std::format("Deity set your explored bit at /{}/{}.\n",
                              star.get_name(), star.get_planet_name(g.pnum())));
    });
    return true;
  }

  if (prop == "noexplorebit") {
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
      planet.info(who).explored = 0;
    });
    const auto& star = *g.entity_manager.peek_star(g.snum());
    warn_player(g.session_registry, g.entity_manager, who, 0,
                std::format("Deity reset your explored bit at /{}/{}.\n",
                            star.get_name(), star.get_planet_name(g.pnum())));
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
    const auto& star = *g.entity_manager.peek_star(g.snum());
    warn_player(
        g.session_registry, g.entity_manager, who, 0,
        std::format("Deity set your population variable to {} at /{}/{}.\n",
                    val->value(), star.get_name(),
                    star.get_planet_name(g.pnum())));
    return true;
  }

  if (prop == "inhabited") {
    g.entity_manager.mutate_star(g.snum(), [&](Star& star) {
      star.mark_inhabited_by(who);
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  std::format("Deity has set your inhabited bit for /{}/{}.\n",
                              star.get_name(), star.get_planet_name(g.pnum())));
    });
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
    const auto& star = *g.entity_manager.peek_star(g.snum());
    warn_player(
        g.session_registry, g.entity_manager, who, 0,
        std::format(
            "Deity set your \"numsectsowned\" variable at /{}/{} to {}.\n",
            star.get_name(), star.get_planet_name(g.pnum()), val->value()));
    return true;
  }

  return std::nullopt;
}

std::optional<char> parse_commodity(std::string_view prop) {
  if (prop == "r" || prop == "resource" || prop == "resources") return 'r';
  if (prop == "d" || prop == "destruct") return 'd';
  if (prop == "f" || prop == "fuel") return 'f';
  if (prop == "x" || prop == "crystal" || prop == "crystals") return 'x';
  if (prop == "a" || prop == "ap" || prop == "action") return 'a';
  return std::nullopt;
}

bool bless_commodity(player_t who, char commod, int amount, GameObj& g) {
  const auto& star = *g.entity_manager.peek_star(g.snum());
  switch (commod) {
    case 'r':
      g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
        planet.info(who).resource += amount;
      });
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  std::format("Deity gave you {} resources at {}/{}.\n", amount,
                              star.get_name(), star.get_planet_name(g.pnum())));
      return true;
    case 'd':
      g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
        planet.info(who).destruct += amount;
      });
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  std::format("Deity gave you {} destruct at {}/{}.\n", amount,
                              star.get_name(), star.get_planet_name(g.pnum())));
      return true;
    case 'f':
      g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
        planet.info(who).fuel += amount;
      });
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  std::format("Deity gave you {} fuel at {}/{}.\n", amount,
                              star.get_name(), star.get_planet_name(g.pnum())));
      return true;
    case 'x':
      g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
        planet.info(who).crystals += amount;
      });
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  std::format("Deity gave you {} crystals at {}/{}.\n", amount,
                              star.get_name(), star.get_planet_name(g.pnum())));
      return true;
    case 'a':
      g.entity_manager.mutate_star(g.snum(),
                                   [&](Star& s) { s.AP(who) += amount; });
      warn_player(g.session_registry, g.entity_manager, who, 0,
                  std::format("Deity gave you {} action points at {}.\n",
                              amount, star.get_name()));
      return true;
    default:
      g.out << "No such commodity.\n";
      return false;
  }
}

}  // namespace

bool bless(const command_t& argv, GameObj& g) {
  auto parsed_who = scn::scan<int>(argv[1], "{}");
  if (!parsed_who || parsed_who->value() < 1 ||
      parsed_who->value() > g.entity_manager.num_races()) {
    g.out << "No such player number.\n";
    return false;
  }
  player_t who{parsed_who->value()};

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
