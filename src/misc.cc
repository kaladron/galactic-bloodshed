// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* scales used in production efficiency etc.
 * input both: int 0-100
 * output both: float 0.0 - 1.0 (logscaleOB 0.5 - .95)
 */

#include "misc.h"

#include <cmath>

double logscale(int x) { return log10((double)x + 1.0) / 2.0; }
