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
#include "h3r_dbg.h"

H3R_NAMESPACE

GameFont::GameFont(const String & name)
    : Font {name}, _fnt {Game::GetResource (name)}
{
    if (_fnt) Log::Info (String::Format ("Font load: %s" EOL, name.AsZStr ()));
    else Log::Err (String::Format ("Font load failed: %s" EOL, name.AsZStr ()));
}

static inline int HorisontalAdvance(const byte * buf, int i, Fnt & fnt)
{
    return (i > 0 ? fnt.GlyphXOffset2 (buf[i-1]) : 0)
        + fnt.GlyphXOffset1 (buf[i]) + fnt.GlyphWidth (buf[i]);
}

Point GameFont::MeasureText(const String & text)
{
    if (! _fnt) return Point {};
    Point result {};
    result.Y = _fnt.Height ();
    const byte * buf = text.AsByteArray ();
    for (int i = 0; i < text.Length (); i++)
        result.X += HorisontalAdvance (buf, i, _fnt);
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
        int delta = HorisontalAdvance (buf, i, _fnt);
        result.X += delta;
        if ((w += delta) > width) { w = delta; if (on_width) on_width (i); }
    }
    return result;
}

// Convert 1 byte .fnt to 2 bytes GL_LUMINANCE_ALPHA
static void CopyGlyph(byte * dst, int w, int px, byte * src, int gw, int gh)
{
    /* H3R_ENSURE(px >= 0, "can't render in front of the bitmap")
    printf (" bug hunter: dst: %p, w: %d, px: %d, src: %p, gw: %d, gh: %d\n",
        dst, w, px, src, gw, gh);*/
    size_t dst_pitch = 2*w;
    byte * dst_row = dst + 2*(px < 0 ? 0 : px);
    for (int y = 0; y < gh ; y++) {
        for (int x = (px < 0 ? -px : 0); x < gw ; x++) {
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
    /*Point pp = MeasureText (text);
    printf ("bug-hunter: buf: %p, w: %d, h: %d, txt.w:%d\n", buf, w, h,
        pp.Width);*/
    (void)h; //TODO
    const byte * txt = text.AsByteArray ();
    int gh = _fnt.Height (); int advance_x = 0;
    for (int i = 0; i < text.Length (); i++) {
        int gw = _fnt.GlyphWidth (txt[i]);
        byte * glyph = _fnt.GlyphBitmap (txt[i]);
        /*Dbg.Fmt ("Glyph %3d, advance: ", txt[i]) << advance_x << " :" << EOL;
        PrintBuf2D ("%3d ", glyph, gw, gh, 1);*/
        /*printf (" bug-hunter g:[%d, %d, %d]\n", _fnt.GlyphXOffset1 (txt[i]),
            _fnt.GlyphWidth (txt[i]), _fnt.GlyphXOffset2 (txt[i]));*/
        advance_x += _fnt.GlyphXOffset1 (txt[i])
            + (i > 0 ? _fnt.GlyphXOffset2 (txt[i-1]) : 0);
        CopyGlyph (buf, w, advance_x, glyph, gw, gh);
        /*Dbg.Fmt ("buf after glyph %3d:", txt[i]) << EOL;
        PrintBuf2D ("%3d ", buf, w, h, 2);*/
        advance_x += _fnt.GlyphWidth (txt[i]);
    }
}// GameFont::RenderText()

NAMESPACE_H3R