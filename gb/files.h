// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef FILES_H
#define FILES_H

#define PATH(file) PKGDATADIR #file
#define DIRPATH(dir, file) PKGSTATEDIR dir #file
#define DATA(file) PKGSTATEDIR #file
#define TELE(file) DIRPATH("Tele/", file)

inline constexpr auto EXAM_FL = PATH(exam.dat);
inline constexpr auto TELEGRAMDIR = DATA(Tele);
inline constexpr auto TELEGRAMFL = DATA(tele);

#define PLANETLIST PATH(planet.list)
#define STARLIST PATH(star.list)

inline constexpr std::string_view nogofl = PKGSTATEDIR "nogo";

#endif  // FILES_H
