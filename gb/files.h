// SPDX-License-Identifier: Apache-2.0

/// \file files.h
/// \brief Path definitions for game data and state files.

#ifndef FILES_H
#define FILES_H

#define PATH(file) PKGDATADIR #file
#define DIRPATH(dir, file) PKGSTATEDIR dir #file
#define DATA(file) PKGSTATEDIR #file

inline constexpr auto EXAM_FL = PATH(exam.dat);


#define PLANETLIST PATH(planet.list)
#define STARLIST PATH(star.list)

inline constexpr const char* nogofl = PKGSTATEDIR "nogo";

#endif  // FILES_H
