// SPDX-License-Identifier: Apache-2.0

/// \file read_messages_test.cc
/// \brief Unit tests for read command and telegram/news retrieval services

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_read_command_descriptor() {
  const auto& desc = GB::commands::read_cmd;
  test::expect_eq(desc.name, "read");
  test::expect_eq(desc.min_args, 1);
  test::expect_eq(desc.ap.model, GB::commands::APModel::Free);
  test::expect_eq(desc.ap.amount, 0);
  test::expect_true(desc.scopes.allows(ScopeLevel::LEVEL_UNIV));
  test::expect_true(desc.scopes.allows(ScopeLevel::LEVEL_STAR));
  test::expect_true(desc.scopes.allows(ScopeLevel::LEVEL_PLAN));
  test::expect_true(desc.scopes.allows(ScopeLevel::LEVEL_SHIP));
}

void test_read_command_matrix() {
  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  TestCommandMatrix(ctx, "read")
      .with_valid_argv({"read"})
      .with_invalid_argv({"read", "unrecognized_target"})
      .run_matrix(g);

  ctx.verify_universe_invariants();
}

void test_read_telegrams() {
  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // Initially empty mailbox
  check_for_telegrams(g);
  test::expect_true(g.out.str().empty());

  ctx.assert_dispatch_success(g, {"read"});
  test::expect_contains(g.out.str(), "Telegrams:\n None.\n");
  g.out.str("");

  // 'read telegram' subcommand is equivalent to 'read'
  ctx.assert_dispatch_success(g, {"read", "telegram"});
  test::expect_contains(g.out.str(), "Telegrams:\n None.\n");
  g.out.str("");

  // Post telegrams to Player 1, Governor 0
  push_telegram(ctx.em, 1, 0, "First telegram with newline\n");
  push_telegram(ctx.em, 1, 0, "Second telegram without newline");

  // Check notification prompt
  check_for_telegrams(g);
  test::expect_contains(
      g.out.str(), "You have telegram(s) waiting. Use 'read' to read them.\n");
  g.out.str("");

  // Read telegrams
  ctx.assert_dispatch_success(g, {"read"});
  std::string output = g.out.str();
  test::expect_contains(output, "Telegrams:\n");
  test::expect_contains(output, "First telegram with newline");
  test::expect_contains(output, "Second telegram without newline\n");
  g.out.str("");

  // Verify delete-on-read behavior
  test::expect_false(ctx.em.has_telegrams(1, 0));

  ctx.assert_dispatch_success(g, {"read"});
  test::expect_contains(g.out.str(), "Telegrams:\n None.\n");
  g.out.str("");

  check_for_telegrams(g);
  test::expect_true(g.out.str().empty());

  ctx.verify_universe_invariants();
}

void test_read_telegrams_governor_isolation() {
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_race(1, [](Race& race) { race.governor[1].active = true; });

  auto& registry = get_test_session_registry();
  GameObj g0(ctx.em, registry);
  ctx.setup_game_obj(g0, 1, 0);

  GameObj g1(ctx.em, registry);
  ctx.setup_game_obj(g1, 1, 1);

  // Send separate telegrams to Gov 0 and Gov 1
  push_telegram(ctx.em, 1, 0, "Confidential for Governor 0\n");
  push_telegram(ctx.em, 1, 1, "Confidential for Governor 1\n");

  test::expect_true(ctx.em.has_telegrams(1, 0));
  test::expect_true(ctx.em.has_telegrams(1, 1));

  // Governor 0 reads messages
  ctx.assert_dispatch_success(g0, {"read"});
  test::expect_contains(g0.out.str(), "Confidential for Governor 0");
  test::expect_false(g0.out.str().contains("Confidential for Governor 1"));

  // Governor 0's mailbox is now empty, but Governor 1's is untouched
  test::expect_false(ctx.em.has_telegrams(1, 0));
  test::expect_true(ctx.em.has_telegrams(1, 1));

  // Governor 1 reads messages
  ctx.assert_dispatch_success(g1, {"read"});
  test::expect_contains(g1.out.str(), "Confidential for Governor 1");
  test::expect_false(g1.out.str().contains("Confidential for Governor 0"));

  test::expect_false(ctx.em.has_telegrams(1, 1));

  ctx.verify_universe_invariants();
}

void test_read_news() {
  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // Initially empty news feed
  ctx.assert_dispatch_success(g, {"read", "news"});
  std::string empty_news = g.out.str();
  test::expect_contains(empty_news, CUTE_MESSAGE);
  test::expect_contains(empty_news, "Declarations");
  test::expect_contains(empty_news, "Combat");
  test::expect_contains(empty_news, "Business");
  test::expect_contains(empty_news, "Bulletins");
  g.out.str("");

  // Post news items in all four categories using post() API
  post(ctx.em, "Alliance treaty signed;peace declared|official",
       NewsType::DECLARATION);
  post(ctx.em, "Battle of Vega Prime ended\n", NewsType::COMBAT);
  post(ctx.em, "Ship sale concluded\n", NewsType::TRANSFER);
  post(ctx.em, "Server reboot announced without newline", NewsType::ANNOUNCE);

  // Read news
  ctx.assert_dispatch_success(g, {"read", "news"});
  std::string full_news = g.out.str();
  test::expect_contains(full_news, CUTE_MESSAGE);
  test::expect_contains(full_news,
                        "Alliance treaty signed\npeace declared\tofficial");
  test::expect_contains(full_news, "Battle of Vega Prime ended");
  test::expect_contains(full_news, "Ship sale concluded");
  test::expect_contains(full_news, "Server reboot announced without newline\n");
  g.out.str("");

  // Governor's newspos array in database should have advanced to latest IDs
  int latest_decl = ctx.em.get_latest_news_id(NewsType::DECLARATION);
  int latest_combat = ctx.em.get_latest_news_id(NewsType::COMBAT);
  int latest_transfer = ctx.em.get_latest_news_id(NewsType::TRANSFER);
  int latest_announce = ctx.em.get_latest_news_id(NewsType::ANNOUNCE);

  ctx.em.with_race(1, [&](const Race& race) {
    test::expect_eq(
        race.governor[0].newspos[std::to_underlying(NewsType::DECLARATION)],
        latest_decl);
    test::expect_eq(
        race.governor[0].newspos[std::to_underlying(NewsType::COMBAT)],
        latest_combat);
    test::expect_eq(
        race.governor[0].newspos[std::to_underlying(NewsType::TRANSFER)],
        latest_transfer);
    test::expect_eq(
        race.governor[0].newspos[std::to_underlying(NewsType::ANNOUNCE)],
        latest_announce);
  });

  // Reading again immediately produces no new articles
  ctx.assert_dispatch_success(g, {"read", "news"});
  std::string repeat_news = g.out.str();
  test::expect_false(repeat_news.contains("Alliance treaty signed"));
  test::expect_false(repeat_news.contains("Battle of Vega Prime"));
  g.out.str("");

  // Post a new single combat article
  post(ctx.em, "Second clash reported at Sol\n", NewsType::COMBAT);

  ctx.assert_dispatch_success(g, {"read", "news"});
  std::string updated_news = g.out.str();
  test::expect_contains(updated_news, "Second clash reported at Sol");
  test::expect_false(updated_news.contains("Battle of Vega Prime ended"));
  g.out.str("");

  // Purge all news
  purge(ctx.em);
  test::expect_true(ctx.em.get_news_since(NewsType::COMBAT, 0).empty());
  test::expect_true(ctx.em.get_news_since(NewsType::DECLARATION, 0).empty());
  test::expect_true(ctx.em.get_news_since(NewsType::TRANSFER, 0).empty());
  test::expect_true(ctx.em.get_news_since(NewsType::ANNOUNCE, 0).empty());

  ctx.verify_universe_invariants();
}

void test_read_invalid_arguments() {
  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  ctx.assert_dispatch_rejected(g, {"read", "invalid_topic"});
  test::expect_contains(g.out.str(), "Read what?\n");
  g.out.str("");

  ctx.assert_dispatch_rejected(g, {"read", "news", "extra_arg"});
  test::expect_contains(g.out.str(), "Read what?\n");
  g.out.str("");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_read_command_descriptor();
  test_read_command_matrix();
  test_read_telegrams();
  test_read_telegrams_governor_isolation();
  test_read_news();
  test_read_invalid_arguments();

  std::println(std::cout, "✓ read_messages_test passed!");
  return 0;
}
