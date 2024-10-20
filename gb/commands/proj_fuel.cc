// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void proj_fuel(const command_t &argv, GameObj &g) {
  int opt_settings;
  bool computing = true;
  segments_t current_segs;
  double fuel_usage;
  double level;
  double dist;
  double current_fuel = 0.0;
  double gravity_factor = 0.0;

  if ((argv.size() < 2) || (argv.size() > 3)) {
    g.out << "Invalid number of options.\n\"fuel #<shipnumber> "
             "[destination]\"...\n";
    return;
  }
  if (argv[1][0] != '#') {
    g.out << "Invalid first option.\n\"fuel #<shipnumber> [destination]\"...\n";
    return;
  }
  auto shipno = string_to_shipnum(argv[1]);
  if (!shipno || *shipno > Numships() || *shipno < 1) {
    g.out << std::format("rst: no such ship #{}\n", *shipno);
    return;
  }
  auto ship = getship(*shipno);
  if (ship->owner != g.player) {
    g.out << "You do not own this ship.\n";
    return;
  }
  if (landed(*ship) && (argv.size() == 2)) {
    g.out << "You must specify a destination for landed or docked ships...\n";
    return;
  }
  if (!ship->speed) {
    g.out << "That ship is not moving!\n";
    return;
  }
  if ((!speed_rating(*ship)) || (ship->type == ShipType::OTYPE_FACTORY)) {
    g.out << "That ship does not have a speed rating...\n";
    return;
  }
  std::string plan_buf;
  if (landed(*ship) && (ship->whatorbits == ScopeLevel::LEVEL_PLAN)) {
    const auto p = getplanet(ship->storbits, ship->pnumorbits);
    gravity_factor = p.gravity();
    plan_buf =
        std::format("/{}/{}", stars[ship->storbits].get_name(),
                    stars[ship->storbits].get_planet_name(ship->pnumorbits));
  }
  std::string deststr;
  if (argv.size() == 2) {
    deststr = prin_ship_dest(*ship);
  } else {
    deststr = argv[2];
  }
  Place tmpdest{g, deststr, true};
  if (tmpdest.err) {
    g.out << "fuel:  bad scope.\n";
    return;
  }
  if (tmpdest.level == ScopeLevel::LEVEL_SHIP) {
    auto tmpship = getship(tmpdest.shipno);
    if (!followable(*ship, *tmpship)) {
      g.out << "The ship's destination is out of range.\n";
      return;
    }
  }
  if (tmpdest.level != ScopeLevel::LEVEL_UNIV &&
      tmpdest.level != ScopeLevel::LEVEL_SHIP &&
      ((ship->storbits != tmpdest.snum) &&
       tmpdest.level != ScopeLevel::LEVEL_STAR) &&
      isclr(stars[tmpdest.snum].explored(), ship->owner)) {
    g.out << "You haven't explored the destination system.\n";
    return;
  }
  if (tmpdest.level == ScopeLevel::LEVEL_UNIV) {
    g.out << "Invalid ship destination.\n";
    return;
  }
  double x_0 = ship->xpos;
  double y_0 = ship->ypos;

  double x_1;
  double y_1;

  if (tmpdest.level == ScopeLevel::LEVEL_UNIV) {
    g.out << "That ship currently has no destination orders...\n";
    return;
  }
  if (tmpdest.level == ScopeLevel::LEVEL_SHIP) {
    auto tmpship = getship(tmpdest.shipno);
    if (tmpship->owner != g.player) {
      g.out << "Nice try.\n";
      return;
    }
    x_1 = tmpship->xpos;
    y_1 = tmpship->ypos;
  } else if (tmpdest.level == ScopeLevel::LEVEL_PLAN) {
    const auto p = getplanet(tmpdest.snum, tmpdest.pnum);
    x_1 = p.xpos + stars[tmpdest.snum].xpos();
    y_1 = p.ypos + stars[tmpdest.snum].ypos();
  } else if (tmpdest.level == ScopeLevel::LEVEL_STAR) {
    x_1 = stars[tmpdest.snum].xpos();
    y_1 = stars[tmpdest.snum].ypos();
  } else {
    printf("ERROR 99\n");
    return;
  }

  /* compute the distance */
  dist = sqrt(Distsq(x_0, y_0, x_1, y_1));

  if (dist <= DIST_TO_LAND) {
    g.out << "That ship is within 10.0 units of the destination.\n";
    return;
  }

  /*  First get the results based on current fuel load.  */
  auto fuelcheckship = getship(*shipno);
  level = fuelcheckship->fuel;
  auto [current_settings, number_segments] = do_trip(
      tmpdest, *fuelcheckship, fuelcheckship->fuel, gravity_factor, x_1, y_1);
  current_segs = number_segments;
  if (current_settings) current_fuel = level - fuelcheckship->fuel;
  level = fuelcheckship->max_fuel;

  /*  2nd loop to determine lowest fuel needed...  */
  fuel_usage = level;
  opt_settings = 0;
  while (computing) {
    auto tmpship = getship(*shipno);
    std::tie(computing, number_segments) =
        do_trip(tmpdest, *tmpship, level, gravity_factor, x_1, y_1);
    if ((computing) && (tmpship->fuel >= 0.05)) {
      fuel_usage = level;
      opt_settings = 1;
      level -= tmpship->fuel;
    } else if (computing) {
      computing = false;
      fuel_usage = level;
    }
  }

  auto tmpship = getship(*shipno);
  g.out << std::format(
      "\n  ----- ===== FUEL ESTIMATES ===== ----\n\nAt Current Fuel "
      "Cargo ({:.2f}f):\n",
      tmpship->fuel);
  domass(*tmpship);
  if (!current_settings) {
    g.out << "The ship will not be able to complete the trip.\n";
  } else {
    fuel_output(g, dist, current_fuel, gravity_factor, tmpship->mass,
                current_segs, plan_buf);
  }
  g.out << std::format("At Optimum Fuel Level ({:.2f}f):\n", fuel_usage);
  if (!opt_settings) {
    g.out << std::format("The ship will not be able to complete the trip.\n");
  } else {
    tmpship->fuel = fuel_usage;
    domass(*tmpship);
    fuel_output(g, dist, fuel_usage, gravity_factor, tmpship->mass,
                number_segments, plan_buf);
  }
}
}  // namespace GB::commands
