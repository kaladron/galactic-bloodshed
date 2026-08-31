// SPDX-License-Identifier: Apache-2.0

/// \file gblib-dosector.cppm
/// \brief Module interface partition for surface sector turn processing.

export module gblib:dosector;

import :planet;
import :sector;
import :services;
import :star;
import :turnstats;

export population_t calculate_migrating_colonists(const Race&, double,
                                                  const Sector&, population_t);
export population_t attempt_colonist_migration(EntityManager&, const Planet&,
                                               Sector&, Coordinates,
                                               population_t, SectorMap&,
                                               TurnStats&);

export void produce(EntityManager&, const Star&, const Planet&, Sector&,
                    TurnStats&);
export void spread(EntityManager&, const Planet&, Sector&, SectorMap&,
                   TurnStats&);
