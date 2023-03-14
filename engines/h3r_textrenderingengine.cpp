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
    H3R_CREATE_OBJECT(f, Fnt) {Game::GetResource ("verd10b.fnt")}; _fonts <<f;*/
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

// const int H3R_MAX_TEXT_WIDTH {640}; // TODO measure it at the game
// const int H3R_MAX_TEXT_HEIGHT {480}; // TODO measure it at the game

// The game handles very long words by wrapping them.
// The game handles \r\n.
static inline bool is_ws(const byte * i)
{
    return *i <= 32 && '\r' != *i && '\n' != *i;
}
static inline bool is_sym(const byte * i) { return *i > 32; }
static inline bool is_eol(const byte * & i, const byte * m)
{
    if (i > m) return false;
    if (m-1 == i) return i++, '\n' == *i;
    if ('\r' == *i && '\n' == *(i+1)) return i += 2, true;
    if ('\n' == *i) return i++, true;
    return false;
}
static inline void skip_ws(const byte * & i, const byte * m)
{
    while (i < m && is_ws (i)) i++;
}
static inline void handle_token(List<String> & rows, String & line,
    const String & token, int w, int & tw, Font * fnt)
{
    static List<int> idx {};
    idx.Clear ();
    Point ts = fnt->MeasureText (token);
    if (ts.Width > w) { // a token is wider than the allowed width
        // This requires a special MeasureText() in order to avoid creating
        // too many Strings.
        //TODO there is at least one bug here; testme
        fnt->MeasureText (token, w, [](int i) { idx.Add (i); });
        int a = 0;
        auto buf = token.AsByteArray ();
        for (auto i : idx) {
            // printf ("<> TooLong ends at: %d\n", i - 1);
            if (i-a > 0) {
                String p {buf + a, i - a};
                rows.Put (static_cast<String &&>(p));
            }
            a = i;
        }
        if (a < token.Length ()) { // put the last one if any
            String p {buf + a, token.Length () - a};
            rows.Put (static_cast<String &&>(p));
        }
    }
    else {
        tw -= ts.Width;
        if (tw < 0) {
            if (! line.Empty ()) {
                rows.Put (static_cast<String &&>(line));
                if (" " == token) return; // skip " " at line start
            }
            line += token; tw = w - ts.Width;
            H3R_ENSURE(tw >= 0, "Unhandled width overflow")
        }
        else
            line += token;
    }
}// handle_token()

// VScrollBar & text layout: the lay-outing is done without the vscrollbar. If a
// vscrollbar is needed, the lay-outing is re-done against the adjusted width.
List<String> TextRenderingEngine::LayoutText(const String & font_name,
    const String & txt, int w/*, Box & actualbox*/)
{
    Font * fnt = TryLoadFont (font_name);
    H3R_ENSURE(fnt != nullptr, "Font not found")

    List<String> txt_rows {};
    // int line_spacing = 0; // so far the spacing is built-in into the font
    const byte * l = txt.AsByteArray (), * i = l, * e = l + txt.Length ();
    int tw = w;
    String line {""};
    for (;i < e;) {
        // handle new line(s)
        while (i < e && is_eol (i, e)) {
            // printf ("<> EOL\n");
            txt_rows.Put (static_cast<String &&>(line));
            line += "";
            tw = 0;
        }
        if (i >= e) break;
        l = i;
        if (! is_ws (i)) {
            auto chk = i;
            while (i < e && is_sym (i)) i++;
            // if (i == chk) printf ("%002X, e-i:%ld\n", *i, e-i);
            H3R_ENSURE(i > chk, "neither ws nor sym leads to infinity")
            /*printf ("<> Word: ");
                for (auto c = l; c < i; c++) printf ("%c", *c);
            printf ("\n");*/
            String word {l, (int)(i-l)};
            handle_token (txt_rows, line, word, w, tw, fnt);
            l = i;
        }
        else {
            skip_ws (l, e);
            if (l > i) { // whitespace
                // printf ("<> Whitespace of size %ld\n", l-i);
                String t {i, (int)(l-i)};
                handle_token (txt_rows, line, t, w, tw, fnt);
            }
            i = l;
        }
    }// (;i < e;)
    if (! line.Empty ()) {
        if (txt_rows.Count () <= 0)
            txt_rows.Put (static_cast<String &&>(line));
        else if (txt_rows[txt_rows.Count ()-1] != line)
            txt_rows.Put (static_cast<String &&>(line));
    }
    /*for (int j = 0; j < txt_rows.Count (); j++) {
        if (txt_rows[j].Empty ())
             printf ("<> [%2d]: \"\"\n", j);
        else printf ("<> [%2d]: \"%s\"\n", j, txt_rows[j].AsZStr ());
    }*/
    return static_cast<List<String> &&>(txt_rows);
}// TextRenderingEngine::LayoutText()

byte * TextRenderingEngine::RenderText(
    const String & font_name, const String & txt, int & tw, int & th)
{
    Font * fnt = TryLoadFont (font_name);
    H3R_ENSURE(fnt != nullptr, "Font not found")

    tw = th = 0;
    if (txt.Empty ()) return nullptr;
    Point size = fnt->MeasureText (txt);
    tw = size.Width;
    th = size.Height;
    // printf ("tw: %d, th: %d\n", tw, th);
    //TODO shall I render an "Error: Empty String"?

    byte * row_buf = Font::AllocateBuffer (tw, th); // One buffer for each row
    OS::__pointless_verbosity::__try_finally_free<byte> ____ {row_buf};
    // Well, I can't do that without providing pitch for copy_rectangle()s
    if (tw > _text_width || th > _text_height) {
        if (_text_buffer) OS::Free (_text_buffer);
        _text_buffer =
            Font::AllocateBuffer (_text_width = tw, _text_height = th);
        printf ("TRE: resizing rendering buffer to: w:%d, h:%d" EOL, tw, th);
    }

    fnt->RenderText (txt, row_buf, tw, th);
    /*printf ("After RenderText()\n");
    for (int y = 0; y < th; y++) {
        for (int x = 0; x < tw; x++)
            printf ("%2X ", row_buf[y*2*tw+2*x]);
        printf ("\n");
    }*/
    // printf ("TRE: copy rectangle: _tw: %d, 0:%d, 0:%d, w:%d, h:%d" EOL,
    //       _text_width, 0, 0, tw, th);
    Font::CopyRectangle (
        _text_buffer, _text_width, 0, 0, row_buf, tw, th, tw*2);
    /*printf ("After CopyRectangle()\n");
    for (int y = 0; y < _text_height; y++) {
        for (int x = 0; x < _text_width; x++)
            printf ("%2X ", _text_buffer[y*2*_text_width+2*x]);
        printf ("\n");
    }*/
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

int TextRenderingEngine::FontHeight(const String & name)
{
    Font * fnt = TryLoadFont (name);
    return fnt->Height ();
}

NAMESPACE_H3R