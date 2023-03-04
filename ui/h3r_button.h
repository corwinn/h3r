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

#ifndef _H3R_BUTTON_H_
#define _H3R_BUTTON_H_

#include "h3r.h"
#include "h3r_string.h"
#include "h3r_point.h"
#include "h3r_control.h"
#include "h3r_eventargs.h"
#include "h3r_event.h"
#include "h3r_label.h"

//  The code here did depend on the role of the Def parser. Wrong design.
// To decouple them: Button shall store copies of the bitmaps it is using.

H3R_NAMESPACE

// Its size is stored at the resource it is created from.
#undef public
class Button: public Control
#define public public:
{
    public enum {
        H3R_UI_BTN_UP      = 1,
        H3R_UI_BTN_DOWN    = 2,
        H3R_UI_BTN_HOVER   = 4,
        H3R_UI_BTN_GRAYOUT = 8,
        H3R_UI_BTN_DEFAULT = 7,
        H3R_UI_BTN_UPDN    = 3};
    private int _rkey {};
    private int _on {}; // up
    private int _oh {}; // hover
    private int _os {}; // down
    private int _od {}; // grayed out
    public struct TempSpriteData final // from Button() to UploadFrames()
    {
        Array<byte> Bitmap {};
        Point TopLeft {};
        String UniqueKey {};
    };
    private TempSpriteData * _tsdn{}, * _tsdh{}, * _tsds{}, * _tsdd{};
    private void Init(const String &, int flags);

    public Button(const String &, Control *, int = H3R_UI_BTN_DEFAULT);
    public Button(const String &, Window *, int = H3R_UI_BTN_DEFAULT);
    public virtual ~Button() override;

    public virtual Control * SetPos(int, int) override;
    public void UploadFrames() override;

    protected virtual void OnMouseMove(const EventArgs &) override;
    protected virtual void OnMouseDown(const EventArgs &) override;
    protected virtual void OnMouseUp(const EventArgs &) override;
    protected virtual void OnVisibilityChanged() override;

    private bool _mouse_over {};
    private bool _mouse_down {};

    // Usage: Click.Subscribe (this, &descendant_of_IHandleEvents::handler)
    public Event Click {};

    // label and label_down: it moves 1 pixel down, when I click the button
    private Label * _lbl {}, * _lbl_dn {};
    // Calling it a second time will ignore all parameters, except the 1st one.
    // Again: this is not a general use UI framework; should you happen to want
    // to design one, look at the ".NET" "Windows.Forms" or the "Delphi" "VCL"!
    // Usually all Controls have a Text/Name property, an independent Font
    // property, Fore/Back-ground color/fill/brush/bla-bla properties; the
    // controls have design/runtime state * tons of other state, etc. Designing
    // a general-use UI framework is pointless, certainly not the point of this
    // code, and so far the result is slow, heavy, unmanageable mess, with ages
    // of docs to read.
    // Don't waste your time writing frameworks, craft programs instead.
    public void SetText(const String &, const String &, unsigned int);
};// Button

NAMESPACE_H3R

#endif