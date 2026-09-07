// SPDX-License-Identifier: Apache-2.0

/// \file racegen_session.cc
/// \brief Interactive session implementation for player race generation.

module;

import gb.entities;
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

std::optional<PlanetType> parse_planet_type(std::string_view s) {
  const std::string lower = to_lower(trim(s));
  if (lower == "earth" || lower == "class m" || lower == "m") {
    return PlanetType::EARTH;
  }
  if (lower == "forest") return PlanetType::FOREST;
  if (lower == "desert") return PlanetType::DESERT;
  if (lower == "water" || lower == "waterball") return PlanetType::WATER;
  if (lower == "mars" || lower == "airless") return PlanetType::MARS;
  if (lower == "iceball" || lower == "ice") return PlanetType::ICEBALL;
  if (lower == "gasgiant" || lower == "jovian" || lower == "gas") {
    return PlanetType::GASGIANT;
  }
  return std::nullopt;
}

std::optional<SectorType> parse_sector_type(std::string_view s) {
  const std::string lower = to_lower(trim(s));
  if (lower == "water" || lower == "sea" || lower == "ocean") {
    return SectorType::SEC_SEA;
  }
  if (lower == "land") return SectorType::SEC_LAND;
  if (lower == "mountain" || lower == "mount" || lower == "mountainous") {
    return SectorType::SEC_MOUNT;
  }
  if (lower == "gas" || lower == "gaseous") return SectorType::SEC_GAS;
  if (lower == "ice") return SectorType::SEC_ICE;
  if (lower == "forest") return SectorType::SEC_FOREST;
  if (lower == "desert") return SectorType::SEC_DESERT;
  if (lower == "plated") return SectorType::SEC_PLATED;
  return std::nullopt;
}

std::optional<bool> parse_bool(std::string_view s) {
  const std::string lower = to_lower(trim(s));
  if (lower == "yes" || lower == "true" || lower == "1" || lower == "y") {
    return true;
  }
  if (lower == "no" || lower == "false" || lower == "0" || lower == "n") {
    return false;
  }
  return std::nullopt;
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

const std::array<RacegenSession::CommandDescriptor, 4>&
RacegenSession::commands() {
  static constexpr std::array<CommandDescriptor, 4> cmds{{
      {"modify", "modify <field> <value>",
       "Modify a race attribute, sector compatibility, or setting",
       &RacegenSession::do_modify},
      {"print", "print", "Display current specification and point costs",
       &RacegenSession::do_print},
      {"help", "help [topic]", "Show help for commands or modifiable fields",
       &RacegenSession::do_help},
      {"quit", "quit", "Exit the race generator", &RacegenSession::do_quit},
  }};
  return cmds;
}

RacegenSession::RacegenSession(std::istream& in, std::ostream& out)
    : in_(in), out_(out), engine_(), spec_(engine_.create_default_spec(false)) {
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
  } else if (field == "password" || field == "pass") {
    spec_.password = std::string(val_trimmed);
  } else if (field == "gov_password" || field == "governor_password") {
    spec_.governor_password = std::string(val_trimmed);
  } else if (field == "address" || field == "email") {
    spec_.address = std::string(val_trimmed);
  } else if (field == "planet") {
    auto ptype = parse_planet_type(val_trimmed);
    if (!ptype) {
      std::println(
          out_,
          "Error: Unknown planet type '{}'. Supported: Class M (Earth), "
          "Forest, Desert, Waterball, Airless (Mars), Iceball, Jovian.",
          val_trimmed);
      return false;
    }
    spec_.home_planet_type = *ptype;
    if (*ptype == PlanetType::GASGIANT) {
      spec_.sector_compatibilities = {};
      spec_.sector_compatibilities[SectorType::SEC_GAS] = 1.0;
      spec_.likesbest = SectorType::SEC_GAS;
      spec_.preferred_sector = SectorType::SEC_GAS;
    } else if (spec_.sector_compatibilities[SectorType::SEC_GAS] > 0.0) {
      spec_.sector_compatibilities[SectorType::SEC_GAS] = 0.0;
      spec_.sector_compatibilities[SectorType::SEC_PLATED] = 1.0;
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
      if (spec_.iq == 0 && spec_.iq_limit > 0) {
        spec_.iq = spec_.iq_limit;
        spec_.iq_limit = 0;
      }
    } else if (rtype == "metamorph") {
      spec_.metamorph = true;
      spec_.absorb = true;
      spec_.pods = true;
      spec_.collective_iq = true;
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
  } else if (field == "adventurism" || field == "advent") {
    if (auto num = parse_number<double>(val_trimmed)) {
      spec_.adventurism = *num;
    } else {
      std::println(out_, "Error: Invalid number '{}' for adventurism.",
                   val_trimmed);
      return false;
    }
  } else if (field == "absorb") {
    if (auto b = parse_bool(val_trimmed)) {
      spec_.absorb = *b;
    } else {
      std::println(out_, "Error: Invalid boolean '{}' (use yes/no).",
                   val_trimmed);
      return false;
    }
  } else if (field == "birthrate" || field == "birth") {
    if (auto num = parse_number<double>(val_trimmed)) {
      spec_.birthrate = *num;
    } else {
      std::println(out_, "Error: Invalid number '{}' for birthrate.",
                   val_trimmed);
      return false;
    }
  } else if (field == "collective_iq" || field == "col_iq" ||
             field == "collective") {
    if (auto b = parse_bool(val_trimmed)) {
      spec_.collective_iq = *b;
    } else {
      std::println(out_, "Error: Invalid boolean '{}' (use yes/no).",
                   val_trimmed);
      return false;
    }
  } else if (field == "fertilize" || field == "fert") {
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
  } else if (field == "iq_limit") {
    if (auto num = parse_number<int>(val_trimmed)) {
      spec_.iq_limit = *num;
    } else {
      std::println(out_, "Error: Invalid integer '{}' for IQ limit.",
                   val_trimmed);
      return false;
    }
  } else if (field == "fighters" || field == "fight") {
    if (auto num = parse_number<int>(val_trimmed)) {
      spec_.fighters = static_cast<fighters_t>(*num);
    } else {
      std::println(out_, "Error: Invalid integer '{}' for fighters.",
                   val_trimmed);
      return false;
    }
  } else if (field == "pods") {
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
  } else if (field == "sexes" || field == "sex") {
    if (auto num = parse_number<int>(val_trimmed)) {
      spec_.number_sexes = static_cast<sexes_t>(*num);
    } else {
      std::println(out_, "Error: Invalid integer '{}' for sexes.", val_trimmed);
      return false;
    }
  } else if (field == "metabolism" || field == "metab") {
    if (auto num = parse_number<double>(val_trimmed)) {
      spec_.metabolism = *num;
    } else {
      std::println(out_, "Error: Invalid number '{}' for metabolism.",
                   val_trimmed);
      return false;
    }
  } else if (field == "likesbest" || field == "preferred" ||
             field == "preferred_sector") {
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
  auto errors = engine_.validate(spec_, /*is_player=*/true, /*rigorous=*/false);
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
               to_string(spec_.home_planet_type), cost_.planet_cost);
  std::println(out_, "  Race Type     : {:<18} [Cost: {:>4}]",
               spec_.metamorph ? "Metamorph" : "Normal", cost_.race_type_cost);
  std::println(out_);

  std::println(out_, "=== Attributes ===");
  auto fmt_bool = [](bool b) { return b ? "yes" : "no"; };

  std::println(out_,
               "  {:<14}: {:>7.2f}  [Cost: {:>4}]    {:<14}: {:>7}  [Cost: "
               "{:>4}]",
               "Adventurism", spec_.adventurism,
               static_cast<int>(cost_.adventurism()), "Absorb",
               fmt_bool(spec_.absorb), static_cast<int>(cost_.absorb()));

  std::println(out_,
               "  {:<14}: {:>7.2f}  [Cost: {:>4}]    {:<14}: {:>7}  [Cost: "
               "{:>4}]",
               "Birthrate", spec_.birthrate,
               static_cast<int>(cost_.birthrate()), "Collective IQ",
               fmt_bool(spec_.collective_iq),
               static_cast<int>(cost_.collective_iq()));

  std::string iq_name = spec_.collective_iq ? "IQ Limit" : "IQ";
  int iq_val = spec_.collective_iq ? spec_.iq_limit : spec_.iq;
  std::println(out_,
               "  {:<14}: {:>6}%   [Cost: {:>4}]    {:<14}: {:>7}  [Cost: "
               "{:>4}]",
               "Fertilize", static_cast<int>(spec_.fertilize),
               static_cast<int>(cost_.fertilize()), iq_name, iq_val,
               static_cast<int>(cost_.iq()));

  std::println(out_,
               "  {:<14}: {:>7}   [Cost: {:>4}]    {:<14}: {:>7}  [Cost: "
               "{:>4}]",
               "Fighters", static_cast<int>(spec_.fighters),
               static_cast<int>(cost_.fight()), "Pods", fmt_bool(spec_.pods),
               static_cast<int>(cost_.pods()));

  std::println(out_,
               "  {:<14}: {:>7.2f}  [Cost: {:>4}]    {:<14}: {:>7}  [Cost: "
               "{:>4}]",
               "Mass", spec_.mass, static_cast<int>(cost_.mass()), "Sexes",
               static_cast<int>(spec_.number_sexes),
               static_cast<int>(cost_.sexes()));

  std::println(out_, "  {:<14}: {:>7.2f}  [Cost: {:>4}]", "Metabolism",
               spec_.metabolism, static_cast<int>(cost_.metabolism()));
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
      static_cast<int>(cost_.sector_costs[SectorType::SEC_SEA]),
      capitalize(to_string(SectorType::SEC_LAND)),
      fmt_sec(SectorType::SEC_LAND),
      static_cast<int>(cost_.sector_costs[SectorType::SEC_LAND]));

  std::println(
      out_, "  {:<14}: {:>5}  [Cost: {:>4}]    {:<14}: {:>5}  [Cost: {:>4}]",
      capitalize(to_string(SectorType::SEC_MOUNT)),
      fmt_sec(SectorType::SEC_MOUNT),
      static_cast<int>(cost_.sector_costs[SectorType::SEC_MOUNT]),
      capitalize(to_string(SectorType::SEC_GAS)), fmt_sec(SectorType::SEC_GAS),
      static_cast<int>(cost_.sector_costs[SectorType::SEC_GAS]));

  std::println(
      out_, "  {:<14}: {:>5}  [Cost: {:>4}]    {:<14}: {:>5}  [Cost: {:>4}]",
      capitalize(to_string(SectorType::SEC_ICE)), fmt_sec(SectorType::SEC_ICE),
      static_cast<int>(cost_.sector_costs[SectorType::SEC_ICE]),
      capitalize(to_string(SectorType::SEC_FOREST)),
      fmt_sec(SectorType::SEC_FOREST),
      static_cast<int>(cost_.sector_costs[SectorType::SEC_FOREST]));

  std::println(out_,
               "  {:<14}: {:>5}  [Cost: {:>4}]    {:<14}: {:>5}  [Cost: {:>4}]",
               capitalize(to_string(SectorType::SEC_DESERT)),
               fmt_sec(SectorType::SEC_DESERT),
               static_cast<int>(cost_.sector_costs[SectorType::SEC_DESERT]),
               capitalize(to_string(SectorType::SEC_PLATED)),
               fmt_sec(SectorType::SEC_PLATED),
               static_cast<int>(cost_.sector_costs[SectorType::SEC_PLATED]));

  std::println(out_, "  Sector Count Penalty:             [Cost: {:>4}]",
               cost_.sector_count_cost);
  std::println(out_, "  (* = common sector type on home planet)");
  std::println(out_);

  std::println(out_, "Total Cost      : {}", cost_.total_cost);
  std::println(out_, "Points Remaining: {}", cost_.points_remaining);
  std::println(out_);
}

void RacegenSession::print_help(std::string_view topic) {
  if (iequals(topic, "fields") || iequals(topic, "modify")) {
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
    std::println(
        out_, "    planet      (Class M, Forest, Desert, Waterball, Airless, "
              "Iceball, Jovian)");
    std::println(out_, "    race        (normal, metamorph)");
    std::println(out_, "    preferred   (preferred sector type)");
    std::println(out_, "  Sector Compatibilities (0% - 100%):");
    std::println(out_,
                 "    ocean, land, mountainous, gaseous, ice, forest, desert");
    return;
  }

  std::println(out_, "Galactic Bloodshed Race Generator Commands:");
  for (const auto& cmd : commands()) {
    std::println(out_, "  {:<24} {}", cmd.syntax, cmd.description);
  }
  std::println(out_);
  std::println(
      out_,
      "Type 'help fields' or 'help modify' to see all modifiable fields.");
}

}  // namespace GB::creator
