// SPDX-License-Identifier: Apache-2.0

/// \file player_bitset_test.cc
/// \brief Unit tests for PlayerBitset container, operators, iteration, and JSON
/// serialization.

import gb.entities;
import gb.repositories;
import test;
import glaze.core;
import glaze.json;
import std;

struct MockRace {
  player_t Playernum;
};

int main() {
  std::println(std::cout, "Running PlayerBitset unit tests...\n");

  // 1. Default construction
  std::println(std::cout, "Default construction...");
  {
    PlayerBitset<MAXPLAYERS> bitset;
    test::expect_true(bitset.none());
    test::expect_false(bitset.any());
    test::expect_false(bitset.all());
    test::expect_eq(bitset.count(), 0zu);
    test::expect_eq(bitset.size(), static_cast<std::size_t>(MAXPLAYERS));
    test::expect_eq(bitset.to_ullong(), 0ULL);
    std::println(std::cout, "  ✓ Default construction produces empty bitset");
  }

  // 2. Integer bitmask construction
  std::println(std::cout, "Integer bitmask construction...");
  {
    // Bit 0 corresponds to player 1, bit 2 corresponds to player 3
    PlayerBitset<MAXPLAYERS> bitset(0b101ULL);
    test::expect_true(bitset.test(player_t{1}));
    test::expect_false(bitset.test(player_t{2}));
    test::expect_true(bitset.test(player_t{3}));
    test::expect_false(bitset.test(player_t{4}));
    test::expect_eq(bitset.count(), 2zu);
    test::expect_eq(bitset.to_ullong(), 0b101ULL);
    std::println(std::cout,
                 "  ✓ Integer bitmask construction matches bit layout");
  }

  // 3. Singleton factory
  std::println(std::cout, "Singleton factory...");
  {
    auto bitset = PlayerBitset<MAXPLAYERS>::singleton(player_t{5});
    test::expect_true(bitset.test(player_t{5}));
    test::expect_eq(bitset.count(), 1zu);
    test::expect_false(bitset.test(player_t{1}));
    std::println(std::cout, "  ✓ Singleton factory works as expected");
  }

  // 4. set, reset, flip, test with player_t
  std::println(std::cout, "set, reset, flip, and test operations...");
  {
    PlayerBitset<MAXPLAYERS> bitset;

    bitset.set(player_t{1});
    test::expect_true(bitset.test(player_t{1}));
    test::expect_false(bitset.test(player_t{2}));

    bitset.set(player_t{MAXPLAYERS});
    test::expect_true(bitset.test(player_t{MAXPLAYERS}));
    test::expect_eq(bitset.count(), 2zu);

    bitset.reset(player_t{1});
    test::expect_false(bitset.test(player_t{1}));
    test::expect_true(bitset.test(player_t{MAXPLAYERS}));
    test::expect_eq(bitset.count(), 1zu);

    bitset.flip(player_t{1});
    test::expect_true(bitset.test(player_t{1}));
    bitset.flip(player_t{1});
    test::expect_false(bitset.test(player_t{1}));

    // Bulk set, reset, flip
    bitset.set();
    test::expect_true(bitset.all());
    test::expect_eq(bitset.count(), static_cast<std::size_t>(MAXPLAYERS));

    bitset.reset();
    test::expect_true(bitset.none());

    bitset.flip();
    test::expect_true(bitset.all());
    std::println(std::cout, "  ✓ set, reset, and flip work correctly");
  }

  // 5. Subscript operator
  std::println(std::cout, "Subscript operator...");
  {
    PlayerBitset<MAXPLAYERS> bitset;

    bitset[player_t{3}] = true;
    test::expect_true(bitset[player_t{3}]);
    test::expect_false(bitset[player_t{4}]);

    const auto& cbitset = bitset;
    test::expect_true(cbitset[player_t{3}]);
    test::expect_false(cbitset[player_t{4}]);

    bitset[player_t{3}] = false;
    test::expect_false(bitset[player_t{3}]);
    std::println(std::cout, "  ✓ Subscript operator works for read and write");
  }

  // 6. RaceLike indexing
  std::println(std::cout, "RaceLike indexing...");
  {
    PlayerBitset<MAXPLAYERS> bitset;
    MockRace r1{player_t{2}};
    MockRace r2{player_t{4}};

    bitset.set(r1);
    test::expect_true(bitset.test(r1));
    test::expect_false(bitset.test(r2));
    test::expect_true(bitset[r1]);
    test::expect_false(bitset[r2]);

    bitset[r2] = true;
    test::expect_true(bitset.test(r2));

    bitset.reset(r1);
    test::expect_false(bitset.test(r1));
    test::expect_true(bitset.test(r2));

    bitset.flip(r2);
    test::expect_false(bitset.test(r2));
    std::println(std::cout, "  ✓ RaceLike operations work as expected");
  }

  // 7. Bounds checking
  std::println(std::cout, "Bounds checking...");
  {
    PlayerBitset<MAXPLAYERS> bitset;

    test::expect_throws<std::out_of_range>(
        [&]() { (void)bitset.test(player_t{0}); });
    test::expect_throws<std::out_of_range>(
        [&]() { (void)bitset.test(player_t{MAXPLAYERS + 1}); });
    test::expect_throws<std::out_of_range>([&]() { bitset.set(player_t{0}); });
    test::expect_throws<std::out_of_range>(
        [&]() { bitset.set(player_t{MAXPLAYERS + 1}); });
    test::expect_throws<std::out_of_range>(
        [&]() { bitset.reset(player_t{0}); });
    test::expect_throws<std::out_of_range>(
        [&]() { bitset.reset(player_t{MAXPLAYERS + 1}); });
    test::expect_throws<std::out_of_range>([&]() { bitset.flip(player_t{0}); });
    test::expect_throws<std::out_of_range>(
        [&]() { bitset.flip(player_t{MAXPLAYERS + 1}); });
    test::expect_throws<std::out_of_range>(
        [&]() { (void)bitset[player_t{0}]; });
    test::expect_throws<std::out_of_range>(
        [&]() { (void)bitset[player_t{MAXPLAYERS + 1}]; });

    const auto& cbitset = bitset;
    test::expect_throws<std::out_of_range>(
        [&]() { (void)cbitset[player_t{0}]; });
    test::expect_throws<std::out_of_range>(
        [&]() { (void)cbitset[player_t{MAXPLAYERS + 1}]; });
    std::println(std::cout, "  ✓ Bounds checking rejects invalid player IDs");
  }

  // 8. Bitwise operators
  std::println(std::cout, "Bitwise operators...");
  {
    PlayerBitset<MAXPLAYERS> a;
    PlayerBitset<MAXPLAYERS> b;

    a.set(player_t{1});
    a.set(player_t{2});

    b.set(player_t{2});
    b.set(player_t{3});

    auto and_res = a & b;
    test::expect_eq(and_res.count(), 1zu);
    test::expect_true(and_res.test(player_t{2}));

    auto or_res = a | b;
    test::expect_eq(or_res.count(), 3zu);
    test::expect_true(or_res.test(player_t{1}));
    test::expect_true(or_res.test(player_t{2}));
    test::expect_true(or_res.test(player_t{3}));

    auto xor_res = a ^ b;
    test::expect_eq(xor_res.count(), 2zu);
    test::expect_true(xor_res.test(player_t{1}));
    test::expect_false(xor_res.test(player_t{2}));
    test::expect_true(xor_res.test(player_t{3}));

    auto not_a = ~a;
    test::expect_false(not_a.test(player_t{1}));
    test::expect_false(not_a.test(player_t{2}));
    test::expect_true(not_a.test(player_t{3}));

    a &= b;
    test::expect_eq(a, and_res);

    a |= b;
    test::expect_eq(a, b);

    a ^= b;
    test::expect_true(a.none());
    std::println(std::cout,
                 "  ✓ Bitwise operators &, |, ^, ~ behave correctly");
  }

  // 9. Range view iteration over set players
  std::println(std::cout, "Range view iteration (.players())...");
  {
    PlayerBitset<MAXPLAYERS> bitset;
    bitset.set(player_t{2});
    bitset.set(player_t{5});
    bitset.set(player_t{9});

    std::vector<player_t> visited;
    for (player_t p : bitset.players()) {
      visited.push_back(p);
    }

    test::expect_eq(visited.size(), 3zu);
    test::expect_eq(visited[0], player_t{2});
    test::expect_eq(visited[1], player_t{5});
    test::expect_eq(visited[2], player_t{9});
    std::println(std::cout,
                 "  ✓ .players() range view iterates over set players");
  }

  // 10. String conversion and std::format
  std::println(std::cout, "String conversion and formatting...");
  {
    PlayerBitset<4> small;
    small.set(player_t{1});
    small.set(player_t{3});

    // In std::bitset::to_string(), bits are displayed MSB to LSB:
    // bit 3 is '0', bit 2 (player 3) is '1', bit 1 is '0', bit 0 (player 1) is
    // '1'
    test::expect_eq(small.to_string(), "0101");
    test::expect_eq(std::format("{}", small), "0101");
    std::println(std::cout, "  ✓ String conversion and std::format work");
  }

  // 11. Glaze JSON round-trip serialization
  std::println(std::cout, "Glaze JSON serialization...");
  {
    PlayerBitset<MAXPLAYERS> original;
    original.set(player_t{1});
    original.set(player_t{3});
    original.set(player_t{7});

    auto json_str = glz::write_json(original);
    test::expect_true(json_str.has_value());

    PlayerBitset<MAXPLAYERS> deserialized;
    auto ec = glz::read_json(deserialized, json_str.value());
    test::expect_false(static_cast<bool>(ec));
    test::expect_eq(deserialized, original);
    test::expect_true(deserialized.test(player_t{1}));
    test::expect_true(deserialized.test(player_t{3}));
    test::expect_true(deserialized.test(player_t{7}));
    test::expect_false(deserialized.test(player_t{2}));
    std::println(std::cout, "  ✓ Glaze JSON round-trip serialization succeeds");
  }

  std::println(std::cout, "\n✓ All PlayerBitset unit tests passed!");
  return 0;
}
