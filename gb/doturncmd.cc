// SPDX-License-Identifier: Apache-2.0

/* doturn -- does one turn. */

module;

import gblib;
import std.compat;

#include <strings.h>

#include "gb/GB_server.h"
#include "gb/buffers.h"

module gblib;

static constexpr void maintain(Race &r, Race::gov &governor,
                               const money_t amount) noexcept {
  if (governor.money >= amount)
    governor.money -= amount;
  else {
    r.morale -= (amount - governor.money) / 10;
    governor.money = 0;
  }
}

static ap_t APadd(const int, const population_t, const Race &);
static bool attack_planet(const Ship &);
static void fix_stability(Star &);
static bool governed(const Race &);
static void make_discoveries(Race &);
static void output_ground_attacks();
static void initialize_data(int update);
static void process_ships();
static void process_stars_and_planets(int update);
static void process_races(int update);
static void process_market(Db &db, int update);
static void process_ship_masses_and_ownership();
static void process_ship_turns(int update);
static void prepare_dead_ships();
static void insert_ships_into_lists();
static void process_abms_and_missiles(int update);
static void update_victory_scores(int update);
static void finalize_turn(int update);

void do_turn(Db &db, int update) {
  initialize_data(update);
  process_ships();
  process_stars_and_planets(update);
  process_races(update);
  output_ground_attacks();
  process_market(db, update);
  process_ship_masses_and_ownership();
  process_ship_turns(update);
  prepare_dead_ships();
  insert_ships_into_lists();
  process_abms_and_missiles(update);
  update_victory_scores(update);
  finalize_turn(update);
}

static void initialize_data(int update) {
  /* make all 0 for first iteration of doplanet */
  if (update) {
    std::memset(starpopns, 0, sizeof(starpopns));
    std::memset(starnumships, 0, sizeof(starnumships));
    std::memset(Sdatanumships, 0, sizeof(Sdatanumships));
    std::memset(Stinfo, 0, sizeof(Stinfo));
    std::memset(StarsInhab, 0, sizeof(StarsInhab));
    std::memset(Power, 0, sizeof(Power));
    std::memset(inhabited, 0, sizeof(inhabited));
  }

  Num_ships = Numships();
}

static void process_ships() {
  // TODO(jeffbailey): We loop through the ships twice here because that's what
  // the code did before.  It's probably not necessary.

  ships = (Ship **)malloc(sizeof(Ship *) * (Num_ships + 1));
  for (shipnum_t i = 1; i <= Num_ships; i++) {
    (void)getship(&ships[i], i);
    domine(*ships[i], 0);
  }

  for (shipnum_t i = 1; i <= Num_ships; i++) (void)getship(&ships[i], i);
}

static void process_stars_and_planets(int update) {
  /* get all stars and planets */
  getsdata(&Sdata);
  Planet_count = 0;
  for (starnum_t star = 0; star < Sdata.numstars; star++) {
    stars[star] = getstar(star);
    if (update) fix_stability(stars[star]); /* nova */

    for (planetnum_t i = 0; i < stars[star].numplanets(); i++) {
      planets[star][i] = std::make_unique<Planet>(getplanet(star, i));
      if (planets[star][i]->type != PlanetType::ASTEROID) Planet_count++;
      if (update) moveplanet(star, *planets[star][i], i);
      if (!stars[star].planet_name_isset(i))
        stars[star].set_planet_name(i, std::format("NULL-{}", i));
    }
    if (stars[star].get_name()[0] == '\0')
      stars[star].set_name(std::format("NULL-{}", star));
  }
}

static void process_races(int update) {
  VN_brain.Most_mad = 0; /* not mad at anyone for starts */

  for (player_t i = 1; i <= Num_races; i++) {
    /* increase tech; change to something else */
    if (update) {
      /* Reset controlled planet count */
      races[i - 1].controlled_planets = 0;
      races[i - 1].planet_points = 0;
      for (auto &governor : races[i - 1].governor)
        if (governor.active) {
          governor.maintain = 0;
          governor.cost_market = 0;
          governor.profit_market = 0;
          governor.cost_tech = 0;
          governor.income = 0;
        }
      /* add VN program */
      VN_brain.Total_mad += Sdata.VN_hitlist[i - 1];
      /* find out who they're most mad at */
      if (VN_brain.Most_mad > 0 &&
          Sdata.VN_hitlist[VN_brain.Most_mad - 1] <= Sdata.VN_hitlist[i - 1])
        VN_brain.Most_mad = i;
    }
    if (VOTING) {
      /* Reset their vote for Update go. */
      // TODO(jeffbailey): This doesn't seem to work.
      races[i - 1].votes = false;
    }
  }

  output_ground_attacks();
}

static void process_market(Db &db, int update) {
  if (MARKET && update) {
    /* reset market */
    Num_commods = db.Numcommods();
    clr_commodfree();
    for (commodnum_t i = Num_commods; i >= 1; i--) {
      auto c = getcommod(i);
      if (!c.deliver) {
        c.deliver = true;
        putcommod(c, i);
        continue;
      }
      if (c.owner && c.bidder &&
          (races[c.bidder - 1].governor[c.bidder_gov].money >= c.bid)) {
        races[c.bidder - 1].governor[c.bidder_gov].money -= c.bid;
        races[c.owner - 1].governor[c.governor].money += c.bid;
        auto [cost, dist] = shipping_cost(c.star_to, c.star_from, c.bid);
        races[c.bidder - 1].governor[c.bidder_gov].cost_market += c.bid + cost;
        races[c.owner - 1].governor[c.governor].profit_market += c.bid;
        maintain(races[c.bidder - 1],
                 races[c.bidder - 1].governor[c.bidder_gov], cost);
        switch (c.type) {
          case CommodType::RESOURCE:
            planets[c.star_to][c.planet_to]->info[c.bidder - 1].resource +=
                c.amount;
            break;
          case CommodType::FUEL:
            planets[c.star_to][c.planet_to]->info[c.bidder - 1].fuel +=
                c.amount;
            break;
          case CommodType::DESTRUCT:
            planets[c.star_to][c.planet_to]->info[c.bidder - 1].destruct +=
                c.amount;
            break;
          case CommodType::CRYSTAL:
            planets[c.star_to][c.planet_to]->info[c.bidder - 1].crystals +=
                c.amount;
            break;
        }
        std::string purchased_msg = std::format(
            "Lot {} purchased from {} [{}] at a cost of {}.\n   {} {} "
            "arrived at /{}/{}\n",
            i, races[c.owner - 1].name, c.owner, c.bid, c.amount, c.type,
            stars[c.star_to].get_name(),
            stars[c.star_to].get_planet_name(c.planet_to));
        push_telegram(c.bidder, c.bidder_gov, purchased_msg);
        std::string sold_msg = std::format(
            "Lot {} ({} {}) sold to {} [{}] at a cost of {}.\n", i, c.amount,
            c.type, races[c.bidder - 1].name, c.bidder, c.bid);
        push_telegram(c.owner, c.governor, sold_msg);
        c.owner = c.governor = 0;
        c.bidder = c.bidder_gov = 0;
      } else {
        c.bidder = c.bidder_gov = 0;
        c.bid = 0;
      }
      if (!c.owner) makecommoddead(i);
      putcommod(c, i);
    }
  }
}

static void process_ship_masses_and_ownership() {
  /* check ship masses - ownership */
  for (shipnum_t i = 1; i <= Num_ships; i++)
    if (ships[i]->alive) {
      domass(*ships[i]);
      doown(*ships[i]);
    }
}

static void process_ship_turns(int update) {
  /* do all ships one turn - do slower ships first */
  for (int j = 0; j <= 9; j++)
    for (shipnum_t i = 1; i <= Num_ships; i++) {
      if (ships[i]->alive && ships[i]->speed == j) {
        doship(*ships[i], update);
        if ((ships[i]->type == ShipType::STYPE_MISSILE) &&
            !attack_planet(*ships[i]))
          domissile(*ships[i]);
      }
    }

  if (MARKET) {
    /* do maintenance costs */
    if (update)
      for (shipnum_t i = 1; i <= Num_ships; i++)
        if (ships[i]->alive && Shipdata[ships[i]->type][ABIL_MAINTAIN]) {
          if (ships[i]->popn)
            races[ships[i]->owner - 1].governor[ships[i]->governor].maintain +=
                ships[i]->build_cost;
          if (ships[i]->troops)
            races[ships[i]->owner - 1].governor[ships[i]->governor].maintain +=
                UPDATE_TROOP_COST * ships[i]->troops;
        }
  }
}

static void prepare_dead_ships() {
  /* prepare dead ships for recycling */
  clr_shipfree();
  for (shipnum_t i = 1; i <= Num_ships; i++)
    if (!ships[i]->alive) makeshipdead(i);
}

static void insert_ships_into_lists() {
  /* erase next ship pointers - reset in insert_sh_... */
  for (shipnum_t i = 1; i <= Num_ships; i++) {
    ships[i]->nextship = 0;
    ships[i]->ships = 0;
  }

  /* clear ship list for insertion */
  Sdata.ships = 0;
  for (starnum_t star = 0; star < Sdata.numstars; star++) {
    stars[star].ships() = 0;
    for (planetnum_t i = 0; i < stars[star].numplanets(); i++)
      planets[star][i]->ships = 0;
  }

  /* insert ship into the list of wherever it might be */
  for (shipnum_t i = Num_ships; i >= 1; i--) {
    if (ships[i]->alive) {
      switch (ships[i]->whatorbits) {
        case ScopeLevel::LEVEL_UNIV:
          insert_sh_univ(&Sdata, ships[i]);
          break;
        case ScopeLevel::LEVEL_STAR:
          insert_sh_star(stars[ships[i]->storbits], ships[i]);
          break;
        case ScopeLevel::LEVEL_PLAN:
          insert_sh_plan(*planets[ships[i]->storbits][ships[i]->pnumorbits],
                         ships[i]);
          break;
        case ScopeLevel::LEVEL_SHIP:
          insert_sh_ship(ships[i], ships[ships[i]->destshipno]);
          break;
      }
    }
  }
}

static void process_abms_and_missiles(int update) {
  /* put ABMs and surviving missiles here because ABMs need to have the missile
     in the shiplist of the target planet  Maarten */
  for (shipnum_t i = 1; i <= Num_ships; i++) /* ABMs defend planet */
    if ((ships[i]->type == ShipType::OTYPE_ABM) && ships[i]->alive)
      doabm(*ships[i]);

  for (shipnum_t i = 1; i <= Num_ships; i++)
    if ((ships[i]->type == ShipType::STYPE_MISSILE) && ships[i]->alive &&
        attack_planet(*ships[i]))
      domissile(*ships[i]);

  for (shipnum_t i = Num_ships; i >= 1; i--) putship(*ships[i]);

  for (starnum_t star = 0; star < Sdata.numstars; star++) {
    for (planetnum_t i = 0; i < stars[star].numplanets(); i++) {
      /* store occupation for VPs */
      for (player_t j = 1; j <= Num_races; j++) {
        if (planets[star][i]->info[j - 1].numsectsowned) {
          setbit(inhabited[star], j);
          setbit(stars[star].inhabited(), j);
        }
        if (planets[star][i]->type != PlanetType::ASTEROID &&
            (planets[star][i]->info[j - 1].numsectsowned >
             planets[star][i]->Maxx * planets[star][i]->Maxy / 2))
          races[j - 1].controlled_planets++;

        if (planets[star][i]->info[j - 1].numsectsowned)
          races[j - 1].planet_points += planets[star][i]->get_points();
      }
      if (update) {
        if (doplanet(star, *planets[star][i], i)) {
          /* save smap gotten & altered by doplanet
             only if the planet is expl*/
          // TODO(jeffbailey): Added this in doplanet, but need to audit other
          // getsmaps to make sure they have matching putsmaps
          // putsmap(smap, *planets[star][i]);
        }
      }
      putplanet(*planets[star][i], stars[star], i);
    }

    /* do AP's for ea. player  */
    if (update)
      for (player_t i = 1; i <= Num_races; i++) {
        if (starpopns[star][i - 1])
          setbit(stars[star].inhabited(), i);
        else
          clrbit(stars[star].inhabited(), i);

        if (isset(stars[star].inhabited(), i)) {
          ap_t APs;

          APs = stars[star].AP(i - 1) + APadd((int)starnumships[star][i - 1],
                                              starpopns[star][i - 1],
                                              races[i - 1]);
          if (APs < LIMIT_APs)
            stars[star].AP(i - 1) = APs;
          else
            stars[star].AP(i - 1) = LIMIT_APs;
        }
        /* compute victory points for the block */
        if (inhabited[star] != 0) {
          uint64_t dummy = Blocks[i - 1].invite & Blocks[i - 1].pledge;
          Blocks[i - 1].systems_owned += (inhabited[star] | dummy) == dummy;
        }
      }
    putstar(stars[star], star);
  }

  /* add APs to sdata for ea. player */
  if (update)
    for (player_t i = 1; i <= Num_races; i++) {
      Blocks[i - 1].systems_owned = 0; /*recount systems owned*/
      if (governed(races[i - 1])) {
        ap_t APs;

        APs = Sdata.AP[i - 1] + races[i - 1].planet_points;
        if (APs < LIMIT_APs)
          Sdata.AP[i - 1] = APs;
        else
          Sdata.AP[i - 1] = LIMIT_APs;
      }
    }

  putsdata(&Sdata);

  /* here is where we do victory calculations. */
}

static void update_victory_scores(int update) {
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

    for (player_t i = 1; i <= Num_races; i++) {
      victory[i - 1].morale = races[i - 1].morale;
      victory[i - 1].money = races[i - 1].governor[0].money;
      for (auto &governor : races[i - 1].governor)
        if (governor.active) victory[i - 1].money += governor.money;
    }

    for (starnum_t star = 0; star < Sdata.numstars; star++) {
      /* do planets in the star next */
      for (planetnum_t i = 0; i < stars[star].numplanets(); i++) {
        for (player_t j = 0; j < Num_races; j++) {
          if (!planets[star][i]->info[j].explored) continue;
          victory[j].numsects += (int)planets[star][i]->info[j].numsectsowned;
          victory[j].res += planets[star][i]->info[j].resource;
          victory[j].des += (int)planets[star][i]->info[j].destruct;
          victory[j].fuel += (int)planets[star][i]->info[j].fuel;
        }
      } /* end of planet searchings */
    } /* end of star searchings */

    for (shipnum_t i = 1; i <= Num_ships; i++) {
      if (!ships[i]->alive) continue;
      victory[ships[i]->owner - 1].shipcost += ships[i]->build_cost;
      victory[ships[i]->owner - 1].shiptech += ships[i]->tech;
      victory[ships[i]->owner - 1].res += ships[i]->resource;
      victory[ships[i]->owner - 1].des += ships[i]->destruct;
      victory[ships[i]->owner - 1].fuel += ships[i]->fuel;
    }
    /* now that we have the info.. calculate the raw score */

    for (player_t i = 0; i < Num_races; i++) {
      races[i].victory_score =
          (VICT_SECT * (int)victory[i].numsects) +
          (VICT_SHIP * ((int)victory[i].shipcost +
                        (VICT_TECH * (int)victory[i].shiptech))) +
          (VICT_RES * ((int)victory[i].res + (int)victory[i].des)) +
          (VICT_FUEL * (int)victory[i].fuel) +
          (VICT_MONEY * (int)victory[i].money);
      races[i].victory_score /= VICT_DIVISOR;
      races[i].victory_score = (int)(morale_factor((double)victory[i].morale) *
                                     races[i].victory_score);
    }
  } /* end of if (update) */
}

static void finalize_turn(int update) {
  for (shipnum_t i = 1; i <= Num_ships; i++) {
    putship(*ships[i]);
    free(ships[i]);
  }

  if (update) {
    for (player_t i = 1; i <= Num_races; i++) {
      /* collective intelligence */
      if (races[i - 1].collective_iq) {
        double x = ((2. / 3.14159265) *
                    atan((double)Power[i - 1].popn / MESO_POP_SCALE));
        races[i - 1].IQ = races[i - 1].IQ_limit * x * x;
      }
      races[i - 1].tech += (double)(races[i - 1].IQ) / 100.0;
      races[i - 1].morale += Power[i - 1].planets_owned;
      make_discoveries(races[i - 1]);
      races[i - 1].turn += 1;
      if (races[i - 1].controlled_planets >=
          Planet_count * VICTORY_PERCENT / 100)
        races[i - 1].victory_turns++;
      else
        races[i - 1].victory_turns = 0;

      if (races[i - 1].controlled_planets >=
          Planet_count * VICTORY_PERCENT / 200)
        for (player_t j = 1; j <= Num_races; j++)
          races[j - 1].translate[i - 1] = 100;

      Blocks[i - 1].VPs = 10L * Blocks[i - 1].systems_owned;
      if (MARKET) {
        for (auto &governor : races[i - 1].governor)
          if (governor.active)
            maintain(races[i - 1], governor, governor.maintain);
      }
    }
    for (player_t i = 1; i <= Num_races; i++) putrace(races[i - 1]);
  }

  free(ships);

  if (update) {
    compute_power_blocks();
    for (player_t i = 1; i <= Num_races; i++) {
      Power[i - 1].money = 0;
      for (auto &governor : races[i - 1].governor)
        if (governor.active) Power[i - 1].money += governor.money;
    }
    putpower(Power);
    Putblock(Blocks);
  }

  for (player_t j = 1; j <= Num_races; j++) {
    if (update)
      notify_race(j, "Finished with update.\n");
    else
      notify_race(j, "Finished with movement segment.\n");
  }
}

/* routine for number of AP's to add to each player in ea. system,scaled
    by amount of crew in their palace */

static ap_t APadd(const int sh, const population_t popn, const Race &race) {
  ap_t APs;

  APs = round_rand((double)sh / 10.0 + 5. * log10(1.0 + (double)popn));

  if (governed(race)) return APs;
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
static bool governed(const Race &race) {
  return (race.Gov_ship && race.Gov_ship <= Num_ships &&
          ships[race.Gov_ship] != nullptr && ships[race.Gov_ship]->alive &&
          ships[race.Gov_ship]->docked &&
          (ships[race.Gov_ship]->whatdest == ScopeLevel::LEVEL_PLAN ||
           (ships[race.Gov_ship]->whatorbits == ScopeLevel::LEVEL_SHIP &&
            ships[ships[race.Gov_ship]->destshipno]->type ==
                ShipType::STYPE_HABITAT &&
            (ships[ships[race.Gov_ship]->destshipno]->whatorbits ==
                 ScopeLevel::LEVEL_PLAN ||
             ships[ships[race.Gov_ship]->destshipno]->whatorbits ==
                 ScopeLevel::LEVEL_STAR))));
}

/* fix stability for stars */
void fix_stability(Star &s) {
  int a;
  int i;

  if (s.nova_stage() > 0) {
    if (s.nova_stage() > 14) {
      s.stability() = 20;
      s.nova_stage() = 0;
      sprintf(telegram_buf, "Notice\n");
      sprintf(buf, "\n  Scientists report that star %s\n",
              s.get_name().c_str());
      strcat(telegram_buf, buf);
      sprintf(buf, "is no longer undergoing nova.\n");
      strcat(telegram_buf, buf);
      for (i = 1; i <= Num_races; i++) push_telegram_race(i, telegram_buf);

      /* telegram everyone when nova over? */
    } else
      s.nova_stage()++;
  } else if (s.stability() > 20) {
    a = int_rand(-1, 3);
    /* nova just starting; notify everyone */
    if ((s.stability() + a) > 100) {
      s.stability() = 100;
      s.nova_stage() = 1;
      sprintf(telegram_buf, "***** BULLETIN! ******\n");
      sprintf(buf, "\n  Scientists report that star %s\n",
              s.get_name().c_str());
      strcat(telegram_buf, buf);
      sprintf(buf, "is undergoing nova.\n");
      strcat(telegram_buf, buf);
      for (i = 1; i <= Num_races; i++) push_telegram_race(i, telegram_buf);
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

void handle_victory() {
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
      sprintf(telegram_buf, "*** Attention ***");
      push_telegram_race(i, telegram_buf);
      sprintf(telegram_buf, "This game of Galactic Bloodshed is now *over*");
      push_telegram_race(i, telegram_buf);
      sprintf(telegram_buf, "The big winner%s",
              (game_over == 1) ? " is" : "s are");
      push_telegram_race(i, telegram_buf);
      for (j = 1; j <= Num_races; j++)
        if (win_category[j - 1] == BIG_WINNER) {
          sprintf(telegram_buf, "*** [%2d] %-30.30s ***", j, races[j - 1].name);
          push_telegram_race(i, telegram_buf);
        }
      sprintf(telegram_buf, "Lesser winners:");
      push_telegram_race(i, telegram_buf);
      for (j = 1; j <= Num_races; j++)
        if (win_category[j - 1] == LITTLE_WINNER) {
          sprintf(telegram_buf, "+++ [%2d] %-30.30s +++", j, races[j - 1].name);
          push_telegram_race(i, telegram_buf);
        }
    }
  }
}

static void make_discoveries(Race &r) {
  /* would be nicer to do this with a loop of course - but it's late */
  if (!Hyper_drive(r) && r.tech >= TECH_HYPER_DRIVE) {
    push_telegram_race(r.Playernum,
                       "You have discovered HYPERDRIVE technology.\n");
    r.discoveries[D_HYPER_DRIVE] = 1;
  }
  if (!Laser(r) && r.tech >= TECH_LASER) {
    push_telegram_race(r.Playernum, "You have discovered LASER technology.\n");
    r.discoveries[D_LASER] = 1;
  }
  if (!Cew(r) && r.tech >= TECH_CEW) {
    push_telegram_race(r.Playernum, "You have discovered CEW technology.\n");
    r.discoveries[D_CEW] = 1;
  }
  if (!Vn(r) && r.tech >= TECH_VN) {
    push_telegram_race(r.Playernum, "You have discovered VN technology.\n");
    r.discoveries[D_VN] = 1;
  }
  if (!Tractor_beam(r) && r.tech >= TECH_TRACTOR_BEAM) {
    push_telegram_race(r.Playernum,
                       "You have discovered TRACTOR BEAM technology.\n");
    r.discoveries[D_TRACTOR_BEAM] = 1;
  }
  if (!Transporter(r) && r.tech >= TECH_TRANSPORTER) {
    push_telegram_race(r.Playernum,
                       "You have discovered TRANSPORTER technology.\n");
    r.discoveries[D_TRANSPORTER] = 1;
  }
  if (!Avpm(r) && r.tech >= TECH_AVPM) {
    push_telegram_race(r.Playernum, "You have discovered AVPM technology.\n");
    r.discoveries[D_AVPM] = 1;
  }
  if (!Cloak(r) && r.tech >= TECH_CLOAK) {
    push_telegram_race(r.Playernum, "You have discovered CLOAK technology.\n");
    r.discoveries[D_CLOAK] = 1;
  }
  if (!Wormhole(r) && r.tech >= TECH_WORMHOLE) {
    push_telegram_race(r.Playernum,
                       "You have discovered WORMHOLE technology.\n");
    r.discoveries[D_WORMHOLE] = 1;
  }
  if (!Crystal(r) && r.tech >= TECH_CRYSTAL) {
    push_telegram_race(r.Playernum,
                       "You have discovered CRYSTAL technology.\n");
    r.discoveries[D_CRYSTAL] = 1;
  }
}

static bool attack_planet(const Ship &ship) {
  return ship.whatdest == ScopeLevel::LEVEL_PLAN;
}

static void output_ground_attacks() {
  int star;
  int i;
  int j;

  for (star = 0; star < Sdata.numstars; star++)
    for (i = 1; i <= Num_races; i++)
      for (j = 1; j <= Num_races; j++)
        if (ground_assaults[i - 1][j - 1][star]) {
          sprintf(buf, "%s: %s [%d] assaults %s [%d] %d times.\n",
                  stars[star].get_name().c_str(), races[i - 1].name, i,
                  races[j - 1].name, j, ground_assaults[i - 1][j - 1][star]);
          post(buf, NewsType::COMBAT);
          ground_assaults[i - 1][j - 1][star] = 0;
        }
}
