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

#ifndef _H3R_TEXTRENDERINGENGINE_H_
#define _H3R_TEXTRENDERINGENGINE_H_

#include "h3r.h"
#include "h3r_string.h"
#include "h3r_list.h"
#include "h3r_box.h"
#include "h3r_font.h"

H3R_NAMESPACE

// Renders text into off-screen buffers. Manages glyph caches.
// Manages font objects.
// Singleton.
class TextRenderingEngine final
{
    private struct FontInfo final
    {
        FontInfo() {} // List<T> constructor
        FontInfo(Font * f, const String & name)
            : F{f}, Name{name} {}
        Font * F {};
        String Name {};
    };
    private List<FontInfo> _fonts;
    private bool _ft2 {};
    private TextRenderingEngine();
    public ~TextRenderingEngine();

    // increases on demand; lowers alloc() calls
    private byte * _text_buffer {};
    private int _text_width {};
    private int _text_height {};
    public inline int TexBufferPitch() { return _text_width * 2; }
    // Render a single line of text into 2-channel LUMINANCE_ALPHA buffer.
    public byte * RenderText(const String & font_name, const String & txt,
        int & w, int & h);

    // Word-wrap "txt" to fit "box". "actualbox" is not used so far - it shall
    // be used if and when I find out "how" the game does auto-size of its
    // message boxes. There is probably some max size and some grow steps ...
    // Returns a list of rows to render.
    //
    // Differs the game one: This one can put more text prior using a vertical
    // scroll-bar: no idea why? - the same text fits and is rendered just fine.
    //
    //LATER Unicode requires stream processing of "txt".
    public List<String> LayoutText(const String & font_name, const String & txt,
        int w/*, Box & actualbox*/);

    public Point MeasureText(const String & font_name, const String & txt);

    // Helper for font loading + memory management + handle duplicates.
    // Returns nullptr on failure. The "! nullptr" one is managed by the engine.
    // The Font object can be used at RenderText().
    // Feel free to create and manage font objects outside the
    // TextRenderingEngine.
    private Font * TryLoadFont(const String & name);

    public static TextRenderingEngine & One();

    public int FontHeight(const String & name);
};// TextRenderingEngine

NAMESPACE_H3R

#endif