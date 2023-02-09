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
// A sprite collection. All sprites share the same palette.
// Some sprites in the collection are sprite collection themselves. I'm
// referring to them as sprite groups; but its just animated sprites I suppose.
// Usage:
//   auto sprite_handler = def.Query ("foo.pcx");
//   if (sprite_handler) bitmap = sprite_handler->RGBA ();
//   if (sprite_handler) the_same_bitmap = sprite_handler->RGBA ();
class Def final : public ResDecoder
{
    private: Stream * _s;
    private: Array<byte> _rgba {};
    private: Array<byte> _rgb {};
    private: int _w {};
    private: int _h {};
    private: int _n {};
    private: Array<byte> _palette {}; // RGB

    public: Def(Stream * stream) : ResDecoder {}, _s{stream}
    {
        if (nullptr == stream)
            Log::Info ("DEF: no stream " EOL);
        Init ();
    }
    public: ~Def() override
    {
        // A test unit signalled memory leaks here.
        // The game however did not. It looks like the leak sanitizer isn't ok.
        //TODO "valgrind" the truth.
        for (int i = 0; i < _sprites.Length (); i++) {
            for (int j = 0; j < _sprites[i].Entries.Length (); j++)
                _sprites[i].Entries[j].~SubSprite ();
            _sprites[i].~Sprite ();
        }
   }
    public: int Num() const { return _n; }
    public: inline Array<byte> * ToRGBA() override
    {
        if (! _rgba.Empty ()) return &_rgba;
        return Decode (_rgba, 4);
    }
    public: inline Array<byte> * ToRGB() override
    {
        if (! _rgb.Empty ()) return &_rgb;
        return Decode (_rgb, 3);
    }
    public: int Width () override { return _w; }
    public: int Height () override { return _h; }

    private: struct SubSprite final
    {
        String Name;
        int Offset; // 0-based
        void Read(Stream & s)
        {
            Stream::Read (s, &Offset);
        }
    };
    private: struct Sprite final
    {
        Array<SubSprite> Entries;
        void Read(Stream & s)
        {
            s.Seek (+4); // unknown1;
            int cnt;
            Stream::Read (s, &cnt); //TODO Why - you trust your input
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
    };
    private: Array<Sprite> _sprites {};
    private: SubSprite * _request {};
    private: int _request_id {-1};
    private: inline void SetRequest(SubSprite * value)
    {
        if (value != _request) {
            _rgba.Resize (0);
            _rgb.Resize (0);
            _request = value;
        }
    }

    public: class SpriteNameIterator final
    {
        private: Def * _src;
        private: int _idx {-1}, _subidx {0};
        private: String * _result {};
        public: SpriteNameIterator(Def * src) : _src{src} { MoveNext (); }
        public: inline void MoveNext()
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
        public: inline const String * Current() { return _result; }
    };
    friend class Def::SpriteNameIterator;
    public: inline SpriteNameIterator GetIterator()
    {
        return SpriteNameIterator {this};
    }

    // Set what RGBA() and RGB() shall return. Null means it wasn't found.
    //TODO are there duplicate names?
    public: inline Def * Query(const String & sprite_name)
    {
        _request = nullptr; _request_id = -1;
        for (int i = 0; i < _sprites.Length (); i++) {
            auto req = _sprites[i].ByName (sprite_name, _request_id);
            if (req) {
                SetRequest (req);
                return this;
            }
        }
        return nullptr;
    }
    //TODO to Decoder as a virtual one
    // Use Query() first.
    public: inline String GetUniqueKey(const String & suffix)
    {
        H3R_ENSURE(_request != nullptr, "Use Query() first.")
        return String::Format ("%s%d%s",
            _request->Name.AsZStr (), _request_id, suffix.AsZStr ());
    }

    private: void Init();

    private: Array<byte> * Decode(Array<byte> & buf, int u8_num);
};// Def

NAMESPACE_H3R

#endif