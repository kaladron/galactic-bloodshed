// SPDX-License-Identifier: Apache-2.0

/// \file commod.cppm
/// \brief Module interface partition for Commod market lot entity.

export module gb.entities:commod;

import :types;
import :tweakables;
import std;

/// \brief Serialization and designated-initializer data structure for a market
/// commodity lot.
export struct commod_struct {
  int id{0};
  player_t owner{0};
  governor_t governor{1};
  CommodType type{CommodType::RESOURCE};
  resource_t amount{0};
  bool deliver{false};
  money_t bid{0};
  std::optional<player_t> bidder{std::nullopt};
  governor_t bidder_gov{1};
  starnum_t star_from{0};
  planetnum_t planet_from{0};
  std::optional<starnum_t> star_to{std::nullopt};
  std::optional<planetnum_t> planet_to{std::nullopt};
};

/// \brief Domain entity representing a commodity lot listed on the market.
export class Commod {
public:
  Commod() = delete;

  Commod(const commod_struct& in) : data_(in) {
    validate(data_);
  }

  [[nodiscard]] int id() const noexcept {
    return data_.id;
  }

  [[nodiscard]] player_t owner() const noexcept {
    return data_.owner;
  }

  [[nodiscard]] governor_t governor() const noexcept {
    return data_.governor;
  }

  [[nodiscard]] CommodType type() const noexcept {
    return data_.type;
  }

  [[nodiscard]] resource_t amount() const noexcept {
    return data_.amount;
  }

  void set_amount(resource_t amount) {
    if (amount < 1) {
      throw std::invalid_argument(
          std::format("Commod amount must be >= 1 (got {})", amount));
    }
    data_.amount = amount;
  }

  [[nodiscard]] bool deliver() const noexcept {
    return data_.deliver;
  }

  void set_deliver(bool deliver) noexcept {
    data_.deliver = deliver;
  }

  void mark_ready_for_delivery() noexcept {
    data_.deliver = true;
  }

  [[nodiscard]] money_t bid() const noexcept {
    return data_.bid;
  }

  void set_bid(money_t bid) {
    if (bid < 0) {
      throw std::invalid_argument(
          std::format("Commod bid must be >= 0 (got {})", bid));
    }
    data_.bid = bid;
  }

  [[nodiscard]] std::optional<player_t> bidder() const noexcept {
    return data_.bidder;
  }

  [[nodiscard]] governor_t bidder_gov() const noexcept {
    return data_.bidder_gov;
  }

  [[nodiscard]] starnum_t star_from() const noexcept {
    return data_.star_from;
  }

  [[nodiscard]] planetnum_t planet_from() const noexcept {
    return data_.planet_from;
  }

  void set_origin(starnum_t star, planetnum_t planet) noexcept {
    data_.star_from = star;
    data_.planet_from = planet;
  }

  [[nodiscard]] std::optional<starnum_t> star_to() const noexcept {
    return data_.star_to;
  }

  [[nodiscard]] std::optional<planetnum_t> planet_to() const noexcept {
    return data_.planet_to;
  }

  /// Returns whether this commodity lot originated from `(star, planet)`.
  [[nodiscard]] constexpr bool is_origin(starnum_t star,
                                         planetnum_t planet) const noexcept {
    return data_.star_from == star && data_.planet_from == planet;
  }

  /// Returns whether this commodity lot has a complete active bid.
  [[nodiscard]] constexpr bool has_bid() const noexcept {
    return data_.bidder.has_value();
  }

  /// Returns the current bid price per unit of commodity.
  [[nodiscard]] constexpr double cost_per_unit() const noexcept {
    return static_cast<double>(data_.bid) / static_cast<double>(data_.amount);
  }

  /// Returns the minimum bid required to outbid the current price.
  [[nodiscard]] money_t min_next_bid() const noexcept {
    return static_cast<money_t>(static_cast<double>(data_.bid) *
                                (1.0 + UP_BID));
  }

  /// Places or raises a bid on this commodity lot, atomically recording the
  /// bidder, governor, bid amount, and destination planet.
  void place_bid(player_t new_bidder, governor_t gov, money_t amount,
                 starnum_t dest_star, planetnum_t dest_planet) noexcept {
    data_.bid = amount;
    data_.bidder = new_bidder;
    data_.bidder_gov = gov;
    data_.star_to = dest_star;
    data_.planet_to = dest_planet;
  }

  /// Clears any active bid and destination on this commodity lot.
  constexpr void clear_bid() noexcept {
    data_.bid = 0;
    data_.bidder = std::nullopt;
    data_.bidder_gov = 1;
    data_.star_to = std::nullopt;
    data_.planet_to = std::nullopt;
  }

  /// Reassigns ownership or active bid jurisdiction from `src_gov` to `tgt_gov`
  /// for `player`.
  void transfer_governor(player_t player, governor_t src_gov,
                         governor_t tgt_gov) noexcept {
    if (data_.owner == player && data_.governor == src_gov) {
      data_.governor = tgt_gov;
    }
    if (data_.bidder == player && data_.bidder_gov == src_gov) {
      data_.bidder_gov = tgt_gov;
    }
  }

  // For repository serialization only.
  [[nodiscard]] const commod_struct& get_struct() const noexcept {
    return data_;
  }

private:
  static void validate(const commod_struct& d) {
    if (d.id < 1) {
      throw std::invalid_argument(
          std::format("Commod id must be >= 1 (got {})", d.id));
    }
  }

  commod_struct data_{};
};
