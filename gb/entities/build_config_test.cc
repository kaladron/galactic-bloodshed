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

extern "C" {
int setenv(const char* name, const char* value, int overwrite) noexcept;
int unsetenv(const char* name) noexcept;
}

namespace {

void test_configured_constants_and_path_helpers() {
  // 1. Unset environment falls back to compiled PKG_DATA_DIR / PKG_STATE_DIR
  unsetenv("GB_DATADIR");
  unsetenv("GB_STATEDIR");

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

  // 2. Empty environment variables also fall back to compiled defaults
  setenv("GB_DATADIR", "", 1);
  setenv("GB_STATEDIR", "", 1);
  test::expect_eq(pkg_data_path("exam.dat"),
                  std::filesystem::path{PKG_DATA_DIR} / "exam.dat");
  test::expect_eq(pkg_state_path("nogo"),
                  std::filesystem::path{PKG_STATE_DIR} / "nogo");

  // 3. Non-empty GB_DATADIR / GB_STATEDIR override compiled install paths
  setenv("GB_DATADIR", "/tmp/gb-test-data", 1);
  setenv("GB_STATEDIR", "/tmp/gb-test-state", 1);
  test::expect_eq(pkg_data_path("exam.dat"),
                  std::filesystem::path{"/tmp/gb-test-data/exam.dat"});
  test::expect_eq(pkg_state_path("nogo"),
                  std::filesystem::path{"/tmp/gb-test-state/nogo"});
  test::expect_eq(default_db_path(),
                  std::filesystem::path{"/tmp/gb-test-state/gb.db"});

  const std::filesystem::path help_dir{HELP_DIR};
  test::expect_false(help_dir.empty());
}

}  // namespace

int main() {
  test_configured_constants_and_path_helpers();
  std::println(std::cout, "✓ build_config_test passed!");
  return 0;
}
