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

#endif  // MAKESTAR_H
