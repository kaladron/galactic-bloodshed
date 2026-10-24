// SPDX-License-Identifier: Apache-2.0

/// \file help_test.cc
/// \brief Unit tests for help command dispatch

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

struct ScopedTempDirAndCwd {
  std::filesystem::path orig_cwd;
  std::filesystem::path tmp_dir;

  ScopedTempDirAndCwd()
      : orig_cwd(std::filesystem::current_path()),
        tmp_dir(
            std::filesystem::temp_directory_path() /
            std::format(
                "gb_help_test_{}",
                std::chrono::steady_clock::now().time_since_epoch().count())) {
    std::filesystem::create_directories(tmp_dir);
  }

  ~ScopedTempDirAndCwd() {
    std::filesystem::current_path(orig_cwd);
    std::filesystem::remove_all(tmp_dir);
  }
};

void test_help_matrix() {
  TestContext ctx;
  ctx.with_standard_universe();
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  TestCommandMatrix(ctx, "help")
      .with_valid_argv({"help"})
      .with_invalid_argv({"help", "this_topic_does_not_exist"})
      .with_valid_scope(ScopeLevel::LEVEL_UNIV)
      .with_expected_star_ap(0)
      .run_matrix(g);
}

// Test help command dispatch with valid and invalid topics
void test_help_command_dispatch() {
  TestContext ctx;
  ctx.with_standard_universe();
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  // 1. Happy Path: General help success
  ctx.assert_dispatch_success(g, {"help"});
  test::expect_false(g.out.str().empty());

  // 2. Happy Path: Topic help success
  g.out.str("");
  ctx.assert_dispatch_success(g, {"help", "build"});
  test::expect_contains(g.out.str(), "Finished.");

  // 3. Domain Error: Non-existent and empty topics
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"help", "this_topic_does_not_exist"});
  test::expect_contains(g.out.str(), "Help on that subject unavailable.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"help", ""});
  test::expect_contains(g.out.str(), "Help on that subject unavailable.");

  // 4. Domain Error: Path traversal and extra trailing arguments rejected
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"help", "../help"});
  test::expect_contains(g.out.str(), "Help on that subject unavailable.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"help", "build", "extra"});
  test::expect_contains(g.out.str(), "Syntax: help [<topic>]");

  // 5. UiMode::JSON rendering for index and topic help
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"help"});
  test::expect_contains(g.out.str(), "\"type\":\"help\"");
  test::expect_contains(g.out.str(), "\"topic\":\"help\"");
  test::expect_contains(g.out.str(), "\"is_index\":true");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"help", "build"});
  test::expect_contains(g.out.str(), "\"topic\":\"build\"");
  test::expect_contains(g.out.str(), "\"is_index\":false");

  ctx.verify_universe_invariants();
}

void test_help_dir_fallbacks() {
  TestContext ctx;
  ctx.with_standard_universe();
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  ScopedTempDirAndCwd guard;
  const std::string tmp_dir_str = guard.tmp_dir.string();

  // 1. Empty help_dir_override falls back to HELPDIR / dev_dir
  g.out.str("");
  test::expect_true(GB::commands::help({"help", "build"}, g, ""));
  test::expect_contains(g.out.str(), "Finished.");

  // 2. Empty directory override falls back to relative dev_dir
  g.out.str("");
  test::expect_true(GB::commands::help({"help", "build"}, g, tmp_dir_str));
  test::expect_contains(g.out.str(), "Finished.");

  // 3. Directory named *.md rejected & missing index reports Help file not
  // found
  std::filesystem::create_directories(guard.tmp_dir / "dir_topic.md");
  std::filesystem::current_path(guard.tmp_dir);

  g.out.str("");
  test::expect_false(GB::commands::help({"help", "dir_topic"}, g, tmp_dir_str));
  test::expect_contains(g.out.str(), "Help on that subject unavailable.");

  g.out.str("");
  test::expect_false(GB::commands::help({"help"}, g, tmp_dir_str));
  test::expect_contains(g.out.str(), "Help file not found.");
}

}  // namespace

int main() {
  test_help_matrix();
  test_help_command_dispatch();
  test_help_dir_fallbacks();

  std::println(std::cout, "✓ help_command_test passed!");
  return 0;
}
