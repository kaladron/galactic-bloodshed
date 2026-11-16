// SPDX-License-Identifier: Apache-2.0

/// \file capture.cc
/// \brief Capture ship command.

module;

import std;
import gb.entities;
import gb.mechanics;
import gb.presentation.combat;
import gb.services;

module commands;

namespace {

struct ParsedCaptureArgs {
  PopulationType what{PopulationType::CIV};
  std::optional<population_t> boarders{std::nullopt};
};

std::optional<ParsedCaptureArgs> parse_capture_args(const command_t& argv) {
  if (argv.size() < 3) {
    return ParsedCaptureArgs{};
  }
  if (argv.size() == 3) {
    if (argv[2].starts_with("civ")) {
      return ParsedCaptureArgs{.what = PopulationType::CIV};
    }
    if (argv[2].starts_with("mil")) {
      return ParsedCaptureArgs{.what = PopulationType::MIL};
    }
    return ParsedCaptureArgs{
        .what = PopulationType::CIV,
        .boarders = parse_int<population_t>(argv[2]).value_or(population_t{0}),
    };
  }
  const population_t cnt =
      parse_int<population_t>(argv[2]).value_or(population_t{0});
  if (argv[3].starts_with("civ")) {
    return ParsedCaptureArgs{.what = PopulationType::CIV, .boarders = cnt};
  }
  if (argv[3].starts_with("mil")) {
    return ParsedCaptureArgs{.what = PopulationType::MIL, .boarders = cnt};
  }
  return std::nullopt;
}

void notify_capture_result(GameObj& g, const LandedBoardingResult& res) {
  warn_player(g.session_registry, g.entity_manager, res.old_defender_owner,
              res.old_defender_gov,
              GB::presentation::render_capture_defender_telegram(res));

  const auto news = GB::presentation::render_capture_news(res);
  if (res.captured || !res.ship_alive) {
    post(g.entity_manager, news, NewsType::COMBAT);
  }
  notify_star(g.session_registry, g.entity_manager, g.player(), g.governor(),
              res.star_id, news);
}

}  // namespace

namespace GB::commands {

bool capture(const command_t& argv, GameObj& g) {
  const auto target_shipno = string_to_shipnum(argv[1]);
  if (!target_shipno || *target_shipno <= 0) {
    g.out << "Illegal ship value.\n";
    return false;
  }

  const auto parsed = parse_capture_args(argv);
  if (!parsed) {
    g.out << "Capture with what?\n";
    return false;
  }

  auto res = capture_landed_ship(g.entity_manager, g.snum(), g.pnum(),
                                 *target_shipno, parsed->boarders, parsed->what,
                                 g.player(), g.governor(), g.god());
  if (!res) {
    g.out << GB::presentation::format_capture_error(res.error());
    return false;
  }

  notify_capture_result(g, *res);
  g.present(*res);
  return true;
}

const CommandDescriptor capture_cmd{
    .name = "capture",
    .roles = {.no_guests = true, .star_control = true},
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::dynamic(),
    .min_args = 2,
    .syntax = "capture <ship> [<number>] [civilians|military]",
    .description = "Attempt to capture a landed enemy ship with ground forces",
    .handler = &capture,
};

}  // namespace GB::commands
