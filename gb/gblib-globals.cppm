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

// Ground assault tracking - modified by commands, reported during turn
// Cannot move to TurnStats because commands need access
export unsigned char ground_assaults[MAXPLAYERS][MAXPLAYERS][NUMSTARS];

// Power blocks - computed during turn processing, read by commands (e.g., block
// command)
export power_blocks Power_blocks;

export bool update_flag = false;
export std::list<DescriptorData> descriptor_list;
