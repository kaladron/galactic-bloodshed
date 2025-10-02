// SPDX-License-Identifier: Apache-2.0

/*  order.c -- give orders to ship */

module;

import gblib;
import std.compat;

module gblib;

namespace {
std::string prin_aimed_at(const Ship &ship) {
  if (!std::holds_alternative<AimedAtData>(ship.special)) {
    return "Not aimed";
  }
  const auto& aimed_at = std::get<AimedAtData>(ship.special);
  Place targ{aimed_at.level, aimed_at.snum, aimed_at.pnum, aimed_at.shipno};
  return targ.to_string();
}

/*
 * mark wherever the ship is aimed at, as explored by the owning player.
 */
void mk_expl_aimed_at(GameObj &g, const Ship &s) {
  if (!std::holds_alternative<AimedAtData>(s.special)) {
    g.out << "Ship is not aimed.\n";
    return;
  }
  const auto& aimed_at = std::get<AimedAtData>(s.special);
  auto &str = stars[aimed_at.snum];

  auto xf = s.xpos;
  auto yf = s.ypos;

  switch (aimed_at.level) {
    case ScopeLevel::LEVEL_UNIV:
      g.out << "There is nothing out here to aim at.\n";
      break;
    case ScopeLevel::LEVEL_STAR:
      g.out << std::format("Star {}\n", prin_aimed_at(s));
      if (auto dist = sqrt(Distsq(xf, yf, str.xpos(), str.ypos()));
          dist <= tele_range(s.type, s.tech)) {
        str = getstar(aimed_at.snum);
        setbit(str.explored(), g.player);
        putstar(str, aimed_at.snum);
        g.out << std::format("Surveyed, distance {}.\n", dist);
      } else {
        g.out << std::format("Too far to see ({}, max {}).\n", dist,
                             tele_range(s.type, s.tech));
      }
      break;
    case ScopeLevel::LEVEL_PLAN: {
      g.out << std::format("Planet {}\n", prin_aimed_at(s));
      auto p = getplanet(aimed_at.snum, aimed_at.pnum);
      if (auto dist =
              sqrt(Distsq(xf, yf, str.xpos() + p.xpos, str.ypos() + p.ypos));
          dist <= tele_range(s.type, s.tech)) {
        setbit(str.explored(), g.player);
        p.info[g.player - 1].explored = 1;
        putplanet(p, stars[aimed_at.snum], aimed_at.pnum);
        g.out << std::format("Surveyed, distance {}.\n", dist);
      } else {
        g.out << std::format("Too far to see ({}, max {}).\n", dist,
                             tele_range(s.type, s.tech));
      }
    } break;
    case ScopeLevel::LEVEL_SHIP:
      g.out << std::format("You can't see anything of use there.\n");
      break;
  }
}

// Order handler functions
bool handle_order_defense(GameObj &g, const command_t &argv, Ship &ship) {
  if (!can_bombard(ship)) {
    g.out << "That ship cannot be assigned those orders.\n";
    return false;
  }
  ship.protect.planet = (argv[3] != "off") ? 1 : 0;
  return true;
}

bool handle_order_scatter(GameObj &g, const command_t &argv, Ship &ship) {
  if (ship.type != ShipType::STYPE_MISSILE) {
    g.out << "Only missiles can be given this order.\n";
    return false;
  }
  ship.special = ImpactData{.x = 0, .y = 0, .scatter = 1};
  return true;
}

bool handle_order_impact(GameObj &g, const command_t &argv, Ship &ship) {
  if (ship.type != ShipType::STYPE_MISSILE) {
    g.out << "Only missiles can be designated for this.\n";
    return false;
  }
  int x, y;
  sscanf(argv[3].c_str(), "%d,%d", &x, &y);
  ship.special = ImpactData{
    .x = static_cast<unsigned char>(x),
    .y = static_cast<unsigned char>(y),
    .scatter = 0
  };
  return true;
}

bool handle_order_jump(GameObj &g, const command_t &argv, Ship &ship) {
  if (ship.docked) {
    g.out << "That ship is docked. Use 'launch' or 'undock' first.\n";
    return false;
  }
  if (!ship.hyper_drive.has) {
    g.out << "This ship does not have hyper drive capability.\n";
    return false;
  }
  if (argv[3] == "off") {
    ship.hyper_drive.on = 0;
  } else {
    if (ship.whatdest != ScopeLevel::LEVEL_STAR &&
        ship.whatdest != ScopeLevel::LEVEL_PLAN) {
      g.out << "Destination must be star or planet.\n";
      return false;
    }
    ship.hyper_drive.on = 1;
    ship.navigate.on = 0;
    if (ship.mounted) {
      ship.hyper_drive.charge = 1;
      ship.hyper_drive.ready = 1;
    }
  }
  return true;
}

bool handle_order_protect(GameObj &g, const command_t &argv, Ship &ship) {
  int j = 0;
  if (argv.size() > 3)
    sscanf(argv[3].c_str() + (argv[3][0] == '#'), "%d", &j);
  if (j == ship.number) {
    g.out << "You can't do that.\n";
    return false;
  }
  if (!can_bombard(ship)) {
    g.out << "That ship cannot protect.\n";
    return false;
  }
  if (!j) {
    ship.protect.on = 0;
  } else {
    ship.protect.on = 1;
    ship.protect.ship = j;
  }
  return true;
}

bool handle_order_navigate(GameObj &g, const command_t &argv, Ship &ship) {
  if (argv.size() >= 5) {
    ship.navigate.on = 1;
    ship.navigate.bearing = std::stoi(argv[3]);
    ship.navigate.turns = std::stoi(argv[4]);
  } else {
    ship.navigate.on = 0;
  }
  if (ship.hyper_drive.on) ship.hyper_drive.on = 0;
  return true;
}

bool handle_order_switch(GameObj &g, const command_t &argv, Ship &ship) {
  if (ship.type == ShipType::OTYPE_FACTORY) {
    g.out << "Use \"on\" to bring factory online.\n";
    return false;
  }
  if (!has_switch(ship)) {
    g.out << "That ship does not have an on/off setting.\n";
    return false;
  }
  if (ship.whatorbits == ScopeLevel::LEVEL_SHIP) {
    g.out << "That ship is being transported.\n";
    return false;
  }
  ship.on = !ship.on;
  if (ship.on) {
    switch (ship.type) {
      case ShipType::STYPE_MINE:
        g.out << "Mine armed and ready.\n";
        break;
      case ShipType::OTYPE_TRANSDEV:
        g.out << "Transporter ready to receive.\n";
        break;
      default:
        break;
    }
  } else {
    switch (ship.type) {
      case ShipType::STYPE_MINE:
        g.out << "Mine disarmed.\n";
        break;
      case ShipType::OTYPE_TRANSDEV:
        g.out << "No longer receiving.\n";
        break;
      default:
        break;
    }
  }
  return true;
}

bool handle_order_destination(GameObj &g, const command_t &argv, Ship &ship) {
  if (!speed_rating(ship)) {
    g.out << "That ship cannot be launched.\n";
    return false;
  }
  if (ship.docked) {
    g.out << "That ship is docked; use undock or launch first.\n";
    return false;
  }
  Place where{g, argv[3], true};
  if (where.err) return false;
  
  if (where.level == ScopeLevel::LEVEL_SHIP) {
    auto tmpship = getship(where.shipno);
    if (!followable(ship, *tmpship)) {
      g.out << "Warning: that ship is out of range.\n";
      return false;
    }
    ship.destshipno = where.shipno;
    ship.whatdest = ScopeLevel::LEVEL_SHIP;
  } else {
    /* to foil cheaters */
    if (where.level != ScopeLevel::LEVEL_UNIV &&
        ((ship.storbits != where.snum) &&
         where.level != ScopeLevel::LEVEL_STAR) &&
        isclr(stars[where.snum].explored(), ship.owner)) {
      g.out << "You haven't explored this system.\n";
      return false;
    }
    ship.whatdest = where.level;
    ship.deststar = where.snum;
    ship.destpnum = where.pnum;
  }
  return true;
}

bool handle_order_evade(GameObj &g, const command_t &argv, Ship &ship) {
  if (!max_crew(ship) || !max_speed(ship)) return false;
  ship.protect.evade = (argv[3] == "on") ? 1 : 0;
  return true;
}

bool handle_order_bombard(GameObj &g, const command_t &argv, Ship &ship) {
  if (ship.type == ShipType::OTYPE_OMCL) return true;
  if (!can_bombard(ship)) {
    g.out << "This type of ship cannot be set to retaliate.\n";
    return false;
  }
  ship.bombard = (argv[3] == "on") ? 1 : 0;
  return true;
}

bool handle_order_retaliate(GameObj &g, const command_t &argv, Ship &ship) {
  if (ship.type == ShipType::OTYPE_OMCL) return true;
  if (!can_bombard(ship)) {
    g.out << "This type of ship cannot be set to retaliate.\n";
    return false;
  }
  ship.protect.self = (argv[3] == "on") ? 1 : 0;
  return true;
}

bool handle_order_focus(GameObj &g, const command_t &argv, Ship &ship) {
  if (!ship.laser) {
    g.out << "No laser.\n";
    return false;
  }
  ship.focus = (argv[3] == "on") ? 1 : 0;
  return true;
}

bool handle_order_laser(GameObj &g, const command_t &argv, Ship &ship) {
  if (!ship.laser) {
    g.out << "This ship is not equipped with combat lasers.\n";
    return false;
  }
  if (!can_bombard(ship)) {
    g.out << "This type of ship cannot be set to retaliate.\n";
    return false;
  }
  if (!ship.mounted) {
    g.out << "You do not have a crystal mounted.\n";
    return false;
  }
  ship.fire_laser = (argv[3] == "on") ? std::stoi(argv[4]) : 0;
  return true;
}

bool handle_order_merchant(GameObj &g, const command_t &argv, Ship &ship) {
  if (argv[3] == "off") {
    ship.merchant = 0;
  } else {
    int j = std::stoi(argv[3]);
    if (j < 0 || j > MAX_ROUTES) {
      g.out << "Bad route number.\n";
      return false;
    }
    ship.merchant = j;
  }
  return true;
}

bool handle_order_speed(GameObj &g, const command_t &argv, Ship &ship) {
  if (!speed_rating(ship)) {
    g.out << "This ship does not have a speed rating.\n";
    return false;
  }
  int j = std::stoi(argv[3]);
  if (j < 0) {
    g.out << "Specify a positive speed.\n";
    return false;
  }
  ship.speed = std::min<int>(j, speed_rating(ship));
  return true;
}

bool handle_order_salvo(GameObj &g, const command_t &argv, Ship &ship) {
  if (!can_bombard(ship)) {
    g.out << "This ship cannot be set to retaliate.\n";
    return false;
  }
  int j = std::stoi(argv[3]);
  if (j < 0) {
    g.out << "Specify a positive number of guns.\n";
    return false;
  }
  if (ship.guns == PRIMARY && j > ship.primary)
    j = ship.primary;
  else if (ship.guns == SECONDARY && j > ship.secondary)
    j = ship.secondary;
  else if (ship.guns == GTYPE_NONE)
    j = 0;
  ship.retaliate = j;
  return true;
}

bool handle_order_primary(GameObj &g, const command_t &argv, Ship &ship) {
  if (!ship.primary) {
    g.out << "This ship does not have primary guns.\n";
    return false;
  }
  if (argv.size() < 4) {
    ship.guns = PRIMARY;
    ship.retaliate = std::min<unsigned long>(ship.retaliate, ship.primary);
  } else {
    int j = std::stoi(argv[3]);
    if (j < 0) {
      g.out << "Specify a nonnegative number of guns.\n";
      return false;
    }
    ship.retaliate = std::min<unsigned long>(j, ship.primary);
    ship.guns = PRIMARY;
  }
  return true;
}

bool handle_order_secondary(GameObj &g, const command_t &argv, Ship &ship) {
  if (!ship.secondary) {
    g.out << "This ship does not have secondary guns.\n";
    return false;
  }
  if (argv.size() < 4) {
    ship.guns = SECONDARY;
    ship.retaliate = std::min<unsigned long>(ship.retaliate, ship.secondary);
  } else {
    int j = std::stoi(argv[3]);
    if (j < 0) {
      g.out << "Specify a nonnegative number of guns.\n";
      return false;
    }
    ship.retaliate = std::min<unsigned long>(j, ship.secondary);
    ship.guns = SECONDARY;
  }
  return true;
}

bool handle_order_explosive(GameObj &g, const command_t &argv, Ship &ship) {
  if (ship.type != ShipType::STYPE_MINE && ship.type != ShipType::OTYPE_GR)
    return false;
  ship.mode = 0;
  return true;
}

bool handle_order_radiative(GameObj &g, const command_t &argv, Ship &ship) {
  if (ship.type != ShipType::STYPE_MINE && ship.type != ShipType::OTYPE_GR)
    return false;
  ship.mode = 1;
  return true;
}

bool handle_order_move(GameObj &g, const command_t &argv, Ship &ship) {
  if (ship.type != ShipType::OTYPE_TERRA &&
      ship.type != ShipType::OTYPE_PLOW) {
    g.out << "That ship is not a terraformer or a space plow.\n";
    return false;
  }
  std::string moveseq = (argv.size() > 3) ? argv[3] : "5";
  
  for (auto i = 0; i < moveseq.size(); ++i) {
    if (i == SHIP_NAMESIZE - 1) {
      g.out << std::format("Warning: that is more than {} moves.\n",
                           SHIP_NAMESIZE - 1);
      g.out << "These move orders have been truncated.\n";
      moveseq.resize(i);
      break;
    }
    if ((moveseq[i] == 'c') || (moveseq[i] == 's')) {
      if ((i == 0) && (moveseq[0] == 'c')) {
        g.out << "Cycling move orders can not be empty!\n";
        return false;
      }
      if (moveseq[i + 1]) {
        g.out << std::format(
            "Warning: '{}' should be the last character in the move order.\n",
            moveseq[i]);
        g.out << "These move orders have been truncated.\n";
        moveseq.resize(i + 1);
        break;
      }
    } else if ((moveseq[i] < '1') || ('9' < moveseq[i])) {
      g.out << std::format("'{}' is not a valid move direction.\n",
                           moveseq[i]);
      return false;
    }
  }
  strcpy(ship.shipclass, moveseq.c_str());
  ship.special = TerraformData{.index = 0};
  return true;
}

bool handle_order_trigger(GameObj &g, const command_t &argv, Ship &ship) {
  if (ship.type != ShipType::STYPE_MINE) {
    g.out << "This ship cannot be assigned a trigger radius.\n";
    return false;
  }
  unsigned short radius = std::max(0, std::stoi(argv[3]));
  ship.special = TriggerData{.radius = radius};
  return true;
}

bool handle_order_transport(GameObj &g, const command_t &argv, Ship &ship) {
  if (ship.type != ShipType::OTYPE_TRANSDEV) {
    g.out << "This ship is not a transporter.\n";
    return false;
  }
  unsigned short target = std::stoi(argv[3]);
  if (target == ship.number) {
    g.out << "A transporter cannot transport to itself.";
    target = 0;
  } else {
    g.out << std::format("Target ship is {}.\n", target);
  }
  ship.special = TransportData{.target = target};
  return true;
}

bool handle_order_aim(GameObj &g, const command_t &argv, Ship &ship) {
  if (!can_aim(ship)) {
    g.out << "You can't aim that kind of ship.\n";
    return false;
  }
  if (ship.type != ShipType::OTYPE_GTELE &&
      ship.type != ShipType::OTYPE_TRACT && ship.fuel < FUEL_MANEUVER) {
    g.out << std::format("Not enough maneuvering fuel ({:.2f}).\n",
                         FUEL_MANEUVER);
    return false;
  }
  if (ship.type == ShipType::STYPE_MIRROR && ship.docked) {
    g.out << "docked; use undock or launch first.\n";
    return false;
  }
  Place pl{g, argv[3], true};
  if (pl.err) {
    g.out << "Error in destination.\n";
    return false;
  }
  ship.special = AimedAtData{.shipno = pl.shipno,
                             .snum = pl.snum,
                             .intensity = 0,
                             .pnum = pl.pnum,
                             .level = pl.level};
  if (ship.type != ShipType::OTYPE_TRACT &&
      ship.type != ShipType::OTYPE_GTELE)
    use_fuel(ship, FUEL_MANEUVER);
  if (ship.type == ShipType::OTYPE_GTELE ||
      ship.type == ShipType::OTYPE_STELE)
    mk_expl_aimed_at(g, ship);
  g.out << std::format("Aimed at {}\n", prin_aimed_at(ship));
  return true;
}

bool handle_order_intensity(GameObj &g, const command_t &argv, Ship &ship) {
  if (ship.type != ShipType::STYPE_MIRROR) return false;
  if (std::holds_alternative<AimedAtData>(ship.special)) {
    auto aimed_at = std::get<AimedAtData>(ship.special);
    aimed_at.intensity = std::max(0, std::min(100, std::stoi(argv[3])));
    ship.special = aimed_at;
  }
  return true;
}

bool handle_order_on(GameObj &g, const command_t &argv, Ship &ship) {
  if (!has_switch(ship)) {
    g.out << "This ship does not have an on/off setting.\n";
    return false;
  }
  if (ship.damage && ship.type != ShipType::OTYPE_FACTORY) {
    g.out << "Damaged ships cannot be activated.\n";
    return false;
  }
  if (ship.on) {
    g.out << "This ship is already activated.\n";
    return false;
  }
  if (ship.type == ShipType::OTYPE_FACTORY) {
    unsigned int oncost = 0;
    if (ship.whatorbits == ScopeLevel::LEVEL_SHIP) {
      auto s2 = getship(ship.destshipno);
      if (s2->type == ShipType::STYPE_HABITAT) {
        oncost = HAB_FACT_ON_COST * ship.build_cost;
        if (s2->resource < oncost) {
          g.out << std::format(
              "You don't have {} resources on Habitat #{} to activate this "
              "factory.\n",
              oncost, ship.destshipno);
          return false;
        }
        int hangerneeded =
            (1 + (int)(HAB_FACT_SIZE * (double)ship_size(ship))) -
            ((s2->max_hanger - s2->hanger) + ship.size);
        if (hangerneeded > 0) {
          g.out << std::format(
              "Not enough hanger space free on Habitat #{}. Need {} more.\n",
              ship.destshipno, hangerneeded);
          return false;
        }
        s2->resource -= oncost;
        s2->hanger -= ship.size;
        ship.size = 1 + (int)(HAB_FACT_SIZE * (double)ship_size(ship));
        s2->hanger += ship.size;
        putship(*s2);
      } else {
        g.out << "The factory is currently being transported.\n";
        return false;
      }
    } else if (!landed(ship)) {
      g.out << "You cannot activate the factory here.\n";
      return false;
    } else {
      auto planet = getplanet(ship.deststar, ship.destpnum);
      oncost = 2 * ship.build_cost;
      if (planet.info[g.player - 1].resource < oncost) {
        g.out << std::format(
            "You don't have {} resources on the planet to activate this "
            "factory.\n",
            oncost);
        return false;
      }
      planet.info[g.player - 1].resource -= oncost;
      putplanet(planet, stars[ship.deststar], ship.destpnum);
    }
    g.out << std::format("Factory activated at a cost of {} resources.\n",
                         oncost);
  }
  ship.on = 1;
  return true;
}

bool handle_order_off(GameObj &g, const command_t &argv, Ship &ship) {
  if (ship.type == ShipType::OTYPE_FACTORY && ship.on) {
    g.out << "You can't deactivate a factory once it's online. Consider "
             "using 'scrap'.\n";
    return false;
  }
  ship.on = 0;
  return true;
}
}  // namespace

// TODO(jeffbailey): We take in a non-zero APcount, and do nothing with it!
void give_orders(GameObj &g, const command_t &argv, int /* APcount */,
                 Ship &ship) {
  if (!ship.active) {
    g.out << std::format("{} is irradiated ({}); it cannot be given orders.\n",
                         ship_to_string(ship), ship.rad);
    return;
  }
  if (ship.type != ShipType::OTYPE_TRANSDEV && !ship.popn && max_crew(ship)) {
    g.out << std::format("{} has no crew and is not a robotic ship.\n",
                         ship_to_string(ship));
    return;
  }

  bool success = false;
  const auto& order = argv[2];
  
  if (order == "defense") {
    success = handle_order_defense(g, argv, ship);
  } else if (order == "scatter") {
    success = handle_order_scatter(g, argv, ship);
  } else if (order == "impact") {
    success = handle_order_impact(g, argv, ship);
  } else if (order == "jump") {
    success = handle_order_jump(g, argv, ship);
  } else if (order == "protect") {
    success = handle_order_protect(g, argv, ship);
  } else if (order == "navigate") {
    success = handle_order_navigate(g, argv, ship);
  } else if (order == "switch") {
    success = handle_order_switch(g, argv, ship);
  } else if (order == "destination") {
    success = handle_order_destination(g, argv, ship);
  } else if (order == "evade") {
    success = handle_order_evade(g, argv, ship);
  } else if (order == "bombard") {
    success = handle_order_bombard(g, argv, ship);
  } else if (order == "retaliate") {
    success = handle_order_retaliate(g, argv, ship);
  } else if (order == "focus") {
    success = handle_order_focus(g, argv, ship);
  } else if (order == "laser") {
    success = handle_order_laser(g, argv, ship);
  } else if (order == "merchant") {
    success = handle_order_merchant(g, argv, ship);
  } else if (order == "speed") {
    success = handle_order_speed(g, argv, ship);
  } else if (order == "salvo") {
    success = handle_order_salvo(g, argv, ship);
  } else if (order == "primary") {
    success = handle_order_primary(g, argv, ship);
  } else if (order == "secondary") {
    success = handle_order_secondary(g, argv, ship);
  } else if (order == "explosive") {
    success = handle_order_explosive(g, argv, ship);
  } else if (order == "radiative") {
    success = handle_order_radiative(g, argv, ship);
  } else if (order == "move") {
    success = handle_order_move(g, argv, ship);
  } else if (order == "trigger") {
    success = handle_order_trigger(g, argv, ship);
  } else if (order == "transport") {
    success = handle_order_transport(g, argv, ship);
  } else if (order == "aim") {
    success = handle_order_aim(g, argv, ship);
  } else if (order == "intensity") {
    success = handle_order_intensity(g, argv, ship);
  } else if (order == "on") {
    success = handle_order_on(g, argv, ship);
  } else if (order == "off") {
    success = handle_order_off(g, argv, ship);
  }
  
  if (success) {
    ship.notified = 0;
    putship(ship);
  }
}

void DispOrdersHeader(int Playernum, int Governor) {
  notify(Playernum, Governor,
         "    #       name       sp orbits     destin     options\n");
}

void DispOrders(int Playernum, int Governor, const Ship &ship) {
  if (ship.owner != Playernum || !authorized(Governor, ship) || !ship.alive)
    return;

  std::stringstream buffer;
  if (ship.docked)
    if (ship.whatdest == ScopeLevel::LEVEL_SHIP)
      buffer << "D#" << ship.destshipno;
    else
      buffer << std::format("L{:2d},{:2d}", ship.land_x, ship.land_y);
  else
    buffer << prin_ship_dest(ship);

  buffer << std::format("{:5} {} {:14.14} {}{} {:10.10} {}", ship.number,
                        Shipltrs[ship.type], ship.name,
                        ship.hyper_drive.has ? (ship.mounted ? '+' : '*') : ' ',
                        ship.speed, dispshiploc_brief(ship), buffer.str());

  if (ship.hyper_drive.on) {
    buffer << std::format("/jump {} {}",
                          (ship.hyper_drive.ready ? "ready" : "charging"),
                          ship.hyper_drive.charge);
  }
  if (ship.protect.self) {
    buffer << "/retal";
  }

  if (ship.guns == PRIMARY) {
    switch (ship.primtype) {
      case GTYPE_LIGHT:
        buffer << "/lgt primary";
        break;
      case GTYPE_MEDIUM:
        buffer << "/med primary";
        break;
      case GTYPE_HEAVY:
        buffer << "/hvy primary";
        break;
      case GTYPE_NONE:
        buffer << "/none";
        break;
    }
  } else if (ship.guns == SECONDARY) {
    switch (ship.sectype) {
      case GTYPE_LIGHT:
        buffer << "/lgt secondary";
        break;
      case GTYPE_MEDIUM:
        buffer << "/med secndry";
        break;
      case GTYPE_HEAVY:
        buffer << "/hvy secndry";
        break;
      case GTYPE_NONE:
        buffer << "/none";
        break;
    }
  }

  if (ship.fire_laser) {
    buffer << std::format("/laser {}", ship.fire_laser);
  }
  if (ship.focus) buffer << "/focus";

  if (ship.retaliate) {
    buffer << std::format("/salvo {}", ship.retaliate);
  }
  if (ship.protect.planet) buffer << "/defense";
  if (ship.protect.on) {
    buffer << std::format("/prot {}", ship.protect.ship);
  }
  if (ship.navigate.on) {
    buffer << std::format("/nav {} ({})", ship.navigate.bearing,
                          ship.navigate.turns);
  }
  if (ship.merchant) {
    buffer << std::format("/merchant {}", ship.merchant);
  }
  if (has_switch(ship)) {
    if (ship.on)
      buffer << "/on";
    else
      buffer << "/off";
  }
  if (ship.protect.evade) buffer << "/evade";
  if (ship.bombard) buffer << "/bomb";
  if (ship.type == ShipType::STYPE_MINE || ship.type == ShipType::OTYPE_GR) {
    if (ship.mode)
      buffer << "/radiate";
    else
      buffer << "/explode";
  }
  if (ship.type == ShipType::OTYPE_TERRA || ship.type == ShipType::OTYPE_PLOW) {
    if (std::holds_alternative<TerraformData>(ship.special)) {
      auto terraform = std::get<TerraformData>(ship.special);
      std::string temp = &(ship.shipclass[terraform.index]);
      buffer << std::format("/move {}", temp);

      if (temp[temp.length() - 1] == 'c') {
        std::string hidden = temp;
        hidden = hidden.substr(0, terraform.index);
        buffer << std::format("{}c", hidden);
      }
    }
  }

  if (ship.type == ShipType::STYPE_MISSILE &&
      ship.whatdest == ScopeLevel::LEVEL_PLAN) {
    if (std::holds_alternative<ImpactData>(ship.special)) {
      auto impact = std::get<ImpactData>(ship.special);
      if (impact.scatter)
        buffer << "/scatter";
      else {
        buffer << std::format("/impact {},{}", impact.x, impact.y);
      }
    }
  }

  if (ship.type == ShipType::STYPE_MINE) {
    if (std::holds_alternative<TriggerData>(ship.special)) {
      buffer << std::format("/trigger {}", std::get<TriggerData>(ship.special).radius);
    }
  }
  if (ship.type == ShipType::OTYPE_TRANSDEV) {
    if (std::holds_alternative<TransportData>(ship.special)) {
      buffer << std::format("/target {}", std::get<TransportData>(ship.special).target);
    }
  }
  if (ship.type == ShipType::STYPE_MIRROR) {
    std::string intensity_str = "0";
    if (std::holds_alternative<AimedAtData>(ship.special)) {
      intensity_str = std::to_string(std::get<AimedAtData>(ship.special).intensity);
    }
    buffer << std::format("/aim {}/int {}", prin_aimed_at(ship), intensity_str);
  }

  buffer << "\n";
  notify(Playernum, Governor, buffer.str());
  /* if hyper space is on estimate how much fuel it will cost to get to the
   * destination */
  if (ship.hyper_drive.on) {
    double dist = sqrt(Distsq(ship.xpos, ship.ypos, stars[ship.deststar].xpos(),
                              stars[ship.deststar].ypos()));
    auto distfac = HYPER_DIST_FACTOR * (ship.tech + 100.0);

    double fuse =
        ship.mounted && dist > distfac
            ? HYPER_DRIVE_FUEL_USE * sqrt(ship.mass) * (dist / distfac)
            : HYPER_DRIVE_FUEL_USE * sqrt(ship.mass) * (dist / distfac) *
                  (dist / distfac);

    notify(Playernum, Governor,
           std::format("  *** distance {:.0f} - jump will cost {:.1f}f ***\n",
                       dist, fuse));
    if (ship.max_fuel < fuse)
      notify(Playernum, Governor,
             "Your ship cannot carry enough fuel to do this jump.\n");
  }
}
