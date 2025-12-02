// SPDX-License-Identifier: Apache-2.0

export module gblib:dosector;

import :planet;
import :sector;
import :services;
import :star;

export void produce(EntityManager&, const Star&, const Planet&, Sector&);
export void spread(EntityManager&, const Planet&, Sector&, SectorMap&);
export void explore(const Planet&, Sector&, int, int, int);
