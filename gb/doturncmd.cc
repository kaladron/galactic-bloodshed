// SPDX-License-Identifier: Apache-2.0

/* doturn -- does one turn. */

module;

import gblib;
import std.compat;

#include <strings.h>
#include <sys/stat.h>
#include <cassert>
#include <cstdio>

#include "gb/files.h"

module gblib;

namespace {
// TurnState: Encapsulates turn-processing state that was previously global.
// This struct lives only for the duration of a single turn and provides
// bounds-checked access to turn-specific tracking arrays.
//
// The stats member contains arrays that are passed to doplanet() and doship()
// for accumulating per-turn statistics (Power[], starpopns[], etc.).
struct TurnState {
  // Reference to EntityManager for loading/saving entities
  EntityManager& entity_manager;

  // Turn statistics - passed to doplanet() and doship() for accumulation
  TurnStats stats;

  // Constructor requires EntityManager reference
  explicit TurnState(EntityManager& em) : entity_manager(em) {}

  // Bounds-checked accessors for star population data (delegate to stats)
  unsigned long& star_popn(starnum_t star, player_t player) noexcept {
    assert(star >= 0 && star < NUMSTARS && "Star index out of bounds");
    assert(player.value >= 1 && player.value <= MAXPLAYERS &&
           "Player index out of bounds");
    return stats.starpopns[star][player.value - 1];
  }

  const unsigned long& star_popn(starnum_t star,
                                 player_t player) const noexcept {
    assert(star >= 0 && star < NUMSTARS && "Star index out of bounds");
    assert(player.value >= 1 && player.value <= MAXPLAYERS &&
           "Player index out of bounds");
    return stats.starpopns[star][player.value - 1];
  }
};

}  // anonymous namespace

// Note: RaceList, StarList, PlanetList are now imported from gblib:entitylists

static constexpr void maintain(Race& r, Race::gov& governor,
                               const money_t amount) noexcept {
  if (governor.money >= amount)
    governor.money -= amount;
  else {
    r.morale -= (amount - governor.money) / 10;
    governor.money = 0;
  }
}

static ap_t APadd(const int, const population_t, const Race&, const TurnState&);
static bool attack_planet(const Ship&);
static void fix_stability(EntityManager&, Star&);
static bool governed(const Race&, const TurnState&);
static void make_discoveries(EntityManager&, Race&);
static void output_ground_attacks(TurnState& state);
static void process_ships(TurnState& state);
static void process_stars_and_planets(TurnState& state, int update);
static void process_races(TurnState& state, int update);
static void process_market(TurnState& state, int update);
static void process_ship_masses_and_ownership(TurnState& state);
static void process_ship_turns(TurnState& state, int update);
static void prepare_dead_ships(TurnState& state);
static void insert_ships_into_lists(TurnState& state);
static void process_abms_and_missiles(TurnState& state, int update);
static void update_victory_scores(TurnState& state, int update);
static void finalize_turn(TurnState& state, int update);

/**
 * Process one game turn - either a movement segment or a full update.
 *
 * The game operates on a cycle of multiple movement segments followed by
 * a full update:
 * - Movement segments (update=0): Ships move, repairs occur, missiles/ABMs
 *   process, but planets don't produce and races don't gain tech/APs
 * - Full updates (update=1): Everything from movement segments PLUS planet
 *   production, tech advances, AP calculations, discoveries, victory checks
 *
 * Typically 2-6 movement segments occur between each full update. This allows
 * ship movement and combat to happen more frequently than economic/tech growth.
 *
 * @param entity_manager Database entity manager for persistent storage
 * @param update 1 for full update with production/tech, 0 for movement only
 *               TODO: Should be bool for type safety
 */
void do_turn(EntityManager& entity_manager, SessionRegistry&, int update) {
  TurnState state(
      entity_manager);  // Create turn-local state with EntityManager ref

  process_ships(state);
  process_stars_and_planets(state, update);
  process_races(state, update);
  output_ground_attacks(state);
  process_market(state, update);
  process_ship_masses_and_ownership(state);
  process_ship_turns(state, update);
  prepare_dead_ships(state);
  insert_ships_into_lists(state);
  process_abms_and_missiles(state, update);
  update_victory_scores(state, update);

  // Flush all dirty entities to database in one batch
  entity_manager.flush_all();

  finalize_turn(state, update);

  // Clear cache to free memory after turn completes
  entity_manager.clear_cache();
}

static void process_ships(TurnState& state) {
  // Process mine detonation for each ship
  for (auto ship_handle :
       ShipList(state.entity_manager, ShipList::IterationType::All)) {
    domine(*ship_handle, 0, state.entity_manager);
  }
}

static void process_stars_and_planets(TurnState& state, int update) {
  for (auto star_handle : StarList(state.entity_manager)) {
    const starnum_t star = star_handle->get_struct().star_id;

    if (update) {
      fix_stability(state.entity_manager, *star_handle); /* nova */

      state.stats.StarsInhab[star] = !!(star_handle->inhabited());
      state.stats.StarsExpl[star] = !!(star_handle->explored());
    }

    for (auto planet_handle :
         PlanetList(state.entity_manager, star, *star_handle)) {
      const planetnum_t pnum = planet_handle->planet_order();
      if (update) {
        if (planet_handle->popn() || planet_handle->ships()) {
          state.stats.Stinfo[star][pnum].inhab = 1;
        }
        moveplanet(state.entity_manager, *star_handle, *planet_handle);
      }
      if (!star_handle->planet_name_isset(pnum)) {
        star_handle->set_planet_name(pnum, std::format("NULL-{}", pnum));
      }
    }
    if (star_handle->get_name()[0] == '\0') {
      star_handle->set_name(std::format("NULL-{}", star));
    }
  }
}

static void process_races(TurnState& state, int update) {
  state.stats.VN_brain.Most_mad = 0; /* not mad at anyone for starts */

  // Get universe data for VN hitlist
  auto sdata_handle = state.entity_manager.get_universe();

  for (auto race_handle : RaceList(state.entity_manager)) {
    const player_t player = race_handle->Playernum;

    /* increase tech; change to something else */
    if (update) {
      /* Reset controlled planet count */
      race_handle->controlled_planets = 0;
      race_handle->planet_points = 0;
      for (auto& governor : race_handle->governor) {
        if (governor.active) {
          governor.maintain = 0;
          governor.cost_market = 0;
          governor.profit_market = 0;
          governor.cost_tech = 0;
          governor.income = 0;
        }
      }
      /* add VN program */
      state.stats.VN_brain.Total_mad +=
          sdata_handle->VN_hitlist[player.value - 1];
      /* find out who they're most mad at */
      if (state.stats.VN_brain.Most_mad > 0 &&
          sdata_handle->VN_hitlist[state.stats.VN_brain.Most_mad - 1] <=
              sdata_handle->VN_hitlist[player.value - 1]) {
        state.stats.VN_brain.Most_mad = player.value;
      }
    }
    if (VOTING) {
      /* Reset their vote for Update go. */
      // TODO(jeffbailey): This doesn't seem to work.
      race_handle->votes = false;
    }
  }

  output_ground_attacks(state);
}

/**
 * Process commodity market transactions.
 *
 * Only runs during full updates (update=1), not movement segments.
 * For each commodity on the market:
 * - Delivers commodities that weren't yet delivered
 * - Processes bids: transfers money and goods if bidder can afford it
 * - Charges shipping costs based on distance
 * - Sends telegrams to buyers and sellers
 * - Removes sold/expired commodities
 *
 * @param state Turn state with entity data
 * @param update 1 to process market, 0 to skip (movement segment only)
 */
static void process_market(TurnState& state, int update) {
  if (MARKET && update) {
    /* reset market - note: CommodList filters out null/invalid entries */
    for (auto commod_handle : CommodList(state.entity_manager)) {
      auto& c = *commod_handle;

      if (!c.deliver) {
        c.deliver = true;
        continue;
      }

      auto bidder_race = state.entity_manager.get_race(c.bidder);
      auto owner_race = state.entity_manager.get_race(c.owner);

      if (c.owner != 0 && c.bidder != 0 && bidder_race.get() &&
          owner_race.get() &&
          (bidder_race->governor[c.bidder_gov.value].money >= c.bid)) {
        bidder_race->governor[c.bidder_gov.value].money -= c.bid;
        owner_race->governor[c.governor.value].money += c.bid;
        auto [cost, dist] =
            shipping_cost(state.entity_manager, c.star_to, c.star_from, c.bid);
        bidder_race->governor[c.bidder_gov.value].cost_market += c.bid + cost;
        owner_race->governor[c.governor.value].profit_market += c.bid;
        maintain(*bidder_race, bidder_race->governor[c.bidder_gov.value], cost);

        auto planet_handle =
            state.entity_manager.get_planet(c.star_to, c.planet_to);
        if (planet_handle.get()) {
          switch (c.type) {
            case CommodType::RESOURCE:
              planet_handle->info(c.bidder).resource += c.amount;
              break;
            case CommodType::FUEL:
              planet_handle->info(c.bidder).fuel += c.amount;
              break;
            case CommodType::DESTRUCT:
              planet_handle->info(c.bidder).destruct += c.amount;
              break;
            case CommodType::CRYSTAL:
              planet_handle->info(c.bidder).crystals += c.amount;
              break;
          }
        }

        auto star_handle = state.entity_manager.get_star(c.star_to);
        std::string purchased_msg = std::format(
            "Lot {} purchased from {} [{}] at a cost of {}.\n   {} {} "
            "arrived at /{}/{}\n",
            c.id, owner_race->name, c.owner, c.bid, c.amount, c.type,
            star_handle.get() ? star_handle->get_name() : "unknown",
            star_handle.get() ? star_handle->get_planet_name(c.planet_to)
                              : "unknown");
        push_telegram(state.entity_manager, c.bidder, c.bidder_gov,
                      purchased_msg);
        std::string sold_msg = std::format(
            "Lot {} ({} {}) sold to {} [{}] at a cost of {}.\n", c.id, c.amount,
            c.type, bidder_race->name, c.bidder, c.bid);
        push_telegram(state.entity_manager, c.owner, c.governor, sold_msg);
        c.owner = 0;
        c.governor = 0;
        c.bidder = 0;
        c.bidder_gov = 0;
      } else {
        c.bidder = 0;
        c.bidder_gov = 0;
        c.bid = 0;
      }
      if (c.owner == player_t{0}) {
        // Commodity is dead - delete it after handle releases
        state.entity_manager.delete_commod(c.id);
      }
    }
  }
}

static void process_ship_masses_and_ownership(TurnState& state) {
  /* check ship masses - ownership */
  for (auto ship_handle :
       ShipList(state.entity_manager, ShipList::IterationType::AllAlive)) {
    domass(*ship_handle, state.entity_manager);
    doown(*ship_handle, state.entity_manager);
  }
}

static void process_ship_turns(TurnState& state, int update) {
  /* do all ships one turn - do slower ships first */
  for (int j = 0; j <= 9; j++) {
    for (auto ship_handle :
         ShipList(state.entity_manager, ShipList::IterationType::AllAlive)) {
      if (ship_handle->speed() == j) {
        doship(*ship_handle, update, state.entity_manager, state.stats);
        if ((ship_handle->type() == ShipType::STYPE_MISSILE) &&
            !attack_planet(*ship_handle)) {
          domissile(*ship_handle, state.entity_manager);
        }
      }
    }
  }

  if (MARKET) {
    /* do maintenance costs */
    if (update) {
      for (auto ship_handle :
           ShipList(state.entity_manager, ShipList::IterationType::AllAlive)) {
        if (Shipdata[ship_handle->type()][ABIL_MAINTAIN]) {
          auto race_handle =
              state.entity_manager.get_race(ship_handle->owner());
          if (!race_handle.get()) continue;

          if (ship_handle->popn()) {
            race_handle->governor[ship_handle->governor().value].maintain +=
                ship_handle->build_cost();
          }
          if (ship_handle->troops()) {
            race_handle->governor[ship_handle->governor().value].maintain +=
                UPDATE_TROOP_COST * ship_handle->troops();
          }
        }
      }
    }
  }
}

static void prepare_dead_ships(TurnState& state) {
  /* prepare dead ships for recycling */
  // Collect ship numbers to delete (can't delete while iterating)
  std::vector<shipnum_t> dead_ships;
  const ShipList ships(state.entity_manager, ShipList::IterationType::All);
  for (const Ship* ship : ships) {
    if (!ship->alive()) {
      dead_ships.push_back(ship->number());
    }
  }

  // Delete dead ships
  for (shipnum_t num : dead_ships) {
    state.entity_manager.delete_ship(num);
  }
}

static void insert_ships_into_lists(TurnState& state) {
  /* erase next ship pointers - reset in insert_sh_... */
  for (auto ship_handle :
       ShipList(state.entity_manager, ShipList::IterationType::All)) {
    ship_handle->nextship() = 0;
    ship_handle->ships() = 0;
  }

  /* clear ship list for insertion */
  auto sdata_handle = state.entity_manager.get_universe();
  sdata_handle->ships = 0;

  for (auto star_handle : StarList(state.entity_manager)) {
    const starnum_t star = star_handle->get_struct().star_id;
    star_handle->ships() = 0;
    for (auto planet_handle :
         PlanetList(state.entity_manager, star, *star_handle)) {
      planet_handle->ships() = 0;
    }
  }

  /* insert ship into the list of wherever it might be */
  for (shipnum_t i = state.entity_manager.num_ships(); i >= 1; i--) {
    auto ship_handle = state.entity_manager.get_ship(i);
    if (ship_handle.get() && ship_handle->alive()) {
      switch (ship_handle->whatorbits()) {
        case ScopeLevel::LEVEL_UNIV: {
          auto sdata = state.entity_manager.get_universe();
          insert_sh_univ(sdata.get(), ship_handle.get());
          break;
        }
        case ScopeLevel::LEVEL_STAR: {
          auto star = state.entity_manager.get_star(ship_handle->storbits());
          if (star.get()) {
            insert_sh_star(*star, ship_handle.get());
          }
          break;
        }
        case ScopeLevel::LEVEL_PLAN: {
          auto planet = state.entity_manager.get_planet(
              ship_handle->storbits(), ship_handle->pnumorbits());
          if (planet.get()) {
            insert_sh_plan(*planet, ship_handle.get());
          }
          break;
        }
        case ScopeLevel::LEVEL_SHIP: {
          auto dest_ship =
              state.entity_manager.get_ship(ship_handle->destshipno());
          if (dest_ship.get()) {
            insert_sh_ship(ship_handle.get(), dest_ship.get());
          }
          break;
        }
      }
    }
  }
}

static void process_abms_and_missiles(TurnState& state, int update) {
  /* put ABMs and surviving missiles here because ABMs need to have the missile
     in the shiplist of the target planet  Maarten */
  for (auto ship_handle :
       ShipList(state.entity_manager, ShipList::IterationType::AllAlive)) {
    if (ship_handle->type() == ShipType::OTYPE_ABM) {
      doabm(*ship_handle, state.entity_manager);
    }
  }

  for (auto ship_handle :
       ShipList(state.entity_manager, ShipList::IterationType::AllAlive)) {
    if (ship_handle->type() == ShipType::STYPE_MISSILE &&
        attack_planet(*ship_handle)) {
      domissile(*ship_handle, state.entity_manager);
    }
  }

  // Local inhabited bitmap - tracks which players inhabit each star this turn
  std::array<uint64_t, NUMSTARS> inhabited{};

  for (auto star_handle : StarList(state.entity_manager)) {
    const starnum_t star = star_handle->get_struct().star_id;

    for (auto planet_handle :
         PlanetList(state.entity_manager, star, *star_handle)) {
      for (auto race_handle : RaceList(state.entity_manager)) {
        const player_t player = race_handle->Playernum;

        if (planet_handle->info(player).numsectsowned) {
          setbit(inhabited[star], player);
        }

        if (planet_handle->type() != PlanetType::ASTEROID &&
            (planet_handle->info(player).numsectsowned >
             planet_handle->Maxx() * planet_handle->Maxy() / 2)) {
          race_handle->controlled_planets++;
        }

        if (planet_handle->info(player).numsectsowned) {
          race_handle->planet_points += planet_handle->get_points();
        }
      }
      if (update) {
        if (doplanet(state.entity_manager, *star_handle, *planet_handle,
                     state.stats)) {
          /* save smap gotten & altered by doplanet
             only if the planet is expl*/
        }
      }
    }

    if (update) {
      // Build inhabited bitmap from starpopns and calculate APs
      star_handle->inhabited() = 0;
      for (auto race_handle : RaceList(state.entity_manager)) {
        const player_t player = race_handle->Playernum;

        if (state.stats.starpopns[star][player.value - 1]) {
          setbit(star_handle->inhabited(), player);

          ap_t APs =
              star_handle->AP(player) +
              APadd(static_cast<int>(
                        state.stats.starnumships[star][player.value - 1]),
                    state.stats.starpopns[star][player.value - 1], *race_handle,
                    state);
          if (APs < LIMIT_APs) {
            star_handle->AP(player) = APs;
          } else {
            star_handle->AP(player) = LIMIT_APs;
          }
        }
        // Compute victory points for the block
        if (inhabited[star] != 0) {
          const auto* block_player =
              state.entity_manager.peek_block(player.value);
          if (block_player) {
            uint64_t allied_members =
                block_player->invite & block_player->pledge;
            if ((inhabited[star] | allied_members) == allied_members) {
              auto block_handle = state.entity_manager.get_block(player.value);
              block_handle->systems_owned++;
            }
          }
        }
      }
    }
  }

  /* add APs to sdata for ea. player */
  if (update) {
    auto sdata_handle = state.entity_manager.get_universe();
    for (auto race_handle : RaceList(state.entity_manager)) {
      const player_t player = race_handle->Playernum;
      auto block_handle = state.entity_manager.get_block(player.value);
      block_handle->systems_owned = 0; /*recount systems owned*/
      if (governed(*race_handle, state)) {
        ap_t APs =
            sdata_handle->AP[player.value - 1] + race_handle->planet_points;
        if (APs < LIMIT_APs) {
          sdata_handle->AP[player.value - 1] = APs;
        } else {
          sdata_handle->AP[player.value - 1] = LIMIT_APs;
        }
      }
    }
  }

  /* here is where we do victory calculations. */
}

static void update_victory_scores(TurnState& state, int update) {
  if (update) {
    struct victstruct {
      int numsects{0};
      int shipcost{0};
      int shiptech{0};
      int morale{0};
      resource_t res{0};
      int des{0};
      int fuel{0};
      money_t money{0};
    };

    std::array<victstruct, MAXPLAYERS> victory;

    for (auto race_handle : RaceList(state.entity_manager)) {
      const player_t player = race_handle->Playernum;
      victory[player.value - 1].morale = race_handle->morale;
      victory[player.value - 1].money = race_handle->governor[0].money;
      for (auto& governor : race_handle->governor) {
        if (governor.active) {
          victory[player.value - 1].money += governor.money;
        }
      }
    }

    for (auto star_handle : StarList(state.entity_manager)) {
      /* do planets in the star next */
      for (auto planet_handle :
           PlanetList(state.entity_manager, star_handle->get_struct().star_id,
                      *star_handle)) {
        for (auto race_handle : RaceList(state.entity_manager)) {
          const player_t player = race_handle->Playernum;
          if (!planet_handle->info(player).explored) {
            continue;
          }
          victory[player.value - 1].numsects +=
              static_cast<int>(planet_handle->info(player).numsectsowned);
          victory[player.value - 1].res += planet_handle->info(player).resource;
          victory[player.value - 1].des +=
              static_cast<int>(planet_handle->info(player).destruct);
          victory[player.value - 1].fuel +=
              static_cast<int>(planet_handle->info(player).fuel);
        }
      } /* end of planet searchings */
    } /* end of star searchings */

    const ShipList ships(state.entity_manager,
                         ShipList::IterationType::AllAlive);
    for (const Ship* ship : ships) {
      victory[ship->owner().value - 1].shipcost += ship->build_cost();
      victory[ship->owner().value - 1].shiptech += ship->tech();
      victory[ship->owner().value - 1].res += ship->resource();
      victory[ship->owner().value - 1].des += ship->destruct();
      victory[ship->owner().value - 1].fuel += ship->fuel();
    }
    /* now that we have the info.. calculate the raw score */

    for (auto race_handle : RaceList(state.entity_manager)) {
      const player_t player = race_handle->Playernum;
      race_handle->victory_score =
          (VICT_SECT * victory[player.value - 1].numsects) +
          (VICT_SHIP * (victory[player.value - 1].shipcost +
                        (VICT_TECH * victory[player.value - 1].shiptech))) +
          (VICT_RES *
           (victory[player.value - 1].res + victory[player.value - 1].des)) +
          (VICT_FUEL * victory[player.value - 1].fuel) +
          (VICT_MONEY * static_cast<int>(victory[player.value - 1].money));
      race_handle->victory_score /= VICT_DIVISOR;
      race_handle->victory_score = static_cast<int>(
          morale_factor(static_cast<double>(victory[player.value - 1].morale)) *
          race_handle->victory_score);
    }
  } /* end of if (update) */
}

static void finalize_turn(TurnState& state, int update) {
  const planetnum_t planet_count =
      state.entity_manager.peek_universe()->planet_count;
  if (update) {
    for (auto race_handle : RaceList(state.entity_manager)) {
      const player_t player = race_handle->Playernum;

      /* collective intelligence */
      if (race_handle->collective_iq) {
        double x = ((2. / 3.14159265) *
                    atan(static_cast<double>(
                             state.stats.Power[player.value - 1].popn) /
                         MESO_POP_SCALE));
        race_handle->IQ = race_handle->IQ_limit * x * x;
      }
      race_handle->tech += static_cast<double>(race_handle->IQ) / 100.0;
      race_handle->morale += state.stats.Power[player.value - 1].planets_owned;
      make_discoveries(state.entity_manager, *race_handle);
      race_handle->turn += 1;
      if (race_handle->controlled_planets >=
          planet_count * VICTORY_PERCENT / 100) {
        race_handle->victory_turns++;
      } else {
        race_handle->victory_turns = 0;
      }

      if (race_handle->controlled_planets >=
          planet_count * VICTORY_PERCENT / 200) {
        for (auto other_race : RaceList(state.entity_manager)) {
          other_race->translate[player.value - 1] = 100;
        }
      }

      auto block_handle = state.entity_manager.get_block(player.value);
      block_handle->VPs = 10L * block_handle->systems_owned;
      if (MARKET) {
        for (auto& governor : race_handle->governor) {
          if (governor.active) {
            maintain(*race_handle, governor, governor.maintain);
          }
        }
      }
    }
  }

  // No manual free() needed - vector cleanup is automatic

  if (update) {
    compute_power_blocks(state.entity_manager);
    for (auto race_handle : RaceList(state.entity_manager)) {
      const player_t player = race_handle->Playernum;
      state.stats.Power[player.value - 1].money = 0;
      for (auto& governor : race_handle->governor) {
        if (governor.active) {
          state.stats.Power[player.value - 1].money += governor.money;
        }
      }
    }
    // Save power data via EntityManager
    for (int i : std::views::iota(0, MAXPLAYERS)) {
      auto power_handle = state.entity_manager.get_power(i);
      *power_handle = state.stats.Power[i];
    }
  }

  // Note: Notification to players about update/segment completion is now
  // handled by the caller (do_update/do_segment in GB_server.cc)
}

/* routine for number of AP's to add to each player in ea. system,scaled
    by amount of crew in their palace */

static ap_t APadd(const int sh, const population_t popn, const Race& race,
                  const TurnState& state) {
  ap_t APs;

  APs = round_rand((double)sh / 10.0 + 5. * log10(1.0 + (double)popn));

  if (governed(race, state)) return APs;
  /* dont have an active gov center */
  return round_rand((double)APs / 20.);
}

/**
 * Checks if a given race is governed.
 *
 * This function determines whether a race is governed. A race is considered
 * governed if the following conditions are met:
 * - The race has a government ship assigned.
 * - The government ship is a valid ship index.
 * - The government ship is alive and docked.
 * - The government ship is either orbiting a planet or orbiting another ship
 * that is a habitat orbiting a planet or a star.
 *
 * @param race The race to check for governance.
 * @return True if the race is governed, false otherwise.
 */
static bool governed(const Race& race, const TurnState& state) {
  if (!race.Gov_ship || race.Gov_ship > state.entity_manager.num_ships()) {
    return false;
  }

  const auto* gov_ship = state.entity_manager.peek_ship(race.Gov_ship);
  if (!gov_ship || !gov_ship->alive() || !gov_ship->docked()) {
    return false;
  }

  // Check if docked at a planet
  if (gov_ship->whatdest() == ScopeLevel::LEVEL_PLAN) {
    return true;
  }

  // Check if docked at a habitat that's orbiting a planet or star
  if (gov_ship->whatorbits() == ScopeLevel::LEVEL_SHIP) {
    const auto* habitat =
        state.entity_manager.peek_ship(gov_ship->destshipno());
    if (habitat && habitat->type() == ShipType::STYPE_HABITAT &&
        (habitat->whatorbits() == ScopeLevel::LEVEL_PLAN ||
         habitat->whatorbits() == ScopeLevel::LEVEL_STAR)) {
      return true;
    }
  }

  return false;
}

/* fix stability for stars */
void fix_stability(EntityManager& em, Star& s) {
  int a;
  int i;

  if (s.nova_stage() > 0) {
    if (s.nova_stage() > 14) {
      s.stability() = 20;
      s.nova_stage() = 0;
      std::string telegram_msg = std::format(
          "Notice\n\n  Scientists report that star {}\nis no longer undergoing "
          "nova.\n",
          s.get_name());
      for (i = 1; i <= em.num_races(); i++)
        push_telegram_race(em, i, telegram_msg);

      /* telegram everyone when nova over? */
    } else
      s.nova_stage()++;
  } else if (s.stability() > 20) {
    a = int_rand(-1, 3);
    /* nova just starting; notify everyone */
    if ((s.stability() + a) > 100) {
      s.stability() = 100;
      s.nova_stage() = 1;
      std::string telegram_msg = std::format(
          "***** BULLETIN! ******\n\n  Scientists report that star {}\nis "
          "undergoing nova.\n",
          s.get_name());
      for (i = 1; i <= em.num_races(); i++)
        push_telegram_race(em, i, telegram_msg);
    } else
      s.stability() += a;
  } else {
    a = int_rand(-1, 1);
    if (((int)s.stability() + a) < 0)
      s.stability() = 0;
    else
      s.stability() += a;
  }
}

void handle_victory(EntityManager& em) {
  if (!VICTORY) return;

  const planetnum_t planet_count = em.peek_universe()->planet_count;
  int i, j;
  int game_over = 0;
  int win_category[64];

  const int BIG_WINNER = 1;
  const int LITTLE_WINNER = 2;

  for (i = 1; i <= em.num_races(); i++) {
    win_category[i - 1] = 0;
    const auto* race = em.peek_race(i);
    if (!race) continue;
    if (race->controlled_planets >= planet_count * VICTORY_PERCENT / 100) {
      win_category[i - 1] = LITTLE_WINNER;
    }
    if (race->victory_turns >= VICTORY_UPDATES) {
      game_over++;
      win_category[i - 1] = BIG_WINNER;
    }
  }
  if (game_over) {
    for (i = 1; i <= em.num_races(); i++) {
      push_telegram_race(em, i, "*** Attention ***");
      push_telegram_race(em, i,
                         "This game of Galactic Bloodshed is now *over*");
      std::string winner_msg =
          std::format("The big winner{}", (game_over == 1) ? " is" : "s are");
      push_telegram_race(em, i, winner_msg);
      for (j = 1; j <= em.num_races(); j++)
        if (win_category[j - 1] == BIG_WINNER) {
          const auto* winner_race = em.peek_race(j);
          if (!winner_race) continue;
          std::string big_winner_msg =
              std::format("*** [{:2d}] {:<30.30s} ***", j, winner_race->name);
          push_telegram_race(em, i, big_winner_msg);
        }
      push_telegram_race(em, i, "Lesser winners:");
      for (j = 1; j <= em.num_races(); j++)
        if (win_category[j - 1] == LITTLE_WINNER) {
          const auto* winner_race = em.peek_race(j);
          if (!winner_race) continue;
          std::string little_winner_msg =
              std::format("+++ [{:2d}] {:<30.30s} +++", j, winner_race->name);
          push_telegram_race(em, i, little_winner_msg);
        }
    }
  }
}

static void make_discoveries(EntityManager& em, Race& r) {
  /* would be nicer to do this with a loop of course - but it's late */
  if (!Hyper_drive(r) && r.tech >= TECH_HYPER_DRIVE) {
    push_telegram_race(em, r.Playernum,
                       "You have discovered HYPERDRIVE technology.\n");
    r.discoveries[D_HYPER_DRIVE] = 1;
  }
  if (!Laser(r) && r.tech >= TECH_LASER) {
    push_telegram_race(em, r.Playernum,
                       "You have discovered LASER technology.\n");
    r.discoveries[D_LASER] = 1;
  }
  if (!Cew(r) && r.tech >= TECH_CEW) {
    push_telegram_race(em, r.Playernum,
                       "You have discovered CEW technology.\n");
    r.discoveries[D_CEW] = 1;
  }
  if (!Vn(r) && r.tech >= TECH_VN) {
    push_telegram_race(em, r.Playernum, "You have discovered VN technology.\n");
    r.discoveries[D_VN] = 1;
  }
  if (!Tractor_beam(r) && r.tech >= TECH_TRACTOR_BEAM) {
    push_telegram_race(em, r.Playernum,
                       "You have discovered TRACTOR BEAM technology.\n");
    r.discoveries[D_TRACTOR_BEAM] = 1;
  }
  if (!Transporter(r) && r.tech >= TECH_TRANSPORTER) {
    push_telegram_race(em, r.Playernum,
                       "You have discovered TRANSPORTER technology.\n");
    r.discoveries[D_TRANSPORTER] = 1;
  }
  if (!Avpm(r) && r.tech >= TECH_AVPM) {
    push_telegram_race(em, r.Playernum,
                       "You have discovered AVPM technology.\n");
    r.discoveries[D_AVPM] = 1;
  }
  if (!Cloak(r) && r.tech >= TECH_CLOAK) {
    push_telegram_race(em, r.Playernum,
                       "You have discovered CLOAK technology.\n");
    r.discoveries[D_CLOAK] = 1;
  }
  if (!Wormhole(r) && r.tech >= TECH_WORMHOLE) {
    push_telegram_race(em, r.Playernum,
                       "You have discovered WORMHOLE technology.\n");
    r.discoveries[D_WORMHOLE] = 1;
  }
  if (!Crystal(r) && r.tech >= TECH_CRYSTAL) {
    push_telegram_race(em, r.Playernum,
                       "You have discovered CRYSTAL technology.\n");
    r.discoveries[D_CRYSTAL] = 1;
  }
}

static bool attack_planet(const Ship& ship) {
  return ship.whatdest() == ScopeLevel::LEVEL_PLAN;
}

static void output_ground_attacks(TurnState& state) {
  EntityManager& em = state.entity_manager;
  for (auto star_handle : StarList(em)) {
    const auto& star = *star_handle;
    const starnum_t star_num = star.star_id();

    for (auto race_i_handle : RaceList(em)) {
      const auto& race_i = *race_i_handle;
      const player_t i = race_i.Playernum;

      for (auto race_j_handle : RaceList(em)) {
        const auto& race_j = *race_j_handle;
        const player_t j = race_j.Playernum;

        if (ground_assaults[i.value - 1][j.value - 1][star_num]) {
          std::string assault_news =
              std::format("{}: {} [{}] assaults {} [{}] {} times.\n",
                          star.get_name(), race_i.name, i, race_j.name, j,
                          ground_assaults[i.value - 1][j.value - 1][star_num]);
          post(em, assault_news, NewsType::COMBAT);
          ground_assaults[i.value - 1][j.value - 1][star_num] = 0;
        }
      }
    }
  }
}

void compute_power_blocks(EntityManager& entity_manager) {
  /* compute alliance block power */
  Power_blocks.time = time(nullptr);
  for (auto race_i_handle : RaceList(entity_manager)) {
    const auto& race_i = race_i_handle.read();
    const player_t i = race_i.Playernum;

    const auto* block_i = entity_manager.peek_block(i.value);
    if (!block_i) continue;

    uint64_t allied_members = block_i->invite & block_i->pledge;
    Power_blocks.members[i.value - 1] = 0;
    Power_blocks.sectors_owned[i.value - 1] = 0;
    Power_blocks.popn[i.value - 1] = 0;
    Power_blocks.ships_owned[i.value - 1] = 0;
    Power_blocks.resource[i.value - 1] = 0;
    Power_blocks.fuel[i.value - 1] = 0;
    Power_blocks.destruct[i.value - 1] = 0;
    Power_blocks.money[i.value - 1] = 0;
    Power_blocks.systems_owned[i.value - 1] = block_i->systems_owned;
    Power_blocks.VPs[i.value - 1] = block_i->VPs;

    for (auto race_j_handle : RaceList(entity_manager)) {
      const auto& race_j = race_j_handle.read();
      const player_t j = race_j.Playernum;

      if (isset(allied_members, j)) {
        const auto* power_ptr = entity_manager.peek_power(j.value);
        if (!power_ptr) continue;
        Power_blocks.members[i.value - 1] += 1;
        Power_blocks.sectors_owned[i.value - 1] += power_ptr->sectors_owned;
        Power_blocks.money[i.value - 1] += power_ptr->money;
        Power_blocks.popn[i.value - 1] += power_ptr->popn;
        Power_blocks.ships_owned[i.value - 1] += power_ptr->ships_owned;
        Power_blocks.resource[i.value - 1] += power_ptr->resource;
        Power_blocks.fuel[i.value - 1] += power_ptr->fuel;
        Power_blocks.destruct[i.value - 1] += power_ptr->destruct;
      }
    }
  }
}

// --- Update/Segment state (previously in GB_server.cc) ---
namespace {
ScheduleInfo schedule_info;
}  // namespace

const ScheduleInfo& get_schedule_info() {
  return schedule_info;
}

void set_server_start_time(std::time_t start_time) {
  schedule_info.start_buf =
      std::format("Server started  : {}", ctime(&start_time));
}

void do_update(EntityManager& entity_manager, SessionRegistry& session_registry,
               bool force) {
  time_t clk = time(nullptr);
  struct stat stbuf;

  // Get server state handle (will auto-save on scope exit)
  auto state_handle = entity_manager.get_server_state();
  auto& state = *state_handle;

  bool fakeit = (!force && stat(nogofl.data(), &stbuf) >= 0);

  std::string update_msg = std::format("{}DOING UPDATE...\n", ctime(&clk));
  if (!fakeit) {
    for (auto i = 1; i <= entity_manager.num_races(); i++)
      session_registry.notify_race(i, update_msg);
    // Flush immediately so players see the message before long-running
    // do_turn()
    session_registry.flush_all();
  }

  if (state.segments <= 1) {
    /* Disables movement segments. */
    state.next_segment_time = clk + (144 * 3600);
    state.nsegments_done = state.segments;
  } else {
    if (force)
      state.next_segment_time =
          clk + state.update_time_minutes * 60 / state.segments;
    else
      state.next_segment_time = state.next_update_time +
                                state.update_time_minutes * 60 / state.segments;
    state.nsegments_done = 1;
  }
  if (force)
    state.next_update_time = clk + state.update_time_minutes * 60;
  else
    state.next_update_time += state.update_time_minutes * 60;

  if (!fakeit) schedule_info.nupdates_done++;

  Power_blocks.time = clk;
  schedule_info.last_update_time = clk;
  schedule_info.update_buf = std::format(
      "Last Update {0:3d} : {1}", schedule_info.nupdates_done, ctime(&clk));
  std::print(stderr, "{}", ctime(&clk));
  std::print(stderr, "Next Update {0:3d} : {1}",
             schedule_info.nupdates_done + 1, ctime(&state.next_update_time));
  schedule_info.last_segment_time = clk;
  schedule_info.segment_buf = std::format("Last Segment {0:2d} : {1}",
                                          state.nsegments_done, ctime(&clk));
  std::print(stderr, "{}", ctime(&clk));
  std::print(stderr, "Next Segment {0:2d} : {1}",
             state.nsegments_done == state.segments ? 1
                                                    : state.nsegments_done + 1,
             ctime(&state.next_segment_time));

  session_registry.set_update_in_progress(true);
  if (!fakeit) do_turn(entity_manager, session_registry, 1);
  session_registry.set_update_in_progress(false);
  clk = time(nullptr);
  std::string finish_msg = std::format("{}Update {} finished\n", ctime(&clk),
                                       schedule_info.nupdates_done);
  handle_victory(entity_manager);
  if (!fakeit) {
    for (auto i = 1; i <= entity_manager.num_races(); i++)
      session_registry.notify_race(i, finish_msg);
  }
}

void do_segment(EntityManager& entity_manager,
                SessionRegistry& session_registry, int override, int segment) {
  time_t clk = time(nullptr);
  struct stat stbuf;

  // Get server state handle (will auto-save on scope exit)
  auto state_handle = entity_manager.get_server_state();
  auto& state = *state_handle;

  bool fakeit = (!override && stat(nogofl.data(), &stbuf) >= 0);

  if (!override && state.segments <= 1) return;

  std::string movement_msg = std::format("{}DOING MOVEMENT...\n", ctime(&clk));
  if (!fakeit) {
    for (auto i = 1; i <= entity_manager.num_races(); i++)
      session_registry.notify_race(i, movement_msg);
    // Flush immediately so players see the message before long-running
    // do_turn()
    session_registry.flush_all();
  }
  if (override) {
    state.next_segment_time =
        clk + state.update_time_minutes * 60 / state.segments;
    if (segment) {
      state.nsegments_done = segment;
      state.next_update_time = clk + state.update_time_minutes * 60 *
                                         (state.segments - segment + 1) /
                                         state.segments;
    } else {
      state.nsegments_done++;
    }
  } else {
    state.next_segment_time += state.update_time_minutes * 60 / state.segments;
    state.nsegments_done++;
  }

  session_registry.set_update_in_progress(true);
  if (!fakeit) do_turn(entity_manager, session_registry, 0);
  session_registry.set_update_in_progress(false);
  schedule_info.last_segment_time = clk;
  schedule_info.segment_buf = std::format("Last Segment {0:2d} : {1}",
                                          state.nsegments_done, ctime(&clk));
  std::print(stderr, "{0}", ctime(&clk));
  std::print(stderr, "Next Segment {0:2d} : {1}", state.nsegments_done,
             ctime(&state.next_segment_time));
  clk = time(nullptr);
  std::string segment_msg = std::format("{}Segment finished\n", ctime(&clk));
  if (!fakeit) {
    for (auto i = 1; i <= entity_manager.num_races(); i++)
      session_registry.notify_race(i, segment_msg);
  }
}

void do_next_thing(EntityManager& entity_manager,
                   SessionRegistry& session_registry) {
  const auto* state = entity_manager.peek_server_state();
  if (!state) return;

  if (state->nsegments_done < state->segments)
    do_segment(entity_manager, session_registry, 0, 1);
  else
    do_update(entity_manager, session_registry, false);
}
