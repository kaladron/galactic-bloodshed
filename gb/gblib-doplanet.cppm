// SPDX-License-Identifier: Apache-2.0

export module gblib:doplanet;

import :planet;
import :services;
import :star;
import :turnstats;
import :types;

export int doplanet(EntityManager&, const Star& star, Planet& planet,
                    TurnStats& stats);

export void moveplanet(EntityManager& entity_manager, const Star& star,
                       Planet& planet);
