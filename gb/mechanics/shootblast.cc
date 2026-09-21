// SPDX-License-Identifier: Apache-2.0

/// \file shootblast.cc
/// \brief Ship-to-ship, planet-to-ship, and orbital bombardment combat
/// resolution.

module;

import std;

module gb.mechanics;

struct SalvoHitRoll {
  hit_count_t hits{0};
  hit_odds_t probability{0};
};

struct CriticalHitResult {
  hit_count_t count{0};
  damage_t damage{0};
  std::string message;
};

static std::pair<damage_t, std::string> do_radiation(Ship& ship, double tech,
                                                     weapon_power_t strength,
                                                     hit_count_t hits);
static std::pair<damage_t, std::string>
do_damage(EntityManager& em, player_t who, Ship& ship, double tech,
          weapon_power_t strength, hit_count_t hits, armor_t defense,
          guntype_t caliber, double range, const std::string_view weapon,
          hit_odds_t hit_probability);

static std::tuple<bool, speed_t, ship_size_t>
ship_disposition(const Ship& ship);
static SalvoHitRoll roll_cew_hits(double dist, weapon_power_t cew_strength,
                                  weapon_range_t cew_range);
static SalvoHitRoll roll_salvo_hits(double dist, bool focus,
                                    weapon_power_t guns, double tech,
                                    damage_t fdam, bool fevade, bool tevade,
                                    speed_t fspeed, speed_t tspeed,
                                    ship_size_t tbody, guntype_t caliber,
                                    armor_t defense);
static hit_odds_t cew_hit_odds(double dist, weapon_range_t cew_range);
static CriticalHitResult do_critical_hits(hit_count_t penetrate, Ship& ship,
                                          guntype_t caliber);

std::optional<std::tuple<damage_t, std::string, std::string>>
shoot_ship_to_ship(EntityManager& em, const Ship& attacker, Ship& target,
                   const weapon_power_t cew_strength,
                   const weapon_range_t range, const bool ignore) {
  if (cew_strength <= 0) return std::nullopt;

  if (!(attacker.alive() || ignore) || !target.alive()) return std::nullopt;
  if (attacker.whatorbits() == ScopeLevel::LEVEL_SHIP ||
      target.whatorbits() == ScopeLevel::LEVEL_UNIV)
    return std::nullopt;
  if (target.whatorbits() == ScopeLevel::LEVEL_SHIP ||
      target.whatorbits() == ScopeLevel::LEVEL_UNIV)
    return std::nullopt;
  if (attacker.storbits() != target.storbits()) return std::nullopt;
  if (attacker.has_switch() && !attacker.on()) return std::nullopt;

  /* compute caliber */
  const auto caliber = attacker.current_caliber();

  double dist = [&attacker, &target]() -> double {
    if (attacker.type() ==
        ShipType::STYPE_MISSILE) /* missiles hit at point blank range */
      return 0.0;

    double dist = attacker.coordinates().distance_to(target.coordinates());
    if (attacker.type() ==
        ShipType::STYPE_MINE) { /* compute the effective range */
      dist *= dist / 200.0;     /* mines are very effective inside 200 */
    }
    return dist;
  }();

  if (dist > attacker.gun_range()) return std::nullopt;

  /* attack parameters */
  auto [fevade, fspeed, fbody] = ship_disposition(attacker);
  auto [tevade, tspeed, tbody] = ship_disposition(target);
  auto defense = getdefense(em, target);

  bool focus = attacker.is_laser_on() && attacker.focus();

  const auto [hits, hit_probability] =
      (range != 0) ? roll_cew_hits(dist, cew_strength, attacker.cew_range())
                   : roll_salvo_hits(dist, focus, cew_strength, attacker.tech(),
                                     attacker.damage(), fevade, tevade, fspeed,
                                     tspeed, tbody, caliber, defense);

  // mode is whether a ship has been set to radiative with the orders command.
  if (attacker.mode()) {
    auto [damage, damage_msg] =
        do_radiation(target, attacker.tech(), cew_strength, hits);
    std::string short_msg =
        std::format("{}: {} {} {}\n", dispshiploc(em, target), attacker,
                    target.alive() ? "attacked" : "DESTROYED", target);
    std::string long_msg = short_msg;
    long_msg += damage_msg;
    return std::make_tuple(damage, short_msg, long_msg);
  }

  // CEW, destruct, lasers
  auto weapon = [range, &attacker, caliber] -> std::string {
    if (range != 0) return "strength CEW";

    if (attacker.is_laser_on()) {
      if (attacker.focus()) return "strength focused laser";
      return "strength laser";
    }

    switch (caliber) {
      case guntype_t::LIGHT:
        return "light guns";
      case guntype_t::MEDIUM:
        return "medium guns";
      case guntype_t::HEAVY:
        return "heavy guns";
      case guntype_t::NONE:
        return "pea-shooter";
    }
  }();

  if (caliber == guntype_t::NONE) return std::nullopt;

  auto [damage, damage_msg] = do_damage(
      em, attacker.owner(), target, (double)attacker.tech(), cew_strength, hits,
      defense, caliber, dist, weapon, hit_probability);
  std::string short_msg =
      std::format("{}: {} {} {}\n", dispshiploc(em, target), attacker,
                  target.alive() ? "attacked" : "DESTROYED", target);
  std::string long_msg = short_msg;
  long_msg += damage_msg;
  return std::make_tuple(damage, short_msg, long_msg);
}

std::optional<std::tuple<damage_t, std::string, std::string>>
shoot_planet_to_ship(EntityManager& em, Race& race, Ship& ship,
                     weapon_power_t strength) {
  if (strength <= 0) return std::nullopt;
  if (!ship.alive()) return std::nullopt;
  if (ship.whatorbits() != ScopeLevel::LEVEL_PLAN) return std::nullopt;

  auto [evade, speed, body] = ship_disposition(ship);

  const auto [hits, hit_probability] =
      roll_salvo_hits(0.0, false, strength, race.tech, 0, evade, false, speed,
                      0, body, guntype_t::MEDIUM, 1);

  auto [damage, damage_msg] =
      do_damage(em, race.Playernum, ship, race.tech, strength, hits, 0,
                guntype_t::MEDIUM, 0.0, "medium guns", hit_probability);

  std::string short_msg = std::format(
      "{} [{}] {} {}\n", dispshiploc(em, ship), race.Playernum.value,
      ship.alive() ? "attacked" : "DESTROYED", ship);
  std::string long_msg = short_msg + damage_msg;

  return std::make_tuple(damage, short_msg, long_msg);
}

/**
 * @return Result containing number of sectors destroyed and which players were
 * hit.
 */
std::optional<BombardResult>
shoot_ship_to_planet(EntityManager& em, const Ship& ship, Planet& pl,
                     weapon_power_t strength, Coordinates target_sector,
                     SectorMap& smap, bool ignore, guntype_t caliber) {
  if (strength <= 0) return std::nullopt;
  if (!(ship.alive() || ignore)) return std::nullopt;
  if (ship.has_switch() && !ship.on()) return std::nullopt;
  if (ship.whatorbits() != ScopeLevel::LEVEL_PLAN) return std::nullopt;
  if (!pl.is_valid(target_sector)) return std::nullopt;

  sector_count_t numdest{0};
  PlayerVector<bool, MAXPLAYERS> nuked{};

  double r = .4 * strength;
  if (caliber == guntype_t::NONE) {
    /* figure out the appropriate gun caliber if not given*/
    if (ship.fire_laser()) {
      caliber = guntype_t::LIGHT;
    } else {
      const auto* battery = ship.active_gun_battery();
      caliber = battery ? battery->caliber : guntype_t::LIGHT;
    }
  }

  auto& target = smap.get(target_sector);
  player_t oldowner = target.get_owner();

  PlayerVector<int, MAXPLAYERS> sum_mob{};

  for (auto y2 = 0; y2 < pl.dimensions().y; y2++) {
    for (auto x2 = 0; x2 < pl.dimensions().x; x2++) {
      int dx =
          std::min(std::abs(x2 - target_sector.x),
                   std::abs(target_sector.x + (pl.dimensions().x - 1) - x2));
      int dy = std::abs(y2 - target_sector.y);
      double d = std::sqrt((double)(dx * dx + dy * dy));
      auto& s = smap.get(Coordinates{x2, y2});

      if (d <= r) {
        double fac = SECTOR_DAMAGE * (double)strength *
                     (double)gun_caliber(caliber) / (d + 1.);

        if (s.get_owner() != 0) {
          population_t kills = 0;
          if (s.get_popn()) {
            kills = int_rand(0, ((int)(fac / 10.0) * s.get_popn())) /
                    (1 + s.is_plated());
            if (kills > s.get_popn())
              s.clear_popn();
            else
              s.subtract_popn(kills);
          }
          // Entrenched troops are sheltered unless blast intensity exceeds
          // TROOP_BOMBARD_FORTIFICATION_SCALE times the sector defense bonus.
          if (s.get_troops() &&
              (fac > TROOP_BOMBARD_FORTIFICATION_SCALE *
                         static_cast<double>(s.defense_bonus()))) {
            kills = int_rand(0, ((int)(fac / 20.0) * s.get_troops())) /
                    (1 + s.is_plated());
            if (kills > s.get_troops())
              s.set_troops(0);
            else
              s.set_troops(s.get_troops() - kills);
          }

          s.clear_owner_if_empty();
        }

        // High-intensity blasts can strip surface terraforming back to the
        // sector's underlying geological type.
        if (fac >= TERRAFORM_STRIP_BLAST_THRESHOLD && !int_rand(0, 10)) {
          if (int_rand(0, 6) >= s.defense_bonus())
            s.set_condition(s.get_type());
        }

        if (round_rand(fac) > s.defense_bonus() * int_rand(0, 10)) {
          if (s.get_owner() != 0) nuked[s.get_owner()] = true;
          s.clear_popn();
          s.set_troops(int_rand(0, (int)s.get_troops()));
          if (!s.get_troops()) /* troops may survive this */
            s.set_owner(0);
          s.clear_efficiency();
          s.set_resource(s.get_resource() / ((int)fac + 1));
          s.set_mobilization(0);
          s.set_fert(0); /*all is lost !*/
          s.set_crystals(int_rand(0, (int)s.get_crystals()));
          s.set_condition(SectorType::SEC_WASTED);
          numdest++;
        } else {
          s.set_fert(std::max(0, (int)s.get_fert() - (int)fac));
          s.degrade_efficiency(std::lround(fac));
          s.set_mobilization(std::max(0, (int)s.get_mobilization() - (int)fac));
          s.deplete_resource(std::lround(fac));
        }
      }
      if (s.get_owner() != 0) sum_mob[s.get_owner()] += s.get_mobilization();
    }
  }
  auto num_sectors = pl.num_sectors();
  for (const Race& race : RaceList::readonly(em)) {
    player_t i = race.Playernum;
    pl.info(i).mob_points = sum_mob[i];
    pl.info(i).comread = sum_mob[i] / num_sectors;
    pl.info(i).guns = planet_guns(sum_mob[i]);
  }

  /* planet toxicity goes up a bit */
  pl.toxic() += static_cast<int>((100 - pl.toxic()) *
                                 ((double)numdest / (double)num_sectors));

  std::string short_msg = std::format("{} bombards {} [{}]\n", ship,
                                      dispshiploc(em, ship), oldowner);
  std::string long_msg =
      short_msg + std::format("\t{} sectors destroyed\n", numdest);
  return BombardResult{
      .sectors_destroyed = numdest,
      .nuked_players = nuked,
      .short_message = std::move(short_msg),
      .long_message = std::move(long_msg),
  };
}

static std::pair<damage_t, std::string> do_radiation(Ship& ship, double tech,
                                                     weapon_power_t strength,
                                                     hit_count_t hits) {
  std::stringstream msg;
  const double fac = p_factor(tech, ship.tech());

  const auto armor_reduction =
      static_cast<armor_t>(hits / HITS_PER_ARMOR_PENETRATION);
  const armor_t arm = ship.effective_armor() > armor_reduction
                          ? ship.effective_armor() - armor_reduction
                          : 0;
  const auto body = std::max<ship_size_t>(1, ship.shipbody());

  hit_count_t penetrate = 0;
  const double r = std::pow(fac, static_cast<double>(arm));

  /* check to see how many hits penetrate */
  for (auto _ : std::views::iota(hit_count_t{0}, hits)) {
    if (double_rand() <= r) penetrate += 1;
  }

  auto dosage = static_cast<radiation_t>(std::max(
      0, round_rand(40. * static_cast<double>(penetrate) / (double)body)));
  dosage = std::min<radiation_t>(100, dosage);

  ship.apply_radiation(dosage);
  if (success(ship.rad())) ship.active() = false;

  // Radiation does not kill crew immediately upon impact; instead, irradiated
  // ships suffer 20% crew/troop attrition per turn update in
  // process_ship_radiation() (help/ships.md).
  msg << std::format("\tAttack: {} radiation\n\t  Hits: {}\n", strength, hits);
  msg << std::format("\t   Rad: {}% for a total of {}%\n", dosage, ship.rad());
  return {static_cast<damage_t>(dosage), msg.str()};
}

static std::pair<damage_t, std::string>
do_damage(EntityManager& em, player_t who, Ship& ship, double tech,
          weapon_power_t strength, hit_count_t hits, armor_t defense,
          guntype_t caliber, double range, const std::string_view weapon,
          hit_odds_t hit_probability) {
  std::stringstream msg;

  msg << std::format("\tAttack: {} {} at a range of {:.0f}\n", strength, weapon,
                     range);
  msg << std::format("\t  Hits: {}  {}% probability\n", hits, hit_probability);
  /* ship may lose some armor */
  if (ship.armor())
    if (success(hits * gun_caliber(caliber))) {
      ship.armor()--;
      msg << std::format("\t\tArmor reduced to {}\n", ship.armor());
    }

  const double fac = p_factor(tech, ship.tech());
  const armor_t total_defense = ship.effective_armor() + defense;
  const armor_t saturation = hits / HITS_PER_ARMOR_PENETRATION;
  const armor_t arm =
      total_defense > saturation ? total_defense - saturation : 0;
  const auto body_size = std::max<ship_size_t>(1, ship.shipbody());
  const double body = std::sqrt(0.1 * static_cast<double>(body_size));

  hit_count_t penetrate = 0;
  const double r = std::pow(fac, static_cast<double>(arm));

  /* check to see how many hits penetrate */
  for (auto _ : std::views::iota(hit_count_t{0}, hits)) {
    if (double_rand() <= r) penetrate += 1;
  }

  auto damage = static_cast<damage_t>(std::max(
      0, round_rand(SHIP_DAMAGE * static_cast<double>(gun_caliber(caliber)) *
                    static_cast<double>(penetrate) / body)));

  const auto crit = do_critical_hits(penetrate, ship, caliber);

  if (crit.count > 0) damage += crit.damage;

  damage = std::min<damage_t>(100, damage);
  const auto damage_result = ship.apply_damage(damage);

  double race_mass = 1.0;
  try {
    const auto& r = *em.peek_race(ship.owner());
    race_mass = r.mass;
  } catch (const EntityNotFoundError&) {
    race_mass = 1.0;
  }
  auto [casualties, casualties1, primgundamage, secgundamage] =
      do_collateral(ship, damage, race_mass);
  /* set laser strength for ships to maximum safe limit */
  if (ship.fire_laser()) {
    const auto safe = static_cast<weapon_power_t>(
        std::max(0.0, (1.0 - .01 * ship.damage()) * ship.tech() / 4.0));
    if (ship.fire_laser() > safe) ship.fire_laser() = safe;
  }

  if (penetrate) {
    msg << std::format(
        "\t\t{} penetrations  eff armor={} defense={} prob={:.3f}\n", penetrate,
        arm, defense, r);
  }
  if (crit.count > 0) {
    msg << std::format("\t\t{} CRITICAL hits do {}% damage\n", crit.count,
                       crit.damage);
    msg << crit.message;
  }
  if (damage) {
    msg << std::format("\tDamage: {}% damage for a total of {}%\n", damage,
                       ship.damage());
  }
  if (primgundamage || secgundamage) {
    msg << std::format("\t Other: {} primary/{} secondary guns destroyed\n",
                       primgundamage, secgundamage);
  }
  if (casualties || casualties1) {
    msg << std::format("\tKilled: {} civ + {} mil casualties\n", casualties,
                       casualties1);
  }

  if (damage_result.destroyed) em.kill_ship(who, ship);
  ship.build_cost() = cost(ship);
  return {damage, msg.str()};
}

/**
 * @brief Determines the disposition of a ship.
 *
 * This function calculates the values of evade, speed, and body based on the
 * given ship.
 *
 * @param ship The ship for which the disposition is being determined.
 * @return A tuple containing the evade value, speed, and body size of the ship.
 */
static std::tuple<bool, speed_t, ship_size_t>
ship_disposition(const Ship& ship) {
  bool evade = false;
  speed_t speed = 0;
  ship_size_t body = ship.size();
  if (ship.active() && !ship.docked() &&
      (ship.whatdest() || ship.navigate().on)) {
    evade = ship.protect().evade;
    speed = ship.speed();
  }
  return {evade, speed, body};
}

static SalvoHitRoll roll_cew_hits(double dist, weapon_power_t cew_strength,
                                  weapon_range_t cew_range) {
  const hit_odds_t prob = cew_hit_odds(dist, cew_range);
  return {
      .hits = success(prob) ? cew_strength : 0u,
      .probability = prob,
  };
}

static SalvoHitRoll roll_salvo_hits(double dist, bool focus,
                                    weapon_power_t guns, double tech,
                                    damage_t fdam, bool fev, bool tev,
                                    speed_t fspeed, speed_t tspeed,
                                    ship_size_t body, guntype_t caliber,
                                    armor_t defense) {
  auto [prob, factor] = hit_odds(dist, tech, fdam, fev, tev, fspeed, tspeed,
                                 body, caliber, defense);

  hit_count_t hits = 0;
  hit_odds_t hit_probability = 0;
  if (focus) {
    hit_probability = (prob * prob) / 100;
    if (success(hit_probability)) hits = guns;
  } else {
    for (auto _ : std::views::iota(weapon_power_t{0}, guns)) {
      if (success(prob)) hits++;
    }
    hit_probability = prob;
  }

  return {
      .hits = hits,
      .probability = hit_probability,
  };
}

/**
 * @brief Calculates the odds of hitting a target and a range factor based on
 * combat parameters.
 *
 * This function computes the probability (odds) of a successful hit and a range
 * factor for a shot, given various parameters such as weapon technology,
 * damage, speeds, body size, gun caliber, and target defense.
 *
 * @param range     The distance to the target.
 * @param tech      The technology level of the shooter.
 * @param fdam      The damage factor of the firing entity (percentage, 0-100).
 * @param fev       The evasion state of the firing entity.
 * @param tev       The evasion state of the target entity.
 * @param fspeed    The speed of the firing entity.
 * @param tspeed    The speed of the target entity.
 * @param body      The body size of the target.
 * @param caliber   The caliber of the gun (guntype_t).
 * @param defense   The defense factor of the target (percentage, 0-100).
 * @return std::pair<hit_odds_t, weapon_range_t> A pair where the first element
 * is the hit odds (percentage), and the second element is the computed range
 * factor.
 */
std::pair<hit_odds_t, weapon_range_t>
hit_odds(double range, double tech, damage_t fdam, bool fev, bool tev,
         speed_t fspeed, speed_t tspeed, ship_size_t body, guntype_t caliber,
         armor_t defense) {
  if (caliber == guntype_t::NONE) {
    return {0, 0};
  }

  double fev_d = fev ? 1.0 : 0.0;
  double tev_d = tev ? 1.0 : 0.0;
  double a =
      std::log10(1.0 + (double)tech) * 80.0 * std::pow((double)body, 0.33333);
  double b = 72.0 / ((2.0 + tev_d) * (2.0 + fev_d) *
                     (18.0 + (double)tspeed + (double)fspeed));
  double c = a * b / static_cast<double>(gun_caliber(caliber));
  const auto factor = static_cast<weapon_range_t>(std::max(
      0.0, c * (1.0 - static_cast<double>(fdam) / 100.))); /* 50% hit range */
  hit_odds_t odds = 0;
  if (factor > 0) {
    const double raw_odds =
        (static_cast<double>(factor) * 100.0) /
        (static_cast<double>(factor) + std::max(0.0, range));
    const double mitigated =
        raw_odds * std::max(0.0, 1.0 - 0.1 * static_cast<double>(defense));
    odds = static_cast<hit_odds_t>(mitigated);
  }
  return {odds, factor};
}

static hit_odds_t cew_hit_odds(double range, weapon_range_t cew_range) {
  double factor =
      (range + 1.0) / ((double)cew_range + 1.0); /* maximum chance */
  const auto odds = static_cast<hit_odds_t>(
      100.0 * std::exp((double)(-50.0 * (factor - 1.0) * (factor - 1.0))));
  return odds;
}

static CriticalHitResult do_critical_hits(hit_count_t penetrate, Ship& ship,
                                          guntype_t caliber) {
  std::stringstream critmsg;
  hit_count_t crithits = 0;
  damage_t critdam = 0;
  const unsigned int caliber_val = std::max(1u, gun_caliber(caliber));
  const auto eff_size = std::max<ship_size_t>(1, ship.shipbody() / caliber_val);
  for (auto _ : std::views::iota(hit_count_t{0}, penetrate)) {
    if (!int_rand(0, static_cast<int>(eff_size) - 1)) {
      crithits += 1;
      const auto dam = static_cast<damage_t>(int_rand(0, 100));
      critdam += dam;
    }
  }
  critdam = std::min<damage_t>(100, critdam);
  /* check for special systems damage */
  critmsg << "\t\tSpecial systems damage: ";
  if (ship.cew() && success(critdam)) {
    critmsg << "CEW ";
    ship.cew() = 0;
  }
  if (ship.laser() && success(critdam)) {
    critmsg << "Laser ";
    ship.laser() = 0;
  }
  if (ship.cloak() && success(critdam)) {
    critmsg << "Cloak ";
    ship.cloak() = 0;
  }
  if (ship.hyper_drive().has && success(critdam)) {
    critmsg << "Hyper-drive ";
    ship.hyper_drive().has = 0;
  }
  if (ship.max_speed() && success(critdam)) {
    ship.speed() = 0;
    ship.max_speed() = int_rand(0, ship.max_speed() - 1);
    critmsg << std::format("Speed={} ", ship.max_speed());
  }
  if (ship.armor() && success(critdam)) {
    ship.armor() = int_rand(0, ship.armor() - 1);
    critmsg << std::format("Armor={} ", ship.armor());
  }
  critmsg << "\n";
  return {
      .count = crithits,
      .damage = critdam,
      .message = critmsg.str(),
  };
}

CollateralDamage do_collateral(Ship& ship, damage_t damage, double race_mass) {
  /* compute crew/troop casualties */
  population_t casualties = 0;
  population_t casualties1 = 0;
  gun_count_t primgundamage = 0;
  gun_count_t secgundamage = 0;

  for (auto _ : std::views::iota(population_t{0}, ship.popn())) {
    casualties += success(damage);
  }
  for (auto _ : std::views::iota(population_t{0}, ship.troops())) {
    casualties1 += success(damage);
  }
  auto applied = ship.apply_casualties(casualties, casualties1, race_mass);
  for (auto _ :
       std::views::iota(gun_count_t{0}, ship.primary_battery().count)) {
    primgundamage += success(damage);
  }
  const auto prim_lost = ship.damage_primary_guns(primgundamage);
  for (auto _ :
       std::views::iota(gun_count_t{0}, ship.secondary_battery().count)) {
    secgundamage += success(damage);
  }
  const auto sec_lost = ship.damage_secondary_guns(secgundamage);
  return {
      .civilian_casualties = applied.crew,
      .military_casualties = applied.troops,
      .primary_guns_lost = prim_lost,
      .secondary_guns_lost = sec_lost,
  };
}

double p_factor(double attacker_tech, double defender_tech) {
  return (2.0 * std::numbers::inv_pi) *
         std::atan(TECH_PENETRATION_SCALE *
                   ((attacker_tech + 1.0) / (defender_tech + 1.0)));
}

bool check_mine_proximity_trigger(const Ship& mine,
                                  EntityManager& entity_manager) {
  if (mine.type() != ShipType::STYPE_MINE || !mine.alive() ||
      mine.owner() == 0 || !mine.on()) {
    return false;
  }
  if (mine.whatorbits() != ScopeLevel::LEVEL_STAR &&
      mine.whatorbits() != ScopeLevel::LEVEL_PLAN) {
    return false;
  }

  const auto* mine_data = mine.as<MineShip>();
  if (!mine_data) {
    return false;
  }

  const auto& race = *entity_manager.peek_race(mine.owner());
  const ShipList scoped_ships =
      (mine.whatorbits() == ScopeLevel::LEVEL_STAR)
          ? ShipList::readonly_in_star(entity_manager, mine.storbits())
          : ShipList::readonly_on_planet(entity_manager, mine.storbits(),
                                         mine.pnumorbits());

  for (const auto& s : scoped_ships) {
    if (s.number() == mine.number() || !s.alive()) {
      continue;
    }
    if (s.owner() == mine.owner() || race.is_allied_with(s.owner())) {
      continue;
    }
    double range = mine.coordinates().distance_to(s.coordinates());
    if (range <= static_cast<double>(mine_data->trigger_radius())) {
      return true;
    }
  }
  return false;
}

void detonate_mine_against_ships(Ship& mine, EntityManager& entity_manager) {
  if (mine.whatorbits() != ScopeLevel::LEVEL_STAR &&
      mine.whatorbits() != ScopeLevel::LEVEL_PLAN) {
    return;
  }

  std::vector<shipnum_t> victims;
  const ShipList scoped_ships =
      (mine.whatorbits() == ScopeLevel::LEVEL_STAR)
          ? ShipList::readonly_in_star(entity_manager, mine.storbits())
          : ShipList::readonly_on_planet(entity_manager, mine.storbits(),
                                         mine.pnumorbits());

  for (const auto& s : scoped_ships) {
    if (s.number() != mine.number() && s.alive() &&
        s.type() != ShipType::OTYPE_CANIST &&
        s.type() != ShipType::OTYPE_GREEN) {
      victims.push_back(s.number());
    }
  }

  for (shipnum_t victim_num : victims) {
    entity_manager.mutate_ship(victim_num, [&](Ship& s) {
      if (!s.alive()) return;
      auto s2sresult = shoot_ship_to_ship(entity_manager, mine, s,
                                          mine.destruct_power(), 0, false);
      if (s2sresult) {
        auto const& [damage, short_buf, long_buf] = *s2sresult;
        post(entity_manager, short_buf, NewsType::COMBAT);
        push_telegram(entity_manager, s.owner(), s.governor(), long_buf);
      }
    });
  }
}

void detonate_mine_against_planet(Ship& mine, const std::string& postmsg,
                                  EntityManager& entity_manager) {
  if (mine.whatorbits() != ScopeLevel::LEVEL_PLAN) {
    return;
  }

  /* pick a random sector to nuke */
  entity_manager.mutate_planet(
      mine.storbits(), mine.pnumorbits(), [&](Planet& planet) {
        entity_manager.mutate_sectormap(
            mine.storbits(), mine.pnumorbits(), [&](SectorMap& smap) {
              const Coordinates target_coords =
                  mine.is_landed() ? mine.land_coords()
                                   : smap.get_random().coords();

              if (auto result_opt = shoot_ship_to_planet(
                      entity_manager, mine, planet, mine.destruct_power(),
                      target_coords, smap, false, guntype_t::LIGHT)) {
                std::stringstream telegram;
                telegram << postmsg;
                if (result_opt->sectors_destroyed > 0) {
                  telegram << std::format(" - {} sectors destroyed.",
                                          result_opt->sectors_destroyed);
                }
                telegram << "\n";

                const auto& star = *entity_manager.peek_star(mine.storbits());
                for (const Race& race : RaceList::readonly(entity_manager)) {
                  if (result_opt->nuked_players[race.Playernum]) {
                    push_telegram(entity_manager, race.Playernum,
                                  star.governor(race.Playernum),
                                  telegram.str());
                  }
                }
                push_telegram(entity_manager, mine.owner(), mine.governor(),
                              telegram.str());
              }
            });
      });
}

void domine(Ship& ship, bool detonate, EntityManager& entity_manager) {
  if (ship.type() != ShipType::STYPE_MINE || !ship.alive() ||
      ship.owner() == 0) {
    return;
  }

  /* check around and see if we should explode. */
  if (!ship.on() && !detonate) {
    return;
  }

  if (ship.whatorbits() == ScopeLevel::LEVEL_UNIV ||
      ship.whatorbits() == ScopeLevel::LEVEL_SHIP) {
    return;
  }

  if (!detonate && !check_mine_proximity_trigger(ship, entity_manager)) {
    return;
  }

  std::string postmsg = std::format("{} detonated at {}\n", ship,
                                    prin_ship_orbits(entity_manager, ship));
  post(entity_manager, postmsg, NewsType::COMBAT);
  telegram_star(entity_manager, ship.storbits(), ship.owner(), ship.governor(),
                postmsg);

  detonate_mine_against_ships(ship, entity_manager);
  detonate_mine_against_planet(ship, postmsg, entity_manager);

  entity_manager.kill_ship(ship.owner(), ship);
}
