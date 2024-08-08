// SPDX-License-Identifier: Apache-2.0

module;

#include "gb/sql/dbdecl.h"

export module gblib:globals;

import :misc;
import :planet;
import :race;
import :star;
import :types;

#include "gb/tweakables.h"

export std::vector<Race> races;

export struct stardata Sdata;

export unsigned char Nuked[MAXPLAYERS];
export unsigned long StarsInhab[NUMSTARS];
export unsigned long StarsExpl[NUMSTARS];
export std::vector<Star> stars;
export unsigned short Sdatanumships[MAXPLAYERS];
export unsigned long Sdatapopns[MAXPLAYERS];
export unsigned short starnumships[NUMSTARS][MAXPLAYERS];
export unsigned long starpopns[NUMSTARS][MAXPLAYERS];

export unsigned long tot_resdep, prod_eff, prod_res[MAXPLAYERS];
export unsigned long prod_fuel[MAXPLAYERS], prod_destruct[MAXPLAYERS];
export unsigned long prod_crystals[MAXPLAYERS], prod_money[MAXPLAYERS];
export unsigned long tot_captured, prod_mob;
export unsigned long avg_mob[MAXPLAYERS];
export unsigned char sects_gained[MAXPLAYERS], sects_lost[MAXPLAYERS];
export unsigned char Claims;

export std::array<std::array<std::unique_ptr<Planet>, MAXPLANETS>, NUMSTARS>
    planets;
export unsigned char ground_assaults[MAXPLAYERS][MAXPLAYERS][NUMSTARS];
export uint64_t inhabited[NUMSTARS];
export double Compat[MAXPLAYERS];
export player_t Num_races;
export unsigned long Num_commods;
export planetnum_t Planet_count;
export unsigned long newslength[4];

/* number of movement segments (global variable) */
export unsigned long segments;

export power Power[MAXPLAYERS];
export block Blocks[MAXPLAYERS];
export power_blocks Power_blocks;

export bool update_flag = false;
export std::list<DescriptorData> descriptor_list;

export sqlite3 *dbconn;

export char plan_buf[1024];
