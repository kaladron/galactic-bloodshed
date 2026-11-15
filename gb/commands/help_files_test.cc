// SPDX-License-Identifier: Apache-2.0

/// \file help_files_test.cc
/// \brief Unit tests verifying help documentation files exist in HELP_DIR, are
/// readable markdown, and have valid headers.

import commands;
import gb.entities;
import test;
import std;

std::filesystem::path get_help_dir() {
  if (const char* env_help = std::getenv("GB_HELPDIR");
      env_help && *env_help != '\0') {
    return env_help;
  }
  std::filesystem::path help_dir{HELP_DIR};
  if (!std::filesystem::exists(help_dir)) {
    for (const auto& dev_dir : {"help", "../help", "../../help"}) {
      if (std::filesystem::exists(std::filesystem::path(dev_dir) / "help.md")) {
        return dev_dir;
      }
    }
  }
  return help_dir;
}

// Test that help files exist in HELP_DIR with .md extension
void test_help_files_exist() {
  std::println(std::cout, "Test: Help files exist with .md extension");

  std::filesystem::path help_dir = get_help_dir();

  // Check that the help directory exists
  test::expect_true(std::filesystem::exists(help_dir));
  test::expect_true(std::filesystem::is_directory(help_dir));
  std::println(std::cout, "  ✓ HELP_DIR exists: {}", help_dir.string());

  // Count .md files
  int md_count = 0;
  for (const auto& entry : std::filesystem::directory_iterator(help_dir)) {
    if (entry.path().extension() == ".md") {
      md_count++;
    }
  }
  test::expect_gt(md_count, 0);
  std::println(std::cout, "  ✓ Found {} .md help files", md_count);
}

// Test that specific help files can be opened and read
void test_help_file_readable() {
  std::println(std::cout, "Test: Help files can be opened and read");

  auto help_dir = get_help_dir();

  // Test a few common help files
  std::vector<std::string> test_files = {"help", "build", "cs", "map", "orbit"};

  for (const auto& name : test_files) {
    std::string filepath = std::format("{}/{}.md", help_dir.string(), name);

    std::ifstream file(filepath);
    test::expect_true(file.is_open());

    // Read first line to verify content
    std::string first_line;
    std::getline(file, first_line);
    test::expect_false(first_line.empty());

    // Verify the first line contains the expected markdown header
    test::expect_eq(first_line[0], '#');

    file.close();
    std::println(std::cout, "  ✓ {} readable, starts with: {}", name,
                 first_line.substr(0, 20));
  }
}

// Test that help file format is correct (markdown headers)
void test_help_file_format() {
  std::println(std::cout, "Test: Help files have proper markdown format");

  auto help_dir = get_help_dir();
  std::string filepath = std::format("{}/build.md", help_dir.string());

  std::ifstream file(filepath);
  test::expect_true(file.is_open());

  std::string line;
  bool found_title = false;
  bool found_section = false;

  while (std::getline(file, line)) {
    // Check for title (# TITLE)
    if (line.size() >= 2 && line[0] == '#' && line[1] == ' ') {
      found_title = true;
    }
    // Check for section header (## Section)
    if (line.size() >= 3 && line[0] == '#' && line[1] == '#' &&
        line[2] == ' ') {
      found_section = true;
    }
  }

  file.close();

  test::expect_true(found_title);
  test::expect_true(found_section);
  std::println(std::cout, "  ✓ build.md has proper markdown structure");
}

// Test that requesting a non-existent help topic fails gracefully
void test_nonexistent_help_file() {
  std::println(std::cout, "Test: Non-existent help file returns null");

  auto help_dir = get_help_dir();
  std::string filepath =
      std::format("{}/this_topic_does_not_exist.md", help_dir.string());

  std::ifstream file(filepath);
  test::expect_false(file.is_open());
  std::println(std::cout, "  ✓ Non-existent help file correctly not found");
}

void test_all_help_headers_links_and_registry_parity() {
  std::println(std::cout,
               "Test: All help files have H1 headers, valid links, and mortal "
               "command parity");

  const auto help_dir = get_help_dir();
  std::set<std::string> topics;
  for (const auto& entry : std::filesystem::directory_iterator(help_dir)) {
    if (entry.path().extension() != ".md") continue;
    const std::string topic = entry.path().stem().string();
    topics.insert(topic);

    std::ifstream file(entry.path());
    test::expect_true(file.is_open());
    std::string first_line;
    std::getline(file, first_line);
    test::expect_true(first_line.starts_with("# "));
  }

  for (std::string_view removed : {"prance", "automove", "use_stockpile",
                                   "promote", "ranks", "emote", "bug"}) {
    test::expect_false(topics.contains(std::string{removed}));
  }

  // Every tab-indented topic in help.md must resolve 1:1 with help/*.md.
  std::set<std::string> indexed_topics;
  {
    std::ifstream help_index(help_dir / "help.md");
    test::expect_true(help_index.is_open());
    for (std::string line; std::getline(help_index, line);) {
      if (!line.starts_with('\t')) continue;
      std::istringstream iss(line);
      for (std::string tok; iss >> tok;) {
        test::expect_true(topics.contains(tok));
        indexed_topics.insert(std::move(tok));
      }
    }
  }
  test::expect_eq(indexed_topics.size(), topics.size());

  // Every entry in commands.md must resolve to a help file and mortal command.
  const auto& registry = GB::commands::get_command_registry();
  std::set<std::string> command_doc_entries;
  {
    std::ifstream cmd_index(help_dir / "commands.md");
    test::expect_true(cmd_index.is_open());
    for (std::string line; std::getline(cmd_index, line);) {
      const auto sep = line.find(" -- ");
      if (sep == std::string::npos) continue;
      std::string cmd = line.substr(0, sep);
      test::expect_true(topics.contains(cmd));
      const auto it = registry.find(cmd);
      test::expect_true(it != registry.end());
      if (it != registry.end()) {
        test::expect_false(it->second->roles.god_only);
      }
      command_doc_entries.insert(std::move(cmd));
    }
  }

  // Every mortal command must have a help page and commands.md listing.
  for (const auto& [name, desc] : registry) {
    if (desc->roles.god_only) continue;
    test::expect_true(topics.contains(std::string{desc->name}));
    if (name == desc->name) {
      test::expect_true(command_doc_entries.contains(std::string{name}));
    }
  }

  // Every topic referenced in a "## See Also" section must exist.
  for (const auto& topic : topics) {
    std::ifstream file(help_dir / std::format("{}.md", topic));
    bool in_see_also = false;
    for (std::string line; std::getline(file, line);) {
      if (line.starts_with("## See Also") || line.starts_with("## SEE ALSO")) {
        in_see_also = true;
        continue;
      }
      if (!in_see_also) continue;
      if (line.starts_with('#')) {
        in_see_also = false;
        continue;
      }
      for (char& ch : line) {
        if (ch == ',' || ch == ';') ch = ' ';
      }
      std::istringstream iss(line);
      for (std::string tok; iss >> tok;) {
        if (tok.ends_with('.')) tok.pop_back();
        if (tok.empty() || tok == "none" || tok == "None") continue;
        test::expect_true(topics.contains(tok));
      }
    }
  }
}

void test_ships_build_and_order_help_parity() {
  std::println(std::cout,
               "Test: ships.md, build.md, order.md, and update.md match "
               "canonical mechanics");

  const auto help_dir = get_help_dir();
  const auto read_all = [&](std::string_view name) {
    std::ifstream file(help_dir / std::format("{}.md", name));
    test::expect_true(file.is_open());
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
  };

  for (const auto& entry : std::filesystem::directory_iterator(help_dir)) {
    if (entry.path().extension() != ".md") continue;
    const std::string text = read_all(entry.path().stem().string());
    for (std::string_view hut_holdover :
         {"Super Pod", "Dhuttle", "Colonizer", "Mine Sweeper", "Light Carrier",
          "autoscrap", "order stockpile", "48% damage", "72% damage"}) {
      test::expect_eq(text.find(hut_holdover), std::string::npos);
    }
  }

  const std::string ships_md = read_all("ships");
  for (std::string_view canonical_ship :
       {"Godship", "Von Neumann Machine", "Berserker", "Tractor-Repulsor Beam",
        "Mind Control Laser", "Berserker Control Center", "Berserker Autofac",
        R"(order \ aim)"}) {
    test::expect_contains(ships_md, canonical_ship);
  }

  const std::string build_md = read_all("build");
  test::expect_contains(build_md, "25%");
  test::expect_eq(build_md.find("24%"), std::string::npos);

  const std::string order_md = read_all("order");
  for (std::string_view order_subcmd :
       {"merchant",  "destination", "speed",   "jump",      "impact",
        "scatter",   "on",          "off",     "switch",    "aim",
        "intensity", "transport",   "evade",   "primary",   "secondary",
        "bombard",   "retaliate",   "laser",   "focus",     "navigate",
        "protect",   "salvo",       "defense", "explosive", "radiative",
        "move",      "trigger"}) {
    test::expect_contains(order_md, order_subcmd);
  }
  for (std::string_view removed_subcmd :
       {"order <shipnumber> online", "  hop --", "  limit --",
        "  disperse --"}) {
    test::expect_eq(order_md.find(removed_subcmd), std::string::npos);
  }
}

void test_command_help_ap_header_parity() {
  std::println(std::cout,
               "Test: Command help [AP] headers match CommandDescriptor::ap");

  const auto help_dir = get_help_dir();
  const auto extract_name_ap_brackets = [&](std::string_view topic) {
    std::ifstream file(help_dir / std::format("{}.md", topic));
    test::expect_true(file.is_open());
    std::vector<std::string> brackets;
    bool in_name = false;
    for (std::string line; std::getline(file, line);) {
      if (line.starts_with("## Name") || line.starts_with("## NAME")) {
        in_name = true;
        continue;
      }
      if (!in_name) continue;
      if (line.starts_with('#')) break;
      for (std::size_t pos = 0;
           (pos = line.find('[', pos)) != std::string::npos;) {
        const auto close = line.find(']', pos + 1);
        test::expect_ne(close, std::string::npos);
        if (close == std::string::npos) break;
        brackets.push_back(line.substr(pos + 1, close - pos - 1));
        pos = close + 1;
      }
    }
    return brackets;
  };

  for (const auto& [name, desc] : GB::commands::get_command_registry()) {
    if (desc->roles.god_only) continue;
    if (!std::filesystem::exists(help_dir / std::format("{}.md", name))) {
      continue;
    }
    const auto brackets = extract_name_ap_brackets(name);
    test::expect_false(brackets.empty());
    for (const auto& ap_text : brackets) {
      switch (desc->ap.model) {
        case GB::commands::APModel::Free:
          test::expect_eq(ap_text, std::string{"0"});
          break;
        case GB::commands::APModel::FixedStar:
        case GB::commands::APModel::FixedUniv:
          test::expect_eq(ap_text, std::to_string(desc->ap.amount));
          break;
        case GB::commands::APModel::Dynamic:
          test::expect_ne(ap_text, std::string{"0"});
          break;
      }
    }
  }
}

int main() {
  test_help_files_exist();
  test_help_file_readable();
  test_help_file_format();
  test_nonexistent_help_file();
  test_all_help_headers_links_and_registry_parity();
  test_ships_build_and_order_help_parity();
  test_command_help_ap_header_parity();

  std::println(std::cout, "\n✅ All help_test tests passed!");
  return 0;
}
