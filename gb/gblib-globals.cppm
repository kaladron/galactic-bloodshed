// SPDX-License-Identifier: Apache-2.0

module;

import std;

export module gblib:globals;

import :misc;
import :planet;
import :race;
import :star;
import :types;
import :universe;

export std::array<char, MAXPLAYERS> Nuked;
export unsigned long StarsInhab[NUMSTARS];
export unsigned long StarsExpl[NUMSTARS];
export unsigned short Sdatanumships[MAXPLAYERS];
export unsigned long Sdatapopns[MAXPLAYERS];
export unsigned short starnumships[NUMSTARS][MAXPLAYERS];
export unsigned long starpopns[NUMSTARS][MAXPLAYERS];

export unsigned long tot_resdep, prod_eff, prod_res[MAXPLAYERS];
export unsigned long prod_fuel[MAXPLAYERS], prod_destruct[MAXPLAYERS];
export unsigned long prod_crystals[MAXPLAYERS], prod_money[MAXPLAYERS];
export unsigned long tot_captured, prod_mob;
export unsigned long avg_mob[MAXPLAYERS];
export unsigned char Claims;

export unsigned char ground_assaults[MAXPLAYERS][MAXPLAYERS][NUMSTARS];
export uint64_t inhabited[NUMSTARS];
export double Compat[MAXPLAYERS];
export player_t Num_races;
export unsigned long Num_commods;
export planetnum_t Planet_count;

export unsigned long segments;  // number of movement segments (global variable)
export time_t next_update_time;   // When will next update be... approximately
export time_t next_segment_time;  // When will next segment be... approximately
export std::chrono::minutes update_time;  // Interval between updates
export segments_t nsegments_done;  // How many movements have we done so far?

export power Power[MAXPLAYERS];
export block Blocks[MAXPLAYERS];
export power_blocks Power_blocks;

export bool update_flag = false;
export std::list<DescriptorData> descriptor_list;

export struct stinfo Stinfo[NUMSTARS][MAXPLANETS];
export struct vnbrain VN_brain;
export struct sectinfo Sectinfo[MAX_X][MAX_Y];

export constexpr std::array<const char, 8> Psymbol = {'@', 'o', 'O', '#',
                                                      '~', '.', ')', '-'};

export constexpr std::array<const char*, 8> Planet_types = {
    "Class M", "Asteroid",  "Airless", "Iceball",
    "Jovian",  "Waterball", "Forest",  "Desert"};

export constexpr std::array<const char*, 9> Desnames = {
    "ocean",  "land",   "mountainous", "gaseous", "ice",
    "forest", "desert", "plated",      "wasted"};

export constexpr std::array<const char, 9> Dessymbols = {
    CHAR_SEA,    CHAR_LAND,   CHAR_MOUNT,  CHAR_GAS,   CHAR_ICE,
    CHAR_FOREST, CHAR_DESERT, CHAR_PLATED, CHAR_WASTED};

// These map to SectorType and give the natural defenses for each type of
// sector.
export constexpr std::array<int, 9> Defensedata = {1, 1, 3, 2, 2, 3, 2, 4, 0};
