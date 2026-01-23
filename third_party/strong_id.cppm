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

// -----------------------------------------------------------------------------
// 3. Standard Library Specializations
// -----------------------------------------------------------------------------
// Note: We do not 'export' namespace std, but these specializations become
// visible/reachable when this module is imported.

namespace std {

// Hash support
template <FixedString Tag, typename T>
struct hash<ID<Tag, T>> {
  size_t operator()(const ID<Tag, T>& id) const noexcept {
    return std::hash<T>{}(id.value);
  }
};

// Formatter support
template <FixedString Tag, typename T>
struct formatter<ID<Tag, T>> : formatter<T> {
  auto format(const ID<Tag, T>& id, auto& ctx) const {
    return formatter<T>::format(id.value, ctx);
  }
};

// Numeric Limits
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
}  // namespace std

// -----------------------------------------------------------------------------
// 4. Compile-Time Verification
// -----------------------------------------------------------------------------
// Not exported, these check internal consistency during module compilation.

module :private;  // Optional: Hide implementation details if splitting file

namespace strong_id_traits_check {
using test_t = ID<"check">;

static_assert(std::regular<test_t>);
static_assert(std::is_trivially_copyable_v<test_t> &&
              std::is_trivially_default_constructible_v<test_t>);
static_assert(std::is_standard_layout_v<test_t>);
static_assert(std::is_trivially_copyable_v<test_t>);
static_assert(!std::is_integral_v<test_t>);
static_assert(sizeof(test_t) == sizeof(int));
}  // namespace strong_id_traits_check