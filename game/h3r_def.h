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

#ifndef _H3R_DEF_H_
#define _H3R_DEF_H_

#include "h3r.h"
#include "h3r_array.h"
#include "h3r_log.h"
#include "h3r_resdecoder.h"
#include "h3r_stream.h"
#include "h3r_string.h"
#include "h3r_os.h"
#include "h3r_pal.h"
#include "h3r_memorystream.h"

#define H3R_DEF_MAX_SPRITE_NUM (1<<10)
#define H3R_DEF_MAX_FILE_SZIE  (1<<22)

H3R_NAMESPACE

namespace {
#pragma pack(push, 1)
struct SubSpriteHeader final
{
    int Size; // sprite size w/o this "header"
    int Type; // 0 - uncompressed; 2, 3 - 224; 1 - 255; see below
    int AnimWidth;  // matches the Sprite header one
    int AnimHeight; // matches the Sprite header one
    int Width;   // actual bitmap width (the stored one)
    int Height;  // actual bitmap height (the stored one)
    int Left;    // offset of the actual bitmap inside AnimWidth
    int Top;     // offset of the actual bitmap inside AnimHeight
};
#pragma pack(pop)
}

// Not thread-safe.
//
// Local stream: you can load it at place 1 and re-use it at place(s) != 1.
//
// A sprite collection. All sub-sprites share the same palette.
// Some sprites in the collection are sprite collection themselves. I'm
// referring to them as sprite groups; but its just animated sprites I suppose.
// Usage:
//   auto sprite_handler = def.Query ("foo.pcx");
//   if (sprite_handler) bitmap = sprite_handler->RGBA ();
//   if (sprite_handler) the_same_bitmap = sprite_handler->RGBA ();
#undef public
class Def final : public ResDecoder
#define public public:
{
    // private Stream * _s;
    private Array<byte> _rgba {};
    private Array<byte> _rgb {};
    private int _w {};
    private int _h {};
    private int _n {};
    private Array<byte> _palette {}; // RGB

    private MemoryStream _s;

    public Def(Stream *);
    public ~Def() override
    {
        for (int i = 0; i < _sprites.Length (); i++) {
            for (int j = 0; j < _sprites[i].Entries.Length (); j++)
                _sprites[i].Entries[j].~SubSprite ();
            _sprites[i].~Sprite ();
        }
    }
    public int Num() const { return _n; }
    public inline Array<byte> * ToRGBA() override
    {
        if (! _rgba.Empty ()) return &_rgba;
        return Decode (_rgba, 4);
    }
    public inline Array<byte> * ToRGB() override
    {
        if (! _rgb.Empty ()) return &_rgb;
        return Decode (_rgb, 3);
    }

    // Common for all sub-sprites.
    public inline int Width() override { return _w; }
    public inline int Height() override { return _h; }

    // For the last queried one only.
    public inline int FLeft() const { return _request ? _request->H.Left : 0; }
    public inline int FTop() const { return _request ? _request->H.Top : 0; }

    private struct SubSprite final
    {
        String Name {};
        int Offset {}; // 0-based
        SubSpriteHeader H {};
        void Read(Stream & s)
        {
            Stream::Read (s, &Offset);
            auto sentinel = s.Tell ();
            { s.Reset (); s.Seek (Offset); }
            Stream::Read (s, &H);
            { s.Reset (); s.Seek (sentinel); }
        }
    };
    private struct Sprite final
    {
        Array<SubSprite> Entries;
        void Read(Stream & s)
        {
            s.Seek (+4); // unknown1;
            int cnt;
            Stream::Read (s, &cnt);
            H3R_ENSURE(cnt > 0 && cnt < H3R_DEF_MAX_SPRITE_NUM, "Bad .def")
            s.Seek (+4); // unknown2
            s.Seek (+4); // unknown3
            Entries.Resize (cnt);
            for (int j = 0; j < cnt; j++) {
                char name[13];
                Stream::Read (s, name, 13);
                Entries[j].Name = static_cast<const char *>(name);
            }
            for (int j = 0; j < cnt; j++)
                Entries[j].Read (s);
        }
        // Return both the sprite and its id; used for the unque key id.
        SubSprite * ByName(const String & name, int & id)
        {
            for (int i = 0; i < Entries.Length (); i++)
                if (Entries[i].Name == name) return &(Entries[id=i]);
            return nullptr;
        }
        SubSprite * ByNameCI(const String & name, int & id)
        {
            for (int i = 0; i < Entries.Length (); i++)
                if (Entries[i].Name.ToLower () == name) return &(Entries[id=i]);
            return nullptr;
        }
    };
    private Array<Sprite> _sprites {};
    private SubSprite * _request {};
    private int _request_id {-1};
    private inline void SetRequest(SubSprite * value)
    {
        if (value != _request) {
            _rgba.Resize (0);
            _rgb.Resize (0);
            _request = value;
        }
    }

    public class SpriteNameIterator final
    {
        private Def * _src;
        private int _idx {-1}, _subidx {0};
        private String * _result {};
        public SpriteNameIterator(Def * src) : _src{src} { MoveNext (); }
        public inline void MoveNext()
        {
            if (_src->_sprites.Length () <= 0) return;
            if (_idx >= 0
                && _subidx < _src->_sprites[_idx].Entries.Length () - 1)
                _subidx++;
            else {
                if (_idx < _src->_sprites.Length () - 1)
                    _idx++, _subidx = 0;
                else { _result = nullptr; return; }
            }
            _result = &(_src->_sprites[_idx].Entries[_subidx].Name);
        }
        public inline const String * Current() { return _result; }
    };
    friend class Def::SpriteNameIterator;
    public inline SpriteNameIterator GetIterator()
    {
        return SpriteNameIterator {this};
    }

    // Set what RGBA() and RGB() shall return. Null means it wasn't found.
    //TODO are there duplicate names? Yes there are.
    public inline Def * Query(const String & sprite_name)
    {
        _request = nullptr; _request_id = -1;
        for (int i = 0; i < _sprites.Length (); i++) {
            auto req = _sprites[i].ByName (sprite_name, _request_id);
            if (req) {
                SetRequest (req);
                return this;
            }
        }
        return QueryCI (sprite_name);
    }
    private inline Def * QueryCI(const String & sprite_name)
    {
        _request = nullptr; _request_id = -1;
        for (int i = 0; i < _sprites.Length (); i++) {
            auto req = _sprites[i].ByNameCI (sprite_name, _request_id);
            if (req) {
                SetRequest (req);
                return this;
            }
        }
        return nullptr;
    }
    public inline Def * Query(int block, int sub_sprite)
    {
        H3R_ENSURE(block >= 0 && block < _sprites.Length (), "No such block")
        auto & b = _sprites[block];
        H3R_ENSURE(sub_sprite >= 0 && sub_sprite < b.Entries.Length (),
            "No such sprite")
        //TODO generate distinct _request_id: f(block, sub_sprite)
        SetRequest (&(b.Entries[_request_id = sub_sprite]));
        return this;
    }
    public inline int BlockNum() const { return _sprites.Length (); }
    public inline int SpriteNum(int block) const
    {
        H3R_ENSURE(block >= 0 && block < _sprites.Length (), "No such block")
        return _sprites[block].Entries.Length ();
    }
    //TODO to Decoder as a virtual one
    // Use Query() first.
    public inline String GetUniqueKey(const String & suffix)
    {
        H3R_ENSURE(_request != nullptr, "Use Query() first.")
        return String::Format ("%s%d%s",
            _request->Name.AsZStr (), _request_id, suffix.AsZStr ());
    }
    // Use Query() first.
    public inline const String & FrameName() const
    {
        H3R_ENSURE(_request != nullptr, "Use Query() first.")
        return _request->Name;
    }

    private void Init();

    private Array<byte> * Decode(Array<byte> & buf, int u8_num);

    //TODO think about a way to automatically set this based on description,
    //     e.g. a property; for example:
    //     sprite dialgbox.def
    //       flags PLAYER_COLOR
    //TODO check the unknown bytes at the sprite - perhaps something within it
    //     hints about the above
    public inline void SetPlayerColor(h3rPlayerColor pc, const Pal & pal)
    {
        H3R_ENSURE(H3R_VALID_PLAYER_COLOR(pc), "pc shall be [0;7]")
        // "dialgbox.def" is using the last 32 for PlayerColor - I hope they all
        // are using the same offset and number.
        pal.Replace (static_cast<byte *>(_palette), 256-32, 32, pc << 5);
    }

    // Roll colors at the palette. State: to get the new bitmap, call ToRGB(A).
    public inline void PaletteAnimationL(int index, int count)
    {
        H3R_ENSURE(count > 1, "\"animation\" requires 2, for starters")
        H3R_ENSURE((index + count) <= (_palette.Length () / 3), "out of range")
        auto p = _palette.operator byte * () + 3*index;
        // roll left
        int i = 0, j = 1, c = count-1;
        byte r = *p, g = *(p+1), b = *(p+2);
        while (c--) {
            *(p+3*i+0) = *(p+3*j+0);
            *(p+3*i+1) = *(p+3*j+1);
            *(p+3*i+2) = *(p+3*j+2);
            i++;
            j = (j + 1) % count;
        }
        H3R_ENSURE(0 == j, "you have a bug")
        *(p+3*i+0) = r;
        *(p+3*i+1) = g;
        *(p+3*i+2) = b;
        _rgba.Resize (0);
        _rgb.Resize (0);
    }// PaletteAnimationL()

    public inline void PaletteAnimationR(int index, int count)
    {
        H3R_ENSURE(count > 1, "\"animation\" requires 2, for starters")
        H3R_ENSURE((index + count) <= (_palette.Length () / 3), "out of range")
        auto p = _palette.operator byte * () + 3*index;
        // roll right
        int i = count-2, j = count-1, c = count-1;
        byte r = *(p+3*c), g = *(p+3*c+1), b = *(p+3*c+2);
        while (c--) {
            *(p+3*j+0) = *(p+3*i+0);
            *(p+3*j+1) = *(p+3*i+1);
            *(p+3*j+2) = *(p+3*i+2);
            j--;
            i--;
        }
        H3R_ENSURE(0 == j, "you have a bug")
        *p = r;
        *(p+1) = g;
        *(p+2) = b;
        _rgba.Resize (0);
        _rgb.Resize (0);
    }// PaletteAnimationR()
};// Def

NAMESPACE_H3R

#endif