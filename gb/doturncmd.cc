// SPDX-License-Identifier: Apache-2.0

/* doturn -- does one turn. */

module;

import gblib;
import std.compat;

#include <strings.h>
#include <cassert>

#include "gb/GB_server.h"

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

  // Power blocks for each player
  block Blocks[MAXPLAYERS];

  // Constructor requires EntityManager reference
  explicit TurnState(EntityManager& em) : entity_manager(em) {}

  // Bounds-checked accessors for star population data (delegate to stats)
  unsigned long& star_popn(starnum_t star, player_t player) noexcept {
    assert(star >= 0 && star < NUMSTARS && "Star index out of bounds");
    assert(player >= 1 && player <= MAXPLAYERS && "Player index out of bounds");
    return stats.starpopns[star][player - 1];
  }

  const unsigned long& star_popn(starnum_t star,
                                 player_t player) const noexcept {
    assert(star >= 0 && star < NUMSTARS && "Star index out of bounds");
    assert(player >= 1 && player <= MAXPLAYERS && "Player index out of bounds");
    return stats.starpopns[star][player - 1];
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
static void output_ground_attacks();
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
void do_turn(EntityManager& entity_manager, int update) {
  TurnState state(
      entity_manager);  // Create turn-local state with EntityManager ref

  process_ships(state);
  process_stars_and_planets(state, update);
  process_races(state, update);
  output_ground_attacks();
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
  Planet_count = 0;
  for (auto star_handle : StarList(state.entity_manager)) {
    const starnum_t star = star_handle->get_struct().star_id;

    if (update) {
      fix_stability(state.entity_manager, *star_handle); /* nova */
    }

    for (auto planet_handle :
         PlanetList(state.entity_manager, star, *star_handle)) {
      const planetnum_t pnum = planet_handle->planet_order();
      if (planet_handle->type() != PlanetType::ASTEROID) {
        Planet_count++;
      }
      if (update) {
        moveplanet(state.entity_manager, star, *planet_handle, pnum,
                   state.stats);
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
  VN_brain.Most_mad = 0; /* not mad at anyone for starts */

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
      VN_brain.Total_mad += sdata_handle->VN_hitlist[player - 1];
      /* find out who they're most mad at */
      if (VN_brain.Most_mad > 0 &&
          sdata_handle->VN_hitlist[VN_brain.Most_mad - 1] <=
              sdata_handle->VN_hitlist[player - 1]) {
        VN_brain.Most_mad = player;
      }
    }
    if (VOTING) {
      /* Reset their vote for Update go. */
      // TODO(jeffbailey): This doesn't seem to work.
      race_handle->votes = false;
    }
  }

  output_ground_attacks();
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
    /* reset market */
    Num_commods = state.entity_manager.num_commods();
    clr_commodfree();
    for (commodnum_t i = Num_commods; i >= 1; i--) {
      auto c = getcommod(i);
      if (!c.deliver) {
        c.deliver = true;
        putcommod(c, i);
        continue;
      }

      auto bidder_race = state.entity_manager.get_race(c.bidder);
      auto owner_race = state.entity_manager.get_race(c.owner);

      if (c.owner && c.bidder && bidder_race.get() && owner_race.get() &&
          (bidder_race->governor[c.bidder_gov].money >= c.bid)) {
        bidder_race->governor[c.bidder_gov].money -= c.bid;
        owner_race->governor[c.governor].money += c.bid;
        auto [cost, dist] =
            shipping_cost(state.entity_manager, c.star_to, c.star_from, c.bid);
        bidder_race->governor[c.bidder_gov].cost_market += c.bid + cost;
        owner_race->governor[c.governor].profit_market += c.bid;
        maintain(*bidder_race, bidder_race->governor[c.bidder_gov], cost);

        auto planet_handle =
            state.entity_manager.get_planet(c.star_to, c.planet_to);
        if (planet_handle.get()) {
          switch (c.type) {
            case CommodType::RESOURCE:
              planet_handle->info(c.bidder - 1).resource += c.amount;
              break;
            case CommodType::FUEL:
              planet_handle->info(c.bidder - 1).fuel += c.amount;
              break;
            case CommodType::DESTRUCT:
              planet_handle->info(c.bidder - 1).destruct += c.amount;
              break;
            case CommodType::CRYSTAL:
              planet_handle->info(c.bidder - 1).crystals += c.amount;
              break;
          }
        }

        auto star_handle = state.entity_manager.get_star(c.star_to);
        std::string purchased_msg = std::format(
            "Lot {} purchased from {} [{}] at a cost of {}.\n   {} {} "
            "arrived at /{}/{}\n",
            i, owner_race->name, c.owner, c.bid, c.amount, c.type,
            star_handle.get() ? star_handle->get_name() : "unknown",
            star_handle.get() ? star_handle->get_planet_name(c.planet_to)
                              : "unknown");
        push_telegram(c.bidder, c.bidder_gov, purchased_msg);
        std::string sold_msg =
            std::format("Lot {} ({} {}) sold to {} [{}] at a cost of {}.\n", i,
                        c.amount, c.type, bidder_race->name, c.bidder, c.bid);
        push_telegram(c.owner, c.governor, sold_msg);
        c.owner = c.governor = 0;
        c.bidder = c.bidder_gov = 0;
      } else {
        c.bidder = c.bidder_gov = 0;
        c.bid = 0;
      }
      if (!c.owner) {
        makecommoddead(i);
      }
      putcommod(c, i);
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
            race_handle->governor[ship_handle->governor()].maintain +=
                ship_handle->build_cost();
          }
          if (ship_handle->troops()) {
            race_handle->governor[ship_handle->governor()].maintain +=
                UPDATE_TROOP_COST * ship_handle->troops();
          }
        }
      }
    }
  }
}

static void prepare_dead_ships(TurnState& state) {
  /* prepare dead ships for recycling */
  clr_shipfree();
  const ShipList ships(state.entity_manager, ShipList::IterationType::All);
  for (const Ship* ship : ships) {
    if (!ship->alive()) {
      makeshipdead(ship->number());
    }
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
      /* store occupation for VPs */
      for (auto race_handle : RaceList(state.entity_manager)) {
        const player_t player = race_handle->Playernum;

        if (planet_handle->info(player - 1).numsectsowned) {
          setbit(inhabited[star], player);
          setbit(star_handle->inhabited(), player);
        }
        if (planet_handle->type() != PlanetType::ASTEROID &&
            (planet_handle->info(player - 1).numsectsowned >
             planet_handle->Maxx() * planet_handle->Maxy() / 2)) {
          race_handle->controlled_planets++;
        }

        if (planet_handle->info(player - 1).numsectsowned) {
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

    /* do AP's for ea. player  */
    if (update) {
      for (auto race_handle : RaceList(state.entity_manager)) {
        const player_t player = race_handle->Playernum;

        if (state.stats.starpopns[star][player - 1]) {
          setbit(star_handle->inhabited(), player);
        } else {
          clrbit(star_handle->inhabited(), player);
        }

        if (isset(star_handle->inhabited(), player)) {
          ap_t APs =
              star_handle->AP(player - 1) +
              APadd(
                  static_cast<int>(state.stats.starnumships[star][player - 1]),
                  state.stats.starpopns[star][player - 1], *race_handle, state);
          if (APs < LIMIT_APs) {
            star_handle->AP(player - 1) = APs;
          } else {
            star_handle->AP(player - 1) = LIMIT_APs;
          }
        }
        /* compute victory points for the block */
        if (inhabited[star] != 0) {
          uint64_t dummy =
              state.Blocks[player - 1].invite & state.Blocks[player - 1].pledge;
          state.Blocks[player - 1].systems_owned +=
              (inhabited[star] | dummy) == dummy;
        }
      }
    }
  }

  /* add APs to sdata for ea. player */
  if (update) {
    auto sdata_handle = state.entity_manager.get_universe();
    for (auto race_handle : RaceList(state.entity_manager)) {
      const player_t player = race_handle->Playernum;
      state.Blocks[player - 1].systems_owned = 0; /*recount systems owned*/
      if (governed(*race_handle, state)) {
        ap_t APs = sdata_handle->AP[player - 1] + race_handle->planet_points;
        if (APs < LIMIT_APs) {
          sdata_handle->AP[player - 1] = APs;
        } else {
          sdata_handle->AP[player - 1] = LIMIT_APs;
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
      victory[player - 1].morale = race_handle->morale;
      victory[player - 1].money = race_handle->governor[0].money;
      for (auto& governor : race_handle->governor) {
        if (governor.active) {
          victory[player - 1].money += governor.money;
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
          if (!planet_handle->info(player - 1).explored) {
            continue;
          }
          victory[player - 1].numsects +=
              static_cast<int>(planet_handle->info(player - 1).numsectsowned);
          victory[player - 1].res += planet_handle->info(player - 1).resource;
          victory[player - 1].des +=
              static_cast<int>(planet_handle->info(player - 1).destruct);
          victory[player - 1].fuel +=
              static_cast<int>(planet_handle->info(player - 1).fuel);
        }
      } /* end of planet searchings */
    } /* end of star searchings */

    const ShipList ships(state.entity_manager,
                         ShipList::IterationType::AllAlive);
    for (const Ship* ship : ships) {
      victory[ship->owner() - 1].shipcost += ship->build_cost();
      victory[ship->owner() - 1].shiptech += ship->tech();
      victory[ship->owner() - 1].res += ship->resource();
      victory[ship->owner() - 1].des += ship->destruct();
      victory[ship->owner() - 1].fuel += ship->fuel();
    }
    /* now that we have the info.. calculate the raw score */

    for (auto race_handle : RaceList(state.entity_manager)) {
      const player_t player = race_handle->Playernum;
      race_handle->victory_score =
          (VICT_SECT * victory[player - 1].numsects) +
          (VICT_SHIP * (victory[player - 1].shipcost +
                        (VICT_TECH * victory[player - 1].shiptech))) +
          (VICT_RES * (victory[player - 1].res + victory[player - 1].des)) +
          (VICT_FUEL * victory[player - 1].fuel) +
          (VICT_MONEY * static_cast<int>(victory[player - 1].money));
      race_handle->victory_score /= VICT_DIVISOR;
      race_handle->victory_score = static_cast<int>(
          morale_factor(static_cast<double>(victory[player - 1].morale)) *
          race_handle->victory_score);
    }
  } /* end of if (update) */
}

static void finalize_turn(TurnState& state, int update) {
  if (update) {
    for (auto race_handle : RaceList(state.entity_manager)) {
      const player_t player = race_handle->Playernum;

      /* collective intelligence */
      if (race_handle->collective_iq) {
        double x =
            ((2. / 3.14159265) *
             atan(static_cast<double>(state.stats.Power[player - 1].popn) /
                  MESO_POP_SCALE));
        race_handle->IQ = race_handle->IQ_limit * x * x;
      }
      race_handle->tech += static_cast<double>(race_handle->IQ) / 100.0;
      race_handle->morale += state.stats.Power[player - 1].planets_owned;
      make_discoveries(state.entity_manager, *race_handle);
      race_handle->turn += 1;
      if (race_handle->controlled_planets >=
          Planet_count * VICTORY_PERCENT / 100) {
        race_handle->victory_turns++;
      } else {
        race_handle->victory_turns = 0;
      }

      if (race_handle->controlled_planets >=
          Planet_count * VICTORY_PERCENT / 200) {
        for (auto other_race : RaceList(state.entity_manager)) {
          other_race->translate[player - 1] = 100;
        }
      }

      state.Blocks[player - 1].VPs =
          10L * state.Blocks[player - 1].systems_owned;
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
    compute_power_blocks();
    for (auto race_handle : RaceList(state.entity_manager)) {
      const player_t player = race_handle->Playernum;
      state.stats.Power[player - 1].money = 0;
      for (auto& governor : race_handle->governor) {
        if (governor.active) {
          state.stats.Power[player - 1].money += governor.money;
        }
      }
    }
    putpower(state.stats.Power.data());
    Putblock(state.Blocks);
  }

  for (auto race_handle : RaceList(state.entity_manager)) {
    if (update) {
      notify_race(race_handle->Playernum, "Finished with update.\n");
    } else {
      notify_race(race_handle->Playernum, "Finished with movement segment.\n");
    }
  }
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
      for (i = 1; i <= Num_races; i++)
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
      for (i = 1; i <= Num_races; i++)
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

  int i, j;
  int game_over = 0;
  int win_category[64];

  const int BIG_WINNER = 1;
  const int LITTLE_WINNER = 2;

  for (i = 1; i <= Num_races; i++) {
    win_category[i - 1] = 0;
    if (races[i - 1].controlled_planets >=
        Planet_count * VICTORY_PERCENT / 100) {
      win_category[i - 1] = LITTLE_WINNER;
    }
    if (races[i - 1].victory_turns >= VICTORY_UPDATES) {
      game_over++;
      win_category[i - 1] = BIG_WINNER;
    }
  }
  if (game_over) {
    for (i = 1; i <= Num_races; i++) {
      push_telegram_race(em, i, "*** Attention ***");
      push_telegram_race(em, i, "This game of Galactic Bloodshed is now *over*");
      std::string winner_msg =
          std::format("The big winner{}", (game_over == 1) ? " is" : "s are");
      push_telegram_race(em, i, winner_msg);
      for (j = 1; j <= Num_races; j++)
        if (win_category[j - 1] == BIG_WINNER) {
          std::string big_winner_msg =
              std::format("*** [{:2d}] {:<30.30s} ***", j, races[j - 1].name);
          push_telegram_race(em, i, big_winner_msg);
        }
      push_telegram_race(em, i, "Lesser winners:");
      for (j = 1; j <= Num_races; j++)
        if (win_category[j - 1] == LITTLE_WINNER) {
          std::string little_winner_msg =
              std::format("+++ [{:2d}] {:<30.30s} +++", j, races[j - 1].name);
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
    push_telegram_race(em, r.Playernum, "You have discovered LASER technology.\n");
    r.discoveries[D_LASER] = 1;
  }
  if (!Cew(r) && r.tech >= TECH_CEW) {
    push_telegram_race(em, r.Playernum, "You have discovered CEW technology.\n");
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
    push_telegram_race(em, r.Playernum, "You have discovered AVPM technology.\n");
    r.discoveries[D_AVPM] = 1;
  }
  if (!Cloak(r) && r.tech >= TECH_CLOAK) {
    push_telegram_race(em, r.Playernum, "You have discovered CLOAK technology.\n");
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

static void output_ground_attacks() {
  int star;
  int i;
  int j;

  for (star = 0; star < Sdata.numstars; star++)
    for (i = 1; i <= Num_races; i++)
      for (j = 1; j <= Num_races; j++)
        if (ground_assaults[i - 1][j - 1][star]) {
          std::string assault_news = std::format(
              "{}: {} [{}] assaults {} [{}] {} times.\n",
              stars[star].get_name(), races[i - 1].name, i, races[j - 1].name,
              j, ground_assaults[i - 1][j - 1][star]);
          post(assault_news, NewsType::COMBAT);
          ground_assaults[i - 1][j - 1][star] = 0;
        }
}
