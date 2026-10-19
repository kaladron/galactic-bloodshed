// SPDX-License-Identifier: Apache-2.0

/// \file rand.cc
/// \brief Random number generators.

module;

#include <cstdlib>

import std;

module gb.entities;

namespace {
std::mt19937& get_rng() {
  thread_local std::mt19937 rng(1337);
  return rng;
}
}  // namespace

std::mt19937& game_rng() {
  return get_rng();
}

void seed_rand(unsigned int seed) {
  get_rng().seed(seed);
  srandom(seed);
}

bool success(int x) {
  return uniform_rand(1, 100) <= (x);
}

bool bool_rand(double p) {
  std::bernoulli_distribution dist(p);
  return dist(get_rng());
}
