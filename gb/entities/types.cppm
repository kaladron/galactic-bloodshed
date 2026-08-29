// SPDX-License-Identifier: Apache-2.0

/// \file types.cppm
/// \brief Standalone foundational strong ID types module.

export module types;

export import strong_id;
import std;

// Core ID types - these are foundational types used across all layers
export using commodnum_t = ID<"commod", std::int64_t>;
export using shipnum_t = ID<"ship", std::uint64_t>;
export using starnum_t = ID<"star", std::uint32_t>;
export using planetnum_t = ID<"planet", std::uint32_t>;
export using player_t = ID<"player">;
export using governor_t = ID<"governor">;
export using blocknum_t = ID<"block", int>;
export using powernum_t = ID<"power", int>;

export using segments_t = std::uint32_t;
export using ap_t = std::uint32_t;
export using resource_t = std::int64_t;
export using money_t = std::int64_t;
export using population_t = std::int64_t;

// Semantic domain metric types
export using armor_t =
    std::uint32_t;  ///< Armor defense rating absorbing combat damage
export using damage_t = std::uint32_t;     ///< Hull damage percentage (0..100)
export using speed_t = std::uint32_t;      ///< Tactical engine speed throttle
export using radiation_t = std::uint32_t;  ///< Accumulated radiation dose level
export using gun_count_t =
    std::uint32_t;  ///< Number of gun mounts or battery power rating
export using bearing_t =
    std::uint32_t;  ///< Course navigation heading angle in degrees (0..359)
export using hangar_t = std::uint32_t;  ///< Internal carried ship hangar space
export using ship_size_t = std::uint32_t;  ///< Ship physical size / volume
export using weapon_power_t =
    std::uint32_t;  ///< Concentrated energy weapon / laser power setting
