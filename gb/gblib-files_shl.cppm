// SPDX-License-Identifier: Apache-2.0

module;

#include <sqlite3.h>
#include <sys/types.h>

#include "gb/sql/dbdecl.h"

export module gblib:files_shl;

import :ships;
import :star;

export void close_file(int);
export void openstardata(int *);
export void openshdata(int *);
export void opencommoddata(int *);
export void opensectdata(int *);
export void openracedata(int *);
export void getsdata(stardata *S);
export Race getrace(player_t);
export Star getstar(starnum_t);
export Planet getplanet(starnum_t, planetnum_t);
export std::optional<Ship> getship(std::string_view shipstring);
export std::optional<Ship> getship(shipnum_t);
export std::optional<Ship> getship(Ship **, shipnum_t);
export Commod getcommod(commodnum_t);
export Sector getsector(const Planet &, int x, int y);
export SectorMap getsmap(const Planet &);
export int getdeadship();
export int getdeadcommod();
export void initsqldata();
export void putsdata(stardata *);
export void putrace(const Race &);
export void putstar(const Star &, starnum_t);
export void putplanet(const Planet &, const Star &, planetnum_t);
export void putsector(const Sector &, const Planet &);
export void putsector(const Sector &, const Planet &, int x, int y);
export void putsmap(const SectorMap &map, const Planet &p);
export void putship(const Ship &);
export void putcommod(const Commod &, int);
export shipnum_t Numships();
export off_t getnewslength(NewsType);
export void clr_shipfree();
export void clr_commodfree();
export void makeshipdead(int);
export void makecommoddead(int);
export void putpower(power[MAXPLAYERS]);
export void getpower(power[MAXPLAYERS]);
export void Putblock(block[MAXPLAYERS]);
export void Getblock(block[MAXPLAYERS]);
export void open_files();
export void close_files();

// JSON serialization functions for Race (for SQLite migration)
export std::optional<std::string> race_to_json(const Race&);
export std::optional<Race> race_from_json(const std::string&);
