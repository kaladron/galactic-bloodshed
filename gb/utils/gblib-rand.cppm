// SPDX-License-Identifier: Apache-2.0

export module gblib:rand;

import std;

export void seed_rand(unsigned int seed);
export bool success(int x);
export double double_rand();
export int int_rand(int low, int high);
export long long_rand(long low, long high);
export std::mt19937& game_rng();

export template <std::integral Result = int>
Result round_rand(std::floating_point auto x) {
  const double d = static_cast<double>(x);
  const double floor_x = std::floor(d);
  const auto rounded =
      (double_rand() > (d - floor_x)) ? floor_x : (floor_x + 1.0);
  return static_cast<Result>(rounded);
}

export template <std::integral Result = int>
Result round_rand(std::integral auto num, std::integral auto denom) {
  if (denom == 0) return Result{0};
  return round_rand<Result>(static_cast<double>(num) /
                            static_cast<double>(denom));
}

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
