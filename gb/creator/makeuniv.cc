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

int main(int argc, char* argv[]) {
  std::string db_path = PKGSTATEDIR "gb.db";
  GB::creator::UniverseConfig config{};
  bool interactive = true;

  for (int i = 1; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      goto usage;
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
          break;
        case 'b':
          config.auto_name_planets = true;
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
          config.min_planets = static_cast<planetnum_t>(std::atoi(argv[++i]));
          break;
        case 'm':
          if (i + 1 >= argc) {
            std::println(std::cerr, "Option \"-m\" requires an argument.");
            return 1;
          }
          config.max_planets = static_cast<planetnum_t>(std::atoi(argv[++i]));
          break;
        case 's':
          if (i + 1 >= argc) {
            std::println(std::cerr, "Option \"-s\" requires an argument.");
            return 1;
          }
          config.num_stars = static_cast<starnum_t>(std::atoi(argv[++i]));
          break;
        case 'v':
          config.print_planet_info = true;
          break;
        case 'w':
          config.print_star_info = true;
          break;
        case 'd':
          config.auto_name_stars = true;
          config.auto_name_planets = true;
          config.print_planet_info = true;
          config.print_star_info = true;
          config.num_stars = 128;
          config.min_planets = 1;
          config.max_planets = 10;
          interactive = false;
          break;
        default:
          std::println(std::cout, "Unknown option \"{}\".\n", argv[i]);
usage:
          std::println(std::cout,
                       "Usage: makeuniv [-a] [-b] [-d] [-e E] [-l MIN] [-m "
                       "MAX] [-s N] [-v] "
                       "[-w] [-D|--database|--db <path>] [-h|--help]");
          std::println(std::cout,
                       "  -a                         Autoload star names.");
          std::println(std::cout,
                       "  -b                         Autoload planet names.");
          std::println(std::cout, "  -d                         Use all "
                                  "defaults and autoloaded names.");
          std::println(
              std::cout,
              "  -e E                       Make E% of stars have no planets.");
          std::println(std::cout, "  -l MIN                     Other systems "
                                  "will have at least MIN planets.");
          std::println(std::cout, "  -m MAX                     Other systems "
                                  "will have at most  MAX planets.");
          std::println(
              std::cout,
              "  -s S                       The universe will have S stars.");
          std::println(std::cout, "  -v                         Print info and "
                                  "map of planets generated.");
          std::println(
              std::cout,
              "  -w                         Print info on stars generated.");
          std::println(std::cout,
                       "  -D, --database, --db <path> Path to SQLite database "
                       "(default: " PKGSTATEDIR "gb.db)");
          std::println(std::cout, "  -h, --help                 Display this "
                                  "help message and exit.\n");
          return 0;
      }
    }
  }

  // Interactive prompts if not running in default mode and values unconfigured
  if (interactive) {
    if (!config.auto_name_stars) {
      std::print("\nDo you wish to use the file \"{}\" for star names? [y/n]> ",
                 config.star_names_file);
      int c = std::getchar();
      if (c != '\n') std::getchar();
      config.auto_name_stars = (c == 'y');
    }
    if (!config.auto_name_planets) {
      std::print(
          "\nDo you wish to use the file \"{}\" for planet names? [y/n]> ",
          config.planet_names_file);
      int c = std::getchar();
      if (c != '\n') std::getchar();
      config.auto_name_planets = (c == 'y');
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
