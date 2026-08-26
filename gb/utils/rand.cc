// SPDX-License-Identifier: Apache-2.0

/// \file rand.cc
/// \brief Random number generators.

module;

#include <cstdlib>

import std;

module gblib;

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

/* double double_rand() this returns a random number between 0 and 1 */
double double_rand() {
  std::uniform_real_distribution<double> dist(0.0, 1.0);
  return dist(get_rng());
}

/* int int_rand(low,hi) - this returns an integer random number
 * between hi and low, inclusive. */
long long_rand(long low, long hi) {
  if (hi <= low) return low;
  std::uniform_int_distribution<long> dist(low, hi);
  return dist(get_rng());
}

/* int int_rand(low,hi) - this returns an integer random number
 * between hi and low, inclusive. */
int int_rand(int low, int hi) {
  if (hi <= low) return low;
  std::uniform_int_distribution<int> dist(low, hi);
  return dist(get_rng());
}

bool success(int x) {
  return int_rand(1, 100) <= (x);
}
