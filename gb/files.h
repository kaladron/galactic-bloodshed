// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef FILES_H
#define FILES_H

#define PATH(file) PKGDATADIR #file
#define DIRPATH(dir, file) PKGSTATEDIR dir #file
#define DATA(file) PKGSTATEDIR #file
#define NEWS(file) DIRPATH("News/", file)
#define TELE(file) DIRPATH("Tele/", file)

#define PATHLEN 200 /* length of file paths to the game.. */

#define CUTE_MESSAGE "\nThe Galactic News\n\n"

inline constexpr auto DATADIR = PKGSTATEDIR;
inline constexpr auto DOCSDIR = DOCDIR;
inline constexpr auto EXAM_FL = PATH(exam.dat);
inline constexpr auto ENROLL_FL = DATA(enroll.dat);
inline constexpr auto STARDATAFL = DATA(star);
inline constexpr auto PLANETDATAFL = DATA(planet);
inline constexpr auto RACEDATAFL = DATA(race);
inline constexpr auto BLOCKDATAFL = DATA(block);
inline constexpr auto SHIPDATAFL = DATA(ship);
inline constexpr auto SHIPFREEDATAFL = DATA(shipfree);
inline constexpr auto DUMMYFL = DATA(dummy);
inline constexpr auto PLAYERDATAFL = DATA(players);
inline constexpr auto TELEGRAMDIR = DATA(Tele);
inline constexpr auto TELEGRAMFL = DATA(tele);
inline constexpr auto NEWSDIR = DATA(News);
inline constexpr auto DECLARATIONFL = NEWS(declaration);
inline constexpr auto TRANSFERFL = NEWS(transfer);
inline constexpr auto COMBATFL = NEWS(combat);
inline constexpr auto ANNOUNCEFL = NEWS(announce);
inline constexpr auto COMMODDATAFL = DATA(commod);
inline constexpr auto COMMODFREEDATAFL = DATA(commodfree);
inline constexpr auto UPDATEFL = DATA(Update.time);
inline constexpr auto SEGMENTFL = DATA(Segment.time);

#define PLANETLIST PATH(planet.list)
#define STARLIST PATH(star.list)

#define NOGOFL DATA(nogo)
#define ADDRESSFL DATA(Addresses)

extern const char *Files[];

#endif  // FILES_H
