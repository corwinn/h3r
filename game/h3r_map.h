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

#ifndef _H3R_MAP_H_
#define _H3R_MAP_H_

#include "h3r.h"
#include "h3r_ffdnode.h"
#include "h3r_list.h"

H3R_NAMESPACE

using h3rMapVersion = int;
using h3rObjRef = int;
using h3rObjType = byte;
using h3rCoord = byte;

// "not set", "use the default", etc.
byte const H3R_DEFAULT_BYTE {255};

#define H3R_VERSION_ROE 0x0e
#define H3R_VERSION_AB  0x15
#define H3R_VERSION_SOD 0x1c
#define H3R_VERSION_WOG 0x33

// There something odd going on:
// https://github.com/orgs/community/discussions/51047#discussioncomment-5437811
// no more commits today.

//TODO "Memento" (1 - save/load; 2 - network); nothing new here: MapStream
class Map final
{
    H3R_CANT_COPY(Map)
    H3R_CANT_MOVE(Map)

    // Not the best solution, but should do for the time being.
    private static FFD * _ffd_h;
    private static FFD * _ffd;
    private static int _map_cnt;

    private FFDNode * _map {};

    // cached - no need for live update
    private h3rMapVersion _version {};
    private String _version_name {};
    private bool _has_players {}; //TODO an "enigma"
    private int _nxy {}; // 36, 72, 108, 144
    private int _nz {}; // 1 or 2
    private String _name {};      // h3m.Name
    private String _file_name {}; // disambiguate
    private String _description {};
    private byte _difficulty {};
    private String _diff_name {};
    private byte _level_cap {};
    public struct Location final { int X{-1}, Y{-1}, Z{-1}; };
    public struct Player final
    {
        bool Human {};    // Allowed to be
        bool Computer {}; //
        byte Behavior {};  // random, warrior, builder, explorer
        short Factions {}; // Allowed
        bool GenAtMT {};  // Generate at Main Town
        Location MT {};
        bool   PHRnd {};      // primary hero - is random
        byte   PHIdentity {}; // the pre-defined hero - ancestor
        // Odd game behavior: if this happens to be 255 (use the default) - it
        // fails to fetch the Identity default portrait (at the new game
        // dialog), and states the hero is random, although PHRnd=0; I won't be
        // replicating that.
        byte   PHPortrait {}; // 255: not customized?
        String PHName {};     // What if its not customized - a copy?
        struct CustomizedHero final { byte Id; String Name; };
        List <CustomizedHero> CustomizedHeroes {};
        inline bool CanPlay() const { return Human || Computer; }
    };
    private List<Player> _players {};
    private int _players_can_play {};
    private int _players_human {};
    //
    private byte _vcon {}; // "vcdesc.txt" - line 0 has the "default"
    private bool _vcon_ai {};
    private bool _vcon_default_too {};
    private byte _vcon_type {};
    private byte _vcon_hlevel {};
    private byte _vcon_clevel {};
    private Location _vcon_loc {};
    private int _vcon_quantity {};
    private byte _lcon {}; // "lcdesc.txt" - line 0 has the "default"
    private short _lcon_quantity {};
    private Location _lcon_loc {};
    // a set of hash keys - the hash table: "PlColors.txt"
    private List<byte> _teams {};

    public Map(const String &, bool = true);
    public ~Map();
    // defines if this map is supported by this project
    public bool SupportedVersion();

    public inline h3rMapVersion Version() const { return _version; }
    public inline const String & VersionName() const { return _version_name; }
    public inline int Size() const { return _nxy; }
    public inline const String SizeName() const
    {
        switch (_nxy) {
            case  36: return "S";
            case  72: return "M";
            case 108: return "L";
            case 144: return "XL";
            default: return "42";//TODO
        };
    }
    public inline int Levels() const { return _nz; }
    public inline const String & Name() const { return _name; }
    public inline const String & Descr() const { return _description; }
    public inline int Difficulty() const { return _difficulty; }
    public inline const String & DifficultyName() const { return _diff_name; }
    public inline int PlayerNum() const { return _players_can_play; }
    public inline int HumanPlayers() const { return _players_human; }
    public inline const Player & PlayerAt(int i) const { return _players[i]; }

    // 255 means "default" - which happens to be line 0 at game resources.
    // In order to avoid toying with "what would be a signed 8-bit", 255 gets
    // translated to 0; use "vcdesc.txt" and "lcdesc.txt" (0-based) to get the
    // text.
    // E.g. these do not match the enum at the FFD, but the lines at the .txt.
    //
    // One more thing: there are two additional vcon at "vcdesc.txt" I haven't
    // seen used:
    //   "Defeat all monsters"
    //   "Survive beyond a time limit"
    //PERHAPS binary-edit a map to see if the game handles them.
    public inline int VCon() const { return _vcon; }
    public const String & VConText() const;
    public inline bool VConDefaultToo() const { return _vcon_default_too; }
    public inline int LCon() const { return _lcon; }
    public const String & LConText() const;
    public inline const List<byte> & Teams() const { return _teams; }
    public inline int FirstHumanPlayer() const
    {
        for (int i = 0; i < _players.Count (); i++)
            if (_players[i].Human) return i;
        return -1;
    }
    // Use-case:
    //   Map m {foo, header_only = true}
    //   ...
    //   Map game {m.FileName (), header_only = false}
    // Way more simple than "game Map {m}"
    public const String & FileName() const { return _file_name; }
};// Map

NAMESPACE_H3R

#endif
