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

#ifndef _H3R_LABEL_H_
#define _H3R_LABEL_H_

#include "h3r.h"
#include "h3r_string.h"
#include "h3r_point.h"
#include "h3r_control.h"
#include "h3r_window.h"
#include "h3r_list.h"

H3R_NAMESPACE

// A very simple control for positioning text on screen.
#undef public
class Label final : public Control, public IHandleEvents
#define public public:
{
    private bool _ml {};
    private Point _mb {};
    private List<RenderEngine::TextKey> _tkeys {};
    private String _font;
    private String _text;
    private unsigned int _color {H3R_TEXT_COLOR_MSGB};
    private class ScrollBar * _vs {};
    private int _num_visible {};
    private int _font_h {};
    private int _last_tx {}, _last_ty {}; // used when _mb is present

    // text, font, location, manager, color, multi-line, multi-line box.
    // When mline, and the text is longer than the box height, a vscrollbar
    // shall be shown.
    // When not mline, but mbox is specified, the text shall be centered at it:
    //   box=(location,mbox) <- vertical and horizontal center at it
    public Label(
        const String &, const String &, const Point &, Control *,
        unsigned int = H3R_TEXT_COLOR_MSGB, bool = false, Point = Point {});
    public Label(
        const String &, const String &, const Point &, Window *,
        unsigned int = H3R_TEXT_COLOR_MSGB, bool = false, Point = Point {});
    public ~Label() override;
    public inline const String & Text() const { return _text; }
    public inline const String & Font() const { return _font; }
    public void SetText(const String & value);
    public void SetColor(unsigned int);

    private void SetText(); // used on init
    private void OnVisibilityChanged() override;
    private void UpdateVisible(); // on/off text keys based on _mb.Height
    private void HandleScroll(EventArgs *);
};// Label

NAMESPACE_H3R

#endif