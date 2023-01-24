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

#ifndef _H3R_GAMEARCHIVES_H_
#define _H3R_GAMEARCHIVES_H_

#include "h3r.h"
#include "h3r_string.h"
#include "h3r_list.h"

H3R_NAMESPACE

#define public public:
#define private private:

// Game data archives state.
// Also, because case sensitive FS requires specific handling.
class GameArchives final
{
    public class Entry final
    {
        private bool _found {};
        public List<String> Locations;
        public String Name;
        public explicit Entry(String && name) : Name{name} {}
        public void Found() { _found = true; }
        public bool operator==(const Entry & s) const { return s.Name == Name; }
    };
    private List<Entry> _ba;
    public bool Has(const String & name)
    {
        for (var & e : _ba) if (e.Name == name) return true;
        return false;
    }
    public GameArchives()
        : _ba {List<Entry> {}
            << Entry {String {"test.lod"    }.ToLower ()}
            << Entry {String {"H3ab_ahd.snd"}.ToLower ()}
            << Entry {String {"H3ab_bmp.lod"}.ToLower ()}
            << Entry {String {"H3ab_ahd.vid"}.ToLower ()}
            << Entry {String {"H3ab_spr.lod"}.ToLower ()}
            << Entry {String {"Heroes3.snd" }.ToLower ()} // 2 of them
            << Entry {String {"Heroes3.vid" }.ToLower ()}
            << Entry {String {"H3bitmap.lod"}.ToLower ()}
            << Entry {String {"VIDEO.VID"   }.ToLower ()}
            << Entry {String {"H3sprite.lod"}.ToLower ()}}
    {
    }

    // [MOD] public void Add(String archive); //LATER
};// GameArchives

#undef private
#undef public

NAMESPACE_H3R
#endif
