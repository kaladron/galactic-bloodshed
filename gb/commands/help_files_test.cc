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

int main() {
  test_help_files_exist();
  test_help_file_readable();
  test_help_file_format();
  test_nonexistent_help_file();
  test_all_help_headers_links_and_registry_parity();

  std::println(std::cout, "\n✅ All help_test tests passed!");
  return 0;
}
