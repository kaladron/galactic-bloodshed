// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;
import tabulate;

#include "gb/csp.h"
#include "gb/csp_types.h"

module commands;

namespace {
constexpr int MAX_SHIPS_PER_SECTOR = 10;

const char* Tox[] = {"Stage 0, mild",
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

// Ship location data for CSP output
struct ShipLocInfo {
  int shipno;
  char ltr;
  unsigned char owner;
};

struct SectorShipData {
  int count;
  std::array<ShipLocInfo, MAX_SHIPS_PER_SECTOR> ships;
};

// Sector row data for rendering
struct SectorRowData {
  int x, y;
  const Sector* sector;
  char desshow_char;
  double compat;
  int toxic;
  const SectorShipData* ship_data;
};

// Abstract base class for survey output formatting
class SurveyFormatter {
public:
  virtual ~SurveyFormatter() = default;

  virtual void render_survey(std::ostream& out, const Planet& p,
                             const Star& star, std::string_view planet_name,
                             player_t player, const Race& race, bool all,
                             bool player_presence,
                             const std::vector<SectorRowData>& rows) = 0;

  virtual bool tracks_ships() const = 0;
};

// Human-readable formatter (for "survey" command)
class HumanFormatter : public SurveyFormatter {
public:
  void render_survey(std::ostream& out, const Planet& p, const Star& star,
                     std::string_view planet_name, player_t player,
                     const Race& race, bool all, bool player_presence,
                     const std::vector<SectorRowData>& rows) override {
    tabulate::Table table;
    table.format().hide_border().column_separator(" ");

    // Configure column widths and alignments
    table.column(0).format().width(6);  // x,y
    table.column(1).format().width(9);  // cond/type
    table.column(2).format().width(5).font_align(
        tabulate::FontAlign::right);  // owner
    table.column(3).format().width(4).font_align(
        tabulate::FontAlign::right);  // race
    table.column(4).format().width(3).font_align(
        tabulate::FontAlign::right);  // eff
    table.column(5).format().width(3).font_align(
        tabulate::FontAlign::right);  // mob
    table.column(6).format().width(3).font_align(
        tabulate::FontAlign::right);  // frt
    table.column(7).format().width(4).font_align(
        tabulate::FontAlign::right);  // res
    table.column(8).format().width(4).font_align(
        tabulate::FontAlign::right);  // mil
    table.column(9).format().width(4).font_align(
        tabulate::FontAlign::right);  // popn
    table.column(10).format().width(5).font_align(
        tabulate::FontAlign::right);  // ^popn
    table.column(11).format().width(5).font_align(
        tabulate::FontAlign::right);  // xtals

    // Add header row
    table.add_row({"x,y", "cond/type", "owner", "race", "eff", "mob", "frt",
                   "res", "mil", "popn", "^popn", "xtals"});
    table[0].format().font_style({tabulate::FontStyle::bold});

    // Add data rows
    for (const auto& row : rows) {
      const auto& s = *row.sector;
      if (row.desshow_char == CHAR_CLOAKED) {
        table.add_row({std::format("{},{}", row.x, row.y), "?  (    ?)", "", "",
                       "", "", "", "", "", "", "", ""});
      } else {
        std::string cond_type =
            std::format(" {}   {}", Dessymbols[s.get_condition()],
                        Dessymbols[s.get_type()]);
        std::string crystals =
            (s.get_crystals() && (race.discoveries[D_CRYSTAL] || race.God))
                ? "yes"
                : "";
        table.add_row(
            {std::format("{},{}", row.x, row.y), cond_type,
             std::format("{}", s.get_owner()), std::format("{}", s.get_race()),
             std::format("{}", s.get_eff()),
             std::format("{}", s.get_mobilization()),
             std::format("{}", s.get_fert()),
             std::format("{}", s.get_resource()),
             std::format("{}", s.get_troops()), std::format("{}", s.get_popn()),
             std::format("{}", maxsupport(race, s, row.compat, row.toxic)),
             crystals});
      }
    }

    out << table << "\n";
  }

  bool tracks_ships() const override {
    return false;
  }
};

// CSP (Client Server Protocol) formatter (for "client_survey" command)
class CspFormatter : public SurveyFormatter {
public:
  void render_survey(std::ostream& out, const Planet& p, const Star& star,
                     std::string_view planet_name, player_t player,
                     const Race& race, bool all, bool player_presence,
                     const std::vector<SectorRowData>& rows) override {
    // Write CSP header
    if (all) {
      out << std::format(
          "{} {} {} {} {} {} {} {} {} {} {} {} {:.2f} {}\n", CSP_CLIENT,
          CSP_SURVEY_INTRO, p.Maxx(), p.Maxy(), star.get_name(), planet_name,
          p.info(player - 1).resource, p.info(player - 1).fuel,
          p.info(player - 1).destruct, p.popn(), p.maxpopn(),
          p.conditions(TOXIC), p.compatibility(race), p.slaved_to());
    }

    // Write sector rows
    for (const auto& row : rows) {
      const auto& s = *row.sector;
      char sect_char = get_sector_char(s.get_condition());

      out << std::format(
          "{} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {}", CSP_CLIENT,
          CSP_SURVEY_SECTOR, row.x, row.y, sect_char, row.desshow_char,
          (s.is_wasted() ? 1 : 0), s.get_owner(), s.get_eff(), s.get_fert(),
          s.get_mobilization(),
          ((s.get_crystals() && (race.discoveries[D_CRYSTAL] || race.God)) ? 1
                                                                           : 0),
          s.get_resource(), s.get_popn(), s.get_troops(),
          maxsupport(race, s, row.compat, row.toxic));

      if (row.ship_data && row.ship_data->count > 0 && player_presence) {
        out << ";";
        for (int i = 0; i < row.ship_data->count; i++) {
          out << std::format(" {} {} {};", row.ship_data->ships[i].shipno,
                             row.ship_data->ships[i].ltr,
                             row.ship_data->ships[i].owner);
        }
      }
      out << "\n";
    }

    // Write CSP footer
    out << std::format("{} {}\n", CSP_CLIENT, CSP_SURVEY_END);
  }

  bool tracks_ships() const override {
    return true;
  }
};

std::string_view stability_label(int pct) {
  if (pct < 20) return "stable";
  if (pct < 40) return "unstable";
  if (pct < 60) return "dangerous";
  if (pct < 100) return "WARNING! Nova iminent!";
  return "undergoing nova";
}

// Helper: Parse survey arguments and determine location
std::optional<Place> parse_survey_location(const command_t& argv, GameObj& g,
                                           bool& all, std::string& range_arg) {
  all = false;
  range_arg.clear();

  if (argv.size() == 1) {
    // No args - use current scope
    return Place(g.level, g.snum, g.pnum);
  }

  // Parse argument to determine survey type
  if (argv[1].empty()) {
    return Place(g.level, g.snum, g.pnum);
  }

  if ((std::isdigit(argv[1][0]) && argv[1].contains(',')) ||
      ((argv[1][0] == '-') && (all = true))) {
    // Sector range or full survey
    if (g.level != ScopeLevel::LEVEL_PLAN) {
      g.out << "There are no sectors here.\n";
      return std::nullopt;
    }
    if (!all) {
      range_arg = argv[1];
    }
    return Place(ScopeLevel::LEVEL_PLAN, g.snum, g.pnum);
  }

  // Survey a named location
  Place where(g, argv[1]);
  if (where.err || where.level == ScopeLevel::LEVEL_SHIP) {
    return std::nullopt;
  }
  return where;
}

// Helper: Survey planet sectors (detailed sector-by-sector view)
void survey_planet_sectors(GameObj& g, const Place& where,
                           const std::string& range_arg, bool all,
                           SurveyFormatter& formatter) {
  const auto& race = *g.race;

  const auto* star_ptr = g.entity_manager.peek_star(where.snum);
  if (!star_ptr) {
    g.out << "Star not found.\n";
    return;
  }
  const auto& star = *star_ptr;

  const auto* p_ptr = g.entity_manager.peek_planet(where.snum, where.pnum);
  if (!p_ptr) {
    g.out << "Planet not found.\n";
    return;
  }
  const auto& p = *p_ptr;

  double compat = p.compatibility(race);
  const auto* smap = g.entity_manager.peek_sectormap(where.snum, where.pnum);
  if (!smap) {
    g.out << "Sector map not found.\n";
    return;
  }

  // Determine sector range
  int lowx, hix, lowy, hiy;
  if (!all) {
    int x2;
    get4args(range_arg.c_str(), &x2, &hix, &lowy, &hiy);
    lowx = std::max(0, x2);
    hix = std::min(hix, p.Maxx() - 1);
    lowy = std::max(0, lowy);
    hiy = std::min(hiy, p.Maxy() - 1);
  } else {
    lowx = 0;
    hix = p.Maxx() - 1;
    lowy = 0;
    hiy = p.Maxy() - 1;
  }

  // Build ship location data if needed
  std::vector<std::vector<SectorShipData>> shiplocs(
      p.Maxx(), std::vector<SectorShipData>(p.Maxy()));
  bool inhere = false;  // Track if player has presence on planet
  if (formatter.tracks_ships()) {
    inhere = p.info(g.player - 1).numsectsowned > 0;
    const ShipList kShips(g.entity_manager, p.ships());
    for (const Ship* shipa : kShips) {
      if (shipa->owner() == g.player &&
          (shipa->popn() || (shipa->type() == ShipType::OTYPE_PROBE))) {
        inhere = true;
      }
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

  // Accumulate sector row data
  std::vector<SectorRowData> rows;
  for (const auto& s : *smap) {
    int x = s.get_x();
    int y = s.get_y();
    if (x < lowx || x > hix || y < lowy || y > hiy) continue;

    const SectorShipData* ship_data =
        (shiplocs[x][y].count > 0) ? &shiplocs[x][y] : nullptr;
    rows.push_back({.x = x,
                    .y = y,
                    .sector = &s,
                    .desshow_char = desshow(g.player, g.governor, race, s),
                    .compat = compat,
                    .toxic = p.conditions(TOXIC),
                    .ship_data = ship_data});
  }

  // Render the complete survey
  formatter.render_survey(g.out, p, star, star.get_planet_name(where.pnum),
                          g.player, race, all, inhere, rows);
}

// Helper: Survey planet overview (conditions, stats, etc.)
void survey_planet_overview(GameObj& g, const Place& where) {
  const auto& race = *g.race;

  const auto* star_ptr = g.entity_manager.peek_star(where.snum);
  if (!star_ptr) {
    g.out << "Star not found.\n";
    return;
  }
  const auto& star = *star_ptr;

  const auto* p_ptr = g.entity_manager.peek_planet(where.snum, where.pnum);
  if (!p_ptr) {
    g.out << "Planet not found.\n";
    return;
  }
  const auto& p = *p_ptr;

  g.out << std::format("{}:\n", star.get_planet_name(where.pnum));
  g.out << std::format("gravity   x,y absolute     x,y relative to {}\n",
                       star.get_name());
  g.out << std::format("{:7.2f}   {:7.1f},{:7.1f}   {:8.1f},{:8.1f}\n",
                       p.gravity(), p.xpos() + star.xpos(),
                       p.ypos() + star.ypos(), p.xpos(), p.ypos());
  g.out << "======== Planetary conditions: ========\n";
  g.out << "atmosphere concentrations:\n";
  g.out << std::format(
      "     methane {:02d}%({:02d}%)     oxygen {:02d}%({:02d}%)\n",
      p.conditions(METHANE), race.conditions[METHANE], p.conditions(OXYGEN),
      race.conditions[OXYGEN]);
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
      p.conditions(HELIUM), race.conditions[HELIUM], p.conditions(OTHER),
      race.conditions[OTHER]);

  int tindex = p.conditions(TOXIC) / 10;
  if (tindex < 0) {
    tindex = 0;
  } else if (tindex > 10) {
    tindex = 11;
  }
  g.out << std::format("                     Toxicity: {}% ({})\n",
                       p.conditions(TOXIC), Tox[tindex]);
  g.out << std::format("Total planetary compatibility: {:.2f}%\n",
                       p.compatibility(race));

  const auto* smap = g.entity_manager.peek_sectormap(where.snum, where.pnum);
  if (!smap) {
    g.out << "Sector map not found.\n";
    return;
  }

  int crystal_count = 0;
  int avg_fert = 0;
  int avg_resource = 0;
  for (const auto& s : *smap) {
    avg_fert += s.get_fert();
    avg_resource += s.get_resource();
    if (race.discoveries[D_CRYSTAL] || race.God) {
      crystal_count += !!s.get_crystals();
    }
  }

  g.out << std::format("{:>29}: {}\n{:>29}: {}\n{:>29}: {}\n",
                       "Average fertility", avg_fert / (p.Maxx() * p.Maxy()),
                       "Average resource", avg_resource / (p.Maxx() * p.Maxy()),
                       "Crystal sectors", crystal_count);
  g.out << std::format("{:>29}: {}\n", "Total resource deposits",
                       p.total_resources());
  g.out << std::format("fuel_stock  resource_stock dest_pot.   {}    ^{}\n",
                       race.Metamorph ? "biomass" : "popltn",
                       race.Metamorph ? "biomass" : "popltn");
  g.out << std::format("{:10}  {:14} {:9}  {:7}{:11}\n",
                       p.info(g.player - 1).fuel, p.info(g.player - 1).resource,
                       p.info(g.player - 1).destruct, p.popn(), p.maxpopn());
  if (p.slaved_to()) {
    g.out << std::format("This planet ENSLAVED to player {}!\n", p.slaved_to());
  }
}

// Helper: Survey star system
void survey_star(GameObj& g, const Place& where) {
  const auto& race = *g.race;

  const auto* star_ptr = g.entity_manager.peek_star(where.snum);
  if (!star_ptr) {
    g.out << "Star not found.\n";
    return;
  }
  const auto& star = *star_ptr;

  g.out << std::format("Star {}\n", star.get_name());
  g.out << std::format("locn: {},{}\n", star.xpos(), star.ypos());

  if (race.God) {
    for (int i = 0; i < star.numplanets(); i++) {
      g.out << std::format(" \"{}\"\n", star.get_planet_name(i));
    }
  }

  g.out << std::format("Gravity: {:.2f}\tInstability: ", star.gravity());

  if (race.tech >= TECH_SEE_STABILITY || race.God) {
    g.out << std::format("{}% ({})\n", star.stability(),
                         stability_label(star.stability()));
  } else {
    g.out << "(cannot determine)\n";
  }

  g.out << std::format("temperature class (1->10) {}\n", star.temperature());
  g.out << std::format("{} planets are ", star.numplanets());
  for (int i = 0; i < star.numplanets(); i++) {
    g.out << std::format("{} ", star.get_planet_name(i));
  }
  g.out << "\n";
}

}  // namespace

namespace GB::commands {

void survey(const command_t& argv, GameObj& g) {
  // Create appropriate formatter based on command name
  auto formatter = [&argv]() -> std::unique_ptr<SurveyFormatter> {
    if (argv[0] == "survey") {
      return std::make_unique<HumanFormatter>();
    }
    return std::make_unique<CspFormatter>();
  }();

  // Parse arguments and determine what to survey
  bool all = false;
  std::string range_arg;
  auto where_opt = parse_survey_location(argv, g, all, range_arg);
  if (!where_opt) return;

  const auto& where = *where_opt;

  // Dispatch based on scope level
  switch (where.level) {
    case ScopeLevel::LEVEL_PLAN:
      // Check if this is a sector survey or planet overview
      if ((argv.size() > 1 && std::isdigit(argv[1][0]) &&
           argv[1].contains(',')) ||
          all) {
        survey_planet_sectors(g, where, range_arg, all, *formatter);
      } else {
        survey_planet_overview(g, where);
      }
      break;

    case ScopeLevel::LEVEL_STAR:
      survey_star(g, where);
      break;

    case ScopeLevel::LEVEL_UNIV:
      g.out << "It's just _there_, you know?\n";
      break;

    default:
      g.out << "Illegal scope.\n";
      break;
  }
}
}  // namespace GB::commands