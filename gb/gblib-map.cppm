// SPDX-License-Identifier: Apache-2.0

export module gblib:map;

import :planet;
import :race;
import :sector;
import :types;

export char desshow(const player_t, const governor_t, const Race &,
                    const Sector &);
export void show_map(GameObj &g, const starnum_t, const planetnum_t,
                     const Planet &);
