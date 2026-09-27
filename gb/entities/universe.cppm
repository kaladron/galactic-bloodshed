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

// Underlying universe-level singleton data structure
// This was previously called "stardata" but that name was confusing
// as it contains universe-wide data, not star-specific data
export struct universe_struct {
  std::flat_map<player_t, ap_t> AP{};
  std::flat_map<player_t, VnTargetRecord> vn_targets{};

  [[nodiscard]] ap_t get_AP(player_t p) const {
    if (p < 1) {
      throw std::out_of_range(
          std::format("Player ID {} out of range (must be >= 1)", p.value));
    }
    if (const auto it = AP.find(p); it != AP.end()) {
      return it->second;
    }
    return 0;
  }

  void set_AP(player_t p, ap_t amount) {
    if (p < 1) {
      throw std::out_of_range(
          std::format("Player ID {} out of range (must be >= 1)", p.value));
    }
    if (amount <= 0) {
      AP.erase(p);
    } else {
      AP[p] = amount;
    }
  }

  void add_AP(player_t p, ap_t amount) {
    set_AP(p, get_AP(p) + amount);
  }

  void deduct_AP(player_t p, ap_t amount) {
    const ap_t current = get_AP(p);
    set_AP(p, (current > amount) ? (current - amount) : 0);
  }

  [[nodiscard]] const VnTargetRecord& vn_target(player_t p) const {
    if (p < 1) {
      throw std::out_of_range(
          std::format("Player ID {} out of range (must be >= 1)", p.value));
    }
    static constexpr VnTargetRecord default_target{};
    if (const auto it = vn_targets.find(p); it != vn_targets.end()) {
      return it->second;
    }
    return default_target;
  }

  [[nodiscard]] VnTargetRecord& vn_target(player_t p) {
    if (p < 1) {
      throw std::out_of_range(
          std::format("Player ID {} out of range (must be >= 1)", p.value));
    }
    return vn_targets[p];
  }

  [[nodiscard]] std::uint32_t vn_hits(player_t p) const {
    return vn_target(p).hits;
  }

  void record_vn_kill(player_t killer,
                      std::optional<starnum_t> star = std::nullopt,
                      bool replace_primary = false) {
    auto& record = vn_target(killer);
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
    if (auto it = vn_targets.find(p);
        it != vn_targets.end() && it->second.hits > 0) {
      --it->second.hits;
    }
  }
};
