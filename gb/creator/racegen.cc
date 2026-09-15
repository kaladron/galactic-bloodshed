// SPDX-License-Identifier: Apache-2.0

/// \file racegen.cc
/// \brief Interactive race generator CLI tool.

import std;
import gb.entities;
import gb.services;
import gb.creator;
import dallib;

int main(int argc, char* argv[]) {
  std::string db_path = PKGSTATEDIR "gb.db";
  bool db_path_specified = false;
  std::optional<std::filesystem::path> spec_file;
  std::optional<std::string> archetype_arg;

  for (int i = 1; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      std::println(std::cout, "Usage: racegen [options]");
      std::println(std::cout, "");
      std::println(std::cout, "Options:");
      std::println(std::cout,
                   "  -d, --database, --db <path> Path to SQLite database "
                   "(default: {}gb.db)",
                   PKGSTATEDIR);
      std::println(std::cout,
                   "  -f, --file [path]           Load race specification from "
                   "JSON file (default: {})",
                   GB::creator::DEFAULT_RACEGEN_FILENAME);
      std::println(
          std::cout,
          "  -a, --archetype <id|name>   Pre-load one of the 11 preset "
          "evolutionary archetypes");
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
      db_path_specified = true;
    } else if (arg.starts_with("--database=")) {
      db_path = arg.substr(std::string_view("--database=").size());
      db_path_specified = true;
    } else if (arg.starts_with("--db=")) {
      db_path = arg.substr(std::string_view("--db=").size());
      db_path_specified = true;
    } else if (arg == "-f" || arg == "--file") {
      if (i + 1 < argc && !std::string_view(argv[i + 1]).starts_with("-")) {
        spec_file = argv[++i];
      } else {
        spec_file = GB::creator::DEFAULT_RACEGEN_FILENAME;
      }
    } else if (arg.starts_with("--file=")) {
      spec_file = arg.substr(std::string_view("--file=").size());
    } else if (arg == "-a" || arg == "--archetype") {
      if (i + 1 >= argc) {
        std::println(std::cerr, "Error: Option \"{}\" requires an argument.",
                     arg);
        return 1;
      }
      archetype_arg = argv[++i];
    } else if (arg.starts_with("--archetype=")) {
      archetype_arg = arg.substr(std::string_view("--archetype=").size());
    } else {
      std::println(std::cerr, "Unknown option \"{}\".", arg);
      std::println(
          std::cerr,
          "Usage: racegen [-d|--database|--db <path>] [-f|--file [path]] "
          "[-a|--archetype <id|name>] [-h|--help]");
      return 1;
    }
  }

  std::optional<Database> db;
  std::optional<EntityManager> em;
  std::optional<GB::creator::EnrollmentService> service;

  if (std::filesystem::exists(db_path)) {
    db.emplace(db_path);
    em.emplace(*db);
    service.emplace(*em);
  } else if (db_path_specified) {
    std::println(std::cerr,
                 "Warning: Database '{}' not found. Running in offline design "
                 "mode (enrollment disabled).",
                 db_path);
  }

  GB::creator::RacegenSession session(std::cin, std::cout,
                                      service ? &*service : nullptr);
  if (spec_file) {
    if (!session.load_from_file(*spec_file)) {
      return 1;
    }
    std::println(std::cout, "Loaded specification from '{}'.",
                 spec_file->string());
  }
  if (archetype_arg) {
    if (!session.apply_archetype(*archetype_arg, /*randomize=*/false)) {
      return 1;
    }
    const auto* arch = GB::creator::find_archetype(*archetype_arg);
    std::println(std::cout, "Pre-loaded archetype '{}' ({} points remaining).",
                 arch->name, session.cost().points_remaining);
  }
  session.run();
  return 0;
}
