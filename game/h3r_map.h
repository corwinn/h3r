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

#define H3R_VERSION_ROE 0x0e
#define H3R_VERSION_AB  0x15
#define H3R_VERSION_SOD 0x1c
#define H3R_VERSION_WOG 0x33

class Map final
{
    private FFDNode * _map {};

    // cached - no need for live update
    private h3rMapVersion _version {};
    private String _version_name {};
    private bool _has_players {}; //TODO an "enigma"
    private int _nxy {}; // 36, 72, 108, 144
    private int _nz {}; // 1 or 2
    private String _name {};
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
        byte   PHPortrait {}; // 255: not customized?
        String PHName {};     // What if its not customized - a copy?
        struct CustomizedHero final { byte Id; String Name; };
        List <CustomizedHero> CustomizedHeroes {};
    };
    private List<Player> _players {};
    private int _players_can_play {};
    //
    private byte _vcon {};
    private bool _vcon_ai {};
    private bool _vcon_default_too {};
    private byte _vcon_type {};
    private byte _vcon_hlevel {};
    private byte _vcon_clevel {};
    private Location _vcon_loc {};
    private int _vcon_quantity {};
    private byte _lcon {};
    private short _lcon_quantity {};
    private Location _lcon_loc {};
    //
    private List<byte> _teams {};

    public Map(const String &, bool = true);
    public ~Map();

    public inline h3rMapVersion Version() const { return _version; }
    public inline const String & VersionName() const { return _version_name; }
    public inline int Size() const { return _nxy; }
    public inline int Levels() const { return _nz; }
    public inline const String & Name() const { return _name; }
    public inline const String & Descr() const { return _description; }
    public inline int Difficulty() const { return _difficulty; }
    public inline const String & DifficultyName() const { return _diff_name; }
    public inline int PlayerNum() const { return _players_can_play; }
    public inline const Player & PlayerAt(int i) const { return _players[i]; }
    public inline int VCon() const { return _vcon; }
    public inline int LCon() const { return _lcon; }
};// Map

NAMESPACE_H3R

#endif
