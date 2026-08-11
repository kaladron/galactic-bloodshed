// SPDX-License-Identifier: Apache-2.0

/// \file makestar.h
/// \brief Header for star system generation functions.

#ifndef MAKESTAR_H
#define MAKESTAR_H

int Temperature(double dist, int stemp);
void Makestar_init();
Star Makestar(Database& db, starnum_t);
void Makeplanet_init();
void PrintStatistics();
void set_planet_list_permutation(const std::vector<int>& indices);
void set_star_list_permutation(const std::vector<int>& indices);

#endif  // MAKESTAR_H
