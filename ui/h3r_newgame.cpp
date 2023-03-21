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
#include "h3r_gc.h"

H3R_NAMESPACE

NewGameDialog::NewGameDialog(Window * base_window)
    : DialogWindow {base_window, Point {370, 585}},
    _maps {}, _map_gate {},
    _scan_for_maps {"Maps", _maps, _map_gate, _map_list} // async
{
    H3R_ENSURE(Window::MainWindow != nullptr,
        "NewGameDialog requires MainWindow")
    _t = 6;
    _l = 396;

    auto RE = Window::UI;

    Pcx new_game_background {Game::GetResource ("gamselb1.pcx")};
    UploadFrame (RE->GenKey (), 0, 0, new_game_background, "gamselb1.pcx",
        Depth ());

    Pcx dlg_main {Game::GetResource ("GSelPop1.pcx")};
    UploadFrame (RE->GenKey (), _l, _t, dlg_main, "GSelPop1.pcx", Depth ());

    // By default if shows info about the 1st available scenario. The default
    // difficulty is normal: GSPBUT4.DEF.
    // Text Colors: Gold: 238,214,123; White: 255,255,255

    // "Scenario Name:" smallfont.fnt, Gold, 422,31
    Label * lbl {};
    H3R_CREATE_OBJECT(lbl, Label) {
        H3R_TEXT(GENRLTXT, H3R_GENRLTXT_SCENARIO_NAME), "smalfont.fnt",
        Point {422, 28}, this, H3R_TEXT_COLOR_GOLD};

    // Scenario Name: bigfont.fnt, Gold,     422,50
    H3R_CREATE_OBJECT(_lid_sname_lbl, Label) {"H3R", "bigfont.fnt",
        Point {422, 45}, this, H3R_TEXT_COLOR_GOLD};

    // Map Size Icon
    H3R_CREATE_OBJECT(_lid_map_size_sc, SpriteControl) {"ScnrMpSz.def", this,
        Point {714, 28}};
    _lid_map_size_sc->Map ("ScnrAllz.pcx", 0);
    _lid_map_size_sc->Map ("Scnr036z.pcx", 36);
    _lid_map_size_sc->Map ("Scnr072z.pcx", 72);
    _lid_map_size_sc->Map ("Scnr108z.pcx", 108);
    _lid_map_size_sc->Map ("Scnr144z.pcx", 144);

    // "Show Available Scenarios": smalfont.fnt, White, 440,85(up), 441,86(dn)
    Button * btn {};
    H3R_CREATE_OBJECT(btn, Button) {
        "GSPBUTT.DEF", this, Button::H3R_UI_BTN_UPDN};
    btn->SetPos (414, 81);
    btn->UploadFrames ();
    btn->Click.Subscribe (this, &NewGameDialog::ToggleAvailScen);
    btn->SetText (H3R_TEXT(GENRLTXT, H3R_GENRLTXT_SHOW_AVAIL_SCEN),
        "smalfont.fnt", H3R_TEXT_COLOR_WHITE);

    // "Random Map": smalfont.fnt, White, 475,109(up), 476,110(dn)
    H3R_CREATE_OBJECT(btn, Button) {
        "GSPBUTT.DEF", this, Button::H3R_UI_BTN_UPDN};
    btn->SetPos (414, 105);
    btn->UploadFrames ();
    btn->Click.Subscribe (this, &NewGameDialog::ToggleRndScen);
    btn->SetText (H3R_TEXT(GENRLTXT, H3R_GENRLTXT_RANDOM_MAP), "smalfont.fnt",
        H3R_TEXT_COLOR_WHITE);

    // "Scenario Description:": smalfont.fnt, Gold, 422,141
    H3R_CREATE_OBJECT(lbl, Label) {
        H3R_TEXT(GENRLTXT, H3R_GENRLTXT_SCENARIO_DESCR), "smalfont.fnt",
        Point {422, 138}, this, H3R_TEXT_COLOR_GOLD};

    // Scenario Description: smalfont.fnt, White, 422, 155; layout word-wrap
    // at x=740; after y=269. I won't reproduce the weird behavior of the
    // original, like word-wrapping with imaginary scroll-bar; and scroll-bar
    // jumping to max position on item switching.
    H3R_CREATE_OBJECT(_lid_sdescr_lbl, Label) {
        "A remake to be of the game engine of the well-known computer game "
        "\"Heroes of Might and Magic III\" and its official expansions.",
        "smalfont.fnt", Point {422, 155}, this, H3R_TEXT_COLOR_WHITE, true,
        Point {741-422, 270-155}};

    // "Victory Condition:": smalfont.fnt, Gold, 422,292
    H3R_CREATE_OBJECT(lbl, Label) {H3R_TEXT(GENRLTXT, H3R_GENRLTXT_VCON),
        "smalfont.fnt", Point {422, 289}, this, H3R_TEXT_COLOR_GOLD};
    // victory condition mini-icon: SCNRVICT.def (w:  29, h:  21):
    // ScnrVL.def - unknown purpose - looks like specular highlight?!
    H3R_CREATE_OBJECT(_lid_vcon_sc, SpriteControl) {"SCNRVICT.def", this,
        Point {420, 308}};
    _lid_vcon_sc->Map ("ScnrVLwn.pcx",  0); // "Defeat All Enemies"
    _lid_vcon_sc->Map ("ScnrVLaa.pcx",  1); // "Acquire Artifact"
    _lid_vcon_sc->Map ("ScnrVLac.pcx",  2); // "Accumulate Creatures"
    _lid_vcon_sc->Map ("ScnrVLar.pcx",  3); // "Accumulate Resources"
    _lid_vcon_sc->Map ("ScnrVLut.pcx",  4); // "Upgrade Town"
    _lid_vcon_sc->Map ("ScnrVLbg.pcx",  5); // "Build a Grail Structure"
    _lid_vcon_sc->Map ("ScnrVLdh.pcx",  6); // "Defeat Hero"
    _lid_vcon_sc->Map ("ScnrVLct.pcx",  7); // "Capture Town"
    _lid_vcon_sc->Map ("ScnrVLdm.pcx",  8); // "Defeat Monster"
    _lid_vcon_sc->Map ("ScnrVLfg.pcx",  9); // "Flag All Creature Dwellings"
    _lid_vcon_sc->Map ("ScnrVLfm.pcx", 10); // "Flag All Mines"
    _lid_vcon_sc->Map ("ScnrVLta.pcx", 11); // "Transport Artifact"
    // Victory Condition: smalfont.fnt, White, 456,316
    H3R_CREATE_OBJECT(_lid_vcon_lbl, Label) {H3R_TEXT(vcdesc, H3R_VCON_DEFAULT),
        "smalfont.fnt", Point {456, 313}, this, H3R_TEXT_COLOR_WHITE};

    // "Loss Condition:": smalfont.fnt, Gold, 422,348
    H3R_CREATE_OBJECT(lbl, Label) {H3R_TEXT(GENRLTXT, H3R_GENRLTXT_LCON),
        "smalfont.fnt", Point {422, 345}, this, H3R_TEXT_COLOR_GOLD};
    // loss condition mini-icon: SCNRLOSS.def    (w:  29, h:  21)
    H3R_CREATE_OBJECT(_lid_lcon_sc, SpriteControl) {"SCNRLOSS.def", this,
        Point {420, 365}};
    _lid_lcon_sc->Map ("ScnrVLls.pcx", 0); // "Lose All Your Towns and Heroes"
    _lid_lcon_sc->Map ("ScnrVLlt.pcx", 1); // "Lose Town"
    _lid_lcon_sc->Map ("ScnrVLlh.pcx", 2); // "Lose Hero"
    _lid_lcon_sc->Map ("ScnrVLtl.pcx", 3); // "Time Expires"
    // Loss Condition: smalfont.fnt, White, 456,375
    H3R_CREATE_OBJECT(_lid_lcon_lbl, Label) {H3R_TEXT(lcdesc, H3R_LCON_DEFAULT),
        "smalfont.fnt", Point {456, 372}, this, H3R_TEXT_COLOR_WHITE};

    // "Allies:": smalfont.fnt, White, 420,409
    //TODO R&D there is "Allies", but no "Allies:" at "GENRLTXT.TXT"
    H3R_CREATE_OBJECT(lbl, Label) {"Allies:", "smalfont.fnt",
        Point {420, 406}, this, H3R_TEXT_COLOR_WHITE};
    // mini-static-flags: itgflags.def (w:  15, h:  20); 460, 405
    // The names are misleading. The order matches the editor one matches
    // "PlColors.txt"; as usual the egg dilemma stands.
    //  name[ 0]: "iTG0LtBl.pcx" - red
    //  name[ 1]: "iTG1Gren.pcx" - blue
    //  name[ 2]: "iTG2Red.pcx"  - tan
    //  name[ 3]: "iTG3DkBl.pcx" - green
    //  name[ 4]: "iTG4Brwn.pcx" - orange
    //  name[ 5]: "iTG5Prpl.pcx" - purple
    //  name[ 6]: "iTG6Whit.pcx" - teal
    //  name[ 7]: "iTG7Blak.pcx" - pink
    Def spritef {Game::GetResource ("itgflags.def")};
    for (int i = 0; i < spritef.SpriteNum (0); i++) { // render all flags
        UploadFrame (
            //TODO this becomes an issue if the RE can't guarantee sequence
            (0 == i ? _lid_allies_base_key = RE->GenKey () : RE->GenKey ()),
            460 + i*spritef.Width (), 405, spritef,
            "itgflags.def", spritef.Query (0, i)->FrameName (), Depth ());
        RE->ChangeVisibility (_lid_allies_base_key+i, false);
    }

    // "Enemies:": smalfont.fnt, White, 586,409 ; same q as with "Allies:"
    H3R_CREATE_OBJECT(lbl, Label) {"Enemies:", "smalfont.fnt",
        Point {586, 406}, this, H3R_TEXT_COLOR_WHITE};
    // 7 max - the eight one is drawn over the border
    for (int i = 0; i < spritef.SpriteNum (0); i++) { // render all flags
        UploadFrame (
            (0 == i ? _lid_enemies_base_key = RE->GenKey () : RE->GenKey ()),
            640 + i*spritef.Width (), 405, spritef,
            "itgflags.def", spritef.Query (0, i)->FrameName (), Depth ());
        RE->ChangeVisibility (_lid_enemies_base_key+i, false);
    }

    // "Map Diff:": smalfont.fnt, Gold, 429,439
    // Map Diff: the text is centered at top=473,x=416;500
    H3R_CREATE_OBJECT(lbl, Label) {
        H3R_TEXT(GENRLTXT, H3R_GENRLTXT_MAP_DIFFICULTY), "smalfont.fnt",
        Point {429, 436}, this, H3R_TEXT_COLOR_GOLD};
    H3R_CREATE_OBJECT(_lid_diff_lbl, Label) {"Hard", "smalfont.fnt",
        Point {413, 454}, this, H3R_TEXT_COLOR_WHITE, false,
        Point {503-413, 502-454}};

    // "Player Difficulty:": smalfont.fnt, Gold, 529,439; as "Allies:"
    H3R_CREATE_OBJECT(lbl, Label) {"Player Difficulty:", "smalfont.fnt",
        Point {529, 436}, this, H3R_TEXT_COLOR_GOLD};
    // diff. buttons: GSPBUT[3;7].DEF - 0-4 (w:  30, h:  46)
    // Button group. New button behavior: the highlight sprite is used as
    // checked state - not on mouse hover. The group is done by handling
    // Checked.
    // Rating: 80, 100, 130, 160, 200 [%] - the text changes based on button
    //         clicked.
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

    // "Rating:": smalfont.fnt, Gold, 686,439; as "Allies:"
    // Rating: the text is centered at top=473,x=668;747
    H3R_CREATE_OBJECT(lbl, Label) {"Rating:", "smalfont.fnt",
        Point {686, 436}, this, H3R_TEXT_COLOR_GOLD};
    H3R_CREATE_OBJECT(_lid_rating_lbl, Label) {"100%", "smalfont.fnt",
        Point {665, 454}, this, H3R_TEXT_COLOR_WHITE, false,
        Point {749-665, 502-454}};

    // "Show Advanced Options:" 443,513 (these appear to be centered at the long
    // buttons)
    H3R_CREATE_OBJECT(btn, Button) {
        "GSPBUTT.DEF", this, Button::H3R_UI_BTN_UPDN};
    btn->SetPos (414, 509);
    btn->UploadFrames ();
    btn->Click.Subscribe (this, &NewGameDialog::ToggleAdvOpt);
    btn->SetText (H3R_TEXT(GENRLTXT, H3R_GENRLTXT_SHOW_ADV_OPT), "smalfont.fnt",
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

    Map * map {};
    {
        __pointless_verbosity::CriticalSection_Acquire_finally_release
            ___ {_map_gate};
        if (_maps.Count () > 0) map = _maps.FirstMap ();
    }
    SetListItem (map);
}// NewGameDialog::NewGameDialog()

NewGameDialog::~NewGameDialog()
{
    _scan_for_maps.Stop = true;
    while (! _scan_for_maps.Complete ())
        // The MainWindow could be unavailable, so just wait
        OS::Thread::SleepForAWhile ();

    for (int i = 0; i < _map_items.Count (); i++)
        H3R_DESTROY_NESTED_OBJECT(_map_items[i], NewGameDialog::ListItem,
            ListItem)
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

void NewGameDialog::ToggleAvailScen(EventArgs *)
{
    auto RE = Window::UI;

    if (! _tab_avail_scen) {
        H3R_CREATE_OBJECT(_tab_avail_scen, Control) {this};
        _tab_avail_scen->SetPos (3, 6);

        // _tab_avail_scen->Resize (tab_ascen.Width (), tab_ascen.Height ());
        // SCSelBck.pcx - 575 x 585
        //TODO helper or description?
        Pcx tab_ascen {Game::GetResource ("SCSelBck.pcx")};
        UploadFrame (_tab_avail_scen_keys.Add (RE->GenKey ()),
            _tab_avail_scen->Pos ().Left, _tab_avail_scen->Pos ().Top,
            tab_ascen, "SCSelBck.pcx", Depth ());

        // "Select a Scenario to Play" - MedFont.fnt, 114,28(center at
        // 24,22-392,46), gold
        Label * lbl {};
        H3R_CREATE_OBJECT(lbl, Label) {"Select a Scenario to Play",
            "MedFont.fnt", Point {24, 22}, _tab_avail_scen, H3R_TEXT_COLOR_GOLD,
            false, Point {392-24, 46-22}};

        // "Map Sizes" - smalfont.fnt, 60,63(c:24,52-157,84), gold
        H3R_CREATE_OBJECT(lbl, Label) {"Map Sizes", "smalfont.fnt",
            Point {24, 52}, _tab_avail_scen, H3R_TEXT_COLOR_GOLD,
            false, Point {157-24, 84-52}};

        // Column header buttons:
        Button * btn {};
        List<String> ch {6};
        int x = 26; // y=92
        ch.Put ("SCBUTT1.DEF").Put ("SCBUTT2.DEF").Put ("SCButCp.DEF")
          .Put ("SCBUTT3.DEF").Put ("SCBUTT4.DEF").Put ("SCBUTT5.DEF");
        for (auto & n : ch) {
            H3R_CREATE_OBJECT(btn, Button) {
                n, _tab_avail_scen, Button::H3R_UI_BTN_UPDN};
            btn->SetPos (x, 92);
            btn->UploadFrames ();
            x += btn->Width () + 1;
        }

        // filter by size buttons: SCALBUT.DEF
        //      - alt1: SC L GBUT.DEF ; alt2: SC M DBUT.DEF ?!
        //      - alt3: SC S MBUT.DEF; alt4: SC X LBUT.DEF
        x = 161; // y=52
        List<String> grp {5};
        grp.Put ("SCSMBUT.DEF").Put ("SCMDBUT.DEF").Put ("SCLGBUT.DEF")
           .Put ("SCXLBUT.DEF").Put ("SCALBUT.DEF");
        for (auto & n : grp) {
            H3R_CREATE_OBJECT(btn, Button) {
                n, _tab_avail_scen, Button::H3R_UI_BTN_UPDN};
            btn->SetPos (x, 52);
            btn->UploadFrames ();
            x += btn->Width () + 3;
        }

        // VSCrollBar
        H3R_CREATE_OBJECT(_tab_avail_scen_vs, ScrollBar) {
            _tab_avail_scen, Point {375, 92}, 572-92};
            // _tab_avail_scen_vs->Scroll.Subscribe (this, ?);
        _tab_avail_scen_vs->Min = 1; // just to be on the safe side
        _tab_avail_scen_vs->LargeStep = H3R_VISIBLE_LIST_ITEMS;
        _tab_avail_scen_vs->Scroll.Subscribe (this, &NewGameDialog::Scroll);

        int x1=24, x2=56, x3=91, x4=122, x5=309, x6=342, y=121, y2=123;
        NewGameDialog::ListItem * itm {};
        //TODO this is causing OnRender due to the threaded IO
        // The idea is to create them all and hide/show according to
        // _maps.Count ()
        for (int i = 0; i < H3R_VISIBLE_LIST_ITEMS; i++) {
            H3R_CREATE_OBJECT(itm, NewGameDialog::ListItem) {
                _tab_avail_scen,
                Point {x1,  y}, Point {x2,  y}, Point {x4,  y},// text (1, 2, 4)
                Point {x3, y2}, Point {x5, y2}, Point {x6, y2} // icon (3, 5, 6)
            };
            _map_items.Add (itm);
            y+=25, y2+=25;
        }

        // hide all
        _tab_avail_scen->SetHidden (! _tab_avail_scen->Hidden ());
    }
    //
    _tab_avail_scen->SetHidden (! _tab_avail_scen->Hidden ());
    for (auto k : _tab_avail_scen_keys)
        RE->ChangeVisibility (k, ! _tab_avail_scen->Hidden ());
}// NewGameDialog::ToggleAvailScen()

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
    static const char * rating[5] = {"80%", "100%", "130%", "160%", "200%"};
    if (working) return;
    working = true;
    H3R_ENSURE(nullptr != e, "EventArgs can't be null")
    auto args = static_cast<Button::ButtonEventArgs *>(e);
    H3R_ENSURE(nullptr != args, "EventArgs can't be null")
    H3R_ENSURE(nullptr != args->Sender, "Sender can't be null")
    for (int i = 0; i < 5; i++)
        if (_btn_grp[i]) {
            _btn_grp[i]->Checked = (_btn_grp[i] == args->Sender);
            if (_btn_grp[i]->Checked && nullptr != _lid_rating_lbl)
                _lid_rating_lbl->SetText (rating[i]);
        }
    working = false;
}

void NewGameDialog::OnRender()
{
    Window::OnRender ();

    if (nullptr == _tab_avail_scen_vs) return;
    if (_maps.Count () <= 0 ) {
        _tab_avail_scen_vs->SetHidden (true);
        return;
    }
    if (_tab_avail_scen->Hidden ()) return;
    __pointless_verbosity::CriticalSection_Acquire_finally_release
        ___ {_map_gate};
    /*printf ("Min: %d, Max: %d, cnt: %d\n", (int)_tab_avail_scen_vs->Min,
        (int)_tab_avail_scen_vs->Max, _maps.Count ());*/
    if (_tab_avail_scen_vs->Max != _maps.Count ()) { // update in real-time
        if (_maps.Count () > H3R_VISIBLE_LIST_ITEMS) {
            _tab_avail_scen_vs->SetHidden (false);
            _tab_avail_scen_vs->Max =
                _maps.Count () - H3R_VISIBLE_LIST_ITEMS + 1;
            // it clamps to Max-Min: no worries
            _tab_avail_scen_vs->LargeStep = H3R_VISIBLE_LIST_ITEMS;
        }
        else // no scroll
            _tab_avail_scen_vs->SetHidden (true);
    }
    if (_map_items.Count () < H3R_VISIBLE_LIST_ITEMS)
        return; // aren't created yet
    Model2View ();
}// NewGameDialog::OnRender()

void NewGameDialog::ListItem::SetMap(class Map * map, bool selected)
{
    if (nullptr == map) {
        SetDefaults ();
        SetHidden (true);
        return;
    }

    if (Selected != selected) { // transient
        auto color = selected ? H3R_TEXT_COLOR_GOLD : H3R_TEXT_COLOR_WHITE;
        Players->SetColor (color);
        Size->SetColor (color);
        Name->SetColor (color);
        Selected = selected;
    }

    if (this->Map == map) return;
    this->Map = map;

    SetHidden (false); // SpiteControl denies changes when hidden, so
    Players->SetText (
        String::Format ("%d/%d", map->PlayerNum (), map->HumanPlayers ()));
    Size->SetText (map->SizeName ());
    Version->Show (map->Version ());
    // The original game acts this way:
    Name->SetText (map->Name ().Empty () ? "Unnamed" : map->Name ());
    Victory->Show (map->VCon ());
    Loss->Show (map->LCon ());
    VConText = map->VConText ();
    LConText = map->LConText ();
}// NewGameDialog::ListItem::SetMap()

void NewGameDialog::LidSetFlags(Map * map) //TODO StackedSpritesControl
{
    if (nullptr == map) {
        for (int i = 0; i < 8; i++) {//TODO 8 is "PlColors.txt" - based
            Window::UI->ChangeVisibility (_lid_allies_base_key+i, false);
            Window::UI->ChangeVisibility (_lid_enemies_base_key+i, false);
        }
        return;
    }
    // advanced options is not done yet so
     //TODO param at adv_opt; "PlColors.txt"-0-based
    int selected_player = map->FirstHumanPlayer ();
    H3R_ENSURE(selected_player >= 0 && selected_player < 8,
        "Can't handle AI-only maps yet")
    int tk = map->Teams ().Count () <= 0 ? -1 : map->Teams ()[selected_player];
    int sprite_w = 15, y = 405, ax = 460, ex = 640;
    auto my_team = [&](int i) { return tk != -1 && map->Teams ()[i] == tk; };
    for (int i = 0, a=0, e=0; i < 8; i++) // 8 - h3m_ffd and "PlColors.txt"
        if (! map->PlayerAt (i).CanPlay ()) {
            Window::UI->ChangeVisibility (_lid_allies_base_key+i, false);
            Window::UI->ChangeVisibility (_lid_enemies_base_key+i, false);
        }
        else if (i == selected_player
            || (i != selected_player && my_team (i))) {
            Window::UI->SetLocation (
                _lid_allies_base_key+i, ax+(a++)*sprite_w, y);
            Window::UI->ChangeVisibility (_lid_allies_base_key+i, true);
            Window::UI->ChangeVisibility (_lid_enemies_base_key+i, false);
        }
        else {
            Window::UI->SetLocation (
                _lid_enemies_base_key+i, ex+(e++)*sprite_w, y);
            Window::UI->ChangeVisibility (_lid_enemies_base_key+i, true);
            Window::UI->ChangeVisibility (_lid_allies_base_key+i, false);
        }
}// NewGameDialog::LidSetFlags()

void NewGameDialog::SetListItem(ListItem * itm)
{
    if (nullptr != itm) {//TODO these just scream: Property<T>
        if (itm->Map == _lid_map) return;
        _lid_map = itm->Map;
    }

    if (nullptr == itm || nullptr == itm->Map) {// original
        _lid_sname_lbl->SetText ("-"); //       vs ""
        _lid_map_size_sc->Show (0);    // "All" vs "S"
        _lid_sdescr_lbl->SetText ("-");//       vs ""
        _lid_vcon_sc->Show (0);        //       vs nothing
        _lid_vcon_lbl->SetText ("-");  //       vs ""
        _lid_lcon_sc->Show (0);        //       vs nothing
        _lid_lcon_lbl->SetText ("-");  //       vs ""
        _lid_diff_lbl->SetText ("-");  //       vs ""
        _btn_grp[1]->Checked = true;   // 100%  vs ""
        _lid_rating_lbl->SetText ("100%");//    vs ""
        LidSetFlags (nullptr);
    }
    else {
        _lid_sname_lbl->SetText  (itm->Map->Name ());
        _lid_map_size_sc->Show   (itm->Map->Size ());
        _lid_sdescr_lbl->SetText (itm->Map->Descr ());
        _lid_vcon_sc->Show       (itm->Map->VCon ());
        _lid_vcon_lbl->SetText   (itm->VConText);
        _lid_lcon_sc->Show       (itm->Map->LCon ());
        _lid_lcon_lbl->SetText   (itm->LConText);
        _lid_diff_lbl->SetText   (itm->Map->DifficultyName ());
        LidSetFlags (itm->Map);
    }
}// NewGameDialog::SetListItem()

void NewGameDialog::SetListItem(Map * map)
{
    if (map == _lid_map) return;//TODO Property<T>
    _lid_map = map;

    if (nullptr == map) SetListItem(static_cast<ListItem *>(nullptr));
    else {
        _lid_sname_lbl->SetText  (map->Name ());
        _lid_map_size_sc->Show   (map->Size ());
        _lid_sdescr_lbl->SetText (map->Descr ());
        _lid_vcon_sc->Show       (map->VCon ());
        _lid_vcon_lbl->SetText   (map->VConText ());
        _lid_lcon_sc->Show       (map->LCon ());
        _lid_lcon_lbl->SetText   (map->LConText ());
        _lid_diff_lbl->SetText   (map->DifficultyName ());
        LidSetFlags (map);
    }
}// NewGameDialog::SetListItem()

NewGameDialog::MapListInit::MapListInit(String p, MapList & l,
    OS::CriticalSection & lg, Array<Map *> & list)
    : MList{l}, MapListGate{lg}, MList2{list}, _subject{
//np base_path: p, observer: this, handle_on_item: &MapListInit::HandleItem
    p, this, &MapListInit::HandleItem, &MapListInit::Done} {}

void NewGameDialog::OnMouseDown(const EventArgs & e)
{
    DialogWindow::OnMouseDown (e);

    //LATER stop ignoring modifiers
    // printf ("Mouse down at: %d, %d\n", e.X, e.Y);

    __pointless_verbosity::CriticalSection_Acquire_finally_release
        ___ {_map_gate};

    // ListItem
    int l=25, r=373, t=123, h=25;
    int row = e.X >= l && e.X <= r && e.Y >= 122 && e.Y <= 572 ? (e.Y-t)/h : -1;
    if (row >= 0 && row <= _map_items.Count ()
        // handle avail maps < list-items
        && nullptr != _map_items[row]->Map) {
        _ml_selected = _ml_top + row;
        _ml_selected_map = _map_items[row]->Map;
    }
}

void NewGameDialog::Scroll(EventArgs *)
{
    //TODO ui:timer - to prevent too many events/sec
    if (_map_list.Length () <= 0) return;
    //TODO appropriate function at the tab-control (when its ready)
    if (_tab_avail_scen_vs->Hidden ()) return; // no scroll
    // the scroll-bar is set-up to match the _map_list
    _ml_top = _tab_avail_scen_vs->Pos - _tab_avail_scen_vs->Min;
}

void NewGameDialog::Model2View() // ensure _map_gate is acquired
{
    for (int i = 0, j = _ml_top; i < _map_items.Count (); i++, j++)
        _map_items[i]->SetMap (
            j < _map_list.Length () ? _map_list[j] : nullptr,
            j == _ml_selected);
    if (_map_list.Length () > 0) {
        if (nullptr != _ml_selected_map
            && _ml_selected_map != _map_list[_ml_selected]
            && ! _scan_for_maps.Complete ())
            // the user interfered during a background scan and the sort()
            // has changed the _ml_selected meaning
            for (int i = 0; i < _map_list.Length (); i++)
                if (_map_list[i] == _lid_map) { _ml_selected = i; break; }
        SetListItem (_map_list[_ml_selected]);
    }
}

void NewGameDialog::OnKeyDown(const EventArgs & e)
{
    static EventArgs kbd {};
    DialogWindow::OnKeyDown (e);

    if (nullptr == _tab_avail_scen) return;
    if (nullptr == _tab_avail_scen_vs) return;
    if (_tab_avail_scen->Hidden ()) return;

    __pointless_verbosity::CriticalSection_Acquire_finally_release
        ___ {_map_gate};

    switch (e.Key) {
        case H3R_KEY_ARROW_DN: {
            if (_map_list.Length ()-1 == _ml_selected) break;
            _ml_selected++;
            if (_ml_selected - _ml_top >= H3R_VISIBLE_LIST_ITEMS) {
                _tab_avail_scen_vs->Pos = _tab_avail_scen_vs->Pos + 1;
                Scroll (&kbd);
            }
        } break;
        case H3R_KEY_ARROW_UP: {
            if (0 == _ml_selected) break;
            _ml_selected--;
            if (_ml_selected < _ml_top) {
                _tab_avail_scen_vs->Pos = _tab_avail_scen_vs->Pos - 1;
                Scroll (&kbd);
            }
        } break;
        // original bug: the key goes to the description scroll-bar as well
        case H3R_KEY_PGDN: {
            // I'm having hard time figuring out the logic of the original.
            int ns = _ml_selected + H3R_VISIBLE_LIST_ITEMS-1;
            if (ns >= _map_list.Length ()) break;
            // anchor bottom
            int ds = ns - _ml_selected; // delta-selected
            int nt = _ml_top + ds;
            if (nt > _map_list.Length ()-H3R_VISIBLE_LIST_ITEMS) {
                int kt = nt - (_map_list.Length ()-H3R_VISIBLE_LIST_ITEMS);
                nt = _map_list.Length ()-H3R_VISIBLE_LIST_ITEMS;
                if (nt < 0) nt = 0;
                if (_ml_selected == _ml_top)
                    _ml_selected = nt;      // one time: do this;
                else _ml_selected = ns-kt ; // the next time: ...; the point?!
            }
            else
                _ml_selected = ns;
            _tab_avail_scen_vs->Pos = _tab_avail_scen_vs->Min + (_ml_top = nt);
        } break;
        // original bug: the key goes to the description scroll-bar as well
        case H3R_KEY_PGUP: {//TODO the same odd behavior as H3R_KEY_PGDN
            //TODO arrow down until 1 line scrolls down; page-up; play the odd
            //     game again;
            if (_ml_selected < H3R_VISIBLE_LIST_ITEMS && 0 == _ml_top) break;
            int np = _ml_selected - H3R_VISIBLE_LIST_ITEMS+1;
            if (np < 0) np = 0;
            int dp = _ml_selected - np;
            _ml_selected = np;
            _tab_avail_scen_vs->Pos = _tab_avail_scen_vs->Pos - dp;
            Scroll (&kbd);
        } break;
        default: return;
    }// switch (e.Key)
    // focus - a.k.a. - make the selection visible if it went off-screen
    if (_ml_selected < _ml_top ||
        _ml_selected >= _ml_top + H3R_VISIBLE_LIST_ITEMS)
        _ml_top = _ml_selected;
    if (_ml_top > _map_list.Length ()-H3R_VISIBLE_LIST_ITEMS)
        _ml_top = _map_list.Length ()-H3R_VISIBLE_LIST_ITEMS;
    if (_ml_top < 0) _ml_top = 0;
    _tab_avail_scen_vs->Pos = _tab_avail_scen_vs->Min + _ml_top;

    _ml_selected_map = _map_items[_ml_selected-_ml_top]->Map;
}// NewGameDialog::OnKeyDown()

NAMESPACE_H3R
