// SPDX-License-Identifier: Apache-2.0

/// \file capture.cc
/// \brief Capture ship command.

module;

import std;
import gb.entities;
import gb.mechanics;
import gb.presentation.combat;
import gb.services;
import scnlib;

module commands;

namespace {

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
  const auto Playernum = g.player();
  const auto Governor = g.governor();
  constexpr ap_t APcount = 1;

  if (argv.size() < 2) {
    g.out << "Capture what?\n";
    return false;
  }
  const auto& planet_peek = *g.entity_manager.peek_planet(g.snum(), g.pnum());
  if (planet_peek.is_enslaved_to_foreign(Playernum)) {
    g.out << "That planet has been enslaved!\n";
    return false;
  }
  if (!g.entity_manager.peek_star(g.snum())->control(Playernum, Governor)) {
    g.out << "You are not authorized in this system.\n";
    return false;
  }

  const auto target_shipno = string_to_shipnum(argv[1]);
  if (!target_shipno) {
    g.out << "Illegal ship value.\n";
    return false;
  }

  const auto* ship_peek = g.entity_manager.peek_ship(*target_shipno);
  if (!ship_peek || !ship_peek->alive()) {
    return false;
  }
  if (ship_peek->owner() == Playernum) {
    return false;
  }
  if (!ship_peek->is_landed()) {
    g.out << std::format("{} #{} is not landed on a planet.\n",
                         ship_peek->type_name(), *target_shipno);
    return false;
  }
  if (ship_peek->type() == ShipType::OTYPE_VN) {
    g.out << "You can't capture Von Neumann machines.\n";
    return false;
  }
  if (!g.deduct_ap(ship_peek->storbits(), APcount)) {
    g.out << std::format("You don't have {} action points there.\n", APcount);
    return false;
  }

  const auto& smap_peek = *g.entity_manager.peek_sectormap(
      ship_peek->storbits(), ship_peek->pnumorbits());
  const auto& sect_peek = smap_peek.get(ship_peek->land_coords());
  if (sect_peek.get_owner() != Playernum) {
    g.out << std::format(
        "You don't own the sector where the ship is landed [{}].\n",
        sect_peek.get_owner());
    return false;
  }

  auto what = PopulationType::CIV;
  if (argv.size() >= 4) {
    if (argv[3] == "civilians" || argv[3] == "civ") {
      what = PopulationType::CIV;
    } else if (argv[3] == "military" || argv[3] == "mil") {
      what = PopulationType::MIL;
    } else {
      g.out << "Capture with what?\n";
      return false;
    }
  }

  const auto available_boarders = (what == PopulationType::CIV)
                                      ? sect_peek.get_popn()
                                      : sect_peek.get_troops();
  auto boarders = available_boarders;
  if (argv.size() >= 3) {
    auto parsed = scn::scan<population_t>(argv[2], "{}");
    boarders = parsed ? std::min(parsed->value(), available_boarders) : 0;
  }
  if (boarders <= 0) {
    g.out << "Illegal number of boarders.\n";
    return false;
  }

  const auto res = execute_landed_ship_boarding(
      g.entity_manager, *target_shipno, boarders, what, Playernum, Governor);
  notify_capture_result(g, res);
  g.present(res);
  return true;
}

const CommandDescriptor capture_cmd{
    .name = "capture",
    .roles =
        {
            .star_control = true,
        },
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::dynamic(),
    .min_args = 2,
    .syntax = "capture <ship> [<number>] [civilians|military]",
    .description = "Attempt to capture a landed enemy ship with ground forces",
    .handler = &capture,
};

}  // namespace GB::commands
