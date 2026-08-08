// SPDX-License-Identifier: Apache-2.0

export module gblib:doplanet;

import :planet;
import :services;
import :star;
import :turnstats;
import :types;

import :ships;

export int doplanet(EntityManager&, const Star& star, Planet& planet,
                    TurnStats& stats);

export void moveplanet(EntityManager& entity_manager, const Star& star,
                       Planet& planet);

export bool moveship_onplanet(Ship& ship, const Planet& planet,
                              EntityManager& entity_manager);
export void terraform(Ship& ship, Planet& planet, SectorMap& smap,
                      EntityManager& entity_manager);
export void plow(Ship* ship, Planet& planet, SectorMap& smap,
                 EntityManager& entity_manager);
export void do_dome(EntityManager& entity_manager, Ship* ship, SectorMap& smap);
export void do_quarry(Ship* ship, Planet& planet, SectorMap& smap,
                      EntityManager& entity_manager, TurnStats& stats);
export void do_recover(EntityManager& entity_manager, const Star& star,
                       Planet& planet);
