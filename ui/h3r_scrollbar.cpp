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

// #include <math.h>

#include "h3r_scrollbar.h"
#include "h3r_window.h"
#include "h3r_def.h"
#include "h3r_game.h"
#include "h3r_log.h"

H3R_NAMESPACE

ScrollBar::~ScrollBar() {}

// ScrollBar set:
//   slidback.def: 007EE020, 007EEC10
//    - SlidBack 230x30 - a cece29 (yello-wish, index 253) rectangle, filled
//                            with 000000 (index 254)
//   slidknob.def: 007EE020, 007EEC10
//    - SlidKnob 30x30 - a cece29 (index 254) cube
//   SLIDBUH.def, SLIDEBUH.def: 007ED050, 007EC0A0
//    - ScnrBLfn, ScnrBLfs, ScnrBRtn, ScnrBRts, ScnrBSln 16x16
//   SLIDBUV.def, SLIDEBUV.def: 007ED050, 007EC0A0
//    - ScnrBUpn, ScnrBUps, ScnrBDnn, ScnrBDns, ScnrBSln 16x16
//
// Using these, since my Button is made handling sprites like them:
//   ScnrBDn.def - nsdh 16x16 : 007EEC10, 007EC120
//   ScnrBUp.def - nsdh 16x16 : 007EEC10, 007EC120
//   ScnrBLf.def - nsdh 16x16 : 007EEC10, 007EC120
//   ScnrBRt.def - nsdh 16x16 : 007EEC10, 007EC120
//   ScnrBSl.def - nsdh 16x16 : 007EEC10, 007EC120
//
// No idea why are all of the above duplicated; you need 1 arrow and 1 button.
//   SLIDBUH - duplicates SLIDEBUH, and {ScnrBLf, ScnrBRt, and ScnrBSl}
//   SLIDBUV - duplicates SLIDEBUV, and {ScnrBDn, ScnrBUp, and ScnrBSl}
//
// The scrollbars aren't using the d and h variants. the middle button is using
// n only.
// I see no reason loading "slidback" (what to do with that 230x30 yellow
// rectangle?!) and "slidknob".
void ScrollBar::Init(Point pos, int h)
{
    H3R_ENSURE (h > 0 && h < H3R_UI_MAX_VALUE, "Bug: suspicious pixel size")
    H3R_ENSURE (pos.X >= 0 && pos.X < H3R_UI_MAX_VALUE,
        "Bug: suspicious pixel size")
    H3R_ENSURE (pos.Y >= 0 && pos.Y < H3R_UI_MAX_VALUE,
        "Bug: suspicious pixel size")

    SetPosNoNotify (pos.X, pos.Y);

    Min.Changed.Subscribe (this, &ScrollBar::UpdateView);
    Max.Changed.Subscribe (this, &ScrollBar::UpdateView);
    SmallStep.Changed.Subscribe (this, &ScrollBar::UpdateView);
    LargeStep.Changed.Subscribe (this, &ScrollBar::UpdateView);
    Pos.Changed.Subscribe (this, &ScrollBar::UpdateView);

    // managed by the Window destructor via Add()
    H3R_CREATE_OBJECT(btn_dn, Button) { // TODO no sound fx
        "ScnrBDn.def", this, Button::H3R_UI_BTN_UPDN};
    _a = btn_dn->Width ();
    _t = pos.Y + _a;
    // This will be validated, and hidden, in case of inadequate h, so no need
    // to double-check here.
    H3R_ENSURE(btn_dn->Width () == btn_dn->Height (), "Quad art please")
    btn_dn->SetPos (pos.X, pos.Y + h - btn_dn->Height ());
    Resize (_a, h);
    btn_dn->UploadFrames ();
    btn_dn->Click.Subscribe (this, &ScrollBar::HandleScrollDown);

    H3R_CREATE_OBJECT(btn_up, Button) { // TODO no sound fx
        "ScnrBUp.def", this, Button::H3R_UI_BTN_UPDN};
    H3R_ENSURE(btn_up->Width () == btn_up->Height (), "Quad art please")
    H3R_ENSURE(btn_up->Width () == btn_dn->Width (), "Same size art please")
    btn_up->SetPos (pos.X, pos.Y);
    btn_up->UploadFrames ();
    btn_up->Click.Subscribe (this, &ScrollBar::HandleScrollUp);

    auto RE = Window::UI;
    static Array<byte> * bitmap {};
    auto bitmap_data = []() { return bitmap->operator byte * (); };
    Array<byte> backgnd {_a*(h-2*_a)*4}; // RGBA
    for (int i = 3; i < backgnd.Length (); i += 4) backgnd[i] = 255;
    bitmap = &backgnd;
    _key_b = RE->GenKey ();
    RE->UploadFrame (_key_b, pos.X, pos.Y + _a, _a, h-2*_a, bitmap_data,
        h3rBitmapFormat::RGBA, String::Format ("VSCrollBarBackgnd%dx%d", _a, h),
        Depth ());

    // No need to create a Button because I need to drag this one with the
    // mouse, and there is really no need to implement that for all static
    // controls, because this is the only mouse-drag-able one.
    Def sprite {Game::GetResource ("ScnrBSl.def")};
    H3R_ENSURE(sprite.Width () == sprite.Height (), "Quad art please")
    H3R_ENSURE(btn_up->Width () == sprite.Width (), "Same size art please")
    auto s1 = sprite.Query ("ScnrBSln.pcx");
    H3R_ENSURE(s1, "Sprite not found")
    bitmap = s1->ToRGBA ();
    H3R_ENSURE(bitmap, "Sprite->ToRGBA() failed")
    _key_m = RE->GenKey ();
    printf ("<> : m_pos: %d, %d\n", pos.X, _t);
    //TODO for some unknown to me reason this started rendering below _key_b?!
    //     e.g. the +1
    RE->UploadFrame (_key_m, pos.X, _t, _a, _a, bitmap_data,
        h3rBitmapFormat::RGBA, sprite.GetUniqueKey ("ScnrBSl.def"), Depth ()+1);
}

static bool the_way_is_shut {};

void ScrollBar::UpdateView(EventArgs *)
{
    static bool working {};
    H3R_ARG_EXC_IF(the_way_is_shut, "Don't do that")
    if (working) return; // filter-out re-entry caused by validation
    working = true;

    H3R_ARG_EXC_IF(Max < Min, "Be very careful when changing Min/Max")
    // Validate.
    if (Pos < Min) Pos = Min;
    if (Pos > Max) Pos = Max;
    if (LargeStep > (Max - Min)) LargeStep = (Max - Min);
    if (LargeStep < 1) LargeStep = 1;
    if (SmallStep > LargeStep) SmallStep = LargeStep / 10;
    if (SmallStep < 1) SmallStep = 1;

    Model2View ();

    working = false;
}

void ScrollBar::Model2View()
{
    auto RE = Window::UI;
    // At this game, the only thing to update is the Top of the middle button.
    int d1 = Size ().Height - 3 * _a; // up + dn + mid = 3 * a
    if (d1 < 0) { // nothing to render
        RE->ChangeVisibility (_key_m, false);
        RE->ChangeVisibility (_key_b, false);
    }
    else {
        RE->ChangeVisibility (_key_m, true);
        RE->ChangeVisibility (_key_b, true);
        int d2 = (Max - Min);
        if (0 == d2) {
            Log::Info ("Warning: pointless scrollbar: Min = Max" EOL);
            return;
        }
        int t = static_cast<int>(
            Control::Pos ().Y + _a + (Pos-Min) * (1.0*d1/d2));
        /*printf ("<> d1: %d, d2: %d, t:%d, _t:%d, pos: %d, min: %d\n",
            d1, d2, t, _t, (int)Pos, (int)Min);*/
        if (t != _t) {
            RE->UpdateLocation (_key_m, 0, t-_t);
            _t = t;
        }
    }
}// ScrollBar::Model2View()

void ScrollBar::NotifyOnScroll(int d)
{
    static EventArgs e {};
    the_way_is_shut = true;
    e.Delta = d;
    Scroll (&e);
    the_way_is_shut = false;
}

void ScrollBar::HandleScrollDown(EventArgs *)
{
    bool notify = Pos != Max;
    Pos = Pos + SmallStep; // Why using Property? - that's why
    if (notify) NotifyOnScroll (SmallStep);
}

void ScrollBar::HandleScrollUp(EventArgs *)
{
    bool notify = Pos != Min;
    Pos = Pos - SmallStep;
    if (notify) NotifyOnScroll (-SmallStep);
}

void ScrollBar::OnVisibilityChanged()
{
    H3R_ENSURE(btn_up && btn_dn, "bug: not initialized yet")
    auto RE = Window::UI;
    RE->ChangeVisibility (_key_b, ! Hidden ());
    RE->ChangeVisibility (_key_m, ! Hidden ());
}

void ScrollBar::OnMoved(int dx, int dy)
{
    H3R_ENSURE(btn_up && btn_dn, "bug: not initialized yet")
    auto RE = Window::UI;
    RE->UpdateLocation (_key_m, dx, dy);
    RE->UpdateLocation (_key_b, dx, dy);
    btn_up->SetPos (btn_up->Pos ().X + dx, btn_up->Pos ().Y + dy);
    btn_dn->SetPos (btn_dn->Pos ().X + dx, btn_dn->Pos ().Y + dy);
}

void ScrollBar::OnMouseDown(const EventArgs & e)
{
    Control::OnMouseDown (e);

    Point pos = Control::Pos (), size = Size ();
    if (e.X < pos.X || e.X > pos.X + _a) return;
    if (e.Y < pos.Y + _a || e.Y > pos.Y + size.Height - _a) return;
    if (e.Y < _t) {
        bool notify = Pos != Min;
        int p = Pos;
        Pos = Pos - LargeStep;
        if (notify) NotifyOnScroll (Pos-p);
    }
    else if (e.Y > _t+_a) {
        bool notify = Pos != Max;
        int p = Pos;
        Pos = Pos + LargeStep;
        if (notify) NotifyOnScroll (Pos-p);
    }
}

NAMESPACE_H3R
