// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;
import tabulate;

module commands;

static constexpr int CARE = 5;

namespace {
struct AnalSect {
  unsigned int x, y;
  unsigned int des;
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
  std::optional<player_t> player;

  static PlayerFilter all_players() {
    return {.mode = PlayerFilterMode::AllPlayers, .player = std::nullopt};
  }

  static PlayerFilter unoccupied() {
    return {.mode = PlayerFilterMode::Unoccupied, .player = std::nullopt};
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
        return player.has_value() && owner == *player;
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
        return std::format(" sectors owned by {}.\n", player->value);
    }
    return " sectors.\n";
  }
};

void insert(Mode mode, std::array<struct AnalSect, CARE>& arr, AnalSect in) {
  auto* insertion_point = std::ranges::find_if(arr, [&](const auto& elem) {
    return (mode == Mode::TopFive && elem.value < in.value) ||
           (mode == Mode::BottomFive &&
            (elem.value > in.value || elem.value == -1));
  });

  if (insertion_point != arr.end()) {
    std::shift_right(insertion_point, arr.end(), 1);
    *insertion_point = in;
  }
}

void print_top(GameObj& g, const std::array<struct AnalSect, CARE> kArr,
               const std::string& name) {
  g.out << std::format("{:>8}:", name);

  for (const auto& as : kArr) {
    if (as.value == -1) continue;
    g.out << std::format("{:>5}{}({:>2},{:>2})", as.value, Dessymbols[as.des],
                         as.x, as.y);
  }
  g.out << "\n";
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
  std::array<PlayerSectorStats, MAXPLAYERS + 1> players{};
};

SectorStats accumulate_statistics(GameObj& g, const SectorMap& smap) {
  SectorStats stats;

  for (const auto& sect : smap) {
    auto p = sect.get_owner().value;

    stats.players[p].eff += sect.get_eff();
    stats.players[p].mob += sect.get_mobilization();
    stats.players[p].res += sect.get_resource();
    stats.players[p].popn += sect.get_popn();
    stats.players[p].troops += sect.get_troops();
    stats.players[p].sect[sect.get_condition()]++;
    stats.players[p].t_sect++;
    stats.total_eff += sect.get_eff();
    stats.total_mob += sect.get_mobilization();
    stats.total_res += sect.get_resource();
    stats.total_popn += sect.get_popn();
    stats.total_troops += sect.get_troops();
    stats.sect[sect.get_condition()]++;

    if (sect.is_wasted()) {
      stats.players[p].wasted_sect++;
    }
    if (sect.get_crystals() && g.race->tech >= TECH_CRYSTAL) {
      stats.players[p].crys++;
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
        insert(mode, tops.res,
               {.x = sect.get_x(),
                .y = sect.get_y(),
                .des = sect.get_condition(),
                .value = sect.get_resource()});
        insert(mode, tops.eff,
               {.x = sect.get_x(),
                .y = sect.get_y(),
                .des = sect.get_condition(),
                .value = sect.get_eff()});
        insert(mode, tops.mob,
               {.x = sect.get_x(),
                .y = sect.get_y(),
                .des = sect.get_condition(),
                .value = sect.get_mobilization()});
        insert(mode, tops.frt,
               {.x = sect.get_x(),
                .y = sect.get_y(),
                .des = sect.get_condition(),
                .value = sect.get_fert()});
        insert(mode, tops.popn,
               {.x = sect.get_x(),
                .y = sect.get_y(),
                .des = sect.get_condition(),
                .value = sect.get_popn()});
        insert(mode, tops.troops,
               {.x = sect.get_x(),
                .y = sect.get_y(),
                .des = sect.get_condition(),
                .value = sect.get_troops()});
        insert(
            mode, tops.m_popn,
            {.x = sect.get_x(),
             .y = sect.get_y(),
             .des = sect.get_condition(),
             .value = maxsupport(*g.race, sect, planet.compatibility(*g.race),
                                 planet.conditions(TOXIC))});
      }
    }
  }

  return tops;
}

void do_analysis(GameObj& g, const PlayerFilter& filter, Mode mode,
                 std::optional<SectorType> sector_type, starnum_t Starnum,
                 planetnum_t Planetnum) {
  auto planet_handle = g.entity_manager.get_planet(Starnum, Planetnum);
  if (!planet_handle.get()) {
    g.out << "Planet not found.\n";
    return;
  }
  const auto& planet = planet_handle.read();

  if (!planet.info(g.player()).explored) {
    return;
  }

  auto total_sect = planet.Maxx() * planet.Maxy();

  const auto& smap = *g.entity_manager.peek_sectormap(Starnum, Planetnum);

  // Accumulate statistics for all sectors
  auto stats = accumulate_statistics(g, smap);

  // Find top sectors matching the filter
  auto tops = find_top_sectors(g, smap, planet, mode, filter, sector_type);

  std::stringstream header;
  const auto& star = *g.entity_manager.peek_star(Starnum);
  header << std::format("\nAnalysis of /{}/{}:\n", star.get_name(),
                        star.get_planet_name(Planetnum));
  header << std::format("{} {}", (mode == Mode::TopFive ? "Highest" : "Lowest"),
                        CARE);
  if (!sector_type.has_value()) {
    header << " of all";
  } else {
    switch (*sector_type) {
      case SectorType::SEC_SEA:
        header << " Ocean";
        break;
      case SectorType::SEC_LAND:
        header << " Land";
        break;
      case SectorType::SEC_MOUNT:
        header << " Mountain";
        break;
      case SectorType::SEC_GAS:
        header << " Gas";
        break;
      case SectorType::SEC_ICE:
        header << " Ice";
        break;
      case SectorType::SEC_FOREST:
        header << " Forest";
        break;
      case SectorType::SEC_DESERT:
        header << " Desert";
        break;
      case SectorType::SEC_PLATED:
        header << " Plated";
        break;
      case SectorType::SEC_WASTED:
        header << " Wasted";
        break;
    }
  }
  header << filter.describe();
  g.out << header.str();

  print_top(g, tops.troops, "Troops");
  print_top(g, tops.res, "Res");
  print_top(g, tops.eff, "Eff");
  print_top(g, tops.frt, "Frt");
  print_top(g, tops.mob, "Mob");
  print_top(g, tops.popn, "Popn");
  print_top(g, tops.m_popn, "^Popn");

  g.out << "\n";

  // Build table with dynamic sector-type columns
  tabulate::Table table;
  table.format().hide_border().column_separator("  ");

  // Configure fixed columns
  table.column(0).format().width(2).font_align(tabulate::FontAlign::right);
  table.column(1).format().width(3).font_align(tabulate::FontAlign::right);
  table.column(2).format().width(7).font_align(tabulate::FontAlign::right);
  table.column(3).format().width(6).font_align(tabulate::FontAlign::right);
  table.column(4).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(5).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(6).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(7).format().width(2).font_align(tabulate::FontAlign::right);
  // Dynamic sector columns configured after adding rows

  // Build header row
  std::vector<std::string> table_header = {"Pl",    "sec",   "popn", "troops",
                                           "a.eff", "a.mob", "res",  "x"};
  for (int i = 0; i <= SectorType::SEC_WASTED; i++) {
    table_header.emplace_back(1, Dessymbols[i]);
  }
  table.add_row(
      tabulate::Table::Row_t(table_header.begin(), table_header.end()));
  table[0].format().font_style({tabulate::FontStyle::bold});

  // Add player rows
  for (int p = 0; p <= g.entity_manager.num_races().value; p++) {
    if (stats.players[p].t_sect != 0) {
      std::vector<std::string> row = {
          std::format("{}", p),
          std::format("{}", stats.players[p].t_sect),
          std::format("{}", stats.players[p].popn),
          std::format("{}", stats.players[p].troops),
          std::format("{:.1f}", static_cast<double>(stats.players[p].eff) /
                                    stats.players[p].t_sect),
          std::format("{:.1f}", static_cast<double>(stats.players[p].mob) /
                                    stats.players[p].t_sect),
          std::format("{}", stats.players[p].res),
          std::format("{}", stats.players[p].crys)};
      for (int i = 0; i <= SectorType::SEC_WASTED; i++) {
        row.push_back(std::format("{}", stats.players[p].sect[i]));
      }
      table.add_row(tabulate::Table::Row_t(row.begin(), row.end()));
    }
  }

  // Add totals row
  std::vector<std::string> totals = {
      "Tl",
      std::format("{}", total_sect),
      std::format("{}", stats.total_popn),
      std::format("{}", stats.total_troops),
      std::format("{:.1f}", static_cast<double>(stats.total_eff) / total_sect),
      std::format("{:.1f}", static_cast<double>(stats.total_mob) / total_sect),
      std::format("{}", stats.total_res),
      std::format("{}", stats.total_crys)};
  for (int i = 0; i <= SectorType::SEC_WASTED; i++) {
    totals.push_back(std::format("{}", stats.sect[i]));
  }
  table.add_row(tabulate::Table::Row_t(totals.begin(), totals.end()));

  // Configure dynamic sector columns
  for (int i = 0; i <= SectorType::SEC_WASTED; i++) {
    table.column(8 + i).format().width(4).font_align(
        tabulate::FontAlign::right);
  }

  g.out << table << "\n";
}

// Parse a single character into a sector type
std::optional<SectorType> parse_sector_type(char c) {
  static const std::unordered_map<char, SectorType> kSectorTypes = {
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

  if (auto it = kSectorTypes.find(c); it != kSectorTypes.end()) {
    return it->second;
  }
  return std::nullopt;
}

// Parse a string as a player number
std::optional<int> parse_player_number(const std::string& arg) {
  if (arg.empty() || !std::isdigit(arg[0])) return std::nullopt;
  return std::stoi(arg);
}

}  // namespace

namespace GB::commands {
void analysis(const command_t& argv, GameObj& g) {
  std::optional<SectorType> sector_type;  // nullopt does analysis on all types
  auto filter = PlayerFilter::all_players();
  auto mode = Mode::TopFive;

  auto where = Place{g.level(), g.snum(), g.pnum()};

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
      if (*player_num > g.entity_manager.num_races()) {
        g.out << "No such player #.\n";
        return;
      }
      filter = (*player_num == 0)
                   ? PlayerFilter::unoccupied()
                   : PlayerFilter::specific(player_t{*player_num});
      continue;
    }

    // Scope
    Place maybe_where{g, arg};
    if (!maybe_where.err) {
      where = maybe_where;
      continue;
    }

    // Nothing matched - unrecognized argument
    g.out << std::format("Unrecognized argument: {}\n", arg);
    return;
  }

  if (where.err) {
    g.out << "Invalid scope.\n";
    return;
  }

  switch (where.level) {
    case ScopeLevel::LEVEL_UNIV:
      [[fallthrough]];
    case ScopeLevel::LEVEL_SHIP:
      g.out << "You can only analyze planets.\n";
      break;
    case ScopeLevel::LEVEL_PLAN:
      do_analysis(g, filter, mode, sector_type, where.snum, where.pnum);
      break;
    case ScopeLevel::LEVEL_STAR: {
      const auto& star = *g.entity_manager.peek_star(where.snum);
      for (planetnum_t pnum = 0; pnum < star.numplanets(); pnum++) {
        do_analysis(g, filter, mode, sector_type, where.snum, pnum);
      }
      break;
    }
  }
}
}  // namespace GB::commands