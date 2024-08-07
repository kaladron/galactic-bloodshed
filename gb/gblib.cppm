// SPDX-License-Identifier: Apache-2.0

export module gblib;

export import :files_shl;
export import :misc;
export import :planet;
export import :types;
export import :sector;
export import :race;
export import :rand;
export import :ships;
export import :shlmisc;
export import :sql;
export import :star;
export import :tele;
export import :tweakables;
export import :globals;

/**
 * \brief Convert input string to a shipnum_t
 * \param s User-provided input string
 * \return If the user provided a valid number, return it.
 */
export constexpr std::optional<shipnum_t> string_to_shipnum(
    std::string_view s) {
  while (s.size() > 1 && s.front() == '#') {
    s.remove_prefix(1);
  }

  if (s.size() > 0 && std::isdigit(s.front())) {
    return std::stoi(std::string(s.begin(), s.end()));
  }
  return {};
}

/**
 * \brief Scales used in production efficiency etc.
 * \param x Integer from 0-100
 * \return Float 0.0 - 1.0 (logscaleOB 0.5 - .95)
 */
export constexpr double logscale(const int x) {
  return log10((double)x + 1.0) / 2.0;
}

export constexpr double morale_factor(const double x) {
  return (atan((double)x / 10000.) / 3.14159565 + .5);
}

export template <typename T>
concept Unsigned = std::is_unsigned<T>::value;

export template <typename T>
void setbit(T &target, const Unsigned auto pos)
  requires Unsigned<T>
{
  T bit = 1;
  target |= (bit << pos);
}

export template <typename T>
void clrbit(T &target, const Unsigned auto pos)
  requires Unsigned<T>
{
  T bit = 1;
  target &= ~(bit << pos);
}

export template <typename T>
bool isset(const T target, const Unsigned auto pos)
  requires Unsigned<T>
{
  T bit = 1;
  return target & (bit << pos);
}

export template <typename T>
bool isclr(const T target, const Unsigned auto pos)
  requires Unsigned<T>
{
  return !isset(target, pos);
}

export constexpr int M_FUEL = 0x1;
export constexpr int M_DESTRUCT = 0x2;
export constexpr int M_RESOURCES = 0x4;
export constexpr int M_CRYSTALS = 0x8;

export bool Fuel(int x) { return x & M_FUEL; }
export bool Destruct(int x) { return x & M_DESTRUCT; };
export bool Resources(int x) { return x & M_RESOURCES; };
export bool Crystals(int x) { return x & M_CRYSTALS; };

export std::vector<Victory> create_victory_list();
