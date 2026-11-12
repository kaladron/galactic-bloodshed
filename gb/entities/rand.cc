// SPDX-License-Identifier: Apache-2.0

/// \file rand.cc
/// \brief Random number generators.

module;

import std;

module gb.entities;

namespace {
std::mt19937_64 make_seeded_rng() {
  std::random_device rd;
  std::array<std::random_device::result_type, 8> entropy{};
  std::ranges::generate(entropy, std::ref(rd));
  std::seed_seq seq(entropy.begin(), entropy.end());
  return std::mt19937_64(seq);
}

std::mt19937_64& get_rng() {
  thread_local std::mt19937_64 rng = make_seeded_rng();
  return rng;
}
}  // namespace

std::mt19937_64& game_rng() {
  return get_rng();
}

void seed_rand(unsigned int seed) {
  get_rng().seed(seed);
}

bool success(int x) {
  return uniform_rand(1, 100) <= (x);
}

bool bool_rand(double p) {
  std::bernoulli_distribution dist(p);
  return dist(get_rng());
}
