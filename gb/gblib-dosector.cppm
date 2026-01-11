// SPDX-License-Identifier: Apache-2.0

export module gblib:dosector;

import :planet;
import :sector;
import :services;
import :star;
import :turnstats;

export void produce(EntityManager&, const Star&, const Planet&, Sector&,
                    TurnStats&);
export void spread(EntityManager&, const Planet&, Sector&, SectorMap&,
                   TurnStats&);
export void explore(const Planet&, Sector&, int, int, player_t, TurnStats&);
