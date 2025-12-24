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

inline constexpr auto EXAM_FL = PATH(exam.dat);
inline constexpr auto STARDATAFL = DATA(star);
inline constexpr auto PLANETDATAFL = DATA(planet);
inline constexpr auto RACEDATAFL = DATA(race);
inline constexpr auto BLOCKDATAFL = DATA(block);
inline constexpr auto SHIPDATAFL = DATA(ship);
inline constexpr auto SHIPFREEDATAFL = DATA(shipfree);
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

#define PLANETLIST PATH(planet.list)
#define STARLIST PATH(star.list)

inline constexpr std::string_view nogofl = PKGSTATEDIR "nogo";

#endif  // FILES_H
