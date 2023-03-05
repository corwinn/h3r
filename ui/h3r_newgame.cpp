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

    auto RE = Window::UI;

    Pcx new_game_background {Game::GetResource ("gamselb1.pcx")};
    auto byte_arr_ptr = new_game_background.ToRGB ();
    if (! byte_arr_ptr || byte_arr_ptr->Empty ()) {
        H3R_NS::Log::Err ("Failed to load gamselb1.pcx" EOL);
        return;
    }
    static byte * bmp_data {};
    auto bitmap_data = []() { return bmp_data; };
    auto key = RE->GenKey ();
    bmp_data = byte_arr_ptr->operator byte * ();
    RE->UploadFrame (key, 0, 0, new_game_background.Width (),
        new_game_background.Height (), bitmap_data,
        h3rBitmapFormat::RGB, "gamselb1.pcx", Depth ());

    Pcx dlg_main {Game::GetResource ("GSelPop1.pcx")};
    byte_arr_ptr = dlg_main.ToRGBA ();
    if (! byte_arr_ptr || byte_arr_ptr->Empty ()) {
        H3R_NS::Log::Err ("Failed to load GSelPop1.pcx" EOL);
        return;
    }
    bmp_data = byte_arr_ptr->operator byte * ();
    key = RE->GenKey ();
    RE->UploadFrame (key, _l, _t, dlg_main.Width (),
        dlg_main.Height (), bitmap_data,
        h3rBitmapFormat::RGBA, "GSelPop1.pcx", Depth ());

    // By default if shows info about the 1st available scenario. The default
    // difficulty is normal: GSPBUT4.DEF.

    // Set text color.
    // Gold: 238,214,123; White: 255,255,255
    // "Scenario Name:" smallfont.fnt, Gold, 422,31
    Label * lbl {};
    H3R_CREATE_OBJECT(lbl, Label) {"Scenario Name:", "smalfont.fnt",
        Point {422, 28}, this, H3R_TEXT_COLOR_GOLD};

    //TODO property; Scenario Name: bigfont.fnt, Gold,     422,50
    H3R_CREATE_OBJECT(lbl, Label) {"H3R", "bigfont.fnt",
        Point {422, 45}, this, H3R_TEXT_COLOR_GOLD};

    //TODO property; Map Size Icon
    static Array<byte> * bitmap {};
    Def sprite {Game::GetResource ("ScnrMpSz.def")};
    auto s1 = sprite.Query ("Scnr144z.pcx");
    H3R_ENSURE(s1, "Sprite not found")
    bitmap = s1->ToRGBA ();
    H3R_ENSURE(bitmap, "Sprite->ToRGBA() failed")
    bmp_data = bitmap->operator byte * ();
    RE->UploadFrame (RE->GenKey (), 714, 28, sprite.Width (), sprite.Height (),
        bitmap_data,
        h3rBitmapFormat::RGBA, sprite.GetUniqueKey ("Scnr144z.pcx"), Depth ());

    // "Show Available Scenarios": smalfont.fnt, White, 440,85(up), 441,86(dn)
    Button * tab_btn_ascen {};
    H3R_CREATE_OBJECT(tab_btn_ascen, Button) {
        "GSPBUTT.DEF", this, Button::H3R_UI_BTN_UPDN};
    tab_btn_ascen->SetPos (414, 81);
    tab_btn_ascen->UploadFrames ();
    tab_btn_ascen->Click.Subscribe (this, &NewGameDialog::ToggleAvailScen);
    tab_btn_ascen->SetText ("Show Available Scenarios", "smalfont.fnt",
        H3R_TEXT_COLOR_WHITE);

    // "Random Map": smalfont.fnt, White, 475,109(up), 476,110(dn)
    Button * tab_btn_rnd_scen {};
    H3R_CREATE_OBJECT(tab_btn_rnd_scen, Button) {
        "GSPBUTT.DEF", this, Button::H3R_UI_BTN_UPDN};
    tab_btn_rnd_scen->SetPos (414, 105);
    tab_btn_rnd_scen->UploadFrames ();
    tab_btn_rnd_scen->Click.Subscribe (this, &NewGameDialog::ToggleRndScen);
    tab_btn_rnd_scen->SetText ("Random Map", "smalfont.fnt",
        H3R_TEXT_COLOR_WHITE);

    // "Scenario Description:": smalfont.fnt, Gold, 422,141
    H3R_CREATE_OBJECT(lbl, Label) {"Scenario Description:", "smalfont.fnt",
        Point {422, 138}, this, H3R_TEXT_COLOR_GOLD};

    // Scenario Description: smalfont.fnt, White, 422, 155; layout word-wrap
    // at x=740; after y=269. I won't reproduce the weird behavior of the
    // original, like word-wrapping with imaginary scroll-bar; and scroll-bar
    // jumping to max position on item switching.
    H3R_CREATE_OBJECT(lbl, Label) {
        "A remake to be of the game engine of the well-known computer game "
        "\"Heroes of Might and Magic III\" and its official expansions.",
        "smalfont.fnt", Point {422, 155}, this, H3R_TEXT_COLOR_WHITE, true,
        Point {741-422, 270-155}};

    // "Victory Condition:": smalfont.fnt, Gold, 422,292
    // Victory Condition: smalfont.fnt, White, 456,316
    // "Loss Condition:": smalfont.fnt, Gold, 422,348
    // Loss Condition: smalfont.fnt, White, 456,375
    // "Allies:": smalfont.fnt, White, 420,409
    // "Enemies:": smalfont.fnt, White, 586,409
    // "Map Diff:": smalfont.fnt, Gold, 429,439
    // "Player Difficulty:": smalfont.fnt, Gold, 529,439
    // "Rating:": smalfont.fnt, Gold, 686,439
    // Map Diff: the text is centered at top=473,x=416;500
    // Rating: the text is centered at top=473,x=668;747
    // "Show Advanced Options:" 443,513 (these appear to be centered at the long
    // buttons)
}// NewGameDialog::NewGameDialog()

NewGameDialog::~NewGameDialog() {}

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

/*void NewGameDialog::HandleBeginClick(EventArgs *)
{
    _dr = DialogResult::OK; _has_dr = true;
}

void NewGameDialog::HandleBackClick(EventArgs *)
{
    _dr = DialogResult::Cancel; _has_dr = true;
}*/

void NewGameDialog::ToggleAvailScen(EventArgs *)
{
}

void NewGameDialog::ToggleRndScen(EventArgs *)
{
}

NAMESPACE_H3R
