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

#include "h3r_gamefont.h"
#include "h3r_game.h"
#include "h3r_log.h"

H3R_NAMESPACE

GameFont::GameFont(const String & name)
    : Font {name}, _fnt {Game::GetResource (name)}
{
    if (_fnt) Log::Info (String::Format ("Font load: %s" EOL, name.AsZStr ()));
    else Log::Err (String::Format ("Font load failed: %s" EOL, name.AsZStr ()));
}

Point GameFont::MeasureText(const String & text)
{
    if (! _fnt) return Point {};
    Point result {};
    result.Y = _fnt.Height ();
    const byte * buf = text.AsByteArray ();
    for (int i = 0; i < text.Length (); i++)
        result.X += _fnt.HorisontalAdvance (buf[i]);
    return result;
}

Point GameFont::MeasureText(const String & text, int width,
    void (*on_width)(int index))
{
    if (! _fnt) return Point {};
    Point result {};
    result.Y = _fnt.Height ();
    const byte * buf = text.AsByteArray ();
    int w {};
    for (int i = 0; i < text.Length (); i++) {
        int delta = _fnt.HorisontalAdvance (buf[i]);
        result.X += delta;
        if ((w += delta) > width) { w = delta; if (on_width) on_width (i); }
    }
    return result;
}

// Convert 1 byte .fnt to 2 bytes GL_LUMINANCE_ALPHA
static void CopyGlyph(byte * dst, int w, int px, byte * src, int gw, int gh)
{
    H3R_ENSURE(px >= 0, "can't render in front of the bitmap")
    size_t dst_pitch = 2*w;
    byte * dst_row = dst + 2*px;
    for (int y = 0; y < gh ; y++) {
        for (int x = 0; x < gw ; x++) {
            byte c = src[y*gw+x];
            // dst_row[2*x] = c > 0 ? 255 : 0;    // 255: 1, 255
            // last resort to protect against wrong memory access
            // if (dst_row+2*x >= dst && dst_row+2*x+1 < dst+(y+1)*dst_pitch) {
            dst_row[2*x] = c; // unmodified; they're using 255,243,222
            // dst_row[2*x+1] = 1 == c ? 128 : c; // 0.5 alpha for smoothing
            dst_row[2*x+1] = 1 == c ? 200 : c; // 0.78 alpha for smoothing
            // }
        }
        dst_row += dst_pitch;
    }
}

void GameFont::RenderText(const String & text, byte * buf, int w, int h)
{
    (void)h; //TODO
    const byte * txt = text.AsByteArray ();
    int advance0 = _fnt.GlyphXOffset1 (txt[0]);
    if (advance0 < 0) advance0 = 0;//TODO can't render in front of the bitmap
    int gh = _fnt.Height (); int advance_x = advance0;
    for (int i = 0; i < text.Length (); i++) {
        int gw = _fnt.GlyphWidth (txt[i]);
        byte * glyph = _fnt.GlyphBitmap (txt[i]);
    // TODO h3r_debug_helpers
    /*printf ("Glyph %3d, advance:%d :" EOL, txt[i], advance_x);
    for (int y = 0; y < gh; y++) {
        for (int x = 0; x < gw; x++)
            printf ("%3d ", glyph[y*gw+x]);
        printf (EOL);
    }*/
        CopyGlyph (buf, w, advance_x, glyph, gw, gh);
    /*printf ("buf after glyph %3d:" EOL, txt[i]);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++)
            printf ("%3d ", buf[y*2*w+2*x]);
        printf (EOL);
    }*/
        advance_x += _fnt.HorisontalAdvance (txt[i]) - advance0;
    }
}


NAMESPACE_H3R