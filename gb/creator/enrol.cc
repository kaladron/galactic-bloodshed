// SPDX-License-Identifier: Apache-2.0

/// \file enrol.cc
/// \brief Player race enrollment CLI executable.

import std;
import gb.entities;
import gb.services;
import gb.creator;
import dallib;
import scnlib;
#undef stdout

int main(int argc, char* argv[]) {
  using namespace GB::creator;

  std::string db_path = PKGSTATEDIR "gb.db";
  std::optional<std::filesystem::path> spec_file;

  for (int i = 1; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      std::println(std::cout, "Usage: enrol [options]");
      std::println(std::cout, "");
      std::println(std::cout, "Options:");
      std::println(std::cout,
                   "  -d, --database, --db <path> Path to SQLite database "
                   "(default: {}gb.db)",
                   PKGSTATEDIR);
      std::println(std::cout,
                   "  -f, --file [path]           Enroll directly from a JSON "
                   "race specification file (default: {})",
                   DEFAULT_RACEGEN_FILENAME);
      std::println(std::cout,
                   "  -h, --help                  Display this help message "
                   "and exit");
      return 0;
    }
    if (arg == "-d" || arg == "--database" || arg == "--db") {
      if (i + 1 >= argc) {
        std::println(std::cerr, "Error: Option \"{}\" requires an argument.",
                     arg);
        return 1;
      }
      db_path = argv[++i];
    } else if (arg.starts_with("--database=")) {
      db_path = arg.substr(std::string_view("--database=").size());
    } else if (arg.starts_with("--db=")) {
      db_path = arg.substr(std::string_view("--db=").size());
    } else if (arg == "-f" || arg == "--file") {
      if (i + 1 < argc && !std::string_view(argv[i + 1]).starts_with("-")) {
        spec_file = argv[++i];
      } else {
        spec_file = DEFAULT_RACEGEN_FILENAME;
      }
    } else if (arg.starts_with("--file=")) {
      spec_file = arg.substr(std::string_view("--file=").size());
    } else {
      std::println(std::cerr, "Unknown option \"{}\".", arg);
      std::println(
          std::cerr,
          "Usage: enrol [-d|--database|--db <path>] [-f|--file [path]] "
          "[-h|--help]");
      return 1;
    }
  }

  // Create Database, EntityManager, and EnrollmentService
  Database database{db_path};
  EntityManager entity_manager{database};
  EnrollmentService service{entity_manager};

  // Direct non-interactive enrollment from JSON file
  if (spec_file) {
    auto loaded = load_race_spec(*spec_file);
    if (!loaded) {
      std::println(std::cerr, "Error: {}", loaded.error());
      return 1;
    }
    auto result = service.enroll_player(*loaded);
    if (!result.success) {
      std::println(std::cerr, "Error: Enrollment failed - {}", result.message);
      return 1;
    }
    std::println(std::cout, "\nYou are player {}.\n", result.player_num);
    std::println(std::cout, "Your race has been created on sector {},{} on",
                 result.capital_coords.x, result.capital_coords.y);
    entity_manager.with_star(result.star, [&](const Star& home_star) {
      std::println(std::cout, "{}/{}.\n", home_star.get_name(),
                   home_star.get_planet_name(result.pnum));
    });
    return 0;
  }

  // Interactive quick-start enrollment wizard
  player_t Playernum{entity_manager.num_races().value + 1};
  if (Playernum >= player_t{MAXPLAYERS}) {
    std::println(std::cout, "There are already {} players; No more allowed.",
                 MAXPLAYERS - 1);
    return -1;
  }

  const auto* universe_ptr = entity_manager.peek_universe();
  if (!universe_ptr) {
    std::println(std::cerr, "Error: Cannot load universe data");
    return -1;
  }
  std::println(std::cout, "There is still space for player {}.", Playernum);

  std::println(std::cout, "\n=== Available Racial Archetypes ===\n");
  std::cout << create_archetypes_table() << "\n\n";

  std::print("Enter racial type to be created (1-{}): ",
             race_archetypes.size());
  std::string input_line;
  std::getline(std::cin, input_line);
  auto idx_result = scn::scan<std::size_t>(input_line, "{}");
  if (!idx_result) {
    std::println(std::cerr, "Error: Cannot read input - {}",
                 idx_result.error().msg());
    return -1;
  }
  std::size_t chosen_idx = idx_result->value();

  if (chosen_idx < 1 || chosen_idx > race_archetypes.size()) {
    std::println(std::cout, "Bad racial index.");
    return 1;
  }
  const auto& archetype = race_archetypes[chosen_idx - 1];

  std::print("Enter the name of this race [{}]: ", archetype.name);
  std::string race_name;
  std::getline(std::cin, race_name);
  if (race_name.empty()) {
    race_name = std::string(archetype.name);
  }

  std::print("Enter the password for this race: ");
  std::string race_password;
  std::getline(std::cin, race_password);

  std::print("Enter the password for this leader: ");
  std::string gov_password;
  std::getline(std::cin, gov_password);

  std::print("Enter your email address [player@localhost]: ");
  std::string email_address;
  std::getline(std::cin, email_address);
  if (email_address.empty()) {
    email_address = "player@localhost";
  }

  std::print("\n\tDeity/Guest/Normal (d/g/n) [{}]? ",
             Playernum == 1 ? 'd' : 'n');
  std::string deity_line;
  std::getline(std::cin, deity_line);
  char role_char =
      (!deity_line.empty()) ? deity_line[0] : (Playernum == 1 ? 'd' : 'n');
  bool is_god = (role_char == 'd');
  bool is_guest = (role_char == 'g');

  starnum_t star{0};
  planetnum_t pnum{0};
  PlanetType ppref = archetype.default_planet;
  bool found = false;
  std::set<PlanetType> exhausted_planet_types;

  do {
    std::print(
        "\nLive on what type planet (default: {}):\n     (e)arth, (g)asgiant, "
        "(m)ars, (i)ce, (w)ater, (d)esert, (f)orest? ",
        Planet_types[archetype.default_planet]);
    std::string planet_line;
    std::getline(std::cin, planet_line);
    char c = (!planet_line.empty()) ? planet_line[0] : '\0';

    if (c == '\0') {
      ppref = archetype.default_planet;
    } else {
      switch (c) {
        case 'w':
          ppref = PlanetType::WATER;
          break;
        case 'e':
          ppref = PlanetType::EARTH;
          break;
        case 'm':
          ppref = PlanetType::MARS;
          break;
        case 'g':
          ppref = PlanetType::GASGIANT;
          break;
        case 'i':
          ppref = PlanetType::ICEBALL;
          break;
        case 'd':
          ppref = PlanetType::DESERT;
          break;
        case 'f':
          ppref = PlanetType::FOREST;
          break;
        default:
          std::println(std::cout, "Oh well.");
          return -1;
      }
    }

    std::println(std::cout, "Looking for type {} planet...",
                 Planet_types[ppref]);

    found = false;
    if (auto found_loc = service.find_suitable_planet(ppref)) {
      star = found_loc->first;
      pnum = found_loc->second;
      found = true;
    }

    if (!found) {
      std::println(std::cout, "planet type not found in any free systems.");
      exhausted_planet_types.insert(ppref);
      if (exhausted_planet_types.size() >= habitable_planet_types.size()) {
        std::println(std::cout,
                     "Looks like there aren't any free planets left.  bye..");
        return -1;
      }
      std::println(std::cout, "  Try a different one...");
    }
  } while (!found);

  RacegenEngine engine;
  RaceEnrollmentSpec spec;
  char ok_char = '\0';
  do {
    spec = archetype.to_enrollment_spec(ppref, /*randomize=*/true);
    spec.name = race_name;
    spec.password = race_password;
    spec.governor_password = gov_password;
    spec.address = email_address;
    spec.is_god = is_god;
    spec.is_guest = is_guest;
    spec.target_planet = std::make_pair(star, pnum);

    const auto cost = engine.calculate_cost(spec);

    std::println(std::cout,
                 "\n=== Sampled Race Specification ({}) ===", archetype.name);
    if (spec.metamorph) {
      std::println(std::cout, "       Race Type: METAMORPHIC (Absorb, Pods)");
      std::println(std::cout, "        IQ Limit: {}", spec.iq_limit);
    } else {
      std::println(std::cout, "       Race Type: Normal");
      std::println(std::cout, "              IQ: {}", spec.iq);
    }
    std::println(std::cout, "       Birthrate: {:.3f}", spec.birthrate);
    std::println(std::cout, "Fighting ability: {}", spec.fighters);
    std::println(std::cout, "      Metabolism: {:.2f}", spec.metabolism);
    std::println(std::cout, "     Adventurism: {:.2f}", spec.adventurism);
    std::println(std::cout, "            Mass: {:.2f}", spec.mass);
    std::println(std::cout, " Number of sexes: {} (min req'd for colonization)",
                 spec.number_sexes);
    std::println(std::cout, "     Home Planet: {}", Planet_types[ppref]);

    std::print("  Sector Compats: ");
    bool first = true;
    for (auto [st, compat] : spec.sector_compatibilities.settleable()) {
      if (compat > 0.0) {
        if (!first) std::print(", ");
        std::print("{} {:.0f}%", Desnames[st], compat * 100.0);
        first = false;
      }
    }
    std::println(std::cout, "");
    std::println(std::cout, "      Total Cost: {} / {} pts ({} remaining)",
                 cost.total_cost, STARTING_POINTS, cost.points_remaining);

    std::print("\nLook OK(y/n)? ");
    std::string ok_line;
    std::getline(std::cin, ok_line);
    ok_char = (!ok_line.empty()) ? ok_line[0] : '\0';
  } while (ok_char != 'y');

  if (auto save_res = save_race_spec(spec); !save_res) {
    std::println(std::cerr, "Warning: Could not save {}: {}",
                 DEFAULT_RACEGEN_FILENAME, save_res.error());
  } else {
    std::println(std::cout, "Saved race specification to '{}'.",
                 DEFAULT_RACEGEN_FILENAME);
  }

  auto result = service.enroll_player(spec);
  if (!result.success) {
    std::println(std::cerr, "Error: Enrollment failed - {}", result.message);
    return -1;
  }

  std::println(std::cout, "\nYou are player {}.\n", result.player_num);
  std::println(std::cout, "Your race has been created on sector {},{} on",
               result.capital_coords.x, result.capital_coords.y);
  entity_manager.with_star(star, [&](const Star& home_star) {
    std::println(std::cout, "{}/{}.\n", home_star.get_name(),
                 home_star.get_planet_name(pnum));
  });
  return 0;
}
