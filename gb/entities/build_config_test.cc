// SPDX-License-Identifier: Apache-2.0

/// \file build_config_test.cc
/// \brief Unit tests for CMake-configured version and directory path constants.

import gb.entities;
import test;
import std;

static_assert(!GB_VERSION.empty());
static_assert(!PKG_DATA_DIR.empty());
static_assert(!PKG_STATE_DIR.empty());
static_assert(!HELP_DIR.empty());

namespace {

void test_configured_constants_and_path_helpers() {
  const auto star_list = pkg_data_path("star.list");
  test::expect_eq(star_list.filename().string(), "star.list");
  test::expect_eq(star_list, std::filesystem::path{PKG_DATA_DIR} / "star.list");
  test::expect_not_contains(star_list.string(), "//");

  const auto nogo = pkg_state_path("nogo");
  test::expect_eq(nogo.filename().string(), "nogo");
  test::expect_eq(nogo, std::filesystem::path{PKG_STATE_DIR} / "nogo");
  test::expect_not_contains(nogo.string(), "//");

  const auto db = default_db_path();
  test::expect_eq(db.filename().string(), "gb.db");
  test::expect_eq(db, pkg_state_path("gb.db"));
  test::expect_not_contains(db.string(), "//");

  const std::filesystem::path help_dir{HELP_DIR};
  test::expect_false(help_dir.empty());
}

}  // namespace

int main() {
  test_configured_constants_and_path_helpers();
  std::println(std::cout, "✓ build_config_test passed!");
  return 0;
}
