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

#ifndef _H3R_FNT_H_
#define _H3R_FNT_H_

#include "h3r.h"
#include "h3r_resdecoder.h"
#include "h3r_array.h"
#include "h3r_log.h"
#include "h3r_stream.h"
#include "h3r_string.h"

H3R_NAMESPACE

class Fnt final : public ResDecoder
{
    private: Stream * _s;
#pragma pack(push, 1)
    private: struct Entry
    {
        int XOffset1;
        int Width;
        int XOffset2;
    };
#pragma pack(pop)
    private: Array<Fnt::Entry> _entries {256}; // read as one block
    private: Array<byte> _data {}; // bitmaps read as one block
    private: int _ww {}; // widest width
    private: int _h {};  // highest height
    private: bool _ok {false};
    public: Fnt(Stream * stream)
        : ResDecoder {}, _s{stream}
    {
        if (nullptr == stream)
            Log::Info ("FNT: no stream " EOL);
        if (! (_ok = Init ()))  {
            _s = nullptr; // block all ops
            _ww = _h = 0;
            _entries.Clear ();
            _data.Resize (0);
            _glyphs.Clear ();
        }
    }
    public: ~Fnt() override {}
    // widest width
    public: int Width () override { return _ww; }
    // highest height
    public: int Height () override { return _h; }
    public: struct Glyph : public Entry
    {
        byte * Bitmap; // refers _data
        Glyph(const Entry & c)
        {
            XOffset1 = c.XOffset1; XOffset2 = c.XOffset2; Width = c.Width;
        }
    };
    // Init size and format info.
    private: bool Init()
    {
        if (nullptr == _s) return false;
        if (! *_s) return false;
        auto & s = _s->Reset ();

        s.Seek (5);
        byte h;
        Stream::Read (s, &h); _h = h;
        s.Seek (26);
        if (_h <= 0 || _h >= ResDecoder::MAX_SIZE) {
            Log::Err (String::Format ("Fnt: Wrong height: %d" EOL, _ww));
            _ww = _h = 0;
            return false;
        }
        Stream::Read (s,
            static_cast<Fnt::Entry *>(_entries), _entries.Length ());
        Array<int> ofs {256};
        Stream::Read (s, static_cast<int *>(ofs), ofs.Length ());
        _data.Resize (s.Size () - (32+256*4*4));
        Stream::Read (s, static_cast<byte *>(_data), _data.Length ());
        // printf ("fnt: Tell: %zu, Size: %zu" EOL, s.Tell (), s.Size ());
        int pos = 0;
        for (int i = 0; i < ofs.Length (); i++) {
            // printf ("ofs[%d]: %d, pos: %d" EOL, i, ofs[i], pos);
            H3R_ENSURE(ofs[i] == pos, "Fnt: corrupted data")
            _glyphs[i] = _entries[i];
            _glyphs[i].Bitmap = static_cast<byte *>(_data) + pos;
            pos += _entries[i].Width * _h;
        }
        return true;
    }// Init
    private: Array<Fnt::Glyph> _glyphs {256}; // cache
    public: operator bool() { return _ok; }//TODO at the base class
    public: inline int HorisontalAdvance(byte char_index)
    {
        const Fnt::Glyph & g = _glyphs[char_index];
        return g.XOffset1 + g.Width + g.XOffset2;
    }
    public: inline int GlyphWidth(byte char_index)
    {
        return _glyphs[char_index].Width;
    }
    public: inline byte * GlyphBitmap(byte char_index)
    {
        return _glyphs[char_index].Bitmap;
    }
    // For line start
    public: inline int GlyphXOffset1(byte char_index)
    {
        return _glyphs[char_index].XOffset1;
    }
};// Fnt

NAMESPACE_H3R

#endif