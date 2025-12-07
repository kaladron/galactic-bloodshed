// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* survey.c -- print out survey for planets */

module;

import gblib;
import std;

#include <ctype.h>
#include <strings.h>

#include "gb/csp.h"
#include "gb/csp_types.h"

module commands;

constexpr int MAX_SHIPS_PER_SECTOR = 10;

static const char* Tox[] = {
    "Stage 0, mild",
    "Stage 1, mild",
    "Stage 2, semi-mild",
    "Stage 3, semi-semi mild",
    "Stage 4, ecologically unsound",
    "Stage 5: ecologically unsound",
    "Stage 6: below birth threshold",
    "Stage 7: ecologically unstable--below birth threshold",
    "Stage 8: ecologically poisonous --below birth threshold",
    "Stage 9: WARNING: nearing 100% toxicity",
    "Stage 10: WARNING: COMPLETELY TOXIC!!!",
    "???"};

namespace GB::commands {

// Ship location data for CSP output
struct ShipLocInfo {
  int shipno;
  char ltr;
  unsigned char owner;
};

struct SectorShipData {
  int count;
  ShipLocInfo ships[MAX_SHIPS_PER_SECTOR];
};

// Abstract base class for survey output formatting
class SurveyFormatter {
public:
  virtual ~SurveyFormatter() = default;

  virtual void write_header(std::ostream& out, const Planet& p,
                            const Star& star, std::string_view planet_name,
                            player_t player, const Race& race, bool all,
                            int player_presence) = 0;

  virtual void write_sector(std::ostream& out, int x, int y, const Sector& s,
                            const Race& race, player_t player, governor_t gov,
                            double compat, int toxic,
                            const SectorShipData* ship_data,
                            int player_presence) = 0;

  virtual void write_footer(std::ostream& out) = 0;

  virtual bool tracks_ships() const = 0;
};

// Human-readable formatter (for "survey" command)
class HumanFormatter : public SurveyFormatter {
public:
  void write_header(std::ostream& out, const Planet& p, const Star& star,
                    std::string_view planet_name, player_t player,
                    const Race& race, bool all, int player_presence) override {
    out << " x,y cond/type  owner race eff mob frt  res  mil popn ^popn "
           "xtals\n";
  }

  void write_sector(std::ostream& out, int x, int y, const Sector& s,
                    const Race& race, player_t player, governor_t gov,
                    double compat, int toxic, const SectorShipData* ship_data,
                    int player_presence) override {
    out << std::format("{:2d},{:<2d} ", x, y);
    char d = desshow(player, gov, race, s);
    if (d == CHAR_CLOAKED) {
      out << "?  (    ?    )\n";
    } else {
      out << std::format(
          " {}   {}   {:6}{:5}{:4}{:4}{:4}{:5}{:5}{:5}{:6}{}\n",
          Dessymbols[s.get_condition()], Dessymbols[s.get_type()],
          s.get_owner(), s.get_race(), s.get_eff(), s.get_mobilization(),
          s.get_fert(), s.get_resource(), s.get_troops(), s.get_popn(),
          maxsupport(race, s, compat, toxic),
          ((s.get_crystals() && (race.discoveries[D_CRYSTAL] || race.God))
               ? " yes"
               : " "));
    }
  }

  void write_footer(std::ostream& out) override {
    // Human format has no footer
  }

  bool tracks_ships() const override {
    return false;
  }
};

// CSP (Client Server Protocol) formatter (for "client_survey" command)
class CspFormatter : public SurveyFormatter {
public:
  void write_header(std::ostream& out, const Planet& p, const Star& star,
                    std::string_view planet_name, player_t player,
                    const Race& race, bool all, int player_presence) override {
    if (all) {
      out << std::format(
          "{} {} {} {} {} {} {} {} {} {} {} {} {:.2f} {}\n", CSP_CLIENT,
          CSP_SURVEY_INTRO, p.Maxx(), p.Maxy(), star.get_name(), planet_name,
          p.info(player - 1).resource, p.info(player - 1).fuel,
          p.info(player - 1).destruct, p.popn(), p.maxpopn(),
          p.conditions(TOXIC), p.compatibility(race), p.slaved_to());
    }
  }

  void write_sector(std::ostream& out, int x, int y, const Sector& s,
                    const Race& race, player_t player, governor_t gov,
                    double compat, int toxic, const SectorShipData* ship_data,
                    int player_presence) override {
    char sect_char;
    switch (s.get_condition()) {
      case SectorType::SEC_SEA:
        sect_char = CHAR_SEA;
        break;
      case SectorType::SEC_LAND:
        sect_char = CHAR_LAND;
        break;
      case SectorType::SEC_MOUNT:
        sect_char = CHAR_MOUNT;
        break;
      case SectorType::SEC_GAS:
        sect_char = CHAR_GAS;
        break;
      case SectorType::SEC_PLATED:
        sect_char = CHAR_PLATED;
        break;
      case SectorType::SEC_ICE:
        sect_char = CHAR_ICE;
        break;
      case SectorType::SEC_DESERT:
        sect_char = CHAR_DESERT;
        break;
      case SectorType::SEC_FOREST:
        sect_char = CHAR_FOREST;
        break;
      default:
        sect_char = '?';
        break;
    }

    out << std::format(
        "{} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {}", CSP_CLIENT,
        CSP_SURVEY_SECTOR, x, y, sect_char, desshow(player, gov, race, s),
        (s.is_wasted() ? 1 : 0), s.get_owner(), s.get_eff(), s.get_fert(),
        s.get_mobilization(),
        ((s.get_crystals() && (race.discoveries[D_CRYSTAL] || race.God)) ? 1
                                                                         : 0),
        s.get_resource(), s.get_popn(), s.get_troops(),
        maxsupport(race, s, compat, toxic));

    if (ship_data && ship_data->count > 0 && player_presence) {
      out << ";";
      for (int i = 0; i < ship_data->count; i++) {
        out << std::format(" {} {} {};", ship_data->ships[i].shipno,
                           ship_data->ships[i].ltr, ship_data->ships[i].owner);
      }
    }
    out << "\n";
  }

  void write_footer(std::ostream& out) override {
    out << std::format("{} {}\n", CSP_CLIENT, CSP_SURVEY_END);
  }

  bool tracks_ships() const override {
    return true;
  }
};

static std::string_view stability_label(int pct) {
  if (pct < 20) return "stable";
  if (pct < 40) return "unstable";
  if (pct < 60) return "dangerous";
  if (pct < 100) return "WARNING! Nova iminent!";
  return "undergoing nova";
}
void survey(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int lowx;
  int hix;
  int lowy;
  int hiy;
  int x2;
  int tindex;
  double compat;
  int avg_fert;
  int avg_resource;
  int crystal_count;
  int all = 0; /* full survey 1, specific 0 */
  SectorShipData shiplocs[MAX_X][MAX_Y]{};
  int i;

  // Create appropriate formatter based on command name
  auto formatter = [&argv]() -> std::unique_ptr<SurveyFormatter> {
    if (argv[0] == "survey") {
      return std::make_unique<HumanFormatter>();
    } else {
      return std::make_unique<CspFormatter>();
    }
  }();

  std::unique_ptr<Place> where;
  if (argv.size() == 1) { /* no args */
    where = std::make_unique<Place>(g.level, g.snum, g.pnum);
  } else {
    /* they are surveying a sector */
    if ((isdigit(argv[1][0]) && index(argv[1].c_str(), ',') != nullptr) ||
        ((argv[1][0] == '-') && (all = 1))) {
      if (g.level != ScopeLevel::LEVEL_PLAN) {
        g.out << "There are no sectors here.\n";
        return;
      }
      where = std::make_unique<Place>(ScopeLevel::LEVEL_PLAN, g.snum, g.pnum);

    } else {
      where = std::make_unique<Place>(g, argv[1]);
      if (where->err || where->level == ScopeLevel::LEVEL_SHIP) return;
    }
  }

  // Use g.race for read-only access to current player's race
  auto& race = *g.race;

  if (where->level == ScopeLevel::LEVEL_PLAN) {
    const auto* star_ptr = g.entity_manager.peek_star(where->snum);
    if (!star_ptr) {
      g.out << "Star not found.\n";
      return;
    }
    const auto& star = *star_ptr;

    const auto* p_ptr = g.entity_manager.peek_planet(where->snum, where->pnum);
    if (!p_ptr) {
      g.out << "Planet not found.\n";
      return;
    }
    const auto& p = *p_ptr;

    compat = p.compatibility(race);

    if ((argv.size() > 1 && isdigit(argv[1][0]) &&
         index(argv[1].c_str(), ',') != nullptr) ||
        all) {
      auto smap = getsmap(p);

      if (!all) {
        get4args(argv[1].c_str(), &x2, &hix, &lowy, &hiy);
        /* ^^^ translate from lowx:hix,lowy:hiy */
        x2 = std::max(0, x2);
        hix = std::min(hix, p.Maxx() - 1);
        lowy = std::max(0, lowy);
        hiy = std::min(hiy, p.Maxy() - 1);
      } else {
        x2 = 0;
        hix = p.Maxx() - 1;
        lowy = 0;
        hiy = p.Maxy() - 1;
      }

      int inhere = 0;  // Track if player has presence on planet
      if (formatter->tracks_ships()) {
        inhere = p.info(Playernum - 1).numsectsowned;
        const ShipList kShips(g.entity_manager, p.ships());
        for (const Ship* shipa : kShips) {
          if (shipa->owner() == Playernum &&
              (shipa->popn() || (shipa->type() == ShipType::OTYPE_PROBE)))
            inhere = 1;
          if (shipa->alive() && landed(*shipa) &&
              shiplocs[shipa->land_x()][shipa->land_y()].count <
                  MAX_SHIPS_PER_SECTOR) {
            auto& loc = shiplocs[shipa->land_x()][shipa->land_y()];
            loc.ships[loc.count].shipno = shipa->number();
            loc.ships[loc.count].owner = shipa->owner();
            loc.ships[loc.count].ltr = Shipltrs[shipa->type()];
            loc.count++;
          }
        }
      }

      formatter->write_header(g.out, p, star, star.get_planet_name(where->pnum),
                              Playernum, race, all, inhere);

      for (; lowy <= hiy; lowy++)
        for (lowx = x2; lowx <= hix; lowx++) {
          auto& s = smap.get(lowx, lowy);
          const SectorShipData* ship_data = (shiplocs[lowx][lowy].count > 0)
                                                ? &shiplocs[lowx][lowy]
                                                : nullptr;
          formatter->write_sector(g.out, lowx, lowy, s, race, Playernum,
                                  Governor, compat, p.conditions(TOXIC),
                                  ship_data, inhere);
        }

      formatter->write_footer(g.out);
    } else {
      /* survey of planet */
      g.out << std::format("{}:\n",
                           star.get_planet_name(where->pnum));
      g.out << std::format("gravity   x,y absolute     x,y relative to {}\n",
                           star.get_name());
      g.out << std::format("{:7.2f}   {:7.1f},{:7.1f}   {:8.1f},{:8.1f}\n",
                           p.gravity(), p.xpos() + star.xpos(),
                           p.ypos() + star.ypos(), p.xpos(),
                           p.ypos());
      g.out << "======== Planetary conditions: ========\n";
      g.out << "atmosphere concentrations:\n";
      g.out << std::format(
          "     methane {:02d}%({:02d}%)     oxygen {:02d}%({:02d}%)\n",
          p.conditions(METHANE), race.conditions[METHANE],
          p.conditions(OXYGEN), race.conditions[OXYGEN]);
      g.out << std::format("         CO2 {:02d}%({:02d}%)   hydrogen "
                           "{:02d}%({:02d}%)      temperature: {:3d} ({:3d})\n",
                           p.conditions(CO2), race.conditions[CO2],
                           p.conditions(HYDROGEN), race.conditions[HYDROGEN],
                           p.conditions(TEMP), race.conditions[TEMP]);
      g.out << std::format("    nitrogen {:02d}%({:02d}%)     sulfur "
                           "{:02d}%({:02d}%)           normal: {:3d}\n",
                           p.conditions(NITROGEN), race.conditions[NITROGEN],
                           p.conditions(SULFUR), race.conditions[SULFUR],
                           p.conditions(RTEMP));
      g.out << std::format(
          "      helium {:02d}%({:02d}%)      other {:02d}%({:02d}%)\n",
          p.conditions(HELIUM), race.conditions[HELIUM],
          p.conditions(OTHER), race.conditions[OTHER]);
      if ((tindex = p.conditions(TOXIC) / 10) < 0)
        tindex = 0;
      else if (tindex > 10)
        tindex = 11;
      g.out << std::format("                     Toxicity: {}% ({})\n",
                           p.conditions(TOXIC), Tox[tindex]);
      g.out << std::format("Total planetary compatibility: {:.2f}%\n",
                           p.compatibility(race));

      auto smap = getsmap(p);

      crystal_count = avg_fert = avg_resource = 0;
      for (lowx = 0; lowx < p.Maxx(); lowx++)
        for (lowy = 0; lowy < p.Maxy(); lowy++) {
          auto& s = smap.get(lowx, lowy);
          avg_fert += s.get_fert();
          avg_resource += s.get_resource();
          if (race.discoveries[D_CRYSTAL] || race.God)
            crystal_count += !!s.get_crystals();
        }
      g.out << std::format("{:>29}: {}\n{:>29}: {}\n{:>29}: {}\n",
                           "Average fertility", avg_fert / (p.Maxx() * p.Maxy()),
                           "Average resource",
                           avg_resource / (p.Maxx() * p.Maxy()),
                           "Crystal sectors", crystal_count);
      g.out << std::format("{:>29}: {}\n", "Total resource deposits",
                           p.total_resources());
      g.out << std::format("fuel_stock  resource_stock dest_pot.   {}    ^{}\n",
                           race.Metamorph ? "biomass" : "popltn",
                           race.Metamorph ? "biomass" : "popltn");
      g.out << std::format(
          "{:10}  {:14} {:9}  {:7}{:11}\n", p.info(Playernum - 1).fuel,
          p.info(Playernum - 1).resource, p.info(Playernum - 1).destruct,
          p.popn(), p.maxpopn());
      if (p.slaved_to()) {
        g.out << std::format("This planet ENSLAVED to player {}!\n", p.slaved_to());
      }
    }
  } else if (where->level == ScopeLevel::LEVEL_STAR) {
    const auto* star_ptr = g.entity_manager.peek_star(where->snum);
    if (!star_ptr) {
      g.out << "Star not found.\n";
      return;
    }
    const auto& star = *star_ptr;

    g.out << std::format("Star {}\n", star.get_name());
    g.out << std::format("locn: {},{}\n", star.xpos(),
                         star.ypos());
    if (race.God) {
      for (i = 0; i < star.numplanets(); i++) {
        g.out << std::format(" \"{}\"\n", star.get_planet_name(i));
      }
    }
    g.out << std::format("Gravity: {:.2f}\tInstability: ",
                         star.gravity());

    if (race.tech >= TECH_SEE_STABILITY || race.God) {
      g.out << std::format("{}% ({})\n", star.stability(),
                           stability_label(star.stability()));
    } else {
      g.out << "(cannot determine)\n";
    }
    g.out << std::format("temperature class (1->10) {}\n",
                         star.temperature());
    g.out << std::format("{} planets are ", star.numplanets());
    for (x2 = 0; x2 < star.numplanets(); x2++) {
      g.out << std::format("{} ", star.get_planet_name(x2));
    }
    g.out << "\n";
  } else if (where->level == ScopeLevel::LEVEL_UNIV) {
    g.out << "It's just _there_, you know?\n";
  } else {
    g.out << "Illegal scope.\n";
  }
} /* end survey */
}  // namespace GB::commands