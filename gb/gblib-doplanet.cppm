// SPDX-License-Identifier: Apache-2.0

export module gblib:doplanet;

import :planet;
import :services;

export int doplanet(EntityManager&, starnum_t starnum, Planet& planet,
                    planetnum_t planetnum);
