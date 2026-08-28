// SPDX-License-Identifier: Apache-2.0

/// \file gblib-dosector.cppm
/// \brief Module interface partition for surface sector turn processing.

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
