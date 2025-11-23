// SPDX-License-Identifier: Apache-2.0

export module gblib:dosector;

import :planet;
import :sector;
import :star;

export void produce(const Star&, const Planet&, Sector&);
export void spread(const Planet&, Sector&, SectorMap&);
export void explore(const Planet&, Sector&, int, int, int);
