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

    //TODO Property; Scenario Name: bigfont.fnt, Gold,     422,50
    H3R_CREATE_OBJECT(lbl, Label) {"H3R", "bigfont.fnt",
        Point {422, 45}, this, H3R_TEXT_COLOR_GOLD};

    //TODO Property; Map Size Icon
    static Array<byte> * bitmap {};
    Def sprite {Game::GetResource ("ScnrMpSz.def")};
    auto s1 = sprite.Query ("Scnr144z.pcx");
    H3R_ENSURE(s1, "Sprite not found")
    bitmap = s1->ToRGBA ();
    H3R_ENSURE(bitmap, "Sprite->ToRGBA() failed")
    bmp_data = bitmap->operator byte * ();
    RE->UploadFrame (RE->GenKey (), 714, 28, sprite.Width (), sprite.Height (),
        bitmap_data,
        h3rBitmapFormat::RGBA, sprite.GetUniqueKey ("ScnrMpSz.def"), Depth ());

    // "Show Available Scenarios": smalfont.fnt, White, 440,85(up), 441,86(dn)
    Button * btn {};
    H3R_CREATE_OBJECT(btn, Button) {
        "GSPBUTT.DEF", this, Button::H3R_UI_BTN_UPDN};
    btn->SetPos (414, 81);
    btn->UploadFrames ();
    btn->Click.Subscribe (this, &NewGameDialog::ToggleAvailScen);
    btn->SetText ("Show Available Scenarios", "smalfont.fnt",
        H3R_TEXT_COLOR_WHITE);

    // "Random Map": smalfont.fnt, White, 475,109(up), 476,110(dn)
    H3R_CREATE_OBJECT(btn, Button) {
        "GSPBUTT.DEF", this, Button::H3R_UI_BTN_UPDN};
    btn->SetPos (414, 105);
    btn->UploadFrames ();
    btn->Click.Subscribe (this, &NewGameDialog::ToggleRndScen);
    btn->SetText ("Random Map", "smalfont.fnt",
        H3R_TEXT_COLOR_WHITE);

    // "Scenario Description:": smalfont.fnt, Gold, 422,141
    H3R_CREATE_OBJECT(lbl, Label) {"Scenario Description:", "smalfont.fnt",
        Point {422, 138}, this, H3R_TEXT_COLOR_GOLD};

    // Scenario Description: smalfont.fnt, White, 422, 155; layout word-wrap
    // at x=740; after y=269. I won't reproduce the weird behavior of the
    // original, like word-wrapping with imaginary scroll-bar; and scroll-bar
    // jumping to max position on item switching.
    //TODO Property
    H3R_CREATE_OBJECT(lbl, Label) {
        "A remake to be of the game engine of the well-known computer game "
        "\"Heroes of Might and Magic III\" and its official expansions.",
        "smalfont.fnt", Point {422, 155}, this, H3R_TEXT_COLOR_WHITE, true,
        Point {741-422, 270-155}};

    // "Victory Condition:": smalfont.fnt, Gold, 422,292
    H3R_CREATE_OBJECT(lbl, Label) {"Victory Condition:", "smalfont.fnt",
        Point {422, 289}, this, H3R_TEXT_COLOR_GOLD};
    //TODO Property (bitmap+text)
    // victory condition mini-icon: SCNRVICT.def (w:  29, h:  21):
    //  name[ 0]: "ScnrVLaa.pcx" - a artifact
    //  name[ 1]: "ScnrVLac.pcx" - a creatures
    //  name[ 2]: "ScnrVLar.pcx" - a resources
    //  name[ 3]: "ScnrVLut.pcx" - up town
    //  name[ 4]: "ScnrVLbg.pcx" - grail
    //  name[ 5]: "ScnrVLdh.pcx" - d hero
    //  name[ 6]: "ScnrVLct.pcx" - capture town
    //  name[ 7]: "ScnrVLdm.pcx" - d monster
    //  name[ 8]: "ScnrVLfg.pcx" - flag cre dwe
    //  name[ 9]: "ScnrVLfm.pcx" - flag mines
    //  name[10]: "ScnrVLta.pcx" - transport art
    //  name[11]: "ScnrVLwn.pcx" - defeat all
    // They all have offset of t,l = 1,1 because?
    // ScnrVL.def - unknown purpose - looks like specular highlight of the
    //              above?! There is no similar thing for the Loss ones
    //TODO see if the index access is better alternative to string access
    Def spritev {Game::GetResource ("SCNRVICT.def")};
    s1 = spritev.Query ("ScnrVLbg.pcx");
    H3R_ENSURE(s1, "Sprite not found")
    bitmap = s1->ToRGBA ();
    H3R_ENSURE(bitmap, "Sprite->ToRGBA() failed")
    bmp_data = bitmap->operator byte * ();
    RE->UploadFrame (RE->GenKey (), 421 - spritev.Left (), 309 - spritev.Top (),
        spritev.Width (), spritev.Height (), bitmap_data,
        h3rBitmapFormat::RGBA, spritev.GetUniqueKey ("SCNRVICT.def"), Depth ());
    // Victory Condition: smalfont.fnt, White, 456,316
    H3R_CREATE_OBJECT(lbl, Label) {
        "Build a Grail structure or Defeat All Enemies", "smalfont.fnt",
        Point {456, 313}, this, H3R_TEXT_COLOR_WHITE};

    // "Loss Condition:": smalfont.fnt, Gold, 422,348
    H3R_CREATE_OBJECT(lbl, Label) {"Loss Condition:", "smalfont.fnt",
        Point {422, 345}, this, H3R_TEXT_COLOR_GOLD};
    //TODO Property (bitmap+text)
    // loss condition mini-icon: SCNRLOSS.def    (w:  29, h:  21)
    //  name[ 0]: "ScnrVLlt.pcx" - l town
    //  name[ 1]: "ScnrVLlh.pcx" - l hero
    //  name[ 2]: "ScnrVLtl.pcx" - time expires
    //  name[ 3]: "ScnrVLls.pcx" - l all
    // Again, they all have offset of t,l = 1,1 because?
    Def spritel {Game::GetResource ("SCNRLOSS.def")};
    s1 = spritel.Query ("ScnrVLls.pcx");
    H3R_ENSURE(s1, "Sprite not found")
    bitmap = s1->ToRGBA ();
    H3R_ENSURE(bitmap, "Sprite->ToRGBA() failed")
    bmp_data = bitmap->operator byte * ();
    RE->UploadFrame (RE->GenKey (), 422 - spritel.Left (), 366 - spritel.Top (),
        spritel.Width (), spritel.Height (), bitmap_data,
        h3rBitmapFormat::RGBA, spritev.GetUniqueKey ("SCNRLOSS.def"), Depth ());
    // Loss Condition: smalfont.fnt, White, 456,375
    H3R_CREATE_OBJECT(lbl, Label) {
        "Loose All Your Towns and Heroes", "smalfont.fnt",
        Point {456, 372}, this, H3R_TEXT_COLOR_WHITE};

    // "Allies:": smalfont.fnt, White, 420,409
    H3R_CREATE_OBJECT(lbl, Label) {"Allies:", "smalfont.fnt",
        Point {420, 406}, this, H3R_TEXT_COLOR_WHITE};
    //TODO Property
    // mini-static-flags: itgflags.def (w:  15, h:  20); 460, 405
    //  name[ 0]: "iTG0LtBl.pcx"
    //  name[ 1]: "iTG1Gren.pcx"
    //  name[ 2]: "iTG2Red.pcx"
    //  name[ 3]: "iTG3DkBl.pcx"
    //  name[ 4]: "iTG4Brwn.pcx"
    //  name[ 5]: "iTG5Prpl.pcx"
    //  name[ 6]: "iTG6Whit.pcx"
    //  name[ 7]: "iTG7Blak.pcx"
    Def spritef {Game::GetResource ("itgflags.def")};
    for (int i = 0; i < spritef.SpriteNum (0); i++) { // render all flags
        s1 = spritef.Query (0, i);
        H3R_ENSURE(s1, "Sprite not found")
        bitmap = s1->ToRGBA ();
        H3R_ENSURE(bitmap, "Sprite->ToRGBA() failed")
        bmp_data = bitmap->operator byte * ();
        RE->UploadFrame (RE->GenKey (), 460+ i*spritef.Width (), 405,
            spritef.Width (), spritef.Height (), bitmap_data,
            h3rBitmapFormat::RGBA, spritef.GetUniqueKey ("itgflags.def"),
            Depth ());
    }

    // "Enemies:": smalfont.fnt, White, 586,409
    H3R_CREATE_OBJECT(lbl, Label) {"Enemies:", "smalfont.fnt",
        Point {586, 406}, this, H3R_TEXT_COLOR_WHITE};
    //TODO Property
    //     7 max - the eight one is drawn over the border
    for (int i = 0; i < spritef.SpriteNum (0); i++) { // render all flags
        s1 = spritef.Query (0, i);
        H3R_ENSURE(s1, "Sprite not found")
        bitmap = s1->ToRGBA ();
        H3R_ENSURE(bitmap, "Sprite->ToRGBA() failed")
        bmp_data = bitmap->operator byte * ();
        RE->UploadFrame (RE->GenKey (), 640+ i*spritef.Width (), 405,
            spritef.Width (), spritef.Height (), bitmap_data,
            h3rBitmapFormat::RGBA, spritef.GetUniqueKey ("itgflags.def"),
            Depth ());
    }

    // "Map Diff:": smalfont.fnt, Gold, 429,439
    // Map Diff: the text is centered at top=473,x=416;500
    H3R_CREATE_OBJECT(lbl, Label) {"Map Diff:", "smalfont.fnt",
        Point {429, 436}, this, H3R_TEXT_COLOR_GOLD};
    //TODO Property
    H3R_CREATE_OBJECT(lbl, Label) {"Hard", "smalfont.fnt",
        Point {413, 454}, this, H3R_TEXT_COLOR_WHITE, false,
        Point {503-413, 502-454}};

    // "Player Difficulty:": smalfont.fnt, Gold, 529,439
    H3R_CREATE_OBJECT(lbl, Label) {"Player Difficulty:", "smalfont.fnt",
        Point {529, 436}, this, H3R_TEXT_COLOR_GOLD};
    // diff. buttons: GSPBUT[3;7].DEF - 0-4 (w:  30, h:  46)
    // Button group. New button behavior: the highlight sprite is used as
    // checked state - not on mouse hover. The group is done by handling
    // Checked.
    // Rating: 80, 100, 130, 160, 200 [%] - the text changes based on button
    //         clicked.
    //TODO Property
    for (int i = 0, j = 506; i < 5; i++, j+=32) {
        H3R_CREATE_OBJECT(btn, Button) {
            String::Format ("GSPBUT%d.DEF", i+3), this};
        btn->SetPos (j, 456);
        btn->UploadFrames ();
        btn->Checked.Changed.Subscribe (this, &NewGameDialog::BtnGroup);
        btn->Checkable = true;
        _btn_grp[i] = btn;
    }
    _btn_grp[1]->Checked = true; // 100% is the default

    // "Rating:": smalfont.fnt, Gold, 686,439
    // Rating: the text is centered at top=473,x=668;747
    H3R_CREATE_OBJECT(lbl, Label) {"Rating:", "smalfont.fnt",
        Point {686, 436}, this, H3R_TEXT_COLOR_GOLD};
    //TODO Property
    H3R_CREATE_OBJECT(lbl, Label) {"100%", "smalfont.fnt",
        Point {665, 454}, this, H3R_TEXT_COLOR_WHITE, false,
        Point {749-665, 502-454}};

    // "Show Advanced Options:" 443,513 (these appear to be centered at the long
    // buttons)
    H3R_CREATE_OBJECT(btn, Button) {
        "GSPBUTT.DEF", this, Button::H3R_UI_BTN_UPDN};
    btn->SetPos (414, 509);
    btn->UploadFrames ();
    btn->Click.Subscribe (this, &NewGameDialog::ToggleAdvOpt);
    btn->SetText ("Show Advanced Options", "smalfont.fnt",
        H3R_TEXT_COLOR_WHITE);

    // - another one: ScnrBeg.def   (w: 166, h:  40) ?!
    H3R_CREATE_OBJECT(btn, Button) {
        "ScnrBeg.def", this, Button::H3R_UI_BTN_UPDN};
    btn->SetPos (414, 535);
    btn->UploadFrames ();
    btn->Click.Subscribe (this, &NewGameDialog::Begin);

    // - button "back" : SCNRBACK.def (w: 166, h:  40) (n & s only)
    H3R_CREATE_OBJECT(btn, Button) {
        "SCNRBACK.def", this, Button::H3R_UI_BTN_UPDN};
    btn->SetPos (584, 535);
    btn->UploadFrames ();
    btn->Click.Subscribe (this, &NewGameDialog::Back);
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
    auto RE = Window::UI;

    if (! _tab_avail_scen) {
        H3R_CREATE_OBJECT(_tab_avail_scen, Control) {this};
        _tab_avail_scen->SetPos (3, 6);
        // _tab_avail_scen->Resize (tab_ascen.Width (), tab_ascen.Height ());
        // SCSelBck.pcx - 575 x 585

        static byte * bmp_data {};
        // static Array<byte> * bitmap {};
        auto bitmap_data = []() { return bmp_data; };

        //TODO helper or description?
        Pcx tab_ascen {Game::GetResource ("SCSelBck.pcx")};
        auto byte_arr_ptr = tab_ascen.ToRGBA ();
        if (! byte_arr_ptr || byte_arr_ptr->Empty ()) {
            H3R_NS::Log::Err ("Failed to load SCSelBck.pcx" EOL);
            return;
        }
        bmp_data = byte_arr_ptr->operator byte * ();
        _tab_avail_scen_key1 = RE->GenKey ();
        RE->UploadFrame (_tab_avail_scen_key1, _tab_avail_scen->Pos ().Left,
            _tab_avail_scen->Pos ().Top,
            tab_ascen.Width (), tab_ascen.Height (), bitmap_data,
            h3rBitmapFormat::RGBA, "SCSelBck.pcx", Depth ());
        _tab_avail_scen->SetHidden (! _tab_avail_scen->Hidden ());
    }
    //
    _tab_avail_scen->SetHidden (! _tab_avail_scen->Hidden ());
    RE->ChangeVisibility (_tab_avail_scen_key1, ! _tab_avail_scen->Hidden ());
}

void NewGameDialog::ToggleRndScen(EventArgs *)
{
}

void NewGameDialog::ToggleAdvOpt(EventArgs *)
{
}

void NewGameDialog::Begin(EventArgs *)
{
}

void NewGameDialog::Back(EventArgs *)
{
    _dr = DialogResult::Cancel; _has_dr = true;
}

void NewGameDialog::BtnGroup(EventArgs * e)
{
    static bool working {};
    if (working) return;
    working = true;
    H3R_ENSURE(nullptr != e, "EventArgs can't be null")
    auto args = static_cast<Button::ButtonEventArgs *>(e);
    H3R_ENSURE(nullptr != args, "EventArgs can't be null")
    H3R_ENSURE(nullptr != args->Sender, "Sender can't be null")
    for (int i = 0; i < 5; i++)
        if (_btn_grp[i])
            _btn_grp[i]->Checked = (_btn_grp[i] == args->Sender);
    working = false;
}

NAMESPACE_H3R
