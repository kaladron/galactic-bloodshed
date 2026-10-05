// SPDX-License-Identifier: Apache-2.0

/// \file notification_test.cc
/// \brief Comprehensive unit tests for notification delivery, filtering, gag
/// toggles, and star/race broadcasting.

import std;
import dallib;
import gb.entities;
import gb.services;
import gb.server;
import session;
import notification;
import test;

// Mock Session for testing (doesn't need actual socket)
class MockSession {
public:
  MockSession(player_t player, governor_t governor, starnum_t snum,
              bool connected, bool gag)
      : player_(player), governor_(governor), snum_(snum),
        connected_(connected), gag_(gag) {}

  player_t player() const {
    return player_;
  }
  governor_t governor() const {
    return governor_;
  }
  starnum_t snum() const {
    return snum_;
  }
  bool connected() const {
    return connected_;
  }
  bool gag() const {
    return gag_;
  }

  void send(std::string_view message) {
    out_.append(message);
  }
  std::string get_output() const {
    return out_;
  }
  void clear_output() {
    out_.clear();
  }

private:
  player_t player_;
  governor_t governor_;
  starnum_t snum_;
  bool connected_;
  bool gag_;
  std::string out_;
};

// Mock SessionRegistry for testing
class MockRegistry : public SessionRegistry {
public:
  explicit MockRegistry(bool update_in_progress = false)
      : update_in_progress_(update_in_progress) {}

  void add_session(const std::shared_ptr<MockSession>& session) {
    sessions_.push_back(session);
  }

  [[nodiscard]] bool update_in_progress() const override {
    return update_in_progress_;
  }

  void set_update_in_progress(bool val) override {
    update_in_progress_ = val;
  }

  [[nodiscard]] bool is_connected(player_t race,
                                  governor_t gov) const override {
    return std::ranges::any_of(sessions_, [&](const auto& s) {
      return s->connected() && s->player() == race && s->governor() == gov;
    });
  }

  // Override notification methods for testing
  void notify_race(player_t race, const std::string& message) override {
    if (update_in_progress_) return;
    if (stage_race_notification(race, message)) return;
    for (auto& session : sessions_) {
      if (session->connected() && session->player() == race) {
        session->send(message);
      }
    }
  }

  bool notify_player(player_t race, governor_t gov,
                     const std::string& message) override {
    if (update_in_progress_) return false;
    if (!is_player_connected(race, gov)) return false;
    if (stage_player_notification(race, gov, message)) return true;
    for (auto& session : sessions_) {
      if (session->connected() && session->player() == race &&
          session->governor() == gov) {
        session->send(message);
        return true;
      }
    }
    return false;
  }

  // Accessor for test verification
  std::vector<std::shared_ptr<MockSession>>& sessions() {
    return sessions_;
  }

private:
  std::vector<std::shared_ptr<MockSession>> sessions_;
  bool update_in_progress_;
};

// Helper to create race with specific settings
Race create_race(player_t player, bool god = false) {
  Race race{};
  race.Playernum = player;
  race.Guest = false;
  race.God = god;
  for (governor_t i{2}; i <= 5; ++i) {
    race.appoint_governor(i);
  }
  return race;
}

// Helper to create star (returns star_struct, not Star class)
star_struct create_star(starnum_t snum) {
  star_struct star{};
  star.star_id = snum;
  star.name = std::format("Star{}", snum);
  star.coordinates = {0.0, 0.0};
  star.stability = 100;
  star.nova_stage = 0;
  star.temperature = 50;
  star.gravity = 1.0;
  return star;
}

void test_notify_player_basic() {
  std::println(std::cout, "Testing notify_player basic functionality...");

  MockRegistry registry;
  auto session1 = std::make_shared<MockSession>(1, 1, 1, true, false);
  auto session2 = std::make_shared<MockSession>(1, 2, 1, true, false);
  auto session3 = std::make_shared<MockSession>(2, 1, 1, true, false);

  registry.add_session(session1);
  registry.add_session(session2);
  registry.add_session(session3);

  // Test: Message to player 1, governor 1
  bool delivered = registry.notify_player(1, 1, "Message to 1/1\n");
  test::expect_true(delivered);
  test::expect_eq(session1->get_output(), "Message to 1/1\n");
  test::expect_true(session2->get_output().empty());
  test::expect_true(session3->get_output().empty());

  session1->clear_output();

  // Test: Message to player 1, governor 2
  delivered = registry.notify_player(1, 2, "Message to 1/2\n");
  test::expect_true(delivered);
  test::expect_true(session1->get_output().empty());
  test::expect_eq(session2->get_output(), "Message to 1/2\n");
  test::expect_true(session3->get_output().empty());

  session2->clear_output();

  // Test: Message to non-existent player
  delivered = registry.notify_player(99, 1, "No one here\n");
  test::expect_false(delivered);

  std::println(std::cout, "  ✓ notify_player basic tests passed");
}

void test_notify_race_basic() {
  std::println(std::cout, "Testing notify_race basic functionality...");

  MockRegistry registry;
  auto session1 = std::make_shared<MockSession>(1, 1, 1, true, false);
  auto session2 = std::make_shared<MockSession>(1, 2, 1, true, false);
  auto session3 = std::make_shared<MockSession>(2, 1, 1, true, false);

  registry.add_session(session1);
  registry.add_session(session2);
  registry.add_session(session3);

  // Test: Message to all governors of race 1
  registry.notify_race(1, "Message to race 1\n");
  test::expect_eq(session1->get_output(), "Message to race 1\n");
  test::expect_eq(session2->get_output(), "Message to race 1\n");
  test::expect_true(session3->get_output().empty());

  session1->clear_output();
  session2->clear_output();

  // Test: Message to race 2
  registry.notify_race(2, "Message to race 2\n");
  test::expect_true(session1->get_output().empty());
  test::expect_true(session2->get_output().empty());
  test::expect_eq(session3->get_output(), "Message to race 2\n");

  std::println(std::cout, "  ✓ notify_race basic tests passed");
}

void test_disconnected_sessions() {
  std::println(std::cout, "Testing disconnected sessions are skipped...");

  MockRegistry registry;
  auto session1 =
      std::make_shared<MockSession>(1, 1, 1, true, false);  // connected
  auto session2 =
      std::make_shared<MockSession>(1, 2, 1, false, false);  // disconnected

  registry.add_session(session1);
  registry.add_session(session2);

  // Only connected session should receive message
  registry.notify_race(1, "Test message\n");
  test::expect_eq(session1->get_output(), "Test message\n");
  test::expect_true(session2->get_output().empty());

  std::println(std::cout, "  ✓ Disconnected session tests passed");
}

void test_d_broadcast_announce_think_and_shout() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  auto race1 = create_race(1);
  race1.leader().toggle.gag = false;
  race1.governor(2).toggle.gag = true;
  race1.governor(3).toggle.gag = false;
  auto race2 = create_race(2);
  auto race3 = create_race(3);
  auto race4 = create_race(4, /*god=*/true);

  JsonStore store(db);
  RaceRepository races(store);
  for (const auto& r : {race1, race2, race3, race4})
    races.save(r);

  Star star{create_star(5)};
  star.mark_inhabited_by(player_t{1});
  star.mark_inhabited_by(player_t{2});
  StarRepository(store).save(star);

  MockRegistry registry;
  auto s1_1 = std::make_shared<MockSession>(1, 1, 5, true, false);
  auto s1_2 = std::make_shared<MockSession>(1, 2, 5, true, true);
  auto s1_3 = std::make_shared<MockSession>(1, 3, 5, true, false);
  auto s2_1 = std::make_shared<MockSession>(2, 1, 5, true, false);
  auto s3_1 = std::make_shared<MockSession>(3, 1, 5, true, false);
  auto s4_1 = std::make_shared<MockSession>(4, 1, 5, true, false);
  for (const auto& s : {s1_1, s1_2, s1_3, s2_1, s3_1, s4_1})
    registry.add_session(s);

  auto clear_all = [&]() {
    for (auto& s : registry.sessions())
      s->clear_output();
  };

  d_broadcast(registry, em, 1, 1, "Broadcast!\n");
  test::expect_true(s1_1->get_output().empty());
  test::expect_true(s1_2->get_output().empty());
  test::expect_eq(s1_3->get_output(), "Broadcast!\n");
  test::expect_eq(s2_1->get_output(), "Broadcast!\n");
  test::expect_eq(s3_1->get_output(), "Broadcast!\n");
  test::expect_eq(s4_1->get_output(), "Broadcast!\n");
  clear_all();

  d_announce(registry, em, 1, 1, 5, "Announce!\n");
  test::expect_true(s1_1->get_output().empty());
  test::expect_true(s1_2->get_output().empty());
  test::expect_eq(s1_3->get_output(), "Announce!\n");
  test::expect_eq(s2_1->get_output(), "Announce!\n");
  test::expect_true(s3_1->get_output().empty());
  test::expect_eq(s4_1->get_output(), "Announce!\n");
  clear_all();

  d_think(registry, em, 1, 1, "Think!\n");
  test::expect_true(s1_1->get_output().empty());
  test::expect_true(s1_2->get_output().empty());
  test::expect_eq(s1_3->get_output(), "Think!\n");
  test::expect_true(s2_1->get_output().empty());
  clear_all();

  d_shout(registry, em, 1, 1, "Shout!\n");
  test::expect_true(s1_1->get_output().empty());
  test::expect_eq(s1_2->get_output(), "Shout!\n");
  test::expect_eq(s1_3->get_output(), "Shout!\n");
  test::expect_eq(s2_1->get_output(), "Shout!\n");
}

void test_warn_player_and_leader_fallback() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);
  RaceRepository(store).save(create_race(1));

  MockRegistry registry;
  registry.set_update_in_progress(true);
  warn_player(registry, em, 1, 1, "Update message\n");
  test::expect_eq(em.get_telegrams(1, 1).size(), 1u);

  registry.set_update_in_progress(false);
  warn_player(registry, em, 1, 1, "Offline leader\n");
  test::expect_eq(em.get_telegrams(1, 1).size(), 2u);

  warn_player(registry, em, 1, 2, "Offline subordinate\n");
  test::expect_eq(em.get_telegrams(1, 2).size(), 1u);

  auto s1_1 = std::make_shared<MockSession>(1, 1, 1, true, false);
  registry.add_session(s1_1);

  warn_player(registry, em, 1, 1, "Online leader\n");
  test::expect_eq(s1_1->get_output(), "Online leader\n");
  s1_1->clear_output();

  warn_player(registry, em, 1, 2, "Fallback to leader\n");
  test::expect_eq(s1_1->get_output(), "Fallback to leader\n");
  test::expect_eq(em.get_telegrams(1, 2).size(), 1u);
}

void test_warn_race_all_governors() {
  std::println(
      std::cout,
      "Testing warn_race calls warn_player for all active governors...");

  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  Race race1{};
  race1.Playernum = 1;
  race1.Guest = false;
  race1.appoint_governor(2);

  JsonStore store(db);
  RaceRepository races(store);
  races.save(race1);

  MockRegistry registry(false);
  auto session1 = std::make_shared<MockSession>(1, 1, 1, true, false);
  auto session2 = std::make_shared<MockSession>(1, 2, 1, true, false);

  registry.add_session(session1);
  registry.add_session(session2);

  warn_race(registry, em, 1, "Warning to all governors\n");
  test::expect_eq(session1->get_output(), "Warning to all governors\n");
  test::expect_eq(session2->get_output(), "Warning to all governors\n");

  std::println(std::cout, "  ✓ warn_race all governors tests passed");
}

void test_notify_star() {
  std::println(std::cout, "Testing notify_star functionality...");

  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race1 = create_race(1);
  Race race2 = create_race(2);
  Race race3 = create_race(3);

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);
  races.save(race3);

  Star star{create_star(5)};
  star.mark_inhabited_by(player_t{1});
  star.mark_inhabited_by(player_t{2});

  StarRepository stars(store);
  stars.save(star);

  MockRegistry registry(false);
  auto session1_1 = std::make_shared<MockSession>(1, 1, 5, true, false);
  auto session1_2 = std::make_shared<MockSession>(1, 2, 5, true, false);
  auto session2_1 = std::make_shared<MockSession>(2, 1, 5, true, false);
  auto session3_1 = std::make_shared<MockSession>(3, 1, 5, true, false);

  registry.add_session(session1_1);
  registry.add_session(session1_2);
  registry.add_session(session2_1);
  registry.add_session(session3_1);

  // Notify star from race 1 leader (1, 1):
  // - (1, 1) is skipped (sender)
  // - (1, 2) receives live message (subordinate governor of sender race!)
  // - (1, 3) is offline and receives telegram
  // - (2, 1) receives live message
  // - (3, 1) is skipped (uninhabited)
  notify_star(registry, em, 1, 1, 5, "Star event\n");
  test::expect_true(session1_1->get_output().empty());
  test::expect_eq(session1_2->get_output(), "Star event\n");
  test::expect_eq(session2_1->get_output(), "Star event\n");
  test::expect_true(session3_1->get_output().empty());
  test::expect_eq(em.get_telegrams(1, 3).size(), 1u);

  // During update_in_progress, even online governors receive telegrams
  session1_2->clear_output();
  registry.set_update_in_progress(true);
  notify_star(registry, em, 1, 1, 5, "Update star event\n");
  test::expect_true(session1_2->get_output().empty());
  test::expect_eq(em.get_telegrams(1, 2).size(), 1u);

  std::println(std::cout, "  ✓ notify_star tests passed");
}

void test_warn_star_and_telegram_star() {
  std::println(std::cout, "Testing warn_star and telegram_star...");

  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race1 = create_race(1);
  Race race2 = create_race(2);
  Race race3 = create_race(3);

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);
  races.save(race3);

  Star star{create_star(7)};
  star.mark_inhabited_by(player_t{1});
  star.mark_inhabited_by(player_t{2});

  StarRepository stars(store);
  stars.save(star);

  MockRegistry registry(false);
  auto session1_1 = std::make_shared<MockSession>(1, 1, 7, true, false);
  auto session2_1 = std::make_shared<MockSession>(2, 1, 7, true, false);
  registry.add_session(session1_1);
  registry.add_session(session2_1);

  warn_star(registry, em, 1, 7, "Warning message\n");
  test::expect_true(session1_1->get_output().empty());
  test::expect_false(session2_1->get_output().empty());

  telegram_star(em, 7, 1, 1, "Telegram from P1G1\n");
  test::expect_eq(em.get_telegrams(1, 1).size(), 0u);
  test::expect_eq(em.get_telegrams(1, 2).size(), 1u);
  test::expect_eq(em.get_telegrams(2, 1).size(), 1u);

  std::println(std::cout, "  ✓ warn_star and telegram_star tests passed");
}

void test_transactional_socket_outbox() {
  std::println(std::cout, "Testing SessionRegistry transactional outbox...");

  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  Race race1 = create_race(1);
  Race race2 = create_race(2);
  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  RecordingSessionRegistry registry;
  registry.sessions = {
      SessionInfo{.player = 1, .governor = 1, .connected = true},
      SessionInfo{.player = 1, .governor = 2, .connected = true},
      SessionInfo{.player = 2, .governor = 1, .connected = false},
  };

  // 1. Rollback discards staged socket notifications while offline telegrams
  // were written to SQLite inside the caller's transaction
  registry.begin_outbox();
  test::expect_true(registry.outbox_active());
  warn_player(registry, em, 1, 1, "Staged online warning\n");
  warn_player(registry, em, 2, 1, "Offline telegram warning\n");
  registry.notify_race(1, "Staged broadcast\n");

  test::expect_true(registry.notifications.empty());
  test::expect_eq(em.get_telegrams(2, 1).size(), 1u);

  registry.rollback_outbox();
  test::expect_false(registry.outbox_active());
  test::expect_true(registry.notifications.empty());

  // 2. Commit flushes staged notifications and returns any that disconnected
  registry.begin_outbox();
  warn_player(registry, em, 1, 1, "Committed warning\n");
  warn_player(registry, em, 1, 2, "Disconnected before commit\n");
  registry.notify_race(1, "Committed broadcast\n");

  // Simulate (1, 2) disconnecting before commit_outbox()
  registry.sessions[1].connected = false;
  auto undelivered = registry.commit_outbox();
  test::expect_false(registry.outbox_active());
  test::expect_eq(registry.notifications.size(), 2u);
  test::expect_true(registry.has_received(1, "Committed warning"));
  test::expect_true(registry.has_broadcast("Committed broadcast"));
  test::expect_eq(undelivered.size(), 1u);
  test::expect_eq(undelivered[0].player, 1);
  test::expect_eq(undelivered[0].governor, 2);

  std::println(std::cout, "  ✓ SessionRegistry transactional outbox passed");
}

int main() {
  std::println(std::cout,
               "Running notification service comprehensive tests...\n");

  test_notify_player_basic();
  test_notify_race_basic();
  test_disconnected_sessions();
  test_d_broadcast_announce_think_and_shout();
  test_warn_player_and_leader_fallback();
  test_warn_race_all_governors();
  test_notify_star();
  test_warn_star_and_telegram_star();
  test_transactional_socket_outbox();

  std::println(std::cout, "\n✅ All notification service tests passed!");
  return 0;
}
