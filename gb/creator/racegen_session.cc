// SPDX-License-Identifier: Apache-2.0

/// \file racegen_session.cc
/// \brief Interactive session implementation for player race generation.

module;

import strong_id;
import glaze.core;
import glaze.json;
import gb.entities;
import gb.repositories.glaze;
import std;

module gb.creator;

namespace GB::creator {

namespace {

std::string to_lower(std::string_view s) {
  return s | std::views::transform([](char c) {
           return static_cast<char>(
               std::tolower(static_cast<unsigned char>(c)));
         }) |
         std::ranges::to<std::string>();
}

std::string_view trim(std::string_view s) {
  auto not_space = [](char c) {
    return !std::isspace(static_cast<unsigned char>(c));
  };
  auto start = std::ranges::find_if(s, not_space);
  if (start == s.end()) return {};
  auto end = std::ranges::find_if(s | std::views::reverse, not_space).base();
  return std::string_view(start, end);
}

bool iequals(std::string_view a, std::string_view b) noexcept {
  return std::ranges::equal(
      a, b, {},
      [](char c) { return std::tolower(static_cast<unsigned char>(c)); },
      [](char c) { return std::tolower(static_cast<unsigned char>(c)); });
}

std::string capitalize(std::string_view s) {
  std::string result(s);
  if (!result.empty()) {
    result.front() = static_cast<char>(
        std::toupper(static_cast<unsigned char>(result.front())));
  }
  return result;
}

template <typename T, std::size_t N>
std::optional<T>
lookup_table(const std::array<std::pair<std::string_view, T>, N>& table,
             std::string_view key) noexcept {
  for (const auto& [name, val] : table) {
    if (iequals(name, key)) return val;
  }
  return std::nullopt;
}

std::optional<PlanetType> parse_planet_type(std::string_view s) {
  static constexpr std::array<std::pair<std::string_view, PlanetType>, 14>
      planet_aliases{{
          {"earth", PlanetType::EARTH},
          {"class m", PlanetType::EARTH},
          {"m", PlanetType::EARTH},
          {"forest", PlanetType::FOREST},
          {"desert", PlanetType::DESERT},
          {"water", PlanetType::WATER},
          {"waterball", PlanetType::WATER},
          {"mars", PlanetType::MARS},
          {"airless", PlanetType::MARS},
          {"iceball", PlanetType::ICEBALL},
          {"ice", PlanetType::ICEBALL},
          {"gasgiant", PlanetType::GASGIANT},
          {"jovian", PlanetType::GASGIANT},
          {"gas", PlanetType::GASGIANT},
      }};
  return lookup_table(planet_aliases, trim(s));
}

std::optional<SectorType> parse_sector_type(std::string_view s) {
  static constexpr std::array<std::pair<std::string_view, SectorType>, 13>
      sector_aliases{{
          {"water", SectorType::SEC_SEA},
          {"sea", SectorType::SEC_SEA},
          {"ocean", SectorType::SEC_SEA},
          {"land", SectorType::SEC_LAND},
          {"mountain", SectorType::SEC_MOUNT},
          {"mount", SectorType::SEC_MOUNT},
          {"mountainous", SectorType::SEC_MOUNT},
          {"gas", SectorType::SEC_GAS},
          {"gaseous", SectorType::SEC_GAS},
          {"ice", SectorType::SEC_ICE},
          {"forest", SectorType::SEC_FOREST},
          {"desert", SectorType::SEC_DESERT},
          {"plated", SectorType::SEC_PLATED},
      }};
  return lookup_table(sector_aliases, trim(s));
}

std::optional<bool> parse_bool(std::string_view s) {
  static constexpr std::array<std::pair<std::string_view, bool>, 8>
      bool_aliases{{
          {"yes", true},
          {"true", true},
          {"1", true},
          {"y", true},
          {"no", false},
          {"false", false},
          {"0", false},
          {"n", false},
      }};
  return lookup_table(bool_aliases, trim(s));
}

template <typename T>
std::optional<T> parse_number(std::string_view s) {
  s = trim(s);
  if (s.empty()) return std::nullopt;
  T val{};
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
  if (ec == std::errc{} && ptr == s.data() + s.size()) {
    return val;
  }
  return std::nullopt;
}

}  // namespace

const std::array<RacegenSession::CommandDescriptor, 8>&
RacegenSession::commands() {
  static constexpr std::array<CommandDescriptor, 8> cmds{{
      {"modify", "modify <field> <value>",
       "Modify a race attribute, sector compatibility, or setting",
       &RacegenSession::do_modify},
      {"archetype", "archetype [1-11|name] [random]",
       "List or load one of the 11 preset evolutionary archetypes",
       &RacegenSession::do_archetype},
      {"print", "print", "Display current specification and point costs",
       &RacegenSession::do_print},
      {"save", "save [filename]", "Save the race specification to a JSON file",
       &RacegenSession::do_save},
      {"load", "load [filename]", "Load a race specification from a JSON file",
       &RacegenSession::do_load},
      {"enroll", "enroll",
       "Enroll the configured race into the active universe database",
       &RacegenSession::do_enroll},
      {"help", "help [topic]", "Show help for commands or modifiable fields",
       &RacegenSession::do_help},
      {"quit", "quit", "Exit the race generator", &RacegenSession::do_quit},
  }};
  return cmds;
}

std::expected<void, std::string>
save_race_spec(const RaceEnrollmentSpec& spec,
               const std::filesystem::path& path) {
  auto json = glz::write_json(spec);
  if (!json) {
    return std::unexpected("Failed to serialize race specification to JSON.");
  }
  std::ofstream file(path);
  if (!file.is_open()) {
    return std::unexpected(
        std::format("Cannot open file '{}' for writing.", path.string()));
  }
  file << *json;
  return {};
}

std::expected<RaceEnrollmentSpec, std::string>
load_race_spec(const std::filesystem::path& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return std::unexpected(
        std::format("Cannot open file '{}' for reading.", path.string()));
  }
  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
  RaceEnrollmentSpec loaded{};
  auto ec = glz::read_json(loaded, content);
  if (ec) {
    return std::unexpected(
        std::format("Failed to parse JSON file '{}'.", path.string()));
  }
  return loaded;
}

RacegenSession::RacegenSession(std::istream& in, std::ostream& out,
                               EnrollmentService* enrollment_service)
    : in_(in), out_(out), enrollment_service_(enrollment_service), engine_(),
      spec_(engine_.create_default_spec(false)) {
  update_cost();
}

void RacegenSession::update_cost() {
  cost_ = engine_.calculate_cost(spec_);
}

void RacegenSession::run() {
  print_race();
  std::string line;
  while (!quit_requested_) {
    out_ << "racegen> ";
    out_.flush();
    if (!std::getline(in_, line)) {
      break;
    }
    execute_command(line);
  }
}

bool RacegenSession::execute_command(std::string_view line) {
  std::string_view trimmed = trim(line);
  if (trimmed.empty()) {
    return true;
  }

  auto space_pos = trimmed.find_first_of(" \t");
  std::string_view cmd_str = trimmed.substr(0, space_pos);
  std::string_view args = (space_pos == std::string_view::npos)
                              ? std::string_view{}
                              : trim(trimmed.substr(space_pos));

  for (const auto& cmd : commands()) {
    if (iequals(cmd.name, cmd_str)) {
      return (this->*(cmd.handler))(args);
    }
  }

  if (iequals(cmd_str, "preset")) {
    return do_archetype(args);
  }
  if (iequals(cmd_str, "exit")) {
    return do_quit(args);
  }

  std::println(out_,
               "Unknown command '{}'. Type 'help' for a list of commands.",
               cmd_str);
  return true;
}

bool RacegenSession::do_modify(std::string_view args) {
  if (args.empty()) {
    std::println(out_,
                 "Usage: modify <field> <value>. Type 'help modify' for a "
                 "list of fields.");
    return true;
  }

  auto field_space_pos = args.find_first_of(" \t");
  if (field_space_pos == std::string_view::npos) {
    std::println(out_, "Error: Missing value for field '{}'.", args);
    return true;
  }

  std::string_view field = args.substr(0, field_space_pos);
  std::string_view val = trim(args.substr(field_space_pos));
  modify_field(field, val);
  return true;
}

bool RacegenSession::do_print(std::string_view) {
  print_race();
  return true;
}

bool RacegenSession::apply_archetype(std::string_view query, bool randomize) {
  const auto* arch = find_archetype(query);
  if (!arch) {
    std::println(out_,
                 "Error: Unknown archetype '{}'. Type 'archetype' for a list "
                 "of available presets (1-{}).",
                 query, race_archetypes.size());
    return false;
  }

  // Preserve user identity and credential fields if already customized
  std::string saved_name = spec_.name;
  std::string saved_pass = spec_.password;
  std::string saved_gov_pass = spec_.governor_password;
  std::string saved_address = spec_.address;
  bool saved_god = spec_.is_god;
  bool saved_guest = spec_.is_guest;

  RaceEnrollmentSpec loaded =
      arch->to_enrollment_spec(arch->default_planet, randomize);

  if (!saved_name.empty() && saved_name != "New Empire") {
    loaded.name = std::move(saved_name);
  }
  loaded.password = std::move(saved_pass);
  loaded.governor_password = std::move(saved_gov_pass);
  loaded.address = std::move(saved_address);
  loaded.is_god = saved_god;
  loaded.is_guest = saved_guest;

  spec_ = std::move(loaded);
  update_cost();
  return true;
}

bool RacegenSession::do_archetype(std::string_view args) {
  std::string_view trimmed = trim(args);
  if (trimmed.empty()) {
    std::println(out_, "\n=== Available Preset Evolutionary Archetypes ===\n");
    out_ << create_archetypes_table() << "\n";
    std::println(
        out_,
        "Usage: archetype <1-{}|name> [random]  (loads deterministic base "
        "stats by default)\n",
        race_archetypes.size());
    return true;
  }

  bool randomize = false;
  std::string_view query = trimmed;

  // Check if trailing token is 'random' or 'rand'
  auto last_space = trimmed.find_last_of(" \t");
  if (last_space != std::string_view::npos) {
    std::string_view suffix = trim(trimmed.substr(last_space));
    if (iequals(suffix, "random") || iequals(suffix, "rand")) {
      randomize = true;
      query = trim(trimmed.substr(0, last_space));
    }
  }

  if (apply_archetype(query, randomize)) {
    const auto* arch = find_archetype(query);
    std::println(out_, "Loaded {} archetype '{}' ({} points remaining).",
                 randomize ? "randomized" : "base", arch->name,
                 cost_.points_remaining);
    print_race();
  }
  return true;
}

bool RacegenSession::save_to_file(const std::filesystem::path& path) {
  auto res = save_race_spec(spec_, path);
  if (!res) {
    std::println(out_, "Error: {}", res.error());
    return false;
  }
  return true;
}

bool RacegenSession::load_from_file(const std::filesystem::path& path) {
  auto res = load_race_spec(path);
  if (!res) {
    std::println(out_, "Error: {}", res.error());
    return false;
  }
  spec_ = *res;
  update_cost();
  return true;
}

EnrollmentResult RacegenSession::enroll() {
  if (!enrollment_service_) {
    return {.success = false,
            .message = "No active universe database connected to session."};
  }

  if (cost_.points_remaining < 0) {
    return {.success = false,
            .message = std::format(
                "Cannot enroll: specification exceeds point budget by {} "
                "points.",
                -cost_.points_remaining)};
  }

  auto errors =
      engine_.validate(spec_, /*is_player=*/!spec_.is_god && !spec_.is_guest);
  if (!errors.empty()) {
    std::string msg = "Cannot enroll: validation errors:";
    for (const auto& err : errors) {
      msg += "\n  - " + err;
    }
    return {.success = false, .message = std::move(msg)};
  }

  return enrollment_service_->enroll_player(spec_);
}

bool RacegenSession::do_save(std::string_view args) {
  std::string_view filename = trim(args);
  std::string target_file;
  if (filename.empty()) {
    target_file = std::string(DEFAULT_RACEGEN_FILENAME);
  } else {
    target_file = std::string(filename);
    if (!target_file.ends_with(".json")) {
      target_file += ".json";
    }
  }

  if (save_to_file(target_file)) {
    std::println(out_, "Specification saved to '{}'.", target_file);
  }
  return true;
}

bool RacegenSession::do_load(std::string_view args) {
  std::string_view filename = trim(args);
  std::string target_file;
  if (filename.empty()) {
    target_file = std::string(DEFAULT_RACEGEN_FILENAME);
  } else {
    target_file = std::string(filename);
    if (!target_file.ends_with(".json") &&
        !std::filesystem::exists(target_file)) {
      if (std::filesystem::exists(target_file + ".json")) {
        target_file += ".json";
      }
    }
  }

  if (load_from_file(target_file)) {
    std::println(out_, "Specification loaded from '{}'.", target_file);
    print_race();
  }
  return true;
}

bool RacegenSession::do_enroll(std::string_view) {
  if (!enrollment_service_) {
    std::println(
        out_, "Error: Cannot enroll: no active universe database connected.");
    return true;
  }

  std::println(out_, "Enrolling player '{}'...", spec_.name);
  auto res = enroll();
  if (res.success) {
    std::println(out_, "Enrollment successful!");
    std::println(out_, "  Player ID    : {}", res.player_num);
    std::println(out_, "  Home Star    : {}", res.star);
    std::println(out_, "  Home Planet  : {}", res.pnum);
    std::println(out_, "  Capital      : ({}, {})", res.capital_coords.x,
                 res.capital_coords.y);
    std::println(out_, "  Gov Ship     : #{}", res.gov_ship);
    quit_requested_ = true;
    return false;
  }

  std::println(out_, "Enrollment failed: {}", res.message);
  return true;
}

bool RacegenSession::do_help(std::string_view args) {
  print_help(args);
  return true;
}

bool RacegenSession::do_quit(std::string_view) {
  quit_requested_ = true;
  return false;
}

bool RacegenSession::modify_field(std::string_view field_name,
                                  std::string_view value_str) {
  const std::string field = to_lower(trim(field_name));
  const std::string_view val_trimmed = trim(value_str);

  const auto backup = spec_;

  // 1. Sector compatibilities
  if (field == "plated") {
    std::println(out_,
                 "Error: Plated sector compatibility is fixed at 100% and "
                 "cannot be modified.");
    return false;
  }
  if (auto sec = parse_sector_type(field); sec.has_value()) {
    std::string_view num_str = val_trimmed;
    if (num_str.ends_with('%')) {
      num_str.remove_suffix(1);
    }
    auto parsed = parse_number<double>(num_str);
    if (!parsed) {
      std::println(out_, "Error: Invalid numeric value '{}' for sector {}.",
                   value_str, field_name);
      return false;
    }
    double compat = *parsed;
    if (compat > 1.0) {
      compat /= 100.0;
    }
    spec_.sector_compatibilities[*sec] = compat;
  } else if (field == "name") {
    spec_.name = std::string(val_trimmed);
  } else if (field.starts_with("pass")) {
    spec_.password = std::string(val_trimmed);
  } else if (field.starts_with("gov")) {
    spec_.governor_password = std::string(val_trimmed);
  } else if (field.starts_with("addr")) {
    spec_.address = std::string(val_trimmed);
  } else if (field.starts_with("plan")) {
    auto ptype = parse_planet_type(val_trimmed);
    if (!ptype) {
      std::println(out_,
                   "Error: Unknown planet type '{}'. Supported: Earth, "
                   "Forest, Desert, Waterball, Airless, Iceball, Jovian.",
                   val_trimmed);
      return false;
    }
    spec_.home_planet_type = *ptype;
    if (*ptype == PlanetType::GASGIANT) {
      spec_.sector_compatibilities = {.gas = 1.0, .plated = 0.0};
      spec_.likesbest = SectorType::SEC_GAS;
      spec_.preferred_sector = SectorType::SEC_GAS;
    } else if (spec_.sector_compatibilities.gas > 0.0) {
      spec_.sector_compatibilities.gas = 0.0;
      spec_.sector_compatibilities.plated = 1.0;
      spec_.likesbest = SectorType::SEC_PLATED;
      spec_.preferred_sector = SectorType::SEC_PLATED;
    }
  } else if (field == "race") {
    std::string rtype = to_lower(val_trimmed);
    if (rtype == "normal") {
      spec_.metamorph = false;
      spec_.absorb = false;
      spec_.pods = false;
      spec_.collective_iq = false;
      if (backup.metamorph) {
        spec_.number_sexes = 2;
      }
      if (spec_.iq == 0 && spec_.iq_limit > 0) {
        spec_.iq = spec_.iq_limit;
        spec_.iq_limit = 0;
      }
    } else if (rtype == "metamorph") {
      spec_.metamorph = true;
      spec_.absorb = true;
      spec_.pods = true;
      spec_.collective_iq = true;
      spec_.number_sexes = 1;
      if (spec_.iq_limit == 0 && spec_.iq > 0) {
        spec_.iq_limit = spec_.iq;
        spec_.iq = 0;
      }
    } else {
      std::println(
          out_, "Error: Unknown race type '{}'. Supported: normal, metamorph.",
          val_trimmed);
      return false;
    }
  } else if (field.starts_with("adv")) {
    if (auto num = parse_number<double>(val_trimmed)) {
      spec_.adventurism = *num;
    } else {
      std::println(out_, "Error: Invalid number '{}' for adventurism.",
                   val_trimmed);
      return false;
    }
  } else if (field.starts_with("abs")) {
    if (auto b = parse_bool(val_trimmed)) {
      spec_.absorb = *b;
    } else {
      std::println(out_, "Error: Invalid boolean '{}' (use yes/no).",
                   val_trimmed);
      return false;
    }
  } else if (field.starts_with("birth")) {
    if (auto num = parse_number<double>(val_trimmed)) {
      spec_.birthrate = *num;
    } else {
      std::println(out_, "Error: Invalid number '{}' for birthrate.",
                   val_trimmed);
      return false;
    }
  } else if (field.starts_with("col")) {
    if (auto b = parse_bool(val_trimmed)) {
      spec_.collective_iq = *b;
    } else {
      std::println(out_, "Error: Invalid boolean '{}' (use yes/no).",
                   val_trimmed);
      return false;
    }
  } else if (field.starts_with("fert")) {
    std::string_view num_str = val_trimmed;
    if (num_str.ends_with('%')) {
      num_str.remove_suffix(1);
    }
    if (auto num = parse_number<double>(num_str)) {
      double f = *num;
      if (f <= 1.0 && f > 0.0) {
        f *= 100.0;
      }
      spec_.fertilize = static_cast<fertilize_t>(std::round(f));
    } else {
      std::println(out_, "Error: Invalid number '{}' for fertilize.",
                   val_trimmed);
      return false;
    }
  } else if (field == "iq_limit") {
    if (auto num = parse_number<int>(val_trimmed)) {
      spec_.iq_limit = *num;
    } else {
      std::println(out_, "Error: Invalid integer '{}' for IQ limit.",
                   val_trimmed);
      return false;
    }
  } else if (field == "iq") {
    if (auto num = parse_number<int>(val_trimmed)) {
      if (spec_.collective_iq) {
        spec_.iq_limit = *num;
        spec_.iq = 0;
      } else {
        spec_.iq = *num;
        spec_.iq_limit = 0;
      }
    } else {
      std::println(out_, "Error: Invalid integer '{}' for IQ.", val_trimmed);
      return false;
    }
  } else if (field.starts_with("fight")) {
    if (auto num = parse_number<int>(val_trimmed)) {
      spec_.fighters = *num;
    } else {
      std::println(out_, "Error: Invalid integer '{}' for fighters.",
                   val_trimmed);
      return false;
    }
  } else if (field.starts_with("pod")) {
    if (auto b = parse_bool(val_trimmed)) {
      spec_.pods = *b;
    } else {
      std::println(out_, "Error: Invalid boolean '{}' (use yes/no).",
                   val_trimmed);
      return false;
    }
  } else if (field == "mass") {
    if (auto num = parse_number<double>(val_trimmed)) {
      spec_.mass = *num;
    } else {
      std::println(out_, "Error: Invalid number '{}' for mass.", val_trimmed);
      return false;
    }
  } else if (field.starts_with("sex")) {
    if (auto num = parse_number<int>(val_trimmed)) {
      spec_.number_sexes = *num;
    } else {
      std::println(out_, "Error: Invalid integer '{}' for sexes.", val_trimmed);
      return false;
    }
  } else if (field.starts_with("metab")) {
    if (auto num = parse_number<double>(val_trimmed)) {
      spec_.metabolism = *num;
    } else {
      std::println(out_, "Error: Invalid number '{}' for metabolism.",
                   val_trimmed);
      return false;
    }
  } else if (field.starts_with("pref")) {
    auto sec = parse_sector_type(val_trimmed);
    if (!sec) {
      std::println(out_,
                   "Error: Unknown sector type '{}' for preferred sector.",
                   val_trimmed);
      return false;
    }
    spec_.likesbest = *sec;
    spec_.preferred_sector = *sec;
  } else {
    std::println(out_,
                 "Error: Unknown field '{}'. Type 'help fields' for a list of "
                 "fields.",
                 field_name);
    return false;
  }

  // Validate changes against non-rigorous game rules
  auto errors =
      engine_.validate(spec_, /*is_player=*/!spec_.is_god && !spec_.is_guest,
                       /*rigorous=*/false);
  if (!errors.empty()) {
    std::println(out_, "Error: {}", errors.front());
    spec_ = backup;
    return false;
  }

  update_cost();
  std::println(out_, "Modified {} to {}. Points left: {}", field_name,
               val_trimmed, cost_.points_remaining);
  return true;
}

void RacegenSession::print_race() {
  update_cost();

  std::println(out_, "=== Race Specification ===");
  std::println(out_, "  Name          : {}", spec_.name);
  std::println(out_, "  Password      : {}", spec_.password);
  std::println(out_, "  Address       : {}", spec_.address);
  std::println(out_, "  Home Planet   : {:<18} [Cost: {:>4}]",
               spec_.home_planet_type, cost_.planet_cost);
  std::println(out_, "  Race Type     : {:<18} [Cost: {:>4}]",
               spec_.metamorph ? "Metamorph" : "Normal", cost_.race_type_cost);
  std::println(out_);

  std::println(out_, "=== Attributes ===");
  auto fmt_bool = [](bool b) { return b ? "yes" : "no"; };

  std::println(out_,
               "  {:<14}: {:>7.2f}  [Cost: {:>4}]    {:<14}: {:>7}  [Cost: "
               "{:>4}]",
               "Adventurism", spec_.adventurism, cost_.adventurism(), "Absorb",
               fmt_bool(spec_.absorb), cost_.absorb());

  std::println(out_,
               "  {:<14}: {:>7.2f}  [Cost: {:>4}]    {:<14}: {:>7}  [Cost: "
               "{:>4}]",
               "Birthrate", spec_.birthrate, cost_.birthrate(), "Collective IQ",
               fmt_bool(spec_.collective_iq), cost_.collective_iq());

  std::string iq_name = spec_.collective_iq ? "IQ Limit" : "IQ";
  int iq_val = spec_.collective_iq ? spec_.iq_limit : spec_.iq;
  std::println(out_,
               "  {:<14}: {:>6}%   [Cost: {:>4}]    {:<14}: {:>7}  [Cost: "
               "{:>4}]",
               "Fertilize", static_cast<int>(spec_.fertilize),
               cost_.fertilize(), iq_name, iq_val, cost_.iq());

  std::println(out_,
               "  {:<14}: {:>7}   [Cost: {:>4}]    {:<14}: {:>7}  [Cost: "
               "{:>4}]",
               "Fighters", spec_.fighters, cost_.fight(), "Pods",
               fmt_bool(spec_.pods), cost_.pods());

  std::println(out_,
               "  {:<14}: {:>7.2f}  [Cost: {:>4}]    {:<14}: {:>7}  [Cost: "
               "{:>4}]",
               "Mass", spec_.mass, cost_.mass(), "Sexes", spec_.number_sexes,
               cost_.sexes());

  std::println(out_, "  {:<14}: {:>7.2f}  [Cost: {:>4}]", "Metabolism",
               spec_.metabolism, cost_.metabolism());
  std::println(out_);

  std::println(out_, "=== Sector Compatibilities ===");
  auto fmt_sec = [this](SectorType st) {
    double pct = spec_.sector_compatibilities[st] * 100.0;
    bool common = Planet::is_common_sector(spec_.home_planet_type, st);
    char mark = common ? '*' : ' ';
    return std::format("{:>3.0f}%{}", pct, mark);
  };

  std::println(
      out_, "  {:<14}: {:>5}  [Cost: {:>4}]    {:<14}: {:>5}  [Cost: {:>4}]",
      capitalize(to_string(SectorType::SEC_SEA)), fmt_sec(SectorType::SEC_SEA),
      cost_.sector_costs[SectorType::SEC_SEA],
      capitalize(to_string(SectorType::SEC_LAND)),
      fmt_sec(SectorType::SEC_LAND), cost_.sector_costs[SectorType::SEC_LAND]);

  std::println(
      out_, "  {:<14}: {:>5}  [Cost: {:>4}]    {:<14}: {:>5}  [Cost: {:>4}]",
      capitalize(to_string(SectorType::SEC_MOUNT)),
      fmt_sec(SectorType::SEC_MOUNT), cost_.sector_costs[SectorType::SEC_MOUNT],
      capitalize(to_string(SectorType::SEC_GAS)), fmt_sec(SectorType::SEC_GAS),
      cost_.sector_costs[SectorType::SEC_GAS]);

  std::println(
      out_, "  {:<14}: {:>5}  [Cost: {:>4}]    {:<14}: {:>5}  [Cost: {:>4}]",
      capitalize(to_string(SectorType::SEC_ICE)), fmt_sec(SectorType::SEC_ICE),
      cost_.sector_costs[SectorType::SEC_ICE],
      capitalize(to_string(SectorType::SEC_FOREST)),
      fmt_sec(SectorType::SEC_FOREST),
      cost_.sector_costs[SectorType::SEC_FOREST]);

  std::println(out_,
               "  {:<14}: {:>5}  [Cost: {:>4}]    {:<14}: {:>5}  [Cost: {:>4}]",
               capitalize(to_string(SectorType::SEC_DESERT)),
               fmt_sec(SectorType::SEC_DESERT),
               cost_.sector_costs[SectorType::SEC_DESERT],
               capitalize(to_string(SectorType::SEC_PLATED)),
               fmt_sec(SectorType::SEC_PLATED),
               cost_.sector_costs[SectorType::SEC_PLATED]);

  std::println(out_, "  Sector Count Penalty:             [Cost: {:>4}]",
               cost_.sector_count_cost);
  std::println(out_, "  (* = common sector type on home planet)");
  std::println(out_);

  std::println(out_, "Total Cost      : {}", cost_.total_cost);
  std::println(out_, "Points Remaining: {}", cost_.points_remaining);
  std::println(out_);
}

void RacegenSession::print_help(std::string_view topic) {
  static constexpr std::array<std::string_view, 2> field_topics{
      {"fields", "modify"}};
  if (std::ranges::any_of(field_topics, [&](std::string_view t) {
        return iequals(topic, t);
      })) {
    std::println(out_, "Modifiable Fields:");
    std::println(out_, "  Attributes:");
    std::println(out_, "    adventurism (0.05 - 0.99)");
    std::println(out_, "    birthrate   (0.20 - 1.00)");
    std::println(out_, "    fighters    (1 - 20)");
    std::println(out_, "    iq          (50 - 220)");
    std::println(out_, "    mass        (0.10 - 3.00)");
    std::println(out_, "    metabolism  (0.10 - 4.00)");
    std::println(out_, "    sexes       (1 - 53)");
    std::println(out_, "    fertilize   (0% - 100%)");
    std::println(out_, "    absorb      (yes/no, metamorph only)");
    std::println(out_, "    collective  (yes/no, metamorph only)");
    std::println(out_, "    pods        (yes/no, metamorph only)");
    std::println(out_, "  Identity & World:");
    std::println(out_, "    name        (empire/race name)");
    std::println(out_, "    password    (player password)");
    std::println(out_, "    address     (email or player address)");
    std::println(out_,
                 "    planet      (Earth, Forest, Desert, Waterball, Airless, "
                 "Iceball, Jovian)");
    std::println(out_, "    race        (normal, metamorph)");
    std::println(out_, "    preferred   (preferred sector type)");
    std::println(out_, "  Sector Compatibilities (0% - 100%):");
    std::println(out_,
                 "    ocean, land, mountainous, gaseous, ice, forest, desert");
    return;
  }

  static constexpr std::array<std::string_view, 4> archetype_topics{
      {"archetype", "archetypes", "preset", "presets"}};
  if (std::ranges::any_of(archetype_topics, [&](std::string_view t) {
        return iequals(topic, t);
      })) {
    do_archetype("");
    return;
  }

  std::println(out_, "Galactic Bloodshed Race Generator Commands:");
  for (const auto& cmd : commands()) {
    std::println(out_, "  {:<32} {}", cmd.syntax, cmd.description);
  }
  std::println(out_);
  std::println(
      out_,
      "Type 'help fields' or 'help modify' to see all modifiable fields, or "
      "'archetype' to list the 11 presets.");
}

}  // namespace GB::creator
