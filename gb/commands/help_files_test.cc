// SPDX-License-Identifier: Apache-2.0

/// \file help_test.cc
/// \brief Unit tests verifying help documentation files exist in HELPDIR, are
/// readable markdown, and have valid headers.

import test;
import std;

// Test that help files exist in the HELPDIR with .md extension
void test_help_files_exist() {
  std::println(std::cout, "Test: Help files exist with .md extension");

  // HELPDIR is defined at compile time via CMake
  std::filesystem::path help_dir(HELPDIR);

  // Check that the help directory exists
  test::expect_true(std::filesystem::exists(help_dir));
  test::expect_true(std::filesystem::is_directory(help_dir));
  std::println(std::cout, "  ✓ HELPDIR exists: {}", HELPDIR);

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

  // Test a few common help files
  std::vector<std::string> test_files = {"help", "build", "cs", "map", "orbit"};

  for (const auto& name : test_files) {
    std::string filepath = std::format("{}/{}.md", HELPDIR, name);

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

  std::string filepath = std::format("{}/build.md", HELPDIR);

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

  std::string filepath =
      std::format("{}/this_topic_does_not_exist.md", HELPDIR);

  std::ifstream file(filepath);
  test::expect_false(file.is_open());
  std::println(std::cout, "  ✓ Non-existent help file correctly not found");
}

int main() {
  test_help_files_exist();
  test_help_file_readable();
  test_help_file_format();
  test_nonexistent_help_file();

  std::println(std::cout, "\n✅ All help_test tests passed!");
  return 0;
}
