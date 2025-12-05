// SPDX-License-Identifier: Apache-2.0

/// \file help_test.cc
/// \brief Test that help files exist and can be read

import std;

#include <cassert>

// Test that help files exist in the HELPDIR with .md extension
void test_help_files_exist() {
  std::println("Test: Help files exist with .md extension");

  // HELPDIR is defined at compile time via CMake
  std::filesystem::path help_dir(HELPDIR);

  // Check that the help directory exists
  assert(std::filesystem::exists(help_dir));
  assert(std::filesystem::is_directory(help_dir));
  std::println("  ✓ HELPDIR exists: {}", HELPDIR);

  // Count .md files
  int md_count = 0;
  for (const auto& entry : std::filesystem::directory_iterator(help_dir)) {
    if (entry.path().extension() == ".md") {
      md_count++;
    }
  }
  assert(md_count > 0);
  std::println("  ✓ Found {} .md help files", md_count);
}

// Test that specific help files can be opened and read
void test_help_file_readable() {
  std::println("Test: Help files can be opened and read");

  // Test a few common help files
  std::vector<std::string> test_files = {"help", "build", "cs", "map", "orbit"};

  for (const auto& name : test_files) {
    std::string filepath = std::format("{}/{}.md", HELPDIR, name);

    std::ifstream file(filepath);
    assert(file.is_open());

    // Read first line to verify content
    std::string first_line;
    std::getline(file, first_line);
    assert(!first_line.empty());

    // Verify the first line contains the expected markdown header
    assert(first_line[0] == '#');

    file.close();
    std::println("  ✓ {} readable, starts with: {}", name,
                 first_line.substr(0, 20));
  }
}

// Test that help file format is correct (markdown headers)
void test_help_file_format() {
  std::println("Test: Help files have proper markdown format");

  std::string filepath = std::format("{}/build.md", HELPDIR);

  std::ifstream file(filepath);
  assert(file.is_open());

  std::string line;
  bool found_title = false;
  bool found_section = false;

  while (std::getline(file, line)) {
    // Check for title (# TITLE)
    if (line.size() >= 2 && line[0] == '#' && line[1] == ' ') {
      found_title = true;
    }
    // Check for section header (## Section)
    if (line.size() >= 3 && line[0] == '#' && line[1] == '#' && line[2] == ' ') {
      found_section = true;
    }
  }

  file.close();

  assert(found_title);
  assert(found_section);
  std::println("  ✓ build.md has proper markdown structure");
}

// Test that requesting a non-existent help topic fails gracefully
void test_nonexistent_help_file() {
  std::println("Test: Non-existent help file returns null");

  std::string filepath = std::format("{}/this_topic_does_not_exist.md", HELPDIR);

  std::ifstream file(filepath);
  assert(!file.is_open());
  std::println("  ✓ Non-existent help file correctly not found");
}

int main() {
  test_help_files_exist();
  test_help_file_readable();
  test_help_file_format();
  test_nonexistent_help_file();

  std::println("\n✅ All help_test tests passed!");
  return 0;
}
