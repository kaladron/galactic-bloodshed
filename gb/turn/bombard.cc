// SPDX-License-Identifier: Apache-2.0

/// \file bombard.cc
/// \brief Simulates autonomous berserker planetary bombardment.

module;

import std;

module gblib;

bool check_orbital_pdn_defense(EntityManager& entity_manager,
                               const Planet& planet, player_t attacker) {
  for (const auto& s : ShipList::readonly(entity_manager, planet.ships())) {
    if (s.alive() && s.type() == ShipType::OTYPE_PLANDEF &&
        s.owner() != attacker) {
      return true;
    }
  }
  return false;
}

std::optional<Coordinates>
find_bombardment_target(EntityManager& entity_manager, const Ship& ship,
                        const Race& attacker_race) {
  std::optional<Coordinates> target;

  entity_manager.with_sectormap(
      ship.storbits(), ship.pnumorbits(), [&](const SectorMap& smap) {
        const auto* bers = ship.as<BerserkerShip>();
        const std::optional<player_t> programmed_target =
            bers && bers->target() != 0 ? std::optional{bers->target()}
                                        : std::nullopt;

        auto candidates =
            smap.shuffle() | std::views::filter([&](const Sector& s) noexcept {
              return s.is_bombardable_by(ship.owner());
            });

        // 1. Look for an active enemy colony or programmed target first
        for (const Sector& sect : candidates) {
          const player_t owner = sect.get_owner();
          if (attacker_race.is_at_war_with(owner) ||
              programmed_target == owner) {
            target = sect.coords();
            return;
          }
        }

        // 2. If no enemy colonies exist, fall back to any foreign colony
        for (const Sector& sect : candidates) {
          target = sect.coords();
          return;
        }
      });

  return target;
}

int calculate_bombardment_strength(const Ship& ship) {
  const double effective_guns =
      static_cast<double>(ship.max_guns_capacity()) * ship.hull_efficiency();
  return std::max(0, std::min(static_cast<int>(effective_guns),
                              static_cast<int>(ship.destruct())));
}

void dispatch_bombardment_alerts(EntityManager& entity_manager,
                                 const Ship& ship, const Star& star,
                                 Coordinates target, player_t old_owner,
                                 int sectors_destroyed,
                                 const BombardResult& result) {
  /* tell the bombarding player about it.. */
  std::stringstream telegram_report;
  telegram_report << std::format("REPORT from ship #{}\n\n", ship.number());
  telegram_report << result.short_message;
  telegram_report << std::format(
      "sector {} (owner {}). {} sectors destroyed.\n", target, old_owner,
      sectors_destroyed);
  push_telegram(entity_manager, ship.owner(), ship.governor(),
                telegram_report.str());

  /* notify other player. */
  std::stringstream telegram_alert;
  telegram_alert << std::format("ALERT from planet /{}/{}\n", star.get_name(),
                                star.get_planet_name(ship.pnumorbits()));
  telegram_alert << std::format(
      "{}{} {} bombarded sector {}; {} sectors destroyed.\n",
      ship.type_letter(), ship.number(), ship.name(), target,
      sectors_destroyed);

  for (const Race& race : RaceList::readonly(entity_manager)) {
    player_t i = race.Playernum;
    if (result.nuked_players[i] && i != ship.owner()) {
      push_telegram(entity_manager, i, star.governor(i), telegram_alert.str());
    }
  }

  std::string combatpost =
      std::format("{}{} {} [{}] bombards {}/{}\n", ship.type_letter(),
                  ship.number(), ship.name(), ship.owner(), star.get_name(),
                  star.get_planet_name(ship.pnumorbits()));
  post(entity_manager, combatpost, NewsType::COMBAT);
}

/**
 * Performs a bombardment action by a berserker ship on a planet.
 *
 * This function checks if there are any Point Defense Networks (PDNs) present
 * on the planet. If PDNs are present, the bombardment is cancelled and a
 * warning message is sent to the ship's owner. Otherwise, the function searches
 * for a sector to bombard. It looks for sectors owned by other races that are
 * at war with the ship's race or are the target of the berserker ship. If no
 * suitable sector is found, a notification is sent to the ship's owner
 * indicating that there are no sectors worth bombing.
 *
 * If a suitable sector is found, the function calculates the strength of the
 * bombardment based on the ship's guns and damage. It then proceeds to destroy
 * sectors on the planet using the shoot_ship_to_planet function. The number of
 * destroyed sectors is returned. The ship's owner is notified of the
 * bombardment results, and an alert is sent to the other players. If the ship
 * has no weapons, a notification is sent to the ship's owner indicating the
 * lack of weapons.
 *
 * \param entity_manager Entity manager for spatial queries, updates, and
 * messaging.
 * \param ship The berserker ship performing the bombardment.
 * \param planet The planet being bombarded.
 * \param r The race to which the ship belongs.
 * \return The number of sectors destroyed during the bombardment.
 */
int berserker_bombard(EntityManager& entity_manager, Ship& ship, Planet& planet,
                      const Race& r) {
  // Get star for telegrams - lookup once for efficiency
  const auto& star = *entity_manager.peek_star(ship.storbits());

  /* check to see if PDNs are present */
  if (check_orbital_pdn_defense(entity_manager, planet, ship.owner())) {
    std::string notice =
        std::format("Bombardment of {} cancelled, PDNs are present.\n",
                    prin_ship_orbits(entity_manager, ship));
    push_telegram(entity_manager, ship.owner(), ship.governor(), notice);
    return 0;
  }

  /* look for someone to bombard-check for war */
  const auto target = find_bombardment_target(entity_manager, ship, r);
  if (!target.has_value()) {
    /* there were no sectors worth bombing. */
    if (!ship.notified()) {
      ship.notified() = 1;
      std::stringstream telegram;
      telegram << std::format("Report from {}{} {}\n\n", ship.type_letter(),
                              ship.number(), ship.name());
      telegram << std::format("Planet /{}/{} has been saturation bombed.\n",
                              star.get_name(),
                              star.get_planet_name(ship.pnumorbits()));
      push_telegram(entity_manager, ship.owner(), ship.governor(),
                    telegram.str());
    }
    return 0;
  }

  const int str = calculate_bombardment_strength(ship);
  if (str <= 0) {
    /* no weapons! */
    if (!ship.notified()) {
      ship.notified() = 1;
      std::string telegram =
          std::format("Bulletin\n\n {}{} {} has no weapons to bombard with.\n",
                      ship.type_letter(), ship.number(), ship.name());
      push_telegram(entity_manager, ship.owner(), ship.governor(), telegram);
    }
    return 0;
  }

  // Enemy planet retaliates along with defending forces

  // save owner of destroyed sector
  player_t oldown = 0;
  entity_manager.with_sectormap(
      ship.storbits(), ship.pnumorbits(),
      [&](const SectorMap& smap) { oldown = smap.get(*target).get_owner(); });
  ship.consume_destruct(str);

  std::optional<BombardResult> opt_result;
  entity_manager.mutate_sectormap(
      ship.storbits(), ship.pnumorbits(), [&](SectorMap& smap) {
        opt_result = shoot_ship_to_planet(entity_manager, ship, planet, str,
                                          *target, smap, 0, guntype_t::NONE);
      });
  if (!opt_result) return 0;
  const auto& result = *opt_result;
  /* (0=dont get smap) */
  const auto numdest = std::max(result.sectors_destroyed, 0);

  dispatch_bombardment_alerts(entity_manager, ship, star, *target, oldown,
                              numdest, result);

  return numdest;
}
