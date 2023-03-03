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

#include "h3r.h"
#include "h3r_messagebox.h"
#include "h3r_renderengine.h"
#include "h3r_textrenderingengine.h"
#include "h3r_array.h"
#include "h3r_game.h"
#include "h3r_def.h"
#include "h3r_pcx.h"
#include "h3r_label.h"
#include "h3r_button.h"

H3R_NAMESPACE

static int Align(int value, int boundary)
{
    if (! (value % boundary)) return value;
    return (value / boundary + 1) * boundary;
}

MessageBox::MessageBox(Window * base_window, Point && size,
    const String & msg, const String & fnt, MessageBox::Buttons btn)
    : DialogWindow {base_window, static_cast<Point &&>(size)}
{
    (void)btn;
    H3R_ENSURE(Window::MainWindow != nullptr, "MessageBox requires MainWindow")
    _t = (Window::MainWindow->GetSize ().Y - size.Y) / 2;
    _l = (Window::MainWindow->GetSize ().X - size.X) / 2;
    // printf ("top, left = %d, %d" EOL, _t, _l);

    RenderEngine & re = *Window::UI;

    //TODO there is a lot to think about:

    // The better message box.
    Pcx dlg_back {Game::GetResource ("DiBoxBck.pcx")};
    auto dlg_back_arr = dlg_back.ToRGBA (); // Ensure GL_UNPACK_ALIGNMENT
    H3R_ENSURE(dlg_back_arr, "Bitmap not found")
    re.ShadowRectangle (_l, _t, size.X, size.Y,
        dlg_back_arr->operator byte * (), h3rBitmapFormat::RGBA,
        dlg_back.Width (), dlg_back.Height (), Depth ());

    static Def * sprite_p {};
    auto bitmap_data = []()
    {
        auto result = sprite_p->ToRGBA ();
        H3R_ENSURE(result, "Sprite not found")
        return result->operator byte * ();
    };

    // Decoration
    int key;
    // using the buttons depth, because the big UI VBO is rendered prior
    // everything else
    int depth = Depth () + 1;
    Pal pp {Game::GetResource ("PLAYERS.PAL")};
    Def sprite {Game::GetResource ("dialgbox.def")};
    sprite_p = &sprite;
    sprite.SetPlayerColor (Game::CurrentPlayerColor, pp);
    int dw = sprite.Width (), dh = sprite.Height ();
    // printf ("deco size: %d %d" EOL, dw, dh);
    int tile_x = (size.X - 2*dw), tile_y = (size.Y - 2*dh);
    H3R_ENSURE(tile_x > 0 && tile_y > 0, "Can't decorate")
    tile_x = Align (tile_x, dw) / dw;
    tile_y = Align (tile_y, dh) / dh;
    H3R_ENSURE(sprite.Query ("DiBoxT.pcx"), "Sprite not found")
    for (int i = 0; i < tile_x; i++) {
        key = re.GenKey ();
        re.UploadFrame (key, _l+dw+i*dw, _t, dw,
            dh, bitmap_data, h3rBitmapFormat::RGBA,
            sprite.GetUniqueKey ("dialgbox.def"), depth);
    }
    H3R_ENSURE(sprite.Query ("DiBoxB.pcx"), "Sprite not found")
    for (int i = 0; i < tile_x; i++) {
        key = re.GenKey ();
        re.UploadFrame (key, _l+dw+i*dw, _t+size.Y-dh, dw,
            dh, bitmap_data, h3rBitmapFormat::RGBA,
            sprite.GetUniqueKey ("dialgbox.def"), depth);
    }
    H3R_ENSURE(sprite.Query ("DiBoxL.pcx"), "Sprite not found")
    for (int i = 0; i < tile_y; i++) {
        key = re.GenKey ();
        re.UploadFrame (key, _l, _t+dh, dw,
            dh, bitmap_data, h3rBitmapFormat::RGBA,
            sprite.GetUniqueKey ("dialgbox.def"), depth);
    }
    H3R_ENSURE(sprite.Query ("DiBoxR.pcx"), "Sprite not found")
    for (int i = 0; i < tile_y; i++) {
        key = re.GenKey ();
        re.UploadFrame (key, _l+size.X-dw, _t+dh, dw,
            dh, bitmap_data, h3rBitmapFormat::RGBA,
            sprite.GetUniqueKey ("dialgbox.def"), depth);
    }
    H3R_ENSURE(sprite.Query ("DiBoxTL.pcx"), "Sprite not found")
    key = re.GenKey ();
    re.UploadFrame (key, _l, _t, dw, dh, bitmap_data, h3rBitmapFormat::RGBA,
        sprite.GetUniqueKey ("dialgbox.def"), depth);
    H3R_ENSURE(sprite.Query ("DiBoxBL.pcx"), "Sprite not found")
    key = re.GenKey ();
    re.UploadFrame (key, _l, _t+size.Y-dh, dw, dh,
        bitmap_data, h3rBitmapFormat::RGBA,
        sprite.GetUniqueKey ("dialgbox.def"), depth);
    H3R_ENSURE(sprite.Query ("DiBoxTR.pcx"), "Sprite not found")
    key = re.GenKey ();
    re.UploadFrame (key, _l+size.X-dw, _t, dw, dh,
        bitmap_data, h3rBitmapFormat::RGBA,
        sprite.GetUniqueKey ("dialgbox.def"), depth);
    H3R_ENSURE(sprite.Query ("DiBoxBR.pcx"), "Sprite not found")
    key = re.GenKey ();
    re.UploadFrame (key, _l+size.X-dw, _t+size.Y-dh, dw, dh,
        bitmap_data, h3rBitmapFormat::RGBA,
        sprite.GetUniqueKey ("dialgbox.def"), depth);

    // Text
    Label * lbl;
    // managed by the Window destructor via Add()
    H3R_CREATE_OBJECT(lbl, Label) {msg, fnt, Point {277, 267}, this};

    // Buttons: "box66x32.pcx", iCANCEL.def, iOKAY.def
    // ok the above one is 68x34
    // This: Box64x30.pcx is 66x32
    static Pcx * pcx {};
    Pcx pcx_bmp {Game::GetResource ("Box64x30.pcx")};
    pcx = &pcx_bmp;
    auto pcx_bitmap = []()
    {
        auto result = pcx->ToRGBA ();
        H3R_ENSURE(result, "Bitmap not found")
        return result->operator byte * ();
    };
    key = re.GenKey ();
    re.UploadFrame (key, 326, 334, pcx_bmp.Width (), pcx_bmp.Height (),
        pcx_bitmap, h3rBitmapFormat::RGBA, "Box64x30.pcx", depth);
    key = re.GenKey ();
    re.UploadFrame (key, 409, 334, pcx_bmp.Width (), pcx_bmp.Height (),
        pcx_bitmap, h3rBitmapFormat::RGBA, "Box64x30.pcx", depth);
    Button * btn_ok {}, * btn_cancel {};
    // managed by the Window destructor via Add()
    H3R_CREATE_OBJECT(btn_ok, Button) {
        "iOKAY.def", this, Button::H3R_UI_BTN_UPDN};
    btn_ok->SetPos (327, 335);
    btn_ok->UploadFrames ();
    btn_ok->Click.Subscribe (this, &MessageBox::HandleOKClick);
    H3R_CREATE_OBJECT(btn_cancel, Button) {
        "iCANCEL.def", this, Button::H3R_UI_BTN_UPDN};
    btn_cancel->SetPos (410, 335);
    btn_cancel->UploadFrames ();
    btn_cancel->Click.Subscribe (this, &MessageBox::HandleCancelClick);
}

MessageBox::~MessageBox()
{
    Window::UI->DeleteShadowRectangle ();
}

/*static*/ DialogResult MessageBox::Show(
    const String & msg, const String & fnt, MessageBox::Buttons btn)
{
    (void)btn; //TODO OK only, or OKCancel, etc.
    if (! Window::ActiveWindow) {
        return DialogResult::Cancel;
    }

    //TODO it somehow converts text to message box size; there are max w,h;
    // when max_w is reached: partition the text by inserting new lines; when
    // max_h is reached: add a scroll bar, and re-partition the text to the
    // reduced width. Its possible it tries to keep the message box aspect ratio
    // close to 1 (sqrt(text.width)).
    Point ts = TextRenderingEngine::One ().MeasureText (fnt, msg);
    printf ("Text size: %d %d" EOL, ts.X, ts.Y);
    // My Text size: 233 20; measured from a screen-shot: 232 x 20.
    // The text starts at 277,267.
    // 240+(320-233)/2 = 283 - left padding = 6.
    // I looks like there are two horizontal boxes: 1 has the text; the other:
    // the buttons; the buttons one has 15 v-padding; their height is 32 with
    // the border; so the top rectangle should be 204-62=142 pixels; lets
    // vcenter the text in it: 265 - that's -2 padding.
    // It looks like this was done manually on hand. I can't deduce auto-layout
    // yet.

    MessageBox msgbox {Window::ActiveWindow, Point {320, 192}, msg, fnt, btn};
    return msgbox.ShowDialog ();
}

DialogResult MessageBox::ShowDialog()
{
    _dr = DialogResult::Cancel;
    while (! _has_dr && ! Closed ())
        Window::ActiveWindow->ProcessMessages ();
    bool allow_close = true;
    OnClose (this, allow_close);
    return _dr;
}

void MessageBox::OnKeyUp(const EventArgs & e)
{
    if (H3R_KEY_ESC == e.Key) {
        _dr = DialogResult::Cancel;
        _has_dr = true;
    }
}

void MessageBox::HandleOKClick(EventArgs *)
{
    _dr = DialogResult::OK; _has_dr = true;
}

void MessageBox::HandleCancelClick(EventArgs *)
{
    _dr = DialogResult::Cancel; _has_dr = true;
}

NAMESPACE_H3R
