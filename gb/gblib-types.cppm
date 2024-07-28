// SPDX-License-Identifier: Apache-2.0

export module gblib:types;

import std.compat;

export using shipnum_t = uint64_t;
export using starnum_t = uint32_t;
export using planetnum_t = uint32_t;
export using player_t = uint32_t;
export using governor_t = uint32_t;

export using ap_t = uint32_t;
export using commodnum_t = int64_t;
export using resource_t = int64_t;
export using money_t = int64_t;
export using population_t = int64_t;

export using command_t = std::vector<std::string>;