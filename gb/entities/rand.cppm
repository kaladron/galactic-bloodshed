// SPDX-License-Identifier: Apache-2.0
/// \file rand.cppm
/// \brief Random number generation utilities.

export module gb.entities:rand;

import std;

export void seed_rand(unsigned int seed);
export bool success(int x);
export bool bool_rand(double p = 0.5);
export std::mt19937& game_rng();

export template <typename Target>
  requires((std::integral<Target> || std::floating_point<Target>) &&
           !std::same_as<Target, bool>)
struct RandBound {
  Target value;

  template <typename U>
    requires(std::convertible_to<U, Target> && requires(U u) { Target{u}; })
  constexpr RandBound(U v) : value(static_cast<Target>(v)) {}

  template <typename U>
    requires((std::integral<U> || std::floating_point<U>) &&
             !requires(U u) { Target{u}; })
  consteval RandBound(U v) : value(checked_cast(v)) {}

private:
  template <typename U>
  static consteval Target checked_cast(U v) {
    if constexpr (std::integral<Target> && std::integral<U>) {
      if (!std::in_range<Target>(v)) {
        throw "uniform_rand bound is out of range for target integer type";
      }
    } else if constexpr (std::integral<Target> && std::floating_point<U>) {
      constexpr auto min_v = static_cast<U>(std::numeric_limits<Target>::min());
      constexpr auto max_v = static_cast<U>(std::numeric_limits<Target>::max());
      if (v < min_v || v > max_v ||
          static_cast<U>(static_cast<Target>(v)) != v) {
        throw "uniform_rand floating-point bound is out of range or not an "
              "exact integer";
      }
    }
    return static_cast<Target>(v);
  }
};

export template <typename T>
  requires((std::integral<T> || std::floating_point<T>) &&
           !std::same_as<T, bool>)
T uniform_rand(std::type_identity_t<RandBound<T>> low,
               std::type_identity_t<RandBound<T>> high) {
  if (high.value <= low.value) return low.value;
  if constexpr (std::integral<T>) {
    using DistType = std::conditional_t<
        sizeof(T) < sizeof(int),
        std::conditional_t<std::is_signed_v<T>, int, unsigned int>, T>;
    std::uniform_int_distribution<DistType> dist(
        static_cast<DistType>(low.value), static_cast<DistType>(high.value));
    return static_cast<T>(dist(game_rng()));
  } else {
    std::uniform_real_distribution<T> dist(low.value, high.value);
    return dist(game_rng());
  }
}

export template <int = 0, typename T>
  requires((std::integral<T> || std::floating_point<T>) &&
           !std::same_as<T, bool>)
T uniform_rand(std::type_identity_t<RandBound<T>> low, T high) {
  return uniform_rand<T>(low, RandBound<T>{high});
}

export inline double uniform_rand() {
  return uniform_rand(0.0, 1.0);
}

export template <std::integral Result = int>
Result round_rand(std::floating_point auto x) {
  const auto d = static_cast<double>(x);
  const auto floor_x = std::floor(d);
  const auto rounded =
      (uniform_rand() > (d - floor_x)) ? floor_x : (floor_x + 1.0);
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
