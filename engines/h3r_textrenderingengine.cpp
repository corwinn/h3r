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

#include "h3r_os.h"
#include "h3r_gamefont.h"
#include "h3r_log.h"
#include "h3r_textrenderingengine.h"

#include <new>

H3R_NAMESPACE

TextRenderingEngine::TextRenderingEngine()
{
    /*Fnt * f;
    H3R_CREATE_OBJECT(f, Fnt) {Game::GetResource ("CALLI10R.FNT")}; _fonts << f;
    H3R_CREATE_OBJECT(f, Fnt) {Game::GetResource ("CREDITS.FNT")}; _fonts << f;
    H3R_CREATE_OBJECT(f, Fnt) {Game::GetResource ("HISCORE.FNT")}; _fonts << f;
    H3R_CREATE_OBJECT(f, Fnt) {Game::GetResource ("MedFont.fnt")}; _fonts << f;
    H3R_CREATE_OBJECT(f, Fnt) {Game::GetResource ("TIMES08R.FNT")}; _fonts << f;
    H3R_CREATE_OBJECT(f, Fnt) {Game::GetResource ("bigfont.fnt")}; _fonts << f;
    H3R_CREATE_OBJECT(f, Fnt) {Game::GetResource ("smalfont.fnt")}; _fonts << f;
    H3R_CREATE_OBJECT(f, Fnt) {Game::GetResource ("tiny.fnt")}; _fonts << f;
    H3R_CREATE_OBJECT(f, Fnt) {Game::GetResource ("verd10b")}; _fonts << f;*/
}

TextRenderingEngine::~TextRenderingEngine()
{
    for (int i = 0; i < _fonts.Count (); i++)
        H3R_DESTROY_OBJECT(_fonts[i].F, Font)
    if (_text_buffer) {
        OS::Free (_text_buffer);
        _text_width = _text_height = 0;
    }
}

Font * TextRenderingEngine::TryLoadFont(const String & name)
{
    // 9 fonts - you don't need a "map" I assure you.
    if (_fonts.Count () > 0)//TODO remove after hr3_array gets fixed
    for (int i = _fonts.Count () - 1; i >= 0 ; i--)
        if (name == _fonts[i].Name)
            return _fonts[i].F;

    //LATER The game fonts are a priority for the time being.
    GameFont * f {};
    H3R_CREATE_OBJECT(f, GameFont) {name};
    if (! *f) {
        H3R_DESTROY_OBJECT(f, GameFont)
        if (! _ft2)
            return nullptr;
    }
    if (! _ft2) {
        TextRenderingEngine::FontInfo info {f, name};
        _fonts.Add (info);
        return f;
    }

    //LATER FT2
    return nullptr;
}

// Split to lines. ASCII-only, LF and or CRLF for now.
//LATER TextService::ToLines(const String &); and test me; also iconv, not
//      ASCII-only
template <typename T>
static void TextService__ToLines(const String & txt, T & s,
    void (*on_line)(String &&, T &))
{
    const byte * buf = txt.AsByteArray (), * l =  buf;
    int const tlen = txt.Length ();
    for (int i = 0; i < tlen; i++)
        if ('\n' == buf[i] && (i > 0 && buf[i-1] != '\r')) {
            on_line (String {l, (int)((buf + i) - l)}, s);
            l = buf+i+1;
        }
        else if ('\r' == buf[i] && (i < tlen-1 && '\n' == buf[i+1])) {
            on_line (String {l, (int)((buf + i) - l)}, s);
            l = buf+i+2;
        }
    if (l < buf + tlen)
        on_line (String {l, (int)((buf + tlen) - l)}, s);
}

const int H3R_MAX_TEXT_WIDTH {640}; // TODO measure the at the game
const int H3R_MAX_TEXT_HEIGHT {480}; // TODO measure the at the game

String TextRenderingEngine::LayoutText(const String & font_name,
    const String & txt, const Box & box, Box & actualbox)
{
    H3R_NOT_IMPLEMENTED_EXC

    Font * fnt = TryLoadFont (font_name);
    H3R_ENSURE(fnt != nullptr, "Font not found")
    if (H3R_MAX_TEXT_HEIGHT) return txt;

    List<String> txt_rows {};
    int line_spacing = 0;
    int tw = 0, th = 0; // TODO MeasureMultilineText - see
                        // TextRenderingEngine::RenderText() below
    int bw = box.Size.X - box.Pos.X, bh = box.Size.Y - box.Pos.Y; // box w,h
    if (tw > bw || th > bh) {// doesn't fit
        actualbox = box;
        if (tw > H3R_MAX_TEXT_WIDTH) {
            bw = H3R_MAX_TEXT_WIDTH;
            //TODO wrap the lines; 1st compute without scrollbar; if it fits
            //     H3R_MAX_TEXT_HEIGHT you're done; if it doesn't, re-compute
            //     with -scrollbar.width (because showing a scrollbar shrinks
            //     the render area; if its too slow, consider scrollbar.width
            //     prior calling this function
            H3R_ENSURE(false, "Line wrapping not implemented yet")
            // hh shouldn't change; tw should be updated while wrapping
            th += line_spacing * txt_rows.Count () - 1;
        }
        else // just resize to fit the text
            bw = tw;
        actualbox.Size.X = bw - box.Pos.X;

        if (th > bh) // just resize to fit the text; the caller should apply
                     // a scrollbar
            bh = th;
        actualbox.Size.Y = bh - box.Pos.Y;
    }
    return txt;
}

byte * TextRenderingEngine::RenderText(
    const String & font_name, const String & txt, int & tw, int & th)
{
    Font * fnt = TryLoadFont (font_name);
    H3R_ENSURE(fnt != nullptr, "Font not found")

    tw = th = 0;
    if (txt.Empty ()) return nullptr;
    //TODO shall I render an "Error: Empty String"?

    List<Point> txt_rows_size {};
    List<String> txt_rows {};
    int hh {}; // text width, text height, highest height
    auto trick = [&](String && s)
    {
        txt_rows.Add ((String &&)s);
        Point & size = txt_rows_size.Add (fnt->MeasureText (s));
        if (size.X > tw) tw = size.X;
        if (size.Y > hh) hh = size.Y;
        th += size.Y;
        printf ("TRE: row: %s: w: %d, h: %d" EOL, s.AsZStr (), size.X, size.Y);
    };
    // The code below should call the code above; I doubt it, but lets see.
    // That's what I call "inversion of control" :D. Ok, more "yield return" but
    // that's another topic.
    // If this works, it will greatly simplify my UI events observer by
    // eliminating a whole class and all its descendants altogether.
    //LATER R&D IEnumerable<T> using this.
    TextService__ToLines<decltype(trick)> (txt, trick,
        [](String && s, decltype(trick) & f) -> void { f ((String &&)s); });
    printf ("TRE: rows: %d: tw: %d, th: %d, hh: %d" EOL,
        txt_rows.Count (), tw, th, hh);
    H3R_ENSURE(txt_rows.Count () > 0, "Nope; at least one row shall be here")

    // Use center for now.
    int line_spacing = hh/3; // TODO measure the in-game one.
    th += line_spacing * (txt_rows.Count () - 1);
    byte * row_buf = Font::AllocateBuffer (tw, hh); // One buffer for each row
    OS::__pointless_verbosity::__try_finally_free<byte> ____ {row_buf};
    if (tw > _text_width || th > _text_height) {
        if (_text_buffer) OS::Free (_text_buffer);
        _text_buffer =
            Font::AllocateBuffer (_text_width = tw, _text_height = th);
        printf ("TRE: resizing rendering buffer to: w:%d, h:%d" EOL, tw, th);
    }

    // render the txt_rows
    int b = hh;
    for (int i = 0 ; i < txt_rows.Count (); i++) {
        int l = (tw - txt_rows_size[i].X) / 2; // hcenter
        int t = b - hh;
        printf ("TRE: render text of size: w:%d, h:%d" EOL,
            txt_rows_size[i].X, txt_rows_size[i].Y);
        fnt->RenderText (
            txt_rows[i], row_buf, txt_rows_size[i].X, txt_rows_size[i].Y);
        printf ("TRE: copy rectangle: l:%d, t:%d, w:%d, h:%d" EOL, l, t,
            txt_rows_size[i].X, txt_rows_size[i].Y);
        Font::CopyRectangle (
            _text_buffer, _text_width, l, t,
            row_buf, txt_rows_size[i].X, txt_rows_size[i].Y);
        b += hh+line_spacing;
    }

    return _text_buffer;
}// TextRenderingEngine::RenderText()

Point TextRenderingEngine::MeasureText(
    const String & font_name, const String & txt)
{
    Font * fnt = TryLoadFont (font_name);
    H3R_ENSURE(fnt != nullptr, "Font not found")

    return fnt->MeasureText (txt);
}

TextRenderingEngine & TextRenderingEngine::One()
{
    static TextRenderingEngine e {};
    return e;
}

NAMESPACE_H3R