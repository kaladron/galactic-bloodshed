// SPDX-License-Identifier: Apache-2.0

/// \file makeplanet.h
/// \brief Header for planet generation functions.

#ifndef MAKEPLANET_H
#define MAKEPLANET_H

Planet makeplanet(double dist, short stemp, PlanetType type, starnum_t star_id,
                  planetnum_t planet_order, std::optional<SectorMap>& out_smap);

#endif  // MAKEPLANET_H
