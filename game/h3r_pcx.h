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

#ifndef _H3R_PCX_H_
#define _H3R_PCX_H_

#include "h3r.h"
#include "h3r_resdecoder.h"
#include "h3r_array.h"
#include "h3r_log.h"
#include "h3r_stream.h"
#include "h3r_string.h"
#include "h3r_pal.h"

H3R_NAMESPACE

//DONE at the prev commit. CAMPNOSC.PCX (238x143) looks fine.
#undef public
class Pcx final : public ResDecoder
#define public public:
{
    private Stream * _s;
    private Array<byte> _rgba {};
    private Array<byte> _rgb {};
    private Array<byte> _palette {}; // h3rPlayerColor one
    private int _w {};
    private int _h {};
    private off_t _bitmap_ofs {}; // 0-based
    private int _size {}; // bytes
    private int _fmt {}; // 1 (8-bit palette) or 3 (24-bit RGB, no palette)
    public Pcx(Stream * stream)
        : ResDecoder {}, _s{stream}
    {
        if (nullptr == stream)
            Log::Info ("PCX: no stream " EOL);
        if (! Init ())  {
            _s = nullptr; // block all ops
            _w = _h  = _size = _fmt = 0;
            _bitmap_ofs = 0;
            _rgba.Resize (0);
            _rgb.Resize (0);
        }
    }
    public ~Pcx() override {}
    // Encodes A=0 for fmt1 index=0, and A=255 for fmt1 index!=0.
    public inline Array<byte> * ToRGBA() override
    {
        if (! _rgba.Empty ()) return &_rgba;
        return Decode (_rgba, 4);
    }
    // Bug-artwork: when Width % 4 != 0:
    // glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
   // www.khronos.org/opengl/wiki/Common_Mistakes#Texture_upload_and_pixel_reads
    public inline Array<byte> * ToRGB() override
    {
        if (! _rgb.Empty ()) return &_rgb;
        return Decode (_rgb, 3);
    }
    public inline int Width() override { return _w; }
    public inline int Height() override { return _h; }
    public inline int Fmt() const { return _fmt; }
    // Init size and format info.
    private bool Init()
    {
        if (nullptr == _s) return false;
        if (! *_s) return false;
        auto & s = _s->Reset ();

        Stream::Read (s, &_size).Read (s, &_w).Read (s, &_h);
        if (_w <= 0 || _w >= ResDecoder::MAX_SIZE) {
            Log::Err (String::Format ("PCX: Wrong width: %d" EOL, _w));
            _w = _h = 0;
            return false;
        }
        if (_h <= 0 || _h >= ResDecoder::MAX_SIZE) {
            Log::Err (String::Format ("PCX: Wrong height: %d" EOL, _h));
            _w = _h = 0;
            return false;
        }
        _fmt = _size / (_w * _h);
        if (1 != _fmt && 3 != _fmt) {
            Log::Err (String::Format ("PCX: Unknown format: %d" EOL, _fmt));
            _w = _h = 0;
            return false;
        }
        if (_w*_h*_fmt != _size) {
            Log::Err (String::Format ("PCX: Wrong size: %d" EOL, _size));
            _w = _h = 0;
            return false;
        }

        _bitmap_ofs = _s->Tell ();
        H3R_ENSURE(12 == _s->Tell (), "Bug: fix your stream")
        // printf ("w:%d, h:%d, f:%d, p:%zu" EOL, _w, _h, _fmt, _bitmap_ofs);
        return true;
    }// Init
    private inline Array<byte> * Decode(Array<byte> & buf, int u8_num)
    {
        if (nullptr == _s) return nullptr;
        H3R_ENSURE(3 == u8_num || 4 == u8_num, "Can't help you")
        { _s->Reset (); _s->Seek (_bitmap_ofs); } // these are coupled

        buf.Resize (u8_num*_w*_h); // A, if present, is 0
        byte * b = buf, * e = b + buf.Length ();
        if (1 == _fmt) {
            Array<byte> src_buf {_w*_h};
            byte * src = src_buf;
            Array<byte> pal {3*256};
            byte * p = pal;
            Stream::Read (*_s, src, _w*_h);
            Stream::Read (*_s, p, pal.Length ());
            if (! _palette.Empty ()) // use player color
                OS::Memcpy (p+(256-32)*3, _palette.operator byte * (), 32*3);
            while (b != e) {
                byte i = *src++;
                *b++ = *(p+3*i), *b++ = *(p+3*i+1), *b++ = *(p+3*i+2);
                if (4 == u8_num)//TODO is it always color 0?
                    *b++ = 0 == i ? 0 : 255; // A = 0 for input color 0
            }
        }
        else
            while (b != e) {//TODO do these have transparent color?
                // nope, something is messed up; why not?
                // Stream::Read (s, b, fmt); b += u8_num;
                Stream::Read (*_s, b, _fmt);
                byte t = *b; *b = *(b+2), *(b+2) = t;
                if (4 == u8_num) *(b+3) = 255; // be non-transparent
                b += u8_num;
            }
        return &buf;
    }// Decode

    //TODO virtual at base?
    public inline void SetPlayerColor(h3rPlayerColor pc, const Pal & pal)
    {
        if (_palette.Empty ()) _palette.Resize (32*3);
        H3R_ENSURE(H3R_VALID_PLAYER_COLOR(pc), "pc shall be [0;7]")
        // "dialgbox.def" is using the last 32 for PlayerColor - I hope they all
        // are using the same offset and number.
        pal.Replace (static_cast<byte *>(_palette), 0, 32, pc << 5);
    }
};// Pcx

NAMESPACE_H3R

#endif