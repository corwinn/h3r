/**** BEGIN LICENSE BLOCK ****

BSD 3-Clause License

Copyright (c) 2021-2023, the wind.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**** END LICENCE BLOCK ****/

#include "h3r_map.h"
#include "h3r_ffd.h"
#include "h3r_log.h"
#include "h3r_string.h"

H3R_NAMESPACE

FFD * Map::_ffd_h {};
FFD * Map::_ffd {};
int Map::_map_cnt {0};

namespace {
static void ReadLocation(FFDNode * node, Map::Location & l)
{
    if (node) {
        l.X = node->Get<byte> ("X");
        l.Y = node->Get<byte> ("Y");
        l.Z = node->Get<byte> ("Z");
    }
}

static String ReadMapString(FFDNode * node)
{
    if (nullptr == node) return "<>null";
    return static_cast<String &&>(node->Get<String> ("Chars"));
}

static bool ValidVersion(h3rMapVersion v)
{
    const int V[4] = {H3R_VERSION_SOD, H3R_VERSION_AB, H3R_VERSION_ROE,
        H3R_VERSION_WOG};
    for (int i = 0; i < 4; i++)
        if (V[i] == v) return true;
    return false;
}
}

Map::~Map()
{
    if (_map)
        H3R_DESTROY_OBJECT(_map, FFDNode)
    if (! --_map_cnt) {
        H3R_DESTROY_OBJECT(_ffd_h, FFD)
        H3R_DESTROY_OBJECT(_ffd, FFD)
    }
}

Map::Map(const String & h3m, bool header_only)
{
    H3R_ENSURE (_map_cnt >= 0 && _map_cnt < H3R_MAX_OPEN_MAP_COUNT, "No no")
    H3R_ENSURE (_map_cnt < 0x4ffffffe, "No no no no")
    if (! _map_cnt) {
        //LATER decide where remake resources will be
        H3R_CREATE_OBJECT(_ffd_h, FFD) {"ffd/h3m_newgame_ffd"};
        H3R_CREATE_OBJECT(_ffd, FFD) {"ffd/h3m_ffd"};
    }
    _map_cnt++;

    auto ffd = header_only ? _ffd_h : _ffd;
    _map = ffd->File2Tree (h3m);
    H3R_ENSURE(nullptr != _map, "Map load failed")

    auto version_node = _map->Get<decltype(_map)> ("Version");
    H3R_ENSURE(nullptr != version_node, "\"Version\" shall exist")
    _version = version_node->AsInt ();
    H3R_ENSURE(ValidVersion (_version), "Unknown Version")
    if (version_node->IsEnum ())
        _version_name = version_node->GetEnumName ();

    _has_players = _map->Get<bool> ("HasPlayers");
    _nxy = _map->Get<int> ("Size");
    _nz = _map->Get<bool> ("TwoLevels") ? 2 : 1;
    _name = ReadMapString (_map->Get<decltype(_map)> ("Name"));
    _description = ReadMapString (_map->Get<decltype(_map)> ("Description"));

    auto diff_node = _map->Get<decltype(_map)> ("Difficulty");
    H3R_ENSURE(nullptr != diff_node, "\"Difficulty\" shall exist")
    _difficulty = diff_node->AsByte ();
    if (diff_node->IsEnum ())
        _diff_name = diff_node->GetEnumName ();

    // gets "byte {}" on RoE
    _level_cap = _map->Get<byte> ("LevelLimit");

    auto players = _map->Get<decltype(_map)> ("Players");
    for (int i = 0; i < players->NodeCount (); i++) {
        auto player = players->operator[] (i);
        auto & p = _players.Add (Player {});
        // Smells like code-gen
        p.Human = player->Get<bool> ("Human");
        p.Computer = player->Get<bool> ("Computer");
        if (! p.Human && ! p.Computer) continue;
        _players_can_play++;
        p.Behavior = player->Get<byte> ("AITactics");

        // its conditional, but AsShort() handles byte as well
        p.Factions = player->Get<short> ("AllowedFactions");

        // while conditional, it returns "bool {}" when the node isn't there
        p.GenAtMT = player->Get<decltype(_map)> ("GenAtMT");
        ReadLocation (player->Get<decltype(_map)> ("MainTown"), p.MT);

        auto primary_hero = player->Get<decltype(_map)> ("MainHero");
        H3R_ENSURE(nullptr != primary_hero, "\"MainHero\" shall exist")
        p.PHRnd = primary_hero->Get<bool> ("Random");
        p.PHIdentity = primary_hero->Get<byte> ("Identity");
        // 0xff seems to label "no such thing"
        p.PHPortrait = primary_hero->Get<byte> ("Portrait", 255);
        p.PHName = ReadMapString (primary_hero->Get<decltype(_map)> ("Name"));

        auto c_heroes = player->Get<decltype(_map)> ("CustomizedHeroes");
        if (c_heroes)
            for (int j = 0; j < c_heroes->NodeCount (); j++) {
                auto c_hero = c_heroes->operator[] (j);
                auto & ah  = p.CustomizedHeroes.Add (Player::CustomizedHero {});
                ah.Id = c_hero->Get<byte> ("Id");
                ah.Name = ReadMapString (c_hero->Get<decltype(_map)> ("Name"));
            }
    }// (int i = 0; i < players->NodeCount (); i++)

    auto vcon = _map->Get<decltype(_map)> ("SpecialWCon");
    H3R_ENSURE(nullptr != vcon, "SpecialWCon shall exist")
    _vcon = (vcon->Get<byte> ("VCon") ^ H3R_DEFAULT_BYTE) + 1; // "vcdesc.txt"
    _vcon_default_too = vcon->Get<bool> ("AllowNormalAsWell");
    _vcon_ai = vcon->Get<bool> ("AppliesToAI");
    //TODO if s.o. decides to name them differently:
    //       VConditionCreObj.CreType, VConditionResObj.ResType
    //     instead of:
    //       VConditionCreObj.Type   , VConditionResObj.Type
    //     this code shall stop working, so something has to be done about it
    _vcon_type = vcon->Get<byte> ("Type"); // the type of object
    _vcon_hlevel = vcon->Get<byte> ("HallLevel");
    _vcon_clevel = vcon->Get<byte> ("CastleLevel");
    ReadLocation (vcon->Get<decltype(_map)> ("Pos"), _vcon_loc);
    _vcon_quantity = vcon->Get<int> ("Quantity");

    auto lcon = _map->Get<decltype(_map)> ("SpecialLCon");
    H3R_ENSURE(nullptr != lcon, "SpecialLCon shall exist")
    _lcon = (lcon->Get<byte> ("LCon") ^ H3R_DEFAULT_BYTE) + 1; // "lcdesc.txt"
    ReadLocation (lcon->Get<decltype(_map)> ("Pos"), _lcon_loc);
    _lcon_quantity = lcon->Get<short> ("Quantity");

    auto teams = _map->Get<decltype(_map)> ("Team");
    if (nullptr != teams)
        for (byte b : *(teams->AsByteArray ()))
            _teams.Add (b);
}// Map::Map()

NAMESPACE_H3R
