// SPDX-License-Identifier: Apache-2.0

/// \file order.cc
/// \brief Give orders to ships and format standing ship order reports.

module;

import scnlib;
import std;

module gblib;

namespace {

std::string format_aim_target(EntityManager& em, const Ship& ship) {
  const auto* mirror = ship.as<SpaceMirrorShip>();
  if (!mirror) {
    return "Not aimed";
  }
  const auto& aimed_at = mirror->aim();
  switch (aimed_at.level) {
    case ScopeLevel::LEVEL_UNIV:
      return "";
    case ScopeLevel::LEVEL_STAR: {
      const auto* star = em.peek_star(aimed_at.snum);
      return std::format("/{}", star ? star->get_name() : "Unknown");
    }
    case ScopeLevel::LEVEL_PLAN: {
      const auto* star = em.peek_star(aimed_at.snum);
      return std::format("/{}/{}", star ? star->get_name() : "Unknown",
                         star ? star->get_planet_name(aimed_at.pnum)
                              : "Unknown");
    }
    case ScopeLevel::LEVEL_SHIP:
      return std::format("#{}", aimed_at.shipno);
  }
  return "";
}

/*
 * mark wherever the ship is aimed at, as explored by the owning player.
 */
void survey_aim_target(GameObj& g, const Ship& s) {
  const auto* mirror = s.as<SpaceMirrorShip>();
  if (!mirror) {
    g.out << "Ship is not aimed.\n";
    return;
  }
  const auto& aimed_at = mirror->aim();
  const auto& str = *g.entity_manager.peek_star(aimed_at.snum);
  const auto coords = s.coordinates();

  switch (aimed_at.level) {
    case ScopeLevel::LEVEL_UNIV:
      g.out << "There is nothing out here to aim at.\n";
      break;
    case ScopeLevel::LEVEL_STAR:
      g.out << std::format("Star {}\n", format_aim_target(g.entity_manager, s));
      if (auto dist = coords.distance_to(str.coordinates());
          dist <= tele_range(s.type(), s.tech())) {
        g.entity_manager.mutate_star(aimed_at.snum, [&](Star& star) {
          star.mark_explored_by(g.player());
        });
        g.out << std::format("Surveyed, distance {}.\n", dist);
      } else {
        g.out << std::format("Too far to see ({}, max {}).\n", dist,
                             tele_range(s.type(), s.tech()));
      }
      break;
    case ScopeLevel::LEVEL_PLAN: {
      g.out << std::format("Planet {}\n",
                           format_aim_target(g.entity_manager, s));
      const auto& p =
          *g.entity_manager.peek_planet(aimed_at.snum, aimed_at.pnum);
      if (auto dist = coords.distance_to(p.absolute_coordinates(str));
          dist <= tele_range(s.type(), s.tech())) {
        g.entity_manager.mutate_star(aimed_at.snum, [&](Star& star) {
          star.mark_explored_by(g.player());
        });
        g.entity_manager.mutate_planet(
            aimed_at.snum, aimed_at.pnum,
            [&](Planet& planet) { planet.info(g.player()).explored = 1; });
        g.out << std::format("Surveyed, distance {}.\n", dist);
      } else {
        g.out << std::format("Too far to see ({}, max {}).\n", dist,
                             tele_range(s.type(), s.tech()));
      }
    } break;
    case ScopeLevel::LEVEL_SHIP:
      g.out << "You can't see anything of use there.\n";
      break;
  }
}

void order_defense(GameObj& g, const command_t& argv, Ship& ship) {
  if (!ship.can_bombard()) {
    g.out << "That ship cannot be assigned those orders.\n";
    return;
  }
  ship.protect().planet = (argv.size() <= 3 || argv[3] != "off");
}

void order_scatter(GameObj& g, const command_t& /*argv*/, Ship& ship) {
  auto* missile = ship.as<MissileShip>();
  if (!missile) {
    g.out << "Only missiles can be given this order.\n";
    return;
  }
  missile->set_scatter();
}

void order_impact(GameObj& g, const command_t& argv, Ship& ship) {
  auto* missile = ship.as<MissileShip>();
  if (!missile) {
    g.out << "Only missiles can be designated for this.\n";
    return;
  }
  auto coords = (argv.size() > 3) ? Coordinates::parse(argv[3]) : std::nullopt;
  if (!coords) {
    g.out << "Usage: order <ship> designate <x>,<y>\n";
    return;
  }
  missile->set_impact_coords(*coords);
}

void order_jump(GameObj& g, const command_t& argv, Ship& ship) {
  if (ship.docked()) {
    g.out << "That ship is docked. Use 'launch' or 'undock' first.\n";
    return;
  }
  if (!ship.hyper_drive().has) {
    g.out << "This ship does not have hyper drive capability.\n";
    return;
  }
  if (argv.size() > 3 && argv[3] == "off") {
    ship.hyper_drive().on = 0;
    return;
  }
  if (ship.whatdest() != ScopeLevel::LEVEL_STAR &&
      ship.whatdest() != ScopeLevel::LEVEL_PLAN) {
    g.out << "Destination must be star or planet.\n";
    return;
  }
  ship.hyper_drive().on = true;
  ship.navigate().on = false;
  if (ship.mounted()) {
    ship.hyper_drive().charge = HYPER_DRIVE_READY_CHARGE;
  }
}

void order_protect(GameObj& g, const command_t& argv, Ship& ship) {
  shipnum_t target_ship{0};
  if (argv.size() > 3) {
    if (auto target_num = string_to_shipnum(argv[3])) {
      target_ship = *target_num;
    }
  }
  if (target_ship == ship.number()) {
    g.out << "You can't do that.\n";
    return;
  }
  if (!ship.can_bombard()) {
    g.out << "That ship cannot protect.\n";
    return;
  }
  if (target_ship == 0) {
    ship.protect().on = false;
  } else {
    ship.protect().on = true;
    ship.protect().ship = target_ship;
  }
}

void order_navigate(GameObj& /*g*/, const command_t& argv, Ship& ship) {
  if (argv.size() >= 5) {
    auto bearing = scn::scan<unsigned>(argv[3], "{}");
    auto turns = scn::scan<unsigned>(argv[4], "{}");
    if (bearing && turns) {
      ship.navigate().on = true;
      ship.navigate().bearing = bearing->value();
      ship.navigate().turns = turns->value();
    } else {
      ship.navigate().on = false;
    }
  } else {
    ship.navigate().on = false;
  }
  if (ship.hyper_drive().on) {
    ship.hyper_drive().on = false;
  }
}

void order_switch(GameObj& g, const command_t& /*argv*/, Ship& ship) {
  if (ship.type() == ShipType::OTYPE_FACTORY) {
    g.out << "Use \"on\" to bring factory online.\n";
    return;
  }
  if (!ship.has_switch()) {
    g.out << "That ship does not have an on/off setting.\n";
    return;
  }
  if (ship.whatorbits() == ScopeLevel::LEVEL_SHIP) {
    g.out << "That ship is being transported.\n";
    return;
  }
  ship.on() = !ship.on();
  if (ship.type() == ShipType::STYPE_MINE) {
    g.out << (ship.on() ? "Mine armed and ready.\n" : "Mine disarmed.\n");
  } else if (ship.type() == ShipType::OTYPE_TRANSDEV) {
    g.out << (ship.on() ? "Transporter ready to receive.\n"
                        : "No longer receiving.\n");
  }
}

void set_ship_follow_destination(GameObj& g, Ship& ship,
                                 shipnum_t target_ship) {
  try {
    bool is_followable = false;
    g.entity_manager.with_ship(target_ship, [&](const Ship& tmpship) {
      is_followable = followable(g.entity_manager, ship, tmpship);
    });
    if (!is_followable) {
      g.out << "Warning: that ship is out of range.\n";
      return;
    }
  } catch (const EntityNotFoundError&) {
    g.out << "Warning: that ship is out of range.\n";
    return;
  }
  ship.destshipno() = target_ship;
  ship.whatdest() = ScopeLevel::LEVEL_SHIP;
}

void set_celestial_destination(GameObj& g, Ship& ship, const Place& where) {
  /* to foil cheaters */
  if (where.level != ScopeLevel::LEVEL_UNIV && ship.storbits() != where.snum &&
      where.level != ScopeLevel::LEVEL_STAR &&
      !g.entity_manager.peek_star(where.snum)->is_explored_by(ship.owner())) {
    g.out << "You haven't explored this system.\n";
    return;
  }
  ship.whatdest() = where.level;
  ship.deststar() = where.snum;
  ship.destpnum() = where.pnum;
}

void order_destination(GameObj& g, const command_t& argv, Ship& ship) {
  if (!ship.max_speed_capacity()) {
    g.out << "That ship cannot be launched.\n";
    return;
  }
  if (ship.docked()) {
    g.out << "That ship is docked; use undock or launch first.\n";
    return;
  }
  if (argv.size() <= 3) {
    return;
  }
  Place where{g, argv[3], true};
  if (where.err) {
    return;
  }
  if (where.level == ScopeLevel::LEVEL_SHIP) {
    set_ship_follow_destination(g, ship, where.shipno);
  } else {
    set_celestial_destination(g, ship, where);
  }
}

void order_evade(GameObj& /*g*/, const command_t& argv, Ship& ship) {
  if (!ship.max_crew_capacity() || !ship.max_speed_capacity() ||
      argv.size() <= 3) {
    return;
  }
  if (argv[3] == "on") {
    ship.protect().evade = true;
  } else if (argv[3] == "off") {
    ship.protect().evade = false;
  }
}

void order_bombard(GameObj& g, const command_t& argv, Ship& ship) {
  if (ship.type() == ShipType::OTYPE_OMCL) {
    return;
  }
  if (!ship.can_bombard()) {
    g.out << "This type of ship cannot be set to retaliate.\n";
    return;
  }
  if (argv.size() <= 3) {
    return;
  }
  if (argv[3] == "off") {
    ship.bombard() = 0;
  } else if (argv[3] == "on") {
    ship.bombard() = 1;
  }
}

void order_retaliate(GameObj& g, const command_t& argv, Ship& ship) {
  if (ship.type() == ShipType::OTYPE_OMCL) {
    return;
  }
  if (!ship.can_bombard()) {
    g.out << "This type of ship cannot be set to retaliate.\n";
    return;
  }
  if (argv.size() <= 3) {
    return;
  }
  if (argv[3] == "off") {
    ship.protect().self = false;
  } else if (argv[3] == "on") {
    ship.protect().self = true;
  }
}

void order_focus(GameObj& g, const command_t& argv, Ship& ship) {
  if (!ship.laser()) {
    g.out << "No laser.\n";
    return;
  }
  ship.focus() = (argv.size() > 3 && argv[3] == "on") ? 1 : 0;
}

void order_laser(GameObj& g, const command_t& argv, Ship& ship) {
  if (!ship.laser()) {
    g.out << "This ship is not equipped with combat lasers.\n";
    return;
  }
  if (!ship.can_bombard()) {
    g.out << "This type of ship cannot be set to retaliate.\n";
    return;
  }
  if (!ship.mounted()) {
    g.out << "You do not have a crystal mounted.\n";
    return;
  }
  if (argv.size() > 3 && argv[3] == "on") {
    if (argv.size() > 4) {
      auto res = scn::scan<weapon_power_t>(argv[4], "{}");
      ship.fire_laser() = res ? res->value() : 0;
    } else {
      ship.fire_laser() = 0;
    }
  } else {
    ship.fire_laser() = 0;
  }
}

void order_merchant(GameObj& g, const command_t& argv, Ship& ship) {
  if (argv.size() <= 3) {
    return;
  }
  if (argv[3] == "off") {
    ship.merchant() = 0;
    return;
  }
  auto res = scn::scan<int>(argv[3], "{}");
  if (!res || res->value() < 0 || res->value() > MAX_ROUTES) {
    g.out << "Bad route number.\n";
    return;
  }
  ship.merchant() = res->value();
}

void order_speed(GameObj& g, const command_t& argv, Ship& ship) {
  if (!ship.max_speed_capacity()) {
    g.out << "This ship does not have a speed rating.\n";
    return;
  }
  if (argv.size() <= 3) {
    g.out << "Specify a positive speed.\n";
    return;
  }
  auto res = scn::scan<speed_t>(argv[3], "{}");
  if (!res) {
    g.out << "Specify a positive speed.\n";
    return;
  }
  ship.speed() = std::min(res->value(), ship.max_speed_capacity());
}

void order_salvo(GameObj& g, const command_t& argv, Ship& ship) {
  if (!ship.can_bombard()) {
    g.out << "This ship cannot be set to retaliate.\n";
    return;
  }
  if (argv.size() <= 3) {
    g.out << "Specify a positive number of guns.\n";
    return;
  }
  auto res = scn::scan<gun_count_t>(argv[3], "{}");
  if (!res) {
    g.out << "Specify a positive number of guns.\n";
    return;
  }
  const auto* battery = ship.active_gun_battery();
  ship.retaliate() = battery ? std::min(res->value(), battery->count) : 0;
}

void order_battery(GameObj& g, const command_t& argv, Ship& ship,
                   ActiveBattery mode) {
  const auto& battery =
      (mode == PRIMARY) ? ship.primary_battery() : ship.secondary_battery();
  const char* name = (mode == PRIMARY) ? "primary" : "secondary";
  if (!battery.has_guns()) {
    g.out << std::format("This ship does not have {} guns.\n", name);
    return;
  }
  if (argv.size() < 4) {
    ship.guns() = mode;
    ship.retaliate() = std::min(ship.retaliate(), battery.count);
    return;
  }
  auto res = scn::scan<gun_count_t>(argv[3], "{}");
  if (!res) {
    g.out << "Specify a nonnegative number of guns.\n";
    return;
  }
  ship.retaliate() = std::min(res->value(), battery.count);
  ship.guns() = mode;
}

void order_primary(GameObj& g, const command_t& argv, Ship& ship) {
  order_battery(g, argv, ship, PRIMARY);
}

void order_secondary(GameObj& g, const command_t& argv, Ship& ship) {
  order_battery(g, argv, ship, SECONDARY);
}

void order_explosive(GameObj& /*g*/, const command_t& /*argv*/, Ship& ship) {
  if (auto* mine = ship.as<MineShip>()) {
    mine->set_radiative(false);
  } else if (ship.type() == ShipType::OTYPE_GR) {
    ship.mode() = 0;
  }
}

void order_radiative(GameObj& /*g*/, const command_t& /*argv*/, Ship& ship) {
  if (auto* mine = ship.as<MineShip>()) {
    mine->set_radiative(true);
  } else if (ship.type() == ShipType::OTYPE_GR) {
    ship.mode() = 1;
  }
}

bool validate_move_sequence(GameObj& g, std::string& moveseq) {
  for (std::size_t i = 0; i < moveseq.size(); ++i) {
    if (i == SHIP_NAMESIZE - 1) {
      g.out << std::format("Warning: that is more than {} moves.\n",
                           SHIP_NAMESIZE - 1);
      g.out << "These move orders have been truncated.\n";
      moveseq.resize(i);
      break;
    }
    if (moveseq[i] == 'c' || moveseq[i] == 's') {
      if (i == 0 && moveseq[0] == 'c') {
        g.out << "Cycling move orders can not be empty!\n";
        return false;
      }
      if (i + 1 < moveseq.size()) {
        g.out << std::format(
            "Warning: '{}' should be the last character in the move order.\n",
            moveseq[i]);
        g.out << "These move orders have been truncated.\n";
        moveseq.resize(i + 1);
        break;
      }
    } else if (moveseq[i] < '1' || moveseq[i] > '9') {
      g.out << std::format("'{}' is not a valid move direction.\n", moveseq[i]);
      return false;
    }
  }
  return true;
}

void order_move(GameObj& g, const command_t& argv, Ship& ship) {
  auto* terraform = ship.as<TerraformerShip>();
  if (!terraform) {
    g.out << "That ship is not a terraformer or a space plow.\n";
    return;
  }
  std::string moveseq = (argv.size() > 3) ? argv[3] : "5";
  if (!validate_move_sequence(g, moveseq)) {
    return;
  }
  terraform->shipclass() = moveseq;
  terraform->set_index(0);
}

void order_trigger(GameObj& g, const command_t& argv, Ship& ship) {
  auto* mine = ship.as<MineShip>();
  if (!mine) {
    g.out << "This ship cannot be assigned a trigger radius.\n";
    return;
  }
  if (argv.size() <= 3) {
    mine->set_trigger_radius(0);
    return;
  }
  auto res = scn::scan<weapon_range_t>(argv[3], "{}");
  mine->set_trigger_radius(res ? res->value() : 0);
}

void order_transport(GameObj& g, const command_t& argv, Ship& ship) {
  auto* transporter = ship.as<TransporterShip>();
  if (!transporter) {
    g.out << "This ship is not a transporter.\n";
    return;
  }
  shipnum_t target{0};
  if (argv.size() > 3) {
    auto res = scn::scan<shipnum_t::value_type>(argv[3], "{}");
    target = res ? shipnum_t{res->value()} : 0;
  }
  if (target == ship.number()) {
    g.out << "A transporter cannot transport to itself.\n";
    target = 0;
  } else {
    g.out << std::format("Target ship is {}.\n", target);
  }
  transporter->set_target_ship(target);
}

bool requires_maneuver_fuel_to_aim(const Ship& ship) {
  return ship.type() != ShipType::OTYPE_GTELE &&
         ship.type() != ShipType::OTYPE_TRACT;
}

void order_aim(GameObj& g, const command_t& argv, Ship& ship) {
  if (!ship.can_aim()) {
    g.out << "You can't aim that kind of ship.\n";
    return;
  }
  if (requires_maneuver_fuel_to_aim(ship) && ship.fuel() < FUEL_MANEUVER) {
    g.out << std::format("Not enough maneuvering fuel ({:.2f}).\n",
                         FUEL_MANEUVER);
    return;
  }
  if (ship.type() == ShipType::STYPE_MIRROR && ship.docked()) {
    g.out << "docked; use undock or launch first.\n";
    return;
  }
  if (argv.size() <= 3) {
    g.out << "Error in destination.\n";
    return;
  }
  Place pl{g, argv[3], true};
  if (pl.err) {
    g.out << "Error in destination.\n";
    return;
  }
  if (auto* mirror = ship.as<SpaceMirrorShip>()) {
    mirror->aim() = AimedAtData{.shipno = pl.shipno,
                                .snum = pl.snum,
                                .intensity = 0,
                                .pnum = pl.pnum,
                                .level = pl.level};
  }
  if (requires_maneuver_fuel_to_aim(ship)) {
    use_fuel(ship, FUEL_MANEUVER);
  }
  if (ship.type() == ShipType::OTYPE_GTELE ||
      ship.type() == ShipType::OTYPE_STELE) {
    survey_aim_target(g, ship);
  }
  g.out << std::format("Aimed at {}\n",
                       format_aim_target(g.entity_manager, ship));
}

void order_intensity(GameObj& /*g*/, const command_t& argv, Ship& ship) {
  if (auto* mirror = ship.as<SpaceMirrorShip>()) {
    int val = 0;
    if (argv.size() > 3) {
      if (auto res = scn::scan<int>(argv[3], "{}")) {
        val = res->value();
      }
    }
    mirror->set_intensity(static_cast<char>(std::clamp(val, 0, 100)));
  }
}

bool activate_factory_on_habitat(GameObj& g, Ship& factory,
                                 resource_t& oncost) {
  bool ok = false;
  g.entity_manager.mutate_ship(factory.destshipno(), [&](Ship& habitat) {
    if (habitat.type() != ShipType::STYPE_HABITAT) {
      g.out << "The factory is currently being transported.\n";
      return;
    }
    oncost = HAB_FACT_ON_COST * factory.build_cost();
    if (habitat.resource() < oncost) {
      g.out << std::format(
          "You don't have {} resources on Habitat #{} to activate this "
          "factory.\n",
          oncost, factory.destshipno());
      return;
    }
    const int new_size =
        1 + static_cast<int>(HAB_FACT_SIZE *
                             static_cast<double>(ship_size(factory)));
    const int hanger_needed =
        new_size - ((habitat.max_hanger() - habitat.hanger()) + factory.size());
    if (hanger_needed > 0) {
      g.out << std::format(
          "Not enough hanger space free on Habitat #{}. Need {} more.\n",
          factory.destshipno(), hanger_needed);
      return;
    }
    habitat.resource() -= oncost;
    habitat.hanger() -= factory.size();
    factory.size() = new_size;
    habitat.hanger() += factory.size();
    ok = true;
  });
  return ok;
}

bool activate_factory_on_planet(GameObj& g, const Ship& factory,
                                resource_t& oncost) {
  bool ok = false;
  g.entity_manager.mutate_planet(
      factory.deststar(), factory.destpnum(), [&](Planet& planet) {
        oncost = 2 * factory.build_cost();
        if (planet.info(g.player()).resource < oncost) {
          g.out << std::format(
              "You don't have {} resources on the planet to activate this "
              "factory.\n",
              oncost);
          return;
        }
        planet.info(g.player()).resource -= oncost;
        ok = true;
      });
  return ok;
}

void order_on(GameObj& g, const command_t& /*argv*/, Ship& ship) {
  if (!ship.has_switch()) {
    g.out << "This ship does not have an on/off setting.\n";
    return;
  }
  if (ship.damage() && ship.type() != ShipType::OTYPE_FACTORY) {
    g.out << "Damaged ships cannot be activated.\n";
    return;
  }
  if (ship.on()) {
    g.out << "This ship is already activated.\n";
    return;
  }
  if (ship.type() == ShipType::OTYPE_FACTORY) {
    resource_t oncost = 0;
    if (ship.whatorbits() == ScopeLevel::LEVEL_SHIP) {
      if (!activate_factory_on_habitat(g, ship, oncost)) return;
    } else if (!ship.is_landed()) {
      g.out << "You cannot activate the factory here.\n";
      return;
    } else {
      if (!activate_factory_on_planet(g, ship, oncost)) return;
    }
    g.out << std::format("Factory activated at a cost of {} resources.\n",
                         oncost);
  }
  ship.on() = 1;
}

void order_off(GameObj& g, const command_t& /*argv*/, Ship& ship) {
  if (ship.type() == ShipType::OTYPE_FACTORY && ship.on()) {
    g.out << "You can't deactivate a factory once it's online. Consider "
             "using 'scrap'.\n";
    return;
  }
  ship.on() = 0;
}

struct OrderDispatchEntry {
  std::string_view name;
  void (*handler)(GameObj&, const command_t&, Ship&);
};

constexpr std::array<OrderDispatchEntry, 27> order_handlers = {{
    {"defense", &order_defense},
    {"scatter", &order_scatter},
    {"impact", &order_impact},
    {"jump", &order_jump},
    {"protect", &order_protect},
    {"navigate", &order_navigate},
    {"switch", &order_switch},
    {"destination", &order_destination},
    {"evade", &order_evade},
    {"bombard", &order_bombard},
    {"retaliate", &order_retaliate},
    {"focus", &order_focus},
    {"laser", &order_laser},
    {"merchant", &order_merchant},
    {"speed", &order_speed},
    {"salvo", &order_salvo},
    {"primary", &order_primary},
    {"secondary", &order_secondary},
    {"explosive", &order_explosive},
    {"radiative", &order_radiative},
    {"move", &order_move},
    {"trigger", &order_trigger},
    {"transport", &order_transport},
    {"aim", &order_aim},
    {"intensity", &order_intensity},
    {"on", &order_on},
    {"off", &order_off},
}};

std::string format_ship_destination(const Ship& ship) {
  if (!ship.docked()) {
    return prin_ship_dest(ship);
  }
  if (ship.whatdest() == ScopeLevel::LEVEL_SHIP) {
    return std::format("D#{}", ship.destshipno());
  }
  return std::format("L{:2d},{:2d}", ship.land_coords().x,
                     ship.land_coords().y);
}

std::string format_active_battery_option(const Ship& ship) {
  if (ship.guns() == ActiveBattery::NONE) {
    return "";
  }
  const auto* battery = ship.active_gun_battery();
  if (!battery) {
    return "/none";
  }
  std::string_view cal_prefix;
  switch (battery->caliber) {
    case guntype_t::LIGHT:
      cal_prefix = "/lgt ";
      break;
    case guntype_t::MEDIUM:
      cal_prefix = "/med ";
      break;
    case guntype_t::HEAVY:
      cal_prefix = "/hvy ";
      break;
    default:
      break;
  }
  std::string_view bat_name =
      (ship.guns() == PRIMARY)
          ? "primary"
          : ((battery->caliber == guntype_t::LIGHT) ? "secondary" : "secndry");
  return std::format("{}{}", cal_prefix, bat_name);
}

std::string format_combat_options(const Ship& ship) {
  std::string out;
  if (ship.hyper_drive().on) {
    out += std::format("/jump {} {}",
                       (ship.hyper_drive().is_ready() ? "ready" : "charging"),
                       ship.hyper_drive().charge);
  }
  if (ship.protect().self) out += "/retal";
  out += format_active_battery_option(ship);
  if (ship.fire_laser()) out += std::format("/laser {}", ship.fire_laser());
  if (ship.focus()) out += "/focus";
  if (ship.retaliate()) out += std::format("/salvo {}", ship.retaliate());
  if (ship.protect().planet) out += "/defense";
  if (ship.protect().on) out += std::format("/prot {}", ship.protect().ship);
  return out;
}

std::string format_navigation_and_switch_options(const Ship& ship) {
  std::string out;
  if (ship.navigate().on) {
    out += std::format("/nav {} ({})", ship.navigate().bearing,
                       ship.navigate().turns);
  }
  if (ship.merchant()) out += std::format("/merchant {}", ship.merchant());
  if (ship.has_switch()) {
    out += ship.on() ? "/on" : "/off";
  }
  if (ship.protect().evade) out += "/evade";
  return out;
}

std::string format_specialty_options(EntityManager& em, const Ship& ship) {
  std::string out;
  if (const auto* mine = ship.as<MineShip>()) {
    out += mine->mode() ? "/radiate" : "/explode";
    out += std::format("/trigger {}", mine->trigger_radius());
  } else if (ship.type() == ShipType::OTYPE_GR) {
    out += ship.mode() ? "/radiate" : "/explode";
  }

  if (const auto* terraform = ship.as<TerraformerShip>()) {
    std::string temp = &(terraform->shipclass()[terraform->index()]);
    out += std::format("/move {}", temp);
    if (!temp.empty() && temp.back() == 'c') {
      std::string hidden = terraform->shipclass().substr(0, terraform->index());
      out += std::format("{}c", hidden);
    }
  }

  if (const auto* missile = ship.as<MissileShip>()) {
    if (missile->whatdest() == ScopeLevel::LEVEL_PLAN) {
      out += missile->is_scatter()
                 ? "/scatter"
                 : std::format("/impact {},{}", missile->impact_coords().x,
                               missile->impact_coords().y);
    }
  }

  if (const auto* trans = ship.as<TransporterShip>()) {
    out += std::format("/target {}", trans->target_ship().value);
  }

  if (const auto* mirror = ship.as<SpaceMirrorShip>()) {
    out += std::format("/aim {}/int {}", format_aim_target(em, *mirror),
                       static_cast<int>(mirror->intensity()));
  }
  return out;
}

std::string format_hyperdrive_jump_summary(EntityManager& em,
                                           const Ship& ship) {
  if (!ship.hyper_drive().on) {
    return "";
  }
  const auto* dest_star = em.peek_star(ship.deststar());
  if (!dest_star) {
    return "";
  }
  const double dist = ship.coordinates().distance_to(dest_star->coordinates());
  const double distfac = HYPER_DIST_FACTOR * (ship.tech() + 100.0);
  const double ratio = dist / distfac;
  const double fuse =
      (ship.mounted() && dist > distfac)
          ? HYPER_DRIVE_FUEL_USE * std::sqrt(ship.mass()) * ratio
          : HYPER_DRIVE_FUEL_USE * std::sqrt(ship.mass()) * ratio * ratio;

  std::string out = std::format(
      "  *** distance {:.0f} - jump will cost {:.1f}f ***\n", dist, fuse);
  if (ship.max_fuel_capacity() < fuse) {
    out += "Your ship cannot carry enough fuel to do this jump.\n";
  }
  return out;
}

}  // namespace

// TODO(jeffbailey): We take in a non-zero APcount, and do nothing with it!
void give_orders(GameObj& g, const command_t& argv, int /* APcount */,
                 Ship& ship) {
  if (!ship.active()) {
    g.out << std::format("{} is irradiated ({}); it cannot be given orders.\n",
                         ship, ship.rad());
    return;
  }
  if (ship.type() != ShipType::OTYPE_TRANSDEV && !ship.popn() &&
      ship.max_crew_capacity()) {
    g.out << std::format("{} has no crew and is not a robotic ship.\n", ship);
    return;
  }

  if (argv.size() > 2) {
    for (const auto& entry : order_handlers) {
      if (entry.name == argv[2]) {
        entry.handler(g, argv, ship);
        break;
      }
    }
  }
  ship.notified() = 0;
}

void display_orders_header(GameObj& g) {
  g.out << "    #       name       sp orbits     destin     options\n";
}

void display_orders(GameObj& g, const Ship& ship) {
  if (ship.owner() != g.player() || !authorized(g.governor(), ship) ||
      !ship.alive()) {
    return;
  }

  const char hyper_indicator =
      ship.hyper_drive().has ? (ship.mounted() ? '+' : '*') : ' ';
  const std::string dest_str = format_ship_destination(ship);

  g.out << std::format(
      "{:5} {} {:14.14} {}{} {:10.10} {}{}{}{}\n", ship.number(),
      ship.type_letter(), ship.name(), hyper_indicator, ship.speed(),
      dispshiploc_brief(g.entity_manager, ship), dest_str,
      format_combat_options(ship), format_navigation_and_switch_options(ship),
      format_specialty_options(g.entity_manager, ship));

  g.out << format_hyperdrive_jump_summary(g.entity_manager, ship);
}
