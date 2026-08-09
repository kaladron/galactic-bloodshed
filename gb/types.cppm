// SPDX-License-Identifier: Apache-2.0

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

export using segments_t = std::uint32_t;
export using ap_t = std::uint32_t;
export using resource_t = std::int64_t;
export using money_t = std::int64_t;
export using population_t = std::int64_t;
