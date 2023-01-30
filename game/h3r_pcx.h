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

H3R_NAMESPACE

//LATER Issue: CAMPNOSC.PCX (238x143) looks skewed
class Pcx final : public ResDecoder
{
    private: Stream * _s;
    private: Array<byte> _rgba {};
    private: Array<byte> _rgb {};
    private: int _w {};
    private: int _h {};
    public: Pcx(Stream * stream)
        : ResDecoder {}, _s{stream}
    {
        if (nullptr == stream)
            Log::Info ("PCX: no stream " EOL);
    }
    public: ~Pcx() override {}
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
    private: inline Array<byte> * Decode(Array<byte> & buf, int u8_num)
    {
        H3R_ENSURE(3 == u8_num || 4 == u8_num, "Can't help you")
        if (nullptr == _s) return &buf;
        if (! *_s) return &buf;
        int size, fmt;
        auto & s = _s->Reset ();
        Stream::Read (s, &size).Read (s, &_w).Read (s, &_h);
        if (_w <= 0 || _w >= ResDecoder::MAX_SIZE) {
            Log::Err (String::Format ("PCX: Wrong width: %d" EOL, _w));
            _w = _h = 0;
            return &buf;
        }
        if (_h <= 0 || _h >= ResDecoder::MAX_SIZE) {
            Log::Err (String::Format ("PCX: Wrong height: %d" EOL, _h));
            _w = _h = 0;
            return &buf;
        }
        fmt = size / (_w * _h);
        if (1 != fmt && 3 != fmt) {
            Log::Err (String::Format ("PCX: Unknown format: %d" EOL, fmt));
            _w = _h = 0;
            return &buf;
        }
        if (_w*_h*fmt != size) {
            Log::Err (String::Format ("PCX: Wrong size: %d" EOL, size));
            _w = _h = 0;
            return &buf;
        }
        buf.Resize (u8_num*_w*_h); // A, if present, is 0
        byte * b = buf, * e = b + buf.Length ();
        if (1 == fmt) {
            Array<byte> pal {3*256};
            byte * p = pal;
            // ZipInflateStream can seek forwards only.
            auto bitmap_sentinel = s.Tell ();
            s.Seek (size);
            Stream::Read (s, p, pal.Length ());
            s.Reset ();
            s.Seek (bitmap_sentinel);
            byte i;
            while (b != e) {
                Stream::Read (s, &i, 1);
                OS::Memmove (b, p+3*i, 3);
                b += u8_num;
            }
        }
        else
            while (b != e) {
                // nope, something is messed up; why not?
                // Stream::Read (s, b, fmt); b += u8_num;
                Stream::Read (s, b, fmt);
                byte t = *b; *b = *(b+2), *(b+2) = t;
                b += u8_num;
            }
        return &buf;
    }// Decode
};// Pcx

NAMESPACE_H3R

#endif