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

#include "h3r_label.h"
#include "h3r_scrollbar.h"
#include "h3r_textrenderingengine.h"

H3R_NAMESPACE

Label::Label(
    const String & text, const String & font, const Point & pos, Control * base,
    unsigned int color, bool mline, Point mbox)
    : Control {base}, _ml{mline}, _mb{mbox}, _font {font}, _text {text},
    _color{color}
{
    SetPos (pos.X, pos.Y);
    SetText ();
}

Label::Label(
    const String & text, const String & font, const Point & pos, Window * base,
    unsigned int color, bool mline, Point mbox)
    : Control {base}, _ml{mline}, _mb{mbox}, _font {font}, _text {text},
    _color{color}
{
    SetPos (pos.X, pos.Y);
    SetText ();
}

Label::~Label()
{
    for (int i = 0; i < _tkeys.Count (); i++)
        Window::UI->DeleteText (_tkeys[i]);
    // if (_vs) H3R_DESTROY_OBJECT(_vs, ScrollBar)
}

void Label::OnVisibilityChanged()
{
    for (int i = 0; i < _tkeys.Count (); i++)
        Window::UI->ChangeTextVisibility (_tkeys[i], ! Hidden ());
    if (_ml && _vs) _vs->SetHidden (Hidden ());
}

void Label::SetText(const String & value)
{
    _text = value;
    SetText ();
}

void Label::SetText()
{
    if (_text.Empty ()) return;
    auto RE = Window::UI;
    if (! _ml) {
        if (_tkeys.Count () <= 0)
            Window::UI->UploadText (_tkeys.Add (RE->GenTextKey ()), _font,
                _text, Pos ().X, Pos ().Y, _color, Depth ());
        else
            Window::UI->UpdateText (_tkeys[0], _font, _text, Pos ().X, Pos ().Y,
                _color, Depth ());
        return;
    }
    // Multi-line
    if (_tkeys.Count () > 0)
        for (int i = 0; i < _tkeys.Count (); i++)
            Window::UI->DeleteText (_tkeys[i]);
    _tkeys.Clear ();
    auto & TRE = TextRenderingEngine::One ();
    auto text_rows = TRE.LayoutText (_font, _text, _mb.Width);
    int y = Pos ().Y;
    _font_h = TRE.FontHeight (_font);
    if (text_rows.Count () * _font_h > _mb.Height) { // needs a vscrollbar
        // It it is the same situation as with the button: I need to create
        // foo in order for it to init its size from unknown, so I can position
        // it later. In this case I need ScrollBar::Width() in order to align
        // it right at the text box. This time the VBO gets updated.
        if (!_vs) {
            H3R_CREATE_OBJECT(_vs, ScrollBar) {
                this, Point {Pos ().X, y}, _mb.Height};
            _vs->Scroll.Subscribe (this, &Label::HandleScroll);
            _vs->SetPos (Pos ().X + _mb.Width - _vs->Width (),
                _vs->Control::Pos ().Y);
        }
        text_rows.Clear ();
        //LATER There is some invisible padding here; I doubt the game will
        //      all a glyph to "touch" the scrollbar
        int pad_r = 2, sbar_pad = _mb.Width - (_vs->Width () + pad_r);
        H3R_ENSURE(sbar_pad > 0, "No space to render the text")
        text_rows = TRE.LayoutText (_font, _text, sbar_pad);
        _vs->SetHidden (false);
    }
    else { if (_vs) _vs->SetHidden (true); }
    _num_visible = 0;
    for (auto & text : text_rows) {
        RE->UploadText (_tkeys.Add (RE->GenTextKey ()),
            _font, text, Pos ().X, y, _color, Depth ());
        y += _font_h;
        if (y <= (Pos ().Y+_mb.Height)) _num_visible++;
    }
    if (_vs && ! _vs->Hidden ()) {
        printf ("<> rows: %d, visible: %d\n", text_rows.Count (), _num_visible);
        _vs->Min = 0;
        _vs->Max = _vs->Min + (text_rows.Count () - _num_visible);
        _vs->Pos = 0;
    }
    UpdateVisible ();
}// Label::SetText()

void Label::UpdateVisible()
{
    if (nullptr == _vs) return;
    auto RE = Window::UI;
    int ty {}; // translate y
    for (int i = 0; i < _tkeys.Count (); i++) {
        if (i == _vs->Pos) ty = -(i * _font_h);
        bool v = (i >= _vs->Pos && i < _vs->Pos+_num_visible);
        RE->ChangeTextVisibility (_tkeys[i], v);
        RE->TextSetTranslateTransform (_tkeys[i], v, 0, ty);
    }
}

void Label::HandleScroll(EventArgs *)
{
    // printf ("e->Delta: %d, _vs->Pos: %d\n", e->Delta, (int)(_vs->Pos));
    UpdateVisible ();
}

NAMESPACE_H3R
