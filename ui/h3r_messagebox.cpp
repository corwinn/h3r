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

#include <new>

H3R_NAMESPACE
static int const MB_SPRITES {100};

static int Align(int value, int boundary)
{
    if (! (value % boundary)) return value;
    return (value / boundary + 1) * boundary;
}

// Copy into dst:0,0,w,h from src:0,0,w,h. Used for tiling.
static void CopyRectangleRGB(
    byte * dst, const byte * src, int src_w, int w, int h)
{
    for (int r = 0; r < h; r++) {
        OS::Memcpy (dst, src, 3*w);
        dst += 3*w; src += 3*src_w;
    }
}

MessageBox::MessageBox(Window * base_window, Point && size,
    const String & msg, const String & fnt, MessageBox::Buttons btn)
    : DialogWindow {base_window, static_cast<Point &&>(size)},
    _btn_re {6}
    //, _re {MB_SPRITES}
{
    H3R_ENSURE(Window::MainWindow != nullptr, "MessageBox requires MainWindow")
    _t = (Window::MainWindow->GetSize ().Y - size.Y) / 2;
    _l = (Window::MainWindow->GetSize ().X - size.X) / 2;
    // printf ("top, left = %d, %d" EOL, _t, _l);

    RenderEngine & re = RenderEngine::UI ();

    //TODO there is a lot to think about:

    // DropShadow
    const int DS_SIZE {8};
    Array<uint> b1 {size.X*DS_SIZE};
    uint * b1_ptr = b1;
    for (int y = 0; y < DS_SIZE; y++)
        for (int x = 0; x < size.X; x++)
            if ((DS_SIZE-1) == y || 0 == x || (size.X-1) == x)
            // these two and glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            // closely match the original
                 b1_ptr[y*size.X + x] = 0x82000000u;
            else b1_ptr[y*size.X + x] = 0xc2000000u;
    auto key = _re_keys.Add (re.GenKey ());
    re.UploadFrame (key, _l+DS_SIZE, _t+size.Y, size.X,
        DS_SIZE, reinterpret_cast<byte *>(b1_ptr), 4,
        String::Format ("%d%dMessageBoxShadowBottom", size.X, DS_SIZE));
    H3R_ENSURE(size.Y > DS_SIZE, "MessageBoxes require shadow")
    int s2_h = size.Y-DS_SIZE; // printf ("s2_h: %d" EOL, s2_h);
    b1.Resize (s2_h*DS_SIZE);
    b1_ptr = b1;
    for (int y = 0; y < s2_h; y++)
        for (int x = 0; x < DS_SIZE; x++)
            if (0 == y || (DS_SIZE-1) == x)
            // these two and glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            // closely match the original
                 b1_ptr[y*DS_SIZE + x] = 0x82000000u;
            else b1_ptr[y*DS_SIZE + x] = 0xc2000000u;
    key = _re_keys.Add (re.GenKey ());
    re.UploadFrame (key, _l+size.X, _t+DS_SIZE, DS_SIZE,
        s2_h, reinterpret_cast<byte *>(b1_ptr), 4,
        String::Format ("%d%dMessageBoxShadowRight", DS_SIZE, s2_h));

    // Backround; 256x256 on 320x192
    Pcx dlg_back {Game::GetResource ("DiBoxBck.pcx")};
    auto dlg_back_arr = dlg_back.ToRGB ();
    int tx = Align (size.X, dlg_back.Width ()) / dlg_back.Width (),
        ty = Align (size.Y, dlg_back.Height ()) / dlg_back.Height ();
    // printf ("Background, tiles: tx: %d, ty: %d" EOL, tx, ty);
    for (int i = 0, ay = size.Y; i < ty; i++, ay -= dlg_back.Height ())
        for (int j = 0, ax = size.X; j < tx; j++, ax -= dlg_back.Width ()) {
            int tw = dlg_back.Width () < ax ? dlg_back.Width () : ax;
            int th = dlg_back.Height () < ay ? dlg_back.Height () : ay;
            int xn = _l+j*dlg_back.Width (), yn = _t+i*dlg_back.Height ();
            Array<byte> tmp_buf {tw*th*3};
            CopyRectangleRGB (
                tmp_buf.operator byte * (), dlg_back_arr->begin (),
                dlg_back.Width (), tw, th);
            // printf ("tile [%d,%d]: x:%d, y:%d, tw: %d, th: %d" EOL, i, j,
            //    xn, yn, tw, th);
            key = _re_keys.Add (re.GenKey ());
            re.UploadFrame (key, xn, yn, tw, th, tmp_buf.operator byte * (), 3,
                String::Format ("DiBoxBck%d%d.pcx", tw, th));
        }

    // Decoration
    Def sprite {Game::GetResource ("dialgbox.def")};
    int dw = sprite.Width (), dh = sprite.Height ();
    // printf ("deco size: %d %d" EOL, dw, dh);
    int tile_x = (size.X - 2*dw), tile_y = (size.Y - 2*dh);
    H3R_ENSURE(tile_x > 0 && tile_y > 0, "Can't decorate")
    tile_x = Align (tile_x, dw) / dw;
    tile_y = Align (tile_y, dh) / dh;
    auto frame = sprite.Query ("DiBoxT.pcx");
    auto frame_arr = frame->ToRGBA ();
    for (int i = 0; i < tile_x; i++) {
        key = _re_keys.Add (re.GenKey ());
        re.UploadFrame (key, _l+dw+i*dw, _t, dw,
            dh, frame_arr->operator byte * (), 4,
            sprite.GetUniqueKey ("dialgbox.def"));
    }
    frame = sprite.Query ("DiBoxB.pcx");
    frame_arr = frame->ToRGBA ();
    for (int i = 0; i < tile_x; i++) {
        key = _re_keys.Add (re.GenKey ());
        re.UploadFrame (key, _l+dw+i*dw, _t+size.Y-dh, dw,
            dh, frame_arr->operator byte * (), 4,
            sprite.GetUniqueKey ("dialgbox.def"));
    }
    frame = sprite.Query ("DiBoxL.pcx");
    frame_arr = frame->ToRGBA ();
    for (int i = 0; i < tile_y; i++) {
        key = _re_keys.Add (re.GenKey ());
        re.UploadFrame (key, _l, _t+dh, dw,
            dh, frame_arr->operator byte * (), 4,
            sprite.GetUniqueKey ("dialgbox.def"));
    }
    frame = sprite.Query ("DiBoxR.pcx");
    frame_arr = frame->ToRGBA ();
    for (int i = 0; i < tile_y; i++) {
        key = _re_keys.Add (re.GenKey ());
        re.UploadFrame (key, _l+size.X-dw, _t+dh, dw,
            dh, frame_arr->operator byte * (), 4,
            sprite.GetUniqueKey ("dialgbox.def"));
    }
    frame = sprite.Query ("DiBoxTL.pcx");
    frame_arr = frame->ToRGBA ();
    key = _re_keys.Add (re.GenKey ());
    re.UploadFrame (key, _l, _t, dw, dh, frame_arr->operator byte * (), 4,
        sprite.GetUniqueKey ("dialgbox.def"));
    frame = sprite.Query ("DiBoxBL.pcx");
    frame_arr = frame->ToRGBA ();
    key = _re_keys.Add (re.GenKey ());
    re.UploadFrame (key, _l, _t+size.Y-dh, dw, dh,
        frame_arr->operator byte * (), 4, sprite.GetUniqueKey ("dialgbox.def"));
    frame = sprite.Query ("DiBoxTR.pcx");
    frame_arr = frame->ToRGBA ();
    key = _re_keys.Add (re.GenKey ());
    re.UploadFrame (key, _l+size.X-dw, _t, dw, dh,
        frame_arr->operator byte * (), 4, sprite.GetUniqueKey ("dialgbox.def"));
    frame = sprite.Query ("DiBoxBR.pcx");
    frame_arr = frame->ToRGBA ();
    key = _re_keys.Add (re.GenKey ());
    re.UploadFrame (key, _l+size.X-dw, _t+size.Y-dh, dw, dh,
        frame_arr->operator byte * (), 4, sprite.GetUniqueKey ("dialgbox.def"));

    // Text
    Label * lbl;
    // managed by the Window destructor via Add()
    H3R_CREATE_OBJECT(lbl, Label) {msg, fnt, Point {277,267}};
    Add (lbl);

    // Buttons: "box66x32.pcx", iCANCEL.def, iOKAY.def
    // ok the above one is 68x34
    // This: Box64x30.pcx is 66x32
    Pcx btn_border {Game::GetResource ("Box64x30.pcx")};
    auto btn_border_arr = btn_border.ToRGB ();
    key = _re_keys.Add (re.GenKey ());
    re.UploadFrame (key, 326, 334, btn_border.Width (), btn_border.Height (),
        btn_border_arr->operator byte * (), 3, "Box64x30.pcx");
    key = _re_keys.Add (re.GenKey ());
    re.UploadFrame (key, 409, 334, btn_border.Width (), btn_border.Height (),
        btn_border_arr->operator byte * (), 3, "Box64x30.pcx");
    Button * btn_ok {}, * btn_cancel {};
    // managed by the Window destructor via Add()
    H3R_CREATE_OBJECT(btn_ok, Button) {"iOKAY.def"};
    Add (btn_ok->SetPos (327, 335));
    btn_ok->UploadFrames (&_btn_re);
    H3R_CREATE_OBJECT(btn_cancel, Button) {"iCANCEL.def"};
    Add (btn_cancel->SetPos (410, 335));
    btn_cancel->UploadFrames (&_btn_re);
}

/*static*/ DialogResult MessageBox::Show(
    const String & msg, const String & fnt, MessageBox::Buttons btn)
{
    if (! Window::ActiveWindow) {
        return DialogResult::Cancel;
    }

    //TODO it somehow converts text to message box size
    Point ts = TextRenderingEngine::One ().MeasureText (fnt, msg);
    printf ("Text size: %d %d" EOL, ts.X, ts.Y);
    // my Text size: 233 20; measured: 232 x 20
    // The quit dialog box is 320 x 192 w/o the drop shadow;
    // located at 240,204 - that's centered w/o the drop-shadow.
    // The text starts at 277,267.
    // 240+(320-233)/2 = 283 - padding = 6
    // I looks like there are two horizontal boxes: 1 has the text; the other:
    // the buttons; the buttons one has 15 v-padding; their height is 32 with
    // the border; so the top rectangle should be 204-62=142 pixels; lets
    // vcenter the text in it: 265 - that's -2 padding;
    // It looks like this was done manually on hand; I can find no auto-layout
    // yet.

    MessageBox * msgbox;
    H3R_CREATE_OBJECT(msgbox, MessageBox) {
        Window::ActiveWindow, Point {320, 192}, msg, fnt, btn};
    return msgbox->ShowDialog ();
}

void MessageBox::OnRender()
{
    Window::OnRender ();
    /*glPushAttrib (GL_ENABLE_BIT | GL_CURRENT_BIT);
    glDisable (GL_ALPHA_TEST);
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);*/
    _btn_re.Render ();
    /*glPopAttrib ();*/
}

DialogResult MessageBox::ShowDialog()
{
    _dr = DialogResult::Cancel;
    while (! _has_dr && ! Closed ())
        Window::ActiveWindow->ProcessMessages ();
    bool allow_close = true;
    OnClose (this, allow_close);
    // Hide
    for (auto & k : _re_keys) RenderEngine::UI ().ChangeVisibility (k, false);

    return _dr;
}

void MessageBox::OnKeyUp(const EventArgs & e)
{
    if (H3R_KEY_ESC == e.Key) {
        _dr = DialogResult::Cancel;
        _has_dr = true;
    }
}

NAMESPACE_H3R
