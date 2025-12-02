// SPDX-License-Identifier: Apache-2.0

export module gblib:doplanet;

import :planet;
import :services;
import :star;
import :turnstats;

export int doplanet(EntityManager&, const Star& star, Planet& planet,
                    TurnStats& stats);
