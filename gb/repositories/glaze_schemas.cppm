// SPDX-License-Identifier: Apache-2.0

/// \file glaze_schemas.cppm
/// \brief Internal module partition providing shared Glaze JSON serialization
/// traits for strong IDs, bounded/modular values, and coordinates.

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

template <>
struct from<JSON, Percentage> {
  template <auto Opts>
  static void op(Percentage& p, is_context auto&& ctx, auto&& it, auto&& end) {
    int val{};
    parse<JSON>::op<Opts>(val, ctx, it, end);
    p = Percentage{val};
  }
};

template <>
struct to<JSON, Percentage> {
  template <auto Opts>
  static void op(const Percentage& p, is_context auto&& ctx, auto&& buf,
                 auto&& ix) noexcept {
    const int val = p.value();
    serialize<JSON>::op<Opts>(val, ctx, buf, ix);
  }
};

template <>
struct from<JSON, Temperature> {
  template <auto Opts>
  static void op(Temperature& t, is_context auto&& ctx, auto&& it, auto&& end) {
    int val{};
    parse<JSON>::op<Opts>(val, ctx, it, end);
    t = Temperature{val};
  }
};

template <>
struct to<JSON, Temperature> {
  template <auto Opts>
  static void op(const Temperature& t, is_context auto&& ctx, auto&& buf,
                 auto&& ix) noexcept {
    const int val = t.value();
    serialize<JSON>::op<Opts>(val, ctx, buf, ix);
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
             &T::double_digits, "inverse", &T::inverse, "color", &T::color,
             "geography", &T::geography, "autoload", &T::autoload, "highlight",
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
struct meta<ConditionValues> {
  using T = ConditionValues;
  static constexpr auto value =
      object("methane", &T::methane, "oxygen", &T::oxygen, "co2", &T::co2,
             "hydrogen", &T::hydrogen, "nitrogen", &T::nitrogen, "sulfur",
             &T::sulfur, "helium", &T::helium, "other", &T::other);
};

template <typename Val>
struct meta<NewsValues<Val>> {
  using T = NewsValues<Val>;
  static constexpr auto value =
      object("announce", &T::announce, "combat", &T::combat, "declaration",
             &T::declaration, "transfer", &T::transfer);
};

template <>
struct meta<AimedAtData> {
  using T = AimedAtData;
  static constexpr auto value =
      object("shipno", &T::shipno, "snum", &T::snum, "intensity", &T::intensity,
             "pnum", &T::pnum, "level", &T::level);
};

template <>
struct meta<MindData> {
  using T = MindData;
  static constexpr auto value =
      object("progenitor", &T::progenitor, "target_player", &T::target_player,
             "generation", &T::generation, "busy", &T::busy, "tampered",
             &T::tampered, "who_killed", &T::who_killed);
};

template <>
struct meta<PodData> {
  using T = PodData;
  static constexpr auto value =
      object("decay", &T::decay, "temperature", &T::temperature);
};

template <>
struct meta<TimerData> {
  using T = TimerData;
  static constexpr auto value = object("count", &T::count);
};

template <>
struct meta<ImpactData> {
  using T = ImpactData;
  static constexpr auto value =
      object("coords", &T::coords, "scatter", &T::scatter);
};

template <>
struct meta<TriggerData> {
  using T = TriggerData;
  static constexpr auto value = object("radius", &T::radius);
};

template <>
struct meta<TerraformData> {
  using T = TerraformData;
  static constexpr auto value = object("index", &T::index);
};

template <>
struct meta<TransportData> {
  using T = TransportData;
  static constexpr auto value = object("target_ship", &T::target_ship);
};

template <>
struct meta<WasteData> {
  using T = WasteData;
  static constexpr auto value = object("toxic", &T::toxic);
};

template <>
struct meta<NavigateData> {
  using T = NavigateData;
  static constexpr auto value =
      object("on", &T::on, "speed", &T::speed, "turns", &T::turns, "bearing",
             &T::bearing);
};

template <>
struct meta<ProtectData> {
  using T = ProtectData;
  static constexpr auto value =
      object("on", &T::on, "planet", &T::planet, "retaliate", &T::retaliate,
             "evade", &T::evade, "ship", &T::ship);
};

template <>
struct meta<HyperDriveData> {
  using T = HyperDriveData;
  static constexpr auto value =
      object("charge", &T::charge, "on", &T::on, "has", &T::has);
};

template <>
struct meta<GunBattery> {
  using T = GunBattery;
  static constexpr auto value =
      object("count", &T::count, "caliber", &T::caliber);
};

}  // namespace glz
