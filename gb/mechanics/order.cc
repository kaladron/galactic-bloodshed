// SPDX-License-Identifier: Apache-2.0

/// \file order.cc
/// \brief Give orders to ships and format standing ship order reports.

module;

import scnlib;
import std;

module gb.mechanics;

namespace {

std::string format_aim_target(EntityManager& em, const Ship& ship) {
  const auto* mirror = ship.as<SpaceMirrorShip>();
  if (!mirror) {
    return "Not aimed";
  }
  switch (mirror->aimed_level()) {
    case ScopeLevel::LEVEL_UNIV:
      return "";
    case ScopeLevel::LEVEL_STAR: {
      if (!mirror->aimed_star()) return "/Unknown";
      const auto* star = em.peek_star(*mirror->aimed_star());
      return std::format("/{}", star ? star->get_name() : "Unknown");
    }
    case ScopeLevel::LEVEL_PLAN: {
      if (!mirror->aimed_star() || !mirror->aimed_planet()) {
        return "/Unknown/Unknown";
      }
      const auto* star = em.peek_star(*mirror->aimed_star());
      return std::format("/{}/{}", star ? star->get_name() : "Unknown",
                         star ? star->get_planet_name(*mirror->aimed_planet())
                              : "Unknown");
    }
    case ScopeLevel::LEVEL_SHIP:
      return std::format("#{}", mirror->aimed_ship().value_or(0));
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
  const auto coords = s.coordinates();

  switch (mirror->aimed_level()) {
    case ScopeLevel::LEVEL_UNIV:
      g.out << "There is nothing out here to aim at.\n";
      break;
    case ScopeLevel::LEVEL_STAR: {
      if (!mirror->aimed_star()) break;
      const starnum_t aimed_star = *mirror->aimed_star();
      const auto& str = *g.entity_manager.peek_star(aimed_star);
      g.out << std::format("Star {}\n", format_aim_target(g.entity_manager, s));
      if (auto dist = coords.distance_to(str.coordinates());
          dist <= s.tele_range()) {
        g.entity_manager.mutate_star(
            aimed_star, [&](Star& star) { star.mark_explored_by(g.player()); });
        g.out << std::format("Surveyed, distance {}.\n", dist);
      } else {
        g.out << std::format("Too far to see ({}, max {}).\n", dist,
                             s.tele_range());
      }
      break;
    }
    case ScopeLevel::LEVEL_PLAN: {
      if (!mirror->aimed_star() || !mirror->aimed_planet()) break;
      const starnum_t aimed_star = *mirror->aimed_star();
      const planetnum_t aimed_planet = *mirror->aimed_planet();
      const auto& str = *g.entity_manager.peek_star(aimed_star);
      g.out << std::format("Planet {}\n",
                           format_aim_target(g.entity_manager, s));
      const auto& p = *g.entity_manager.peek_planet(aimed_star, aimed_planet);
      if (auto dist = coords.distance_to(p.absolute_coordinates(str));
          dist <= s.tele_range()) {
        g.entity_manager.mutate_star(
            aimed_star, [&](Star& star) { star.mark_explored_by(g.player()); });
        g.entity_manager.mutate_planet(
            aimed_star, aimed_planet,
            [&](Planet& planet) { planet.info(g.player()).explored = 1; });
        g.out << std::format("Surveyed, distance {}.\n", dist);
      } else {
        g.out << std::format("Too far to see ({}, max {}).\n", dist,
                             s.tele_range());
      }
      break;
    }
    case ScopeLevel::LEVEL_SHIP:
      g.out << "You can't see anything of use there.\n";
      break;
  }
}

std::expected<OrderUpdate, OrderError> order_defense(const command_t& argv,
                                                     Ship& ship) {
  if (!ship.can_bombard()) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::CannotBeAssignedOrders,
    });
  }
  ship.protect().planet = (argv.size() <= 3 || argv[3] != "off");
  return OrderUpdate{};
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

std::expected<OrderUpdate, OrderError> order_jump(const command_t& argv,
                                                  Ship& ship) {
  if (ship.docked()) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::ShipDockedUseLaunchOrUndock,
    });
  }
  if (!ship.hyper_drive().has) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::NoHyperDriveCapability,
    });
  }
  if (argv.size() > 3 && argv[3] == "off") {
    ship.hyper_drive().on = 0;
    return OrderUpdate{};
  }
  if (ship.whatdest() != ScopeLevel::LEVEL_STAR &&
      ship.whatdest() != ScopeLevel::LEVEL_PLAN) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::DestinationMustBeStarOrPlanet,
    });
  }
  ship.hyper_drive().on = true;
  ship.navigate().on = false;
  if (ship.mounted()) {
    ship.hyper_drive().charge = HYPER_DRIVE_READY_CHARGE;
  }
  return OrderUpdate{};
}

std::expected<OrderUpdate, OrderError> order_protect(const command_t& argv,
                                                     Ship& ship) {
  std::optional<shipnum_t> target_ship{std::nullopt};
  if (argv.size() > 3) {
    if (auto target_num = string_to_shipnum(argv[3]);
        target_num && *target_num > 0) {
      target_ship = *target_num;
    }
  }
  if (target_ship == ship.number()) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::CannotProtectSelf,
    });
  }
  if (!ship.can_bombard()) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::CannotProtect,
    });
  }
  if (!target_ship) {
    ship.protect().on = false;
    ship.protect().ship = std::nullopt;
  } else {
    ship.protect().on = true;
    ship.protect().ship = target_ship;
  }
  return OrderUpdate{};
}

std::expected<OrderUpdate, OrderError> order_navigate(const command_t& argv,
                                                      Ship& ship) {
  if (argv.size() >= 5) {
    auto bearing = scn::scan<unsigned>(argv[3], "{}");
    auto turns = scn::scan<unsigned>(argv[4], "{}");
    if (bearing && turns && turns->value() > 0) {
      ship.navigate().on = true;
      ship.navigate().bearing = bearing->value() % 360;
      ship.navigate().turns = turns->value();
    } else {
      ship.navigate().on = false;
      ship.navigate().turns = 0;
    }
  } else {
    ship.navigate().on = false;
  }
  if (ship.hyper_drive().on) {
    ship.hyper_drive().on = false;
  }
  return OrderUpdate{};
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

std::expected<OrderUpdate, OrderError>
set_ship_follow_destination(EntityManager& em, Ship& ship,
                            shipnum_t target_ship) {
  if (!followable(em, ship, *em.peek_ship(target_ship))) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::TargetShipOutOfRange,
    });
  }
  ship.set_ship_destination(target_ship);
  return OrderUpdate{};
}

std::expected<OrderUpdate, OrderError>
set_celestial_destination(EntityManager& em, Ship& ship, const Place& where) {
  /* to foil cheaters */
  if (where.level != ScopeLevel::LEVEL_UNIV && ship.storbits() != where.snum &&
      where.level != ScopeLevel::LEVEL_STAR &&
      !em.peek_star(where.snum)->is_explored_by(ship.owner())) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::SystemUnexplored,
    });
  }
  ship.set_destination(where.level, where.snum, where.pnum);
  return OrderUpdate{};
}

std::expected<OrderUpdate, OrderError>
order_destination(EntityManager& em, const ScopeContext& scope_ctx,
                  const command_t& argv, Ship& ship) {
  if (!ship.max_speed_capacity()) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::CannotBeLaunched,
    });
  }
  if (ship.docked()) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::ShipDockedUndockOrLaunchFirst,
    });
  }
  if (argv.size() <= 3) {
    return OrderUpdate{.modified = false};
  }
  auto where = Place::resolve(em, scope_ctx, argv[3], true);
  if (!where) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::InvalidPlace,
        .place_error = where.error(),
    });
  }
  if (where->level == ScopeLevel::LEVEL_SHIP) {
    return set_ship_follow_destination(em, ship, where->shipno);
  }
  return set_celestial_destination(em, ship, *where);
}

std::expected<OrderUpdate, OrderError> order_evade(const command_t& argv,
                                                   Ship& ship) {
  if (!ship.max_crew_capacity() || !ship.max_speed_capacity() ||
      argv.size() <= 3) {
    return OrderUpdate{.modified = false};
  }
  if (argv[3] == "on") {
    ship.protect().evade = true;
  } else if (argv[3] == "off") {
    ship.protect().evade = false;
  }
  return OrderUpdate{};
}

std::expected<OrderUpdate, OrderError> order_bombard(const command_t& argv,
                                                     Ship& ship) {
  if (ship.type() == ShipType::OTYPE_OMCL) {
    return OrderUpdate{.modified = false};
  }
  if (!ship.can_bombard()) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::ShipTypeCannotRetaliate,
    });
  }
  if (argv.size() <= 3) {
    return OrderUpdate{.modified = false};
  }
  if (argv[3] == "off") {
    ship.bombard() = 0;
  } else if (argv[3] == "on") {
    ship.bombard() = 1;
  }
  return OrderUpdate{};
}

std::expected<OrderUpdate, OrderError> order_retaliate(const command_t& argv,
                                                       Ship& ship) {
  if (ship.type() == ShipType::OTYPE_OMCL) {
    return OrderUpdate{.modified = false};
  }
  if (!ship.can_bombard()) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::ShipTypeCannotRetaliate,
    });
  }
  if (argv.size() <= 3) {
    return OrderUpdate{.modified = false};
  }
  if (argv[3] == "off") {
    ship.protect().retaliate = false;
  } else if (argv[3] == "on") {
    ship.protect().retaliate = true;
  }
  return OrderUpdate{};
}

std::expected<OrderUpdate, OrderError> order_focus(const command_t& argv,
                                                   Ship& ship) {
  if (!ship.laser()) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::NoLaser,
    });
  }
  ship.focus() = (argv.size() > 3 && argv[3] == "on") ? 1 : 0;
  return OrderUpdate{};
}

std::expected<OrderUpdate, OrderError> order_laser(const command_t& argv,
                                                   Ship& ship) {
  if (!ship.laser()) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::NotEquippedWithCombatLasers,
    });
  }
  if (!ship.can_bombard()) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::ShipTypeCannotRetaliate,
    });
  }
  if (!ship.mounted()) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::NoCrystalMounted,
    });
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
  return OrderUpdate{};
}

std::expected<OrderUpdate, OrderError> order_merchant(const command_t& argv,
                                                      Ship& ship) {
  if (argv.size() <= 3) {
    return OrderUpdate{.modified = false};
  }
  if (argv[3] == "off") {
    ship.merchant() = 0;
    return OrderUpdate{};
  }
  auto res = scn::scan<int>(argv[3], "{}");
  if (!res || res->value() < 0 || res->value() > MAX_ROUTES) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::BadRouteNumber,
    });
  }
  ship.merchant() = res->value();
  return OrderUpdate{};
}

std::expected<OrderUpdate, OrderError> order_speed(const command_t& argv,
                                                   Ship& ship) {
  if (!ship.max_speed_capacity()) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::NoSpeedRating,
    });
  }
  if (argv.size() <= 3) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::InvalidSpeed,
    });
  }
  auto res = scn::scan<speed_t>(argv[3], "{}");
  if (!res) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::InvalidSpeed,
    });
  }
  ship.speed() = std::min(res->value(), ship.max_speed_capacity());
  return OrderUpdate{};
}

std::expected<OrderUpdate, OrderError> order_salvo(const command_t& argv,
                                                   Ship& ship) {
  if (!ship.can_bombard()) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::ShipCannotRetaliate,
    });
  }
  if (argv.size() <= 3) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::InvalidSalvoGunCount,
    });
  }
  auto res = scn::scan<gun_count_t>(argv[3], "{}");
  if (!res) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::InvalidSalvoGunCount,
    });
  }
  const auto* battery = ship.active_gun_battery();
  ship.retaliate() = battery ? std::min(res->value(), battery->count) : 0;
  return OrderUpdate{};
}

std::expected<OrderUpdate, OrderError>
order_battery(const command_t& argv, Ship& ship, ActiveBattery mode) {
  const auto& battery =
      (mode == PRIMARY) ? ship.primary_battery() : ship.secondary_battery();
  if (!battery.has_guns()) {
    return std::unexpected(OrderError{
        .reason = (mode == PRIMARY) ? OrderErrorReason::NoPrimaryGuns
                                    : OrderErrorReason::NoSecondaryGuns,
    });
  }
  if (argv.size() < 4) {
    ship.guns() = mode;
    ship.retaliate() = std::min(ship.retaliate(), battery.count);
    return OrderUpdate{};
  }
  auto res = scn::scan<gun_count_t>(argv[3], "{}");
  if (!res) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::InvalidBatteryGunCount,
    });
  }
  ship.retaliate() = std::min(res->value(), battery.count);
  ship.guns() = mode;
  return OrderUpdate{};
}

std::expected<OrderUpdate, OrderError> order_primary(const command_t& argv,
                                                     Ship& ship) {
  return order_battery(argv, ship, PRIMARY);
}

std::expected<OrderUpdate, OrderError> order_secondary(const command_t& argv,
                                                       Ship& ship) {
  return order_battery(argv, ship, SECONDARY);
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
  std::optional<shipnum_t> target{std::nullopt};
  if (argv.size() > 3) {
    auto res = scn::scan<shipnum_t::value_type>(argv[3], "{}");
    if (res && res->value() > 0) {
      target = shipnum_t{res->value()};
    }
  }
  if (target == ship.number()) {
    g.out << "A transporter cannot transport to itself.\n";
    target = std::nullopt;
  } else {
    g.out << std::format("Target ship is {}.\n", target.value_or(0));
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
  auto pl = Place::resolve(g.entity_manager, g.scope_context(), argv[3], true);
  if (!pl) {
    g.out << format_place_error(pl.error());
    g.out << "Error in destination.\n";
    return;
  }
  if (auto* mirror = ship.as<SpaceMirrorShip>()) {
    switch (pl->level) {
      case ScopeLevel::LEVEL_UNIV:
        mirror->clear_aim();
        break;
      case ScopeLevel::LEVEL_STAR:
        mirror->aim_at_star(pl->snum);
        break;
      case ScopeLevel::LEVEL_PLAN:
        mirror->aim_at_planet(pl->snum, pl->pnum);
        break;
      case ScopeLevel::LEVEL_SHIP:
        mirror->aim_at_ship(pl->shipno);
        break;
    }
  }
  if (requires_maneuver_fuel_to_aim(ship)) {
    ship.consume_fuel(FUEL_MANEUVER);
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
    mirror->set_intensity(std::clamp(val, 0, 100));
  }
}

bool activate_factory_on_habitat(GameObj& g, Ship& factory,
                                 resource_t& oncost) {
  if (!factory.destshipno()) {
    return false;
  }
  bool ok = false;
  g.entity_manager.mutate_ship(*factory.destshipno(), [&](Ship& habitat) {
    if (habitat.type() != ShipType::STYPE_HABITAT) {
      g.out << "The factory is currently being transported.\n";
      return;
    }
    oncost = HAB_FACT_ON_COST * factory.build_cost();
    if (habitat.resource() < oncost) {
      g.out << std::format(
          "You don't have {} resources on Habitat #{} to activate this "
          "factory.\n",
          oncost, *factory.destshipno());
      return;
    }
    const int new_size =
        1 + static_cast<int>(HAB_FACT_SIZE *
                             static_cast<double>(factory.calculate_size()));
    const int hanger_needed =
        new_size - ((habitat.max_hanger() - habitat.hanger()) + factory.size());
    if (hanger_needed > 0) {
      g.out << std::format(
          "Not enough hanger space free on Habitat #{}. Need {} more.\n",
          *factory.destshipno(), hanger_needed);
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
      factory.storbits(), factory.pnumorbits(), [&](Planet& planet) {
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

struct ShipOrderDispatchEntry {
  std::string_view name;
  std::expected<OrderUpdate, OrderError> (*handler)(const command_t&, Ship&);
};

constexpr std::array<ShipOrderDispatchEntry, 14> ship_order_handlers = {{
    {"defense", &order_defense},
    {"jump", &order_jump},
    {"protect", &order_protect},
    {"navigate", &order_navigate},
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
}};

struct LegacyOrderDispatchEntry {
  std::string_view name;
  void (*handler)(GameObj&, const command_t&, Ship&);
};

constexpr std::array<LegacyOrderDispatchEntry, 12> legacy_order_handlers = {{
    {"scatter", &order_scatter},
    {"impact", &order_impact},
    {"switch", &order_switch},
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

std::string format_ship_destination(EntityManager& em, const Ship& ship) {
  if (!ship.docked()) {
    return format_ship_dest(em, ship);
  }
  if (ship.whatdest() == ScopeLevel::LEVEL_SHIP) {
    return std::format("D#{}", ship.destshipno().value_or(0));
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
  if (ship.protect().retaliate) out += "/retal";
  out += format_active_battery_option(ship);
  if (ship.fire_laser()) out += std::format("/laser {}", ship.fire_laser());
  if (ship.focus()) out += "/focus";
  if (ship.retaliate()) out += std::format("/salvo {}", ship.retaliate());
  if (ship.protect().planet) out += "/defense";
  if (ship.protect().on && ship.protect().ship) {
    out += std::format("/prot {}", *ship.protect().ship);
  }
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
    out += std::format("/target {}", trans->target_ship().value_or(0));
  }

  if (const auto* mirror = ship.as<SpaceMirrorShip>()) {
    out += std::format("/aim {}/int {}", format_aim_target(em, *mirror),
                       mirror->intensity());
  }
  return out;
}

struct HyperdriveJumpInfo {
  bool active{false};
  double distance{0.0};
  double fuel_cost{0.0};
  bool insufficient_capacity{false};
};

HyperdriveJumpInfo compute_hyperdrive_jump_info(EntityManager& em,
                                                const Ship& ship) {
  if (!ship.hyper_drive().on || !ship.has_celestial_destination()) {
    return HyperdriveJumpInfo{};
  }
  const auto* dest_star = em.peek_star(ship.deststar());
  if (!dest_star) {
    return HyperdriveJumpInfo{};
  }
  const double dist = ship.coordinates().distance_to(dest_star->coordinates());
  const double distfac = HYPER_DIST_FACTOR * (ship.tech() + 100.0);
  const double ratio = dist / distfac;
  const double fuse =
      (ship.mounted() && dist > distfac)
          ? HYPER_DRIVE_FUEL_USE * std::sqrt(ship.mass()) * ratio
          : HYPER_DRIVE_FUEL_USE * std::sqrt(ship.mass()) * ratio * ratio;

  return HyperdriveJumpInfo{
      .active = true,
      .distance = dist,
      .fuel_cost = fuse,
      .insufficient_capacity = (ship.max_fuel_capacity() < fuse),
  };
}

}  // namespace

// TODO(jeffbailey): We take in a non-zero APcount, and do nothing with it!
std::expected<OrderUpdate, OrderError>
give_orders(GameObj& g, const command_t& argv, int /* APcount */, Ship& ship) {
  if (!ship.active()) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::ShipIrradiated,
        .ship_display = std::format("{}", ship),
        .radiation = ship.rad(),
    });
  }
  if (ship.type() != ShipType::OTYPE_TRANSDEV && !ship.popn() &&
      ship.max_crew_capacity()) {
    return std::unexpected(OrderError{
        .reason = OrderErrorReason::ShipHasNoCrew,
        .ship_display = std::format("{}", ship),
    });
  }

  std::expected<OrderUpdate, OrderError> result{OrderUpdate{.modified = false}};
  if (argv.size() > 2) {
    bool handled = false;
    if (argv[2] == "destination") {
      result =
          order_destination(g.entity_manager, g.scope_context(), argv, ship);
      handled = true;
    } else {
      for (const auto& entry : ship_order_handlers) {
        if (entry.name == argv[2]) {
          result = entry.handler(argv, ship);
          handled = true;
          break;
        }
      }
    }
    if (!handled) {
      for (const auto& entry : legacy_order_handlers) {
        if (entry.name == argv[2]) {
          entry.handler(g, argv, ship);
          result = OrderUpdate{.modified = true};
          break;
        }
      }
    }
  }
  ship.notified() = 0;
  return result;
}

ShipOrderStatus query_ship_order(EntityManager& em, const Ship& ship) {
  const char hyper_indicator =
      ship.hyper_drive().has ? (ship.mounted() ? '+' : '*') : ' ';
  const auto jump_info = compute_hyperdrive_jump_info(em, ship);

  return ShipOrderStatus{
      .ship_number = ship.number(),
      .type_letter = ship.type_letter(),
      .name = std::string(ship.name()),
      .hyper_indicator = hyper_indicator,
      .speed = ship.speed(),
      .orbits_display = dispshiploc_brief(em, ship),
      .destination_display = format_ship_destination(em, ship),
      .combat_options = format_combat_options(ship),
      .navigation_options = format_navigation_and_switch_options(ship),
      .specialty_options = format_specialty_options(em, ship),
      .has_hyperdrive_jump = jump_info.active,
      .jump_distance = jump_info.distance,
      .jump_fuel_cost = jump_info.fuel_cost,
      .insufficient_fuel_capacity = jump_info.insufficient_capacity,
  };
}
