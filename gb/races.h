// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef RACES_H
#define RACES_H

#include "gb/tweakables.h"

/* special discoveries */
#define D_HYPER_DRIVE 0  /* hyper-space capable */
#define D_LASER 1        /* can construct/operate combat lasers */
#define D_CEW 2          /* can construct/operate cews */
#define D_VN 3           /* can construct von-neumann machines */
#define D_TRACTOR_BEAM 4 /* tractor/repulsor beam */
#define D_TRANSPORTER 5  /* tractor beam (local) */
#define D_AVPM 6         /* AVPM transporter */
#define D_CLOAK 7        /* cloaking device */
#define D_WORMHOLE 8     /* worm-hole */
#define D_CRYSTAL 9      /* crystal power */

#define Hyper_drive(r) ((r).discoveries[D_HYPER_DRIVE])
#define Laser(r) ((r).discoveries[D_LASER])
#define Cew(r) ((r).discoveries[D_CEW])
#define Vn(r) ((r).discoveries[D_VN])
#define Tractor_beam(r) ((r).discoveries[D_TRACTOR_BEAM])
#define Transporter(r) ((r).discoveries[D_TRANSPORTER])
#define Avpm(r) ((r).discoveries[D_AVPM])
#define Cloak(r) ((r).discoveries[D_CLOAK])
#define Wormhole(r) ((r).discoveries[D_WORMHOLE])
#define Crystal(r) ((r).discoveries[D_CRYSTAL])

#define TECH_HYPER_DRIVE 50.0
#define TECH_LASER 100.0
#define TECH_CEW 150.0
#define TECH_VN 100.0
#define TECH_TRACTOR_BEAM 999.0
#define TECH_TRANSPORTER 999.0
#define TECH_AVPM 250.0
#define TECH_CLOAK 999.0
#define TECH_WORMHOLE 999.0
#define TECH_CRYSTAL 50.0

#endif  // RACES_H
