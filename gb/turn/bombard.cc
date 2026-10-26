// SPDX-License-Identifier: Apache-2.0

/// \file bombard.cc
/// \brief Simulates autonomous berserker planetary bombardment.

module;

import gb.presentation.combat;
import std;

module gb.turn;

bool check_orbital_pdn_defense(EntityManager& entity_manager,
                               const Planet& planet, player_t attacker) {
  for (const auto& s : ShipList::readonly_on_planet(
           entity_manager, planet.star_id(), planet.planet_order())) {
    if (s.type() == ShipType::OTYPE_PLANDEF && s.owner() != attacker) {
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
            bers ? bers->target() : std::nullopt;

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

void dispatch_bombardment_alerts(EntityManager& entity_manager,
                                 const Ship& ship, const Star& star,
                                 Coordinates target, player_t old_owner,
                                 int sectors_destroyed,
                                 const BombardResult& result) {
  const GB::presentation::BerserkerBombardNotice notice{
      .ship_number = ship.number(),
      .ship_letter = ship.type_letter(),
      .ship_name = ship.name(),
      .attacker = ship.owner(),
      .star_name = star.get_name(),
      .planet_name = star.get_planet_name(ship.pnumorbits()),
      .target = target,
      .old_owner = old_owner,
      .sectors_destroyed = sectors_destroyed,
  };

  /* tell the bombarding player about it.. */
  push_telegram(
      entity_manager, ship.owner(), ship.governor(),
      GB::presentation::render_berserker_bombard_owner_report(notice, result));

  /* notify other player. */
  const std::string telegram_alert =
      GB::presentation::render_berserker_bombard_victim_alert(notice);
  for (player_t i : result.nuked_players) {
    if (i != ship.owner()) {
      push_telegram(entity_manager, i, star.governor(i), telegram_alert);
    }
  }

  post(entity_manager, GB::presentation::render_berserker_bombard_news(notice),
       NewsType::COMBAT);
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
    push_telegram(entity_manager, ship.owner(), ship.governor(),
                  GB::presentation::render_berserker_pdn_cancelled(
                      prin_ship_orbits(entity_manager, ship)));
    return 0;
  }

  /* look for someone to bombard-check for war */
  const auto target = find_bombardment_target(entity_manager, ship, r);
  if (!target.has_value()) {
    /* there were no sectors worth bombing. */
    if (!ship.notified()) {
      ship.notified() = true;
      push_telegram(
          entity_manager, ship.owner(), ship.governor(),
          GB::presentation::render_berserker_saturation_report(
              {.ship_number = ship.number(),
               .ship_letter = ship.type_letter(),
               .ship_name = ship.name(),
               .star_name = star.get_name(),
               .planet_name = star.get_planet_name(ship.pnumorbits())}));
    }
    return 0;
  }

  const weapon_power_t str = ship.bombardment_strength();
  if (str == 0) {
    /* no weapons! */
    if (!ship.notified()) {
      ship.notified() = true;
      push_telegram(entity_manager, ship.owner(), ship.governor(),
                    GB::presentation::render_berserker_no_weapons(
                        {.ship_number = ship.number(),
                         .ship_letter = ship.type_letter(),
                         .ship_name = ship.name()}));
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
        opt_result =
            shoot_ship_to_planet(entity_manager, ship, planet, str, *target,
                                 smap, false, guntype_t::NONE);
      });
  if (!opt_result) return 0;
  const auto& result = *opt_result;
  /* (0=dont get smap) */
  const auto numdest = result.sectors_destroyed;

  dispatch_bombardment_alerts(entity_manager, ship, star, *target, oldown,
                              numdest, result);

  return numdest;
}
