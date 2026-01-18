// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;
import tabulate;

module commands;

static constexpr int CARE = 5;

struct anal_sect {
  unsigned int x, y;
  unsigned int des;
  resource_t value = -1;  // -1 means not set
};

namespace {
enum class Mode {
  top_five,
  bottom_five
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

void insert(Mode mode, std::array<struct anal_sect, CARE>& arr, anal_sect in) {
  for (int i = 0; i < CARE; i++) {
    if ((mode == Mode::top_five && arr[i].value < in.value) ||
        (mode == Mode::bottom_five &&
         (arr[i].value > in.value || arr[i].value == -1))) {
      for (int j = CARE - 1; j > i; j--)
        arr[j] = arr[j - 1];
      arr[i] = in;
      return;
    }
  }
}

void PrintTop(GameObj& g, const std::array<struct anal_sect, CARE> arr,
              const std::string& name) {
  g.out << std::format("{:>8}:", name);

  for (const auto& as : arr) {
    if (as.value == -1) continue;
    g.out << std::format("{:>5}{}({:>2},{:>2})", as.value, Dessymbols[as.des],
                         as.x, as.y);
  }
  g.out << "\n";
}

struct SectorStats {
  int TotalCrys = 0;
  std::array<int, MAXPLAYERS + 1> PlayCrys{};
  int TotalTroops = 0;
  std::array<int, MAXPLAYERS + 1> PlayTroops{};
  int TotalPopn = 0;
  std::array<int, MAXPLAYERS + 1> PlayPopn{};
  int TotalMob = 0;
  std::array<int, MAXPLAYERS + 1> PlayMob{};
  int TotalEff = 0;
  std::array<int, MAXPLAYERS + 1> PlayEff{};
  int TotalRes = 0;
  std::array<int, MAXPLAYERS + 1> PlayRes{};
  int PlaySect[MAXPLAYERS + 1][SectorType::SEC_WASTED + 1]{};
  std::array<int, MAXPLAYERS + 1> PlayTSect{};
  std::array<int, MAXPLAYERS + 1> WastedSect{};
  std::array<int, SectorType::SEC_WASTED + 1> Sect{};
};

SectorStats accumulate_statistics(GameObj& g, const SectorMap& smap) {
  SectorStats stats;

  for (auto& sect : smap) {
    auto p = sect.get_owner().value;

    stats.PlayEff[p] += sect.get_eff();
    stats.PlayMob[p] += sect.get_mobilization();
    stats.PlayRes[p] += sect.get_resource();
    stats.PlayPopn[p] += sect.get_popn();
    stats.PlayTroops[p] += sect.get_troops();
    stats.PlaySect[p][sect.get_condition()]++;
    stats.PlayTSect[p]++;
    stats.TotalEff += sect.get_eff();
    stats.TotalMob += sect.get_mobilization();
    stats.TotalRes += sect.get_resource();
    stats.TotalPopn += sect.get_popn();
    stats.TotalTroops += sect.get_troops();
    stats.Sect[sect.get_condition()]++;

    if (sect.is_wasted()) {
      stats.WastedSect[p]++;
    }
    if (sect.get_crystals() && g.race->tech >= TECH_CRYSTAL) {
      stats.PlayCrys[p]++;
      stats.TotalCrys++;
    }
  }

  return stats;
}

struct TopSectorLists {
  std::array<struct anal_sect, CARE> Res;
  std::array<struct anal_sect, CARE> Eff;
  std::array<struct anal_sect, CARE> Frt;
  std::array<struct anal_sect, CARE> Mob;
  std::array<struct anal_sect, CARE> Troops;
  std::array<struct anal_sect, CARE> Popn;
  std::array<struct anal_sect, CARE> mPopn;
};

TopSectorLists find_top_sectors(GameObj& g, const SectorMap& smap,
                                const Planet& planet, Mode mode,
                                const PlayerFilter& filter,
                                std::optional<SectorType> sector_type) {
  TopSectorLists tops;

  for (auto& sect : smap) {
    if (!sector_type || *sector_type == sect.get_condition()) {
      if (filter.matches(sect.get_owner())) {
        insert(mode, tops.Res,
               {.x = sect.get_x(),
                .y = sect.get_y(),
                .des = sect.get_condition(),
                .value = sect.get_resource()});
        insert(mode, tops.Eff,
               {.x = sect.get_x(),
                .y = sect.get_y(),
                .des = sect.get_condition(),
                .value = sect.get_eff()});
        insert(mode, tops.Mob,
               {.x = sect.get_x(),
                .y = sect.get_y(),
                .des = sect.get_condition(),
                .value = sect.get_mobilization()});
        insert(mode, tops.Frt,
               {.x = sect.get_x(),
                .y = sect.get_y(),
                .des = sect.get_condition(),
                .value = sect.get_fert()});
        insert(mode, tops.Popn,
               {.x = sect.get_x(),
                .y = sect.get_y(),
                .des = sect.get_condition(),
                .value = sect.get_popn()});
        insert(mode, tops.Troops,
               {.x = sect.get_x(),
                .y = sect.get_y(),
                .des = sect.get_condition(),
                .value = sect.get_troops()});
        insert(
            mode, tops.mPopn,
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

  auto TotalSect = planet.Maxx() * planet.Maxy();

  const auto& smap = *g.entity_manager.peek_sectormap(Starnum, Planetnum);

  // Accumulate statistics for all sectors
  auto stats = accumulate_statistics(g, smap);

  // Find top sectors matching the filter
  auto tops = find_top_sectors(g, smap, planet, mode, filter, sector_type);

  std::stringstream header;
  const auto& star = *g.entity_manager.peek_star(Starnum);
  header << std::format("\nAnalysis of /{}/{}:\n", star.get_name(),
                        star.get_planet_name(Planetnum));
  header << std::format("{} {}",
                        (mode == Mode::top_five ? "Highest" : "Lowest"), CARE);
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

  PrintTop(g, tops.Troops, "Troops");
  PrintTop(g, tops.Res, "Res");
  PrintTop(g, tops.Eff, "Eff");
  PrintTop(g, tops.Frt, "Frt");
  PrintTop(g, tops.Mob, "Mob");
  PrintTop(g, tops.Popn, "Popn");
  PrintTop(g, tops.mPopn, "^Popn");

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
    if (stats.PlayTSect[p] != 0) {
      std::vector<std::string> row = {
          std::format("{}", p),
          std::format("{}", stats.PlayTSect[p]),
          std::format("{}", stats.PlayPopn[p]),
          std::format("{}", stats.PlayTroops[p]),
          std::format("{:.1f}", static_cast<double>(stats.PlayEff[p]) /
                                    stats.PlayTSect[p]),
          std::format("{:.1f}", static_cast<double>(stats.PlayMob[p]) /
                                    stats.PlayTSect[p]),
          std::format("{}", stats.PlayRes[p]),
          std::format("{}", stats.PlayCrys[p])};
      for (int i = 0; i <= SectorType::SEC_WASTED; i++) {
        row.push_back(std::format("{}", stats.PlaySect[p][i]));
      }
      table.add_row(tabulate::Table::Row_t(row.begin(), row.end()));
    }
  }

  // Add totals row
  std::vector<std::string> totals = {
      "Tl",
      std::format("{}", TotalSect),
      std::format("{}", stats.TotalPopn),
      std::format("{}", stats.TotalTroops),
      std::format("{:.1f}", static_cast<double>(stats.TotalEff) / TotalSect),
      std::format("{:.1f}", static_cast<double>(stats.TotalMob) / TotalSect),
      std::format("{}", stats.TotalRes),
      std::format("{}", stats.TotalCrys)};
  for (int i = 0; i <= SectorType::SEC_WASTED; i++) {
    totals.push_back(std::format("{}", stats.Sect[i]));
  }
  table.add_row(tabulate::Table::Row_t(totals.begin(), totals.end()));

  // Configure dynamic sector columns
  for (int i = 0; i <= SectorType::SEC_WASTED; i++) {
    table.column(8 + i).format().width(4).font_align(
        tabulate::FontAlign::right);
  }

  g.out << table << "\n";
}

}  // namespace

namespace GB::commands {
void analysis(const command_t& argv, GameObj& g) {
  std::optional<SectorType> sector_type;  // nullopt does analysis on all types
  auto filter = PlayerFilter::all_players();
  auto mode = Mode::top_five;

  auto where = Place{g.level(), g.snum(), g.pnum()};

  bool skipped_first = false;

  for (const auto& arg : argv) {
    // Skip the name of the command
    if (!skipped_first) {
      skipped_first = true;
      continue;
    }

    // Top or bottom five
    if (arg == "-") {
      mode = Mode::bottom_five;
      continue;
    }

    // Sector type
    if (arg.length() == 1) {
      switch (arg[0]) {
        case CHAR_SEA:
          sector_type = SectorType::SEC_SEA;
          break;
        case CHAR_LAND:
          sector_type = SectorType::SEC_LAND;
          break;
        case CHAR_MOUNT:
          sector_type = SectorType::SEC_MOUNT;
          break;
        case CHAR_GAS:
          sector_type = SectorType::SEC_GAS;
          break;
        case CHAR_ICE:
          sector_type = SectorType::SEC_ICE;
          break;
        case CHAR_FOREST:
          sector_type = SectorType::SEC_FOREST;
          break;
          /*  Must use 'd' to do an analysis on */
          /*  desert sectors to avoid confusion */
        /*  with the '-' for the mode type    */
        case 'd':
          sector_type = SectorType::SEC_DESERT;
          break;
        case CHAR_PLATED:
          sector_type = SectorType::SEC_PLATED;
          break;
        case CHAR_WASTED:
          sector_type = SectorType::SEC_WASTED;
          break;
      }
      if (sector_type.has_value()) {
        continue;
      }
    }

    // Player number
    if (std::isdigit(arg[0])) {
      int player_num = std::stoi(arg);
      if (player_num > g.entity_manager.num_races()) {
        g.out << "No such player #.\n";
        return;
      }
      if (player_num == 0) {
        filter = PlayerFilter::unoccupied();
      } else {
        filter = PlayerFilter::specific(player_t{player_num});
      }
      continue;
    }

    // Scope
    Place maybe_where{g, arg};
    if (!maybe_where.err) {
      where = maybe_where;
    }
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