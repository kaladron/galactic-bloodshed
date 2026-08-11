// SPDX-License-Identifier: Apache-2.0

export module gblib:rand;

import std;

export void seed_rand(unsigned int seed);
export bool success(int x);
export double double_rand();
export int int_rand(int low, int high);
export long long_rand(long low, long high);
export int round_rand(double);
export std::mt19937& game_rng();

export template <std::integral T>
std::vector<T> shuffled_indices(T count) {
  std::vector<T> indices(count);
  std::ranges::iota(indices, T{0});
  std::ranges::shuffle(indices, game_rng());
  return indices;
}

export template <std::integral T>
std::vector<T> shuffled_indices(T start, T end) {
  if (end <= start) return {};
  std::vector<T> indices(static_cast<std::size_t>(end - start));
  std::ranges::iota(indices, start);
  std::ranges::shuffle(indices, game_rng());
  return indices;
}
