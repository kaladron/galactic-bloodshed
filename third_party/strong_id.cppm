// SPDX-License-Identifier: Apache-2.0

/**
 * @file strong_id.ixx
 * @brief Zero-overhead strong type identifiers (C++ Module version).
 */

export module strong_id;

// We use the unified standard library module available in C++23/26
import std;

// -----------------------------------------------------------------------------
// 1. FixedString Helper
// -----------------------------------------------------------------------------

/**
 * @brief Helper struct to enable string literals as template parameters.
 * @tparam N The length of the string literal (automatically deduced).
 */
export template <unsigned N>
struct FixedString {
  char buf[N + 1]{};

  constexpr FixedString(char const* s) {
    for (unsigned i = 0; i != N; ++i)
      buf[i] = s[i];
  }
  constexpr bool operator==(const FixedString&) const = default;
};

/**
 * @brief Deduction guide to automatically deduce the size N.
 */
export template <unsigned N>
FixedString(char const (&)[N]) -> FixedString<N - 1>;

// -----------------------------------------------------------------------------
// 2. The Strong ID Class
// -----------------------------------------------------------------------------

/**
 * @brief A strongly-typed wrapper around an integral value.
 * @tparam Tag A unique string literal identifier (e.g., "player").
 * @tparam T The underlying storage type (defaults to `int`).
 */
export template <FixedString Tag, typename T = int>
  requires std::integral<T>
class ID {
public:
  using value_type = T;
  using difference_type = std::ptrdiff_t;

  T value;

  // TRIVIAL DEFAULT CONSTRUCTOR (Uninitialized/Garbage value)
  constexpr ID() = default;

  // VALUE CONSTRUCTOR (Not explicit, allows p = 5)
  constexpr ID(T v) : value(v) {}

  // CONVERSIONS
  [[nodiscard]] explicit constexpr operator T() const {
    return value;
  }
  [[nodiscard]] constexpr T operator*() const {
    return value;
  }

  // INCREMENT / DECREMENT
  constexpr ID& operator++() {
    ++value;
    return *this;
  }
  constexpr ID operator++(int) {
    ID temp = *this;
    ++value;
    return temp;
  }
  constexpr ID& operator--() {
    --value;
    return *this;
  }
  constexpr ID operator--(int) {
    ID temp = *this;
    --value;
    return temp;
  }

  // COMPARISONS
  [[nodiscard]] auto operator<=>(const ID&) const = default;
  [[nodiscard]] friend constexpr bool operator==(const ID&,
                                                 const ID&) = default;

  // ADL SWAP
  friend constexpr void swap(ID& lhs, ID& rhs) noexcept {
    T temp = lhs.value;
    lhs.value = rhs.value;
    rhs.value = temp;
  }

  // STREAM OUTPUT
  friend std::ostream& operator<<(std::ostream& os, const ID& id) {
    return os << id.value;
  }
};

/**
 * @brief Extract the underlying value from a strong ID (or return primitive
 * as-is).
 */
export template <typename T>
constexpr auto to_underlying(T&& v) noexcept {
  if constexpr (requires { v.value; }) {
    return v.value;
  } else {
    return std::forward<T>(v);
  }
}

export template <typename T>
struct underlying_type {
  using type = T;
};

export template <FixedString Tag, typename T>
struct underlying_type<ID<Tag, T>> {
  using type = T;
};

export template <typename T>
using underlying_type_t = typename underlying_type<T>::type;

// -----------------------------------------------------------------------------
// 3. The Bounded Class
// -----------------------------------------------------------------------------

/**
 * @brief A strongly-typed wrapper guaranteeing numerical values stay within
 * [Min, Max].
 *
 * Automatically clamps upon construction and arithmetic mutation (+, -, *, /).
 *
 * @tparam Tag A unique string literal identifier (e.g., "damage").
 * @tparam T The underlying numerical storage type (integral or floating-point).
 * @tparam Min Minimum bounded value.
 * @tparam Max Maximum bounded value.
 */
export template <FixedString Tag, typename T, T Min, T Max>
  requires(std::integral<T> || std::floating_point<T>)
class Bounded {
  static_assert(Min <= Max, "Bounded: Min must be less than or equal to Max");

public:
  using value_type = T;
  using difference_type = std::ptrdiff_t;

  T value;

  // DEFAULT CONSTRUCTOR (Trivial default constructible)
  constexpr Bounded() = default;

  // VALUE CONSTRUCTORS
  constexpr explicit Bounded(T v) noexcept : value(std::clamp(v, Min, Max)) {}

  // CONVERSIONS & ACCESSORS
  [[nodiscard]] constexpr T get() const noexcept {
    return value;
  }
  [[nodiscard]] explicit constexpr operator T() const noexcept {
    return value;
  }
  [[nodiscard]] constexpr T operator*() const noexcept {
    return value;
  }
  [[nodiscard]] static constexpr T min() noexcept {
    return Min;
  }
  [[nodiscard]] static constexpr T max() noexcept {
    return Max;
  }

  // MUTATING ARITHMETIC (WITH AUTOMATIC BOUNDS CLAMPING & OVERFLOW SAFETY)
  constexpr Bounded& operator+=(T delta) noexcept {
    if constexpr (std::unsigned_integral<T>) {
      if (delta >= Max - value) {
        value = Max;
      } else {
        value = std::clamp(static_cast<T>(value + delta), Min, Max);
      }
    } else {
      value = std::clamp(value + delta, Min, Max);
    }
    return *this;
  }
  constexpr Bounded& operator-=(T delta) noexcept {
    if constexpr (std::unsigned_integral<T>) {
      if (delta >= value - Min) {
        value = Min;
      } else {
        value = std::clamp(static_cast<T>(value - delta), Min, Max);
      }
    } else {
      value = std::clamp(value - delta, Min, Max);
    }
    return *this;
  }
  constexpr Bounded& operator*=(T factor) noexcept {
    if constexpr (std::unsigned_integral<T>) {
      if (factor == T{0}) {
        value = Min;
      } else if (value > Max / factor) {
        value = Max;
      } else {
        value = std::clamp(static_cast<T>(value * factor), Min, Max);
      }
    } else {
      value = std::clamp(value * factor, Min, Max);
    }
    return *this;
  }
  constexpr Bounded& operator/=(T divisor) noexcept {
    if (divisor != T{0}) {
      value = std::clamp(value / divisor, Min, Max);
    }
    return *this;
  }

  // NON-MUTATING BINARY OPERATORS
  friend constexpr Bounded operator+(Bounded lhs, T rhs) noexcept {
    lhs += rhs;
    return lhs;
  }
  friend constexpr Bounded operator+(T lhs, Bounded rhs) noexcept {
    Bounded b(lhs);
    b += rhs.value;
    return b;
  }
  friend constexpr Bounded operator-(Bounded lhs, T rhs) noexcept {
    lhs -= rhs;
    return lhs;
  }

  // INCREMENT / DECREMENT
  constexpr Bounded& operator++() noexcept {
    if (value < Max) {
      ++value;
    }
    return *this;
  }
  constexpr Bounded operator++(int) noexcept {
    Bounded temp = *this;
    ++(*this);
    return temp;
  }
  constexpr Bounded& operator--() noexcept {
    if (value > Min) {
      --value;
    }
    return *this;
  }
  constexpr Bounded operator--(int) noexcept {
    Bounded temp = *this;
    --(*this);
    return temp;
  }

  // COMPARISONS
  [[nodiscard]] auto operator<=>(const Bounded&) const = default;
  [[nodiscard]] friend constexpr bool operator==(const Bounded&,
                                                 const Bounded&) = default;
  [[nodiscard]] friend constexpr auto operator<=>(const Bounded& b,
                                                  T v) noexcept {
    return b.value <=> v;
  }
  [[nodiscard]] friend constexpr bool operator==(const Bounded& b,
                                                 T v) noexcept {
    return b.value == v;
  }

  // ADL SWAP
  friend constexpr void swap(Bounded& lhs, Bounded& rhs) noexcept {
    std::swap(lhs.value, rhs.value);
  }

  // STREAM OUTPUT
  friend std::ostream& operator<<(std::ostream& os, const Bounded& b) {
    return os << b.value;
  }
};

export template <FixedString Tag, typename T, T Min, T Max>
struct underlying_type<Bounded<Tag, T, Min, Max>> {
  using type = T;
};

// -----------------------------------------------------------------------------
// 4. The Modular Class
// -----------------------------------------------------------------------------

/**
 * @brief A strongly-typed wrapper guaranteeing numerical values wrap modulo
 * Mod.
 *
 * Automatically wraps into [0, Mod - 1] upon construction and arithmetic.
 * Useful for headings, angles, or cyclic states (e.g. 0..359 degrees).
 *
 * @tparam Tag A unique string literal identifier (e.g., "bearing").
 * @tparam T The underlying integral storage type.
 * @tparam Mod Modulo cycle modulus (must be strictly positive).
 */
export template <FixedString Tag, typename T, T Mod>
  requires std::integral<T>
class Modular {
  static_assert(Mod > 0, "Modular: Mod must be strictly positive");

  static constexpr T normalize(T v) noexcept {
    if constexpr (std::unsigned_integral<T>) {
      return v % Mod;
    } else {
      return ((v % Mod) + Mod) % Mod;
    }
  }

public:
  using value_type = T;
  using difference_type = std::ptrdiff_t;

  T value;

  // DEFAULT CONSTRUCTOR (Trivial default constructible)
  constexpr Modular() = default;

  // VALUE CONSTRUCTOR
  constexpr explicit Modular(T v) noexcept : value(normalize(v)) {}

  // CONVERSIONS & ACCESSORS
  [[nodiscard]] constexpr T get() const noexcept {
    return value;
  }
  [[nodiscard]] explicit constexpr operator T() const noexcept {
    return value;
  }
  [[nodiscard]] constexpr T operator*() const noexcept {
    return value;
  }
  [[nodiscard]] static constexpr T modulus() noexcept {
    return Mod;
  }

  // MUTATING ARITHMETIC (WITH AUTOMATIC MODULO WRAPPING & UNDERFLOW SAFETY)
  constexpr Modular& operator+=(T delta) noexcept {
    if constexpr (std::unsigned_integral<T>) {
      value = (value + (delta % Mod)) % Mod;
    } else {
      value = normalize(value + delta);
    }
    return *this;
  }
  constexpr Modular& operator-=(T delta) noexcept {
    if constexpr (std::unsigned_integral<T>) {
      value = (value + Mod - (delta % Mod)) % Mod;
    } else {
      value = normalize(value - delta);
    }
    return *this;
  }

  // NON-MUTATING BINARY OPERATORS
  friend constexpr Modular operator+(Modular lhs, T rhs) noexcept {
    lhs += rhs;
    return lhs;
  }
  friend constexpr Modular operator+(T lhs, Modular rhs) noexcept {
    Modular m(lhs);
    m += rhs.value;
    return m;
  }
  friend constexpr Modular operator-(Modular lhs, T rhs) noexcept {
    lhs -= rhs;
    return lhs;
  }

  // INCREMENT / DECREMENT
  constexpr Modular& operator++() noexcept {
    if (value + 1 >= Mod) {
      value = 0;
    } else {
      ++value;
    }
    return *this;
  }
  constexpr Modular operator++(int) noexcept {
    Modular temp = *this;
    ++(*this);
    return temp;
  }
  constexpr Modular& operator--() noexcept {
    if (value == 0) {
      value = Mod - 1;
    } else {
      --value;
    }
    return *this;
  }
  constexpr Modular operator--(int) noexcept {
    Modular temp = *this;
    --(*this);
    return temp;
  }

  // COMPARISONS
  [[nodiscard]] auto operator<=>(const Modular&) const = default;
  [[nodiscard]] friend constexpr bool operator==(const Modular&,
                                                 const Modular&) = default;
  [[nodiscard]] friend constexpr auto operator<=>(const Modular& m,
                                                  T v) noexcept {
    return m.value <=> v;
  }
  [[nodiscard]] friend constexpr bool operator==(const Modular& m,
                                                 T v) noexcept {
    return m.value == v;
  }

  // ADL SWAP
  friend constexpr void swap(Modular& lhs, Modular& rhs) noexcept {
    std::swap(lhs.value, rhs.value);
  }

  // STREAM OUTPUT
  friend std::ostream& operator<<(std::ostream& os, const Modular& m) {
    return os << m.value;
  }
};

export template <FixedString Tag, typename T, T Mod>
struct underlying_type<Modular<Tag, T, Mod>> {
  using type = T;
};

// -----------------------------------------------------------------------------
// 5. Standard Library Specializations
// -----------------------------------------------------------------------------
// Note: We do not 'export' namespace std, but these specializations become
// visible/reachable when this module is imported.

namespace std {

// Hash support for ID
template <FixedString Tag, typename T>
struct hash<ID<Tag, T>> {
  size_t operator()(const ID<Tag, T>& id) const noexcept {
    return std::hash<T>{}(id.value);
  }
};

// Formatter support for ID
template <FixedString Tag, typename T>
struct formatter<ID<Tag, T>> : formatter<T> {
  auto format(const ID<Tag, T>& id, auto& ctx) const {
    return formatter<T>::format(id.value, ctx);
  }
};

// Numeric Limits for ID
template <FixedString Tag, typename T>
struct numeric_limits<ID<Tag, T>> : public numeric_limits<T> {
  static constexpr ID<Tag, T> min() noexcept {
    return ID<Tag, T>(numeric_limits<T>::min());
  }
  static constexpr ID<Tag, T> max() noexcept {
    return ID<Tag, T>(numeric_limits<T>::max());
  }
  static constexpr ID<Tag, T> lowest() noexcept {
    return ID<Tag, T>(numeric_limits<T>::lowest());
  }
  static constexpr ID<Tag, T> epsilon() noexcept {
    return ID<Tag, T>(numeric_limits<T>::epsilon());
  }
  static constexpr ID<Tag, T> round_error() noexcept {
    return ID<Tag, T>(numeric_limits<T>::round_error());
  }
  static constexpr ID<Tag, T> infinity() noexcept {
    return ID<Tag, T>(numeric_limits<T>::infinity());
  }
  static constexpr ID<Tag, T> quiet_NaN() noexcept {
    return ID<Tag, T>(numeric_limits<T>::quiet_NaN());
  }
  static constexpr ID<Tag, T> signaling_NaN() noexcept {
    return ID<Tag, T>(numeric_limits<T>::signaling_NaN());
  }
  static constexpr ID<Tag, T> denorm_min() noexcept {
    return ID<Tag, T>(numeric_limits<T>::denorm_min());
  }
};

// Hash support for Bounded
template <FixedString Tag, typename T, T Min, T Max>
struct hash<Bounded<Tag, T, Min, Max>> {
  size_t operator()(const Bounded<Tag, T, Min, Max>& b) const noexcept {
    return std::hash<T>{}(b.value);
  }
};

// Formatter support for Bounded
template <FixedString Tag, typename T, T Min, T Max>
struct formatter<Bounded<Tag, T, Min, Max>> : formatter<T> {
  auto format(const Bounded<Tag, T, Min, Max>& b, auto& ctx) const {
    return formatter<T>::format(b.value, ctx);
  }
};

// Numeric Limits for Bounded
template <FixedString Tag, typename T, T Min, T Max>
struct numeric_limits<Bounded<Tag, T, Min, Max>> : public numeric_limits<T> {
  static constexpr Bounded<Tag, T, Min, Max> min() noexcept {
    return Bounded<Tag, T, Min, Max>(Min);
  }
  static constexpr Bounded<Tag, T, Min, Max> max() noexcept {
    return Bounded<Tag, T, Min, Max>(Max);
  }
  static constexpr Bounded<Tag, T, Min, Max> lowest() noexcept {
    return Bounded<Tag, T, Min, Max>(Min);
  }
};

// Hash support for Modular
template <FixedString Tag, typename T, T Mod>
struct hash<Modular<Tag, T, Mod>> {
  size_t operator()(const Modular<Tag, T, Mod>& m) const noexcept {
    return std::hash<T>{}(m.value);
  }
};

// Formatter support for Modular
template <FixedString Tag, typename T, T Mod>
struct formatter<Modular<Tag, T, Mod>> : formatter<T> {
  auto format(const Modular<Tag, T, Mod>& m, auto& ctx) const {
    return formatter<T>::format(m.value, ctx);
  }
};

}  // namespace std

// -----------------------------------------------------------------------------
// 6. Compile-Time Verification
// -----------------------------------------------------------------------------
// Not exported, these check internal consistency during module compilation.

module :private;  // Optional: Hide implementation details if splitting file

namespace strong_id_traits_check {
using test_t = ID<"check">;
using test_bounded_t = Bounded<"check_bounded", std::uint32_t, 0, 100>;
using test_modular_t = Modular<"check_modular", std::uint32_t, 360>;

static_assert(std::regular<test_t>);
static_assert(std::is_trivially_copyable_v<test_t> &&
              std::is_trivially_default_constructible_v<test_t>);
static_assert(std::is_standard_layout_v<test_t>);
static_assert(sizeof(test_t) == sizeof(int));

static_assert(std::regular<test_bounded_t>);
static_assert(std::is_trivially_copyable_v<test_bounded_t> &&
              std::is_trivially_default_constructible_v<test_bounded_t>);
static_assert(std::is_standard_layout_v<test_bounded_t>);
static_assert(sizeof(test_bounded_t) == sizeof(std::uint32_t));

static_assert(std::regular<test_modular_t>);
static_assert(std::is_trivially_copyable_v<test_modular_t> &&
              std::is_trivially_default_constructible_v<test_modular_t>);
static_assert(std::is_standard_layout_v<test_modular_t>);
static_assert(sizeof(test_modular_t) == sizeof(std::uint32_t));
}  // namespace strong_id_traits_check