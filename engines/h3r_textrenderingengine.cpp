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

// const int H3R_MAX_TEXT_WIDTH {640}; // TODO measure the at the game
// const int H3R_MAX_TEXT_HEIGHT {480}; // TODO measure the at the game

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
            if (! line.Empty ()) rows.Put (static_cast<String &&>(line));
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
        }
        if (i >= e) break;
        l = i;
        if (! is_ws (i)) {
            while (i < e && is_sym (i)) i++;
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
        // printf ("TRE: row: %s: w: %d, h: %d" EOL, s.AsZStr (), size.X,
        //     size.Y);
    };
    // The code below should call the code above; I doubt it, but lets see.
    // That's what I call "inversion of control" :D. Ok, more "yield return" but
    // that's another topic.
    // If this works, it will greatly simplify my UI events observer by
    // eliminating a whole class and all its descendants altogether.
    //LATER R&D IEnumerable<T> using this.
    TextService__ToLines<decltype(trick)> (txt, trick,
        [](String && s, decltype(trick) & f) -> void { f ((String &&)s); });
    // printf ("TRE: rows: %d: tw: %d, th: %d, hh: %d" EOL,
    //    txt_rows.Count (), tw, th, hh);
    H3R_ENSURE(txt_rows.Count () > 0, "Nope; at least one row shall be here")

    // Use center for now.
    int line_spacing = hh/3; // TODO measure the in-game one.
    th += line_spacing * (txt_rows.Count () - 1);
    byte * row_buf = Font::AllocateBuffer (tw, hh); // One buffer for each row
    OS::__pointless_verbosity::__try_finally_free<byte> ____ {row_buf};
    // Well, I can't do that without providing pitch for copy_rectangle()s
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
        // printf ("TRE: render text of size: w:%d, h:%d" EOL,
        //    txt_rows_size[i].X, txt_rows_size[i].Y);
        fnt->RenderText (
            txt_rows[i], row_buf, txt_rows_size[i].X, txt_rows_size[i].Y);
    /*printf ("After RenderText()\n");
    for (int y = 0; y < txt_rows_size[i].Y; y++) {
        for (int x = 0; x < txt_rows_size[i].X; x++)
            printf ("%2X ", row_buf[y*2*txt_rows_size[i].X+2*x]);
        printf ("\n");
    }*/
        // printf ("TRE: copy rectangle: _tw: %d, l:%d, t:%d, w:%d, h:%d" EOL,
        //       _text_width, l, t, txt_rows_size[i].X, txt_rows_size[i].Y);
        Font::CopyRectangle (
            _text_buffer, _text_width, l, t,
            row_buf, txt_rows_size[i].X, txt_rows_size[i].Y,
            txt_rows_size[i].X*2);
    /*printf ("After CopyRectangle()\n");
    for (int y = 0; y < _text_height; y++) {
        for (int x = 0; x < _text_width; x++)
            printf ("%2X ", _text_buffer[y*2*_text_width+2*x]);
        printf ("\n");
    }*/
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