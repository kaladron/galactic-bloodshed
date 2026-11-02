// SPDX-License-Identifier: Apache-2.0

/// \file makeuniv.cc
/// \brief Universe creation command-line utility.

#include <unistd.h>
#include <cstdio>
#include <cstdlib>

import std;
import dallib;
import gb.entities;
import gb.services;
import gb.creator;

namespace {

std::optional<int> parse_int(std::string_view s) noexcept {
  int val = 0;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
  if (ec == std::errc{} && ptr == s.data() + s.size()) {
    return val;
  }
  return std::nullopt;
}

bool is_valid_token(std::string_view s) noexcept {
  return !s.empty() && !s.contains(' ');
}

}  // namespace

int main(int argc, char* argv[]) {
  std::string db_path = PKGSTATEDIR "gb.db";
  GB::creator::UniverseConfig config{};
  bool interactive = true;

  bool star_names_set = false;
  bool planet_names_set = false;
  bool num_stars_set = false;
  bool min_planets_set = false;
  bool max_planets_set = false;

  for (int i = 1; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      goto usage;
    }
    if (arg == "-v" || arg == "--version") {
      std::println(std::cout, "makeuniv {}", GB_VERSION);
      return 0;
    }
    if (arg == "--print-planets") {
      config.print_planet_info = true;
      continue;
    }
    if (arg == "--print-stars") {
      config.print_star_info = true;
      continue;
    }
    if (arg == "--deity-password") {
      if (i + 1 >= argc) {
        std::println(std::cerr, "Option \"{}\" requires an argument.", arg);
        return 1;
      }
      config.deity_password = argv[++i];
      continue;
    }
    if (arg.starts_with("--deity-password=")) {
      config.deity_password =
          arg.substr(std::string_view("--deity-password=").size());
      continue;
    }
    if (arg == "--deity-gov-password") {
      if (i + 1 >= argc) {
        std::println(std::cerr, "Option \"{}\" requires an argument.", arg);
        return 1;
      }
      config.deity_gov_password = argv[++i];
      continue;
    }
    if (arg.starts_with("--deity-gov-password=")) {
      config.deity_gov_password =
          arg.substr(std::string_view("--deity-gov-password=").size());
      continue;
    }
    if (arg == "--database" || arg == "--db" || arg == "-D") {
      if (i + 1 >= argc) {
        std::println(std::cerr, "Option \"{}\" requires an argument.", arg);
        return 1;
      }
      db_path = argv[++i];
    } else if (arg.starts_with("--database=")) {
      db_path = arg.substr(std::string_view("--database=").size());
    } else if (arg.starts_with("--db=")) {
      db_path = arg.substr(std::string_view("--db=").size());
    } else if (argv[i][0] != '-') {
      goto usage;
    } else {
      switch (argv[i][1]) {
        case 'a':
          config.auto_name_stars = true;
          star_names_set = true;
          break;
        case 'b':
          config.auto_name_planets = true;
          planet_names_set = true;
          break;
        case 'e':
          if (i + 1 >= argc) {
            std::println(std::cerr, "Option \"-e\" requires an argument.");
            return 1;
          }
          config.planetless_chance_percent = std::atoi(argv[++i]);
          break;
        case 'l':
          if (i + 1 >= argc) {
            std::println(std::cerr, "Option \"-l\" requires an argument.");
            return 1;
          }
          config.min_planets = std::atoi(argv[++i]);
          min_planets_set = true;
          break;
        case 'm':
          if (i + 1 >= argc) {
            std::println(std::cerr, "Option \"-m\" requires an argument.");
            return 1;
          }
          config.max_planets = std::atoi(argv[++i]);
          max_planets_set = true;
          break;
        case 's':
          if (i + 1 >= argc) {
            std::println(std::cerr, "Option \"-s\" requires an argument.");
            return 1;
          }
          config.num_stars = static_cast<starnum_t>(std::atoi(argv[++i]));
          num_stars_set = true;
          break;
        case 'p':
          config.print_planet_info = true;
          break;
        case 'w':
          config.print_star_info = true;
          break;
        case 'd':
          interactive = false;
          break;
        default:
          std::println(std::cout, "Unknown option \"{}\".\n", argv[i]);
usage:
          std::println(std::cout,
                       "Usage: makeuniv [-a] [-b] [-d] [-e E] [-l MIN] [-m "
                       "MAX] [-s N] [-p|--print-planets] "
                       "[-w|--print-stars] [--deity-password <pass>] "
                       "[--deity-gov-password <pass>] [-D|--database|--db "
                       "<path>] [-v|--version] [-h|--help]");
          std::println(std::cout,
                       "  -a                         Autoload star names.");
          std::println(std::cout,
                       "  -b                         Autoload planet names.");
          std::println(std::cout,
                       "  -d                         Use all defaults and "
                       "autoloaded names (requires deity passwords).");
          std::println(
              std::cout,
              "  -e E                       Make E% of stars have no planets.");
          std::println(std::cout,
                       "  -l MIN                     Other systems will have "
                       "at least MIN planets.");
          std::println(std::cout,
                       "  -m MAX                     Other systems will have "
                       "at most  MAX planets.");
          std::println(
              std::cout,
              "  -s S                       The universe will have S stars.");
          std::println(std::cout,
                       "  -p, --print-planets        Print info and map of "
                       "planets generated.");
          std::println(
              std::cout,
              "  -w, --print-stars          Print info on stars generated.");
          std::println(
              std::cout,
              "  --deity-password <pass>    Password for the Deity race.");
          std::println(
              std::cout,
              "  --deity-gov-password <pass> Password for the Deity leader.");
          std::println(std::cout,
                       "  -D, --database, --db <path> Path to SQLite database "
                       "(default: " PKGSTATEDIR "gb.db)");
          std::println(std::cout,
                       "  -v, --version              Display version "
                       "information and exit.");
          std::println(std::cout,
                       "  -h, --help                 Display this help message "
                       "and exit.\n");
          return 0;
      }
    }
  }

  if (!interactive) {
    config.auto_name_stars = true;
    config.auto_name_planets = true;
    config.print_planet_info = true;
    config.print_star_info = true;
    if (!num_stars_set) config.num_stars = 128;
    if (!min_planets_set) config.min_planets = 1;
    if (!max_planets_set) config.max_planets = 10;

    if (!is_valid_token(config.deity_password) ||
        !is_valid_token(config.deity_gov_password)) {
      std::println(std::cerr,
                   "Error: Non-interactive mode (-d) requires valid "
                   "--deity-password and "
                   "--deity-gov-password (non-empty and no spaces).");
      return 1;
    }
  } else {
    std::string line;
    if (!star_names_set) {
      std::print("\nDo you wish to use the file \"{}\" for star names? [y/n]> ",
                 config.star_names_file);
      if (!std::getline(std::cin, line)) return 1;
      config.auto_name_stars =
          (!line.empty() && (line[0] == 'y' || line[0] == 'Y'));
    }
    if (!planet_names_set) {
      std::print(
          "\nDo you wish to use the file \"{}\" for planet names? [y/n]> ",
          config.planet_names_file);
      if (!std::getline(std::cin, line)) return 1;
      config.auto_name_planets =
          (!line.empty() && (line[0] == 'y' || line[0] == 'Y'));
    }
    if (!num_stars_set) {
      while (true) {
        std::print("Number of stars [1-255]: ");
        if (!std::getline(std::cin, line)) return 1;
        auto val = parse_int(line);
        if (val.has_value() && *val >= 1 && *val <= 255) {
          config.num_stars = static_cast<starnum_t>(*val);
          break;
        }
      }
    }
    if (!min_planets_set) {
      while (true) {
        std::print("Minimum number of planets per system [1-10]: ");
        if (!std::getline(std::cin, line)) return 1;
        auto val = parse_int(line);
        if (val.has_value() && *val >= 1 && *val <= 10) {
          config.min_planets = *val;
          break;
        }
      }
    }
    if (!max_planets_set) {
      while (true) {
        std::print("Maximum number of planets per system [{}-10]: ",
                   config.min_planets);
        if (!std::getline(std::cin, line)) return 1;
        auto val = parse_int(line);
        if (val.has_value() && *val >= config.min_planets && *val <= 10) {
          config.max_planets = *val;
          break;
        }
      }
    }
    while (!is_valid_token(config.deity_password)) {
      std::print("Enter the password for the Deity race: ");
      if (!std::getline(std::cin, config.deity_password)) return 1;
    }
    while (!is_valid_token(config.deity_gov_password)) {
      std::print("Enter the password for the Deity leader: ");
      if (!std::getline(std::cin, config.deity_gov_password)) return 1;
    }
  }

  Database db(db_path);
  GB::creator::UniverseGenerator generator(config);
  auto result = generator.generate(db);

  std::println(std::cout,
               "\nUniverse successfully generated in \"{}\":", db_path);
  std::println(std::cout, "  Stars created: {}", result.num_stars);
  std::println(std::cout, "  Planets created (non-asteroid): {}",
               result.planet_count);
  std::println(std::cout, "  Total resources seeded: {}",
               result.total_resources);

  return 0;
}
