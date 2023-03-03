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
#include "h3r_newgame.h"
#include "h3r_renderengine.h"
#include "h3r_textrenderingengine.h"
#include "h3r_array.h"
#include "h3r_game.h"
#include "h3r_def.h"
#include "h3r_pcx.h"
#include "h3r_label.h"
#include "h3r_button.h"

H3R_NAMESPACE

NewGameDialog::NewGameDialog(Window * base_window)
    : DialogWindow {base_window, Point {370, 585}}
{
    H3R_ENSURE(Window::MainWindow != nullptr,
        "NewGameDialog requires MainWindow")
    _t = 6;
    _l = 396;

    RenderEngine & re = *Window::UI;

    Pcx new_game_background {Game::GetResource ("gamselb1.pcx")};
    auto byte_arr_ptr = new_game_background.ToRGB ();
    if (! byte_arr_ptr || byte_arr_ptr->Empty ()) {
        H3R_NS::Log::Err ("Failed to load gamselb1.pcx" EOL);
        return;
    }
    static byte * bmp_data {};
    auto bitmap_data = []() { return bmp_data; };
    auto key = re.GenKey ();
    bmp_data = byte_arr_ptr->operator byte * ();
    re.UploadFrame (key, 0, 0, new_game_background.Width (),
        new_game_background.Height (), bitmap_data,
        h3rBitmapFormat::RGB, "gamselb1.pcx", Depth ());

    Pcx dlg_main {Game::GetResource ("GSelPop1.pcx")};
    byte_arr_ptr = dlg_main.ToRGBA ();
    if (! byte_arr_ptr || byte_arr_ptr->Empty ()) {
        H3R_NS::Log::Err ("Failed to load GSelPop1.pcx" EOL);
        return;
    }
    bmp_data = byte_arr_ptr->operator byte * ();
    key = re.GenKey ();
    re.UploadFrame (key, _l, _t, dlg_main.Width (),
        dlg_main.Height (), bitmap_data,
        h3rBitmapFormat::RGBA, "GSelPop1.pcx", Depth ());

    ScrollBar * test {};
    H3R_CREATE_OBJECT(test, ScrollBar) {this, Point {725, 155}, 115};

    // By default if shows info about the 1st available scenario. The default
    // difficulty is normal: GSPBUT4.DEF.

    // Set text color.
    // Gold: 238,214,123; White: 255,255,255
    // "Scenario Name:" smallfont.fnt, Gold, 422,31
    Window::UI->UploadText (_tkeys.Add (Window::UI->GenTextKey ()),
        "smalfont.fnt", "Scenario Name:", 422, 28, H3R_TEXT_COLOR_GOLD,
        Depth ());
    // Scenario Name: bigfont.fnt, Gold,     422,50
    Window::UI->UploadText (_tkeys.Add (Window::UI->GenTextKey ()),
        "bigfont.fnt", "Adventures", 422, 45, H3R_TEXT_COLOR_GOLD,
        Depth ());
    // "Show Available Scenarios": smallfont.fnt, White, 440,85(up), 441,86(dn)
    // "Random Map": smallfont.fnt, White, 475,109(up), 476,110(dn)
    // "Scenario Description:": smallfont.fnt, Gold, 422,141
    // Scenario Description: smallfont.fnt, White, 422,158; layout word-wrap
    // at x=739; after y=265 - show a scrollbar (word-wrap at 716):
    //  - weird behavior: if you came to an scroll-bar requiring entry, with
    //    "arrow down", the scroll-bar goes to its max. position; "arrow up":
    //    the scroll-bar appears set to its min. position
    //  - the scroll-bar scrolls lines of text, not pixels (ain't smooth)
    //  - the scroll-bar is vertical only
    //  - the scroll-bar resets to its min. position if you switch items with
    //    the mouse
    // Line spacing: ~5 pixels
    // "Victory Condition:": smallfont.fnt, Gold, 422,292
    // Victory Condition: smallfont.fnt, White, 456,316
    // "Loss Condition:": smallfont.fnt, Gold, 422,348
    // Loss Condition: smallfont.fnt, White, 456,375
    // "Allies:": smallfont.fnt, White, 420,409
    // "Enemies:": smallfont.fnt, White, 586,409
    // "Map Diff:": smallfont.fnt, Gold, 429,439
    // "Player Difficulty:": smallfont.fnt, Gold, 529,439
    // "Rating:": smallfont.fnt, Gold, 686,439
    // Map Diff: the text is centered at top=473,x=416;500
    // Rating: the text is centered at top=473,x=668;747
    // "Show Advanced Options:" 443,513 (these appear to be centered at the long
    // buttons)

    /*static Def * sprite_p {};
    auto bitmap_data = [](){ return sprite_p->ToRGBA ()->operator byte * (); };

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
    sprite.Query ("DiBoxT.pcx");
    for (int i = 0; i < tile_x; i++) {
        key = _re_keys.Add (re.GenKey ());
        re.UploadFrame (key, _l+dw+i*dw, _t, dw,
            dh, bitmap_data, h3rBitmapFormat::RGBA,
            sprite.GetUniqueKey ("dialgbox.def"), depth);
    }
    sprite.Query ("DiBoxB.pcx");
    for (int i = 0; i < tile_x; i++) {
        key = _re_keys.Add (re.GenKey ());
        re.UploadFrame (key, _l+dw+i*dw, _t+size.Y-dh, dw,
            dh, bitmap_data, h3rBitmapFormat::RGBA,
            sprite.GetUniqueKey ("dialgbox.def"), depth);
    }
    sprite.Query ("DiBoxL.pcx");
    for (int i = 0; i < tile_y; i++) {
        key = _re_keys.Add (re.GenKey ());
        re.UploadFrame (key, _l, _t+dh, dw,
            dh, bitmap_data, h3rBitmapFormat::RGBA,
            sprite.GetUniqueKey ("dialgbox.def"), depth);
    }
    sprite.Query ("DiBoxR.pcx");
    for (int i = 0; i < tile_y; i++) {
        key = _re_keys.Add (re.GenKey ());
        re.UploadFrame (key, _l+size.X-dw, _t+dh, dw,
            dh, bitmap_data, h3rBitmapFormat::RGBA,
            sprite.GetUniqueKey ("dialgbox.def"), depth);
    }
    sprite.Query ("DiBoxTL.pcx");
    key = _re_keys.Add (re.GenKey ());
    re.UploadFrame (key, _l, _t, dw, dh, bitmap_data, h3rBitmapFormat::RGBA,
        sprite.GetUniqueKey ("dialgbox.def"), depth);
    sprite.Query ("DiBoxBL.pcx");
    key = _re_keys.Add (re.GenKey ());
    re.UploadFrame (key, _l, _t+size.Y-dh, dw, dh,
        bitmap_data, h3rBitmapFormat::RGBA,
        sprite.GetUniqueKey ("dialgbox.def"), depth);
    sprite.Query ("DiBoxTR.pcx");
    key = _re_keys.Add (re.GenKey ());
    re.UploadFrame (key, _l+size.X-dw, _t, dw, dh,
        bitmap_data, h3rBitmapFormat::RGBA,
        sprite.GetUniqueKey ("dialgbox.def"), depth);
    sprite.Query ("DiBoxBR.pcx");
    key = _re_keys.Add (re.GenKey ());
    re.UploadFrame (key, _l+size.X-dw, _t+size.Y-dh, dw, dh,
        bitmap_data, h3rBitmapFormat::RGBA,
        sprite.GetUniqueKey ("dialgbox.def"), depth);

    // Text
    Label * lbl;
    // managed by the Window destructor via Add()
    H3R_CREATE_OBJECT(lbl, Label) {msg, fnt, Point {277,267}, this};

    // Buttons: "box66x32.pcx", iCANCEL.def, iOKAY.def
    // ok the above one is 68x34
    // This: Box64x30.pcx is 66x32
    static Pcx * pcx {};
    Pcx pcx_bmp {Game::GetResource ("Box64x30.pcx")};
    pcx = &pcx_bmp;
    auto pcx_bitmap = []() { return pcx->ToRGBA ()->operator byte * (); };
    key = _re_keys.Add (re.GenKey ());
    re.UploadFrame (key, 326, 334, pcx_bmp.Width (), pcx_bmp.Height (),
        pcx_bitmap, h3rBitmapFormat::RGBA, "Box64x30.pcx", depth);
    key = _re_keys.Add (re.GenKey ());
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
    btn_cancel->Click.Subscribe (this, &MessageBox::HandleCancelClick);*/
}

NewGameDialog::~NewGameDialog()
{
    for (int i = 0; i < _tkeys.Count (); i++)
        Window::UI->DeleteText (_tkeys[i]);
}

DialogResult NewGameDialog::ShowDialog()
{
    _dr = DialogResult::Cancel;
    while (! _has_dr && ! Closed ())
        Window::ActiveWindow->ProcessMessages ();
    bool allow_close = true;
    OnClose (this, allow_close);
    return _dr;
}

void NewGameDialog::OnKeyUp(const EventArgs & e)
{
    if (H3R_KEY_ESC == e.Key) {
        _dr = DialogResult::Cancel;
        _has_dr = true;
    }
}

/*void MessageBox::HandleBeginClick(EventArgs *)
{
    _dr = DialogResult::OK; _has_dr = true;
}

void MessageBox::HandleBackClick(EventArgs *)
{
    _dr = DialogResult::Cancel; _has_dr = true;
}*/

NAMESPACE_H3R
