// SPDX-License-Identifier: Apache-2.0

/// \file analysis.cc
/// \brief Statistical analysis of top/bottom sector categories on planets.

module;

import gb.entities;
import gb.presentation;
import gb.services;
import std;

module commands;

static constexpr int CARE = 5;

namespace {

using GB::presentation::AnalysisPlanetReport;
using GB::presentation::AnalysisSectorEntry;
using GB::presentation::AnalysisStatsRow;
using GB::presentation::AnalysisViewModel;

struct AnalSect {
  Coordinates coords{};
  SectorType des{SectorType::SEC_SEA};
  resource_t value = -1;  // -1 means not set
};

enum class Mode {
  TopFive,
  BottomFive
};

enum class PlayerFilterMode {
  AllPlayers,
  Unoccupied,
  SpecificPlayer
};

struct PlayerFilter {
  PlayerFilterMode mode;
  player_t player{0};

  static PlayerFilter all_players() {
    return {.mode = PlayerFilterMode::AllPlayers};
  }

  static PlayerFilter unoccupied() {
    return {.mode = PlayerFilterMode::Unoccupied};
  }

  static PlayerFilter specific(player_t p) {
    return {.mode = PlayerFilterMode::SpecificPlayer, .player = p};
  }

  [[nodiscard]] bool matches(player_t owner) const {
    switch (mode) {
      case PlayerFilterMode::AllPlayers:
        return true;
      case PlayerFilterMode::Unoccupied:
        return owner.value == 0;
      case PlayerFilterMode::SpecificPlayer:
        return owner == player;
    }
    return false;
  }

  [[nodiscard]] std::string describe() const {
    switch (mode) {
      case PlayerFilterMode::AllPlayers:
        return " sectors.\n";
      case PlayerFilterMode::Unoccupied:
        return " sectors that are unoccupied.\n";
      case PlayerFilterMode::SpecificPlayer:
        return std::format(" sectors owned by {}.\n", player.value);
    }
    return " sectors.\n";
  }
};

void insert(Mode mode, std::array<struct AnalSect, CARE>& arr, AnalSect in) {
  auto* insertion_point = std::ranges::find_if(arr, [&](const auto& elem) {
    if (mode == Mode::TopFive) {
      return elem.value < in.value;
    }
    return elem.value == -1 || elem.value > in.value;
  });

  if (insertion_point != arr.end()) {
    std::shift_right(insertion_point, arr.end(), 1);
    *insertion_point = in;
  }
}

std::vector<AnalysisSectorEntry>
to_entries(const std::array<AnalSect, CARE>& arr) {
  std::vector<AnalysisSectorEntry> out;
  for (const auto& as : arr) {
    if (as.value == -1) continue;
    out.push_back(AnalysisSectorEntry{
        .coords = as.coords,
        .sector_char = get_sector_char(as.des),
        .value = as.value,
    });
  }
  return out;
}

struct PlayerSectorStats {
  int crys = 0;
  int troops = 0;
  int popn = 0;
  int mob = 0;
  int eff = 0;
  int res = 0;
  int t_sect = 0;
  int wasted_sect = 0;
  std::array<int, SectorType::SEC_WASTED + 1> sect{};
};

struct SectorStats {
  int total_crys = 0;
  int total_troops = 0;
  int total_popn = 0;
  int total_mob = 0;
  int total_eff = 0;
  int total_res = 0;
  std::array<int, SectorType::SEC_WASTED + 1> sect{};
  PlayerSectorStats unowned{};
  std::flat_map<player_t, PlayerSectorStats> players{};
};

SectorStats accumulate_statistics(GameObj& g, const SectorMap& smap) {
  SectorStats stats;

  for (const auto& sect : smap) {
    player_t p = sect.get_owner();
    PlayerSectorStats& ps = (p == 0) ? stats.unowned : stats.players[p];

    ps.eff += sect.get_eff();
    ps.mob += sect.get_mobilization();
    ps.res += sect.get_resource();
    ps.popn += sect.get_popn();
    ps.troops += sect.get_troops();
    ps.sect[sect.get_condition()]++;
    ps.t_sect++;
    stats.total_eff += sect.get_eff();
    stats.total_mob += sect.get_mobilization();
    stats.total_res += sect.get_resource();
    stats.total_popn += sect.get_popn();
    stats.total_troops += sect.get_troops();
    stats.sect[sect.get_condition()]++;

    if (sect.is_wasted()) {
      ps.wasted_sect++;
    }
    if (sect.get_crystals() && g.race->tech >= TECH_CRYSTAL) {
      ps.crys++;
      stats.total_crys++;
    }
  }

  return stats;
}

struct TopSectorLists {
  std::array<struct AnalSect, CARE> res;
  std::array<struct AnalSect, CARE> eff;
  std::array<struct AnalSect, CARE> frt;
  std::array<struct AnalSect, CARE> mob;
  std::array<struct AnalSect, CARE> troops;
  std::array<struct AnalSect, CARE> popn;
  std::array<struct AnalSect, CARE> m_popn;
};

TopSectorLists find_top_sectors(GameObj& g, const SectorMap& smap,
                                const Planet& planet, Mode mode,
                                const PlayerFilter& filter,
                                std::optional<SectorType> sector_type) {
  TopSectorLists tops;

  for (const auto& sect : smap) {
    if (!sector_type || *sector_type == sect.get_condition()) {
      if (filter.matches(sect.get_owner())) {
        const auto coords = sect.coords();
        const auto cond = sect.get_condition();
        insert(mode, tops.res,
               {.coords = coords, .des = cond, .value = sect.get_resource()});
        insert(mode, tops.eff,
               {.coords = coords, .des = cond, .value = sect.get_eff()});
        insert(
            mode, tops.mob,
            {.coords = coords, .des = cond, .value = sect.get_mobilization()});
        insert(mode, tops.frt,
               {.coords = coords, .des = cond, .value = sect.get_fert()});
        insert(mode, tops.popn,
               {.coords = coords, .des = cond, .value = sect.get_popn()});
        insert(mode, tops.troops,
               {.coords = coords, .des = cond, .value = sect.get_troops()});
        insert(
            mode, tops.m_popn,
            {.coords = coords,
             .des = cond,
             .value = maxsupport(*g.race, sect, planet.compatibility(*g.race),
                                 planet.toxic())});
      }
    }
  }

  return tops;
}

std::optional<std::string>
format_sector_type_name(std::optional<SectorType> sector_type) {
  if (!sector_type.has_value()) {
    return std::nullopt;
  }
  switch (*sector_type) {
    case SectorType::SEC_SEA:
      return "Ocean";
    case SectorType::SEC_LAND:
      return "Land";
    case SectorType::SEC_MOUNT:
      return "Mountain";
    case SectorType::SEC_GAS:
      return "Gas";
    case SectorType::SEC_ICE:
      return "Ice";
    case SectorType::SEC_FOREST:
      return "Forest";
    case SectorType::SEC_DESERT:
      return "Desert";
    case SectorType::SEC_PLATED:
      return "Plated";
    case SectorType::SEC_WASTED:
      return "Wasted";
  }
  return std::nullopt;
}

void do_analysis(AnalysisViewModel& vm, GameObj& g, const PlayerFilter& filter,
                 Mode mode, std::optional<SectorType> sector_type,
                 starnum_t Starnum, planetnum_t Planetnum) {
  g.entity_manager.with_planet(Starnum, Planetnum, [&](const Planet& planet) {
    if (!planet.info(g.player()).explored) {
      return;
    }

    auto total_sect = planet.num_sectors();

    g.entity_manager.with_sectormap(
        Starnum, Planetnum, [&](const SectorMap& smap) {
          auto stats = accumulate_statistics(g, smap);
          auto tops =
              find_top_sectors(g, smap, planet, mode, filter, sector_type);

          AnalysisPlanetReport report{
              .is_top_five = (mode == Mode::TopFive),
              .care_count = CARE,
              .sector_type_name = format_sector_type_name(sector_type),
              .filter_description = filter.describe(),
              .troops = to_entries(tops.troops),
              .res = to_entries(tops.res),
              .eff = to_entries(tops.eff),
              .frt = to_entries(tops.frt),
              .mob = to_entries(tops.mob),
              .popn = to_entries(tops.popn),
              .m_popn = to_entries(tops.m_popn),
          };
          g.entity_manager.with_star(Starnum, [&](const Star& star) {
            report.star_name = star.get_name();
            report.planet_name = star.get_planet_name(Planetnum);
          });

          auto make_row = [](std::string label, const PlayerSectorStats& ps) {
            return AnalysisStatsRow{
                .label = std::move(label),
                .sectors = ps.t_sect,
                .popn = ps.popn,
                .troops = ps.troops,
                .avg_eff = static_cast<double>(ps.eff) / ps.t_sect,
                .avg_mob = static_cast<double>(ps.mob) / ps.t_sect,
                .res = ps.res,
                .crystals = ps.crys,
                .by_type = ps.sect,
            };
          };

          if (stats.unowned.t_sect != 0) {
            report.rows.push_back(make_row("0", stats.unowned));
          }

          for (const auto& [p, ps] : stats.players) {
            if (ps.t_sect != 0) {
              report.rows.push_back(make_row(std::format("{}", p), ps));
            }
          }

          report.rows.push_back(AnalysisStatsRow{
              .label = "Tl",
              .sectors = total_sect,
              .popn = stats.total_popn,
              .troops = stats.total_troops,
              .avg_eff = static_cast<double>(stats.total_eff) / total_sect,
              .avg_mob = static_cast<double>(stats.total_mob) / total_sect,
              .res = stats.total_res,
              .crystals = stats.total_crys,
              .by_type = stats.sect,
          });

          vm.planets.push_back(std::move(report));
        });
  });
}

// Parse a single character into a sector type
std::optional<SectorType> parse_sector_type(char c) {
  static const std::unordered_map<char, SectorType> SECTOR_TYPES = {
      {CHAR_SEA, SectorType::SEC_SEA},
      {CHAR_LAND, SectorType::SEC_LAND},
      {CHAR_MOUNT, SectorType::SEC_MOUNT},
      {CHAR_GAS, SectorType::SEC_GAS},
      {CHAR_ICE, SectorType::SEC_ICE},
      {CHAR_FOREST, SectorType::SEC_FOREST},
      {'d', SectorType::SEC_DESERT},  // 'd' instead of '-' to avoid mode flag
      {CHAR_PLATED, SectorType::SEC_PLATED},
      {CHAR_WASTED, SectorType::SEC_WASTED},
  };

  if (auto it = SECTOR_TYPES.find(c); it != SECTOR_TYPES.end()) {
    return it->second;
  }
  return std::nullopt;
}

// Parse a string as a player number
std::optional<int> parse_player_number(const std::string& arg) {
  if (arg.empty() || !std::isdigit(static_cast<unsigned char>(arg[0])))
    return std::nullopt;
  return std::stoi(arg);
}

}  // namespace

namespace GB::commands {
bool analysis(const command_t& argv, GameObj& g) {
  std::optional<SectorType> sector_type;  // nullopt does analysis on all types
  auto filter = PlayerFilter::all_players();
  auto mode = Mode::TopFive;

  auto where = Place{g.level(), g.snum(), g.pnum(), g.shipno()};

  for (const auto& arg : argv | std::views::drop(1)) {
    // Mode flag: "-" switches to bottom five
    if (arg == "-") {
      mode = Mode::BottomFive;
      continue;
    }

    // Sector type (single character)
    if (arg.length() == 1) {
      if (auto st = parse_sector_type(arg[0])) {
        sector_type = st;
        continue;
      }
    }

    // Player number
    if (auto player_num = parse_player_number(arg)) {
      if (*player_num == 0) {
        filter = PlayerFilter::unoccupied();
        continue;
      }
      if (!g.entity_manager.find_player_by_name(arg)) {
        g.out << "No such player #.\n";
        return false;
      }
      filter = PlayerFilter::specific(*player_num);
      continue;
    }

    // Scope
    auto maybe_where = Place::resolve(g.entity_manager, g.scope_context(), arg);
    if (maybe_where) {
      where = *maybe_where;
      continue;
    }

    // Nothing matched - unrecognized argument
    g.out << std::format("Unrecognized argument: {}\n", arg);
    return false;
  }

  AnalysisViewModel vm{};
  switch (where.level) {
    case ScopeLevel::LEVEL_UNIV:
      [[fallthrough]];
    case ScopeLevel::LEVEL_SHIP:
      g.out << "You can only analyze planets.\n";
      return false;
    case ScopeLevel::LEVEL_PLAN:
      do_analysis(vm, g, filter, mode, sector_type, where.snum, where.pnum);
      break;
    case ScopeLevel::LEVEL_STAR: {
      const auto& star = *g.entity_manager.peek_star(where.snum);
      for (const auto& planet : PlanetList::readonly(
               g.entity_manager, where.snum, star.numplanets())) {
        do_analysis(vm, g, filter, mode, sector_type, where.snum,
                    planet.planet_order());
      }
      break;
    }
  }
  g.out << GB::presentation::render_for_mode(g.ui_mode(), vm);
  return true;
}

const CommandDescriptor analysis_cmd{
    .name = "analysis",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "analysis [ [-] [<sector type>] [<race #>] [<scope>] ]",
    .description =
        "Statistical analysis of top/bottom sector categories on planets",
    .handler = &analysis,
};

}  // namespace GB::commands