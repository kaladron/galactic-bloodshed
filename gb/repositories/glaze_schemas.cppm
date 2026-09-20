// SPDX-License-Identifier: Apache-2.0

/// \file glaze_schemas.cppm
/// \brief Internal module partition providing shared Glaze JSON serialization
/// traits for strong IDs, bounded/modular values, coordinates, and
/// PlayerVector.

export module gb.repositories.glaze;

import strong_id;
import glaze.core;
import glaze.json;
import gb.entities;
import std;

export namespace glz {

template <FixedString Tag, typename T>
struct from<JSON, ID<Tag, T>> {
  template <auto Opts>
  static void op(ID<Tag, T>& id, is_context auto&& ctx, auto&& it, auto&& end) {
    T val{};
    parse<JSON>::op<Opts>(val, ctx, it, end);
    id = ID<Tag, T>{val};
  }
};

template <FixedString Tag, typename T>
struct to<JSON, ID<Tag, T>> {
  template <auto Opts>
  static void op(const ID<Tag, T>& id, is_context auto&& ctx, auto&& b,
                 auto&& ix) noexcept {
    serialize<JSON>::op<Opts>(id.value, ctx, b, ix);
  }
};

template <FixedString Tag, typename T, T Min, T Max>
struct from<JSON, Bounded<Tag, T, Min, Max>> {
  template <auto Opts>
  static void op(Bounded<Tag, T, Min, Max>& b, is_context auto&& ctx, auto&& it,
                 auto&& end) {
    T val{};
    parse<JSON>::op<Opts>(val, ctx, it, end);
    b = Bounded<Tag, T, Min, Max>{val};
  }
};

template <FixedString Tag, typename T, T Min, T Max>
struct to<JSON, Bounded<Tag, T, Min, Max>> {
  template <auto Opts>
  static void op(const Bounded<Tag, T, Min, Max>& b, is_context auto&& ctx,
                 auto&& buf, auto&& ix) noexcept {
    serialize<JSON>::op<Opts>(b.value, ctx, buf, ix);
  }
};

template <FixedString Tag, typename T, T Modulus>
struct from<JSON, Modular<Tag, T, Modulus>> {
  template <auto Opts>
  static void op(Modular<Tag, T, Modulus>& m, is_context auto&& ctx, auto&& it,
                 auto&& end) {
    T val{};
    parse<JSON>::op<Opts>(val, ctx, it, end);
    m = Modular<Tag, T, Modulus>{val};
  }
};

template <FixedString Tag, typename T, T Modulus>
struct to<JSON, Modular<Tag, T, Modulus>> {
  template <auto Opts>
  static void op(const Modular<Tag, T, Modulus>& m, is_context auto&& ctx,
                 auto&& buf, auto&& ix) noexcept {
    serialize<JSON>::op<Opts>(m.value, ctx, buf, ix);
  }
};

template <std::size_t N>
struct from<JSON, PlayerBitset<N>> {
  template <auto Opts>
  static void op(PlayerBitset<N>& bitset, is_context auto&& ctx, auto&& it,
                 auto&& end) {
    unsigned long long val{};
    parse<JSON>::op<Opts>(val, ctx, it, end);
    bitset = PlayerBitset<N>{val};
  }
};

template <std::size_t N>
struct to<JSON, PlayerBitset<N>> {
  template <auto Opts>
  static void op(const PlayerBitset<N>& bitset, is_context auto&& ctx,
                 auto&& buf, auto&& ix) noexcept {
    auto val = bitset.to_ullong();
    serialize<JSON>::op<Opts>(val, ctx, buf, ix);
  }
};

template <typename T, std::size_t N>
struct meta<PlayerVector<T, N>> {
  using Type = PlayerVector<T, N>;
  static constexpr auto value = [](auto&& self) -> auto& {
    return self.raw_array();
  };
};

template <>
struct meta<Coordinates> {
  using T = Coordinates;
  static constexpr auto value = object("x", &T::x, "y", &T::y);
};

template <>
struct meta<UniverseCoordinates> {
  using T = UniverseCoordinates;
  static constexpr auto value = object("x", &T::x, "y", &T::y);
};

template <>
struct meta<SystemCoordinates> {
  using T = SystemCoordinates;
  static constexpr auto value = object("x", &T::x, "y", &T::y);
};

template <>
struct meta<toggletype> {
  using T = toggletype;
  static constexpr auto value =
      object("invisible", &T::invisible, "gag", &T::gag, "double_digits",
             &T::double_digits, "inverse", &T::inverse, "geography",
             &T::geography, "autoload", &T::autoload, "highlight",
             &T::highlight, "compat", &T::compat);
};

template <>
struct meta<SectorCompatibilities> {
  using T = SectorCompatibilities;
  static constexpr auto value =
      object("sea", &T::sea, "land", &T::land, "mount", &T::mount, "gas",
             &T::gas, "ice", &T::ice, "forest", &T::forest, "desert",
             &T::desert, "plated", &T::plated, "wasted", &T::wasted);
};

template <>
struct meta<ConditionValues<int>> {
  using T = ConditionValues<int>;
  static constexpr auto value =
      object("rtemp", &T::rtemp, "temp", &T::temp, "methane", &T::methane,
             "oxygen", &T::oxygen, "co2", &T::co2, "hydrogen", &T::hydrogen,
             "nitrogen", &T::nitrogen, "sulfur", &T::sulfur, "helium",
             &T::helium, "other", &T::other, "toxic", &T::toxic);
};

template <>
struct meta<NewsValues<int>> {
  using T = NewsValues<int>;
  static constexpr auto value =
      object("announce", &T::announce, "combat", &T::combat, "declaration",
             &T::declaration, "transfer", &T::transfer);
};

}  // namespace glz
