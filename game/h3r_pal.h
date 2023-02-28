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

#ifndef _H3R_PAL_H_
#define _H3R_PAL_H_

#include "h3r.h"
#include "h3r_resdecoder.h"
#include "h3r_array.h"
#include "h3r_log.h"
#include "h3r_stream.h"
#include "h3r_string.h"

H3R_NAMESPACE

#undef public
class Pal final : public ResDecoder
#define public public:
{
    private Stream * _s;
    private Array<byte> _rgb {};
    private short _pal_entry_num {};
    private int _size2 {};
    private off_t _pal_ofs {}; // 0-based
    public Pal(Stream * stream)
        : ResDecoder {}, _s{stream}
    {
        if (nullptr == stream)
            Log::Info ("PAL: no stream " EOL);
        if (! Init ())  {
            _s = nullptr; // block all ops
            _rgb.Resize (0);
            _pal_entry_num = 0;
            _size2 = 0;
            _pal_ofs = 0;
        }
        ToRGB ();
    }
    public ~Pal() override {}
    public inline Array<byte> * ToRGBA() override { return nullptr; }
    // This is a palette, not a bitmap.
    public inline Array<byte> * ToRGB() override
    {
        if (! _rgb.Empty ()) return &_rgb;
        return Decode (_rgb, 3);
    }
    public inline int Width () override { return 0; }
    public inline int Height () override { return 0; }
    // Init size and format info.
    private bool Init()
    {
        if (nullptr == _s) return false;
        if (! *_s) return false;
        auto & s = _s->Reset ();

        char sign1[5] {};
        int size1;
        char sign2[9] {};
        short pal_size_in_bytes;

        Stream::Read (s, sign1, 4).Read (s, &size1);
        if (size1 != s.Size () - 8) {
            Log::Err (String::Format ("PAL: Wrong size1: %d" EOL, size1));
            return false;
        }
        Stream::Read (s, sign2, 8).Read (s, &_size2);
        Stream::Read (s, &pal_size_in_bytes).Read (s, &_pal_entry_num);

        _pal_ofs = 4+4+8+4+2+2;
        return true;
    }
    private inline Array<byte> * Decode(Array<byte> & buf, int u8_num)
    {
        if (nullptr == _s) return nullptr;
        H3R_ENSURE(3 == u8_num, "Can't help you")
        { _s->Reset (); _s->Seek (_pal_ofs); } // these 2 stream ops are coupled

        Array<byte> p4 {4 * _pal_entry_num}; // that's whats read from the file
        buf.Resize (3 * _pal_entry_num); // result palette
        Stream::Read (*_s, static_cast<byte *>(p4), p4.Length ());
        // validate
        int unk_bytes = (_size2/4 - _pal_entry_num);
        H3R_ENSURE((_pal_entry_num+unk_bytes)*4 == _size2,
            "Unexpected PAL format")

        for (int i=0; i < _pal_entry_num; i++)
            buf[3*i] = p4[4*i], buf[3*i+1] = p4[4*i+1], buf[3*i+2] = p4[4*i+2];

        return &buf;
    }

    public inline int Count() { return _pal_entry_num; }

    // RGB -> RGB.
    // Replace "num" palette entries at "dst[index]" with "this[src_index]".
    // "PLAYERS.PAL": def.SetPlayerColor (PlayerColor, Pal &)
    public inline void Replace(byte * dst, int index, int num, int src_index)
        const
    {
        /*TRACE printf (
            "Pal::Replace: dst:%p, dst_idx: %d, num:%d, src:%p, src_idx: %d" EOL,
            dst, index, num, static_cast<byte *>(_rgb), src_index);*/
        for (int i = 0; i < num; i++) {
            /*printf ("Put: %d, Get: %d", 3*(index+i), 3*(src_index+i));*/
            OS::Memmove (
                dst + 3*(index+i),
                static_cast<byte *>(_rgb) + 3*(src_index+i), 3);
        }
    }
};// Pal

NAMESPACE_H3R

#endif