// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef MISC_H
#define MISC_H

#include <cmath>

/**
 * \brief Scales used in production efficiency etc.
 * \param x Integer from 0-100
 * \return Float 0.0 - 1.0 (logscaleOB 0.5 - .95)
 */
inline double logscale(const int x) { return log10((double)x + 1.0) / 2.0; }

#endif  // MISC_H
