// SPDX-License-Identifier: Apache-2.0

/// \file universe.cppm
/// \brief Module interface partition for Universe entity and game-wide
/// statistics.

export module gb.entities:universe;

import :types;
import std;

/// \brief Per-player Von Neumann retaliation telemetry stored in the universe
/// singleton.
///
/// Tracks the number of VN/Berserker machines destroyed by a player alongside
/// up to two star systems where those machines were destroyed.
export struct VnTargetRecord {
  std::uint32_t hits{0};
  std::optional<starnum_t> primary_star{};
  std::optional<starnum_t> secondary_star{};

  /// \brief Records a star system where a Von Neumann machine was destroyed.
  ///
  /// Populates `primary_star` first, then `secondary_star`. Once both slots
  /// are occupied, `replace_primary` selects which slot is overwritten.
  void record_destruction_star(starnum_t star, bool replace_primary = false) {
    if (star < 1) {
      throw std::out_of_range(
          std::format("Star ID {} out of range (must be >= 1)", star.value));
    }
    if (!primary_star.has_value()) {
      primary_star = star;
    } else if (!secondary_star.has_value()) {
      secondary_star = star;
    } else if (replace_primary) {
      primary_star = star;
    } else {
      secondary_star = star;
    }
  }

  /// \brief Selects a target star system for Berserker retaliation.
  ///
  /// Prefers `primary_star` when `prefer_primary` is true (falling back to
  /// `secondary_star`), or prefers `secondary_star` when `prefer_primary` is
  /// false (falling back to `primary_star`).
  [[nodiscard]] constexpr std::optional<starnum_t>
  select_retaliation_star(bool prefer_primary) const noexcept {
    if (prefer_primary) {
      return primary_star.has_value() ? primary_star : secondary_star;
    }
    return secondary_star.has_value() ? secondary_star : primary_star;
  }

  [[nodiscard]] constexpr bool
  operator==(const VnTargetRecord&) const noexcept = default;
};

// Internal struct holding raw universe data for serialization.
export struct universe_struct {
  std::flat_map<player_t, ap_t> AP{};
  std::flat_map<player_t, VnTargetRecord> vn_targets{};
};

/// \brief Domain entity representing global universe-wide state (action points
/// and Von Neumann retaliation telemetry).
export class Universe {
public:
  Universe() = default;
  explicit Universe(const universe_struct& in) : data_(in) {}

  [[nodiscard]] ap_t get_AP(player_t p) const {
    if (p < 1) {
      throw std::out_of_range(
          std::format("Player ID {} out of range (must be >= 1)", p.value));
    }
    if (const auto it = data_.AP.find(p); it != data_.AP.end()) {
      return it->second;
    }
    return 0;
  }

  [[nodiscard]] ap_t AP(player_t p) const {
    return get_AP(p);
  }

  [[nodiscard]] bool has_AP(player_t p) const {
    if (p < 1) {
      throw std::out_of_range(
          std::format("Player ID {} out of range (must be >= 1)", p.value));
    }
    return data_.AP.contains(p);
  }

  [[nodiscard]] bool has_any_AP() const noexcept {
    return !data_.AP.empty();
  }

  void set_AP(player_t p, ap_t amount) {
    if (p < 1) {
      throw std::out_of_range(
          std::format("Player ID {} out of range (must be >= 1)", p.value));
    }
    if (amount <= 0) {
      data_.AP.erase(p);
    } else {
      data_.AP[p] = amount;
    }
  }

  void add_AP(player_t p, ap_t amount) {
    set_AP(p, get_AP(p) + amount);
  }

  void deduct_AP(player_t p, ap_t amount) {
    const ap_t current = get_AP(p);
    set_AP(p, (current > amount) ? (current - amount) : 0);
  }

  [[nodiscard]] bool has_vn_target(player_t p) const {
    if (p < 1) {
      throw std::out_of_range(
          std::format("Player ID {} out of range (must be >= 1)", p.value));
    }
    return data_.vn_targets.contains(p);
  }

  [[nodiscard]] bool has_any_vn_targets() const noexcept {
    return !data_.vn_targets.empty();
  }

  [[nodiscard]] const VnTargetRecord& vn_target(player_t p) const {
    if (p < 1) {
      throw std::out_of_range(
          std::format("Player ID {} out of range (must be >= 1)", p.value));
    }
    static constexpr VnTargetRecord default_target{};
    if (const auto it = data_.vn_targets.find(p);
        it != data_.vn_targets.end()) {
      return it->second;
    }
    return default_target;
  }

  void set_vn_target(player_t p, const VnTargetRecord& record) {
    if (p < 1) {
      throw std::out_of_range(
          std::format("Player ID {} out of range (must be >= 1)", p.value));
    }
    if (record == VnTargetRecord{}) {
      data_.vn_targets.erase(p);
    } else {
      data_.vn_targets[p] = record;
    }
  }

  [[nodiscard]] std::uint32_t vn_hits(player_t p) const {
    return vn_target(p).hits;
  }

  void set_vn_hits(player_t p, std::uint32_t hits) {
    if (p < 1) {
      throw std::out_of_range(
          std::format("Player ID {} out of range (must be >= 1)", p.value));
    }
    data_.vn_targets[p].hits = hits;
  }

  [[nodiscard]] std::optional<starnum_t> vn_primary_star(player_t p) const {
    return vn_target(p).primary_star;
  }

  [[nodiscard]] std::optional<starnum_t> vn_secondary_star(player_t p) const {
    return vn_target(p).secondary_star;
  }

  [[nodiscard]] std::optional<starnum_t>
  select_vn_retaliation_star(player_t p, bool prefer_primary) const {
    return vn_target(p).select_retaliation_star(prefer_primary);
  }

  void record_vn_kill(player_t killer,
                      std::optional<starnum_t> star = std::nullopt,
                      bool replace_primary = false) {
    if (killer < 1) {
      throw std::out_of_range(std::format(
          "Player ID {} out of range (must be >= 1)", killer.value));
    }
    auto& record = data_.vn_targets[killer];
    ++record.hits;
    if (star.has_value()) {
      record.record_destruction_star(*star, replace_primary);
    }
  }

  void decrement_vn_hits(player_t p) {
    if (p < 1) {
      throw std::out_of_range(
          std::format("Player ID {} out of range (must be >= 1)", p.value));
    }
    if (auto it = data_.vn_targets.find(p);
        it != data_.vn_targets.end() && it->second.hits > 0) {
      --it->second.hits;
    }
  }

  // For repository serialization only.
  [[nodiscard]] const universe_struct& get_struct() const noexcept {
    return data_;
  }

private:
  universe_struct data_{};
};
