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

#ifndef _H3R_FONT_H_
#define _H3R_FONT_H_

#include "h3r.h"
#include "h3r_os.h"
#include "h3r_point.h"
#include "h3r_string.h"

H3R_NAMESPACE

class Font
{
    private: String _name;
    protected: Font(const String & name) : _name {name} {}
    public: virtual ~Font() {}
    public: inline bool operator==(const Font & b) const
    {
        return b._name == _name;
    }
    public: inline bool operator!=(const Font & b) const
    {
        return ! operator== (b);
    }

    public: virtual Point MeasureText(const String &) { return Point {}; }

    // Render a byte sequence into "buf" of size w*h*2 bytes.
    // Each pixel: byte[0] - has luminance value; byte[1] has alpha value; e.g.
    // you put at byte[0] the brightness of the glyph pixel; and encode its
    // transparency at byte[1].
    // The contract: GL_LUMINANCE_ALPHA. Use Font::AllocateBuffer(w, h).
    // What was MeasureText() above, shall be give here.
    public: virtual void RenderText(const String &, byte *, int, int) {}

    // This buffer is not mine to handle!
    public: static inline byte * AllocateBuffer(int w, int h)
    {
        byte * buf {};
        OS::Alloc (buf, w*h*2); // GL_LUMINANCE_ALPHA
        return buf;
    }

    // Complement to the above function.
    // copy width*height from "src" at left,top at "dst"; "dw" is dst width;
    public: static inline void CopyRectangle (
        byte * dst, int dw, int l, int t, const byte * src, int w, int h)
    {
        size_t dst_row_pitch = dw * 2;
        byte * dst_row = dst + t * dst_row_pitch + l;
        for (const byte * src_row = src; src_row < src+w*h*2; src_row += w*2) {
            OS::Memcpy (dst_row, src_row, w*2);
            dst_row += dst_row_pitch;
        }
    }
};

NAMESPACE_H3R

#endif