// SPDX-License-Identifier: Apache-2.0

/// \file server_config_test.cc
/// \brief Unit tests for CLI parsing, server configuration, schedule
/// initialization, and block setup.

import dallib;
import gb.entities;
import gb.services;
import gb.server;
import server_config;
import test;
import std;

namespace {

void test_parse_server_args_default() {
  const char* argv[] = {"GB"};
  ServerConfig config = parse_server_args(1, argv);
  test::expect_eq(config.port, GB_PORT);
  test::expect_eq(config.update_time, DEFAULT_UPDATE_TIME);
  test::expect_eq(config.segments, MOVES_PER_UPDATE);
}

void test_parse_server_args_custom_port() {
  const char* argv[] = {"GB", "2020"};
  ServerConfig config = parse_server_args(2, argv);
  test::expect_eq(config.port, 2020);
  test::expect_eq(config.update_time, DEFAULT_UPDATE_TIME);
  test::expect_eq(config.segments, MOVES_PER_UPDATE);
}

void test_parse_server_args_custom_update_time() {
  const char* argv[] = {"GB", "2020", "45"};
  ServerConfig config = parse_server_args(3, argv);
  test::expect_eq(config.port, 2020);
  test::expect_eq(config.update_time, std::chrono::minutes(45));
  test::expect_eq(config.segments, MOVES_PER_UPDATE);
}

void test_parse_server_args_custom_segments() {
  const char* argv[] = {"GB", "2020", "45", "6"};
  ServerConfig config = parse_server_args(4, argv);
  test::expect_eq(config.port, 2020);
  test::expect_eq(config.update_time, std::chrono::minutes(45));
  test::expect_eq(config.segments, 6);
}

void test_parse_server_args_flags() {
  {
    const char* argv[] = {"GB", "-p", "2025", "-d", "/tmp/custom.db"};
    ServerConfig config = parse_server_args(5, argv);
    test::expect_eq(config.port, 2025);
    test::expect_eq(config.db_path, std::string("/tmp/custom.db"));
    test::expect_false(config.show_help);
    test::expect_false(config.has_error);
  }
  {
    const char* argv[] = {"GB", "--port", "3000", "--database",
                          "/var/data/gb.sqlite"};
    ServerConfig config = parse_server_args(5, argv);
    test::expect_eq(config.port, 3000);
    test::expect_eq(config.db_path, std::string("/var/data/gb.sqlite"));
    test::expect_false(config.show_help);
    test::expect_false(config.has_error);
  }
  {
    const char* argv[] = {"GB", "--port=4000", "--db=/opt/gb.db"};
    ServerConfig config = parse_server_args(3, argv);
    test::expect_eq(config.port, 4000);
    test::expect_eq(config.db_path, std::string("/opt/gb.db"));
    test::expect_false(config.show_help);
    test::expect_false(config.has_error);
  }
}

void test_parse_server_args_help() {
  {
    const char* argv[] = {"GB", "-h"};
    ServerConfig config = parse_server_args(2, argv);
    test::expect_true(config.show_help);
    test::expect_false(config.has_error);
  }
  {
    const char* argv[] = {"GB", "--help"};
    ServerConfig config = parse_server_args(2, argv);
    test::expect_true(config.show_help);
    test::expect_false(config.has_error);
  }
}

void test_parse_server_args_errors() {
  {
    const char* argv[] = {"GB", "-p"};
    ServerConfig config = parse_server_args(2, argv);
    test::expect_true(config.has_error);
  }
  {
    const char* argv[] = {"GB", "--port", "abc"};
    ServerConfig config = parse_server_args(3, argv);
    test::expect_true(config.has_error);
  }
  {
    const char* argv[] = {"GB", "--unknown"};
    ServerConfig config = parse_server_args(2, argv);
    test::expect_true(config.has_error);
  }
}

void test_initialize_schedule_state_first_run() {
  ServerState state{};
  ServerConfig config{
      .port = 2020, .update_time = std::chrono::minutes(60), .segments = 4};

  std::time_t now = 1000000;
  initialize_schedule_state(state, config, now);

  test::expect_eq(state.update_time_minutes, 60);
  test::expect_eq(state.segments, 4);
  test::expect_eq(state.next_update_time, now + 3600);
  test::expect_eq(state.next_segment_time, now + 900);
}

void test_initialize_schedule_state_single_segment() {
  ServerState state{};
  ServerConfig config{
      .port = 2020, .update_time = std::chrono::minutes(60), .segments = 1};

  std::time_t now = 1000000;
  initialize_schedule_state(state, config, now);

  test::expect_eq(state.segments, 1);
  test::expect_eq(state.next_segment_time, now + (144 * 3600));
}

void test_initialize_schedule_state_catchup_past_segments() {
  ServerState state{};
  state.next_update_time = 1003600;
  state.next_segment_time = 900000;  // Past time (< now)

  ServerConfig config{
      .port = 2020, .update_time = std::chrono::minutes(60), .segments = 4};

  std::time_t now = 1000000;
  initialize_schedule_state(state, config, now);

  test::expect_eq(state.next_segment_time, state.next_update_time);
  test::expect_eq(state.nsegments_done, 4);
}

void test_initialize_block_data() {
  TestContext ctx;
  Race race1{};
  race1.Playernum = 1;
  race1.name = "Race1";

  Race race2{};
  race2.Playernum = 2;
  race2.name = "Race2";

  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(race1);
    races.save(race2);

    BlockRepository blocks(store);
    block b1{};
    b1.Playernum = 1;
    blocks.save(b1);
    block b2{};
    b2.Playernum = 2;
    blocks.save(b2);
  }

  initialize_block_data(ctx.em);

  const auto* block1 = ctx.em.peek_block(1);
  test::expect_ne(block1, nullptr);
  test::expect_true(block1->is_invited(player_t{1}));
  test::expect_true(block1->is_pledged(player_t{1}));

  const auto* block2 = ctx.em.peek_block(2);
  test::expect_ne(block2, nullptr);
  test::expect_true(block2->is_invited(player_t{2}));
  test::expect_true(block2->is_pledged(player_t{2}));
}

}  // namespace

int main() {
  test_parse_server_args_default();
  test_parse_server_args_custom_port();
  test_parse_server_args_custom_update_time();
  test_parse_server_args_custom_segments();
  test_parse_server_args_flags();
  test_parse_server_args_help();
  test_parse_server_args_errors();
  test_initialize_schedule_state_first_run();
  test_initialize_schedule_state_single_segment();
  test_initialize_schedule_state_catchup_past_segments();
  test_initialize_block_data();

  std::println(std::cout, "✓ server_config_test passed!");
  return 0;
}
